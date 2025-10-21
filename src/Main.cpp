#include <iostream>
#include <stdint.h>
#include <windows.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#define internal static
#define local_persist static
#define global_var static

struct bitmap_buffer
{
	// NOTE(Sam): Pixels are always 32-bits wide, Memory Order BB GG RR xx
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch; // How many bytes a pointer has to move in order to go from one row to the next row
};

// TODO(Sam): This is a global for now.
global_var bool GlobalRunning;
global_var bitmap_buffer GlobalBackBuffer;

struct window_dimension
{
	int Width;
	int Height;
};

window_dimension GetWindowDimension(HWND Window)
{
	window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width  = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void RenderGradientUV(bitmap_buffer Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory;
	for(int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = 0; X < Buffer.Width; ++X)
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

		Row += Buffer.Pitch;
	}
}

// Device Independent Bitmap
internal void ResizeDIBSection(bitmap_buffer *Buffer, int Width, int Height)
{
	// TODO(Sam): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width	  = Width;
	Buffer->Height	  = Height;
	int BytesPerPixel = 4;

	// NOTE(Sam): When the biHeight field is negative, this is the clue to
	// windows to treat this bitmap as top down and not bottom up, meaning that
	// the first 3 bytes of the image are the top left pixel not the bottom left
	Buffer->Info.bmiHeader.biSize		 = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth		 = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight		 = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes		 = 1;
	Buffer->Info.bmiHeader.biBitCount	 = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory		 = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Buffer->Width * BytesPerPixel;

	// TODO(Sam): Probably want to clear this to black.
}

internal void BlitBufferToWindow(bitmap_buffer Buffer, HDC DeviceContext, int ClientWidth,
								 int ClientHeight)
{
	// TODO(Sam): Aspect Ratio Correction.
	// TODO(Sam): Play with stretch modes.
	StretchDIBits(DeviceContext, 0, 0, ClientWidth, ClientHeight, 0, 0, Buffer.Width, Buffer.Height,
				  Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_SIZE:
		{
		}
		break;
		case WM_DESTROY:
		{
			// TODO(Sam): Handle this as an error, recreate window?
			GlobalRunning = false;
		}
		break;
		case WM_CLOSE:
		{
			// TODO(Sam): Handle this with a message to the user?
			GlobalRunning = false;
		}
		break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext		   = BeginPaint(Window, &Paint);
			window_dimension Dimension = GetWindowDimension(Window);
			BlitBufferToWindow(GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		}
		break;
		default:
		{
			// OutputDebugStringA("default\n");
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		}
		break;
	}

	return (Result);
}

// Entry point for a Windows desktop application.
// The CALLBACK and WinMain signature are required by the Windows API for GUI apps.
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
	WNDCLASS WindowClass = {};

	ResizeDIBSection(&GlobalBackBuffer, 1920, 1080);

	WindowClass.style		  = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc	  = MainWindowCallback; // Handles messages
	WindowClass.hInstance	  = Instance;
	WindowClass.lpszClassName = "HandmadeGameWindowClass";
	// WindowClass.icon		  = ;

	if(RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0, WindowClass.lpszClassName, "Handmade Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

		if(Window)
		{
			HDC DeviceContext = GetDC(Window);

			int XOffset = 0, YOffset = 0;

			GlobalRunning = true;
			while(GlobalRunning)
			{
				MSG Message;
				while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				RenderGradientUV(GlobalBackBuffer, XOffset, YOffset);

				window_dimension Dimension = GetWindowDimension(Window);
				BlitBufferToWindow(GlobalBackBuffer, DeviceContext, Dimension.Width,
								   Dimension.Height);
				++XOffset;
				--YOffset;
			}
		}
		else
		{
			// TODO(Sam): Logging fail state
		}
	}
	else
	{
		// TODO(Sam): Logging fail state
	}

	return 0;
}
