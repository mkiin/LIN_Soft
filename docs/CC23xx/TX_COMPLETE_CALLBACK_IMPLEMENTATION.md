# 送信完了コールバック実装 (TxComplete Callback)

## 概要

旧実装の`AFTER_SNDDATA_WAIT`状態で実行していた送信完了コールバック処理を、H850互換の実装に統合しました。`AFTER_SNDDATA_WAIT`状態は不要であり、代わりに`l_vol_lin_set_frm_complete()`関数内でコールバックを呼び出します。

**実装日**: 2025-12-01
**対象**: CC2340R53 LINスタック
**目的**: 継続フレーム管理のための送信完了通知

---

## 背景

### 問題点

旧実装では、`AFTER_SNDDATA_WAIT`という専用の状態で送信完了後の処理を行っていました：

```c
/*** データ送信後状態 ***/
case( U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT ):
#if 1   /* 継続フレーム送信時に送信完了フラグで誤検知している可能性があり、
           期待するフレームが送信されたかを確認するため、LIN通信管理へのコールバックを実行する */
    f_LIN_Manager_Callback_TxComplete( xnl_lin_id_sl.u1g_lin_slot, u1l_lin_frm_sz, u1l_lin_rs_tmp );
#endif
    l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp, u1l_lin_frm_sz + U1G_LIN_1);
    /* リードバック失敗 */
    if( l_u1a_lin_read_back != U1G_LIN_OK ) {
        /* BITエラー */
        xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
        l_vol_lin_set_synchbreak();
    }
    /* リードバック成功 */
    else {
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );
        l_vol_lin_set_synchbreak();
    }
    break;
```

**問題**:
- H850には`AFTER_SNDDATA_WAIT`状態が存在しない
- H850互換実装に移行する際、この状態を削除したためコールバックが呼ばれなくなった
- 継続フレーム管理が機能しない

### コールバックの目的

`f_LIN_Manager_Callback_TxComplete()`は以下の目的で使用されます：

1. **継続フレーム管理**: 分割送信されたLINフレームの次フレームをキューから取り出して送信
2. **送信データ検証**: 実際に送信されたデータが期待通りか確認
3. **送信完了フラグ設定**: 全フレーム送信完了時にフラグを立てる

