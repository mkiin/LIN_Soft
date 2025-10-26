# UART_RegInt 設計ドキュメント

## 目的

Texas Instruments CC2340R53マイコン向けに、DMA機能を使用しないレジスタ割り込みベースのUARTドライバを提供します。
LIN通信スタックでの使用を主目的とし、TI社のUART2ドライバとAPI互換性を持たせることで移行を容易にします。

---

## 設計方針

### 1. DMA非依存

**理由**:
- LIN通信は比較的低速 (2400-19200 bps)
- 1フレームあたり8バイトと小容量
- レジスタ割り込み処理で十分な性能
- DMA設定の複雑さを回避

**実装**:
- UARTレジスタ直接アクセス
- 8バイトFIFO活用
- TX/RX割り込みハンドラでバイト単位処理

### 2. TI UART2 API互換

**理由**:
- 既存LINソフトからの移行容易性
- TIエコシステムとの親和性
- 開発者の学習コスト削減

**実装**:
- 同一の関数名・シグネチャ
- 同一のパラメータ構造体
- 同一のコールバック型定義

### 3. CC2340R53最適化

**理由**:
- デバイス固有のレジスタ配置
- CC23X0ファミリ特有の電源管理
- 正確なクロック設定

**実装**:
- UART0_BASE = 0x40034000
- PowerLPF3_PERIPH_UART0使用
- CPUクロックをそのまま使用 (÷2不要)

---

## アーキテクチャ

### レイヤ構造

```
┌─────────────────────────────────────────┐
│     LINソフト (l_slin_drv_cc2340r53.c)   │
│  - l_ifc_rx_ch1() コールバック           │
│  - l_ifc_tx_ch1() コールバック           │
└─────────────────────────────────────────┘
                    ↓ UART_RegInt API
┌─────────────────────────────────────────┐
│      UART_RegInt.h (公開API)             │
│  - UART_RegInt_open/close               │
│  - UART_RegInt_read/write               │
│  - UART_RegInt_rxEnable/rxDisable       │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│      UART_RegInt.c (実装)                │
│  - 送受信トランザクション管理            │
│  - 割り込みハンドラ                      │
│  - エラー処理                            │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  UART_RegInt_Priv.h (内部定義)           │
│  - オブジェクト構造体                    │
│  - 状態管理                              │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  UART_RegInt_Config.h (デバイス設定)     │
│  - CC2340R53固有定数                     │
│  - ベースアドレス、割り込み番号          │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  TI DriverLib (driverlib/uart.h)        │
│  - UARTConfigSetExpClk()                │
│  - UARTEnableInt/DisableInt()           │
│  - UARTGetCharNonBlocking()             │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  UART0 Hardware (CC2340R53)             │
│  - Base: 0x40034000                     │
│  - 8-byte FIFO                          │
│  - Interrupt: INT_UART0_COMB (27)       │
└─────────────────────────────────────────┘
```

---

## データ構造

### UART_RegInt_Object

ドライバのランタイム状態を保持する構造体:

```c
struct UART_RegInt_Object_ {
    // 設定
    UART_RegInt_Params params;       // ユーザー設定
    uint32_t baseAddr;               // UARTベースアドレス
    
    // 状態管理
    UART_RegInt_StateFlags state;    // 状態フラグ
    uint32_t overrunCount;           // オーバーランカウント
    
    // ハードウェアリソース
    HwiP_Struct hwi;                 // 割り込みオブジェクト
    SemaphoreP_Struct readSem;       // 読み込み同期
    SemaphoreP_Struct writeSem;      // 書き込み同期
    
    // RXトランザクション
    uint8_t *rxBuf;                  // 受信バッファ
    size_t rxSize;                   // 受信要求サイズ
    size_t rxCount;                  // 受信済みバイト数
    int_fast16_t rxStatus;           // 受信ステータス
    
    // TXトランザクション
    const uint8_t *txBuf;            // 送信バッファ
    size_t txSize;                   // 送信要求サイズ
    size_t txCount;                  // 送信済みバイト数
    int_fast16_t txStatus;           // 送信ステータス
    
    // 電源管理
    Power_NotifyObj preNotify;       // スタンバイ前
    Power_NotifyObj postNotify;      // スタンバイ後
};
```

### 状態フラグ (UART_RegInt_StateFlags)

ビットフィールドで効率的に状態管理:

