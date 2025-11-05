# UARTコールバックとDMA/割り込みの関係について

## 質問の背景

なぜ「DMA × コールバック」の組み合わせはあるのに、「純粋な割り込み × コールバック」の組み合わせがないのか？

## 結論（先に答え）

**実は、純粋な割り込み × コールバックの組み合わせは存在します！**

ただし、**デフォルトでは無効化**されており、`UART_NO_DMA`マクロを定義することで有効になる隠された機能です。

---

## コールバックとは何か

### コールバックの本質

コールバックとは、**非同期処理完了時にアプリケーションに通知する仕組み**です：

```c
// コールバック関数の定義
void myCallback(UART_Handle handle, void *buffer, size_t count,
                void *userArg, int_fast16_t status)
{
    // データ受信完了時に自動的に呼ばれる
    printf("受信完了: %d バイト\n", count);
}

// UART初期化時にコールバックを登録
uartParams.readMode = UART_Mode_CALLBACK;
uartParams.readCallback = myCallback;
```

### ブロッキングモードとの違い

**ブロッキングモード（コールバックなし）:**
```c
// この行でプログラムが待機（ブロック）する
UART_read(uart, buffer, 10, &bytesRead);
printf("受信完了\n");  // データが来るまで実行されない
```

**コールバックモード（非ブロッキング）:**
```c
// すぐに戻る（ブロックしない）
UART_read(uart, buffer, 10, NULL);
// 他の処理ができる
doOtherWork();
// データが来たら自動的にmyCallback()が呼ばれる
```

---

## MSPM0 UARTドライバの実装詳細

### 3つの実装モード

UARTMSPM0.cには、実は**3つの動作モード**が実装されています：

| モード | DMA | コールバック | 有効化方法 |
|--------|-----|-------------|-----------|
| 1. ブロッキング（割り込み） | ❌ | ❌ | デフォルト |
| 2. コールバック（DMA） | ✅ | ✅ | `dmaSupported=true` + `callbacksSupported=true`（デフォルト） |
| 3. コールバック（純粋割り込み） | ❌ | ✅ | `UART_NO_DMA`マクロ定義 + `callbacksSupported=true` |

**重要な訂正:**
- `#ifndef UART_NO_DMA` = **UART_NO_DMAが定義されていない場合**にDMAコードが有効（デフォルト）
- `#ifdef UART_NO_DMA` = **UART_NO_DMAが定義されている場合**に純粋割り込みコールバックが有効

---

## モード1: ブロッキング（純粋割り込み）- デフォルト

### 設定
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .callbacksSupported = false,  // コールバック無効
            .dmaSupported       = false,  // DMA無効
        },
    },
};
```

### 動作 (UARTMSPM0.c:561-589)
```c
if (status & DL_UART_INTERRUPT_RX)
{
    /* RX割り込みで1バイトずつ受信 */
    while (DL_UART_isRXFIFOEmpty(hwAttrs->regs) == false)
    {
        volatile uint8_t rxData = DL_UART_receiveData(hwAttrs->regs);

        /* リングバッファに格納 */
        if (RingBuf_put(&buffersObject->rxBuf, rxData) == -1)
        {
            /* バッファ満杯 - オーバーラン */
        }

        rxPost = true; /* セマフォを解放して待機中のread()を起こす */
    }
}

if (rxPost)
{
    SemaphoreP_post(buffersObject->rxSem);  // read()のブロック解除
}
```

### 特徴
- **割り込みでリングバッファに格納**
- **セマフォでブロッキング**を実装
- コールバックは呼ばれない
- アプリは`UART_read()`で待機

---

## モード2: コールバック（DMA）- デフォルトで有効

### 設定
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .callbacksSupported = true,   // コールバック有効
            .dmaSupported       = true,   // DMA有効
            .noOfDMAChannels    = 1,
        },
    },
};
```

