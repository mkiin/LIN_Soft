# LINレスポンス送信処理の3プラットフォーム比較分析

## 概要

本ドキュメントは、F24（LIN 2.x、ハードウェアLINモジュール）、H850（LIN 1.3、UART実装）、CC23xx（UART実装）の3つのプラットフォームにおけるレスポンス送信処理を詳細に比較分析し、CC23xxの問題点と修正対応を明確化するものです。

---

## 1. 送信処理アーキテクチャ比較表

### 1-1. 全体比較

| 項目 | F24 (LIN 2.x) | H850 (LIN 1.3) | CC23xx (現状) | CC23xx (正しい実装) |
|------|--------------|---------------|--------------|-------------------|
| **ハードウェア** | 専用LINモジュール | UART | UART | UART |
| **送信トリガー** | IDフィールド受信直後 | レスポンススペース後 | IDフィールド受信直後 | レスポンススペース後 |
| **送信方法** | 8バイト一括（HW自動） | 1バイトずつ（SW制御） | 全バイト一括（誤り） | 1バイトずつ（SW制御） |
| **レスポンススペース** | HWが自動挿入 | タイマー割り込みで実装 | **未実装** | タイマー割り込みで実装 |
| **インターバイトスペース** | HWが自動挿入 | タイマー割り込みで実装 | **未実装** | タイマー割り込みで実装 |
| **タイマー制御** | 不要（HW自動） | SW制御（ビット単位） | **未実装** | SW制御（ビット単位） |
| **リードバック** | 不要（HW自動） | 1バイトずつ | 全バイト一括（誤り） | 1バイトずつ |
| **送信完了割り込み** | HW割り込み | タイマー割り込み | 受信割り込み（誤り） | タイマー割り込み |

### 1-2. 送信処理の詳細比較

#### F24 (LIN 2.x) - ハードウェア自動送信

**特徴:**
- RL78/F24の専用LINモジュールを使用
- レスポンス送信は完全にハードウェア制御

**送信処理の流れ:**

```
[IDフィールド受信割り込み] l_vog_lin_rx_int
    ↓
┌─────────────────────────────────────────────┐
│ レスポンス送信開始                           │
│ l_u1g_lin_resp_snd_start(size, sum_type, data) │
│ [l_slin_drv_rl78f24.c:643-650]              │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ LINデータバッファレジスタに全データ設定       │
│ LDB1 = data[0]                               │
│ LDB2 = data[1]                               │
│ ...                                          │
│ LDB8 = data[7]                               │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ レスポンス送信開始レジスタ設定                │
│ LTRC = RTS (Response Transmission Start)     │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ ★ハードウェアが自動実行★                     │
│ - レスポンススペース挿入                      │
│ - データ送信（1バイトずつ）                   │
│ - インターバイトスペース挿入                  │
│ - チェックサム計算・送信                      │
└─────────────────────────────────────────────┘
    ↓
[送信完了割り込み] l_vog_lin_tx_int
    ↓
┌─────────────────────────────────────────────┐
│ 送信完了フラグ設定                           │
│ l_vol_lin_set_frm_complete(ERR_OFF)         │
└─────────────────────────────────────────────┘
```

**コード:**
```c
// [F24/l_slin_core_rl78f24.c:1036-1038]
u1a_lin_result = l_u1g_lin_resp_snd_start(
    xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_frm_sz,
    xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_sum_type,
    xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte
);

// [F24/l_slin_drv_rl78f24.c:643-652]
U1G_LIN_SFR_LDB1 = pta_lin_snd_data[U1G_LIN_0];
U1G_LIN_SFR_LDB2 = pta_lin_snd_data[U1G_LIN_1];
// ... (LDB3-LDB8)
U1G_LIN_SFR_LTRC = U1G_LIN_LTRC_RTS;  // ★ハードウェア送信開始★
```

#### H850 (LIN 1.3) - ソフトウェア制御送信（正しい実装）

**特徴:**
- UARTを使用したソフトウェア制御
- タイマー割り込み駆動で1バイトずつ送信

**送信処理の流れ:**

