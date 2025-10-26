# TI CC2340R53 UART関連ファイル調査結果

## 調査概要

TIフォルダ内のCC2340R53関連ファイルからUART実装に必要な情報を抽出しました。

## 重要なUART関連ファイル一覧

### 1. ハードウェア定義ファイル

#### hw_ints.h - 割り込み番号定義
**パス**: `/TI/ti/devices/cc23x0r5/inc/hw_ints.h`

```c
#define INT_UART0_COMB          27  // UART0 combined interrupt
                                    // interrupt flags are found here UART0:MIS
```

**重要事項**:
- ✅ UART0の割り込み番号は **27**
- ✅ 割り込み名は `INT_UART0_COMB`
- これは複合割り込み（RX, TX, エラーすべて含む）

#### hw_memmap.h - メモリマップ定義
**パス**: `/TI/ti/devices/cc23x0r5/inc/hw_memmap.h`

```c
#define UART0_BASE              0x40034000  // UART
```

**重要事項**:
- ✅ UART0のベースアドレスは **0x40034000**
- ⚠️ **注意**: 以前のパッチファイルで使用していた`0x40008000`は間違い！

#### hw_uart.h - UARTレジスタ定義
**パス**: `/TI/ti/devices/cc23x0r5/inc/hw_uart.h`

すでにプロジェクトで使用中（`TI_UART_Driver/hw_uart.h`と同じ）

### 2. ドライバライブラリ

#### uart.h - UART DriverLib API
**パス**: `/TI/ti/devices/cc23x0r5/driverlib/uart.h`

**主要API関数**:

```c
// 設定関数
extern void UARTConfigSetExpClk(uint32_t base, uint32_t UARTClkFreq, 
                                 uint32_t baudFreq, uint32_t config);

// FIFO制御
__STATIC_INLINE void UARTEnableFifo(uint32_t base);
__STATIC_INLINE void UARTDisableFIFO(uint32_t base);
__STATIC_INLINE void UARTSetFifoLevel(uint32_t base, uint32_t txLevel, uint32_t rxLevel);

// データ送受信
__STATIC_INLINE bool UARTCharAvailable(uint32_t base);
__STATIC_INLINE uint8_t UARTGetCharNonBlocking(uint32_t base);
__STATIC_INLINE bool UARTSpaceAvailable(uint32_t base);
__STATIC_INLINE void UARTPutCharNonBlocking(uint32_t base, uint8_t data);

// 割り込み制御
extern void UARTEnableInt(uint32_t base, uint32_t intFlags);
extern void UARTDisableInt(uint32_t base, uint32_t intFlags);
extern uint32_t UARTIntStatus(uint32_t base, bool masked);
extern void UARTClearInt(uint32_t base, uint32_t intFlags);

// エラー処理
extern uint32_t UARTGetRxError(uint32_t base);
extern void UARTClearRxError(uint32_t base);

// 有効化/無効化
extern void UARTEnable(uint32_t base);
extern void UARTDisable(uint32_t base);
```

**割り込みフラグ**:
```c
#define UART_INT_EOT       (UART_IMSC_EOT)       // End Of Transmission
#define UART_INT_OE        (UART_IMSC_OE)        // Overrun Error
#define UART_INT_BE        (UART_IMSC_BE)        // Break Error
#define UART_INT_PE        (UART_IMSC_PE)        // Parity Error
#define UART_INT_FE        (UART_IMSC_FE)        // Framing Error
#define UART_INT_RT        (UART_IMSC_RT)        // Receive Timeout
#define UART_INT_RX        (UART_IMSC_RX)        // Receive
#define UART_INT_TX        (UART_IMSC_TX)        // Transmit
#define UART_INT_CTS       (UART_IMSC_CTSM)      // CTS Modem
#define UART_INT_TXDMADONE (UART_IMSC_TXDMADONE) // Tx DMA done
#define UART_INT_RXDMADONE (UART_IMSC_RXDMADONE) // Rx DMA done
```

**FIFOレベル設定**:
```c
// TX FIFOトリガーレベル
#define UART_FIFO_TX2_8 0x00000001  // 1/4 Full (2/8)
#define UART_FIFO_TX4_8 0x00000002  // 1/2 Full (4/8)
#define UART_FIFO_TX6_8 0x00000003  // 3/4 Full (6/8)

// RX FIFOトリガーレベル
#define UART_FIFO_RX2_8 0x00000008  // 1/4 Full (2/8)
#define UART_FIFO_RX4_8 0x00000010  // 1/2 Full (4/8)
#define UART_FIFO_RX6_8 0x00000018  // 3/4 Full (6/8)
```

