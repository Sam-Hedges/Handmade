#include <iostream>
#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{

	LRESULT Result = 0;

	switch (Message)
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
		default:
		{
			// OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, WParam, LParam);
		}
		break;
	}

	return (Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{

	WNDCLASS WindowClass = {};

	// TODO(Sam): Check if OWNDC, HREDRAW and VREDRAW still matter
	WindowClass.style		  = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc	  = MainWindowCallback;
	WindowClass.hInstance	  = Instance;
	WindowClass.lpszClassName = "HandmadeGameWindowClass";
	// WindowClass.icon		  = ;

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle =
			CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
						   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
		if (WindowHandle)
		{
			MSG Message;
			GetMessage(&Message, 0, 0, 0);
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
