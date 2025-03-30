#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t int8;
typedef uint16_t int16;
typedef uint32_t int32;
typedef uint64_t int64;

typedef int32 bool32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// Check what _In_ and _Out_ mean (_In_opt_, _Outptr_): Something with the static code analyzer?
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

typedef X_INPUT_GET_STATE(X_Input_Get_State);
typedef X_INPUT_SET_STATE(X_Input_Set_State);

X_INPUT_GET_STATE(X_Input_Get_State_Stub) { return ERROR_DEVICE_NOT_CONNECTED; }
X_INPUT_SET_STATE(X_Input_Set_State_Stub) { return ERROR_DEVICE_NOT_CONNECTED; }

global_variable X_Input_Get_State *XInputGetState_ = X_Input_Get_State_Stub;
global_variable X_Input_Set_State *XInputSetState_ = X_Input_Set_State_Stub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void) {
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (XInputLibrary) {
        XInputGetState = (X_Input_Get_State *) GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (X_Input_Set_State *) GetProcAddress(XInputLibrary, "XInputSetState");
    } else {
        // Log
    }
}

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

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal void
RenderGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {    
    int8 *Row = (int8 *) Buffer->Memory;
    for (int Y=0; Y<Buffer->Height; ++Y) {
        int32 *Pixel = (int32 *) Row;
        for (int X=0; X<Buffer->Width; ++X) {
            int8 Blue = (X + XOffset);
            int8 Green = (Y + YOffset);
            int8 Red = (X+Y);

            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);          
        }
        Row += Buffer->Pitch;
    }
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSec, int32 BufferSize) {
    // 1. Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary) {
        // 2. Get a DirectSound object
        direct_sound_create* DirectSoundCreate = (direct_sound_create*) GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) 
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSec;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) 
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // 3. Create a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) 
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if (SUCCEEDED(Error)) 
                    {
                        OutputDebugStringA("Primary buffer format was set.\n");
                    } else 
                    {
                        // Diagnostics
                    }
                }
                else {
                    // Diagnostics
                }
            }            
            else 
            {
                // Log
            }

            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            // 3. Create a primary buffer
            LPDIRECTSOUNDBUFFER SecondaryBuffer;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0);
            if (SUCCEEDED(Error)) 
            {
                // 5. Start it playing
                OutputDebugStringA("Secondary buffer successfully created.\n");
            }

            // 4. Create a secondary buffer
            BufferDescription.dwBufferBytes = BufferSize;
        } else {
            // Log as an error
        }
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
Win32DisplayBuffer(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer *Buffer, int X, int Y, int Width, int Height) {
    StretchDIBits(
        DeviceContext,
        // X, Y, Width, Height,
        // X, Y, Width, Height,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
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
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY: {
            // Display a message to the user
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            int32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) != 0);
            if (WasDown != IsDown) {
                if (VKCode == VK_UP) {} 
                else if (VKCode == VK_DOWN) {}
                else if (VKCode == VK_LEFT) {} 
                else if (VKCode == VK_RIGHT) {}
                else if (VKCode == VK_ESCAPE) {}
                else if (VKCode == VK_SPACE) {}
            }
            
            bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
            if ((VKCode == VK_F4) && AltKeyWasDown) {
                GlobalRunning = false;
            }
        } break;

        case WM_CLOSE: {
            // Handle this as an error - recreate window?
            GlobalRunning = false;
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

            Win32DisplayBuffer(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer, X, Y, Width, Height);
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
    Win32LoadXInput();

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

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
            GlobalRunning = true;

            Win32InitDSound(window, 48000, 48000 * sizeof(int16) * 2); 
            
            while(GlobalRunning) {
                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                RenderGradient(&GlobalBackBuffer, XOffset, YOffset);
                HDC DeviceContext = GetDC(window);

                win32_window_dimension Dimension = Win32GetWindowDimension(window);
                
                Win32DisplayBuffer(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer, 0, 0, Dimension.Width, Dimension.Height);
                ReleaseDC(window, DeviceContext);

                ++XOffset;
            }

            // Check polling frequency
            for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex) {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                        // This controller is connected.
                        // See if ControllerState.dwPacketNumber increments too rapidly.
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int StickX = Pad->sThumbLX;
                        int StickY = Pad->sThumbLY;
                        /*
                            Check and change to keyboard and mouse input: 
                            #include <windows.h>

                            bool Up = GetAsyncKeyState(VK_UP) & 0x8000;
                            bool Down = GetAsyncKeyState(VK_DOWN) & 0x8000;
                            bool Left = GetAsyncKeyState(VK_LEFT) & 0x8000;
                            bool Right = GetAsyncKeyState(VK_RIGHT) & 0x8000;
                            bool AButton = GetAsyncKeyState('A') & 0x8000;  
                            bool BButton = GetAsyncKeyState('B') & 0x8000;

                        */
                    } else {
                        // This controller is not connected.
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