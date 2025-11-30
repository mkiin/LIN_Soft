# UART_LIN_1BYTE ドライバ使用ガイド

## 概要

`UART_LIN_1BYTE`は、TI CC2340R53向けのLIN通信専用1バイトUARTドライバです。

### 主な特徴

1. **FIFO無効化による1バイト単位の送受信**
   - 1バイト送信毎に割り込みが発生
   - 1バイト受信毎に割り込みが発生
   - H850のLINライブラリと同じ動作を実現

2. **Half-Duplex通信サポート**
   - 送信したデータを自動的に受信（リードバック）
   - BITエラー検出に必須

3. **柔軟な設定**
   - ボーレート、パリティ、ストップビット、データ長を自由に設定可能
   - コールバック関数による非同期処理

---

## ファイル構成

| ファイル名 | 説明 |
|-----------|------|
| `UART_LIN_1BYTE.h` | ヘッダファイル（API定義） |
| `UART_LIN_1BYTE.c` | 実装ファイル |
| `UART_LIN_1BYTE_USAGE.md` | 本ドキュメント |

---

## 基本的な使用方法

### 1. ハンドル定義

```c
#include "UART_LIN_1BYTE.h"

/* UARTハンドル */
UART_LIN_Handle_t uartHandle;
```

### 2. コールバック関数定義

```c
/**
 * @brief 受信コールバック
 */
void linRxCallback(uint8_t data, uint8_t error)
{
    if (error == 0)
    {
        /* エラーなし: データ処理 */
        // SLIN API の受信割り込みハンドラを呼び出す
        l_vog_lin_rx_int(data, 0);
    }
    else
    {
        /* エラーあり */
        uint8_t linError = 0;

        if (error & UART_LIN_ERR_OVERRUN)
        {
            linError |= U1G_LIN_OVERRUN_ERR;
        }
        if (error & UART_LIN_ERR_FRAMING)
        {
            linError |= U1G_LIN_FRAMING_ERR;
        }
        if (error & UART_LIN_ERR_PARITY)
        {
            linError |= U1G_LIN_PARITY_ERR;
        }

        l_vog_lin_rx_int(data, linError);
    }
}

/**
 * @brief 送信完了コールバック
 */
void linTxCallback(void)
{
    /* 送信完了処理 */
    // 次のバイト送信などを行う
}
```

### 3. UART初期化

```c
void initLINUart(void)
{
    UART_LIN_Config config;
    int result;

    /* UART設定 */
    config.baseAddr    = UART0_BASE;           /* UART0を使用 */
    config.baudRate    = 9600;                 /* 9600 bps */
    config.dataLen     = UART_LIN_DATALEN_8;   /* 8ビット */
    config.stopBits    = UART_LIN_STOPBITS_1;  /* 1ストップビット */
    config.parity      = UART_LIN_PARITY_NONE; /* パリティなし */
    config.rxCallback  = linRxCallback;        /* 受信コールバック */
    config.txCallback  = linTxCallback;        /* 送信完了コールバック */
    config.halfDuplex  = true;                 /* Half-Duplex有効（LIN必須） */

    /* UART初期化 */
    result = UART_LIN_init(&uartHandle, &config);

    if (result != UART_LIN_OK)
    {
        /* 初期化失敗 */
        while (1);
    }

    /* 受信許可 */
    UART_LIN_enableRx(&uartHandle);
}
```

### 4. 割り込みハンドラ登録

```c
#include <ti/drivers/dpl/HwiP.h>

/* UART割り込みハンドラ */
void UART0_IRQHandler(void)
{
    UART_LIN_hwi(&uartHandle);
}

/* HwiP登録例 */
void setupUartInterrupt(void)
{
    HwiP_Params hwiParams;

    HwiP_Params_init(&hwiParams);
    hwiParams.priority = 1;  /* 優先度設定 */

    HwiP_construct(&uartHwiStruct, INT_UART0_COMB, UART0_IRQHandler, &hwiParams);
}
```

### 5. データ送信

```c
/**
 * @brief 1バイト送信
 */
void sendByte(uint8_t data)
{
    int result;

    result = UART_LIN_writeByte(&uartHandle, data);

    if (result != UART_LIN_OK)
    {
        /* 送信失敗（送信中） */
    }
}
```

### 6. データ受信（ポーリング）

