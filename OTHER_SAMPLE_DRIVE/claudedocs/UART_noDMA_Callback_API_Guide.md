# UART noDMA × Callback モードAPI使用ガイド

## 概要

このドキュメントは、LP_MSPM0G3507で**UART_NO_DMA定義 + コールバックモード**を使用する場合のAPI使用方法を解説します。

---

## 前提条件

### ビルド設定

プロジェクトのコンパイルオプションに`-DUART_NO_DMA`を追加：

```makefile
CFLAGS += -DUART_NO_DMA
```

### ドライバ設定

```c
// ti_drivers_config.c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .supportFxns        = &UARTMSPSupportFxns,
            .buffersSupported   = true,
            .eventsSupported    = false,
            .callbacksSupported = true,   // ★コールバック有効★
            .dmaSupported       = false,  // ★DMA無効★
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

---

## API関数一覧

### 初期化・制御API

| 関数 | 説明 | 定義場所 |
|------|------|---------|
| `UART_Params_init()` | パラメータ構造体を初期化 | UART.c:301 |
| `UART_open()` | UARTドライバを開く | UART.c:309 |
| `UART_close()` | UARTドライバを閉じる | UART.c:367 |

### データ送受信API

| 関数 | 説明 | 定義場所 |
|------|------|---------|
| `UART_read()` | データ受信（推奨） | UART.c:385 |
| `UART_write()` | データ送信（推奨） | UART.c:402 |
| `UART_readTimeout()` | タイムアウト付き受信 | UART.c:420 |
| `UART_writeTimeout()` | タイムアウト付き送信 | UART.c:469 |
| `UART_readCancel()` | 受信キャンセル | UART.c:517 |
| `UART_writeCancel()` | 送信キャンセル | UART.c:557 |

### 内部API（直接使用しない）

| 関数 | 説明 | 定義場所 |
|------|------|---------|
| `UART_readCallback()` | コールバックモード受信内部実装 | UART.c:167 |
| `UART_writeCallback()` | コールバックモード送信内部実装 | UART.c:205 |
| `UART_readFullFeatured()` | 受信ディスパッチャ | UART.c:265 |
| `UART_writeFullFeatured()` | 送信ディスパッチャ | UART.c:283 |

---

## データ構造

### UART_Params 構造体

```c
typedef struct {
    UART_Mode readMode;              // 受信モード
    UART_Mode writeMode;             // 送信モード
    UART_Callback readCallback;      // 受信コールバック関数
    UART_Callback writeCallback;     // 送信コールバック関数
    UART_EventCallback eventCallback; // イベントコールバック関数
    uint32_t eventMask;              // イベントマスク
    UART_ReadReturnMode readReturnMode; // 受信戻りモード
    uint32_t baudRate;               // ボーレート
    UART_DataLen dataLength;         // データ長
    UART_StopBits stopBits;          // ストップビット
    UART_Parity parityType;          // パリティ
    void *userArg;                   // ユーザー引数
} UART_Params;
```

**デフォルト値:**
- readMode = `UART_Mode_BLOCKING`
- writeMode = `UART_Mode_BLOCKING`
- readCallback = `NULL`
- writeCallback = `NULL`
- eventCallback = `NULL`
- eventMask = `0`
- readReturnMode = `UART_ReadReturnMode_PARTIAL`
- baudRate = `9600`
- dataLength = `UART_DataLen_8`
- stopBits = `UART_StopBits_1`
- parityType = `UART_Parity_NONE`
- userArg = `NULL`

### UART_Callback 型

```c
typedef void (*UART_Callback)(
    UART_Handle handle,      // UARTハンドル
    void *buf,              // データバッファ
    size_t count,           // 転送バイト数
    void *userArg,          // ユーザー引数
    int_fast16_t status     // ステータス
);
```

### UART_Mode 列挙型

```c
typedef enum {
    UART_Mode_BLOCKING,     // ブロッキングモード
    UART_Mode_CALLBACK,     // コールバックモード（★使用★）
    UART_Mode_NONBLOCKING   // 非ブロッキングモード
} UART_Mode;
```

### UART_STATUS コード

```c
#define UART_STATUS_SUCCESS      0   // 成功
#define UART_STATUS_ERROR       -1   // 一般エラー
#define UART_STATUS_EINUSE      -2   // 使用中
#define UART_STATUS_ETIMEOUT    -3   // タイムアウト
#define UART_STATUS_ECANCELLED  -4   // キャンセル
#define UART_STATUS_EINVALID    -5   // 無効なパラメータ
#define UART_STATUS_EAGAIN      -6   // リトライ必要
```

---

## API使用方法

### 1. 初期化手順

#### 1.1 パラメータ初期化

```c
UART_Params uartParams;

// デフォルト値で初期化
UART_Params_init(&uartParams);
```

**実装 (UART.c:301-304):**
```c
void UART_Params_init(UART_Params *params)
{
    *params = UART_defaultParams;  // デフォルト構造体をコピー
}
```

#### 1.2 コールバックモード設定

```c
// コールバックモードを設定
uartParams.readMode  = UART_Mode_CALLBACK;
uartParams.writeMode = UART_Mode_CALLBACK;