### 初期化 (UARTMSPM0.c:172-188)
```c
if ((params->readMode == UART_Mode_CALLBACK || params->writeMode == UART_Mode_CALLBACK)
    && (uartObject->dmaSupported == true))
{
    /* DMAを初期化 */
    uartObject->DMA_Handle = DMA_Init(&DMATransfer, &dlDMACfg,
                                      uartObject->noOfDMAChannels);

    if (params->readMode == UART_Mode_CALLBACK)
    {
        /* DMA受信完了割り込みを有効化 */
        DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_DMA_DONE_RX);
    }

    if (params->writeMode == UART_Mode_CALLBACK)
    {
        /* DMA送信完了割り込みを有効化 */
        DL_UART_enableInterrupt(hwAttrs->regs,
            DL_UART_INTERRUPT_DMA_DONE_TX | DL_UART_INTERRUPT_EOT_DONE);
    }
}
```

### 動作 (UARTMSPM0.c:480-510)
```c
if ((status & DL_UART_INTERRUPT_DMA_DONE_RX) ||
    (status & DL_UART_INTERRUPT_RX_TIMEOUT_ERROR))
{
    /* DMA受信完了 */
    UARTMSP_dmaStopRx(handle);

    if ((uartObject->readMode == UART_Mode_CALLBACK) && callbackObj->readCount)
    {
        /* リングバッファからユーザーバッファへコピー */
        UARTMSP_readFromRingBuf(handle);

        /* 必要なバイト数を受信したらコールバック呼び出し */
        if (callbackObj->readCount == 0)
        {
            uartObject->readInUse = false;
            callbackObj->readCount = 0;

            /* ★コールバック呼び出し★ */
            callbackObj->readCallback(handle,
                (void *) callbackObj->readBuf, callbackObj->bytesRead,
                callbackObj->userArg, UART_STATUS_SUCCESS);
        }
    }
}
```

### 特徴
- **DMAがハードウェアで自動転送**
- CPU負荷が低い
- 大量データ転送に最適
- DMA完了時にコールバック呼び出し

---

## モード3: コールバック（純粋割り込み）- UART_NO_DMA定義時

### 有効化方法

プロジェクトのビルド設定で`UART_NO_DMA`マクロを定義すると、DMAコードが削除され、純粋な割り込みベースのコールバックが有効になります：

```bash
# コンパイルオプションに追加
-DUART_NO_DMA
```

または、`UARTMSPM0.c`の先頭に追加：
```c
#define UART_NO_DMA
#include <ti/drivers/uart/UARTMSPM0.h>
```

### 設定
```c
UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {
            .callbacksSupported = true,   // コールバック有効
            .dmaSupported       = false,  // DMA無効（重要！）
        },
    },
};
```

### 動作 (UARTMSPM0.c:574-586)
```c
if (status & DL_UART_INTERRUPT_RX)
{
    while (DL_UART_isRXFIFOEmpty(hwAttrs->regs) == false)
    {
        volatile uint8_t rxData = DL_UART_receiveData(hwAttrs->regs);

        /* リングバッファに格納 */
        if (RingBuf_put(&buffersObject->rxBuf, rxData) == -1)
        {
            /* バッファ満杯 */
        }

#ifdef UART_NO_DMA /* ★純粋割り込みモードでのコールバック★ */
        if (callbackObj->readCallback != NULL)
        {
            /* リングバッファからユーザーバッファへ直接コピー */
            count += RingBuf_getn(&buffersObject->rxBuf,
                                  callbackObj->readBuf + count,
                                  callbackObj->readSize - count);

            /* 必要なバイト数を受信完了 */
            if (callbackObj->readSize == count)
            {
                count = 0;  /* カウンタリセット */
                uartObject->readInUse = false;

                /* ★コールバック呼び出し★ */
                callbackObj->readCallback(handle,
                    (void*)callbackObj->readBuf, callbackObj->readSize,
                    callbackObj->userArg, UART_STATUS_SUCCESS);
            }
        }
#endif
        rxPost = true;
    }
}
```

### 送信側の実装 (UARTMSPM0.c:593-622)
```c
if (status & DL_UART_INTERRUPT_TX)
{
#ifdef UART_NO_DMA /* ★純粋割り込みモードでのコールバック★ */
    if (callbackObj->writeCallback != NULL)
    {
        /* 専用の送信コールバック処理 */
        UARTMSP_TxCallback(handle);
    }
    else
    {
#endif
        /* 通常の割り込み送信処理（ブロッキングモード用） */
        do {
            uint8_t txData;
            if (RingBuf_get(&buffersObject->txBuf, &txData) == -1)
            {
                /* 送信データなし */
                DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
                break;
            }
            DL_UART_transmitData(hwAttrs->regs, txData);
        } while (DL_UART_isTXFIFOFull(hwAttrs->regs) == false);
#ifdef UART_NO_DMA
    }
#endif
}
```