#### uart.c - UART DriverLib実装
**パス**: `/TI/ti/devices/cc23x0r5/driverlib/uart.c`

実装ファイル（必要に応じてリンク）

### 3. Powerドライバ

#### PowerCC23X0.h - 電源管理
**パス**: `/TI/ti/drivers/power/PowerCC23X0.h`

```c
#define PowerLPF3_PERIPH_UART0 (PowerCC23X0_PERIPH_GROUP_CLKCTL0 | CLKCTL_DESCEX0_UART0_S)
```

**使用方法**:
```c
// UART0のクロック有効化
Power_setDependency(PowerLPF3_PERIPH_UART0);

// UART0のクロック無効化
Power_releaseDependency(PowerLPF3_PERIPH_UART0);
```

### 4. 既存のUART2ドライバ（参考用）

#### UART2LPF3.c/h - UART2ドライバ実装
**パス**: `/TI/ti/drivers/uart2/UART2LPF3.c`

DMA使用の実装例（参考用）

## レジスタ割り込み実装に必要な修正

### 修正1: ベースアドレスの訂正

**パッチファイル**: `register_interrupt_patch.c`

**誤り**:
```c
u4l_lin_uart_base_addr = 0x40008000U;  // ❌ 間違い！
```

**正しい値**:
```c
u4l_lin_uart_base_addr = 0x40034000U;  // ✅ 正しいUART0ベースアドレス
// または
u4l_lin_uart_base_addr = UART0_BASE;   // ✅ マクロ使用（推奨）
```

### 修正2: インクルードファイルの追加

**ファイル**: `l_slin_drv_cc2340r53.c`

```c
/* 既存のインクルード */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/timer/LGPTimerLPF3.h>
#include <ti/drivers/dpl/HwiP.h>

/* 追加すべきインクルード */
#include <ti/devices/cc23x0r5/inc/hw_memmap.h>  // UART0_BASE定義
#include <ti/devices/cc23x0r5/inc/hw_ints.h>    // INT_UART0_COMB定義
#include <ti/devices/cc23x0r5/inc/hw_uart.h>    // UARTレジスタ定義
#include <ti/devices/cc23x0r5/driverlib/uart.h> // UART API関数
#include <ti/drivers/power/PowerCC23X0.h>       // PowerLPF3_PERIPH_UART0
```

### 修正3: 初期化関数の正しい実装

```c
void l_vog_lin_uart_init(void)
{
    HwiP_Params hwiParams;
    ClockP_FreqHz freq;
    
    if(FALSE == b1l_lin_uart_initialized)
    {
        l_u1g_lin_irq_dis();
        
        /* ✅ 正しいベースアドレスを使用 */
        u4l_lin_uart_base_addr = UART0_BASE;  // 0x40034000
        
        /* ✅ 正しい電源依存設定 */
        Power_setDependency(PowerLPF3_PERIPH_UART0);
        
        /* UART無効化 */
        UARTDisable(u4l_lin_uart_base_addr);
        
        /* CPUクロック周波数取得 */
        ClockP_getCpuFreq(&freq);
        
        /* ボーレート、データフォーマット設定 */
        UARTConfigSetExpClk(u4l_lin_uart_base_addr,
                            freq.lo,  // CC23X0ではfreq.loをそのまま使用
                            U4L_LIN_BAUDRATE,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
        
        /* FIFO有効化 */
        UARTEnableFifo(u4l_lin_uart_base_addr);
        
        /* FIFOレベル設定 */
        UARTSetFifoLevel(u4l_lin_uart_base_addr, UART_FIFO_TX2_8, UART_FIFO_RX6_8);
        
        /* すべての割り込みをクリア */
        UARTClearInt(u4l_lin_uart_base_addr,
                     UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | 
                     UART_INT_RT | UART_INT_TX | UART_INT_RX | UART_INT_CTS);
        
        /* ✅ 正しい割り込み番号を使用 */
        HwiP_Params_init(&hwiParams);
        hwiParams.arg      = (uintptr_t)0;
        hwiParams.priority = 0x20;
        HwiP_construct(&xnl_lin_uart_hwi, INT_UART0_COMB, l_vog_lin_uart_isr, &hwiParams);
        
        /* UART有効化 */
        HWREG(u4l_lin_uart_base_addr + UART_O_CTL) |= UART_CTL_UARTEN;
        
        b1l_lin_uart_initialized = TRUE;
        
        l_vog_lin_irq_res();
    }
}
```

### 修正4: クローズ関数の正しい実装