// コールバック関数を登録
uartParams.readCallback  = myReadCallback;
uartParams.writeCallback = myWriteCallback;

// ボーレート設定
uartParams.baudRate = 115200;

// ユーザー引数（オプション）
uartParams.userArg = &myContext;
```

#### 1.3 UARTを開く

```c
UART_Handle uart;

uart = UART_open(CONFIG_UART_0, &uartParams);
if (uart == NULL) {
    // エラー処理
    __BKPT();
}
```

**実装 (UART.c:309-362):**
```c
UART_Handle UART_open(uint_least8_t index, UART_Params *params)
{
    // コールバックモードでコールバックNULLチェック
    if (((params->readMode == UART_Mode_CALLBACK) && (params->readCallback == NULL)) ||
        ((params->writeMode == UART_Mode_CALLBACK) && (params->writeCallback == NULL)))
    {
        return (NULL);  // ★コールバックNULLはエラー★
    }

    if (index < UART_count)
    {
        UART_Handle handle = (UART_Handle)&UART_config[index];
        UART_Object *object = handle->object;
        uintptr_t key = HwiP_disable();

        if (!object->inUse)
        {
            object->inUse = true;
            HwiP_restore(key);

            // ハードウェア初期化
            if (object->supportFxns->enable(handle, params))
            {
                object->returnMode = params->readReturnMode;
                object->readMode   = params->readMode;
                object->writeMode  = params->writeMode;

                // コールバック関数を保存
                if (object->callbacksSupported)
                {
                    UART_Callback_Object *callbackObject = UART_callbackObject(object);
                    if (params->readCallback != NULL)
                    {
                        callbackObject->readCallback = params->readCallback;
                    }
                    if (params->writeCallback != NULL)
                    {
                        callbackObject->writeCallback = params->writeCallback;
                    }
                }
                return handle;
            }
            // 初期化失敗
            key = HwiP_disable();
            object->inUse = false;
            HwiP_restore(key);
        }
        // already open
    }
    return NULL;
}
```

---

### 2. データ受信 (UART_read)

#### 2.1 基本的な使い方

```c
char rxBuffer[100];

// 非ブロッキング受信開始
int_fast16_t status = UART_read(uart, rxBuffer, sizeof(rxBuffer), NULL);

