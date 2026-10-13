`timescale 1ns/1ps
`define bits 9

module lab3_v3_top (
    input  wire        clk,
    input  wire [31:0] in_pack,
    output wire [31:0] out_pack
);

wire        start;
wire        rst_n;
wire [8:0]  inR, inG, inB;
wire        done;
wire [8:0]  outY, outU, outV;

assign inR   = in_pack[8:0];
assign inG   = in_pack[17:9];
assign inB   = in_pack[26:18];
assign start = in_pack[27];
assign rst_n = in_pack[28];

RGB2YUV u_rgb2yuv (
    .start   (start),
    .clk     (clk),
    .rst_n   (rst_n),
    .inportR (inR),
    .inportG (inG),
    .inportB (inB),
    .done    (done),
    .outportY(outY),
    .outportU(outU),
    .outportV(outV)
);

assign out_pack = {4'b0000, done, outV, outU, outY};

endmodule
