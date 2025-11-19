#include "SDL3/SDL_audio.h"
#include <PlatformLayer.h>

/* ========================================================================
   Variables
   ======================================================================== */

global_var uint32 SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT = 0;

/* ========================================================================
   Structures
   ======================================================================== */

// TODO(Sam): Redo this entire audio buisness at the sdl audio api has changed from version 2 to 3
// https://wiki.libsdl.org/SDL3/README-migration search this for audio stuff and also readup on the
// sdl_audiostream callbacks as I'm not sure we even need a circular buffer any more

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
	SDL_AudioStream *AudioStream;
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

internal int16 SampleSquareWave(platform_audio_config *AudioConfig)
{
	int HalfSquareWaveCounter = AudioConfig->WavePeriod / 2;
	if((AudioConfig->SampleIndex / HalfSquareWaveCounter) % 2 == 0)
	{
		return AudioConfig->ToneVolume;
	}

	return -AudioConfig->ToneVolume;
}

///////////////////////////////////////////////////////////////////////////////

internal int16 SampleSineWave(platform_audio_config *AudioConfig)
{
	int HalfWaveCounter = AudioConfig->WavePeriod / 2;
	return AudioConfig->ToneVolume * sin(TAU * AudioConfig->SampleIndex / HalfWaveCounter);
}

///////////////////////////////////////////////////////////////////////////////

internal void SampleIntoAudioBuffer(platform_audio_buffer *AudioBuffer,
									int16 (*GetSample)(platform_audio_config *))
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

	// TODO(Sam): Fix error, the original code the memory was a uint8 pointer but now it is void
	int16 *Buffer = (int16 *)&((uint8 *)AudioBuffer->Memory)[AudioBuffer->WriteCursor];
	for(int SampleIndex = 0; SampleIndex < Region1Samples; SampleIndex++)
	{
		int16 SampleValue = (*GetSample)(AudioConfig);
		*Buffer++		  = SampleValue;
		*Buffer++		  = SampleValue;
		AudioConfig->SampleIndex++;
	}

	Buffer = (int16 *)AudioBuffer->Memory;
	for(int SampleIndex = 0; SampleIndex < Region2Samples; SampleIndex++)
	{
		int16 SampleValue = (*GetSample)(AudioConfig);
		*Buffer++		  = SampleValue;
		*Buffer++		  = SampleValue;
		AudioConfig->SampleIndex++;
	}

	AudioBuffer->WriteCursor = (AudioBuffer->WriteCursor + BytesWritten) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void PlatformFillAudioDeviceBuffer(void *UserData, uint8 *DeviceBuffer, int Length)
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

	SDL_memcpy(DeviceBuffer, ((uint8 *)AudioBuffer->Memory + AudioBuffer->ReadCursor), Region1Size);
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

	// SDL_AudioSpec ObtainedSettings = {};
	// AudioBuffer->DeviceID		   = SDL_OpenAudioDevice(NULL, 0, &AudioSettings,
	// &ObtainedSettings,
	// SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT);

	AudioBuffer->DeviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &AudioSettings);

	SDL_AudioStream *AudioStream = SDL_CreateAudioStream(&AudioSettings, &AudioSettings);
	SDL_PutAudioStreamData();

	// if(AudioSettings.format != ObtainedSettings.format)
	// {
	// 	SDL_Log("Unable to obtain expected audio settings: %s", SDL_GetError());
	// 	// TODO(Sam): Not sure if I like this exit() way of exiting the program
	// 	exit(1);
	// }

	// Start playing the audio buffer
	SDL_ResumeAudioDevice(AudioBuffer->DeviceID);
}

///////////////////////////////////////////////////////////////////////////////

internal int PlatformAudioThread(void *UserData)
{
	platform_audio_thread_context *AudioThread = (platform_audio_thread_context *)UserData;

	while(AudioThread->ProgramState->IsRunning)
	{
		// SDL_SLockAudioDevice(AudioThread->AudioBuffer->DeviceID);
		SampleIntoAudioBuffer(AudioThread->AudioBuffer, &SampleSquareWave);
		// SDL_UnlockAudioDevice(AudioThread->AudioBuffer->DeviceID);

		// SDL_LockAudioDevice() and SDL_UnlockAudioDevice() have been removed, since there is no
		// callback in another thread to protect. SDL's audio subsystem and SDL_AudioStream maintain
		// their own locks internally, so audio streams are safe to use from any thread. If the app
		// assigns a callback to a specific stream, it can use the stream's lock through
		// SDL_LockAudioStream() if necessary.
	}

	return 0;
}