if (status == UART_STATUS_SUCCESS) {
    // 受信リクエスト成功（コールバックで完了通知）
} else if (status == UART_STATUS_EINUSE) {
    // 前の受信がまだ完了していない
}
```

#### 2.2 UART_read() の動作

**実装 (UART.c:385-397):**
```c
int_fast16_t UART_read(UART_Handle handle, void *buf, size_t size, size_t *bytesRead)
{
    UART_Object *object = UART_Obj_Ptr(handle);
    int_fast16_t result;

    // 内部実装関数を呼び出し（ディスパッチ）
    result = object->supportFxns->read(handle, buf, size, bytesRead);

    if (bytesRead)
    {
        *bytesRead = result < 0 ? 0 : *bytesRead;
    }

    return result < 0 ? result : UART_STATUS_SUCCESS;
}
```

`object->supportFxns->read`は`UART_readFullFeatured`を指します。

**実装 (UART.c:265-278):**
```c
int_fast16_t UART_readFullFeatured(UART_Handle handle, void *buf, size_t size, size_t *bytesRead)
{
    UART_Object *object = UART_Obj_Ptr(handle);

    // ブロッキング/非ブロッキングモード
    if (object->buffersSupported && object->readMode != UART_Mode_CALLBACK)
    {
        return UART_readBuffered(handle, buf, size, bytesRead);
    }

    // ★コールバックモード★
    if (object->callbacksSupported && object->readMode == UART_Mode_CALLBACK)
    {
        return UART_readCallback(handle, buf, size, bytesRead);
    }

    return UART_STATUS_EINVALID;
}
```

#### 2.3 UART_readCallback() の内部動作

**実装 (UART.c:167-200):**
```c
int_fast16_t UART_readCallback(UART_Handle handle, void *buf, size_t size, size_t *bytesRead)
{
    UART_Object *object = UART_Obj_Ptr(handle);
    UART_Buffers_Object *buffersObject = UART_buffersObject(object);
    UART_Callback_Object *callbackObj = UART_callbackObject(object);
    uintptr_t key;

    // セマフォ取得（前回の受信完了を待機）
    SemaphoreP_pend(buffersObject->rxSem, SemaphoreP_WAIT_FOREVER);

    key = HwiP_disable();

    // 使用中チェック
    if (object->readInUse)
    {
        HwiP_restore(key);
        return UART_STATUS_EINUSE;  // ★既に受信中★
    }

    // 受信パラメータ設定
    object->readInUse         = true;
    callbackObj->bytesRead = 0;
    callbackObj->readBuf   = buf;        // ★バッファ保存★
    callbackObj->readSize  = size;       // ★サイズ保存★
    callbackObj->readCount = size;       // 残りバイト数
    callbackObj->bytesRead = 0;          // 受信済みバイト数
    callbackObj->rxStatus  = 0;          // エラーステータスクリア

    // エラー割り込み有効化
    UARTMSP_enableInts(handle);

    // DMA受信開始（UART_NO_DMA定義時は空関数）
#if ((DeviceFamily_PARENT == DeviceFamily_PARENT_MSPM0G1X0X_G3X0X) || ...)
    UARTMSP_dmaRx(handle, false);  // ★UART_NO_DMA定義時は何もしない★
#endif

    HwiP_restore(key);

    return UART_STATUS_SUCCESS;
}
```

**重要ポイント:**
1. `UART_read()`はすぐに戻る（非ブロッキング）
2. バッファとサイズを内部に保存
3. 割り込みハンドラがデータを受信
4. 指定バイト数受信完了でコールバック呼び出し

---

### 3. 受信コールバック

#### 3.1 コールバック関数の定義

```c
void myReadCallback(UART_Handle handle, void *buf, size_t count,
                    void *userArg, int_fast16_t status)
{
    if (status == UART_STATUS_SUCCESS)
    {
        // 受信成功
        // buf: 受信バッファ（UART_read()で渡したもの）
        // count: 実際に受信したバイト数

        // 受信データ処理
        processReceivedData((char*)buf, count);

    }
    else if (status == UART_STATUS_ECANCELLED)
    {
        // 受信がキャンセルされた
    }
}
```

#### 3.2 割り込みハンドラからの呼び出し

**実装 (UARTMSPM0.c:574-586) - UART_NO_DMA定義時:**
```c
if (status & DL_UART_INTERRUPT_RX)
{
    while (DL_UART_isRXFIFOEmpty(hwAttrs->regs) == false)
    {
        volatile uint8_t rxData = DL_UART_receiveData(hwAttrs->regs);

        // リングバッファに格納
        if (RingBuf_put(&buffersObject->rxBuf, rxData) == -1)
        {
            /* バッファ満杯 */
        }

#ifdef UART_NO_DMA  // ★純粋割り込みコールバック★
        if (callbackObj->readCallback != NULL)
        {
            // リングバッファからユーザーバッファへコピー
            count += RingBuf_getn(&buffersObject->rxBuf,
                                  callbackObj->readBuf + count,
                                  callbackObj->readSize - count);

            // 必要なバイト数を受信完了
            if (callbackObj->readSize == count)
            {
                count = 0;  // カウンタリセット
                uartObject->readInUse = false;

                // ★コールバック呼び出し★
                callbackObj->readCallback(handle,
                    (void*)callbackObj->readBuf,
                    callbackObj->readSize,
                    callbackObj->userArg,
                    UART_STATUS_SUCCESS);
            }
        }
#endif
        rxPost = true;
    }
}
```

**動作フロー:**
```
UART_read() 呼び出し
    ↓
バッファ・サイズを内部に保存
    ↓
すぐにリターン（非ブロッキング）
    ↓
━━━━━━━━━━━━━━━━━━
[ハードウェア]
UARTがデータ受信（1バイト）
    ↓
RX割り込み発生 ★1回目
━━━━━━━━━━━━━━━━━━
    ↓
割り込みハンドラ
    ↓
DL_UART_receiveData() → リングバッファ
    ↓
RingBuf_getn() → ユーザーバッファへコピー
    ↓
まだ全バイト未達 → リターン
━━━━━━━━━━━━━━━━━━
UARTがデータ受信（2バイト目）
    ↓
RX割り込み発生 ★2回目
    ↓
... （繰り返し） ...
━━━━━━━━━━━━━━━━━━
UARTがデータ受信（Nバイト目）
    ↓
RX割り込み発生 ★N回目
━━━━━━━━━━━━━━━━━━
    ↓
割り込みハンドラ
    ↓
指定バイト数達成
    ↓
readCallback() 呼び出し ★
    ↓
アプリケーションのコールバック関数実行
```

---

### 4. データ送信 (UART_write)

#### 4.1 基本的な使い方

```c
const char txBuffer[] = "Hello, World!\r\n";

// 非ブロッキング送信開始
int_fast16_t status = UART_write(uart, txBuffer, sizeof(txBuffer) - 1, NULL);

