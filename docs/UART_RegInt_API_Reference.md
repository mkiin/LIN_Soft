# UART_RegInt API リファレンス

## 概要

UART_RegIntドライバは、DMA機能を使用せずレジスタ割り込みベースでUART通信を行うドライバです。
TI社のUART2ドライバとAPI互換性を持たせており、LINソフトからの移行が容易です。

## 特徴

- **DMA不要**: レジスタ直接アクセスと割り込み処理のみで動作
- **TI UART2互換API**: 既存コードからの移行が容易
- **2つの動作モード**: BLOCKING (同期) / CALLBACK (非同期)
- **エラー検出**: Overrun, Framing, Parity, Break エラーに対応
- **電源管理統合**: PowerLPF3ドライバと連携
- **CC2340R53最適化**: CC23X0ファミリ向けに最適化

---

## 初期化と設定

### UART_RegInt_init()

```c
void UART_RegInt_init(void)
```

**説明**: UARTドライバモジュールを初期化します。他のAPI関数を呼び出す前に1回だけ呼び出してください。

**パラメータ**: なし

**戻り値**: なし

**使用例**:
```c
UART_RegInt_init();  // main()の最初で呼び出し
```

---

### UART_RegInt_Params_init()

```c
void UART_RegInt_Params_init(UART_RegInt_Params *params)
```

**説明**: UART_RegInt_Params構造体をデフォルト値で初期化します。

**パラメータ**:
- `params` [out]: 初期化するパラメータ構造体へのポインタ

**デフォルト値**:
| パラメータ | デフォルト値 |
|-----------|------------|
| readMode | UART_REGINT_MODE_BLOCKING |
| writeMode | UART_REGINT_MODE_BLOCKING |
| readCallback | NULL |
| writeCallback | NULL |
| eventCallback | NULL |
| eventMask | 0 |
| readReturnMode | UART_REGINT_ReadReturnMode_FULL |
| baudRate | 9600 |
| dataLength | UART_REGINT_DataLen_8 |
| stopBits | UART_REGINT_StopBits_1 |
| parityType | UART_REGINT_Parity_NONE |
| userArg | NULL |

**使用例**:
```c
UART_RegInt_Params params;
UART_RegInt_Params_init(&params);
params.baudRate = 19200;
params.readMode = UART_REGINT_MODE_CALLBACK;
params.readCallback = myReadCallback;
```

---

### UART_RegInt_open()

```c
UART_RegInt_Handle UART_RegInt_open(uint_least8_t index, 
                                    UART_RegInt_Params *params)
```

**説明**: 指定されたインデックスのUARTインスタンスを開きます。

**パラメータ**:
- `index` [in]: UART設定テーブルのインデックス (通常は0)
- `params` [in]: UARTパラメータ (NULLの場合デフォルト値)

**戻り値**:
- 成功: UARTハンドル
- 失敗: NULL

**注意事項**:
- 同じインデックスを複数回開くことはできません
- 失敗の原因: インデックスが範囲外、既に開かれている、リソース不足

**使用例**:
```c
UART_RegInt_Handle handle;
UART_RegInt_Params params;

UART_RegInt_Params_init(&params);
params.baudRate = 9600;
params.readMode = UART_REGINT_MODE_CALLBACK;
params.readCallback = lin_rx_callback;

handle = UART_RegInt_open(0, &params);
if (handle == NULL) {
    // エラー処理
}
```

---

### UART_RegInt_close()

```c
void UART_RegInt_close(UART_RegInt_Handle handle)
```

**説明**: UARTインスタンスを閉じ、リソースを解放します。

**パラメータ**:
- `handle` [in]: UARTハンドル

**動作**:
- 実行中の送受信処理をキャンセル
- 割り込みを無効化
- UARTハードウェアを無効化
- 電源依存関係を解除

**使用例**:
```c
UART_RegInt_close(handle);
```

---

## データ送受信

### UART_RegInt_read()

```c
int_fast16_t UART_RegInt_read(UART_RegInt_Handle handle,
                              void *buf,
                              size_t size)
```

**説明**: UARTからデータを受信します。

