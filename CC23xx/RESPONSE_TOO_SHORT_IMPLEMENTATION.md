# Response Too Short エラー検出の実装

## 変更日
2025-12-01

## 目的
LIN 2.xで要求される **Response Too Short** エラー検出機能を実装し、**No Response** エラーと区別できるようにする。

---

## LIN 2.x仕様の要件

### Response Too Short vs No Response

LIN 2.x規格では、受信タイムアウト時に以下の2つのエラーを区別します：

| エラー種類 | 条件 | 意味 |
|-----------|------|------|
| **No Response** | 1バイト目すら受信できなかった | スレーブが全く応答しない |
| **Response Too Short** | 1バイト目受信後、残りのバイトが受信できなかった | スレーブが途中まで応答したが、完全なレスポンスを送信できなかった |

### F24（LIN 2.x専用ハードウェア）の実装

**[F24/l_slin_drv_rl78f24.c:824-853](../F24/l_slin_drv_rl78f24.c#L824-L853)**
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

---

## CC23xxでの実装方法

CC23xxは専用LINハードウェアを持たないため、**受信バイトカウンタ（u1l_lin_rs_cnt）** でソフトウェア判定します。

### 実装アプローチ

```
タイムアウト発生時:
  ↓
u1l_lin_rs_cnt > 0？
  ↓              ↓
 YES            NO
  ↓              ↓
Response      No Response
Too Short
```

---

## 実装内容

### 1. エラーフラグの追加

**修正ファイル:** [l_slin_tbl.h](l_slin_tbl.h)

#### リトルエンディアン定義（158-167行目）

**変更前:**
```c
        struct {
            l_u16   u2g_lin_chksum:8;
            l_u16   u2g_lin_e_nores:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_bit:1;
            l_u16   u2g_lin_e_sum:1;
            l_u16   reserve:3;              // ← 3ビット予約
            l_u16   u2g_lin_no_use:1;
        } st_bit;
```

**変更後:**
```c
        struct {
            l_u16   u2g_lin_chksum:8;
            l_u16   u2g_lin_e_nores:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_bit:1;
            l_u16   u2g_lin_e_sum:1;
            l_u16   u2g_lin_e_tooshort:1;  /* Response Too Short エラー (LIN 2.x) */
            l_u16   reserve:2;              // ← 2ビット予約に変更
            l_u16   u2g_lin_no_use:1;
        } st_bit;
```

#### ビッグエンディアン定義（327-336行目）

**変更前:**
```c
        struct {
            l_u16   u2g_lin_no_use:1;
            l_u16   reserve:3;              // ← 3ビット予約
            l_u16   u2g_lin_e_sum:1;
            l_u16   u2g_lin_e_bit:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_nores:1;
            l_u16   u2g_lin_chksum:8;
        } st_bit;
```

**変更後:**
```c
        struct {
            l_u16   u2g_lin_no_use:1;
            l_u16   reserve:2;              // ← 2ビット予約に変更
            l_u16   u2g_lin_e_tooshort:1;  /* Response Too Short エラー (LIN 2.x) */
            l_u16   u2g_lin_e_sum:1;
            l_u16   u2g_lin_e_bit:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_nores:1;
            l_u16   u2g_lin_chksum:8;
        } st_bit;
```

**変更点:**
- `reserve`フィールドを3ビット→2ビットに削減
- `u2g_lin_e_tooshort`フラグを1ビット追加
- ビットフィールドの合計は16ビットで変更なし

---

### 2. タイマー割り込みハンドラの修正

**修正ファイル:** [l_slin_core_CC2340R53.c](l_slin_core_CC2340R53.c)

**修正箇所:** l_vog_lin_tm_int() - RCVDATA_WAITケース（1168-1188行目）

**変更前:**
```c
        /*** データ受信待ち状態 ***/
        case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            /* SLEEPコマンドIDで、フレーム定義なしの場合 */
            if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
                /* 何もせず Synch Break待ち状態へ */
                l_vol_lin_set_synchbreak();
            }
            else{
                /* No Responseエラー */
                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;

                l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
                l_vol_lin_set_synchbreak();
            }
            break;
```

**変更後:**
```c
        /*** データ受信待ち状態 ***/
        case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            /* SLEEPコマンドIDで、フレーム定義なしの場合 */
            if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
                /* 何もせず Synch Break待ち状態へ */
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

**ロジック:**
1. SLEEPコマンドID（0x3C）でフレーム定義がない場合は除外（既存の処理）
2. `u1l_lin_rs_cnt`（受信バイトカウンタ）をチェック
   - `> 0` → 1バイト以上受信済み → **Response Too Short**
   - `== 0` → 何も受信していない → **No Response**
3. どちらの場合もエラーありレスポンス完了処理を実行
4. Synch Break待ち状態へ復帰

---

## 動作シーケンス

### ケース1: No Response（1バイト目も受信できない）

```
時刻  イベント                          カウンタ状態
----  --------------------------------  --------------------
t=0   ID受信完了                        u1l_lin_rs_cnt = 0
      ↓ l_vog_lin_rcv_tm_set()         タイマー起動

      ★ データフィールド送信なし ★

t=X   タイマータイムアウト               u1l_lin_rs_cnt = 0 (まだ0)
      ↓ l_vog_lin_tm_int()
      ↓ u1l_lin_rs_cnt == 0 を検出

      No Responseエラーフラグセット
      ↓ xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_nores = SET
```

**タイムアウト時間:** (DL + 1) × 10 bit × 1.4
- DL=8の場合: 9 × 10 × 1.4 = 126 bit = 6.56 ms @19200bps

---

### ケース2: Response Too Short（1バイト目受信後、残りが受信できない）

```
時刻  イベント                          カウンタ状態
----  --------------------------------  --------------------
t=0   ID受信完了                        u1l_lin_rs_cnt = 0
      ↓ l_vog_lin_rcv_tm_set()         タイマー起動

t=1   Data Field 1バイト目受信          u1l_lin_rs_cnt = 1
      ↓ l_vog_lin_rx_int()
      ↓ u1l_lin_rs_cnt++

      ★ 2バイト目以降が送信されない ★

t=X   タイマータイムアウト               u1l_lin_rs_cnt = 1 (1以上)
      ↓ l_vog_lin_tm_int()
      ↓ u1l_lin_rs_cnt > 0 を検出

      Response Too Shortエラーフラグセット
      ↓ xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_tooshort = SET
```

**タイムアウト時間:** 同じく (DL + 1) × 10 bit × 1.4

---

## バイトカウンタ（u1l_lin_rs_cnt）の動作

### 受信処理での更新

**[l_slin_core_CC2340R53.c:1034-1050](l_slin_core_CC2340R53.c#L1034-L1050)**
```c
case( U1G_LIN_SLSTS_RCVDATA_WAIT ):
    /* データ受信 */
    if( u1l_lin_rs_cnt < u1l_lin_frm_sz ){
        u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] = u1a_lin_data;  /* データバッファに格納 */
        u2l_lin_chksum += (l_u16)u1a_lin_data;            /* チェックサム加算 */
        u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) +
                         ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );
        u1l_lin_rs_cnt++;                                 /* カウンタ増加 ★ */
    }
    // ...
