#include "rgb2yuv.h"

static u8 clip_u8(ap_int<32> x) {
    if (x < 0) return 0;
    if (x > 255) return 255;
    return (u8)x;
}

// Version 1: Vivado HLS C, integer data type.
// Top Function: rgb2yuv
// Formula matches the official SDK reference:
// Y = (( 66R + 129G +  25B + 128) >> 8) +  16
// U = ((-38R -  74G + 112B + 128) >> 8) + 128
// V = ((112R -  94G -  18B + 128) >> 8) + 128
void rgb2yuv(
    volatile u8 *R,
    volatile u8 *G,
    volatile u8 *B,
    volatile u8 *Y,
    volatile u8 *U,
    volatile u8 *V
) {
#pragma HLS INTERFACE s_axilite port=R      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=G      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=B      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=Y      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=U      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=V      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL

    ap_int<32> r = *R;
    ap_int<32> g = *G;
    ap_int<32> b = *B;

    ap_int<32> y_tmp = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
    ap_int<32> u_tmp = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
    ap_int<32> v_tmp = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

    *Y = clip_u8(y_tmp);
    *U = clip_u8(u_tmp);
    *V = clip_u8(v_tmp);
}