**パラメータ**:
- `handle` [in]: UARTハンドル
- `buf` [out]: 受信バッファ
- `size` [in]: 受信バイト数

**戻り値**:
- **BLOCKINGモード**: 
  - 成功: 受信バイト数
  - 失敗: エラーコード (負の値)
- **CALLBACKモード**: 
  - 成功: UART_REGINT_STATUS_SUCCESS (0)
  - 失敗: エラーコード (負の値)

**動作モード別の挙動**:

#### BLOCKINGモード
- 関数は指定バイト数の受信が完了するまでブロック
- 受信完了後、受信バイト数を返す
- エラー発生時はエラーコードを返す

#### CALLBACKモード
- 関数は即座に戻る
- 受信完了時に`readCallback`が呼び出される
- コールバックは割り込みコンテキストで実行される

**使用例**:

```c
// BLOCKINGモード
uint8_t rxBuf[8];
int_fast16_t result = UART_RegInt_read(handle, rxBuf, 8);
if (result == 8) {
    // 8バイト受信成功
} else if (result < 0) {
    // エラー発生
}

// CALLBACKモード
void myReadCallback(UART_RegInt_Handle handle, void *buf, size_t count,
                   void *userArg, int_fast16_t status)
{
    if (status == UART_REGINT_STATUS_SUCCESS) {
        // count バイト受信成功
        processReceivedData(buf, count);
    }
}

// 呼び出し側
UART_RegInt_read(handle, rxBuf, 8);  // 即座に戻る
```

---

### UART_RegInt_readTimeout()

```c
int_fast16_t UART_RegInt_readTimeout(UART_RegInt_Handle handle,
                                     void *buf,
                                     size_t size,
                                     uint32_t timeout)
```

**説明**: タイムアウト付きでUARTからデータを受信します。

**パラメータ**:
- `handle` [in]: UARTハンドル
- `buf` [out]: 受信バッファ
- `size` [in]: 受信バイト数
- `timeout` [in]: タイムアウト (マイクロ秒), UART_REGINT_WAIT_FOREVERで無限待機

**戻り値**:
- 成功: 受信バイト数
- タイムアウト: UART_REGINT_STATUS_ETIMEOUT
- エラー: エラーコード (負の値)

**注意事項**:
- `readReturnMode`が`UART_REGINT_ReadReturnMode_PARTIAL`の場合、タイムアウト時でも部分受信データを返す

**使用例**:
```c
uint8_t rxBuf[8];
// 100msタイムアウト
int_fast16_t result = UART_RegInt_readTimeout(handle, rxBuf, 8, 100000);
if (result == 8) {
    // 8バイト受信成功
} else if (result == UART_REGINT_STATUS_ETIMEOUT) {
    // タイムアウト
} else if (result > 0) {
    // 部分受信 (ReadReturnMode_PARTIALの場合)
}
```

---

### UART_RegInt_readCancel()

```c
void UART_RegInt_readCancel(UART_RegInt_Handle handle)
```

**説明**: 実行中の受信処理をキャンセルします。

**パラメータ**:
- `handle` [in]: UARTハンドル

**動作**:
- 受信割り込みを無効化
- CALLBACKモード: コールバックが`UART_REGINT_STATUS_ECANCELLED`で呼び出される
- BLOCKINGモード: ブロックしているスレッドが起床

**使用例**:
```c
UART_RegInt_readCancel(handle);
```

---

### UART_RegInt_write()

```c
int_fast16_t UART_RegInt_write(UART_RegInt_Handle handle,
                               const void *buf,
                               size_t size)
```

**説明**: UARTへデータを送信します。

**パラメータ**:
- `handle` [in]: UARTハンドル
- `buf` [in]: 送信バッファ
- `size` [in]: 送信バイト数

**戻り値**:
- **BLOCKINGモード**: 
  - 成功: 送信バイト数
  - 失敗: エラーコード (負の値)
- **CALLBACKモード**: 
  - 成功: UART_REGINT_STATUS_SUCCESS (0)
  - 失敗: エラーコード (負の値)

**動作**:
1. 送信バッファのデータを可能な限りFIFOに書き込み
2. FIFOが満杯になったら送信割り込みを有効化
3. 割り込みハンドラで残りのデータを送信

