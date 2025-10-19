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

// TODO(Sam): This is a global for now.
global_var bool Running;

global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;

// TODO(Sam): Remove these at some point.
global_var int BitmapWidth;
global_var int BitmapHeight;
global_var int BytesPerPixel = 4;

internal void RenderGradientUV(int XOffset, int YOffset)
{
	int Pitch  = BitmapWidth * BytesPerPixel;
	uint8 *Row = (uint8 *)BitmapMemory;
	for(int Y = 0; Y < BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = 0; X < BitmapWidth; ++X)
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

		Row += Pitch;
	}
}

// Device Independent Bitmap
internal void ResizeDIBSection(int Width, int Height)
{
	// TODO(Sam): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if(BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth	 = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize		   = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth	   = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight	   = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes	   = 1;
	BitmapInfo.bmiHeader.biBitCount	   = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
	BitmapMemory		 = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	// TODO(Sam): Probably want to clear this to black.
}

internal void BlitWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height)
{
	// StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height, BitmapMemory,
	// 			  &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);

	int ClientWidth	 = ClientRect->right - ClientRect->left;
	int ClientHeight = ClientRect->bottom - ClientRect->top;

	StretchDIBits(DeviceContext, X, Y, BitmapWidth, BitmapHeight, X, Y, ClientWidth, ClientHeight,
				  BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width  = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			ResizeDIBSection(Width, Height);

			// HDC DeviceContext = GetDC(Window);
			// BlitWindow(DeviceContext, &ClientRect, 0, 0, Width, Height);
			// ReleaseDC(Window, DeviceContext);
		}
		break;
		case WM_DESTROY:
		{
			// TODO(Sam): Handle this as an error, recreate window?
			Running = false;
		}
		break;
		case WM_CLOSE:
		{
			// TODO(Sam): Handle this with a message to the user?
			Running = false;
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
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X			  = Paint.rcPaint.left;
			int Y			  = Paint.rcPaint.top;
			int Width		  = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height		  = Paint.rcPaint.bottom - Paint.rcPaint.top;
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			BlitWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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

	// TODO(Sam): Check if OWNDC, HREDRAW and VREDRAW still matter
	WindowClass.style		  = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
			int XOffset = 0, YOffset = 0;

			Running = true;
			while(Running)
			{
				MSG Message;
				while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				RenderGradientUV(XOffset, YOffset);
				HDC DeviceContext = GetDC(Window);
				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				int ClientWidth	 = ClientRect.right - ClientRect.left;
				int ClientHeight = ClientRect.bottom - ClientRect.top;
				BlitWindow(DeviceContext, &ClientRect, 0, 0, ClientWidth, ClientHeight);
				ReleaseDC(Window, DeviceContext);
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
