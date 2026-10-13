# RGB2YUV HW/SW Co-Design on PYNQ-Z2

在 PYNQ-Z2（Zynq-7020）上實作 RGB 轉 YUV 的軟硬體協同設計。FPGA 端（PL）負責 RGB 轉 YUV 的運算，ARM 處理器端（PS）透過 AXI-Lite 介面逐像素送入資料並讀回結果，再於 Jupyter Notebook 中顯示 Y、U、V 三張影像。

本專案用兩種方式實作同一個硬體加速器，比較高階合成與手寫 RTL 的差異：

| 版本 | 設計方式 | 介面 |
|---|---|---|
| Version 1 | Vivado HLS（C 語言、整數運算） | HLS 自動產生的 AXI-Lite slave |
| Version 3 | 手寫 structural Verilog（1 個乘法器 + 1 個加法器） | 自訂 AXI-Lite IP 包裝 |

依課程安排，FPGA 端只實作 Version 1 與 Version 3，Version 2 未實作。

相關專案：

- [rgb2yuv-rtl](https://github.com/WestwoodAS/rgb2yuv-rtl)：Version 3 的 RTL 來源，包含三種資源配置的設計與比較（Lab 1）
- [systemc-modeling](https://github.com/WestwoodAS/systemc-modeling)：RGB 轉 YUV 的 SystemC 定點數模型與精度分析（Lab 2）

## 系統架構

```
┌─────────────────── Zynq-7020 ───────────────────┐
│                                                 │
│  PS (ARM Cortex-A9)          PL (FPGA)          │
│  ┌─────────────────┐        ┌────────────────┐  │
│  │ Jupyter / PYNQ  │ AXI-   │ RGB2YUV IP     │  │
│  │ 讀取影像、逐像素 │ Lite   │ Version 1: HLS │  │
│  │ 寫入 R/G/B、讀回 ├───────▶│ Version 3: RTL │  │
│  │ Y/U/V 並顯示    │ M_AXI_ │                │  │
│  └─────────────────┘ GP0    └────────────────┘  │
│                                                 │
└─────────────────────────────────────────────────┘
```

Block design 由 ZYNQ7 Processing System、AXI Interconnect、Processor System Reset 與 RGB2YUV IP 組成，PL 時脈為 FCLK_CLK0 100 MHz，IP 的 AXI-Lite 基底位址為 `0x43C00000`。

## 目錄結構

```
rgb2yuv-pynq/
├── hls/                       # Version 1：Vivado HLS
│   ├── rgb2yuv.cpp            # Top function：rgb2yuv
│   ├── rgb2yuv.h
│   └── tb_rgb2yuv.cpp         # C simulation testbench
├── rtl/                       # Version 3：structural Verilog
│   ├── lab3_v3_top.v          # 32-bit 打包介面，供 AXI-Lite IP 包裝
│   ├── RGB2YUV.v              # Controller + Datapath
│   ├── Controller.v           # 12 個 state 的 FSM，22-bit 控制字
│   ├── Datapath.v
│   └── Add.v  Mul.v  MUX2.v  MUX3.v  MUX4.v  Register.v  ROM.v
└── overlays/                  # PYNQ overlay（bitstream + hardware handoff）
    ├── lab3_v1.bit  lab3_v1.hwh
    └── lab3_v3.bit  lab3_v3.hwh
```

## Version 1：Vivado HLS

以 C 語言撰寫 `rgb2yuv` 函式，使用 BT.601 limited range 的整數公式：

```
Y = (( 66·R + 129·G +  25·B + 128) >> 8) +  16
U = ((-38·R -  74·G + 112·B + 128) >> 8) + 128
V = ((112·R -  94·G -  18·B + 128) >> 8) + 128
```

運算使用 `ap_int<32>` 避免溢位，結果經過 clip 限制在 0–255。所有參數與回傳值都以 `#pragma HLS INTERFACE s_axilite` 綁定到同一個 `CTRL` bundle，HLS 會自動產生 AXI-Lite slave 與控制暫存器。

開發流程：C Simulation → C Synthesis → Export RTL（IP Catalog）→ 在 Vivado block design 中與 ZYNQ PS 連接 → 產生 bitstream 與 `.hwh`。

### 暫存器對應表（由 `.hwh` 取得）

| 位移 | 名稱 | 說明 |
|---|---|---|
| `0x00` | CTRL | bit 0 `ap_start`、bit 1 `ap_done`、bit 2 `ap_idle` |
| `0x10` | R_V | 輸入 R |
| `0x18` | G_V | 輸入 G |
| `0x20` | B_V | 輸入 B |
| `0x28` | Y_V | 輸出 Y |
| `0x30` | U_V | 輸出 U |
| `0x38` | V_V | 輸出 V |

### C Simulation

Testbench 用 6 組測試像素（包含黑、白與 R/G/B 三原色）比對 HLS 函式與軟體參考值：

```
R=153 G=171 B=203 -> HW(Y,U,V)=(161,145,118) SW(Y,U,V)=(161,145,118)
R=0   G=0   B=0   -> HW(Y,U,V)=(16,128,128)  SW(Y,U,V)=(16,128,128)
R=255 G=255 B=255 -> HW(Y,U,V)=(235,128,128) SW(Y,U,V)=(235,128,128)
R=255 G=0   B=0   -> HW(Y,U,V)=(82,90,240)   SW(Y,U,V)=(82,90,240)
R=0   G=255 B=0   -> HW(Y,U,V)=(144,54,34)   SW(Y,U,V)=(144,54,34)
R=0   G=0   B=255 -> HW(Y,U,V)=(41,240,110)  SW(Y,U,V)=(41,240,110)
PASS
```

## Version 3：Structural Verilog

沿用 [rgb2yuv-rtl](https://github.com/WestwoodAS/rgb2yuv-rtl) 中 Version 1 的設計：只用 1 個乘法器與 1 個加法器，由 12 個 state 的 FSM 分時共用運算單元，係數以 Q8 定點數存放在 ROM 中，使用 full range 公式：

```
Y =  0.299·R + 0.587·G + 0.114·B
U = -0.169·R - 0.331·G + 0.500·B + 128
V =  0.500·R - 0.419·G - 0.081·B + 128
```

`lab3_v3_top.v` 將 `RGB2YUV` 的輸入輸出打包成兩個 32-bit 字，以便放進 AXI-Lite IP 的暫存器中由 PS 讀寫：

| `in_pack` 位元 | 訊號 | | `out_pack` 位元 | 訊號 |
|---|---|---|---|---|
| `[8:0]` | R | | `[8:0]` | Y |
| `[17:9]` | G | | `[17:9]` | U |
| `[26:18]` | B | | `[26:18]` | V |
| `[27]` | start | | `[27]` | done |
| `[28]` | rst_n（active-low） | | `[31:28]` | 0 |

`rst_n` 由軟體控制，正常運算時 bit 28 必須維持為 1。

## 在 PYNQ 上執行

將 `.bit` 與同名的 `.hwh` 複製到 PYNQ 板上的同一個資料夾，PYNQ 會自動讀取 `.hwh` 取得 IP 與暫存器資訊。以下是依照 Version 1 暫存器對應表寫的參考範例，並非實驗當時使用的原始 notebook：

```python
from pynq import Overlay

ol = Overlay("lab3_v1.bit")
ip = ol.rgb2yuv_0

def rgb2yuv_hw(r, g, b):
    ip.write(0x10, r)
    ip.write(0x18, g)
    ip.write(0x20, b)
    ip.write(0x00, 1)                  # ap_start
    while (ip.read(0x00) & 0x2) == 0:  # 等待 ap_done
        pass
    return ip.read(0x28), ip.read(0x30), ip.read(0x38)
```

對 256×256 影像的每個像素呼叫一次，再把結果 reshape 成 256×256 顯示即可。

## 實驗結果

| | DSP | LUT | FF | 時脈頻率 | 單筆執行時間 |
|---|---|---|---|---|---|
| Version 1（HLS） | 3 | 588 | 224 | 97.2 MHz | 20.57 ns（2 cycles） |
| Version 3（Verilog） | 1 | 101 | 91 | 107.0 MHz | 102.8 ns（11 cycles） |

- Version 1 的數據來自 HLS synthesis report 的估計值：estimated clock period 為 10.283 ns（目標 10 ns），latency 為 2 cycles。
- Version 3 的 DSP、LUT、FF 來自 Vivado post-implementation（WNS = 0.04 ns），執行時間依 11 個運算 state 換算。

### PYNQ 實測時間（256×256，共 65,536 像素）

| | 總時間 | 平均每像素 |
|---|---|---|
| Version 1（HLS） | 13.83 s | 約 211 µs |
| Version 3（Verilog） | 29.11 s | 約 444 µs |

## 分析

- **資源與速度的取捨。** HLS 版本為了在 2 個 cycle 內完成，把乘法分配到 3 個 DSP 並平行運算，LUT 約為 Verilog 版本的 5.8 倍。Verilog 版本只用 1 個乘法器分時共用，面積最小，但需要 11 個 cycle。這和 Lab 1 中「面積 vs. 延遲」的觀察一致。
- **實測時間的瓶頸在軟體，而不是硬體。** 硬體單筆運算只需 20–100 ns，但 PYNQ 實測每個像素約 200–450 µs，相差三到四個數量級。時間幾乎都花在 Python 逐像素的 MMIO 讀寫與 polling 上，每個像素都要經過多次 AXI-Lite 交易。因此硬體上 5 倍的延遲差距，在實測中只剩約 2 倍，差異主要反映的是兩個版本在軟體端所需的暫存器存取次數不同。
- **可改進的方向。** 若要讓硬體加速真正發揮效果，應該減少 PS 與 PL 之間的單筆交易，例如改用 AXI-Stream 搭配 DMA 一次傳送整張影像，或讓 IP 一次處理多個像素。
- **兩個版本使用的公式不同。** Version 1 是 BT.601 limited range（Y 範圍 16–235），Version 3 是 full range（Y 範圍 0–255），因此兩者的輸出不會逐位元相同，比較時應以各自的軟體參考值驗證。

## 未包含的檔案

以下檔案在實驗結束後未保留，因此不在本 repo 中：

- **Version 3 的 AXI-Lite 包裝原始碼**（Vivado「Create and Package New IP」產生的 `myip`）：`in_pack` / `out_pack` 對應的暫存器位移由此檔案決定，因此 Version 3 目前只提供 RTL 核心與 bitstream。
- **PYNQ 端的 Jupyter notebook**：上方「PYNQ 實測時間」的數據取自實驗報告。

## 使用工具

Vivado HLS 2018.3 · Vivado · Verilog HDL · PYNQ · Python · PYNQ-Z2（xc7z020clg400-1）