**使用例**:

```c
// BLOCKINGモード
uint8_t txBuf[8] = {0x55, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
int_fast16_t result = UART_RegInt_write(handle, txBuf, 8);
if (result == 8) {
    // 8バイト送信成功
}

// CALLBACKモード
void myWriteCallback(UART_RegInt_Handle handle, void *buf, size_t count,
                    void *userArg, int_fast16_t status)
{
    if (status == UART_REGINT_STATUS_SUCCESS) {
        // 送信完了
        startNextTransmission();
    }
}

UART_RegInt_write(handle, txBuf, 8);  // 即座に戻る
```

---

### UART_RegInt_writeTimeout()

```c
int_fast16_t UART_RegInt_writeTimeout(UART_RegInt_Handle handle,
                                      const void *buf,
                                      size_t size,
                                      uint32_t timeout)
```

**説明**: タイムアウト付きでUARTへデータを送信します。

**パラメータ**:
- `handle` [in]: UARTハンドル
- `buf` [in]: 送信バッファ
- `size` [in]: 送信バイト数
- `timeout` [in]: タイムアウト (マイクロ秒), UART_REGINT_WAIT_FOREVERで無限待機

**戻り値**:
- 成功: 送信バイト数
- タイムアウト: UART_REGINT_STATUS_ETIMEOUT
- エラー: エラーコード (負の値)

**使用例**:
```c
uint8_t txBuf[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
// 50msタイムアウト
int_fast16_t result = UART_RegInt_writeTimeout(handle, txBuf, 8, 50000);
if (result == 8) {
    // 送信成功
} else if (result == UART_REGINT_STATUS_ETIMEOUT) {
    // タイムアウト
}
```

---

### UART_RegInt_writeCancel()

```c
void UART_RegInt_writeCancel(UART_RegInt_Handle handle)
```

**説明**: 実行中の送信処理をキャンセルします。

**パラメータ**:
- `handle` [in]: UARTハンドル

**使用例**:
```c
UART_RegInt_writeCancel(handle);
```

---

## 受信制御

### UART_RegInt_rxEnable()

```c
void UART_RegInt_rxEnable(UART_RegInt_Handle handle)
```

**説明**: UART受信を有効化し、低消費電力モード制約を設定します。

**パラメータ**:
- `handle` [in]: UARTハンドル

**動作**:
- 受信有効フラグをセット
- `PowerLPF3_DISALLOW_STANDBY`制約を設定 (スタンバイ禁止)

**使用例**:
```c
UART_RegInt_rxEnable(handle);
```

---

### UART_RegInt_rxDisable()

```c
void UART_RegInt_rxDisable(UART_RegInt_Handle handle)
```

**説明**: UART受信を無効化し、低消費電力モード制約を解除します。

**パラメータ**:
- `handle` [in]: UARTハンドル

**使用例**:
```c
UART_RegInt_rxDisable(handle);
```

---

## ステータス取得

### UART_RegInt_getOverrunCount()

```c
uint32_t UART_RegInt_getOverrunCount(UART_RegInt_Handle handle)
```

**説明**: 発生したオーバーランエラーの累積カウントを取得します。

**パラメータ**:
- `handle` [in]: UARTハンドル

**戻り値**: オーバーランカウント

**使用例**:
```c
uint32_t overruns = UART_RegInt_getOverrunCount(handle);
if (overruns > 0) {
    // オーバーラン発生
}
```

---

## データ型とマクロ

### ステータスコード

| マクロ | 値 | 説明 |
|--------|---|------|
| UART_REGINT_STATUS_SUCCESS | 0 | 成功 |
| UART_REGINT_STATUS_ERROR | -1 | 一般エラー |
| UART_REGINT_STATUS_EINUSE | -2 | リソース使用中 |
| UART_REGINT_STATUS_ETIMEOUT | -3 | タイムアウト |
| UART_REGINT_STATUS_ECANCELLED | -4 | キャンセル |
| UART_REGINT_STATUS_EFRAMING | -5 | フレーミングエラー |
| UART_REGINT_STATUS_EPARITY | -6 | パリティエラー |
| UART_REGINT_STATUS_EBREAK | -7 | ブレークエラー |
| UART_REGINT_STATUS_EOVERRUN | -8 | オーバーランエラー |
| UART_REGINT_STATUS_EINVALID | -9 | 無効なパラメータ |