```

**動作:**
- データバイト受信ごとに`u1l_lin_rs_cnt++`
- 初期値: 0
- 1バイト目受信後: 1
- 2バイト目受信後: 2
- ...

### カウンタ初期化

**[l_slin_core_CC2340R53.c:1000](l_slin_core_CC2340R53.c#L1000)**
```c
u1l_lin_rs_cnt = U1G_LIN_0;  /* 受信カウンタクリア */
```

**初期化タイミング:**
- ID受信完了時（受信フレーム開始時）

---

## H850との違い

### H850の実装

**[H850/slin_lib/l_slin_core_h83687.c:1147-1160](../H850/slin_lib/l_slin_core_h83687.c#L1147-L1160)**
```c
case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
    if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) &&
        (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
        l_vol_lin_set_synchbreak();
    }
    else{
        /* No Responseエラー (Response Too Shortと区別なし) */
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    break;
```

**H850の特徴:**
- バイトカウント判定なし
- 全て**No Response**として扱う
- LIN 1.3仕様ではこれで問題なし

### CC23xxの拡張

| 項目 | H850（LIN 1.3） | CC23xx（LIN 2.x） |
|-----|----------------|------------------|
| **エラー検出** | No Responseのみ | No Response + Response Too Short |
| **判定方法** | なし | バイトカウンタ判定 |
| **フラグ** | `u2g_lin_e_nores` | `u2g_lin_e_nores` + `u2g_lin_e_tooshort` |
| **仕様準拠** | LIN 1.3 | LIN 2.x |

---

## エラーハンドリング

### 上位層での利用方法

アプリケーション層では、以下のようにエラーを確認できます：

```c
/* エラー確認 */
l_u16 status = xng_lin_frm_buf[slot].un_state.u2g_lin_word;

if( xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_nores ){
    /* No Response エラー処理 */
    // スレーブが全く応答しない → スレーブ故障の可能性
}

if( xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_tooshort ){
    /* Response Too Short エラー処理 */
    // スレーブが途中まで応答 → 通信干渉、スレーブ異常などの可能性
}
```

### エラーの重大度

| エラー | 重大度 | 原因 | 対応 |
|-------|-------|------|------|
| **No Response** | 高 | スレーブ故障、配線断線、スリープ中 | スレーブ診断、配線チェック |
| **Response Too Short** | 中 | 通信干渉、スレーブ異常、タイミング不正 | 再送要求、通信環境確認 |

---

## テスト項目

### 1. No Response検出テスト

**テストケース:**
1. 受信フレームID受信後、データフィールドを一切送信しない
2. タイムアウト待機
3. **期待結果:**
   - `u2g_lin_e_nores` フラグがセット
   - `u2g_lin_e_tooshort` フラグはクリア
   - `u1l_lin_rs_cnt == 0`

### 2. Response Too Short検出テスト

**テストケース1: 1バイト目のみ受信**
1. 受信フレームID受信後、Data Field 1バイト目のみ送信
2. 2バイト目以降は送信しない
3. タイムアウト待機
4. **期待結果:**
   - `u2g_lin_e_tooshort` フラグがセット
   - `u2g_lin_e_nores` フラグはクリア
   - `u1l_lin_rs_cnt == 1`

**テストケース2: 途中まで受信**
1. 受信フレームID受信後、Data Field 3バイト送信（DL=8の場合）
2. 4バイト目以降は送信しない
3. タイムアウト待機
4. **期待結果:**
   - `u2g_lin_e_tooshort` フラグがセット
   - `u2g_lin_e_nores` フラグはクリア
   - `u1l_lin_rs_cnt == 3`

### 3. 正常受信テスト

**テストケース:**
1. 受信フレームID受信後、全データフィールド + チェックサム送信
2. **期待結果:**
   - エラーフラグなし
   - タイムアウト発生せず
   - 正常なレスポンス完了

### 4. SLEEPコマンドテスト

**テストケース:**
1. SLEEPコマンドID（0x3C）受信、フレーム定義なし
2. タイムアウト待機
3. **期待結果:**
   - エラーフラグなし
   - Synch Break待ち状態へ復帰

---

## 実装の利点

### 1. LIN 2.x規格準拠
- Response Too ShortとNo Responseを明確に区別
- 診断機能の向上

### 2. 詳細なエラー分類
- スレーブの故障パターンを細かく識別可能
- No Response: スレーブ完全停止
- Response Too Short: スレーブ部分故障

### 3. H850との互換性維持
- 既存のNo Response検出機能は維持
- Response Too Shortは追加機能として実装
- バイナリ互換性あり（ビットフィールド拡張のみ）

### 4. F24との機能等価性
- ハードウェアD1RCフラグ → ソフトウェアバイトカウンタ
- 同じエラー区別ロジックを実現

---

## まとめ

### 実装内容
1. **エラーフラグ追加:** `u2g_lin_e_tooshort` ビットフィールド
2. **タイマー割り込み拡張:** バイトカウンタ判定によるエラー区別
3. **LIN 2.x準拠:** Response Too Short検出機能の実装完了

### 変更箇所
- `l_slin_tbl.h`: エラーフラグ定義追加（リトル/ビッグエンディアン両対応）
- `l_slin_core_CC2340R53.c`: タイマー割り込みハンドラ修正

### テスト必須項目
- No Response検出（`u1l_lin_rs_cnt == 0`）
- Response Too Short検出（`u1l_lin_rs_cnt > 0`）
- 正常受信時のエラーフラグなし
- SLEEPコマンド除外処理

---

## 関連ドキュメント

- [LIN_1.3_vs_2.x_COMPARISON.md](../LIN_1.3_vs_2.x_COMPARISON.md) - LIN 1.3と2.xの比較
- [LIN_2X_ERROR_DETECTION_DESIGN.md](LIN_2X_ERROR_DETECTION_DESIGN.md) - LIN 2.xエラー検出設計
- [STATE_TRANSITION_ANALYSIS.md](STATE_TRANSITION_ANALYSIS.md) - 状態遷移解析
- [TIMER_FUNCTIONS_IMPLEMENTATION.md](TIMER_FUNCTIONS_IMPLEMENTATION.md) - タイマー関数実装

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Status:** ✅ 実装完了
