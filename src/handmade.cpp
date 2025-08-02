#include "handmade.h"

internal void
RenderGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {    
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

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer)
{
    int XOffset = 0;
    int YOffset = 0;
    RenderGradient(*Buffer, XOffset, YOffset);
}