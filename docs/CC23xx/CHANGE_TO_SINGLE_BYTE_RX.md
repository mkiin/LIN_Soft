# CC23xx LINソフト: 複数バイト受信から1バイト受信への変更リスト

## 概要

現在のCC23xxフォルダのLINソフトは、**複数バイト一括受信方式**を採用しています。
これをH850と同様の**1バイト単位受信方式**に変更するための変更点と問題点をリスト化します。

---

## 現状の実装方式（複数バイト受信）

### アーキテクチャ

```
[UART2ドライバ]
    ↓
複数バイト受信（ヘッダ: 3バイト、データ: 最大9バイト）
    ↓
受信完了コールバック（l_ifc_rx_ch1）
    ↓
受信バッファから一括処理（l_vog_lin_rx_int）
    ↓
状態に応じた処理
```

### 受信フロー

#### 1. ヘッダ受信（3バイト一括）

**`l_slin_core_CC2340R53.c`** - `l_vog_lin_check_header()`

```c
// Synch Break (0x00) + Synch Field (0x55) + Protected ID (0xXX)
// → 3バイトを一括受信
l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE, 3);  // 3バイト受信設定

// 受信完了後
l_vog_lin_rx_int(u1a_lin_data[], u1a_lin_err)
    ↓
l_vog_lin_check_header(u1a_lin_data[], u1a_lin_err)
    ↓
u1a_lin_data[0] = Synch Break (0x00)
u1a_lin_data[1] = Synch Field (0x55)
u1a_lin_data[2] = Protected ID (0xXX)
```

#### 2. データ受信（データ + チェックサム 一括）

```c
// データサイズ + チェックサム(1バイト) を一括受信
l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE, u1l_lin_frm_sz + 1);

// 受信完了後
l_vog_lin_rx_int(u1a_lin_data[], u1a_lin_err)
    ↓
u1a_lin_data[0〜7] = データ
u1a_lin_data[u1l_lin_frm_sz] = チェックサム
    ↓
チェックサム検証
    ↓
フレームバッファへコピー
```

### 主要な関数

| 関数 | ファイル | 役割 |
|------|---------|------|
| `l_ifc_rx_ch1()` | l_slin_drv_cc2340r53.c | UART受信完了コールバック（複数バイト） |
| `l_vog_lin_rx_int()` | l_slin_core_CC2340R53.c | 受信割り込み処理（配列で受け取る） |
| `l_vog_lin_check_header()` | l_slin_core_CC2340R53.c | ヘッダ3バイト検証 |
| `l_vog_lin_rx_enb()` | l_slin_drv_cc2340r53.c | UART受信開始（サイズ指定） |

---

## 1バイト受信方式への変更リスト

### 変更箇所1: DRV層（l_slin_drv_cc2340r53.c）

#### 1-1. 関数シグネチャ変更

**変更前:**
```c
void l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data,
                  size_t count, void *userArg, int_fast16_t u1a_lin_rx_status)
{
    l_u8 u1a_lin_rx_set_err;
    u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;  // エラー検出未実装
    l_vog_lin_rx_int((l_u8 *)u1a_lin_rx_data, u1a_lin_rx_set_err);
}
```

**変更後:**
```c
void l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data,
                  size_t count, void *userArg, int_fast16_t u1a_lin_rx_status)
{
    l_u8 u1a_lin_rx_set_err;
    l_u8 u1a_lin_single_byte;

    // 1バイトのみ取得
    u1a_lin_single_byte = ((l_u8 *)u1a_lin_rx_data)[0];

    // エラー情報生成（UART2_Statusから）
    u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;
    if (u1a_lin_rx_status & UART_RXERROR_FRAMING) {
        u1a_lin_rx_set_err |= U1G_LIN_FRAMING_ERR_ON;
    }
    if (u1a_lin_rx_status & UART_RXERROR_OVERRUN) {
        u1a_lin_rx_set_err |= U1G_LIN_OVERRUN_ERR;
    }
    if (u1a_lin_rx_status & UART_RXERROR_PARITY) {
        u1a_lin_rx_set_err |= U1G_LIN_PARITY_ERR;
    }

    // 1バイト単位でCORE層へ渡す
    l_vog_lin_rx_int(u1a_lin_single_byte, u1a_lin_rx_set_err);
}
```