if (status == UART_STATUS_SUCCESS) {
    // 送信リクエスト成功（コールバックで完了通知）
} else if (status == UART_STATUS_EINUSE) {
    // 前の送信がまだ完了していない
}
```

#### 4.2 UART_write() の動作

**実装 (UART.c:402-415):**
```c
int_fast16_t UART_write(UART_Handle handle, const void *buf, size_t size, size_t *bytesWritten)
{
    UART_Object *object = UART_Obj_Ptr(handle);
    int_fast16_t result;

    // 内部実装関数を呼び出し（ディスパッチ）
    result = object->supportFxns->write(handle, buf, size, bytesWritten);

    if (bytesWritten)
    {
        *bytesWritten = result < 0 ? 0 : *bytesWritten;
    }

    return result < 0 ? result : UART_STATUS_SUCCESS;
}
```

`object->supportFxns->write`は`UART_writeFullFeatured`を指します。

**実装 (UART.c:283-296):**
```c
int_fast16_t UART_writeFullFeatured(UART_Handle handle, const void *buf, size_t size, size_t *bytesWritten)
{
    UART_Object *object = UART_Obj_Ptr(handle);

    // ブロッキング/非ブロッキングモード
    if (object->buffersSupported && object->writeMode != UART_Mode_CALLBACK)
    {
        return UART_writeBuffered(handle, buf, size, bytesWritten);
    }

    // ★コールバックモード★
    if (object->callbacksSupported && object->writeMode == UART_Mode_CALLBACK)
    {
        return UART_writeCallback(handle, buf, size, bytesWritten);
    }

    return UART_STATUS_EINVALID;
}
```

#### 4.3 UART_writeCallback() の内部動作

**実装 (UART.c:205-260):**
```c
int_fast16_t UART_writeCallback(UART_Handle handle, const void *buf, size_t size, size_t *bytesWritten)
{
    UART_Object *object = UART_Obj_Ptr(handle);
    UART_Buffers_Object *buffersObject = UART_buffersObject(object);
    UART_Callback_Object *callbackObj = UART_callbackObject(object);
    uintptr_t key;
    int space;
    static unsigned char *dstAddr;

    // TXセマフォ取得（前回の送信完了を待機）
    SemaphoreP_pend(buffersObject->txSem, SemaphoreP_WAIT_FOREVER);

    key = HwiP_disable();

    // 使用中チェック
    if (object->writeInUse)
    {
        HwiP_restore(key);
        return UART_STATUS_EINUSE;  // ★既に送信中★
    }

    // 送信パラメータ設定
    object->writeInUse           = true;
    callbackObj->bytesWritten = 0;
    callbackObj->writeBuf     = buf;      // ★バッファ保存★
    callbackObj->writeSize    = size;     // ★サイズ保存★
    callbackObj->writeCount   = size;     // 残りバイト数
    callbackObj->txStatus     = 0;        // エラーステータスクリア

    // できるだけリングバッファにコピー
    do {
        // リングバッファの空きスペース取得
        space = RingBuf_putPointer(&buffersObject->txBuf, &dstAddr);

        key = HwiP_disable();
        if (space > callbackObj->writeCount)
        {
            space = callbackObj->writeCount;
        }

        // データコピー
        memcpy(dstAddr, (unsigned char *) callbackObj->writeBuf + callbackObj->bytesWritten, space);

        // リングバッファ更新
        RingBuf_putAdvance(&buffersObject->txBuf, space);

        callbackObj->writeCount -= space;
        callbackObj->bytesWritten += space;

        HwiP_restore(key);
    } while ((space > 0) && (callbackObj->writeCount > 0));

    // TX割り込み開始
    object->supportFxns->txChar(handle);
    HwiP_restore(key);

    return UART_STATUS_SUCCESS;
}
```

**UARTMSP_txChar() の実装 (UARTMSPM0.c:236-257):**
```c
static void UARTMSP_txChar(UART_Handle handle)
{
    UARTMSP_HWAttrs *hwAttrs = UART_HWAttrs_Ptr(handle);
#ifndef UART_NO_DMA
    UART_Object *uartObject = UART_Obj_Ptr(handle);
    UART_Callback_Object *callbackObject = UART_callbackObject(uartObject);

    if (callbackObject->writeCallback != NULL)
    {
        // DMA送信
        UARTMSP_dmaTx(handle);
    }
    else
    {
#endif
        // ★純粋割り込み送信★
        // TX割り込み有効化
        DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
#ifndef UART_NO_DMA
    }
#endif
}
```

**重要ポイント:**
1. `UART_write()`はすぐに戻る（非ブロッキング）
2. データをリングバッファにコピー
3. TX割り込みを有効化
4. 割り込みハンドラがデータを送信
5. 全データ送信完了でコールバック呼び出し

---

### 5. 送信コールバック

#### 5.1 コールバック関数の定義

```c
void myWriteCallback(UART_Handle handle, void *buf, size_t count,
                     void *userArg, int_fast16_t status)
{
    if (status == UART_STATUS_SUCCESS)
    {
        // 送信成功
        // buf: 送信バッファ（UART_write()で渡したもの）
        // count: 実際に送信したバイト数

        // 次の送信準備など
        prepareNextTransmission();

    }
    else if (status == UART_STATUS_ECANCELLED)
    {
        // 送信がキャンセルされた
    }
}
```

#### 5.2 割り込みハンドラからの呼び出し

**実装 (UARTMSPM0.c:593-622) - UART_NO_DMA定義時:**
```c
if (status & DL_UART_INTERRUPT_TX)
{
#ifdef UART_NO_DMA  // ★純粋割り込みコールバック★
    if (callbackObj->writeCallback != NULL)
    {
        // 専用のコールバック送信処理
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
                // 送信データなし
                DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
                DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_EOT_DONE);
                break;
            }
            DL_UART_transmitData(hwAttrs->regs, txData);
        } while (DL_UART_isTXFIFOFull(hwAttrs->regs) == false);