```c
/**
 * @brief 1バイト受信（ポーリング）
 */
int receiveByte(uint8_t *data)
{
    uint8_t error;
    int result;

    result = UART_LIN_readByte(&uartHandle, data, &error);

    if (result == UART_LIN_RX_COMPLETE)
    {
        if (error == 0)
        {
            /* 受信成功 */
            return 0;
        }
        else
        {
            /* エラーあり */
            return -1;
        }
    }
    else
    {
        /* データなし */
        return -2;
    }
}
```

---

## LIN DRV層への組み込み

### l_slin_drv_cc2340r53.c の修正

#### 1. ヘッダインクルード

```c
#include "UART_LIN_1BYTE.h"

/* グローバル変数 */
static UART_LIN_Handle_t xnl_lin_uart_handle;
```

#### 2. UART初期化関数の修正

```c
void l_vog_lin_uart_init(void)
{
    UART_LIN_Config config;

    /* UART設定 */
    config.baseAddr    = UART0_BASE;
    config.baudRate    = 9600;  /* U1G_LIN_BAUDRATE に応じて変更 */
    config.dataLen     = UART_LIN_DATALEN_8;
    config.stopBits    = UART_LIN_STOPBITS_1;
    config.parity      = UART_LIN_PARITY_NONE;
    config.rxCallback  = l_ifc_rx_ch1;  /* 受信コールバック */
    config.txCallback  = l_ifc_tx_ch1;  /* 送信完了コールバック */
    config.halfDuplex  = true;          /* Half-Duplex有効 */

    /* UART初期化 */
    if (UART_LIN_init(&xnl_lin_uart_handle, &config) != UART_LIN_OK)
    {
        /* エラー処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_UART;
    }
}
```

#### 3. 送信関数の修正

```c
void l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    /* 1バイト送信 */
    UART_LIN_writeByte(&xnl_lin_uart_handle, u1a_lin_data);
}
```

#### 4. 受信コールバック関数

```c
/**
 * @brief UART受信コールバック（割り込みコンテキスト）
 */
void l_ifc_rx_ch1(uint8_t data, uint8_t error)
{
    l_u8 linError = 0;

    /* エラー変換 */
    if (error & UART_LIN_ERR_OVERRUN)
    {
        linError |= U1G_LIN_OVERRUN_ERR;
    }
    if (error & UART_LIN_ERR_FRAMING)
    {
        linError |= U1G_LIN_FRAMING_ERR;
    }
    if (error & UART_LIN_ERR_PARITY)
    {
        linError |= U1G_LIN_PARITY_ERR;
    }

    /* SLIN CORE層の受信割り込みハンドラ呼び出し */
    l_vog_lin_rx_int(data, linError);
}
```

#### 5. 送信完了コールバック関数

```c
/**
 * @brief UART送信完了コールバック（割り込みコンテキスト）
 */
void l_ifc_tx_ch1(void)
{
    /* 送信完了処理（必要に応じて） */
}
```

#### 6. 受信許可/禁止関数の修正

```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx)
{
    if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE)
    {
        UART_LIN_flushRx(&xnl_lin_uart_handle);
    }

    UART_LIN_enableRx(&xnl_lin_uart_handle);
}

void l_vog_lin_rx_dis(void)
{
    UART_LIN_disableRx(&xnl_lin_uart_handle);
}
```

#### 7. リードバック関数の修正

```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    uint8_t rxData;
    uint8_t error;
    int result;

    /* 受信データ取得 */
    result = UART_LIN_readByte(&xnl_lin_uart_handle, &rxData, &error);

    if (result != UART_LIN_RX_COMPLETE)
    {
        /* 受信データなし */
        return U1G_LIN_NG;
    }

    /* エラーチェック */
    if (error != 0)
    {
        return U1G_LIN_NG;
    }

    /* データ比較 */
    if (rxData != u1a_lin_data)
    {
        return U1G_LIN_NG;
    }

    return U1G_LIN_OK;
}
```

#### 8. 割り込みハンドラの登録

```c
void l_vog_lin_init_interrupt(void)
{
    HwiP_Params hwiParams;

    HwiP_Params_init(&hwiParams);
    hwiParams.priority = 1;

    /* UART割り込みハンドラ登録 */
    HwiP_construct(&xnl_lin_uart_hwi, INT_UART0_COMB,
                   (HwiP_Fxn)UART_LIN_hwi_wrapper, &hwiParams);
}

/* 割り込みハンドララッパー */
void UART_LIN_hwi_wrapper(void)
{
    UART_LIN_hwi(&xnl_lin_uart_handle);
}
```

