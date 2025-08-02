#ifndef HANDMADE_H
#define HANDMADE_H

struct game_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Pitch;
    int Height;
    int BytesPerPixel;
};

struct game_window_dimension {
    int Width, Height;    
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#endif