#ifdef UART_NO_DMA
    }
#endif
}
```

**UARTMSP_TxCallback() の実装 (UARTMSPM0.c:375-402):**
```c
void UARTMSP_TxCallback(UART_Handle handle)
{
    UARTMSP_HWAttrs *hwAttrs           = UART_HWAttrs_Ptr(handle);
    UART_Object *uartObject            = UART_Obj_Ptr(handle);
    UART_Buffers_Object *buffersObject = UART_buffersObject(uartObject);
    UART_Callback_Object *callbackObj  = UART_callbackObject(uartObject);

    do
    {
        uint8_t txData;
        // ユーザーバッファから直接取得
        txData = *(callbackObj->writeBuf + callbackObj->bytesWritten);

        // 全データ送信完了チェック
        if (callbackObj->writeSize == callbackObj->bytesWritten)
        {
            // TX割り込み無効化、EOT割り込み有効化
            DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
            DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_EOT_DONE);
            break;
        }

        // 1バイト送信
        DL_UART_transmitData(hwAttrs->regs, txData);
        callbackObj->bytesWritten++;

    } while (DL_UART_isTXFIFOFull(hwAttrs->regs) == false);
}
```

**EOT（End of Transmission）割り込み処理:**

送信データがすべてシフトレジスタから送出されたら、EOT割り込みが発生してコールバックが呼ばれます。

**実装 (UARTMSPM0.c:625-643):**
```c
if (status & DL_UART_INTERRUPT_EOT_DONE)
{
    // 送信完了、割り込み無効化
    DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_EOT_DONE);
    txPost = true;

#ifndef UART_NO_DMA
    if (callbackObj->writeCallback != NULL)
    {
        if (uartObject->writeInUse != false)
        {
            uartObject->writeInUse = false;

            // ★コールバック呼び出し★
            callbackObj->writeCallback(handle,
                (uint8_t *) callbackObj->writeBuf,
                callbackObj->writeSize,
                callbackObj->userArg,
                0);
        }
    }
#endif
}
```

**動作フロー:**
```
UART_write() 呼び出し
    ↓
リングバッファにコピー
    ↓
TX割り込み有効化
    ↓
すぐにリターン（非ブロッキング）
    ↓
━━━━━━━━━━━━━━━━━━
[ハードウェア]
TX FIFOに空きあり
    ↓
TX割り込み発生 ★1回目
━━━━━━━━━━━━━━━━━━
    ↓
割り込みハンドラ
    ↓
UARTMSP_TxCallback()
    ↓
ユーザーバッファから1バイト取得
    ↓
DL_UART_transmitData() → UART TXレジスタ
    ↓
まだデータあり → リターン
━━━━━━━━━━━━━━━━━━
TX FIFOに空きあり
    ↓
TX割り込み発生 ★2回目
    ↓
... （繰り返し） ...
━━━━━━━━━━━━━━━━━━
最後のバイト送信
    ↓
TX割り込み無効化
    ↓
EOT割り込み有効化
━━━━━━━━━━━━━━━━━━
シフトレジスタから送出完了
    ↓
EOT割り込み発生 ★
━━━━━━━━━━━━━━━━━━
    ↓
割り込みハンドラ
    ↓
writeCallback() 呼び出し ★
    ↓
