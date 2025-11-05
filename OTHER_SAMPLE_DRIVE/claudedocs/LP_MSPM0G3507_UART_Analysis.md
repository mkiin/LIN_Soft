# LP_MSPM0G3507 UART実装分析

## 概要
LP_MSPM0G3507のUART実装について、DMAの使用有無と純粋な割り込みベースの実装について調査した結果をまとめます。

---

## UART実装例の比較

MSPM0 SDKには、LP_MSPM0G3507向けに**2つのUART実装例**が提供されています：

### 1. uart_echo - 純粋な割り込みベース実装（DMA不使用）

**パス:** [examples/rtos/LP_MSPM0G3507/drivers/uart_echo/](../examples/rtos/LP_MSPM0G3507/drivers/uart_echo/)

#### 特徴
- **DMAを使用しない**純粋な割り込みベースの実装
- ブロッキングモードで動作
- シンプルなエコーアプリケーション

#### 設定 (ti_drivers_config.c:68-86)
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .supportFxns        = &UARTMSPSupportFxns,
            .buffersSupported   = true,
            .eventsSupported    = false,
            .callbacksSupported = false,  // ← コールバック無効
            .dmaSupported       = false,  // ← DMA無効（純粋な割り込み）
        },
        .buffersObject = {
            .rxBufPtr  = rxBuffer,
            .txBufPtr  = txBuffer,
            .rxBufSize = sizeof(rxBuffer),
            .txBufSize = sizeof(txBuffer),
        },
    },
};
```

#### 使用される割り込み
- `DL_UART_INTERRUPT_RX` - 受信割り込み
- `DL_UART_INTERRUPT_TX` - 送信割り込み

#### アプリケーションコード (uartecho.c:103-157)
```c
void mainThread(void *arg0)
{
    char input;
    UART_Handle uart;
    UART_Params uartParams;
    size_t bytesRead, bytesWritten;

    // UART初期化
    UART_Params_init(&uartParams);
    uartParams.baudRate = CONFIG_UART_BAUD_RATE;  // 115200

    uart = UART_open(CONFIG_UART_0, &uartParams);
    if (uart == NULL) {
        __BKPT();
    }

    // エコーループ
    while (1) {
        // ブロッキング読み込み（割り込みで実装）
        bytesRead = 0;
        while (bytesRead == 0) {
            status = UART_read(uart, &input, 1, &bytesRead);
        }

        // ブロッキング書き込み（割り込みで実装）
        bytesWritten = 0;
        while (bytesWritten == 0) {
            status = UART_write(uart, &input, 1, &bytesWritten);
        }
    }
}
```

---

### 2. uart_callback - DMA使用実装

**パス:** [examples/rtos/LP_MSPM0G3507/drivers/uart_callback/](../examples/rtos/LP_MSPM0G3507/drivers/uart_callback/)

#### 特徴
- **DMAを使用**した非ブロッキング実装
- コールバックモードで動作
- セマフォによる同期制御

#### 設定 (ti_drivers_config.c:106-125)
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .supportFxns        = &UARTMSPSupportFxns,
            .buffersSupported   = true,
            .eventsSupported    = false,
            .callbacksSupported = true,   // ← コールバック有効
            .dmaSupported       = true,   // ← DMA有効
            .noOfDMAChannels    = 1,
        },
        .buffersObject = {
            .rxBufPtr  = rxBuffer,
            .txBufPtr  = txBuffer,
            .rxBufSize = sizeof(rxBuffer),
            .txBufSize = sizeof(txBuffer),
        },
    },
};
```

#### DMA設定 (ti_drivers_config.c:47-64)
```c
DMAMSPM0_Object DMAObject[CONFIG_DMA_CH_COUNT] = {
    {.dmaTransfer = {
        .txTrigger              = DMA_UART0_TX_TRIG,
        .txTriggerType          = DL_DMA_TRIGGER_TYPE_EXTERNAL,
        .rxTrigger              = DMA_UART0_RX_TRIG,
        .rxTriggerType          = DL_DMA_TRIGGER_TYPE_EXTERNAL,
        .transferMode           = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode           = DL_DMA_NORMAL_MODE,
        .destWidth              = DL_DMA_WIDTH_BYTE,
        .srcWidth               = DL_DMA_WIDTH_BYTE,
        .destIncrement          = DL_DMA_ADDR_INCREMENT,
        .dmaChannel             = 0,
        .dmaTransferSource      = NULL,
        .dmaTransferDestination = NULL,
        .enableDMAISR           = false,
    }},
};
```

#### 使用される割り込み
- `DL_UART_INTERRUPT_DMA_DONE_RX` - DMA受信完了割り込み
- `DL_UART_INTERRUPT_DMA_DONE_TX` - DMA送信完了割り込み
- `DL_UART_INTERRUPT_EOT_DONE` - 送信終了割り込み