```
[IDフィールド受信割り込み] l_vog_lin_rx_int
    ↓
┌─────────────────────────────────────────────┐
│ データを送信用tmpバッファにコピー              │
│ u1l_lin_rs_tmp[0..7] = frm_buf[0..7]        │
│ [l_slin_core_h83687.c:926-942]              │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ レスポンススペース待ちタイマー設定             │
│ l_vog_lin_bit_tm_set(U1G_LIN_RSSP)          │
│ [l_slin_core_h83687.c:944]                  │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ 状態遷移                                     │
│ u1l_lin_slv_sts = SNDDATA_WAIT              │
│ [l_slin_core_h83687.c:946]                  │
└─────────────────────────────────────────────┘
    ↓
[タイマー割り込み1回目] l_vog_lin_tm_int (レスポンススペース後)
    ↓
┌─────────────────────────────────────────────┐
│ case SNDDATA_WAIT:                           │
│ [l_slin_core_h83687.c:1162-1219]            │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ 1バイト目送信 (u1l_lin_rs_cnt = 0)          │
│ - チェックサム累積計算                        │
│ - l_vog_lin_tx_char(tmp[0]) ★1バイト送信★   │
│ - タイマー設定 (BYTE_LENGTH + BTSP)         │
│ - カウンタインクリメント                      │
│ [l_slin_core_h83687.c:1211-1218]            │
└─────────────────────────────────────────────┘
    ↓
[タイマー割り込み2回目] l_vog_lin_tm_int (インターバイトスペース後)
    ↓
┌─────────────────────────────────────────────┐
│ 2バイト目以降送信 (u1l_lin_rs_cnt > 0)       │
│ [l_slin_core_h83687.c:1163-1208]            │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ 前回送信バイトのリードバックチェック           │
│ l_u1g_lin_read_back(tmp[cnt-1]) ★1バイト★   │
│ [l_slin_core_h83687.c:1165]                 │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ リードバック成功？                           │
└─────────────────────────────────────────────┘
    ↓ OK
┌─────────────────────────────────────────────┐
│ 次のバイト送信                               │
│ - チェックサム累積計算                        │
│ - l_vog_lin_tx_char(tmp[cnt]) ★1バイト送信★ │
│ - タイマー設定 (BYTE_LENGTH + BTSP)         │
│ - カウンタインクリメント                      │
│ [l_slin_core_h83687.c:1186-1206]            │
└─────────────────────────────────────────────┘
    ↓
[タイマー割り込み繰り返し...]
    ↓
┌─────────────────────────────────────────────┐
│ 全バイト送信完了 (cnt > frm_sz)              │
│ l_vol_lin_set_frm_complete(ERR_OFF)         │
│ l_vol_lin_set_synchbreak()                  │
│ [l_slin_core_h83687.c:1177-1181]            │
└─────────────────────────────────────────────┘
```

**コード:**
```c
// [H850/slin_lib/l_slin_core_h83687.c:926-946]
// IDフィールド受信時
u1l_lin_rs_tmp[U1G_LIN_0] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0];
// ... (tmp[1..7])
l_vog_lin_bit_tm_set(U1G_LIN_RSSP);  // レスポンススペース待ち
u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;

// [H850/slin_lib/l_slin_core_h83687.c:1162-1219]
// タイマー割り込み時
case (U1G_LIN_SLSTS_SNDDATA_WAIT):
    if (u1l_lin_rs_cnt > U1G_LIN_0) {
        // 前回送信バイトのリードバック
        l_u1a_lin_read_back = l_u1g_lin_read_back(u1l_lin_rs_tmp[u1l_lin_rs_cnt - U1G_LIN_1]);
        if (l_u1a_lin_read_back == U1G_LIN_OK) {
            // 次のバイト送信
            l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);  // ★1バイト送信★
            l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);
            u1l_lin_rs_cnt++;
        }
    } else {
        // 1バイト目送信
        l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);  // ★1バイト送信★
        l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);
        u1l_lin_rs_cnt++;
    }
    break;
```

#### CC23xx (現状) - **誤った実装**

**問題点:**
- IDフィールド受信直後に全バイト一括送信
- レスポンススペース・インターバイトスペースが未実装
- タイマー割り込み駆動ではない

**現在の送信処理の流れ（誤り）:**

```
[IDフィールド受信割り込み] l_vog_lin_rx_int
    ↓
┌─────────────────────────────────────────────┐
│ データを送信用tmpバッファにコピー              │
│ u1l_lin_rs_tmp[0..7] = frm_buf[0..7]        │
│ [l_slin_core_CC2340R53.c:1137-1152]         │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ チェックサム計算                             │
│ u1l_lin_rs_tmp[sz] = checksum               │
│ [l_slin_core_CC2340R53.c:1156]              │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ ★問題★ 全バイト一括送信                     │
│ l_vog_lin_tx_char(tmp, sz+1) ← 配列を送信   │
│ [l_slin_core_CC2340R53.c:1161]              │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ 状態遷移                                     │
│ u1l_lin_slv_sts = AFTER_SNDDATA_WAIT        │
│ [l_slin_core_CC2340R53.c:1163]              │
└─────────────────────────────────────────────┘
    ↓
[受信割り込み] l_vog_lin_rx_int (送信完了後の受信)
    ↓
┌─────────────────────────────────────────────┐
│ case AFTER_SNDDATA_WAIT:                     │
│ [l_slin_core_CC2340R53.c:822-841]           │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ ★問題★ 全バイトまとめてリードバック          │
│ l_u1g_lin_read_back(tmp, sz+1) ← 配列比較   │
│ [l_slin_core_CC2340R53.c:826]               │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ リードバック成功？                           │
│ l_vol_lin_set_frm_complete(ERR_OFF/ON)      │
│ [l_slin_core_CC2340R53.c:832/838]           │
└─────────────────────────────────────────────┘
```

