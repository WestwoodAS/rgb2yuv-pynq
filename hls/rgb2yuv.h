#ifndef RGB2YUV_H
#define RGB2YUV_H

#include <ap_int.h>

typedef ap_uint<8> u8;

void rgb2yuv(
    volatile u8 *R,
    volatile u8 *G,
    volatile u8 *B,
    volatile u8 *Y,
    volatile u8 *U,
    volatile u8 *V
);

#endif