```c
typedef struct {
    uint8_t opened       : 1;  // ドライバが開かれているか
    uint8_t rxEnabled    : 1;  // 受信が有効か
    uint8_t txEnabled    : 1;  // 送信が有効か
    uint8_t rxInProgress : 1;  // 受信処理中か
    uint8_t txInProgress : 1;  // 送信処理中か
    uint8_t rxCancelled  : 1;  // 受信がキャンセルされたか
    uint8_t txCancelled  : 1;  // 送信がキャンセルされたか
    uint8_t reserved     : 1;  // 予約
} UART_RegInt_StateFlags;
```

---

## 動作フロー

### 受信処理フロー

```
[アプリケーション]
    |
    | UART_RegInt_read(buf, size)
    |
    v
[UART_RegInt.c]
    |
    ├─ トランザクション設定
    |   - rxBuf = buf
    |   - rxSize = size
    |   - rxCount = 0
    |
    ├─ 割り込み有効化
    |   - UART_INT_RX
    |   - UART_INT_RT (Receive Timeout)
    |   - UART_INT_ALL_ERRORS
    |
    ├─ BLOCKINGモード?
    |   YES → セマフォ待機
    |   NO  → 即座にリターン
    |
    v
[割り込みコンテキスト]
    |
    | INT_UART0_COMB 発生
    |
    v
[UART_RegInt_hwiIntFxn()]
    |
    ├─ 割り込みステータス取得
    |   - UARTGetIntStatus()
    |
    ├─ RX割り込み?
    |   YES → processRxInterrupt()
    |       |
    |       ├─ FIFO空まで繰り返し
    |       |   - エラーチェック (RSR/ECR)
    |       |   - データ読み取り (DR)
    |       |   - rxBuf[rxCount++] = data
    |       |
    |       ├─ rxCount >= rxSize?
    |       |   YES → 受信完了
    |       |
    |       ├─ 割り込み無効化
    |       |
    |       └─ コールバック or セマフォ通知
    |
    └─ エラー割り込み?
        YES → processErrorInterrupt()
            - オーバーラン検出
            - フレーミングエラー検出
            - イベントコールバック呼び出し
```

### 送信処理フロー

```
[アプリケーション]
    |
    | UART_RegInt_write(buf, size)
    |
    v
[UART_RegInt.c]
    |
    ├─ トランザクション設定
    |   - txBuf = buf
    |   - txSize = size
    |   - txCount = 0
    |
    ├─ FIFOに書き込み (空きがある限り)
    |   while (txCount < txSize && !FIFO_FULL)
    |       HWREG(DR) = txBuf[txCount++]
    |
    ├─ 全データ書き込み完了?
    |   YES → 送信完了 (割り込み不要)
    |   NO  → TX割り込み有効化
    |
    ├─ BLOCKINGモード?
    |   YES → セマフォ待機
    |   NO  → 即座にリターン
    |
    v
[割り込みコンテキスト] (残りデータがある場合)
    |
    | INT_UART0_COMB 発生
    |
    v
[UART_RegInt_hwiIntFxn()]
    |
    | processTxInterrupt()
    |
    ├─ FIFOに書き込み (空きがある限り)
    |   while (txCount < txSize && !FIFO_FULL)
    |       HWREG(DR) = txBuf[txCount++]
    |
    ├─ txCount >= txSize?
    |   YES → 送信完了
    |
    ├─ 割り込み無効化
    |
    └─ コールバック or セマフォ通知
```

---

## レジスタアクセス

### 使用するUARTレジスタ

| レジスタ | オフセット | 用途 |
|----------|-----------|------|
| DR (Data Register) | 0x000 | データ送受信 |
| RSR/ECR (Receive Status/Error Clear) | 0x004 | エラーステータス |
| FR (Flag Register) | 0x018 | FIFOフラグ |
| IBRD (Integer Baud Rate Divisor) | 0x024 | ボーレート整数部 |
| FBRD (Fractional Baud Rate Divisor) | 0x028 | ボーレート小数部 |
| LCRH (Line Control Register) | 0x02C | データフォーマット |
| CTL (Control Register) | 0x030 | UART制御 |
| IFLS (Interrupt FIFO Level Select) | 0x034 | FIFO割り込みレベル |
| IMSC (Interrupt Mask Set/Clear) | 0x038 | 割り込みマスク |
| RIS (Raw Interrupt Status) | 0x03C | 生割り込みステータス |
| MIS (Masked Interrupt Status) | 0x040 | マスク後割り込みステータス |
| ICR (Interrupt Clear) | 0x044 | 割り込みクリア |