**問題のあるコード:**
```c
// [CC23xx/l_slin_core_CC2340R53.c:1161]
// ★問題★ 全バイト一括送信（レスポンススペースなし）
l_vog_lin_tx_char(u1l_lin_rs_tmp, u1l_lin_frm_sz + U1G_LIN_1);

// [CC23xx/l_slin_core_CC2340R53.c:826]
// ★問題★ 全バイトまとめてリードバック
l_u1a_lin_read_back = l_u1g_lin_read_back(u1l_lin_rs_tmp, u1l_lin_frm_sz + U1G_LIN_1);
```

---

## 2. CC23xxの問題点詳細

### 2-1. レスポンススペースが実装されていない

**問題:**
- LIN仕様では、IDフィールド受信後、レスポンススペース（Response Space）を待ってからデータ送信を開始する必要がある
- CC23xxではIDフィールド受信直後に送信開始している

**影響:**
- LIN仕様違反
- マスターが次のフレームを受信する準備ができていない可能性がある

**レスポンススペースの定義:**
```c
// [H850/slin_lib/l_slin_def.h]
#define U1G_LIN_RSSP  ((l_u8)1)  // レスポンススペース（1ビット長）
```

**正しい実装（H850）:**
```c
// IDフィールド受信後、レスポンススペース待ちタイマー設定
l_vog_lin_bit_tm_set(U1G_LIN_RSSP);  // 1ビット長待機
u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;
```

### 2-2. インターバイトスペースが実装されていない

**問題:**
- LIN仕様では、データバイト間にインターバイトスペース（Inter-Byte Space）を挿入する必要がある
- CC23xxでは全バイトを連続して送信している

**影響:**
- LIN仕様違反
- 受信側がバイト境界を正しく認識できない可能性がある

**インターバイトスペースの定義:**
```c
// [H850/slin_lib/l_slin_def.h]
#define U1G_LIN_BTSP  ((l_u8)1)  // インターバイトスペース（1ビット長）
```

**正しい実装（H850）:**
```c
// 各バイト送信後、インターバイトスペース待ちタイマー設定
l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);  // 1バイト送信
l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);  // 10ビット + 1ビット待機
```

### 2-3. タイマー割り込み駆動ではない

**問題:**
- H850では送信処理がタイマー割り込み駆動で実装されている
- CC23xxでは受信割り込み駆動になっている

**影響:**
- 送信タイミングの制御ができない
- リードバックのタイミングが不正確

**正しい実装:**
```
タイマー割り込み → 1バイト送信 → タイマー設定 → タイマー割り込み → ...
```

**現状の誤った実装:**
```
受信割り込み → 全バイト送信 → 受信割り込み → リードバック
```

### 2-4. 全バイト一括送信の問題

**問題:**
- `l_vog_lin_tx_char(u1a_lin_data[], size)` が配列を受け取って一括送信
- H850では `l_vog_lin_tx_char(u1a_lin_data)` が単一バイトを送信

**影響:**
- 1バイトずつの送信制御ができない
- リードバックが1バイトずつ実行できない
- インターバイトスペースの挿入ができない

**関連ドキュメント:**
- [TX_READBACK_ISSUE_ANALYSIS.md](TX_READBACK_ISSUE_ANALYSIS.md)

### 2-5. 全バイト一括リードバックの問題

**問題:**
- `l_u1g_lin_read_back(u1a_lin_data[], size)` が配列を受け取って一括比較
- H850では `l_u1g_lin_read_back(u1a_lin_data)` が単一バイトを比較

**影響:**
- 1バイトずつのリードバック確認ができない
- 送信エラーの早期検出ができない

**関連ドキュメント:**
- [TX_READBACK_ISSUE_ANALYSIS.md](TX_READBACK_ISSUE_ANALYSIS.md)

---

## 3. H850アーキテクチャに基づく正しい実装方法

### 3-1. レスポンススペース実装

**タイミングチャート:**

```
IDフィールド受信
    ↓
    ├── レスポンススペース（1ビット長）──┐
    ↓                                    ↓
[タイマー割り込み]
    ↓
1バイト目送信開始
```

**実装:**

```c
// CORE層: IDフィールド受信時
if (xng_lin_slot_tbl[slot].u1g_lin_sndrcv == U1G_LIN_CMD_SND)
{
    // データをtmpバッファにコピー
    u1l_lin_rs_tmp[0..7] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[0..7];

    // チェックサム計算
    u1l_lin_rs_tmp[frm_sz] = l_vog_lin_checksum(...);

    // ★レスポンススペース待ちタイマー設定★
    l_vog_lin_bit_tm_set(U1G_LIN_RSSP);  // 1ビット長

    // 状態遷移
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;
}
```

### 3-2. 1バイト送信 + インターバイトスペース

**タイミングチャート:**

```
[タイマー割り込み1]
    ↓
1バイト目送信
    ↓
    ├── データ10ビット ──┐
    ├── インターバイトスペース（1ビット） ──┐
    ↓                                        ↓
[タイマー割り込み2]
    ↓
1バイト目リードバック確認
    ↓
2バイト目送信
    ↓
    ├── データ10ビット ──┐
    ├── インターバイトスペース（1ビット） ──┐
    ↓                                        ↓
[タイマー割り込み3]
    ↓
...
```