アプリケーションのコールバック関数実行
```

---

### 6. キャンセル操作

#### 6.1 受信キャンセル

```c
// 受信をキャンセル
UART_readCancel(uart);
```

**実装 (UART.c:517-552):**
```c
void UART_readCancel(UART_Handle handle)
{
    UART_Object *object = UART_Obj_Ptr(handle);
    uintptr_t key;

    key = HwiP_disable();

    if (object->readMode == UART_Mode_CALLBACK)
    {
        UART_Callback_Object *callbackObject = UART_callbackObject(object);

        if (object->readInUse)
        {
            uint8_t *readBuf   = callbackObject->readBuf;
            uint16_t readSize  = callbackObject->readSize;
            uint16_t bytesRead = callbackObject->bytesRead;

            // 使用中フラグをクリア
            object->readInUse = false;

            // ★コールバック呼び出し（キャンセルステータス）★
            callbackObject->readCallback(handle, readBuf, bytesRead,
                object->userArg,
                readSize != bytesRead ? UART_STATUS_ECANCELLED : UART_STATUS_SUCCESS);
        }
        HwiP_restore(key);
    }
    else
    {
        // ブロッキングモード用
        UART_Buffers_Object *buffersObject = UART_buffersObject(object);
        object->readCancel = true;
        HwiP_restore(key);
        SemaphoreP_post(buffersObject->rxSem);
    }
}
```

#### 6.2 送信キャンセル

```c
// 送信をキャンセル
UART_writeCancel(uart);
```

**実装 (UART.c:557-599):**
```c
void UART_writeCancel(UART_Handle handle)
{
    UARTMSP_HWAttrs *hwAttrs = UART_HWAttrs_Ptr(handle);
    UART_Object *object = UART_Obj_Ptr(handle);
    UART_Buffers_Object *buffersObject = UART_buffersObject(object);
    UART_Callback_Object *callbackObject = UART_callbackObject(object);
    uintptr_t key;

    key = HwiP_disable();

    if (!object->writeCancel)
    {
        object->writeCancel = true;

        // DMA送信停止（UART_NO_DMA定義時は空）
#if ((DeviceFamily_PARENT == DeviceFamily_PARENT_MSPM0G1X0X_G3X0X) || ...)
        UARTMSP_dmaStopTx(handle);
#endif

        if (object->writeInUse)
        {
            // TX FIFOが空になるまで待機
            while (!DL_UART_isTXFIFOEmpty(hwAttrs->regs)) {}

            // ブロッキングモード
            if (object->writeMode == UART_Mode_BLOCKING)
            {
                SemaphoreP_post(&buffersObject->txSem);
            }

            // コールバックモード
            if (object->writeMode == UART_Mode_CALLBACK)
            {
                object->writeInUse = false;
                callbackObject->writeCount = 0;

                // ★コールバック呼び出し（キャンセルステータス）★
                callbackObject->writeCallback(handle,
                    (void *)callbackObject->writeBuf,
                    callbackObject->bytesWritten,
                    callbackObject->userArg,
                    UART_STATUS_ECANCELLED);
            }
        }
    }
    HwiP_restore(key);
}
```

---

### 7. クローズ

```c
// UARTを閉じる
UART_close(uart);
```

**実装 (UART.c:367-380):**
```c
void UART_close(UART_Handle handle)
{
    UART_Object *object = UART_Obj_Ptr(handle);
    uintptr_t key;

    // 進行中の送受信をキャンセル
    UART_readCancel(handle);

    key = HwiP_disable();
    object->inUse = false;
    HwiP_restore(key);

    // ハードウェア無効化
    object->supportFxns->disable(handle);
    return;
}
```

---

## 完全な実装例

### サンプルアプリケーション

```c
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "ti_drivers_config.h"

// グローバル変数
static UART_Handle uart;
static volatile bool rxComplete = false;
static volatile bool txComplete = false;

// 受信バッファ
static char rxBuffer[128];
static size_t rxCount = 0;

// 送信バッファ
static const char welcomeMsg[] = "UART Callback Example\r\n";
static const char prompt[] = "> ";

// 受信コールバック
void rxCallback(UART_Handle handle, void *buf, size_t count,
                void *userArg, int_fast16_t status)
{
    if (status == UART_STATUS_SUCCESS)
    {
        rxCount = count;
        rxComplete = true;

        // データ受信完了
        // 次の受信を開始する場合はここで UART_read() を呼ぶ
    }
    else if (status == UART_STATUS_ECANCELLED)
    {
        // キャンセルされた
    }
}

// 送信コールバック
void txCallback(UART_Handle handle, void *buf, size_t count,
                void *userArg, int_fast16_t status)
{
    if (status == UART_STATUS_SUCCESS)
    {
        txComplete = true;

        // 送信完了
        // 次の送信を開始する場合はここで UART_write() を呼ぶ
    }
    else if (status == UART_STATUS_ECANCELLED)
    {
        // キャンセルされた
    }
}

// UART初期化
bool uartInit(void)
{
    UART_Params uartParams;

    // パラメータ初期化
    UART_Params_init(&uartParams);

    // コールバックモード設定
    uartParams.readMode  = UART_Mode_CALLBACK;
    uartParams.writeMode = UART_Mode_CALLBACK;
    uartParams.readCallback  = rxCallback;
    uartParams.writeCallback = txCallback;

    // ボーレート設定
    uartParams.baudRate = 115200;

    // UARTオープン
    uart = UART_open(CONFIG_UART_0, &uartParams);
    if (uart == NULL)
    {
        // エラー
        return false;
    }

    return true;
}

// メイン処理
void mainThread(void *arg0)
{
    int_fast16_t status;

    // UART初期化
    if (!uartInit())
    {
        __BKPT();  // エラー
    }

    // Welcomeメッセージ送信
    txComplete = false;
    status = UART_write(uart, welcomeMsg, sizeof(welcomeMsg) - 1, NULL);
    if (status != UART_STATUS_SUCCESS)
    {
        __BKPT();  // エラー
    }

    // 送信完了待機
    while (!txComplete)
    {
        // 待機（実際にはRTOSタスクなので他のタスクが実行される）
    }

    // プロンプト表示
    txComplete = false;
    UART_write(uart, prompt, sizeof(prompt) - 1, NULL);
    while (!txComplete) {}

    // エコーループ
    while (1)
    {
        // 1文字受信開始
        rxComplete = false;
        status = UART_read(uart, rxBuffer, 1, NULL);
        if (status != UART_STATUS_SUCCESS)
        {
            __BKPT();  // エラー
        }

        // 受信完了待機
        while (!rxComplete)
        {
            // 待機（実際にはRTOSタスクなので他のタスクが実行される）
        }

        // エコーバック
        txComplete = false;
        UART_write(uart, rxBuffer, rxCount, NULL);
        while (!txComplete) {}

        // Enterキーチェック
        if (rxBuffer[0] == '\r')
        {
            // 改行送信
            txComplete = false;
            UART_write(uart, "\n", 1, NULL);
            while (!txComplete) {}

            // プロンプト再表示
            txComplete = false;
            UART_write(uart, prompt, sizeof(prompt) - 1, NULL);
            while (!txComplete) {}
        }
    }
}
```

### より高度な例（複数バイト受信）

```c
// 複数バイト受信の例
void advancedRxExample(void)
{
    char buffer[128];
    int_fast16_t status;

    // 128バイト受信開始
    rxComplete = false;
    status = UART_read(uart, buffer, sizeof(buffer), NULL);

    if (status == UART_STATUS_SUCCESS)
    {
        // 受信リクエスト成功
        // rxCallback() で完了通知される
    }
    else if (status == UART_STATUS_EINUSE)
    {
        // 前の受信がまだ完了していない
    }
}