### TX専用コールバック処理 (UARTMSPM0.c:375-402)
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
        txData = *(callbackObj->writeBuf + callbackObj->bytesWritten);

        /* 全データ送信完了チェック */
        if (callbackObj->writeSize == callbackObj->bytesWritten)
        {
            /* TX割り込み無効化、EOT割り込み有効化 */
            DL_UART_disableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_TX);
            DL_UART_enableInterrupt(hwAttrs->regs, DL_UART_INTERRUPT_EOT_DONE);
            break;
        }

        /* 1バイト送信 */
        DL_UART_transmitData(hwAttrs->regs, txData);
        callbackObj->bytesWritten++;

    } while (DL_UART_isTXFIFOFull(hwAttrs->regs) == false);
}
```

### 特徴
- **純粋な割り込みでコールバック実現**
- DMA不要（DMAチャネルを節約）
- コード削減（`UART_NO_DMA`）
- 中小規模データ転送に適する
- **デフォルトでは無効**

---

## なぜDMAがデフォルトで有効なのか

### 理由1: DMAの方が効率的

TI（Texas Instruments）は、コールバックモードを使う場合は**DMAを使うべき**と推奨しています：

```c
/* UARTMSPM0.c:41-50 より */
#ifndef UART_NO_DMA /* UART_NO_DMAが未定義 = デフォルト */
#include <ti/drivers/dma/DMAMSPM0.h>
// DMA関連の実装がコンパイルされる
#endif

/* UARTMSPM0.c:172-188 より */
if ((params->readMode == UART_Mode_CALLBACK || params->writeMode == UART_Mode_CALLBACK)
    && (uartObject->dmaSupported == true))
{
    /* コールバックモード = DMAモードを推奨 */
    uartObject->DMA_Handle = DMA_Init(...);
}
```

**DMAの利点:**
- CPU負荷が低い（CPUは割り込み処理から解放される）
- 大量データを高速転送
- タイムクリティカルな処理を邪魔しない

### 理由2: コードサイズ削減の選択肢

メモリが限られた環境では、`UART_NO_DMA`マクロを定義してDMA関連コードを削除できます：

```c
// デフォルト（UART_NO_DMA未定義）
#ifndef UART_NO_DMA
#include <ti/drivers/dma/DMAMSPM0.h>  // DMAコードが含まれる
// ... DMA実装（約2-4KB） ...
#endif