---

## 動作確認方法

### 1. ループバックテスト

```c
void loopbackTest(void)
{
    uint8_t txData = 0x55;
    uint8_t rxData;
    uint8_t error;
    int result;

    /* 送信 */
    UART_LIN_writeByte(&uartHandle, txData);

    /* 少し待つ */
    ClockP_usleep(1000);

    /* 受信（Half-Duplexモードなら送信データが読み戻せる） */
    result = UART_LIN_readByte(&uartHandle, &rxData, &error);

    if (result == UART_LIN_RX_COMPLETE && error == 0 && rxData == txData)
    {
        /* ループバック成功 */
    }
}
```

### 2. BREAK信号送信テスト

```c
void sendBreak(void)
{
    uint32_t baseAddr = uartHandle.config.baseAddr;

    /* BRKビット設定（BREAK信号送信） */
    HWREG(baseAddr + UART_O_LCRH) |= UART_LCRH_BRK;

    /* 少し待つ（13ビット分以上） */
    ClockP_usleep(2000);

    /* BRKビットクリア */
    HWREG(baseAddr + UART_O_LCRH) &= ~UART_LCRH_BRK;
}
```

---

## トラブルシューティング

### 問題1: 受信割り込みが発生しない

**原因:**
- 受信が許可されていない
- FIFOが無効化されていない
- 割り込みハンドラが登録されていない

**対策:**
```c
/* 受信許可確認 */
UART_LIN_enableRx(&uartHandle);

/* FIFO無効化確認 */
uint32_t lcrh = HWREG(uartHandle.config.baseAddr + UART_O_LCRH);
if (lcrh & UART_LCRH_FEN)
{
    /* FIFOが有効になっている（異常） */
}

/* 割り込みマスク確認 */
uint32_t imsc = HWREG(uartHandle.config.baseAddr + UART_O_IMSC);
if (!(imsc & UART_INT_RX))
{
    /* 受信割り込みがマスクされている（異常） */
}
```

### 問題2: 送信が完了しない

**原因:**
- 送信許可されていない
- 送信中フラグがクリアされていない

**対策:**
```c
/* 送信許可確認 */
UART_LIN_enableTx(&uartHandle);

/* 送信中フラグ強制クリア */
uartHandle.txInProgress = false;
```

### 問題3: エラーが頻発する

**原因:**
- ボーレート不一致
- パリティ設定ミス
- ノイズ

**対策:**
```c
/* エラーステータス確認 */
uint8_t error = UART_LIN_getErrorStatus(&uartHandle);

if (error & UART_LIN_ERR_FRAMING)
{
    /* ボーレート不一致の可能性 */
}
if (error & UART_LIN_ERR_PARITY)
{
    /* パリティ設定ミスの可能性 */
}

/* エラークリア */
UART_LIN_clearErrorStatus(&uartHandle);
```

---

## パフォーマンス

### 割り込みオーバーヘッド

| ボーレート | 1バイト送受信時間 | 割り込み頻度 |
|-----------|-----------------|-------------|
| 2400 bps  | 4.17 ms         | 最大240 Hz  |
| 9600 bps  | 1.04 ms         | 最大960 Hz  |
| 19200 bps | 0.52 ms         | 最大1920 Hz |

**注意:** FIFO無効化により、1バイト毎に割り込みが発生します。
高ボーレートでは割り込みオーバーヘッドが増加するため、CPU負荷に注意してください。

---

## 制限事項

1. **DMA非対応**
   - FIFO無効化のため、DMAは使用できません
   - 全てポーリングまたは割り込み駆動で処理します

2. **フロー制御非対応**
   - CTS/RTSフロー制御は未実装です
   - LIN通信では通常不要です

3. **1ポートのみ**
   - 複数UARTポートの同時使用は、各ハンドルを別々に管理すれば可能ですが、
     割り込みハンドラの登録に注意が必要です

---

## まとめ

`UART_LIN_1BYTE`ドライバを使用することで、以下のメリットがあります：

1. **H850のLINライブラリと同じ動作**
   - 1バイト単位の送受信
   - Half-Duplexリードバック対応

2. **シンプルなAPI**
   - 初期化、送信、受信、コールバックの4要素のみ

3. **柔軟な設定**
   - ボーレート、パリティ、ストップビット、データ長を自由に設定

このドライバを使用すれば、CC23xxでもH850と同じLIN通信スタックが動作します。

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: UART_LIN_1BYTE Driver Development Team
