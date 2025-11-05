# UART_NO_DMA プリプロセッサディレクティブの動作まとめ

## 重要な訂正事項

### プリプロセッサの意味

```c
#ifndef UART_NO_DMA  // ← "NOT defined" = UART_NO_DMAが未定義の場合
    // DMA関連コード（デフォルト）
#endif

#ifdef UART_NO_DMA   // ← "defined" = UART_NO_DMAが定義された場合
    // 純粋割り込みコールバック
#endif
```

### デフォルトの動作

| 状態 | プリプロセッサ | 結果 |
|------|-------------|------|
| **デフォルト**<br>（何も定義しない） | `#ifndef UART_NO_DMA` が真 | **DMAコードが有効**<br>（約5-8KB） |
| **UART_NO_DMA定義**<br>（`-DUART_NO_DMA`） | `#ifdef UART_NO_DMA` が真 | **純粋割り込みコールバックが有効**<br>（約3-4KB） |

---

## UARTMSPM0.cの実装構造

### ファイル全体の構造

```c
// UARTMSPM0.c の構造

/* ========== ヘッダー部 ========== */
#ifndef UART_NO_DMA  // ← デフォルトで真（DMAコード有効）
    #include <ti/drivers/dma/DMAMSPM0.h>
    // DMA関連の変数宣言
    static DMAMSPM0_Handle dmaHandle;
    static DMAMSPM0_Transfer DMATransfer;
    static DL_DMA_Config dlDMACfg;
#endif

/* ========== 初期化関数 ========== */
static bool UARTMSP_enable(UART_Handle handle, UART_Params *params)
{
    // ... 共通の初期化 ...

    #ifndef UART_NO_DMA  // ← デフォルトで実行される
    if ((params->readMode == UART_Mode_CALLBACK) && (dmaSupported == true))
    {
        // DMA初期化
        uartObject->DMA_Handle = DMA_Init(...);
        DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_DMA_DONE_RX);
    }
    if (params->readMode != UART_Mode_CALLBACK)
    {
        // 通常の割り込み
        DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_RX);
    }
    #else  // ← UART_NO_DMA定義時のみ実行
    // 常に通常の割り込みを使用
    DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_RX);
    #endif
}

/* ========== 送信開始関数 ========== */
static void UARTMSP_txChar(UART_Handle handle)
{
    #ifndef UART_NO_DMA  // ← デフォルトで実行される
    if (callbackObject->writeCallback != NULL)
    {
        // DMA送信
        UARTMSP_dmaTx(handle);
    }
    else
    {
        // 通常の割り込み送信
        DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
    }
    #else  // ← UART_NO_DMA定義時のみ実行
    // 常に通常の割り込み送信
    DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
    #endif
}

/* ========== DMA送信関数（デフォルトで有効） ========== */
#ifndef UART_NO_DMA  // ← デフォルトで真
void UARTMSP_dmaTx(UART_Handle handle)
{
    // DMA送信の実装
    DMAMSPM0_setupTransfer(&DMATransfer, &dlDMACfg);
    DL_UART_enableDMATransmitEvent(hwAttrs->regs);
}

void UARTMSP_dmaRx(UART_Handle handle, bool copyfifo)
{
    // DMA受信の実装
    DMAMSPM0_setupTransfer(&DMATransfer, &dlDMACfg);
    DL_UART_enableDMAReceiveEvent(hwAttrs->regs, DL_UART_DMA_INTERRUPT_RX);
}
#else  // ← UART_NO_DMA定義時のみ実行

/* ========== 純粋割り込み送信コールバック ========== */
void UARTMSP_TxCallback(UART_Handle handle)
{
    // 割り込みで1バイトずつ送信
    do {
        uint8_t txData = *(callbackObj->writeBuf + callbackObj->bytesWritten);
        if (callbackObj->writeSize == callbackObj->bytesWritten)
        {
            DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
            DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_EOT_DONE);
            break;
        }
        DL_UART_transmitData(hwAttrs->regs, txData);
        callbackObj->bytesWritten++;
    } while (!DL_UART_isTXFIFOFull(hwAttrs->regs));
}
#endif

/* ========== 割り込みハンドラ ========== */
void UARTMSP_interruptHandler(UART_Handle handle)
{
    uint32_t status = DL_UART_getEnabledInterruptStatus(hwAttrs->regs, 0x1FFFF);

    #ifndef UART_NO_DMA  // ← デフォルトで実行される
    /* DMA完了割り込み処理 */
    if (status & DL_UART_INTERRUPT_DMA_DONE_RX)
    {
        UARTMSP_dmaStopRx(handle);
        if (uartObject->readMode == UART_Mode_CALLBACK)
        {
            // コールバック呼び出し
            callbackObj->readCallback(handle, ...);
        }
    }

    if (status & DL_UART_INTERRUPT_DMA_DONE_TX)
    {
        UARTMSP_dmaStopTx(handle);
        if (uartObject->writeMode == UART_Mode_CALLBACK)
        {
            // コールバック呼び出し
            callbackObj->writeCallback(handle, ...);
        }
    }
    #endif

    /* 通常のRX割り込み処理 */
    if (status & DL_UART_INTERRUPT_RX)
    {
        while (!DL_UART_isRXFIFOEmpty(hwAttrs->regs))
        {
            uint8_t rxData = DL_UART_receiveData(hwAttrs->regs);
            RingBuf_put(&buffersObject->rxBuf, rxData);

            #ifdef UART_NO_DMA  // ← UART_NO_DMA定義時のみ実行
            /* 純粋割り込みでのコールバック処理 */
            if (callbackObj->readCallback != NULL)
            {
                count += RingBuf_getn(&buffersObject->rxBuf,
                                      callbackObj->readBuf + count,
                                      callbackObj->readSize - count);
                if (callbackObj->readSize == count)
                {
                    count = 0;
                    uartObject->readInUse = false;
                    // コールバック呼び出し
                    callbackObj->readCallback(handle, ...);
                }
            }
            #endif
        }
    }

    /* 通常のTX割り込み処理 */
    if (status & DL_UART_INTERRUPT_TX)
    {
        #ifdef UART_NO_DMA  // ← UART_NO_DMA定義時のみ実行
        if (callbackObj->writeCallback != NULL)
        {
            // 純粋割り込みでのコールバック送信
            UARTMSP_TxCallback(handle);
        }
        else
        {
        #endif
            // 通常の割り込み送信（ブロッキングモード用）
            do {
                uint8_t txData;
                if (RingBuf_get(&buffersObject->txBuf, &txData) == -1)
                {
                    DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
                    break;
                }
                DL_UART_transmitData(hwAttrs->regs, txData);
            } while (!DL_UART_isTXFIFOFull(hwAttrs->regs));
        #ifdef UART_NO_DMA
        }
        #endif
    }

    #ifndef UART_NO_DMA  // ← デフォルトで実行される
    /* DMA送信完了時のコールバック */
    if (status & DL_UART_INTERRUPT_EOT_DONE)
    {
        if (callbackObj->writeCallback != NULL)
        {
            if (uartObject->writeInUse != false)
            {
                uartObject->writeInUse = false;
                callbackObj->writeCallback(handle, ...);
            }
        }
    }
    #endif
}

/* ========== DMA停止関数（デフォルトで有効） ========== */
#ifndef UART_NO_DMA
void UARTMSP_dmaStopRx(UART_Handle handle)
{
    DMAMSPM0_disableChannel(uartObject->DMA_Handle, uartObject->rxDmaChannel);
    // ...
}

void UARTMSP_dmaStopTx(UART_Handle handle)
{
    DMAMSPM0_disableChannel(uartObject->DMA_Handle, uartObject->txDmaChannel);
    // ...
}
#endif
```

