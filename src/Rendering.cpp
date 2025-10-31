#include <PlatformLayer.h>

/* ========================================================================
   Structures
   ======================================================================== */

struct platform_bitmap_buffer
{
	// NOTE(Sam): Pixels are always 32-bits wide, Memory Order BB GG RR xx
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	// How many bytes a pointer has to move in
	// order to go from one row to the next row
};

/* ========================================================================
   Functions
   ======================================================================== */

internal void RenderGradientUV(platform_bitmap_buffer *Buffer, int XOffset, int YOffset)
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

///////////////////////////////////////////////////////////////////////////////

internal void ResizeBitmap(platform_bitmap_buffer *Buffer, int Width, int Height)
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
