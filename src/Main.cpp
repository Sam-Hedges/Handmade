#include <cstdint>
#include <iostream>
#include <windows.h>

// TODO(Sam): Move to platform independant header at some point.
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// WARNING(Sam): Global Vars
static bool Running; // TODO(Sam): This is a global for now.
static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static const int BytesPerPixel = 4;

static void RenderUVGradient(int XOffset, int YOffset) {

    int Pitch = BitmapWidth * BytesPerPixel;

    uint8 *Row = (uint8 *)BitmapMemory;

    for (int Y = 0; Y < BitmapHeight; Y++) {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < BitmapWidth; X++) {
            // Pixel in memory: BB GG RR 00
            // 0x xxRRGGBB
            uint8 Green = -(uint8)Y + YOffset;
            uint8 Red = (uint8)X + XOffset;

            *Pixel++ = ((Green << 8) | (Red << 16) | ((uint8)255 << 24));
        }

        Row += Pitch;
    }
}

// Device Independant Bitmap
static void ResizeDIBSection(int Width, int Height) {

    if (BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;

    BitmapMemory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO(Sam): Probably clear to black.
}

static void WindowUpdate(HDC DeviceContext, RECT *ClientRect, int X, int Y,
                         int Width, int Height) {

    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;

    StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0,
                  WindowWidth, WindowHeight, BitmapMemory, &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam,
                                    LPARAM LParam) {
    LRESULT Result = 0;

    switch (Message) {
    case WM_SIZE: {
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);
        int Width = ClientRect.right - ClientRect.left;
        int Height = ClientRect.bottom - ClientRect.top;
        ResizeDIBSection(Width, Height);
    } break;
    case WM_CLOSE: {
        // TODO(Sam): Handle this with a message to the user?
        Running = false;
    } break;
    case WM_DESTROY: {
        // TODO(Sam): Handle this as an error - recreate window?
        Running = false;
    } break;

    case WM_ACTIVATEAPP: {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;
    case WM_PAINT: {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;

        RECT ClientRect;
        GetClientRect(Window, &ClientRect);

        WindowUpdate(DeviceContext, &ClientRect, X, Y, Width, Height);
        EndPaint(Window, &Paint);
    } break;
    default: {
        Result = DefWindowProcA(Window, Message, WParam, LParam);
    } break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {

    // char unsigned Test;
    // char unsigned *TestPointer;
    //
    // TestPointer = &Test;
    //
    // Test = 255;
    // Test = Test + 1;

    WNDCLASS WindowClass = {};

    // Set of binary flags, properties of window
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback; // Pointer to func
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon = ; Might want an icon at some point
    // Name for window class
    WindowClass.lpszClassName = "GameProjectWindowClass";

    if (RegisterClass(&WindowClass)) {

        HWND Window = CreateWindowExA(
            0, WindowClass.lpszClassName, "GameProject",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (Window) {
            MSG Message;
            Running = true;
            int XOffset, YOffset;
            while (Running) {
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT)
                        Running = false;
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderUVGradient(XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;

                WindowUpdate(DeviceContext, &ClientRect, 0, 0, WindowWidth,
                             WindowHeight);
                ReleaseDC(Window, DeviceContext);
                ++XOffset;
                ++YOffset;
            }
        } else {
            // TODO(Sam): Logging for error
        }
    } else {
        // TODO(Sam): Logging for error
    }

    std::cout << "Hello World";

    return 0;
}