---

## コンパイル時のコード展開

### デフォルト（UART_NO_DMA未定義）

```c
// -DUART_NO_DMA を指定しない（デフォルト）

// コンパイル後の展開：
#include <ti/drivers/dma/DMAMSPM0.h>  // ← 有効
static DMAMSPM0_Handle dmaHandle;      // ← 有効

void UARTMSP_dmaTx(UART_Handle handle) { /* DMA実装 */ }  // ← 有効
void UARTMSP_dmaRx(UART_Handle handle, bool copyfifo) { /* DMA実装 */ }  // ← 有効
void UARTMSP_dmaStopRx(UART_Handle handle) { /* DMA停止 */ }  // ← 有効
void UARTMSP_dmaStopTx(UART_Handle handle) { /* DMA停止 */ }  // ← 有効

void UARTMSP_interruptHandler(UART_Handle handle)
{
    // DMA完了割り込み処理が含まれる ← 有効
    if (status & DL_UART_INTERRUPT_DMA_DONE_RX) { /* ... */ }
    if (status & DL_UART_INTERRUPT_DMA_DONE_TX) { /* ... */ }

    // 純粋割り込みコールバックは含まれない ← 無効
    // （#ifdef UART_NO_DMA ブロックが除外される）
}

// UARTMSP_TxCallback関数は含まれない ← 無効
```

### UART_NO_DMA定義時

```c
// -DUART_NO_DMA を指定

// コンパイル後の展開：
// #include <ti/drivers/dma/DMAMSPM0.h>  ← 除外
// static DMAMSPM0_Handle dmaHandle;     ← 除外

// void UARTMSP_dmaTx(...) { ... }       ← 除外
// void UARTMSP_dmaRx(...) { ... }       ← 除外
// void UARTMSP_dmaStopRx(...) { ... }   ← 除外
// void UARTMSP_dmaStopTx(...) { ... }   ← 除外

void UARTMSP_TxCallback(UART_Handle handle) { /* 割り込み実装 */ }  // ← 有効

void UARTMSP_interruptHandler(UART_Handle handle)
{
    // DMA完了割り込み処理は除外される ← 無効
    // if (status & DL_UART_INTERRUPT_DMA_DONE_RX) { ... }  ← 除外
    // if (status & DL_UART_INTERRUPT_DMA_DONE_TX) { ... }  ← 除外

    if (status & DL_UART_INTERRUPT_RX)
    {
        // 純粋割り込みコールバックが含まれる ← 有効
        if (callbackObj->readCallback != NULL)
        {
            /* データをコピーしてコールバック呼び出し */
        }
    }

    if (status & DL_UART_INTERRUPT_TX)
    {
        // 純粋割り込みコールバックが含まれる ← 有効
        if (callbackObj->writeCallback != NULL)
        {
            UARTMSP_TxCallback(handle);
        }
    }
}
```