#### 1-2. UART受信開始関数の変更

**変更前:**
```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
{
    l_s16 s2a_lin_ret;
    l_u1g_lin_irq_dis();

    if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE) {
        UART_RegInt_flushRx(xnl_lin_uart_handle);
    }

    UART_RegInt_rxEnable(xnl_lin_uart_handle);

    // 指定サイズで受信開始
    s2a_lin_ret = UART_RegInt_read(xnl_lin_uart_handle,
                                    &u1l_lin_rx_buf,
                                    u1a_lin_rx_data_size);  // ← サイズ指定

    if (UART2_STATUS_SUCCESS != s2a_lin_ret) {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;
    }

    l_vog_lin_irq_res();
}
```

**変更後:**
```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx)
{
    l_s16 s2a_lin_ret;
    l_u1g_lin_irq_dis();

    if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE) {
        UART_RegInt_flushRx(xnl_lin_uart_handle);
    }

    UART_RegInt_rxEnable(xnl_lin_uart_handle);

    // 常に1バイト受信
    s2a_lin_ret = UART_RegInt_read(xnl_lin_uart_handle,
                                    &u1l_lin_rx_buf,
                                    1);  // ← 固定1バイト

    if (UART2_STATUS_SUCCESS != s2a_lin_ret) {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;
    }

    l_vog_lin_irq_res();
}
```

**ヘッダファイルの変更:**
```c
// l_slin_drv_cc2340r53.h

// 変更前
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size);

// 変更後
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx);
```

### 変更箇所2: CORE層（l_slin_core_CC2340R53.c）

#### 2-1. l_vog_lin_rx_int() の引数変更

**変更前:**
```c
void l_vog_lin_rx_int(l_u8 u1a_lin_data[], l_u8 u1a_lin_err)
{
    // 配列として受け取る
    l_vog_lin_check_header(u1a_lin_data, u1a_lin_err);
}
```

**変更後:**
```c
void l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err)
{
    // 1バイトとして受け取る
    // H850の実装と同様の状態遷移処理

    if (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP) {
        // WAKEUPコマンド受信
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;
        l_ifc_init_drv_ch1();
    }
    else if (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY) {
        // Synch Break待ち
        if (u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT) {
            // Synch Break検出処理
            // ...
        }
    }
    else if (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN) {
        switch (u1l_lin_slv_sts) {
        case U1G_LIN_SLSTS_BREAK_UART_WAIT:
            // Synch Break検出
            break;

        case U1G_LIN_SLSTS_SYNCHFIELD_WAIT:
            // Synch Field (0x55) 検出
            break;

        case U1G_LIN_SLSTS_IDENTFIELD_WAIT:
            // Protected ID 受信
            break;

        case U1G_LIN_SLSTS_RCVDATA_WAIT:
            // データ1バイト受信
            u1l_lin_rs_tmp[u1l_lin_rs_cnt] = u1a_lin_data;
            u1l_lin_rs_cnt++;

            if (u1l_lin_rs_cnt < u1l_lin_frm_sz) {
                // 次のバイト受信準備
                l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NOUSE);
            } else {
                // チェックサム受信準備
                // ...
            }
            break;

        case U1G_LIN_SLSTS_SNDDATA_WAIT:
            // 送信データのリードバック
            break;
        }
    }
}
```

#### 2-2. l_vog_lin_check_header() の削除または変更

**現状:**
```c
l_u8 l_vog_lin_check_header(l_u8 u1a_lin_data[], l_u8 u1a_lin_err)
{
    // 3バイト一括でヘッダチェック
    u1a_lin_data[0] = Synch Break
    u1a_lin_data[1] = Synch Field
    u1a_lin_data[2] = Protected ID
}
```

**変更後:**
- この関数は**不要**（1バイト単位で状態遷移するため）
- または、各状態での1バイト処理に分割

#### 2-3. 状態遷移の追加