#### アプリケーションコード (uartcallback.c:58-84)
```c
// 受信コールバック
void callbackFxn(UART_Handle handle, void *buffer, size_t count,
                 void *userArg, int_fast16_t status)
{
    if (status != UART_STATUS_SUCCESS) {
        while (1) {}  // エラー処理
    }
    numBytesRead = count;
    sem_post(&appSema);  // セマフォ解放
}

// 送信コールバック
void callbackFxnTx(UART_Handle handle, void *buffer, size_t count,
                   void *userArg, int_fast16_t status)
{
    if (status != UART_STATUS_SUCCESS) {
        while (1) {}  // エラー処理
    }
    // TX DMA使用時はセマフォ解放が必要な場合あり
    /* sem_post(&appSema); */
}
```

#### メイン処理 (uartcallback.c:103-157)
```c
void mainThread(void *arg0)
{
    // UART初期化
    UART_Params_init(&uartParams);
    uartParams.readMode     = UART_Mode_CALLBACK;  // コールバックモード
    uartParams.baudRate     = CONFIG_UART_BAUD_RATE;
    uartParams.readCallback = callbackFxn;

    uart = UART_open(CONFIG_UART_0, &uartParams);

    // エコーループ
    while (1) {
        numBytesRead = 0;

        // 非ブロッキング読み込み（DMAで実装）
        status = UART_read(uart, &input, 1, NULL);

        // コールバック実行まで待機
        sem_wait(&appSema);

        if (numBytesRead > 0) {
            // 非ブロッキング書き込み
            status = UART_write(uart, &input, 1, NULL);
        }
    }
}
```

---

## 設定比較表

| 項目 | uart_echo (割り込み) | uart_callback (DMA) |
|------|---------------------|---------------------|
| **DMAサポート** | `false` | `true` |
| **コールバック** | `false` | `true` |
| **動作モード** | BLOCKING | CALLBACK |
| **DMA設定** | なし | DMA_UART0_TX/RX_TRIG |
| **割り込みタイプ** | RX/TX割り込み | DMA完了割り込み |
| **ヘッダー** | 標準UART | + DMA (`DMAMSPM0.h`) |
| **同期方法** | ポーリングループ | セマフォ |
| **CPU効率** | 低（ビジーウェイト） | 高（DMA転送） |

---

## ドライバ実装の動作

### 純粋な割り込みモード (UARTMSPM0.c)

**設定条件:**
- `dmaSupported = false`
- `callbacksSupported = false`

**動作:**
```c
// 受信割り込み有効化
DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_RX);

// 送信割り込み有効化
DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
```

**割り込みハンドラ:**
```c
void UART0_IRQHandler(void)
{
    UARTMSP_interruptHandler((UART_Handle) &UART_config[0]);
}

void UARTMSP_interruptHandler(UART_Handle handle)
{
    // 割り込みステータス取得
    uint32_t status = DL_UART_getEnabledInterruptStatus(hwAttrs->regs, 0x1FFFF);

    // RX割り込み処理
    if (status & DL_UART_INTERRUPT_RX) {
        while (!DL_UART_isRXFIFOEmpty(hwAttrs->regs)) {
            uint8_t rxData = DL_UART_receiveData(hwAttrs->regs);
            // リングバッファへ格納
            RingBuf_put(&object->rxBuffer, rxData);
        }
    }

    // TX割り込み処理
    if (status & DL_UART_INTERRUPT_TX) {
        // リングバッファからデータ送信
        while (!DL_UART_isTXFIFOFull(hwAttrs->regs) &&
               RingBuf_getCount(&object->txBuffer) > 0) {
            uint8_t txData = RingBuf_get(&object->txBuffer);
            DL_UART_transmitData(hwAttrs->regs, txData);
        }
    }

    // 割り込みクリア
    DL_UART_clearInterruptStatus(hwAttrs->regs, status);
}
```

### DMAモード (UARTMSPM0.c)

**設定条件:**
- `dmaSupported = true`
- `callbacksSupported = true`
- `readMode = UART_Mode_CALLBACK` または `writeMode = UART_Mode_CALLBACK`

**動作:**
```c
// DMA初期化
uartObject->DMA_Handle = DMA_Init(&DMATransfer, &dlDMACfg,
                                   uartObject->noOfDMAChannels);

// DMA完了割り込み有効化
if (params->readMode == UART_Mode_CALLBACK)
    DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_DMA_DONE_RX);

if (params->writeMode == UART_Mode_CALLBACK)
    DL_UART_enableInterrupt(hwAttrs->regs,
        DL_UART_INTERRUPT_DMA_DONE_TX | DL_UART_INTERRUPT_EOT_DONE);
```

**割り込みハンドラ:**
```c
void UARTMSP_interruptHandler(UART_Handle handle)
{
    uint32_t status = DL_UART_getEnabledInterruptStatus(hwAttrs->regs, 0x1FFFF);

    // DMA受信完了
    if (status & DL_UART_INTERRUPT_DMA_DONE_RX) {
        // コールバック呼び出し
        object->readCallback(handle, object->readBuf, object->readCount,
                            object->userArg, UART_STATUS_SUCCESS);
    }

    // DMA送信完了
    if (status & DL_UART_INTERRUPT_DMA_DONE_TX) {
        // コールバック呼び出し
        object->writeCallback(handle, object->writeBuf, object->writeCount,
                             object->userArg, UART_STATUS_SUCCESS);
    }

    DL_UART_clearInterruptStatus(hwAttrs->regs, status);
}
```