```c
void l_ifc_uart_close(void)
{
    if(b1l_lin_uart_initialized)
    {
        l_u1g_lin_irq_dis();
        
        /* すべての割り込みを無効化 */
        UARTDisableInt(u4l_lin_uart_base_addr,
                       UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | 
                       UART_INT_RT | UART_INT_TX | UART_INT_RX | UART_INT_CTS);
        
        /* UART無効化 */
        UARTDisable(u4l_lin_uart_base_addr);
        
        /* 割り込みハンドラ削除 */
        HwiP_destruct(&xnl_lin_uart_hwi);
        
        /* ✅ 正しい電源依存解除 */
        Power_releaseDependency(PowerLPF3_PERIPH_UART0);
        
        b1l_lin_uart_initialized = FALSE;
        
        l_vog_lin_irq_res();
    }
}
```

## CC2340R53とCC27XXの違い

### クロック周波数の扱い

**CC27XX** (別デバイス):
```c
ClockP_getCpuFreq(&freq);
freq.lo /= 2;  // UARTクロックはSVTCLKの半分
UARTConfigSetExpClk(base, freq.lo, ...);
```

**CC2340R53** (CC23X0ファミリー):
```c
ClockP_getCpuFreq(&freq);
// freq.loをそのまま使用（÷2不要）
UARTConfigSetExpClk(base, freq.lo, ...);
```

### 電源依存マクロ

**CC27XX**:
```c
#define PowerLPF3_PERIPH_UART0 (PowerCC27XX_PERIPH_GROUP_CLKCTL0 | ...)
#define PowerLPF3_PERIPH_UART1 (PowerCC27XX_PERIPH_GROUP_CLKCTL0 | ...)
```

**CC2340R53**:
```c
#define PowerLPF3_PERIPH_UART0 (PowerCC23X0_PERIPH_GROUP_CLKCTL0 | ...)
// UART1は存在しない
```

## レジスタアクセス方法

### データレジスタへの直接アクセス

```c
// 送信
HWREG(u4l_lin_uart_base_addr + UART_O_DR) = data;

// 受信
uint8_t data = (l_u8)HWREG(u4l_lin_uart_base_addr + UART_O_DR);

// または便利関数を使用
UARTPutCharNonBlocking(u4l_lin_uart_base_addr, data);
uint8_t data = UARTGetCharNonBlocking(u4l_lin_uart_base_addr);
```

### ステータスチェック

```c
// RX FIFO空チェック
if(UARTCharAvailable(u4l_lin_uart_base_addr)) {
    // データあり
}

// TX FIFO空きチェック
if(UARTSpaceAvailable(u4l_lin_uart_base_addr)) {
    // 送信可能
}
```

### エラーステータス

```c
// エラー読み取り
uint32_t err = HWREG(u4l_lin_uart_base_addr + UART_O_RSR_ECR);

// または
uint32_t err = UARTGetRxError(u4l_lin_uart_base_addr);

// エラークリア
UARTClearRxError(u4l_lin_uart_base_addr);

// エラービット
if(err & UART_RSR_ECR_OE) { /* オーバーラン */ }
if(err & UART_RSR_ECR_FE) { /* フレーミング */ }
if(err & UART_RSR_ECR_PE) { /* パリティ */ }
```

## 実装時の注意点まとめ

### ✅ 確認済み事項

1. **UART0ベースアドレス**: `0x40034000` (UART0_BASE)
2. **割り込み番号**: `27` (INT_UART0_COMB)
3. **電源マクロ**: `PowerLPF3_PERIPH_UART0`
4. **クロック設定**: CC23X0では`freq.lo`をそのまま使用（÷2不要）

### ⚠️ 修正が必要な箇所

1. **パッチファイル** (`register_interrupt_patch.c`)
   - ベースアドレスを`0x40008000`から`UART0_BASE`に変更
   - 電源依存を`PowerLPF3_PERIPH_UART0`に変更
   - 割り込み番号を`INT_UART0_COMB`に変更
   - クロック周波数の÷2処理を削除（CC23X0には不要）

2. **インクルードファイル**
   - `PowerCC23X0.h`を追加

### 📁 必要なリンク

ビルド時に以下のライブラリが必要：
- driverlib: `uart.c`の実装
- Power driver: 電源管理

## 次のステップ

1. ✅ パッチファイルのベースアドレスを修正
2. ✅ パッチファイルの電源依存を修正
3. ✅ パッチファイルのクロック設定を修正
4. ✅ 更新された実装ガイドを作成

これらの修正を反映した新しいパッチファイルを作成します。
