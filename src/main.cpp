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

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Pitch;
    int Height;
    int BytesPerPixel;
};

struct win32_window_dimension {
    int Width, Height;    
};

win32_window_dimension 
Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return Result;
}

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal void
RenderGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {    
    uint8 *Row = (uint8 *) Buffer->Memory;
    for (int Y=0; Y<Buffer->Height; ++Y) {
        uint32 *Pixel = (uint32 *) Row;
        for (int X=0; X<Buffer->Width; ++X) {
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);
            uint8 Red = (X+Y);

            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);          
        }
        Row += Buffer->Pitch;
    }
}

internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {
    if (Buffer->Memory) {
        // VirtualProtect
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Height = Height;
    Buffer->Width = Width;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    // HeapAlloc; VirtualAlloc
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // RenderGradient(0, 0);
    Buffer->Pitch = Width * (Buffer->BytesPerPixel);
}

internal void
Win32DisplayBuffer(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height) {
    StretchDIBits(
        DeviceContext,
        // X, Y, Width, Height,
        // X, Y, Width, Height,
        0, 0, Buffer.Width, Buffer.Height,
        0, 0, WindowWidth, WindowHeight,
        Buffer.Memory,
        &Buffer.Info,
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
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);
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
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);

            Win32DisplayBuffer(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, X, Y, Width, Height);
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
            
            while(Running) {
                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        Running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                RenderGradient(&GlobalBackBuffer, XOffset, YOffset);
                HDC DeviceContext = GetDC(window);

                win32_window_dimension Dimension = Win32GetWindowDimension(window);
                
                Win32DisplayBuffer(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, 0, 0, Dimension.Width, Dimension.Height);
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