---

## ハードウェア設定

両方の例で共通のハードウェア設定が使用されています：

### ピン配置
- **UART0 RX:** PA11 (J4_33/J26_6) - `IOMUX_PINCM22`
- **UART0 TX:** PA10 (J4_34/J26_5) - `IOMUX_PINCM21`

### UART設定
```c
static const UARTMSP_HWAttrs UARTMSPHWAttrs[CONFIG_UART_COUNT] = {
    {
        .regs          = UART0,
        .irq           = UART0_INT_IRQn,
        .rxPin         = IOMUX_PINCM22,
        .rxPinFunction = IOMUX_PINCM22_PF_UART0_RX,
        .txPin         = IOMUX_PINCM21,
        .txPinFunction = IOMUX_PINCM21_PF_UART0_TX,
        .mode          = DL_UART_MODE_NORMAL,
        .direction     = DL_UART_DIRECTION_TX_RX,
        .flowControl   = DL_UART_FLOW_CONTROL_NONE,
        .clockSource   = DL_UART_CLOCK_BUSCLK,      // 32 MHz
        .clockDivider  = DL_UART_CLOCK_DIVIDE_RATIO_4,  // 8 MHz
        .rxIntFifoThr  = DL_UART_RX_FIFO_LEVEL_ONE_ENTRY,
        .txIntFifoThr  = DL_UART_TX_FIFO_LEVEL_EMPTY,
    },
};
```

### クロック設定
- **クロックソース:** BUSCLK (32 MHz)
- **分周比:** 4 (実効8 MHz)
- **ボーレート:** 115200 bps

### FIFO設定
- **RX FIFO閾値:** 1エントリで割り込み
- **TX FIFO閾値:** 空で割り込み

---

## 推奨事項

### 純粋な割り込みベース実装を使用する場合

**推奨例:** `uart_echo`

**使用ケース:**
- シンプルな通信プロトコル
- 少量のデータ転送
- DMAチャネルを他の目的に使用したい
- デバッグが容易な実装が必要

**設定方法:**
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .supportFxns        = &UARTMSPSupportFxns,
            .buffersSupported   = true,
            .eventsSupported    = false,
            .callbacksSupported = false,  // コールバック無効
            .dmaSupported       = false,  // DMA無効
        },
    },
};
```

### DMAベース実装を使用する場合

**推奨例:** `uart_callback`

**使用ケース:**
- 大量のデータ転送
- 低レイテンシが必要
- CPU使用率を最小化したい
- 非ブロッキング動作が必要

**設定方法:**
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .supportFxns        = &UARTMSPSupportFxns,
            .buffersSupported   = true,
            .eventsSupported    = false,
            .callbacksSupported = true,   // コールバック有効
            .dmaSupported       = true,   // DMA有効
            .noOfDMAChannels    = 1,
        },
    },
};
```

---

## 結論

### uart_echoは純粋な割り込みベース実装

**確認事項:**
- ✅ `dmaSupported = false` - DMA不使用
- ✅ 割り込みのみで送受信を実装
- ✅ `DL_UART_INTERRUPT_RX/TX`を使用
- ✅ ブロッキングモードで動作

### uart_callbackはDMA使用実装

**確認事項:**
- ✅ `dmaSupported = true` - DMA使用
- ✅ `DL_UART_INTERRUPT_DMA_DONE_RX/TX`を使用
- ✅ DMA設定構造体を含む
- ✅ コールバックモードで動作

**純粋な割り込みベースのUART実装が必要な場合は、`uart_echo`の実装を参考にしてください。**

---

## 参考ファイル

### uart_echo (純粋な割り込み)
- [uartecho.c](../examples/rtos/LP_MSPM0G3507/drivers/uart_echo/uartecho.c)
- [ti_drivers_config.c](../examples/rtos/LP_MSPM0G3507/drivers/uart_echo/ti_drivers_config.c)
- [ti_drivers_config.h](../examples/rtos/LP_MSPM0G3507/drivers/uart_echo/ti_drivers_config.h)

### uart_callback (DMA)
- [uartcallback.c](../examples/rtos/LP_MSPM0G3507/drivers/uart_callback/uartcallback.c)
- [ti_drivers_config.c](../examples/rtos/LP_MSPM0G3507/drivers/uart_callback/ti_drivers_config.c)
- [ti_drivers_config.h](../examples/rtos/LP_MSPM0G3507/drivers/uart_callback/ti_drivers_config.h)

### ドライバ実装
- [UARTMSPM0.c](../source/ti/drivers/uart/UARTMSPM0.c)
- [UARTMSPM0.h](../source/ti/drivers/uart/UARTMSPM0.h)
- [dl_uart.h](../source/ti/driverlib/dl_uart.h)
- [dl_uart.c](../source/ti/driverlib/dl_uart.c)

---

**調査日:** 2025-11-06
**SDK:** MSPM0 SDK
**ターゲット:** LP_MSPM0G3507 LaunchPad
