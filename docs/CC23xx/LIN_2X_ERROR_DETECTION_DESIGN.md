# LIN 2.x エラー検出機能の設計と実装

## 変更日
2025-12-01

## 目的
CC23xxのLIN実装にLIN 2.xで要求されるエラー検出機能を追加する。H850（LIN 1.3）のアーキテクチャを維持しつつ、F24（LIN 2.x）の機能をソフトウェアで実装する。

---

## LIN 2.xで要求されるエラー種別

LIN_1.3_vs_2.x_COMPARISON.mdから抽出した実装エラー：

| エラー種類 | 検出方法 | 優先度 | 実装状況 |
|-----------|---------|-------|---------|
| **No Response** | タイムアウト検出 | 高 | ✅ 実装済み |
| **BIT Error** | リードバック比較 | 高 | ✅ 実装済み |
| **Header Timeout** | タイムアウト検出 | 高 | ✅ 実装済み |
| **Physical Bus Error** | Header Timeoutカウンタ（510回連続） | 中 | ✅ 実装済み |
| **Response Too Short** | タイムアウト + バイトカウント | 中 | ✅ 実装済み |

---

## 実装状況の詳細分析

### 1. Header Timeout Error ✅ 実装済み

**検出条件:**
- Synch Break IRQ待ち、Synch Field待ち、Ident Field待ちの状態でタイマータイムアウト発生

