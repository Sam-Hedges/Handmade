#include <iostream>
#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{

	LRESULT Result = 0;

	switch(Message)
	{
		case WM_SIZE:
		{
			OutputDebugStringA("WM_SIZE\n");
		}
		break;
		case WM_DESTROY:
		{
			OutputDebugStringA("WM_DESTROY\n");
		}
		break;
		case WM_CLOSE:
		{
			OutputDebugStringA("WM_CLOSE\n");
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
			HDC DeviceContext	= BeginPaint(Window, &Paint);
			int X				= Paint.rcPaint.left;
			int Y				= Paint.rcPaint.top;
			int Width			= Paint.rcPaint.right - Paint.rcPaint.left;
			int Height			= Paint.rcPaint.bottom - Paint.rcPaint.top;
			static DWORD Colour = WHITENESS;
			PatBlt(DeviceContext, X, Y, Width, Height, Colour);
			Colour = ~Colour;
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
			MSG Message;
			for(;;)
			{
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