**LIN_Manager.c実装** ([LIN_Manager.c:537-570](LIN_Manager.c#L537-L570)):

```c
void f_LIN_Manager_Callback_TxComplete( u1 u1a_frame, u1 u1a_DataLength, u1* u1a_Data )
{
    u1 u1a_HasTxDataQueue;

    if ( u1a_frame == U1G_LIN_FRAME_ID30H )
    {
        /* 送信データと実際のデータを比較 */
        if ( ( u1a_Data[0] == u1l_LIN_Manager_TxData[0] )
          && ( u1a_Data[1] == u1l_LIN_Manager_TxData[1] )
          && ( u1a_Data[2] == u1l_LIN_Manager_TxData[2] )
          && ( u1a_Data[3] == u1l_LIN_Manager_TxData[3] )
          && ( u1a_Data[4] == u1l_LIN_Manager_TxData[4] )
          && ( u1a_Data[5] == u1l_LIN_Manager_TxData[5] )
          && ( u1a_Data[6] == u1l_LIN_Manager_TxData[6] )
          && ( u1a_Data[7] == u1l_LIN_Manager_TxData[7] ) )
        {
            /* 送信データキューがある場合 → 次フレーム送信 */
            u1a_HasTxDataQueue = f_LIN_Manager_HasTxDataQueue();
            if ( u1a_HasTxDataQueue == U1G_DAT_TRUE )
            {
                f_LIN_Manager_DequeueTxData( &u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
                l_slot_wr_ch1( U1G_LIN_FRAME_ID30H, u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
            }
            /* 送信データキューがない場合 → 完了フラグ設定 */
            else
            {
                u1l_LIN_Manager_TxFrameComplete_ID30h = U1G_DAT_TRUE;
            }
        }
        else
        {
            /* 異常: 送信データ不一致 */
        }
    }
}
```

---

## 解決策

### 1. H850の実装を確認

H850には`AFTER_SNDDATA_WAIT`状態が存在しません。送信完了は`SNDDATA_WAIT`状態のタイマー割り込みハンドラで処理されます。

**H850の送信完了処理** ([H850/slin_lib/l_slin_core_h83687.c:1189-1213](../H850/slin_lib/l_slin_core_h83687.c#L1189-L1213)):

```c
case( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    if( u1l_lin_rs_cnt > U1G_LIN_0 ){
        l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );

        if( l_u1a_lin_read_back != U1G_LIN_OK ){
            /* BITエラー */
            xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
            l_vol_lin_set_synchbreak();
        }
        else{
            /* 最後まで送信 */
            if( u1l_lin_rs_cnt > u1l_lin_frm_sz ){
                l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );  // ← ここで完了通知
                l_vol_lin_set_synchbreak();
            }
            /* まだ全てのデータを送信完了していない場合 */
            else{
                // 次バイト送信
            }
        }
    }
    break;
```

**ポイント**: H850でも送信完了時に`l_vol_lin_set_frm_complete()`を呼び出します。

### 2. CC23xxの実装方針

H850互換を維持しつつ、コールバック機能を追加するため、**`l_vol_lin_set_frm_complete()`関数内にコールバック呼び出しを追加**します。

**理由**:
1. ✅ H850互換性を維持（状態遷移は変更しない）
2. ✅ 送信完了時に必ず呼ばれる（受信完了時も呼ばれるが送信フレーム判定で除外）
3. ✅ 実装がシンプル（新しい状態を追加しない）

---

## 実装内容

### 修正ファイル

**ファイル**: [l_slin_core_CC2340R53.c:1310-1355](l_slin_core_CC2340R53.c#L1310-L1355)

### 修正前

```c
static void  l_vol_lin_set_frm_complete(l_u8  u1a_lin_err)
{
    /* 転送成功、もしくは転送エラーのフラグが立っている場合(オーバーラン) */
    if( (xng_lin_bus_sts.u2g_lin_word & U2G_LIN_BUS_STS_CMP_SET) != U2G_LIN_WORD_CLR ){
        xng_lin_bus_sts.st_bit.u2g_lin_ovr_run = U2G_LIN_BIT_SET;
    }
    /* 異常完了の場合 */
    if ( u1a_lin_err == U1G_LIN_ERR_ON ) {
        xng_lin_bus_sts.st_bit.u2g_lin_err_resp = U2G_LIN_BIT_SET;
    }
    /* 正常完了の場合 */
    else {
        xng_lin_bus_sts.st_bit.u2g_lin_ok_resp = U2G_LIN_BIT_SET;
        xng_lin_frm_buf[ slot ].un_state.st_err.u2g_lin_err = U2G_LIN_BYTE_CLR;
    }

    /* 保護IDのセット */
    xng_lin_bus_sts.st_bit.u2g_lin_last_id = (l_u16)u1g_lin_protid_tbl[ xnl_lin_id_sl.u1g_lin_id ];

    /* 送受信処理完了フラグのセット */
    if( xnl_lin_id_sl.u1g_lin_slot < U1G_LIN_16 ){
        xng_lin_sts_buf.un_rs_flg1.u2g_lin_word |= u2g_lin_flg_set_tbl[ slot ];
    }
    // ... (以下省略)
}
```

### 修正後

```c
static void  l_vol_lin_set_frm_complete(l_u8  u1a_lin_err)
{
    /* 転送成功、もしくは転送エラーのフラグが立っている場合(オーバーラン) */
    if( (xng_lin_bus_sts.u2g_lin_word & U2G_LIN_BUS_STS_CMP_SET) != U2G_LIN_WORD_CLR ){
        xng_lin_bus_sts.st_bit.u2g_lin_ovr_run = U2G_LIN_BIT_SET;
    }
    /* 異常完了の場合 */
    if ( u1a_lin_err == U1G_LIN_ERR_ON ) {
        xng_lin_bus_sts.st_bit.u2g_lin_err_resp = U2G_LIN_BIT_SET;
    }
    /* 正常完了の場合 */
    else {
        xng_lin_bus_sts.st_bit.u2g_lin_ok_resp = U2G_LIN_BIT_SET;
        xng_lin_frm_buf[ slot ].un_state.st_err.u2g_lin_err = U2G_LIN_BYTE_CLR;
    }

    /* 保護IDのセット */
    xng_lin_bus_sts.st_bit.u2g_lin_last_id = (l_u16)u1g_lin_protid_tbl[ xnl_lin_id_sl.u1g_lin_id ];

    /* ★ 送信フレームの場合、LIN_Managerへコールバック通知 */
    /* 継続フレーム送信時に送信完了フラグで誤検知している可能性があり、 */
    /* 期待するフレームが送信されたかを確認するため、LIN通信管理へのコールバックを実行する */
    if( (xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND) &&
        (u1a_lin_err == U1G_LIN_ERR_OFF) ) {
        /* 送信成功時のみコールバック */
        f_LIN_Manager_Callback_TxComplete( xnl_lin_id_sl.u1g_lin_slot, u1l_lin_frm_sz, u1l_lin_rs_tmp );
    }

    /* 送受信処理完了フラグのセット */
    if( xnl_lin_id_sl.u1g_lin_slot < U1G_LIN_16 ){
        xng_lin_sts_buf.un_rs_flg1.u2g_lin_word |= u2g_lin_flg_set_tbl[ slot ];
    }
    // ... (以下省略)
}
```

### 追加部分の詳細

```c
/* 送信フレームの場合、LIN_Managerへコールバック通知 */
/* 継続フレーム送信時に送信完了フラグで誤検知している可能性があり、 */
/* 期待するフレームが送信されたかを確認するため、LIN通信管理へのコールバックを実行する */
if( (xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND) &&
    (u1a_lin_err == U1G_LIN_ERR_OFF) ) {
    /* 送信成功時のみコールバック */
    f_LIN_Manager_Callback_TxComplete( xnl_lin_id_sl.u1g_lin_slot, u1l_lin_frm_sz, u1l_lin_rs_tmp );
}
```

**条件**:
1. **送信フレーム判定**: `u1g_lin_sndrcv == U1G_LIN_CMD_SND`
   - 受信フレーム完了時は呼ばれない
2. **送信成功時のみ**: `u1a_lin_err == U1G_LIN_ERR_OFF`
   - BITエラーなど異常完了時は呼ばれない

**引数**:
- `xnl_lin_id_sl.u1g_lin_slot`: フレームID（スロット番号）
- `u1l_lin_frm_sz`: データ長（DL）
- `u1l_lin_rs_tmp`: 送信データバッファ（Data1～DataDL + Checksum）

---

## 動作フロー

### 送信完了シーケンス（継続フレームあり）

```
[Master] → [Slave]
    |
    |-- Header送信（ID30H）
    |        ↓
    |   [PID受信]
    |   [送信判定]
    |   [Response Space待ち]
    |        ↓
    |-- Data1～DataDL, Checksum受信
    |        ↓
    |   [リードバック確認]
    |   [u1l_lin_rs_cnt > DL]
    |        ↓
    |   l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF )
    |        ↓
    |   ★ f_LIN_Manager_Callback_TxComplete( slot, DL, u1l_lin_rs_tmp )
    |        ↓
    |   [送信データ比較]
    |   [キュー確認]
    |        ↓
    |   [次フレームあり？]
    |    YES → l_slot_wr_ch1( ID30H, DL, NextData )  // 次フレーム送信準備
    |    NO  → u1l_LIN_Manager_TxFrameComplete_ID30h = TRUE  // 完了フラグ
```

### タイミングチャート

```
LIN Bus:  [Header ID30H][RS][Data1][IBS]...[DataDL][IBS][Checksum]
              ↓          ↓                                     ↓
状態:     ID_WAIT    SNDDATA_WAIT ------------------------> BREAK_WAIT
                         ↓ (各バイトタイマー)                ↓
リードバック:          Data1 OK, Data2 OK, ... Checksum OK   完了判定
                                                              ↓
関数呼び出し:                                    l_vol_lin_set_frm_complete(ERR_OFF)
                                                              ↓
                                                 f_LIN_Manager_Callback_TxComplete()
                                                              ↓
継続フレーム処理:                                     [キュー確認]
                                                              ↓
                                                    YES: l_slot_wr_ch1()
                                                    NO:  完了フラグ設定
```

---

## 他実装との比較

### 旧実装（AFTER_SNDDATA_WAIT使用）

| 項目 | 内容 |
|-----|------|
| **状態数** | 7状態（AFTER_SNDDATA_WAIT含む） |
| **コールバック呼び出し位置** | AFTER_SNDDATA_WAIT状態 |
| **リードバック** | AFTER_SNDDATA_WAIT状態で全バイト一括確認 |
| **H850互換性** | ❌ 非互換（H850には存在しない状態） |

### 新実装（本実装）

| 項目 | 内容 |
|-----|------|
| **状態数** | 6状態（H850互換） |
| **コールバック呼び出し位置** | `l_vol_lin_set_frm_complete()`内 |
| **リードバック** | SNDDATA_WAIT状態で1バイトずつ確認 |
| **H850互換性** | ✅ 完全互換 |

### H850実装

| 項目 | 内容 |
|-----|------|
| **状態数** | 6状態 |
| **コールバック呼び出し位置** | なし（LIN_Managerなし） |
| **リードバック** | SNDDATA_WAIT状態で1バイトずつ確認 |

---

## テスト項目

### 1. 単発フレーム送信

**テストケース**: 1フレームのみ送信（継続フレームなし）

**期待結果**:
1. フレーム送信成功
2. `f_LIN_Manager_Callback_TxComplete()`が呼ばれる
3. キューが空のため、`u1l_LIN_Manager_TxFrameComplete_ID30h = TRUE`
4. 次のマスター要求まで待機

### 2. 継続フレーム送信

**テストケース**: 3フレーム連続送信（SOF → CONTINUATION → EOF）

**期待結果**:
1. **フレーム1送信完了**:
   - コールバック呼び出し
   - キュー確認 → 次フレームあり
   - `l_slot_wr_ch1()`で次フレーム設定
2. **フレーム2送信完了**:
   - コールバック呼び出し
   - キュー確認 → 次フレームあり
   - `l_slot_wr_ch1()`で次フレーム設定
3. **フレーム3送信完了**:
   - コールバック呼び出し
   - キュー確認 → 次フレームなし
   - 完了フラグ設定

### 3. 送信エラー発生

**テストケース**: BITエラー発生（リードバック失敗）

**期待結果**:
1. リードバック失敗検出
2. `l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON )`呼び出し
3. **コールバックは呼ばれない**（`u1a_lin_err == U1G_LIN_ERR_OFF`条件不成立）
4. エラーフラグ設定
5. Break待ちへ復帰

### 4. 受信フレーム完了

**テストケース**: 受信フレーム（ID20H）完了

**期待結果**:
1. 受信データ取得
2. `l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF )`呼び出し
3. **コールバックは呼ばれない**（`u1g_lin_sndrcv != U1G_LIN_CMD_SND`条件不成立）
4. 受信完了フラグ設定

---

## まとめ

### 実装内容

✅ **H850互換性を維持**: 状態遷移は変更せず、H850と同じ6状態
✅ **コールバック機能追加**: `l_vol_lin_set_frm_complete()`内で呼び出し
✅ **送信フレームのみ対応**: 受信完了時はコールバック呼ばれない
✅ **エラー時は呼ばれない**: 送信成功時のみコールバック実行

### 旧実装からの変更点

| 項目 | 旧実装 | 新実装 |
|-----|-------|-------|
| **AFTER_SNDDATA_WAIT状態** | 使用 | **削除**（H850互換） |
| **コールバック位置** | 専用状態 | **`l_vol_lin_set_frm_complete()`内** |
| **リードバック** | 全バイト一括 | 1バイトずつ（H850互換） |

### 継続フレーム管理

- ✅ 送信完了時に自動的に次フレーム送信
- ✅ キューが空の場合は完了フラグ設定
- ✅ 送信データ検証（期待値と実際の送信データを比較）

---

## 関連ドキュメント

- [HEADER_TO_RESPONSE_SEQUENCE.md](HEADER_TO_RESPONSE_SEQUENCE.md) - 送信シーケンス詳細
- [STATE_TRANSITION_ANALYSIS.md](STATE_TRANSITION_ANALYSIS.md) - 状態遷移解析
- [LIN_Manager.c](LIN_Manager.c) - LINフレーム分割・結合管理

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Status:** ✅ 実装完了