**実装:**

```c
// CORE層: タイマー割り込みハンドラ
case (U1G_LIN_SLSTS_SNDDATA_WAIT):

    // 2バイト目以降
    if (u1l_lin_rs_cnt > U1G_LIN_0)
    {
        // ★前回送信バイトのリードバック★
        l_u1a_lin_read_back = l_u1g_lin_read_back(u1l_lin_rs_tmp[u1l_lin_rs_cnt - U1G_LIN_1]);

        if (l_u1a_lin_read_back != U1G_LIN_OK)
        {
            // BITエラー
            xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete(U1G_LIN_ERR_ON);
            l_vol_lin_set_synchbreak();
        }
        else
        {
            // 送信完了確認
            if (u1l_lin_rs_cnt > u1l_lin_frm_sz)
            {
                // 全バイト送信完了
                l_vol_lin_set_frm_complete(U1G_LIN_ERR_OFF);
                l_vol_lin_set_synchbreak();
            }
            else
            {
                // チェックサム累積計算（チェックサム送信前）
                if (u1l_lin_rs_cnt < u1l_lin_frm_sz)
                {
                    u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[u1l_lin_rs_cnt];
                    u2l_lin_chksum = (u2l_lin_chksum & 0xFF) + ((u2l_lin_chksum >> 8) & 0xFF);
                }

                // ★次のバイト送信（1バイトのみ）★
                l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);

                // ★インターバイトスペース設定★
                l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);  // 10ビット + 1ビット

                u1l_lin_rs_cnt++;
            }
        }
    }
    // 1バイト目
    else
    {
        // チェックサム累積計算
        u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[u1l_lin_rs_cnt];
        u2l_lin_chksum = (u2l_lin_chksum & 0xFF) + ((u2l_lin_chksum >> 8) & 0xFF);

        // ★1バイト目送信★
        l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);

        // ★インターバイトスペース設定★
        l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);  // 10ビット + 1ビット

        u1l_lin_rs_cnt++;
    }
    break;
```

### 3-3. タイマー割り込み駆動の送信ループ

**状態遷移:**

```
IDENTFIELD_WAIT (IDフィールド受信待ち)
    ↓
    [IDフィールド受信割り込み]
    ↓
SNDDATA_WAIT (データ送信待ち)
    ↓
    [タイマー割り込み1] レスポンススペース後
    ↓
    1バイト目送信
    ↓
    [タイマー割り込み2] インターバイトスペース後
    ↓
    1バイト目リードバック + 2バイト目送信
    ↓
    [タイマー割り込み3] インターバイトスペース後
    ↓
    2バイト目リードバック + 3バイト目送信
    ↓
    ...
    ↓
    [タイマー割り込みN] インターバイトスペース後
    ↓
    最終バイトリードバック + 送信完了
    ↓
BREAK_UART_WAIT (次のフレーム待ち)
```

### 3-4. 1バイトずつのリードバック

**DRV層実装:**

```c
// DRV層: リードバック関数（1バイト版）
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)  // ★単一バイト引数★
{
    l_u8 u1a_lin_readback_data;
    l_u8 u1a_lin_result;
    l_u32 u4a_lin_error_status;
    int_fast16_t rxCount;

    // 受信完了確認
    rxCount = UART2_getRxCount(xnl_lin_uart_handle);
    if (rxCount <= 0) {
        return U1G_LIN_NG;  // 受信データなし
    }

    // エラーステータス確認
    u4a_lin_error_status = U4L_LIN_HWREG(
        ((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR
    );

    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        // ★1バイト受信データ取得★
        u1a_lin_readback_data = u1l_lin_rx_buf[0];

        // ★1バイト比較★
        if (u1a_lin_readback_data != u1a_lin_data)
        {
            u1a_lin_result = U1G_LIN_NG;
        }
        else
        {
            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return u1a_lin_result;
}
```

---

## 4. 具体的な修正内容

### 4-1. CORE層の修正

#### 修正箇所1: IDフィールド受信時の処理

**修正前 [CC23xx/l_slin_core_CC2340R53.c:1117-1165]:**
```c
else if (xng_lin_slot_tbl[slot].u1g_lin_sndrcv == U1G_LIN_CMD_SND)
{
    u1l_lin_chksum = U1G_LIN_BYTE_CLR;
    u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;

    l_vog_lin_rx_dis();  // 送信時は受信割り込み禁止

    // データをtmpバッファにコピー
    u1l_lin_rs_tmp[0..7] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[0..7];

    // チェックサム計算
    u1l_lin_rs_tmp[u1l_lin_frm_sz] = l_vog_lin_checksum(...);

    // ★問題★ 全バイト一括送信
    l_vog_lin_tx_char(u1l_lin_rs_tmp, u1l_lin_frm_sz + U1G_LIN_1);

    u1l_lin_slv_sts = U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT;  // ★誤った状態★
}
```