### イベントフラグ

| マクロ | 値 | 説明 |
|--------|---|------|
| UART_REGINT_EVENT_OVERRUN | 0x01 | オーバーランエラー発生 |
| UART_REGINT_EVENT_BREAK | 0x02 | ブレークエラー発生 |
| UART_REGINT_EVENT_PARITY | 0x04 | パリティエラー発生 |
| UART_REGINT_EVENT_FRAMING | 0x08 | フレーミングエラー発生 |
| UART_REGINT_EVENT_TX_BEGIN | 0x10 | 送信開始 |
| UART_REGINT_EVENT_TX_FINISHED | 0x20 | 送信完了 |

### 動作モード

```c
typedef enum {
    UART_REGINT_MODE_BLOCKING = 0,  // ブロッキングモード
    UART_REGINT_MODE_CALLBACK,      // コールバックモード
} UART_RegInt_Mode;
```

### データ設定

```c
typedef enum {
    UART_REGINT_DataLen_5 = 0,
    UART_REGINT_DataLen_6,
    UART_REGINT_DataLen_7,
    UART_REGINT_DataLen_8,  // デフォルト
} UART_REGINT_DataLen;

typedef enum {
    UART_REGINT_StopBits_1 = 0,  // デフォルト
    UART_REGINT_StopBits_2,
} UART_REGINT_StopBits;

typedef enum {
    UART_REGINT_Parity_NONE = 0,  // デフォルト
    UART_REGINT_Parity_EVEN,
    UART_REGINT_Parity_ODD,
    UART_REGINT_Parity_ZERO,
    UART_REGINT_Parity_ONE,
} UART_REGINT_Parity;
```

---

## LINソフトからの使用例

### 初期化コード

```c
#include "UART_RegInt.h"

static UART_RegInt_Handle uart_handle;

void lin_uart_init(void)
{
    UART_RegInt_Params params;
    
    // モジュール初期化
    UART_RegInt_init();
    
    // パラメータ設定
    UART_RegInt_Params_init(&params);
    params.baudRate = 9600;  // または19200
    params.readMode = UART_REGINT_MODE_CALLBACK;
    params.writeMode = UART_REGINT_MODE_CALLBACK;
    params.readCallback = l_ifc_rx_ch1;  // LIN受信コールバック
    params.writeCallback = l_ifc_tx_ch1;  // LIN送信コールバック
    params.eventCallback = NULL;
    params.dataLength = UART_REGINT_DataLen_8;
    params.stopBits = UART_REGINT_StopBits_1;
    params.parityType = UART_REGINT_Parity_NONE;
    
    // UART開始
    uart_handle = UART_RegInt_open(0, &params);
    if (uart_handle == NULL) {
        // エラー処理
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;
    }
}
```

### 受信開始 (既存のl_vog_lin_rx_enb相当)

```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
{
    // 受信開始
    UART_RegInt_read(uart_handle, u1l_lin_rx_buf, u1a_lin_rx_data_size);
}
```

### 送信 (既存のl_vog_lin_tx_char相当)

```c
void l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    // 送信バッファにコピー
    memcpy(u1l_lin_tx_buf, u1a_lin_data, u1a_lin_data_size);
    
    // 送信開始
    UART_RegInt_write(uart_handle, u1l_lin_tx_buf, u1a_lin_data_size);
}
```

### 受信コールバック (既存のl_ifc_rx_ch1)

```c
void l_ifc_rx_ch1(UART_RegInt_Handle handle, void *buf, size_t count,
                 void *userArg, int_fast16_t status)
{
    l_u8 u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;
    
    // エラーチェック
    if (status != UART_REGINT_STATUS_SUCCESS) {
        // エラー処理
        if (status == UART_REGINT_STATUS_EOVERRUN) {
            u1a_lin_rx_set_err = U1G_LIN_OVERRUN_ERR;
        } else if (status == UART_REGINT_STATUS_EFRAMING) {
            u1a_lin_rx_set_err = U1G_LIN_FRAMING_ERR;
        }
    }
    
    // SLIN COREレイヤに通知
    l_vog_lin_rx_int((l_u8 *)buf, u1a_lin_rx_set_err);
}
```