// UART_NO_DMA定義時
#ifdef UART_NO_DMA
// 純粋割り込みコールバック実装が有効（約数百バイト）
#endif
```

**コードサイズ比較（概算）:**
- デフォルト（DMAサポートあり）: ~5-8 KB
- UART_NO_DMA定義（DMAなし）: ~3-4 KB
- 削減量: 約2-4 KB

### 理由3: 用途の違い

| 用途 | 推奨モード |
|------|----------|
| シンプルなデバッグ出力 | ブロッキング（割り込み） |
| 通信プロトコル（少量データ） | ブロッキング（割り込み） |
| 高速・大量データ通信 | コールバック（DMA） |
| リアルタイム処理 | コールバック（DMA） |
| DMAチャネル不足時 | コールバック（純粋割り込み）※隠し機能 |

### 理由4: SDKのサンプルコード構成

SDK提供の2つの例：

**uart_echo (ブロッキング):**
- 初心者向け
- シンプルな実装
- デバッグしやすい
- DMA不使用（`dmaSupported=false`で明示的に無効化）

**uart_callback (DMA):**
- 高度なユーザー向け
- 高効率
- 非ブロッキング
- DMA活用（デフォルトのDMAコードを使用）

**純粋割り込み×コールバックの例が無い理由:**
- 中途半端な性能（DMAより遅い、ブロッキングより複雑）
- 需要が少ない（DMAで十分）
- 必要ならユーザーが`UART_NO_DMA`を定義して実装可能

---

## 実装の選択フローチャート

```
UART通信を実装したい
│
├─ ブロッキングで良い？
│  ├─ YES → モード1: ブロッキング（割り込み）
│  │         例: uart_echo
│  │         設定: dmaSupported=false, callbacksSupported=false
│  │
│  └─ NO（非ブロッキングが必要）
│     │
│     ├─ DMAチャネルが使える？
│     │  ├─ YES → モード2: コールバック（DMA）★推奨★
│     │  │         例: uart_callback
│     │  │         設定: dmaSupported=true, callbacksSupported=true
│     │  │
│     │  └─ NO（DMAチャネルが無い/節約したい）
│     │     │
│     │     └─ モード3: コールバック（純粋割り込み）
│     │               設定: UART_NO_DMA定義 + dmaSupported=false
│     │                    + callbacksSupported=true
│     │               ※自分で実装が必要
```

---

## 各モードの性能比較

| 項目 | モード1<br>ブロッキング | モード2<br>コールバック(DMA) | モード3<br>コールバック(割り込み) |
|------|------------|---------------|------------------|
| **CPU負荷** | 中（セマフォ待機） | 低（DMA自動転送） | 高（割り込み頻発） |
| **データ転送速度** | 中 | 高 | 中〜低 |
| **メモリ使用量** | 少 | 多（DMA設定） | 少 |
| **コードサイズ** | 中 | 大 | 小 |
| **リアルタイム性** | 低（ブロック） | 高 | 中 |
| **実装難易度** | 易 | 易（SDKサポート） | 難（自作必要） |
| **SDK例** | uart_echo | uart_callback | なし |
| **推奨用途** | デバッグ、シンプルな通信 | 高速通信、本番実装 | DMA制約時 |

---

## 具体的な実装例

### モード3の実装方法（純粋割り込み×コールバック）

#### 1. プリプロセッサマクロの定義

**方法A: コンパイラオプション（推奨）**
```makefile
# Makefile or CCS Project Settings
CFLAGS += -DUART_NO_DMA
```

**方法B: ソースコード修正**
```c
// ti_drivers_config.c の先頭に追加
#define UART_NO_DMA
#include "ti_drivers_config.h"
```

#### 2. 設定構造体

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

#### 3. アプリケーションコード

```c
// uartcallback_interrupt.c
#include <stddef.h>
#include <stdint.h>
#include <semaphore.h>
#include "ti_drivers_config.h"

static sem_t rxSema;
static volatile size_t numBytesRead = 0;
static volatile UART_Handle uart;

// 受信コールバック
void rxCallbackFxn(UART_Handle handle, void *buffer, size_t count,
                   void *userArg, int_fast16_t status)
{
    if (status != UART_STATUS_SUCCESS) {
        /* エラー処理 */
        while (1) {}
    }

    numBytesRead = count;
    sem_post(&rxSema);  // メインスレッドを起こす
}

// 送信コールバック
void txCallbackFxn(UART_Handle handle, void *buffer, size_t count,
                   void *userArg, int_fast16_t status)
{
    if (status != UART_STATUS_SUCCESS) {
        /* エラー処理 */
        while (1) {}
    }
    // 送信完了処理
}

void mainThread(void *arg0)
{
    UART_Params uartParams;
    char input;

    // セマフォ初期化
    sem_init(&rxSema, 0, 0);

    // UART初期化
    UART_Params_init(&uartParams);
    uartParams.readMode     = UART_Mode_CALLBACK;   // コールバックモード
    uartParams.writeMode    = UART_Mode_CALLBACK;
    uartParams.baudRate     = 115200;
    uartParams.readCallback = rxCallbackFxn;        // RXコールバック登録
    uartParams.writeCallback = txCallbackFxn;       // TXコールバック登録

    uart = UART_open(CONFIG_UART_0, &uartParams);
    if (uart == NULL) {
        __BKPT();  // エラー
    }

    // エコーループ
    while (1) {
        numBytesRead = 0;

        // 非ブロッキング読み込み開始
        UART_read(uart, &input, 1, NULL);

        // コールバック呼び出しを待機
        sem_wait(&rxSema);

        if (numBytesRead > 0) {
            // 非ブロッキング書き込み
            UART_write(uart, &input, 1, NULL);
        }
    }
}
```

---

## まとめ

### 質問への回答

**Q: なぜ「DMA × コールバック」はあるのに「純粋な割り込み × コールバック」がないのか？**

**A: 実は存在します！ただし以下の理由でデフォルトではDMAが有効になっています：**

### 重要な訂正

プリプロセッサディレクティブの意味：
- `#ifndef UART_NO_DMA` = **UART_NO_DMAが未定義**の場合（デフォルト）→ **DMAコードが有効**
- `#ifdef UART_NO_DMA` = **UART_NO_DMAが定義**された場合 → **純粋割り込みコールバックが有効**

