#include <iostream>
#include <windows.h>

#define internal static
#define local_persist static
#define global_var static

// TODO(Sam): This is a global for now.
global_var bool Running;

global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var HBITMAP BitmapHandle;
global_var HDC BitmapDeviceContext;

// Device Independent Bitmap
internal void ResizeDIBSection(int Width, int Height)
{
	// TODO(Sam): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if(BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}

	if(!BitmapDeviceContext)
	{
		// TODO(Sam): Should we recreate these under certain special circumstances.
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize		   = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth	   = Width;
	BitmapInfo.bmiHeader.biHeight	   = Height;
	BitmapInfo.bmiHeader.biPlanes	   = 1;
	BitmapInfo.bmiHeader.biBitCount	   = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	BitmapHandle =
		CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

internal void BlitWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
	StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height, BitmapMemory,
				  &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
			BlitWindow(DeviceContext, X, Y, Width, Height);
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
		HWND WindowHandle = CreateWindowExA(
			0, WindowClass.lpszClassName, "Handmade Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

		if(WindowHandle)
		{
			Running = true;
			while(Running)
			{
				MSG Message;
				BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
				if(MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				else
				{
					break;
				}
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
