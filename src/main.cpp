#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

global_variable bool Running; 
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderGradient(int XOffset, int YOffset) {
    int Width = BitmapWidth;
    int Height = BitmapHeight;
    int Pitch = Width * BytesPerPixel;
    
    uint8 *Row = (uint8 *) BitmapMemory;
    for (int Y=0; Y<BitmapHeight; ++Y) {
        uint32 *Pixel = (uint32 *) Row;
        for (int X=0; X<BitmapWidth; ++X) {
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);
            uint8 Red = (X+Y);

            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);          
        }
        Row += Pitch;
    }
}

internal void 
Win32ResizeDIBSection(int Width, int Height) {
    if (BitmapMemory) {
        // VirtualProtect
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapHeight = Height;
    BitmapWidth = Width;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
    // HeapAlloc; VirtualAlloc
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    RenderGradient(0, 0);
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height) {
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;
    StretchDIBits(
        DeviceContext,
        // X, Y, Width, Height,
        // X, Y, Width, Height,
        0, 0, BitmapWidth, BitmapHeight,
        0, 0, WindowWidth, WindowHeight,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS, SRCCOPY
    );
}

LRESULT CALLBACK
Win32WindowProc(HWND   Window,
           UINT   message,
           WPARAM WParam,
           LPARAM LParam
) {
    LRESULT Result = 0;

    switch(message) {
        case WM_SIZE: {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY: {
            // Display a message to the user
            Running = false;
        } break;

        case WM_CLOSE: {
            // Handle this as an error - recreate window?
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

            Win32UpdateWindow(DeviceContext, &(ClientRect), X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            Result = DefWindowProc(Window, message, WParam, LParam);
        }
    }
    return Result;
}

int CALLBACK 
WinMain(HINSTANCE hInstance, 
        HINSTANCE hPrevInstance, 
        LPSTR     lpCmdLine, 
        int       nShowCmd) {
    WNDCLASSEXA wndClass = {0};
    wndClass.cbSize = sizeof(WNDCLASSEXA);
    wndClass.style = CS_OWNDC | CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = "HandmadeHeroWindowClass";
    wndClass.lpfnWndProc = Win32WindowProc; 

    if (RegisterClassExA(&wndClass)) {
        HWND window = CreateWindowExA (
            0,
            wndClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );

        if (window) {
            int XOffset = 0, YOffset = 0;
            Running = true;
            MSG message;
            while(Running) {
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        Running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                RenderGradient(XOffset, YOffset);
                HDC DeviceContext = GetDC(window);

                RECT ClientRect;
                GetClientRect(window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(window, DeviceContext);

                ++XOffset;
            }
        } else {
            // TODO: Logging
        }

    } else {
        // TODO: Logging
    }

    return 0;
}