### レジスタアクセスマクロ

```c
// 汎用レジスタアクセス
#define UART_REGINT_HWREG(x) (*((volatile uint32_t *)(x)))

// データレジスタ
#define UART_REGINT_READ_DATA(baseAddr) \
    ((uint8_t)UART_REGINT_HWREG((baseAddr) + UART_O_DR))

#define UART_REGINT_WRITE_DATA(baseAddr, data) \
    UART_REGINT_HWREG((baseAddr) + UART_O_DR) = ((uint32_t)(data))

// フラグレジスタ
#define UART_REGINT_RX_FIFO_EMPTY(baseAddr) \
    (UART_REGINT_HWREG((baseAddr) + UART_O_FR) & UART_FR_RXFE)

#define UART_REGINT_TX_FIFO_FULL(baseAddr) \
    (UART_REGINT_HWREG((baseAddr) + UART_O_FR) & UART_FR_TXFF)
```

---

## 割り込み処理

### 割り込みソース

CC2340R53のUART0は統合割り込み (`INT_UART0_COMB = 27`) を使用:

```c
割り込みビット:
- UART_INT_RX    (0x10) - 受信FIFO閾値到達
- UART_INT_TX    (0x20) - 送信FIFO閾値到達
- UART_INT_RT    (0x40) - 受信タイムアウト
- UART_INT_OE    (0x400) - オーバーランエラー
- UART_INT_BE    (0x200) - ブレークエラー
- UART_INT_PE    (0x100) - パリティエラー
- UART_INT_FE    (0x80)  - フレーミングエラー
```

### 割り込みハンドラ構造

```c
void UART_RegInt_hwiIntFxn(uintptr_t arg)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)arg;
    uint32_t intStatus;
    
    // 1. 割り込みステータス取得
    intStatus = UARTGetIntStatus(baseAddr, true);
    
    // 2. 受信割り込み処理
    if (intStatus & (UART_INT_RX | UART_INT_RT)) {
        UARTClearInt(baseAddr, UART_INT_RX | UART_INT_RT);
        processRxInterrupt(handle);
    }
    
    // 3. 送信割り込み処理
    if (intStatus & UART_INT_TX) {
        UARTClearInt(baseAddr, UART_INT_TX);
        processTxInterrupt(handle);
    }
    
    // 4. エラー割り込み処理
    if (intStatus & UART_INT_ALL_ERRORS) {
        UARTClearInt(baseAddr, UART_INT_ALL_ERRORS);
        processErrorInterrupt(handle, intStatus);
    }
}
```

### 割り込み優先度

デフォルト優先度: `(5 << 5)`

CC23X0は3ビット優先度 (0-7):
- 0: ゼロレイテンシ割り込み (本ドライバ非対応)
- 1-3: 高優先度
- 4-6: 中優先度
- 7: 低優先度

---

## エラー処理

### エラー検出

RSR/ECRレジスタから以下のエラーを検出:

```c
#define UART_REGINT_ERR_OVERRUN  (0x08)  // Overrun Error
#define UART_REGINT_ERR_BREAK    (0x04)  // Break Error
#define UART_REGINT_ERR_PARITY   (0x02)  // Parity Error
#define UART_REGINT_ERR_FRAMING  (0x01)  // Framing Error
```

### エラー優先度

複数エラー同時発生時の優先順位:
1. Overrun (最優先)
2. Break
3. Parity
4. Framing

### エラー通知

2つの通知方法:

1. **ステータスコード返却**:
   - 受信処理完了時にエラーコードを返す
   - BLOCKING: 関数戻り値
   - CALLBACK: コールバックのstatus引数

2. **イベントコールバック**:
   - エラー発生時に即座に通知
   - イベントマスクでフィルタリング可能
   - オーバーランカウントを記録

---

## 電源管理

### 電源依存関係

```c
// UART使用時
Power_setDependency(PowerLPF3_PERIPH_UART0);

// UART未使用時
Power_releaseDependency(PowerLPF3_PERIPH_UART0);
```

### スタンバイ制約

受信有効時はスタンバイ禁止:

```c
// 受信有効化時
Power_setConstraint(PowerLPF3_DISALLOW_STANDBY);

// 受信無効化時
Power_releaseConstraint(PowerLPF3_DISALLOW_STANDBY);
```

### 電源通知ハンドラ