---

## TI UART2からの移行ガイド

### API対応表

| TI UART2 | UART_RegInt | 備考 |
|----------|-------------|------|
| UART2_init() | UART_RegInt_init() | 同じ |
| UART2_Params_init() | UART_RegInt_Params_init() | 同じ |
| UART2_open() | UART_RegInt_open() | 同じ |
| UART2_close() | UART_RegInt_close() | 同じ |
| UART2_read() | UART_RegInt_read() | 同じ |
| UART2_readTimeout() | UART_RegInt_readTimeout() | 同じ |
| UART2_readCancel() | UART_RegInt_readCancel() | 同じ |
| UART2_write() | UART_RegInt_write() | 同じ |
| UART2_writeTimeout() | UART_RegInt_writeTimeout() | 同じ |
| UART2_writeCancel() | UART_RegInt_writeCancel() | 同じ |
| UART2_rxEnable() | UART_RegInt_rxEnable() | 同じ |
| UART2_rxDisable() | UART_RegInt_rxDisable() | 同じ |

### パラメータ対応表

| TI UART2 | UART_RegInt |
|----------|-------------|
| UART2_Mode_BLOCKING | UART_REGINT_MODE_BLOCKING |
| UART2_Mode_CALLBACK | UART_REGINT_MODE_CALLBACK |
| UART2_DataLen_8 | UART_REGINT_DataLen_8 |
| UART2_StopBits_1 | UART_REGINT_StopBits_1 |
| UART2_Parity_NONE | UART_REGINT_Parity_NONE |

### 変更手順

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

3. **API呼び出し変更**:
```c
// 変更前
UART2_init();
UART2_Params_init(&params);
handle = UART2_open(CONFIG_UART_INDEX, &params);

// 変更後
UART_RegInt_init();
UART_RegInt_Params_init(&params);
handle = UART_RegInt_open(0, &params);
```

4. **マクロ変更**:
```c
// 変更前
params.readMode = UART2_Mode_CALLBACK;

// 変更後
params.readMode = UART_REGINT_MODE_CALLBACK;
```

---

## 注意事項

### 割り込みコンテキスト

- コールバック関数は割り込みコンテキストで実行されます
- コールバック内でブロッキング処理を行わないでください
- 長時間の処理はイベントフラグ等で通知し、タスクコンテキストで実行してください

### 同時送受信

- 送信と受信は同時に実行できます
- 同一方向の複数トランザクションは不可 (UART_REGINT_STATUS_EINUSEエラー)

### バッファ管理

- コールバック完了まで送受信バッファを保持してください
- バッファはグローバル変数またはstatic変数として確保を推奨

### 電源管理

- `rxEnable()`呼び出し中はスタンバイモードに入りません
- 受信が不要な期間は`rxDisable()`を呼び出してください

---

## よくある質問

**Q: DMAを使用しないことで性能は低下しますか?**

A: LIN通信のような低ボーレート (2400-19200 bps) では、8バイトFIFOと割り込み処理で十分な性能が得られます。
   1フレーム(8バイト)の送信時間は9600bpsで約8.3msであり、割り込み応答時間は問題になりません。

**Q: 既存のLINソフトとの互換性は?**

A: コールバック関数のシグネチャがTI UART2と同じため、LINソフトの既存コールバック(`l_ifc_rx_ch1`, `l_ifc_tx_ch1`)をそのまま使用できます。

**Q: エラー検出はどのように行われますか?**

A: ハードウェアのUARTエラーステータスレジスタ(RSR/ECR)を読み取り、以下のエラーを検出します:
   - Overrun Error (OE)
   - Break Error (BE)
   - Parity Error (PE)
   - Framing Error (FE)

---

## 関連ドキュメント

- [UART_RegInt_Design.md](UART_RegInt_Design.md) - 設計ドキュメント
- [CLAUDE.md](../CLAUDE.md) - プロジェクト全体の概要
- TI CC2340R53 Technical Reference Manual - UARTレジスタ詳細
