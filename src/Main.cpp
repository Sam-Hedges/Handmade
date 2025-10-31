#include <PlatformLayer.h>

#include "Audio.cpp"
#include "Rendering.cpp"

/* ========================================================================
   Variables
   ======================================================================== */

// TODO(Sam): This is a global for now.
// TODO(Sam): Figure out if I want to keep this "platform" prefix or if this is dumb
global_var platform_program_state ProgramState = {};
global_var platform_bitmap_buffer BackBuffer   = {};
global_var platform_audio_buffer AudioBuffer   = {};
global_var platform_audio_config AudioConfig   = {};

global_var SDL_Window *Window;
global_var SDL_Renderer *Renderer;
global_var SDL_Gamepad *Gamepad;
global_var SDL_AudioStream *AudioStream;

/* ========================================================================
   Application
   ======================================================================== */

internal void PlatformProcessSDLEvent(platform_program_state *ProgramState)
{
	const SDL_Event *Event = &ProgramState->LastEvent;

	switch(Event->type)
	{
		case SDL_EVENT_QUIT:
		{
			ProgramState->IsRunning = false;
		}
		break;
		case SDL_EVENT_KEY_DOWN:
		{
			SDL_Scancode key = Event->key.scancode;
			if(key == SDL_SCANCODE_ESCAPE)
			{
				ProgramState->IsRunning = false;
			}

			SDL_Log("Key: %s DOWN", SDL_GetScancodeName(key));
		}
		break;
		case SDL_EVENT_KEY_UP:
		{
			SDL_Log("Key: %s UP", SDL_GetScancodeName(Event->key.scancode));
		}
		break;
		case SDL_EVENT_GAMEPAD_ADDED:
		{
			if(!Gamepad)
			{
				Gamepad = SDL_OpenGamepad(Event->gdevice.which);
				// TODO(Sam): Look at gamepad polling example a check for error when loading
				// gamepad
			}
		}
		break;
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			if(Gamepad && (SDL_GetGamepadID(Gamepad) == Event->gdevice.which))
			{
				SDL_CloseGamepad(Gamepad);
				Gamepad = 0;
			}
		}
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////

// TODO(Sam): Look into the SDL macro that redefines this to a custom SDL_main
int main(int Argc, char **Argv)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
	SDL_SetAppMetadata("Handmade Game", "1.0", "com.game.handmade");

	int Width  = 1280;
	int Height = 720;
	Window	   = SDL_CreateWindow("Handmade Game", Width, Height, SDL_WINDOW_RESIZABLE);

	Renderer = SDL_CreateRenderer(Window, NULL);

	ResizeBitmap(&BackBuffer, Width, Height);
	SDL_Texture *Texture =
		SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING,
						  BackBuffer.Width, BackBuffer.Height);

	// TODO(Sam): Figure out the application layout because where should these actually be defined
	// probably not just randomly since I have an Init func to setup the buffer.
	AudioConfig.SamplesPerSecond = 44100;
	AudioConfig.BytesPerSample	 = 2 * sizeof(int16);
	AudioConfig.SampleIndex		 = 0;
	AudioConfig.ToneHz			 = 256;
	AudioConfig.ToneVolume		 = 3000;
	AudioConfig.WavePeriod		 = AudioConfig.SamplesPerSecond / AudioConfig.ToneHz;

	InitAudioBuffer(&AudioBuffer, &AudioConfig);
	PlatformInitializeAudio(&AudioBuffer);

	platform_audio_thread_context AudioThreadContext = {};
	AudioThreadContext.AudioBuffer					 = &AudioBuffer;
	AudioThreadContext.ProgramState					 = &ProgramState;
	SDL_Thread *AudioThread =
		SDL_CreateThread(PlatformAudioThread, "Audio", (void *)&AudioThreadContext);

	int XOffset = 0;
	int YOffset = 0;

	ProgramState.IsRunning = true;
	while(ProgramState.IsRunning)
	{
		while(SDL_PollEvent(&ProgramState.LastEvent))
		{
			PlatformProcessSDLEvent(&ProgramState);
		}

		// -----------------------------
		// GAME UPDATE
		// -----------------------------
		const bool *Keys = SDL_GetKeyboardState(NULL);
		if(Keys[SDL_SCANCODE_UP])
			YOffset++;
		if(Keys[SDL_SCANCODE_DOWN])
			YOffset--;
		if(Keys[SDL_SCANCODE_LEFT])
			XOffset++;
		if(Keys[SDL_SCANCODE_RIGHT])
			XOffset--;

		if(Gamepad)
		{
			int16 LeftAxisX = SDL_GetGamepadAxis(Gamepad, SDL_GAMEPAD_AXIS_LEFTX);
			int16 LeftAxisY = SDL_GetGamepadAxis(Gamepad, SDL_GAMEPAD_AXIS_LEFTY);
			XOffset += (-LeftAxisX >> 12) / 2;
			YOffset += (-LeftAxisY >> 12) / 2;

			bool LeftShoulder  = SDL_GetGamepadButton(Gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
			bool RightShoulder = SDL_GetGamepadButton(Gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
			if(LeftShoulder)
			{
				SDL_RumbleGamepad(Gamepad, 60000, 0, 100);
			}
			if(RightShoulder)
			{
				SDL_RumbleGamepad(Gamepad, 0, 60000, 100);
			}
			if(!LeftShoulder && !RightShoulder)
			{
				SDL_RumbleGamepad(Gamepad, 0, 0, 0);
			}
		}

		// -----------------------------
		// RENDER TO SCREEN
		// -----------------------------
		SDL_RenderClear(Renderer);

		SDL_GetWindowSize(Window, &Width, &Height);
		ResizeBitmap(&BackBuffer, Width, Height);
		RenderGradientUV(&BackBuffer, XOffset, YOffset);
		SDL_UpdateTexture(Texture, NULL, BackBuffer.Memory, BackBuffer.Pitch);

		SDL_FRect Destination = {0, 0, (float)Width, (float)Height};
		SDL_RenderTexture(Renderer, Texture, NULL, &Destination);

		SDL_RenderPresent(Renderer);
	}

	// TODO(Sam): Check if I need to free the audio buffer memory and the bitmap buffer memory

	SDL_WaitThread(AudioThread, NULL);

	SDL_CloseAudioDevice(AudioBuffer.DeviceID);
	SDL_DestroyTexture(Texture);
	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return 0;
}
