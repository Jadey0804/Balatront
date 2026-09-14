#pragma once
#include "GamesEngineeringBase.h"

// This is a small fixed font and does not need another font library.
namespace Hud {
    inline void text(GamesEngineeringBase::Window& canvas, int x, int y, const char* message) {
        static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-./!";
        static const unsigned char glyphs[][7] = {
            {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
            {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
            {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
            {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
            {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
            {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
            {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
            {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
            {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
            {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
            {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
            {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
            {14,17,17,15,1,1,14},{0,4,4,0,4,4,0},{0,0,0,31,0,0,0},
            {0,0,0,0,0,4,4},{1,2,2,4,8,8,16},{4,4,4,4,4,0,4}
        };
        constexpr int scale = 2;
        for (; *message; ++message, x += 6 * scale) {
            unsigned int glyph = 0;
            while (alphabet[glyph] && alphabet[glyph] != *message) ++glyph;
            if (!alphabet[glyph]) continue;
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (glyphs[glyph][row] & (1 << (4 - col)))
                        for (int dy = 0; dy < scale; ++dy)
                            for (int dx = 0; dx < scale; ++dx) {
                                const int px = x + col * scale + dx;
                                const int py = y + row * scale + dy;
                                if (px >= 0 && py >= 0 && px < int(canvas.getWidth()) && py < int(canvas.getHeight()))
                                    canvas.draw(px, py, 240, 240, 240);
                            }
        }
    }
}