**修正後:**
```c
else if (xng_lin_slot_tbl[slot].u1g_lin_sndrcv == U1G_LIN_CMD_SND)
{
    l_vog_lin_frm_tm_stop();  // ヘッダータイムアウトタイマの停止
    u2l_lin_chksum = U2G_LIN_WORD_CLR;  // チェックサム演算用変数の初期化
    u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;  // データ送信カウンタの初期化

    l_vog_lin_rx_dis();  // 送信時は受信割り込み禁止

    // NM使用設定フレームの場合
    if (xng_lin_slot_tbl[slot].u1g_lin_nm_use == U1G_LIN_NM_USE)
    {
        // LINフレームバッファのNM部分(データ1のbit4-7)をクリア
        xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0] &= U1G_LIN_BUF_NM_CLR_MASK;
        // LINフレームにレスポンス送信ステータスをセット
        xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0] |= (u1g_lin_nm_info & U1G_LIN_NM_INFO_MASK);
    }

    // LINバッファのデータを送信用tmpバッファにコピー
    u1l_lin_rs_tmp[U1G_LIN_0] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0];
    u1l_lin_rs_tmp[U1G_LIN_1] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_1];
    u1l_lin_rs_tmp[U1G_LIN_2] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_2];
    u1l_lin_rs_tmp[U1G_LIN_3] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_3];
    u1l_lin_rs_tmp[U1G_LIN_4] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_4];
    u1l_lin_rs_tmp[U1G_LIN_5] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_5];
    u1l_lin_rs_tmp[U1G_LIN_6] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_6];
    u1l_lin_rs_tmp[U1G_LIN_7] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[U1G_LIN_7];

    // ★修正★ レスポンススペース待ちタイマセット
    l_vog_lin_bit_tm_set(U1G_LIN_RSSP);  // レスポンススペース（1ビット長）

    // ★修正★ データ送信待ち状態へ遷移
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;
}
```

#### 修正箇所2: タイマー割り込みハンドラの送信処理

**修正前 [CC23xx/l_slin_core_CC2340R53.c:822-841]:**
```c
case (U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT):
    f_LIN_Manager_Callback_TxComplete(slot, u1l_lin_frm_sz, u1l_lin_rs_tmp);

    // ★問題★ 全バイトまとめてリードバック
    l_u1a_lin_read_back = l_u1g_lin_read_back(u1l_lin_rs_tmp, u1l_lin_frm_sz + U1G_LIN_1);

    if (l_u1a_lin_read_back != U1G_LIN_OK)
    {
        // BITエラー
        xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete(U1G_LIN_ERR_ON);
        l_vol_lin_set_synchbreak();
    }
    else
    {
        l_vol_lin_set_frm_complete(U1G_LIN_ERR_OFF);
        l_vol_lin_set_synchbreak();
    }
    break;
```

**修正後:**
```c
// ★新規追加★ データ送信待ち状態
case (U1G_LIN_SLSTS_SNDDATA_WAIT):

    // データ送信2バイト目以降
    if (u1l_lin_rs_cnt > U1G_LIN_0)
    {
        // ★1バイトリードバック★
        l_u1a_lin_read_back = l_u1g_lin_read_back(u1l_lin_rs_tmp[u1l_lin_rs_cnt - U1G_LIN_1]);

        // リードバック失敗
        if (l_u1a_lin_read_back != U1G_LIN_OK)
        {
            // BITエラー
            xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete(U1G_LIN_ERR_ON);
            l_vol_lin_set_synchbreak();
        }
        // リードバック成功
        else
        {
            // 最後まで送信
            if (u1l_lin_rs_cnt > u1l_lin_frm_sz)
            {
                // 送信完了コールバック
                f_LIN_Manager_Callback_TxComplete(slot, u1l_lin_frm_sz, u1l_lin_rs_tmp);

                l_vol_lin_set_frm_complete(U1G_LIN_ERR_OFF);  // 転送成功
                l_vol_lin_set_synchbreak();
            }
            // まだ全てのデータを送信完了していない
            else
            {
                // データの送信（チェックサムまで送信していない）
                if (u1l_lin_rs_cnt < u1l_lin_frm_sz)
                {
                    u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[u1l_lin_rs_cnt];
                    u2l_lin_chksum = (u2l_lin_chksum & U2G_LIN_MASK_FF) + ((u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF);
                }
                // チェックサムの送信
                else
                {
                    // 送信用tmpバッファにコピー
                    u1l_lin_rs_tmp[u1l_lin_frm_sz] = (l_u8)(~((l_u8)u2l_lin_chksum));
                    // LINバッファにもコピー
                    xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_chksum = (l_u16)u1l_lin_rs_tmp[u1l_lin_frm_sz];
                }

                // ★1バイト送信★
                l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);

                // ★インターバイトスペース設定★
                l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);  // データ10bit + Inter-Byte Space

                u1l_lin_rs_cnt++;
            }
        }
    }
    // データ送信1バイト目
    else
    {
        u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[u1l_lin_rs_cnt];
        u2l_lin_chksum = (u2l_lin_chksum & U2G_LIN_MASK_FF) + ((u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF);

        // ★1バイト送信★
        l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);

        // ★インターバイトスペース設定★
        l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);  // データ10bit + Inter-Byte Space

        u1l_lin_rs_cnt++;
    }
    break;
```

