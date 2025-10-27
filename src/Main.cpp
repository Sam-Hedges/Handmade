#include "SDL3/SDL_events.h"
#include <SDL3/SDL.h>

typedef Uint8 uint8;
typedef Uint16 uint16;
typedef Uint32 uint32;
typedef Uint64 uint64;

typedef Sint8 int8;
typedef Sint16 int16;
typedef Sint32 int32;
typedef Sint64 int64;

#define internal static
#define local_persist static
#define global_var static

struct bitmap_buffer
{
	// NOTE(Sam): Pixels are always 32-bits wide, Memory Order BB GG RR xx
	void *Memory;
	int Width;
	int Height;
	int Pitch; // How many bytes a pointer has to move in order to go from one row to the next row
};

struct window_dimension
{
	int Width;
	int Height;
};

// TODO(Sam): This is a global for now.
global_var bool GlobalRunning;
global_var bitmap_buffer GlobalBackBuffer;

internal void RenderGradientUV(bitmap_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	for(int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = 0; X < Buffer->Width; ++X)
		{
			// The reason we get a checkerbox is because by casting to an unsigned 8 bit integer /
			// char we are capping the maximum value to 255. This means that the current row /
			// collumn in the bitmap's low order byte will be taken as the unsigned 8 bit integer
			// and this is constantly wrapping as the width and height of the bitmap exceed 255.
			uint8 Red	= (uint8)(X + XOffset);
			uint8 Green = -(uint8)(Y + YOffset);

			// Pixel in Memory: BB GG RR xx
			*Pixel++ = ((Green << 8) | (Red << 16) | (255 << 24));
			// 255 magic number is to set the empty padding byte to the max value as raddebugger
			// interprets this byte as the alpha and make the bitmap view not display any pixels
		}

		Row += Buffer->Pitch;
	}
}

// Device Independent Bitmap
internal void ResizeDIBSection(bitmap_buffer *Buffer, int Width, int Height)
{
	// TODO(Sam): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if(Buffer->Memory)
	{
		SDL_free(Buffer->Memory);
	}

	Buffer->Width	  = Width;
	Buffer->Height	  = Height;
	int BytesPerPixel = 4;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory		 = SDL_malloc(BitmapMemorySize);

	Buffer->Pitch = Buffer->Width * BytesPerPixel;

	// TODO(Sam): Probably want to clear this to black.
}

// TODO(Sam): From code pre SDL, consider creating a blit function that works with SDL
// internal void BlitBufferToWindow(bitmap_buffer *Buffer, HDC DeviceContext, int ClientWidth,
// 								 int ClientHeight)
// {
// 	// TODO(Sam): Aspect Ratio Correction.
// 	// TODO(Sam): Play with stretch modes.
// 	StretchDIBits(DeviceContext, 0, 0, ClientWidth, ClientHeight, 0, 0, Buffer->Width,
// 				  Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
// }

// NOTE(Sam): SDL has a macro that redefines this to a custom SDL_main
int main(int Argc, char **Argv)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

	SDL_Window *Window = SDL_CreateWindow("Handmade Game", 1920, 1080, SDL_WINDOW_RESIZABLE);

	SDL_Renderer *Renderer = SDL_CreateRenderer(Window, NULL);

	ResizeDIBSection(&GlobalBackBuffer, 1920, 1080);

	SDL_Texture *Texture =
		SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING,
						  GlobalBackBuffer.Width, GlobalBackBuffer.Height);

	int XOffset = 0;
	int YOffset = 0;

	GlobalRunning = true;
	while(GlobalRunning)
	{
		SDL_Event Event;
		while(SDL_PollEvent(&Event))
		{
			switch(Event.type)
			{
				case SDL_EVENT_QUIT:
				{
					GlobalRunning = false;
				}
				break;
				case SDL_EVENT_KEY_DOWN:
				{
					SDL_Scancode key = Event.key.scancode;
					if(key == SDL_SCANCODE_ESCAPE)
					{
						GlobalRunning = false;
					}

					SDL_Log("Key: %s DOWN", SDL_GetScancodeName(key));
				}
				break;
				case SDL_EVENT_KEY_UP:
				{
					SDL_Log("Key: %s UP", SDL_GetScancodeName(Event.key.scancode));
				}
				break;
			}
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

		RenderGradientUV(&GlobalBackBuffer, XOffset, YOffset);

		SDL_UpdateTexture(Texture, NULL, GlobalBackBuffer.Memory, GlobalBackBuffer.Pitch);

		// -----------------------------
		// RENDER TO SCREEN
		// -----------------------------
		SDL_RenderClear(Renderer);

		int Width, Height;
		SDL_GetWindowSize(Window, &Width, &Height);
		SDL_FRect Destination = {0, 0, (float)Width, (float)Height};
		SDL_RenderTexture(Renderer, Texture, NULL, &Destination);

		SDL_RenderPresent(Renderer);
	}

	SDL_DestroyTexture(Texture);
	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return 0;
}
