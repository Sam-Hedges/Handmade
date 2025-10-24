#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <xinput.h>

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

struct window_dimension
{
	int Width;
	int Height;
};

// TODO(Sam): This is a global for now.
global_var bool GlobalRunning;
global_var bitmap_buffer GlobalBackBuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub) { return 0; }
X_INPUT_SET_STATE(XInputSetStateStub) { return 0; }

global_var x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_var x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}
	if(XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal window_dimension GetWindowDimension(HWND Window)
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

// TODO(Sam): Remove once not needed.
void PrintKeyState(bool WasDown, bool IsDown, const char *Key)
{
	OutputDebugStringA(Key);
	OutputDebugStringA(": ");
	if(IsDown)
	{
		OutputDebugStringA("IsDown ");
	}
	if(WasDown)
	{
		OutputDebugStringA("WasDown");
	}
	OutputDebugStringA("\n");
	return;
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
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = (uint32)WParam;
			bool WasDown  = ((LParam & (1 << 30)) != 0);
			bool IsDown	  = ((LParam & (1 << 31)) == 0);

			switch(VKCode)
			{
				case 'W':
				case VK_UP:
				{
					PrintState(WasDown, IsDown, "W");
				}
				break;
				case 'S':
				case VK_DOWN:
				{
					PrintState(WasDown, IsDown, "S");
				}
				break;
				case 'A':
				case VK_LEFT:
				{
					PrintState(WasDown, IsDown, "A");
				}
				break;
				case 'D':
				case VK_RIGHT:
				{
					PrintState(WasDown, IsDown, "D");
				}
				break;
				case 'Q':
				{
					PrintState(WasDown, IsDown, "Q");
				}
				break;
				case 'E':
				{
					PrintState(WasDown, IsDown, "E");
				}
				break;
				case VK_SPACE:
				{
					PrintState(WasDown, IsDown, "Space");
				}
				break;
				case VK_ESCAPE:
				{
					PrintState(WasDown, IsDown, "Escape");
				}
				break;
			}
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
	LoadXInput();

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

				// TODO(Sam): Should we poll this more frequently
				for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
				{
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// NOTE(Sam): This controller is plugged in.
						// TODO(Sam): See if ControllerState.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;

						bool DPadUp		   = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool DPadDown	   = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool DPadLeft	   = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool DPadRight	   = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start		   = (Gamepad->wButtons & XINPUT_GAMEPAD_START);
						bool Back		   = (Gamepad->wButtons & XINPUT_GAMEPAD_BACK);
						bool ShoulderLeft  = (Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool ShoulderRight = (Gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton	   = (Gamepad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton	   = (Gamepad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton	   = (Gamepad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton	   = (Gamepad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickLX = Gamepad->sThumbLX;
						int16 StickLY = Gamepad->sThumbLY;

						if(DPadDown)
						{
							--YOffset;
						}
						if(DPadUp)
						{
							++YOffset;
						}
						if(DPadLeft)
						{
							++XOffset;
						}
						if(DPadRight)
						{
							--XOffset;
						}

						XINPUT_VIBRATION Vibration;
						Vibration.wLeftMotorSpeed  = 0;
						Vibration.wRightMotorSpeed = 0;
						if(ShoulderLeft)
						{
							Vibration.wLeftMotorSpeed = 60000;
						}
						if(ShoulderRight)
						{
							Vibration.wRightMotorSpeed = 60000;
						}
						XInputSetState(0, &Vibration);
					}
					else
					{
						// NOTE(Sam): The controller is not available.
					}
				}

				RenderGradientUV(GlobalBackBuffer, XOffset, YOffset);

				window_dimension Dimension = GetWindowDimension(Window);
				BlitBufferToWindow(GlobalBackBuffer, DeviceContext, Dimension.Width,
								   Dimension.Height);
				// ++XOffset;
				// --YOffset;
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
