#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool Running; 
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;

internal void 
Win32ResizeDIBSection(int Width, int Height) {
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    if (BitmapHandle) {
        DeleteObject(BitmapHandle);
    }

    BitmapHandle = CreateDIBSection(
        DeviceContext,
        &BitmapInfo,
        DIB_RGB_COLORS,
        &BitmapMemory,
        0, 0);
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height) {
    StretchDIBits(
        DeviceContext,
        X, Y, Width, Height,
        X, Y, Width, Height,
        const void *lpBits,
        const BITMAPINFO *lpbmi,
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
            Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
            Running = true;
            MSG message;
            while(Running) {
                BOOL msgResult = GetMessageA(&message, 0, 0, 0);
                if (msgResult > 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                } else {
                    break;
                }
            }
        } else {
            // TODO: Logging
        }

    } else {
        // TODO: Logging
    }

    return 0;
}