```c
// スタンバイ前: ピン設定保存 (必要に応じて実装)
int UART_RegInt_preNotifyFxn(unsigned int eventType, 
                             uintptr_t eventArg,
                             uintptr_t clientArg);

// スタンバイ後: ハードウェア再初期化
int UART_RegInt_postNotifyFxn(unsigned int eventType,
                              uintptr_t eventArg,
                              uintptr_t clientArg);
```

---

## パフォーマンス考察

### LIN通信での性能

**ボーレート**: 9600 bps  
**1バイト送信時間**: 1.04 ms (10ビット: 1スタート + 8データ + 1ストップ)  
**8バイトフレーム**: 8.3 ms

**FIFO深度**: 8バイト

**割り込み頻度**:
- RX: FIFO 6/8 到達時 → 約6.2ms間隔
- TX: FIFO 2/8 到達時 → 約2.1ms間隔

**割り込み処理時間**: ~10μs (FIFOから1バイト読み取り)

**結論**: 割り込み処理時間はフレーム時間の0.12%程度であり、性能上の問題なし。

### 19200 bpsの場合

**1バイト送信時間**: 0.52 ms  
**8バイトフレーム**: 4.16 ms  
**割り込み頻度**: ~3.1ms (RX), ~1.0ms (TX)

依然として問題なし。

---

## メモリ使用量

### オブジェクトサイズ

```c
sizeof(UART_RegInt_Object_) ≈ 100-120 bytes
  - Params: 48 bytes
  - State flags: 1 byte
  - HwiP_Struct: ~20 bytes
  - SemaphoreP_Struct × 2: ~40 bytes
  - Transaction data: ~30 bytes
  - Power notify: ~16 bytes
```

### コードサイズ

推定値:
- UART_RegInt.c: ~3-4 KB
- UART_RegInt.h: ヘッダーのみ
- UART_RegInt_Priv.h: インライン関数含む
- UART_RegInt_Config.h: 定数定義のみ

**合計**: 約4-5 KB (TI UART2の約50-60%削減)

---

## テスト戦略

### 単体テスト

1. **初期化テスト**:
   - open/close の正常動作
   - パラメータバリデーション
   - 重複オープン検出

2. **送信テスト**:
   - BLOCKING送信
   - CALLBACK送信
   - タイムアウト送信
   - キャンセル

3. **受信テスト**:
   - BLOCKING受信
   - CALLBACK受信
   - タイムアウト受信
   - 部分受信モード

4. **エラーテスト**:
   - オーバーラン検出
   - フレーミングエラー検出
   - パリティエラー検出

### 統合テスト

1. **LINソフト統合**:
   - LIN初期化シーケンス
   - LINフレーム送信
   - LINフレーム受信
   - エラーハンドリング

2. **電源管理テスト**:
   - スタンバイ移行/復帰
   - 受信有効時のスタンバイ禁止

3. **ストレステスト**:
   - 連続送受信
   - 高頻度read/write
   - 長時間動作

---

## 今後の拡張

### オプション機能

1. **フロー制御**: CTS/RTSサポート (LINでは不要)
2. **リングバッファ**: 連続受信バッファリング
3. **POLLINGモード**: 割り込みなしのポーリング送受信
4. **9ビットモード**: アドレスビット対応

### パフォーマンス最適化

1. **FIFOトリガレベル調整**: 動的変更
2. **バースト転送最適化**: 連続アクセス高速化
3. **キャッシュ活用**: データキャッシュ利用

---

## 既知の制限事項

1. **DMA非対応**: 高速大容量転送には不向き (LINでは問題なし)
2. **フロー制御なし**: 現在未実装 (LINでは不要)
3. **単一インスタンス**: UART0のみ対応
4. **同時トランザクション**: 送信/受信それぞれ1つまで

---

## 参照ドキュメント

- **CC2340R53 Technical Reference Manual**: UARTレジスタ詳細
- **CC23X0 SimpleLink SDK**: TI DriverLib API
- **LIN Specification 2.2A**: LINプロトコル仕様
- **TI UART2 Driver**: API互換性参照

---

## 変更履歴

| バージョン | 日付 | 変更内容 |
|-----------|------|---------|
| 1.0 | 2025-10-26 | 初版作成 |

---

**作成者**: Claude (Anthropic)  
**対象デバイス**: Texas Instruments CC2340R53 (CC23X0R5)  
**ドライババージョン**: 1.0