---

## 実行時の動作フロー

### デフォルト（DMAモード）

```
アプリケーション
    ↓
UART_read(uart, buffer, 100, NULL)  // コールバックモード
    ↓
[ドライバ内部]
    ↓
UARTMSP_dmaRx() 呼び出し
    ↓
DMA設定（100バイト転送）
    ↓
DL_UART_enableDMAReceiveEvent()
    ↓
━━━━━━━━━━━━━━━━━━━━━━━
[ハードウェア]
    ↓
UARTがRXデータ受信
    ↓
DMAトリガー発行
    ↓
DMAが自動的にUART→メモリへ転送（CPUは他の処理を実行中）
    ↓
100バイト転送完了
    ↓
DMA完了割り込み発生
━━━━━━━━━━━━━━━━━━━━━━━
    ↓
[割り込みハンドラ]
    ↓
UARTMSP_interruptHandler()
    ↓
status & DL_UART_INTERRUPT_DMA_DONE_RX
    ↓
UARTMSP_dmaStopRx()
    ↓
readCallback() 呼び出し ★
    ↓
アプリケーションのコールバック関数実行
```

### UART_NO_DMA定義時（純粋割り込みモード）

```
アプリケーション
    ↓
UART_read(uart, buffer, 100, NULL)  // コールバックモード
    ↓
[ドライバ内部]
    ↓
DL_UART_enableInterrupt(DL_UART_INTERRUPT_RX)
    ↓
━━━━━━━━━━━━━━━━━━━━━━━
[ハードウェア]
    ↓
UARTがRXデータ受信（1バイト目）
    ↓
RX割り込み発生 ★1回目
━━━━━━━━━━━━━━━━━━━━━━━
    ↓
[割り込みハンドラ]
    ↓
UARTMSP_interruptHandler()
    ↓
status & DL_UART_INTERRUPT_RX
    ↓
DL_UART_receiveData() → リングバッファへ格納
    ↓
#ifdef UART_NO_DMA ブロック実行
    ↓
RingBuf_getn() → ユーザーバッファへコピー
    ↓
まだ100バイト未満 → リターン
━━━━━━━━━━━━━━━━━━━━━━━
    ↓
UARTがRXデータ受信（2バイト目）
    ↓
RX割り込み発生 ★2回目
    ↓
... （同じ処理を繰り返し） ...
━━━━━━━━━━━━━━━━━━━━━━━
    ↓
UARTがRXデータ受信（100バイト目）
    ↓
RX割り込み発生 ★100回目
━━━━━━━━━━━━━━━━━━━━━━━
    ↓
[割り込みハンドラ]
    ↓
UARTMSP_interruptHandler()
    ↓
DL_UART_receiveData() → リングバッファへ格納
    ↓
RingBuf_getn() → ユーザーバッファへコピー
    ↓
callbackObj->readSize == count  // 100バイト達成
    ↓
readCallback() 呼び出し ★
    ↓
アプリケーションのコールバック関数実行
```

---

## まとめ表

| 項目 | デフォルト<br>（UART_NO_DMA未定義） | UART_NO_DMA定義 |
|------|---------------------------|---------------|
| **プリプロセッサ** | `#ifndef UART_NO_DMA` = 真 | `#ifdef UART_NO_DMA` = 真 |
| **DMAコード** | 有効（コンパイルされる） | 無効（除外される） |
| **純粋割り込みコールバック** | 無効（除外される） | 有効（コンパイルされる） |
| **コードサイズ** | 約5-8 KB | 約3-4 KB |
| **割り込み頻度（100バイト受信）** | 1回（DMA完了時） | 約100回（1バイト毎） |
| **CPU負荷** | 低 | 高 |
| **転送速度** | 高速 | 中速 |
| **DMAチャネル使用** | 使用 | 不使用 |
| **推奨用途** | 高速通信・本番環境 | メモリ制約・DMA節約 |

---

## コンパイルオプションの設定方法

### Code Composer Studio (CCS)

```
Project Properties
  → Build
    → ARM Compiler
      → Advanced Options
        → Predefined Symbols
          → Add: UART_NO_DMA
```

### Makefile

```makefile
# DMA有効（デフォルト）
CFLAGS = -O2 -Wall

# DMA無効（純粋割り込みコールバック）
CFLAGS = -O2 -Wall -DUART_NO_DMA
```

### ソースコード直接定義（非推奨）

```c
// ti_drivers_config.c の先頭（#include より前）
#define UART_NO_DMA
#include "ti_drivers_config.h"
```

**注意**: ソースコード直接定義は、すべての関連ファイルで統一する必要があるため推奨されません。

---

**作成日:** 2025-11-06
**ファイル:** UARTMSPM0.c
**SDK:** MSPM0 SDK