**実装箇所:** [l_slin_core_CC2340R53.c:1150-1166](l_slin_core_CC2340R53.c#L1150-L1166)

**H850との比較:**
```c
// H850実装 (l_slin_core_h83687.c:1129-1145)
case ( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
case ( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
case ( U1G_LIN_SLSTS_IDENTFIELD_WAIT ):
    /* header timeoutエラー */
    xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time = U2G_LIN_BIT_SET;

    /* Physical Busエラーチェック */
    if(u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT){
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
        xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
        u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;
    }else{
        u2l_lin_herr_cnt++;
    }
    l_vol_lin_set_synchbreak();
    break;

// CC23xx実装 (l_slin_core_CC2340R53.c:1150-1166)
case ( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
case ( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
case ( U1G_LIN_SLSTS_IDENTFIELD_WAIT ):
    /* header timeoutエラー */
    xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time = U2G_LIN_BIT_SET;

    /* Physical Busエラーチェック */
    if(u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT){
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
        xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
        u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;
    }else{
        u2l_lin_herr_cnt++;
    }
    l_vol_lin_set_synchbreak();
    break;
```

**結論:** H850と完全に同じ実装。✅ 問題なし

---

### 2. Physical Bus Error ✅ 実装済み

**検出条件:**
- Header Timeoutが連続3回発生（実際には510回）
- BREAK_UART_WAIT状態で25000bit時間経過（約1.3秒 @19200bps）

**実装箇所:**
1. カウンタ制限定義: [l_slin_core_CC2340R53.h:73](l_slin_core_CC2340R53.h#L73)
```c
#define U2G_LIN_HERR_LIMIT  ((l_u16)510)  /* 25000bitタイム分のヘッダタイム回数(25000 / 49) */
```

2. Header Timeout時のカウント: [l_slin_core_CC2340R53.c:1157-1164](l_slin_core_CC2340R53.c#L1157-L1164)
```c
if(u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT){
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
    xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
    u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;
}else{
    u2l_lin_herr_cnt++;
}
```

3. BREAK_UART_WAIT時の検出: [l_slin_core_CC2340R53.c:1144-1148](l_slin_core_CC2340R53.c#L1144-L1148)
```c
case ( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
    /* Physical Busエラー */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
    xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
    break;
```

4. タイマー起動: [l_slin_core_CC2340R53.c:1286](l_slin_core_CC2340R53.c#L1286)
```c
static void l_vol_lin_set_synchbreak(void)
{
    l_vog_lin_bus_tm_set();  /* Physical Bus Error検出タイマ起動 */
    // ...
}
```

**H850との比較:**
- H850: [l_slin_core_h83687.c:1136-1143](../H850/slin_lib/l_slin_core_h83687.c#L1136-L1143)
- 同じロジック

**結論:** H850と完全に同じ実装。✅ 問題なし

---

### 3. No Response Error ✅ 実装済み

**検出条件:**
- データ受信待ち状態（RCVDATA_WAIT）でタイマータイムアウト発生
- SLEEPコマンドIDの場合は除外

**実装箇所:** [l_slin_core_CC2340R53.c:1168-1181](l_slin_core_CC2340R53.c#L1168-L1181)

**H850との比較:**
```c
// H850実装 (l_slin_core_h83687.c:1147-1160)
case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
    if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) &&
        (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
        l_vol_lin_set_synchbreak();
    }
    else{
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    break;

// CC23xx実装 (l_slin_core_CC2340R53.c:1168-1181)
case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
    if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) &&
        (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
        l_vol_lin_set_synchbreak();
    }
    else{
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    break;
```

**結論:** H850と完全に同じ実装。✅ 問題なし

---

### 4. BIT Error ✅ 実装済み

**検出条件:**
- データ送信待ち状態（SNDDATA_WAIT）でリードバック失敗

**実装箇所:** [l_slin_core_CC2340R53.c:1185-1194](l_slin_core_CC2340R53.c#L1185-L1194)

**H850との比較:**
```c
// H850実装 (l_slin_core_h83687.c:1164-1173)
if( u1l_lin_rs_cnt > U1G_LIN_0 ){
    l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );
    if( l_u1a_lin_read_back != U1G_LIN_OK ){
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    // ...
}

// CC23xx実装 (l_slin_core_CC2340R53.c:1185-1194)
if( u1l_lin_rs_cnt > U1G_LIN_0 ){
    l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );
    if( l_u1a_lin_read_back != U1G_LIN_OK ){
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    // ...
}
```

**結論:** H850と完全に同じ実装。✅ 問題なし

---

### 5. Response Too Short Error ✅ 実装済み

**検出方法（LIN 2.x仕様）:**
- レスポンス受信タイムアウト発生時に、受信バイト数をチェック
- 1バイト目（Data Field 1）を受信している場合 → **Response Too Short**
- 1バイト目も受信していない場合 → **No Response**

**実装状況:**
- ✅ CC23xx: バイトカウンタ（`u1l_lin_rs_cnt`）によるエラー区別を実装
- H850: バイトカウントによる区別なし（LIN 1.3準拠）

**F24の実装（参考）:**
[F24/l_slin_drv_rl78f24.c:824-853](../F24/l_slin_drv_rl78f24.c#L824-L853)
```c
void l_vog_lin_determine_ResTooShort_or_NoRes(l_u8 u1a_lst_d1rc_state)
{
    if( U1G_LIN_BYTE_CLR != (U1G_LIN_TAU_TE & U1G_LIN_TE_ENABLE) ) {
        l_vog_lin_stop_timer();

        if( u1a_lst_d1rc_state == U1G_LIN_LST_D1RC_COMPLETE ) {
            /* データ1受信フラグが 1 の場合、Response Too Shortを確定 */
            l_vog_lin_ResTooShort_error();
        }
        else if( u1a_lst_d1rc_state == U1G_LIN_LST_D1RC_INCOMPLETE ) {
            /* データ1受信フラグが 0 の場合、No Response を確定 */
            l_vog_lin_NoRes_error();
        }

        U1G_LIN_SFR_LST &= (~U1G_LIN_LST_D1RC_MASK);
    }
}
```

**F24の特徴:**
- ハードウェアの`D1RC`フラグ（Data 1 Receive Complete）を利用
- タイムアウト発生時にこのフラグを確認してエラー種別を決定

**CC23xxでの実装（✅ 実装済み）:**

**実装箇所:** [l_slin_core_CC2340R53.c:1168-1188](l_slin_core_CC2340R53.c#L1168-L1188)
```c
case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
    if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) &&
        (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
        l_vol_lin_set_synchbreak();
    }
    else{
        /* 1バイト目を受信済みかチェック (LIN 2.x Response Too Short検出) */
        if( u1l_lin_rs_cnt > U1G_LIN_0 ){
            /* Response Too Short エラー (1バイト目受信後、残りのバイトが受信できなかった) */
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_tooshort = U2G_LIN_BIT_SET;
        }
        else{
            /* No Responseエラー (1バイト目すら受信できなかった) */
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
        }

        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    break;
```

**実装内容:**
1. ✅ `u2g_lin_e_tooshort` フラグの追加（[l_slin_tbl.h:164,330](l_slin_tbl.h#L164)）
2. ✅ タイマー割り込みハンドラでのバイトカウント判定実装

**実装の詳細:** [RESPONSE_TOO_SHORT_IMPLEMENTATION.md](RESPONSE_TOO_SHORT_IMPLEMENTATION.md)

---

## エラーフラグ定義の確認

### 既存フラグ（l_slin_api.h）

**ステータスバッファ構造体:**
```c
typedef union {
    struct {
        l_u16 u2g_lin_sts         : 2;  /* LINステータス */
        l_u16 u2g_lin_e_uart      : 1;  /* UARTエラー */
        l_u16 u2g_lin_e_synch     : 1;  /* Synchフィールドエラー */
        l_u16 u2g_lin_e_pari      : 1;  /* パリティエラー */
        l_u16 u2g_lin_e_sum       : 1;  /* チェックサムエラー */
        l_u16 u2g_lin_e_nores     : 1;  /* No Responseエラー */
        l_u16 u2g_lin_e_bit       : 1;  /* BITエラー */
        l_u16 u2g_lin_e_time      : 1;  /* Header Timeoutエラー */
        l_u16 u2g_lin_phy_bus_err : 1;  /* Physical Busエラー */
        // ...
    } st_bit;
    l_u16 u2g_lin_word;
} un_lin_sts_buf;
```

### Response Too Short用フラグの追加要否

**検討結果:**
- 現状、`u2g_lin_e_nores`フラグでNo Responseを検出
- Response Too Shortを区別するには新しいフラグが必要
- しかし、H850（LIN 1.3ベース）では未定義
- **結論: 現状は追加不要。将来の拡張として保留**

---

## タイマー設定の確認

### Header Timeout用タイマー

**設定箇所:** [l_slin_core_CC2340R53.c:708](l_slin_core_CC2340R53.c#L708), [l_slin_core_CC2340R53.c:748](l_slin_core_CC2340R53.c#L748)
```c
l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
// 49 bit - 10 bit = 39 bit = 2.03ms @19200bps
```

**実装:** [l_slin_drv_cc2340r53.c:631-670](l_slin_drv_cc2340r53.c#L631-L670)
- ✅ 実装済み

### No Response / Response Too Short用タイマー

**設定箇所:** [l_slin_core_CC2340R53.c:1014](l_slin_core_CC2340R53.c#L1014)
```c
l_vog_lin_rcv_tm_set( (u1l_lin_frm_sz + U1G_LIN_1) * U1G_LIN_BYTE_LENGTH );
// (DL + 1) × 10 bit × 1.4 = タイムアウト時間
```

**実装:** [l_slin_drv_cc2340r53.c:709-732](l_slin_drv_cc2340r53.c#L709-L732)
- ✅ 実装済み

### Physical Bus Error用タイマー

**設定箇所:** [l_slin_core_CC2340R53.c:1286](l_slin_core_CC2340R53.c#L1286)
```c
l_vog_lin_bus_tm_set();
// 25000 bit = 1.302s @19200bps
```

**実装:** [l_slin_drv_cc2340r53.c:753-777](l_slin_drv_cc2340r53.c#L753-L777)
- ✅ 実装済み

---

## 実装状況まとめ

| エラー種類 | 優先度 | 実装状況 | 実装箇所 | H850互換性 |
|-----------|-------|---------|---------|-----------|
| **Header Timeout** | 高 | ✅ 完了 | l_vog_lin_tm_int, case BREAK_IRQ_WAIT等 | ✅ 完全互換 |
| **Physical Bus Error** | 中 | ✅ 完了 | l_vog_lin_tm_int, case BREAK_UART_WAIT | ✅ 完全互換 |
| **No Response** | 高 | ✅ 完了 | l_vog_lin_tm_int, case RCVDATA_WAIT | ✅ 完全互換 |
| **BIT Error** | 高 | ✅ 完了 | l_vog_lin_tm_int, case SNDDATA_WAIT | ✅ 完全互換 |
| **Response Too Short** | 中 | ✅ 完了 | l_vog_lin_tm_int, case RCVDATA_WAIT | ⚠️ H850未実装（拡張機能） |

---

## Response Too Short実装の推奨設計

### 実装の必要性評価

**LIN 2.x仕様要件:**
- Response Too ShortとNo Responseは別エラーとして定義
- Response Too Short: 1バイト目受信後、残りのバイトが受信できなかった
- No Response: 1バイト目すら受信できなかった

**実用上の考慮:**
1. **エラーハンドリングは同じ**
   - どちらもフレーム破棄
   - 再送要求（必要な場合）
   - 診断カウンタのインクリメント

2. **H850（LIN 1.3）では未実装**
   - 既存システムでは区別なし
   - No Responseのみで運用実績あり

3. **F24（LIN 2.x）の実装**
   - ハードウェアの`D1RC`フラグで判定
   - CC23xxではソフトウェア実装が必要

**結論:**
- **現時点では実装不要**
- H850互換性を優先
- 将来の拡張として設計案を残す

---

## Response Too Short実装（✅ 実装完了）

### 実装内容

#### 1. エラーフラグの追加（✅ 完了）

**l_slin_tbl.h:**
```c
/* リトルエンディアン（158-167行目） */
struct {
    l_u16   u2g_lin_chksum:8;
    l_u16   u2g_lin_e_nores:1;
    l_u16   u2g_lin_e_uart:1;
    l_u16   u2g_lin_e_bit:1;
    l_u16   u2g_lin_e_sum:1;
    l_u16   u2g_lin_e_tooshort:1;  /* Response Too Short エラー (LIN 2.x) */
    l_u16   reserve:2;
    l_u16   u2g_lin_no_use:1;
} st_bit;

/* ビッグエンディアン（327-336行目） */
struct {
    l_u16   u2g_lin_no_use:1;
    l_u16   reserve:2;
    l_u16   u2g_lin_e_tooshort:1;  /* Response Too Short エラー (LIN 2.x) */
    l_u16   u2g_lin_e_sum:1;
    l_u16   u2g_lin_e_bit:1;
    l_u16   u2g_lin_e_uart:1;
    l_u16   u2g_lin_e_nores:1;
    l_u16   u2g_lin_chksum:8;
} st_bit;
```

#### 2. タイマー割り込みハンドラの修正（✅ 完了）

**l_slin_core_CC2340R53.c: l_vog_lin_tm_int()（1168-1188行目）**
```c
case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
    if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) &&
        (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
        l_vol_lin_set_synchbreak();
    }
    else{
        /* 1バイト目を受信済みかチェック (LIN 2.x Response Too Short検出) */
        if( u1l_lin_rs_cnt > U1G_LIN_0 ){
            /* Response Too Short エラー */
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_tooshort = U2G_LIN_BIT_SET;
        }
        else{
            /* No Response エラー */
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
        }

        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    break;
```

**詳細ドキュメント:** [RESPONSE_TOO_SHORT_IMPLEMENTATION.md](RESPONSE_TOO_SHORT_IMPLEMENTATION.md)

---

## テスト項目

### 1. Header Timeout Error

**テストケース:**
1. Break受信後、Synch Field (0x55) を送信しない
   - **期待結果:** 39bit後にHeader Timeoutエラー
2. Synch Field受信後、ID Fieldを送信しない
   - **期待結果:** タイムアウトでHeader Timeoutエラー
3. Header Timeout連続発生
   - **期待結果:** 510回でPhysical Bus Error

### 2. Physical Bus Error

**テストケース:**
1. BREAK_UART_WAIT状態で25000bit時間経過
   - **期待結果:** Physical Bus Errorフラグセット
2. Header Timeout 510回連続
   - **期待結果:** Physical Bus Errorフラグセット、カウンタリセット

### 3. No Response Error

**テストケース:**
1. 受信フレームID受信後、データフィールドを送信しない
   - **期待結果:** (DL+1)×10×1.4 bit後にNo Responseエラー
2. SLEEPコマンドID (0x3C) でフレーム定義なし
   - **期待結果:** タイムアウトしても何もしない、エラーなし

### 4. BIT Error

**テストケース:**
1. 送信データと異なる値をリードバック
   - **期待結果:** BITエラーフラグセット
2. リードバック時にUARTエラー発生
   - **期待結果:** BITエラーフラグセット

### 5. タイマー干渉テスト

**テストケース:**
1. Header Timeout中にNo Response検出
   - **期待結果:** 状態に応じて適切なエラー検出
2. Physical Bus Timeout中にHeader Timeout
   - **期待結果:** 排他制御により正常動作

---

## 関連ドキュメント

- [LIN_1.3_vs_2.x_COMPARISON.md](../LIN_1.3_vs_2.x_COMPARISON.md) - LIN 1.3と2.xの比較
- [TIMER_FUNCTIONS_IMPLEMENTATION.md](TIMER_FUNCTIONS_IMPLEMENTATION.md) - タイマー関数実装
- [TIMER_INTERFERENCE_ANALYSIS.md](TIMER_INTERFERENCE_ANALYSIS.md) - タイマー干渉解析
- [STATE_TRANSITION_ANALYSIS.md](STATE_TRANSITION_ANALYSIS.md) - 状態遷移解析
- [RX_INT_H850_COMPAT_IMPLEMENTATION.md](RX_INT_H850_COMPAT_IMPLEMENTATION.md) - 受信割り込み実装

---

## 結論

### 実装状況
CC23xxのLIN実装には、**LIN 2.xで要求されるすべてのエラー検出機能が実装済み**です。

| 機能 | 状態 |
|-----|------|
| Header Timeout | ✅ H850完全互換で実装済み |
| Physical Bus Error | ✅ H850完全互換で実装済み |
| No Response | ✅ H850完全互換で実装済み |
| BIT Error | ✅ H850完全互換で実装済み |
| Response Too Short | ✅ LIN 2.x拡張機能として実装済み |

### 実装完了
- **LIN 2.xのすべてのエラー検出要件を満たしている** ✅
- H850アーキテクチャとの互換性を維持（Response Too Shortは拡張機能）
- タイマー駆動による高精度なタイムアウト検出
- ステートマシンによる排他的なタイマー使用
- バイトカウンタ判定によるResponse Too Short / No Response区別

---

**Document Version:** 2.0
**Last Updated:** 2025-12-01
**Status:** ✅ 全機能実装完了（Response Too Short含む）