// 複数バイト送信の例
void advancedTxExample(void)
{
    const char message[] = "This is a long message to test UART transmission.\r\n";
    int_fast16_t status;

    // メッセージ送信開始
    txComplete = false;
    status = UART_write(uart, message, sizeof(message) - 1, NULL);

    if (status == UART_STATUS_SUCCESS)
    {
        // 送信リクエスト成功
        // txCallback() で完了通知される
    }
    else if (status == UART_STATUS_EINUSE)
    {
        // 前の送信がまだ完了していない
    }
}
```

---

## API呼び出しフロー図

### 受信フロー

```
アプリケーション
    │
    ├─ UART_read(uart, buf, size, NULL)
    │      ↓
    │   [UART.c:385] UART_read()
    │      ↓
    │   [UART.c:265] UART_readFullFeatured()
    │      ↓ (readMode == UART_Mode_CALLBACK)
    │   [UART.c:167] UART_readCallback()
    │      ├─ バッファ・サイズを保存
    │      ├─ readInUse = true
    │      └─ RETURN (すぐに戻る)
    │
    │   ... アプリは他の処理を実行 ...
    │
    ├─ [ハードウェア] UARTがデータ受信
    │      ↓
    │   RX割り込み発生
    │      ↓
    │   [ti_drivers_config.c] UART0_IRQHandler()
    │      ↓
    │   [UARTMSPM0.c:430] UARTMSP_interruptHandler()
    │      ↓
    │   [UARTMSPM0.c:561-589] DL_UART_INTERRUPT_RX処理
    │      ├─ DL_UART_receiveData() → リングバッファ
    │      ├─ #ifdef UART_NO_DMA ブロック
    │      ├─ RingBuf_getn() → ユーザーバッファ
    │      └─ 指定バイト数達成？
    │           ↓ YES
    │        readCallback() 呼び出し ★
    │           ↓
    ├─ rxCallback() 実行 ★
    │      └─ アプリケーション処理
```

### 送信フロー

```
アプリケーション
    │
    ├─ UART_write(uart, buf, size, NULL)
    │      ↓
    │   [UART.c:402] UART_write()
    │      ↓
    │   [UART.c:283] UART_writeFullFeatured()
    │      ↓ (writeMode == UART_Mode_CALLBACK)
    │   [UART.c:205] UART_writeCallback()
    │      ├─ バッファ・サイズを保存
    │      ├─ writeInUse = true
    │      ├─ データをリングバッファにコピー
    │      ├─ UARTMSP_txChar() → TX割り込み有効化
    │      └─ RETURN (すぐに戻る)
    │
    │   ... アプリは他の処理を実行 ...
    │
    ├─ [ハードウェア] TX FIFOに空きあり
    │      ↓
    │   TX割り込み発生
    │      ↓
    │   [ti_drivers_config.c] UART0_IRQHandler()
    │      ↓
    │   [UARTMSPM0.c:430] UARTMSP_interruptHandler()
    │      ↓
    │   [UARTMSPM0.c:591-623] DL_UART_INTERRUPT_TX処理
    │      ├─ #ifdef UART_NO_DMA ブロック
    │      ├─ UARTMSP_TxCallback()
    │      │    ├─ ユーザーバッファから1バイト取得
    │      │    ├─ DL_UART_transmitData()
    │      │    └─ 全データ送信完了？
    │      │         ↓ YES
    │      │      TX割り込み無効化
    │      │      EOT割り込み有効化
    │      │
    │      └─ ... 割り込み繰り返し ...
    │
    ├─ [ハードウェア] シフトレジスタ送出完了
    │      ↓
    │   EOT割り込み発生
    │      ↓
    │   [UARTMSPM0.c:625-643] DL_UART_INTERRUPT_EOT_DONE処理
    │      ├─ writeCallback() 呼び出し ★
    │      ↓
    ├─ txCallback() 実行 ★
    │      └─ アプリケーション処理
