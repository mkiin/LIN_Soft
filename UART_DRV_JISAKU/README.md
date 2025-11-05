# UART_RegInt Driver

## 概要

UART_RegIntは、DMA機能を使用しないレジスタ割り込みベースのUARTドライバです。Texas Instruments社のUART2ドライバとAPI互換性を持ち、CC2340R53（CC23X0R5）マイコン向けに最適化されています。

## 特徴

- **DMA不要**: レジスタ直接アクセスと割り込み処理のみで動作
- **TI UART2互換API**: 既存のUART2コードからの移行が容易
- **2つの動作モード**: BLOCKING（同期）/ CALLBACK（非同期）
- **エラー検出**: Overrun, Framing, Parity, Break エラーに対応
- **電源管理統合**: PowerLPF3ドライバと連携
- **低レイテンシ**: 8バイトFIFOを活用した効率的な割り込み処理

## ファイル構成

```
UART_DRV/
├── UART_RegInt.h              # 公開API
├── UART_RegInt.c              # 共通実装
├── uart_regint/               # デバイス固有実装
│   ├── UART_RegIntLPF3.h      # CC23X0R5固有定義
│   ├── UART_RegIntLPF3.c      # CC23X0R5固有実装
│   └── UART_RegIntSupport.h   # 内部サポート関数
└── README.md                  # このファイル
```

## 使用方法

### 1. 初期化

```c
#include "UART_RegInt.h"

// モジュール初期化（main関数で1回のみ）
UART_RegInt_init();

// パラメータ設定
UART_RegInt_Params params;
UART_RegInt_Params_init(&params);
params.baudRate = 9600;
params.readMode = UART_REGINT_MODE_CALLBACK;
params.writeMode = UART_REGINT_MODE_CALLBACK;
params.readCallback = myReadCallback;
params.writeCallback = myWriteCallback;

// UART開始
UART_RegInt_Handle handle = UART_RegInt_open(0, &params);
if (handle == NULL) {
    // エラー処理
}
```

### 2. データ送信

#### BLOCKINGモード
```c
uint8_t txBuf[8] = {0x55, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
int_fast16_t result = UART_RegInt_write(handle, txBuf, 8);
if (result == 8) {
    // 送信成功
}
```

#### CALLBACKモード
```c
void myWriteCallback(UART_RegInt_Handle handle, void *buf, size_t count,
                    void *userArg, int_fast16_t status)
{
    if (status == UART_REGINT_STATUS_SUCCESS) {
        // 送信完了
    }
}

// 送信開始（即座に戻る）
UART_RegInt_write(handle, txBuf, 8);
```

### 3. データ受信

#### BLOCKINGモード
```c
uint8_t rxBuf[8];
int_fast16_t result = UART_RegInt_read(handle, rxBuf, 8);
if (result == 8) {
    // 8バイト受信成功
}
```

#### CALLBACKモード
```c
void myReadCallback(UART_RegInt_Handle handle, void *buf, size_t count,
                   void *userArg, int_fast16_t status)
{
    if (status == UART_REGINT_STATUS_SUCCESS) {
        // 受信完了
        processData(buf, count);
    }
}

// 受信開始（即座に戻る）
UART_RegInt_read(handle, rxBuf, 8);
```

### 4. タイムアウト付き送受信

```c
// 100msタイムアウト
int_fast16_t result = UART_RegInt_readTimeout(handle, rxBuf, 8, 100000);
if (result == UART_REGINT_STATUS_ETIMEOUT) {
    // タイムアウト
}
```

### 5. 受信制御

```c
// 受信有効化（スタンバイ禁止制約を設定）
UART_RegInt_rxEnable(handle);

// 受信無効化（スタンバイ禁止制約を解除）
UART_RegInt_rxDisable(handle);
```

## 設定例（ti_drivers_config.c）

```c
#include "UART_RegInt.h"
#include "uart_regint/UART_RegIntLPF3.h"

/* UART objects */
UART_RegIntLPF3_Object uart_RegIntLPF3Objects[1];

/* UART hardware attributes */
const UART_RegIntLPF3_HWAttrs uart_RegIntLPF3HWAttrs[1] = {
    {
        .baseAddr     = UART0_BASE,
        .intNum       = INT_UART0_COMB,
        .intPriority  = 0x20,
        .rxPin        = CONFIG_GPIO_UART_0_RX,
        .txPin        = CONFIG_GPIO_UART_0_TX,
        .ctsPin       = GPIO_INVALID_INDEX,
        .rtsPin       = GPIO_INVALID_INDEX,
        .flowControl  = UART_REGINT_FLOWCTRL_NONE,
        .rxBufPtr     = NULL,
        .rxBufSize    = 0,
        .txBufPtr     = NULL,
        .txBufSize    = 0,
        .txPinMux     = GPIO_MUX_PORTCFG_PFUNC3,
        .rxPinMux     = GPIO_MUX_PORTCFG_PFUNC3,
        .ctsPinMux    = GPIO_MUX_GPIO_INTERNAL,
        .rtsPinMux    = GPIO_MUX_GPIO_INTERNAL,
        .powerID      = PowerLPF3_PERIPH_UART0,
    },
};

/* UART configuration */
const UART_RegInt_Config UART_RegInt_config[1] = {
    {
        .object  = &uart_RegIntLPF3Objects[0],
        .hwAttrs = &uart_RegIntLPF3HWAttrs[0]
    },
};

const uint_least8_t UART_RegInt_count = 1;
```