#### 修正箇所3: 不要な状態の削除

**削除対象:**
```c
// ★削除★ AFTER_SNDDATA_WAIT 状態は不要
case (U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT):
    // ...（上記コードを削除）
```

### 4-2. DRV層の修正

#### 修正箇所1: 送信関数のシグネチャ変更

**修正前 [CC23xx/l_slin_drv_cc2340r53.h]:**
```c
extern void l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size);
```

**修正後:**
```c
extern void l_vog_lin_tx_char(l_u8 u1a_lin_data);  // ★単一バイト★
```

#### 修正箇所2: 送信関数の実装変更

**修正前 [CC23xx/l_slin_drv_cc2340r53.c:431-440]:**
```c
void l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE, u1a_lin_data_size);

    #if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle, u1a_lin_data, u1a_lin_data_size, NULL);
    #else
    UART_RegInt_write(xnl_lin_uart_handle, u1a_lin_data, u1a_lin_data_size);
    #endif
}
```

**修正後:**
```c
void l_vog_lin_tx_char(l_u8 u1a_lin_data)  // ★単一バイト引数★
{
    // リードバック用に1バイト受信設定
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE);  // ★サイズ指定削除★

    // 送信バッファに1バイトコピー
    u1l_lin_tx_buf[0] = u1a_lin_data;

    // ★1バイト送信★
    #if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1, NULL);
    #else
    UART_RegInt_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1);
    #endif
}
```

#### 修正箇所3: リードバック関数のシグネチャ変更

**修正前 [CC23xx/l_slin_drv_cc2340r53.h]:**
```c
extern l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data[], l_u8 u1a_lin_data_size);
```

**修正後:**
```c
extern l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data);  // ★単一バイト★
```

#### 修正箇所4: リードバック関数の実装変更

**修正前 [CC23xx/l_slin_drv_cc2340r53.c:468-498]:**
```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data[], l_u8 u1a_lin_data_size)
{
    l_u8 u1a_lin_result;
    l_u32 u4a_lin_error_status;
    l_u8 u1a_lin_index;

    u4a_lin_error_status = U4L_LIN_HWREG(((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR);
    u4a_lin_error_status = U4G_DAT_ZERO;  // ★問題★ エラー検出機能オフ

    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        u1a_lin_result = U1G_LIN_OK;
        // ★問題★ 複数バイト比較
        for (u1a_lin_index = 0; u1a_lin_index < u1a_lin_data_size; u1a_lin_index++)
        {
            if (u1l_lin_rx_buf[u1a_lin_index] != u1a_lin_data[u1a_lin_index])
            {
                u1a_lin_result = U1G_LIN_NG;
            }
        }
    }

    return u1a_lin_result;
}
```

**修正後:**
```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)  // ★単一バイト引数★
{
    l_u8 u1a_lin_readback_data;
    l_u8 u1a_lin_result;
    l_u32 u4a_lin_error_status;
    int_fast16_t rxCount;

    // ★追加★ 受信完了確認
    rxCount = UART2_getRxCount(xnl_lin_uart_handle);
    if (rxCount <= 0) {
        return U1G_LIN_NG;  // 受信データなし
    }

    // エラーステータスをレジスタから取得
    u4a_lin_error_status = U4L_LIN_HWREG(
        ((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR
    );
    // ★削除★ u4a_lin_error_status = U4G_DAT_ZERO; （エラー検出有効化）

    // エラーが発生しているかをチェック
    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        // ★修正★ 1バイト受信データ取得
        u1a_lin_readback_data = u1l_lin_rx_buf[0];

        // ★修正★ 1バイト比較
        if (u1a_lin_readback_data != u1a_lin_data)
        {
            u1a_lin_result = U1G_LIN_NG;
        }
        else
        {
            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return u1a_lin_result;
}
```

#### 修正箇所5: 受信許可関数のシグネチャ変更

**修正前 [CC23xx/l_slin_drv_cc2340r53.h]:**
```c
extern void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size);
```

**修正後:**
```c
extern void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx);  // ★サイズ削除★
```

#### 修正箇所6: 受信許可関数の実装変更

**修正前:**
```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
{
    l_vog_lin_irq_dis();

    if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE) {
        UART_RegInt_readCancel(xnl_lin_uart_handle);
    }

    // ★可変サイズ受信★
    UART_RegInt_read(xnl_lin_uart_handle, u1l_lin_rx_buf, u1a_lin_rx_data_size);

    l_vog_lin_irq_res();
}
```

**修正後:**
```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx)  // ★サイズパラメータ削除★
{
    l_vog_lin_irq_dis();

    if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE) {
        UART_RegInt_readCancel(xnl_lin_uart_handle);
    }

    // ★常に1バイト受信★
    UART_RegInt_read(xnl_lin_uart_handle, u1l_lin_rx_buf, 1);

    l_vog_lin_irq_res();
}
```