**追加が必要な状態:**

```c
// l_slin_core_CC2340R53.h

#define U1G_LIN_SLSTS_BREAK_UART_WAIT   ((l_u8)0)  // 既存
#define U1G_LIN_SLSTS_SYNCHFIELD_WAIT   ((l_u8)2)  // 追加
#define U1G_LIN_SLSTS_IDENTFIELD_WAIT   ((l_u8)3)  // 追加
#define U1G_LIN_SLSTS_RCVDATA_WAIT      ((l_u8)4)  // 既存
#define U1G_LIN_SLSTS_SNDDATA_WAIT      ((l_u8)5)  // 既存
```

**現在は `SYNCHFIELD_WAIT` と `IDENTFIELD_WAIT` が欠落している可能性があります。**

#### 2-4. データ受信処理の変更

**変更前（一括受信）:**
```c
case U1G_LIN_SLSTS_RCVDATA_WAIT:
    // チェックサム計算（全データ受信済み）
    u1l_lin_chksum = l_vog_lin_checksum(pid, u1a_lin_data, u1l_lin_frm_sz, ...);

    // チェックサム検証
    if (u1l_lin_chksum != u1a_lin_data[u1l_lin_frm_sz]) {
        // エラー
    }

    // データコピー（一括）
    xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[0] = u1l_lin_rx_buf[0];
    xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[1] = u1l_lin_rx_buf[1];
    // ...
    break;
```

**変更後（1バイト単位）:**
```c
case U1G_LIN_SLSTS_RCVDATA_WAIT:
    // 1バイト受信
    u1l_lin_rs_tmp[u1l_lin_rs_cnt] = u1a_lin_data;
    u1l_lin_chksum += u1a_lin_data;  // チェックサム累積計算
    u1l_lin_rs_cnt++;

    // 全データ受信完了？
    if (u1l_lin_rs_cnt < u1l_lin_frm_sz) {
        // 次のバイト受信
        l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NOUSE);
    } else {
        // チェックサム受信（次の1バイト）
        l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NOUSE);
        // 状態は RCVDATA_WAIT のまま
        // 次の受信でチェックサム検証を行う
    }
    break;
```

### 変更箇所3: 送信処理の変更

#### 3-1. 送信データ設定の変更

**変更前（一括送信）:**
```c
void l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    // 複数バイトを一括送信
    UART_RegInt_write(xnl_lin_uart_handle, u1a_lin_data, u1a_lin_data_size);
}
```

**変更後（1バイト単位送信）:**
```c
void l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    // 1バイト送信
    UART_RegInt_write(xnl_lin_uart_handle, &u1a_lin_data, 1);
}
```

#### 3-2. 送信完了処理の変更

**追加:**
```c
case U1G_LIN_SLSTS_SNDDATA_WAIT:
    // 送信データのリードバック確認
    if (l_u1g_lin_read_back(u1a_lin_data) == U1G_LIN_BIT_SET) {
        // ビットエラー
        // ...
    }

    u1l_lin_rs_cnt++;

    if (u1l_lin_rs_cnt < u1l_lin_frm_sz) {
        // 次のバイト送信
        l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);
    } else {
        // チェックサム送信
        l_vog_lin_tx_char(u1l_lin_chksum);
    }
    break;
```

### 変更箇所4: バッファサイズの変更

**l_slin_drv_cc2340r53.c:**

```c
// 変更前
l_u8 u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];  // 例: 9バイト
l_u8 u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE]; // 例: 9バイト

// 変更後
l_u8 u1l_lin_rx_buf[1];  // 1バイト固定
l_u8 u1l_lin_tx_buf[1];  // 1バイト固定
```

---

## 追加修正が必要な問題点リスト

### 問題点1: エラー検出の実装

**現状:**
```c
u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;  // エラー検出未実装
```

**修正:**
- UART2_Status から Framing Error, Overrun Error, Parity Error を取得
- H850と同様のエラーフラグに変換

**優先度: 高**

### 問題点2: チェックサム累積計算の実装