## TI UART2からの移行

### API対応表

| TI UART2 | UART_RegInt | 備考 |
|----------|-------------|------|
| `UART2_init()` | `UART_RegInt_init()` | 同じ |
| `UART2_open()` | `UART_RegInt_open()` | 同じ |
| `UART2_close()` | `UART_RegInt_close()` | 同じ |
| `UART2_read()` | `UART_RegInt_read()` | 同じ |
| `UART2_write()` | `UART_RegInt_write()` | 同じ |
| `UART2_readTimeout()` | `UART_RegInt_readTimeout()` | 同じ |
| `UART2_writeTimeout()` | `UART_RegInt_writeTimeout()` | 同じ |

### 移行手順

1. **インクルード変更**:
   ```c
   // 変更前
   #include <ti/drivers/UART2.h>
   
   // 変更後
   #include "UART_RegInt.h"
   ```

2. **型名変更**:
   ```c
   // 変更前
   UART2_Handle handle;
   UART2_Params params;
   
   // 変更後
   UART_RegInt_Handle handle;
   UART_RegInt_Params params;
   ```

3. **マクロ変更**:
   ```c
   // 変更前
   UART2_Mode_CALLBACK
   
   // 変更後
   UART_REGINT_MODE_CALLBACK
   ```

## パフォーマンス

### LIN通信での性能（9600 bps）

- **1バイト送信時間**: 1.04 ms
- **8バイトフレーム**: 8.3 ms
- **FIFO深度**: 8バイト
- **割り込み頻度**: 
  - RX: 約6.2ms間隔（FIFO 6/8到達時）
  - TX: 約2.1ms間隔（FIFO 2/8到達時）
- **割り込み処理時間**: ~10μs（1バイト処理）

**結論**: 割り込み処理時間はフレーム時間の0.12%程度であり、LIN通信に十分な性能を提供します。

## エラー処理

### エラーコード

| コード | 値 | 説明 |
|--------|---|------|
| `UART_REGINT_STATUS_SUCCESS` | 0 | 成功 |
| `UART_REGINT_STATUS_EOVERRUN` | -8 | オーバーランエラー |
| `UART_REGINT_STATUS_EFRAMING` | -5 | フレーミングエラー |
| `UART_REGINT_STATUS_EPARITY` | -6 | パリティエラー |
| `UART_REGINT_STATUS_EBREAK` | -7 | ブレークエラー |
| `UART_REGINT_STATUS_ETIMEOUT` | -3 | タイムアウト |
| `UART_REGINT_STATUS_ECANCELLED` | -4 | キャンセル |

### イベントコールバック

エラー発生時に即座に通知を受け取ることができます：

```c
void myEventCallback(UART_RegInt_Handle handle, uint32_t event,
                    uint32_t data, void *userArg)
{
    if (event & UART_REGINT_EVENT_OVERRUN) {
        // オーバーランエラー処理
    }
    if (event & UART_REGINT_EVENT_FRAMING) {
        // フレーミングエラー処理
    }
}

params.eventCallback = myEventCallback;
params.eventMask = UART_REGINT_EVENT_OVERRUN | UART_REGINT_EVENT_FRAMING;
```

## 制限事項

1. **DMA非対応**: 高速大容量転送には不向き（LIN通信では問題なし）
2. **同時トランザクション**: 送信/受信それぞれ1つまで
3. **対応デバイス**: CC2340R53（CC23X0R5ファミリー）

## デバッグ

### よくある問題

**問題**: データが受信できない
- **確認**: `UART_RegInt_rxEnable()`を呼び出しているか
- **確認**: 割り込み優先度が適切か（0以外）
- **確認**: GPIO設定が正しいか

**問題**: オーバーランエラーが頻発
- **原因**: 割り込み処理が間に合っていない
- **対策**: 割り込み優先度を上げる、または他の処理を軽量化

**問題**: 送信が完了しない
- **確認**: writeCallbackが呼ばれているか
- **確認**: TX割り込みが有効化されているか

### オーバーランカウント取得

```c
uint32_t overruns = UART_RegInt_getOverrunCount(handle);
if (overruns > 0) {
    // オーバーランが発生
}
```

## ライセンス

このドライバは、Texas Instruments社のUART2ドライバの構造をベースにしていますが、独自実装です。

## 参照

- Texas Instruments CC2340R53 Technical Reference Manual
- TI-RTOS Kernel User's Guide
- LIN Specification 2.2A

## 変更履歴

| バージョン | 日付 | 変更内容 |
|-----------|------|---------|
| 1.0 | 2025-01-26 | 初版リリース |

---

**作成者**: Based on TI UART2 driver structure  
**対象デバイス**: Texas Instruments CC2340R53 (CC23X0R5)