### 4-3. タイマー関連の実装

**タイマー割り込みの実装方法については以下を参照:**
- [LGPTIMER_LIN_TIMEOUT.md](CC23xx/LGPTIMER_LIN_TIMEOUT.md)

**必要な関数:**
```c
// タイマー初期化
extern void l_vog_lin_timer_init(void);

// ビットタイマ設定
extern void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit);

// フレームタイマ停止
extern void l_vog_lin_frm_tm_stop(void);
```

---

## 5. コード修正例

### 5-1. IDフィールド受信時の処理（完全版）

```c
// [CC23xx/l_slin_core_CC2340R53.c] - 修正後
// IDフィールド受信時 (送信フレームの場合)
else if (xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_sndrcv == U1G_LIN_CMD_SND)
{
    l_vog_lin_frm_tm_stop();                            // ヘッダータイムアウトタイマの停止
    u2l_lin_chksum = U2G_LIN_WORD_CLR;                  // チェックサム演算用変数の初期化
    u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                  // データ送信カウンタの初期化

    l_vog_lin_rx_dis();                                 // 送信時は受信割り込み禁止

    // NM使用設定フレームの場合
    if (xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_nm_use == U1G_LIN_NM_USE)
    {
        // LINフレームバッファのNM部分(データ1のbit4-7)をクリア
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0]
            &= U1G_LIN_BUF_NM_CLR_MASK;
        // LINフレームにレスポンス送信ステータスをセット
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0]
            |= (u1g_lin_nm_info & U1G_LIN_NM_INFO_MASK);
    }

    // LINバッファのデータを送信用tmpバッファにコピー
    u1l_lin_rs_tmp[U1G_LIN_0] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_0];
    u1l_lin_rs_tmp[U1G_LIN_1] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_1];
    u1l_lin_rs_tmp[U1G_LIN_2] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_2];
    u1l_lin_rs_tmp[U1G_LIN_3] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_3];
    u1l_lin_rs_tmp[U1G_LIN_4] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_4];
    u1l_lin_rs_tmp[U1G_LIN_5] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_5];
    u1l_lin_rs_tmp[U1G_LIN_6] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_6];
    u1l_lin_rs_tmp[U1G_LIN_7] =
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].xng_lin_data.u1g_lin_byte[U1G_LIN_7];

    l_vog_lin_bit_tm_set(U1G_LIN_RSSP);                 // レスポンススペース待ちタイマセット

    u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;       // データ送信待ち状態
}
```

### 5-2. タイマー割り込みハンドラの送信処理（完全版）

```c
// [CC23xx/l_slin_core_CC2340R53.c] - タイマー割り込みハンドラ内
// データ送信待ち状態
case (U1G_LIN_SLSTS_SNDDATA_WAIT):

    // データ送信2バイト目以降
    if (u1l_lin_rs_cnt > U1G_LIN_0)
    {
        l_u1a_lin_read_back = l_u1g_lin_read_back(u1l_lin_rs_tmp[u1l_lin_rs_cnt - U1G_LIN_1]);

        // リードバック失敗(受信割り込みがかからない場合、もしくは受信バッファの内容が引数と異なる)
        if (l_u1a_lin_read_back != U1G_LIN_OK)
        {
            // BITエラー
            xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete(U1G_LIN_ERR_ON);     // エラーありレスポンス完了
            l_vol_lin_set_synchbreak();                     // Synch Break受信待ち設定
        }
        // リードバック成功
        else
        {
            // 最後まで送信
            if (u1l_lin_rs_cnt > u1l_lin_frm_sz)            // DL + 1
            {
                // 送信完了コールバック
                f_LIN_Manager_Callback_TxComplete(xnl_lin_id_sl.u1g_lin_slot, u1l_lin_frm_sz, u1l_lin_rs_tmp);

                l_vol_lin_set_frm_complete(U1G_LIN_ERR_OFF); // 転送成功
                l_vol_lin_set_synchbreak();                  // Synch Break受信待ち設定
            }
            // まだ全てのデータを送信完了していない場合
            else
            {
                // データの送信(チェックサムまで送信していない)
                if (u1l_lin_rs_cnt < u1l_lin_frm_sz)
                {
                    u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[u1l_lin_rs_cnt];
                    u2l_lin_chksum = (u2l_lin_chksum & U2G_LIN_MASK_FF) + ((u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF);
                }
                // チェックサムの送信
                else
                {
                    // 送信用tmpバッファにコピー
                    u1l_lin_rs_tmp[u1l_lin_frm_sz] = (l_u8)(~((l_u8)u2l_lin_chksum));
                    // LINバッファにもコピー
                    xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].un_state.st_bit.u2g_lin_chksum
                        = (l_u16)u1l_lin_rs_tmp[u1l_lin_frm_sz];
                }

                l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);     // データ送信
                l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP); // データ10bit + Inter-Byte Space
                u1l_lin_rs_cnt++;
            }
        }
    }
    // データ送信1バイト目
    else
    {
        u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[u1l_lin_rs_cnt];
        u2l_lin_chksum = (u2l_lin_chksum & U2G_LIN_MASK_FF) + ((u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF);

        l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt]);         // データ送信
        l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP);  // データ10bit + Inter-Byte Space
        u1l_lin_rs_cnt++;
    }
    break;
```

