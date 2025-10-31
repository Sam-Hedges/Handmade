#include <PlatformLayer.h>

/* ========================================================================
   Variables
   ======================================================================== */

global_var uint32 SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT = 0;

/* ========================================================================
   Structures
   ======================================================================== */

struct platform_audio_config
{
	int ToneHz;
	int ToneVolume;
	int WavePeriod;
	int SampleIndex;
	int SamplesPerSecond;
	int BytesPerSample;
};

struct platform_audio_buffer
{
	void *Memory;
	int Size;
	int ReadCursor;
	int WriteCursor;
	SDL_AudioDeviceID DeviceID;
	platform_audio_config *AudioConfig;
};

struct platform_audio_thread_context
{
	platform_audio_buffer *AudioBuffer;
	platform_program_state *ProgramState;
};

/* ========================================================================
   Functions
   ======================================================================== */

internal void InitAudioBuffer(platform_audio_buffer *Buffer, platform_audio_config *Config)
{
	// TODO(Sam): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if(Buffer->Memory)
	{
		SDL_free(Buffer->Memory);
	}

	Buffer->Size = Config->SamplesPerSecond * Config->BytesPerSample;
	// Two data points per sample. One for the left speaker, one for the right.
	Buffer->ReadCursor = 0;
	// NOTE(Sam): Offset by 1 sample in order to cause the circular buffer to
	// initially be filled.
	Buffer->WriteCursor = Config->BytesPerSample;
	Buffer->AudioConfig = Config;

	Buffer->Memory = SDL_malloc(Buffer->Size);
}

internal Sint16 SampleSquareWave(platform_audio_config *AudioConfig)
{
	int HalfSquareWaveCounter = AudioConfig->WavePeriod / 2;
	if((AudioConfig->SampleIndex / HalfSquareWaveCounter) % 2 == 0)
	{
		return AudioConfig->ToneVolume;
	}

	return -AudioConfig->ToneVolume;
}

///////////////////////////////////////////////////////////////////////////////

internal Sint16 SampleSineWave(platform_audio_config *AudioConfig)
{
	int HalfWaveCounter = AudioConfig->WavePeriod / 2;
	return AudioConfig->ToneVolume * sin(TAU * AudioConfig->SampleIndex / HalfWaveCounter);
}

///////////////////////////////////////////////////////////////////////////////

internal void SampleIntoAudioBuffer(platform_audio_buffer *AudioBuffer,
									Sint16 (*GetSample)(platform_audio_config *))
{
	int Region1Size = AudioBuffer->ReadCursor - AudioBuffer->WriteCursor;
	int Region2Size = 0;
	if(AudioBuffer->ReadCursor < AudioBuffer->WriteCursor)
	{
		// Fill to the end of the buffer and loop back around and fill to the read
		// cursor.
		Region1Size = AudioBuffer->Size - AudioBuffer->WriteCursor;
		Region2Size = AudioBuffer->ReadCursor;
	}

	platform_audio_config *AudioConfig = AudioBuffer->AudioConfig;

	int Region1Samples = Region1Size / AudioConfig->BytesPerSample;
	int Region2Samples = Region2Size / AudioConfig->BytesPerSample;
	int BytesWritten   = Region1Size + Region2Size;

	Sint16 *Buffer = (Sint16 *)&AudioBuffer->Buffer[AudioBuffer->WriteCursor];
	for(int SampleIndex = 0; SampleIndex < Region1Samples; SampleIndex++)
	{
		Sint16 SampleValue = (*GetSample)(AudioConfig);
		*Buffer++		   = SampleValue;
		*Buffer++		   = SampleValue;
		AudioConfig->SampleIndex++;
	}

	Buffer = (Sint16 *)AudioBuffer->Memory;
	for(int SampleIndex = 0; SampleIndex < Region2Samples; SampleIndex++)
	{
		Sint16 SampleValue = (*GetSample)(AudioConfig);
		*Buffer++		   = SampleValue;
		*Buffer++		   = SampleValue;
		AudioConfig->SampleIndex++;
	}

	AudioBuffer->WriteCursor = (AudioBuffer->WriteCursor + BytesWritten) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void PlatformFillAudioDeviceBuffer(void *UserData, Uint8 *DeviceBuffer, int Length)
{
	platform_audio_buffer *AudioBuffer = (platform_audio_buffer *)UserData;

	// Keep track of two regions. Region1 contains everything from the current
	// PlayCursor up until, potentially, the end of the buffer. Region2 only
	// exists if we need to circle back around. It contains all the data from the
	// beginning of the buffer up until sufficient bytes are read to meet Length.
	int Region1Size = Length;
	int Region2Size = 0;
	if(AudioBuffer->ReadCursor + Length > AudioBuffer->Size)
	{
		// Handle looping back from the beginning.
		Region1Size = AudioBuffer->Size - AudioBuffer->ReadCursor;
		Region2Size = Length - Region1Size;
	}

	SDL_memcpy(DeviceBuffer, (AudioBuffer->Memory + AudioBuffer->ReadCursor), Region1Size);
	SDL_memcpy(&DeviceBuffer[Region1Size], AudioBuffer->Memory, Region2Size);

	AudioBuffer->ReadCursor = (AudioBuffer->ReadCursor + Length) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void PlatformInitializeAudio(platform_audio_buffer *AudioBuffer)
{
	SDL_AudioSpec AudioSettings = {};
	AudioSettings.freq			= AudioBuffer->AudioConfig->SamplesPerSecond;
	AudioSettings.format		= SDL_AUDIO_S16LE;
	AudioSettings.channels		= 2;

	SDL_AudioSpec ObtainedSettings = {};
	AudioBuffer->DeviceID		   = SDL_OpenAudioDevice(NULL, 0, &AudioSettings, &ObtainedSettings,
														 SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT);

	if(AudioSettings.format != ObtainedSettings.format)
	{
		SDL_Log("Unable to obtain expected audio settings: %s", SDL_GetError());
		exit(1);
	}

	// Start playing the audio buffer
	SDL_PauseAudioDevice(AudioBuffer->DeviceID, 0);
}

///////////////////////////////////////////////////////////////////////////////

internal int PlatformAudioThread(void *UserData)
{
	platform_audio_thread_context *AudioThread = (platform_audio_thread_context *)UserData;

	while(AudioThread->ProgramState->IsRunning)
	{
		SDL_LockAudioDevice(AudioThread->AudioBuffer->DeviceID);
		SampleIntoAudioBuffer(AudioThread->AudioBuffer, &SampleSineWave);
		SDL_UnlockAudioDevice(AudioThread->AudioBuffer->DeviceID);
	}

	return 0;
}
