#include <iostream>
#include "rgb2yuv.h"

static unsigned char sw_clip(int x) {
    if (x < 0) return 0;
    if (x > 255) return 255;
    return (unsigned char)x;
}

int main() {
    const unsigned char test_pixels[][3] = {
        {153, 171, 203}, {0, 0, 0}, {255, 255, 255},
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}
    };

    bool pass = true;
    for (unsigned i = 0; i < sizeof(test_pixels) / sizeof(test_pixels[0]); ++i) {
        u8 R = test_pixels[i][0];
        u8 G = test_pixels[i][1];
        u8 B = test_pixels[i][2];
        u8 Y = 0, U = 0, V = 0;
        rgb2yuv(&R, &G, &B, &Y, &U, &V);

        int r = test_pixels[i][0], g = test_pixels[i][1], b = test_pixels[i][2];
        unsigned char ey = sw_clip((( 66*r + 129*g +  25*b + 128) >> 8) +  16);
        unsigned char eu = sw_clip(((-38*r -  74*g + 112*b + 128) >> 8) + 128);
        unsigned char ev = sw_clip(((112*r -  94*g -  18*b + 128) >> 8) + 128);

        std::cout << "R=" << r << " G=" << g << " B=" << b
                  << " -> HW(Y,U,V)=(" << (int)Y << "," << (int)U << "," << (int)V << ")"
                  << " SW(Y,U,V)=(" << (int)ey << "," << (int)eu << "," << (int)ev << ")\n";
        if ((unsigned)Y != ey || (unsigned)U != eu || (unsigned)V != ev) pass = false;
    }

    std::cout << (pass ? "PASS" : "FAIL") << std::endl;
    return pass ? 0 : 1;
}