### 5-3. レスポンススペース/インターバイトスペースのタイマー設定

```c
// [CC23xx/l_slin_def.h] - 定数定義
#define U1G_LIN_RSSP        ((l_u8)1)   // レスポンススペース（1ビット長）
#define U1G_LIN_BTSP        ((l_u8)1)   // インターバイトスペース（1ビット長）
#define U1G_LIN_BYTE_LENGTH ((l_u8)10)  // 1バイト長（スタートビット + 8データビット + ストップビット）

// [CC23xx/l_slin_drv_cc2340r53.c] - タイマー設定実装
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    uint32_t counterTarget;
    uint32_t timeout_us;

    // タイマー停止
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    // ビット長をマイクロ秒に変換
    // @9600bps: 1ビット = 104.17us
    // timeout_us = u1a_lin_bit * 1041667 / 10000
    timeout_us = ((uint32_t)u1a_lin_bit * u4l_lin_us_per_bit_x10000) / 10000UL;

    // カウンタターゲット計算
    // システムクロック48MHz = 48カウント/us
    counterTarget = (timeout_us * 48UL);
    if (counterTarget > 0)
    {
        counterTarget -= 1UL;
    }

    // オーバーフロー対策（16ビットタイマーの場合）
    if (counterTarget > 0xFFFFUL)
    {
        counterTarget = 0xFFFFUL;
    }

    // カウンタターゲット設定
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, counterTarget, true);

    // ターゲット割り込み許可
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    // ワンショットモードで開始
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

---

## 6. 関連ドキュメント

### 6-1. 既存の分析ドキュメント

| ドキュメント | 関連内容 |
|------------|---------|
| [TX_READBACK_ISSUE_ANALYSIS.md](TX_READBACK_ISSUE_ANALYSIS.md) | 送信・リードバック処理の1バイト対応修正 |
| [NM_LAYER_COMPARISON.md](NM_LAYER_COMPARISON.md) | NM層の比較分析（状態管理は同一） |
| [LGPTIMER_LIN_TIMEOUT.md](CC23xx/LGPTIMER_LIN_TIMEOUT.md) | タイマー割り込みの実装方法 |

### 6-2. LIN仕様書

- **LIN Specification 2.x**: レスポンススペース、インターバイトスペースの規定
- **LIN Specification 1.3**: H850で実装されているLIN仕様

---

## 7. まとめ

### 7-1. 3プラットフォームの比較結果

| 項目 | F24 | H850 | CC23xx（現状） | CC23xx（修正後） |
|------|-----|------|---------------|----------------|
| **レスポンススペース** | HW自動 | タイマー実装 | ❌ 未実装 | ✅ タイマー実装 |
| **インターバイトスペース** | HW自動 | タイマー実装 | ❌ 未実装 | ✅ タイマー実装 |
| **1バイト送信** | HW自動 | SW制御 | ❌ 一括送信 | ✅ SW制御 |
| **1バイトリードバック** | 不要 | SW制御 | ❌ 一括比較 | ✅ SW制御 |
| **タイマー割り込み駆動** | 不要 | ✅ 実装済み | ❌ 未実装 | ✅ 実装 |

### 7-2. CC23xxで必要な修正

1. **CORE層の修正**
   - IDフィールド受信時: レスポンススペース待ちタイマー設定
   - タイマー割り込みハンドラ: `SNDDATA_WAIT` 状態の追加
   - 1バイトずつの送信・リードバック処理

2. **DRV層の修正**
   - `l_vog_lin_tx_char(u1a_data)`: 単一バイト送信
   - `l_u1g_lin_read_back(u1a_data)`: 単一バイト比較
   - `l_vog_lin_rx_enb(flush)`: サイズ指定削除
   - `l_vog_lin_bit_tm_set(bit)`: タイマー設定実装

3. **タイマー実装**
   - LGPTimerLPF3を使用したビットタイマー
   - レスポンススペース（1ビット長）
   - インターバイトスペース（1ビット長）

### 7-3. 修正後の動作

修正後のCC23xxは、H850と同等の動作となります：

```
[IDフィールド受信]
    ↓
レスポンススペース（1ビット）
    ↓
[タイマー割り込み1]
    ↓
1バイト目送信
    ↓
データ10ビット + インターバイトスペース（1ビット）
    ↓
[タイマー割り込み2]
    ↓
1バイト目リードバック + 2バイト目送信
    ↓
データ10ビット + インターバイトスペース（1ビット）
    ↓
[タイマー割り込み3]
    ↓
...
    ↓
送信完了
```

これにより、LIN仕様に準拠した正しいレスポンス送信が実現されます。

---

**Document Version**: 1.0
**Last Updated**: 2025-12-01
**Author**: LIN Response Transmission Comparison Analysis Tool