**現状:**
```c
// 一括受信後に計算
u1l_lin_chksum = l_vog_lin_checksum(pid, u1a_lin_data, size, type);
```

**修正:**
```c
// 1バイト受信毎に累積
static l_u16 u2l_lin_chksum_acc;  // 累積用変数追加

case RCVDATA_WAIT:
    u1l_lin_rs_tmp[u1l_lin_rs_cnt] = u1a_lin_data;
    u2l_lin_chksum_acc += u1a_lin_data;  // 累積
    u1l_lin_rs_cnt++;
    // ...
```

**優先度: 高**

### 問題点3: タイミング調整

**問題:**
- 複数バイト受信は受信完了まで待つが、1バイト受信は毎回割り込みが発生
- 割り込み頻度が増加（最大10倍）

**影響:**
- CPU負荷の増加
- リアルタイム性への影響

**対策:**
- 割り込み処理の高速化
- タイマ割り込みの優先度調整
- FIFOの活用（可能であれば）

**優先度: 中**

### 問題点4: UART受信再設定のタイミング

**問題:**
```c
// 現状: 複数バイト受信後、自動的に停止
// 変更後: 1バイト受信毎に再設定が必要

case RCVDATA_WAIT:
    u1l_lin_rs_tmp[u1l_lin_rs_cnt] = u1a_lin_data;
    u1l_lin_rs_cnt++;

    // 次のバイト受信設定が必要
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NOUSE);  // ← 追加
```

**影響:**
- 受信設定のタイミングがずれると、次のバイトを取りこぼす可能性

**対策:**
- 割り込みハンドラ内で即座に次の受信設定
- UART FIFOの活用（CC2340R53は4バイトFIFOあり）

**優先度: 高**

### 問題点5: リードバック処理の変更

**現状:**
```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data[], l_u8 u1a_lin_data_size)
{
    // 複数バイトのリードバック確認
    for (i = 0; i < u1a_lin_data_size; i++) {
        if (送信データ != 受信データ) {
            return U1G_LIN_BIT_SET;  // エラー
        }
    }
}
```

**修正:**
```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    // 1バイトのリードバック確認
    if (送信データ != u1a_lin_data) {
        return U1G_LIN_BIT_SET;  // エラー
    }
    return U1G_LIN_BIT_CLR;  // OK
}
```

**優先度: 中**

### 問題点6: BREAK_IRQ_WAIT 状態の処理

**H850の問題点を踏襲しないように注意:**

```c
// Synch Break検出時の状態遷移

#if U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
    // INT方式: IRQ待ち
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
    l_vog_lin_rx_dis();
#elif U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
    // UART方式: 直接Synch Field待ちへ
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NOUSE);  // 継続
#endif
```

**優先度: 高（H850の問題修正を反映）**

### 問題点7: UART2コールバックモードの制約

**問題:**
```c
xna_lin_uart_params.readMode = UART2_Mode_CALLBACK;
xna_lin_uart_params.readReturnMode = UART2_ReadReturnMode_FULL;
```

- `UART2_ReadReturnMode_FULL` は指定バイト数受信完了までコールバックしない
- 1バイト受信にすると、毎回コールバックが発生

**対策案1: UART2_ReadReturnMode_PARTIAL に変更**
```c
xna_lin_uart_params.readReturnMode = UART2_ReadReturnMode_PARTIAL;
```
- 1バイトでもコールバック発生（望ましい）

**対策案2: カスタムUARTドライバ（UART_RegInt）の活用**
```c
#define ORIGINAL_UART_DRIVER (0)  // カスタムドライバ使用
```

**優先度: 高**

### 問題点8: 受信バッファのクリア処理

**問題:**
```c
if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE) {
    UART_RegInt_flushRx(xnl_lin_uart_handle);
}
```

- 1バイト受信の場合、フラッシュのタイミングが重要
- フラッシュしすぎると、有効なデータも削除される

**対策:**
- Synch Break検出時のみフラッシュ
- それ以外はフラッシュしない

**優先度: 中**

### 問題点9: タイマタイムアウト値の調整