つまり：
- **デフォルト = DMAコードあり**（約5-8KB）
- **UART_NO_DMA定義 = DMAコード削除、純粋割り込みコールバック有効**（約3-4KB）

### なぜデフォルトでDMAが有効なのか

1. **性能面**: DMAの方が圧倒的に効率的
   - 純粋割り込み: 1バイト毎にCPU割り込み（CPU負荷高）
   - DMA: ハードウェアで自動転送、完了時のみ割り込み（CPU負荷低）

2. **設計思想**: TIのSDKの推奨フロー
   - コールバック（非ブロッキング）が必要 → **DMAを使うべき**（デフォルト）
   - シンプルな通信 → ブロッキングで十分（`dmaSupported=false`設定）

3. **柔軟性**: 必要に応じて削減可能
   - デフォルトは機能豊富（DMAサポート）
   - メモリ制約がある場合は`UART_NO_DMA`定義で削減
   - 段階的な最適化が可能

4. **需要**: 用途別の最適化
   - 高性能が必要 → DMA使用（デフォルト）
   - メモリ節約が必要 → `UART_NO_DMA`定義
   - シンプルさが必要 → ブロッキングモード

### 実装の配置

`UARTMSPM0.c`の構造：

**DMA関連コード（デフォルトで有効）:**
- 41-50行: DMAヘッダーと変数（`#ifndef UART_NO_DMA`）
- 172-188行: DMA初期化（`#ifndef UART_NO_DMA`）
- 258-369行: DMA送受信関数（`#ifndef UART_NO_DMA`）
- 479-560行: DMA完了割り込み処理（`#ifndef UART_NO_DMA`）
- 690-788行: DMA停止・タイムアウト関数（`#ifndef UART_NO_DMA`）

**純粋割り込みコールバック（UART_NO_DMA定義時のみ）:**
- 370-402行: TX専用コールバック関数（`#else`ブロック）
- 574-586行: RX割り込みでのコールバック処理（`#ifdef UART_NO_DMA`）
- 593-622行: TX割り込みでのコールバック処理（`#ifdef UART_NO_DMA`）

### UART_NO_DMAを定義する理由

以下の場合に`UART_NO_DMA`を定義してDMAコードを削除：
- DMAチャネルが他の用途で必要（ADC、SPI等）
- Flash/RAMメモリが非常に限られている（2-4KB削減）
- DMAを使いたくない（シンプルな実装を優先）
- デバッグを容易にしたい（DMAの複雑さを排除）

---

## 参考ファイル

### ドライバ実装
- [UARTMSPM0.c:172-195](../source/ti/drivers/uart/UARTMSPM0.c#L172-L195) - 初期化でのDMA/割り込み選択
- [UARTMSPM0.c:480-510](../source/ti/drivers/uart/UARTMSPM0.c#L480-L510) - DMAコールバック処理
- [UARTMSPM0.c:574-586](../source/ti/drivers/uart/UARTMSPM0.c#L574-L586) - 純粋割り込みコールバック（RX）
- [UARTMSPM0.c:593-622](../source/ti/drivers/uart/UARTMSPM0.c#L593-L622) - 純粋割り込みコールバック（TX）
- [UARTMSPM0.c:375-402](../source/ti/drivers/uart/UARTMSPM0.c#L375-L402) - TX専用コールバック関数

### 例
- [uart_echo](../examples/rtos/LP_MSPM0G3507/drivers/uart_echo/) - モード1: ブロッキング
- [uart_callback](../examples/rtos/LP_MSPM0G3507/drivers/uart_callback/) - モード2: コールバック（DMA）

---

**作成日:** 2025-11-06
**SDK:** MSPM0 SDK
**ターゲット:** LP_MSPM0G3507 LaunchPad