```

---

## 重要な注意事項

### 1. コールバック実行コンテキスト

**コールバックは割り込みコンテキストで実行されます。**

```c
void rxCallback(UART_Handle handle, void *buf, size_t count,
                void *userArg, int_fast16_t status)
{
    // ★割り込みコンテキスト★
    // - 長時間処理は禁止
    // - ブロッキング関数呼び出し禁止
    // - できるだけ早くリターンすること

    // OK: フラグ設定
    rxComplete = true;

    // OK: セマフォポスト（非ブロッキング）
    SemaphoreP_post(&rxSemaphore);

    // NG: 長時間ループ
    // for (int i = 0; i < 1000000; i++) { ... }

    // NG: ブロッキング関数
    // SemaphoreP_pend(&someSemaphore, SemaphoreP_WAIT_FOREVER);
}
```

### 2. バッファの有効性

**コールバックが呼ばれるまでバッファを保持すること。**

```c
// NG: ローカルバッファ（スコープ外になる）
void badExample(void)
{
    char buffer[100];
    UART_read(uart, buffer, sizeof(buffer), NULL);
    // 関数を抜けるとbufferが無効になる
}

// OK: グローバルバッファ
static char globalBuffer[100];
void goodExample(void)
{
    UART_read(uart, globalBuffer, sizeof(globalBuffer), NULL);
    // コールバックが呼ばれるまでglobalBufferは有効
}

// OK: 動的メモリ（コールバックで解放）
void dynamicExample(void)
{
    char *buffer = malloc(100);
    UART_read(uart, buffer, 100, NULL);
    // rxCallback()でfree(buffer)する
}
```

### 3. 再入防止

**UART_read/write は再入不可（EINUSE エラー）**

```c
// NG: 前の受信が完了していない
UART_read(uart, buf1, 10, NULL);
UART_read(uart, buf2, 10, NULL);  // ← UART_STATUS_EINUSE

// OK: コールバックで次の受信を開始
void rxCallback(UART_Handle handle, void *buf, size_t count,
                void *userArg, int_fast16_t status)
{
    if (status == UART_STATUS_SUCCESS)
    {
        // 前の受信完了、次を開始
        UART_read(handle, nextBuffer, sizeof(nextBuffer), NULL);
    }
}
```

### 4. UART_NO_DMA定義の確認

**コンパイル時に UART_NO_DMA が定義されていることを確認：**

```c
#ifndef UART_NO_DMA
#error "This application requires UART_NO_DMA to be defined"
#endif
```

---

## トラブルシューティング

### 問題: コールバックが呼ばれない

**原因:**
1. コールバック関数がNULL
2. `callbacksSupported = false`
3. `UART_NO_DMA`が定義されていない

**解決策:**
```c
// 1. コールバック関数を正しく設定
uartParams.readCallback = rxCallback;  // NULLでないこと

// 2. ti_drivers_config.c で確認
.callbacksSupported = true,  // trueにする

// 3. ビルド設定で確認
CFLAGS += -DUART_NO_DMA
```

### 問題: UART_STATUS_EINUSE エラー

**原因:**
前の送受信が完了していない

**解決策:**
```c
// コールバックで完了を待ってから次を開始
static volatile bool transferComplete = false;

void myCallback(...)
{
    transferComplete = true;
}

// 完了待機
while (!transferComplete) {}

// 次の転送
transferComplete = false;
UART_read(uart, buffer, size, NULL);
```

### 問題: データが欠落する

**原因:**
リングバッファオーバーフロー

**解決策:**
```c
// リングバッファサイズを増やす（ti_drivers_config.c）
#define CONFIG_UART_BUFFER_SIZE 1024  // 大きくする

static uint8_t rxBuffer[CONFIG_UART_BUFFER_SIZE];
static uint8_t txBuffer[CONFIG_UART_BUFFER_SIZE];
```

---

## まとめ

### 使用するAPI

| 操作 | API | 備考 |
|------|-----|------|
| 初期化 | `UART_Params_init()` | パラメータ初期化 |
| | `UART_open()` | ドライバオープン |
| 受信 | `UART_read()` | 非ブロッキング受信 |
| 送信 | `UART_write()` | 非ブロッキング送信 |
| キャンセル | `UART_readCancel()` | 受信中止 |
| | `UART_writeCancel()` | 送信中止 |
| 終了 | `UART_close()` | ドライバクローズ |

### コールバック

- `UART_Callback` 型の関数を定義
- **割り込みコンテキスト**で実行される
- できるだけ早くリターンすること

### 設定要件

1. **ビルドオプション:** `-DUART_NO_DMA`
2. **ドライバ設定:** `callbacksSupported = true`, `dmaSupported = false`
3. **パラメータ:** `readMode = UART_Mode_CALLBACK`, `writeMode = UART_Mode_CALLBACK`

---

## 参考ファイル

- [UART.h](../source/ti/drivers/UART.h) - API定義
- [UART.c](../source/ti/drivers/UART.c) - API実装
- [UARTMSPM0.h](../source/ti/drivers/uart/UARTMSPM0.h) - MSPM0ドライバ
- [UARTMSPM0.c](../source/ti/drivers/uart/UARTMSPM0.c) - 割り込みハンドラ

---

**作成日:** 2025-11-06
**SDK:** MSPM0 SDK
**ターゲット:** LP_MSPM0G3507 LaunchPad