**問題:**
```c
// ヘッダタイムアウト: 現在は3バイト受信を想定
// → 1バイト単位に変更すると、タイムアウト値の見直しが必要
```

**対策:**
- H850と同様の49カウント（約2.4ms @ 9600bps）を使用
- ビット単位のタイマ設定

**優先度: 中**

### 問題点10: デバッグの複雑化

**問題:**
- 複数バイト受信: 1回の割り込みで確認
- 1バイト受信: 10回程度の割り込みを追跡

**対策:**
- ログ出力の整備
- 状態遷移のトレース機能
- ロジックアナライザでの確認

**優先度: 低**

---

## 変更作業の優先順位

### フェーズ1: DRV層の変更（必須）

1. **l_ifc_rx_ch1()** の引数処理変更
2. **l_vog_lin_rx_enb()** の引数とサイズ変更
3. **エラー検出の実装**
4. **l_vog_lin_tx_char()** の変更

### フェーズ2: CORE層の状態遷移実装（必須）

1. **l_vog_lin_rx_int()** の1バイト対応
2. **SYNCHFIELD_WAIT** 状態の追加
3. **IDENTFIELD_WAIT** 状態の追加
4. **RCVDATA_WAIT** の1バイト処理化
5. **SNDDATA_WAIT** の1バイト処理化

### フェーズ3: H850問題点の修正反映（推奨）

1. **BREAK_IRQ_WAIT** 状態の条件分岐
2. **チェックサム累積計算** の実装

### フェーズ4: 最適化（オプション）

1. **タイミング調整**
2. **割り込み優先度調整**
3. **デバッグ機能の追加**

---

## テスト項目

### 基本動作テスト

- [ ] Synch Break検出
- [ ] Synch Field (0x55) 検出
- [ ] Protected ID 受信
- [ ] データ受信（1〜8バイト）
- [ ] チェックサム検証
- [ ] データ送信
- [ ] リードバック確認

### エラー処理テスト

- [ ] Framing Error 検出
- [ ] Overrun Error 検出
- [ ] チェックサムエラー検出
- [ ] ヘッダタイムアウトエラー
- [ ] レスポンスタイムアウトエラー

### スリープ/Wake-upテスト

- [ ] スリープ移行
- [ ] Wake-up信号検出
- [ ] Wake-up後の通常通信

### 連続フレームテスト

- [ ] 複数フレームの連続受信
- [ ] 送受信の混在

### 性能テスト

- [ ] 割り込み頻度の測定
- [ ] CPU負荷の測定
- [ ] 通信速度の確認

---

## 移行戦略

### 段階的移行アプローチ（推奨）

#### ステップ1: デバッグ機能追加
```c
#define LIN_RX_DEBUG (1)

#if LIN_RX_DEBUG
void lin_rx_log(l_u8 state, l_u8 data) {
    printf("State:%d Data:0x%02X\n", state, data);
}
#endif
```

#### ステップ2: 並行実装
```c
#define LIN_RX_MODE_SINGLE_BYTE (1)  // 0: 複数バイト, 1: 1バイト

#if LIN_RX_MODE_SINGLE_BYTE
    // 新実装
#else
    // 既存実装
#endif
```

#### ステップ3: 検証
- 既存実装と新実装の比較
- 受信データの一致確認

#### ステップ4: 完全移行
- デバッグコードの削除
- 最適化

---

## 見積もり

### 作業時間

| 項目 | 工数 |
|------|------|
| DRV層変更 | 8時間 |
| CORE層変更 | 16時間 |
| エラー処理実装 | 8時間 |
| テスト | 16時間 |
| デバッグ・修正 | 16時間 |
| **合計** | **64時間（約8人日）** |

### リスク

| リスク | 影響 | 対策 |
|--------|------|------|
| 割り込み頻度増加によるCPU負荷 | 高 | FIFOの活用、優先度調整 |
| タイミング問題によるデータ取りこぼし | 高 | 受信設定の最適化 |
| デバッグの複雑化 | 中 | ログ機能の充実 |
| 予期しない動作の発生 | 中 | 段階的移行 |

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: CC23xx to Single-Byte RX Migration Tool
