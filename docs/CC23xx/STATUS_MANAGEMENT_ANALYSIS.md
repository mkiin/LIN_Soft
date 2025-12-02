# LIN 2.0 ステータスマネジメント (Status Management) 実装状況

## 概要

LIN 2.0で追加されたステータスマネジメント機能について、CC2340R53実装の現状を分析します。ステータスマネジメントは、各スレーブノードがエラーを検知した際にマスターノードに報告する機能です。

**分析日**: 2025-12-01
**対象**: CC2340R53 LINスタック
**LIN仕様**: LIN 2.0 Status Management

---

## LIN 2.0 ステータスマネジメント仕様

### 要求事項

LIN 2.0仕様では、以下のステータスマネジメント機能が定義されています：

1. **response_errorシグナル**: 各スレーブノードは、ステータス管理フレーム（アンコンディショナルフレームの1つ）に1ビットの`response_error`シグナルを定義する

2. **自動エラー設定**: フレームのレスポンスでエラーが検出された場合、スレーブノードのドライバにより**自動的に**設定される

3. **自動クリア**: ステータス管理フレームの送信が完了すると、このシグナルは**自動的に**クリアされる

4. **エラー報告**: マスターノードはこのシグナルを読み取ることで、エラーの発生や原因となるノードを特定できる

### ステータス管理フレームの構造

```
Data Byte 1 (D1):
  bit 7: Sleep.ind
  bit 6: Wakeup.ind
  bit 5: Data.ind
  bit 4: response_error  ← ★ LIN 2.0で追加
  bit 3-0: (その他のNM情報)

Data Byte 2-8:
  (アプリケーション固有データ)
```

---

## CC2340R53実装の現状分析

### 1. エラーフラグ管理

#### フレームバッファのエラーフラグ

**定義箇所**: [l_slin_tbl.h:158-168](l_slin_tbl.h#L158-L168)

```c
typedef struct {
    un_lin_data_type    xng_lin_data;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_err:4;
            l_u16   reserve2:4;
        } st_err;
        struct {
            l_u16   u2g_lin_chksum:8;
            l_u16   u2g_lin_e_nores:1;      /* No Response エラー */
            l_u16   u2g_lin_e_uart:1;       /* UART エラー */
            l_u16   u2g_lin_e_bit:1;        /* BIT エラー */
            l_u16   u2g_lin_e_sum:1;        /* Checksum エラー */
            l_u16   u2g_lin_e_tooshort:1;   /* Response Too Short エラー (LIN 2.x) */
            l_u16   reserve:2;
            l_u16   u2g_lin_no_use:1;
        } st_bit;
    } un_state;
} st_lin_buf_type;
```

**実装状況**:
- ✅ 各フレームごとにエラーフラグを保持
- ✅ LIN 2.x対応エラー（Response Too Short）を実装済み
- ✅ 複数種類のエラーを個別に記録可能

#### エラー自動設定の実装

**No Response エラー**: [l_slin_core_CC2340R53.c:1188](l_slin_core_CC2340R53.c#L1188)
```c
/* データ受信待ち状態でタイムアウト */
xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
```

**BIT エラー**: [l_slin_core_CC2340R53.c:1200](l_slin_core_CC2340R53.c#L1200)
```c
/* リードバック失敗 */
xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
```

**Checksum エラー**: [l_slin_core_CC2340R53.c:991](l_slin_core_CC2340R53.c#L991)
```c
/* チェックサム不一致 */
xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_sum = U2G_LIN_BIT_SET;
```

**Response Too Short エラー**: [l_slin_core_CC2340R53.c:1177](l_slin_core_CC2340R53.c#L1177)
```c
/* 1バイト目受信後タイムアウト */
xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_tooshort = U2G_LIN_BIT_SET;
```

**実装状況**:
- ✅ エラー検出時に**自動的に**フラグ設定
- ✅ LINドライバ層（CORE層）で実装
- ✅ アプリケーション介入不要

### 2. NM (Network Management) 層の実装

#### NM情報管理

**実装箇所**: [l_slin_nmc.c:286-301](l_slin_nmc.c#L286-L301)

```c
/* LINライブラリのNM情報書換え処理 */
u1a_lin_tmp_nm_dat = U1G_LIN_BYTE_CLR;

if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_ON ) {
    /* Sleep.indビットのセット */
    u1a_lin_tmp_nm_dat |= U1L_LIN_SLPIND_SET;  // bit 7
}
if( u1l_lin_wup_ind == U1L_LIN_FLG_ON ) {
    /* Wakeup.indビットのセット */
    u1a_lin_tmp_nm_dat |= U1L_LIN_WUPIND_SET;  // bit 6
}
if( u1l_lin_data_ind == U1L_LIN_FLG_ON ) {
    /* Data.indビットのセット */
    u1a_lin_tmp_nm_dat |= U1L_LIN_DATIND_SET;  // bit 5
}

/* NM情報書換え関数のコール */
l_vog_lin_set_nm_info(u1a_lin_tmp_nm_dat);
```

**定義**: [l_slin_nmc.c:21-23](l_slin_nmc.c#L21-L23)
```c
#define U1L_LIN_SLPIND_SET   ((l_u8)0x80)    /* SLEEP_INDビット (bit 7) */
#define U1L_LIN_WUPIND_SET   ((l_u8)0x40)    /* WAKEUP_INDビット (bit 6) */
#define U1L_LIN_DATIND_SET   ((l_u8)0x20)    /* DATA_INDビット (bit 5) */
```

**実装状況**:
- ✅ Sleep.ind, Wakeup.ind, Data.indは実装済み
- ❌ **response_error (bit 4) は未実装**

#### ステータス管理フレーム送信時の処理

**送信時のNM情報セット**: [l_slin_core_CC2340R53.c:893-903](l_slin_core_CC2340R53.c#L893-L903)

```c
/* NM使用設定フレームの場合 */
if( xng_lin_slot_tbl[ slot ].u1g_lin_nm_use == U1G_LIN_NM_USE )
{
    /* LINフレームバッファのNM部分(データ1のbit4-7)をクリア */
    xng_lin_frm_buf[ slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
        &= U1G_LIN_BUF_NM_CLR_MASK;  // 0x0F (bit 0-3のみ残す)

    /* LINフレームに レスポンス送信ステータスをセット */
    xng_lin_frm_buf[ slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
        |= ( u1g_lin_nm_info & U1G_LIN_NM_INFO_MASK );  // 0xF0 (bit 4-7)
}
```

**マスク定義**: [l_slin_core_CC2340R53.h:57-58](l_slin_core_CC2340R53.h#L57-L58)
```c
#define  U1G_LIN_BUF_NM_CLR_MASK   ((l_u8)0x0F)  /* bit 4-7クリア */
#define  U1G_LIN_NM_INFO_MASK      ((l_u8)0xF0)  /* bit 4-7マスク */
```

**実装状況**:
- ✅ NM使用フレーム判定機能あり
- ✅ Data1のbit 4-7にNM情報をセット
- ✅ 送信前に自動的にNM情報を反映
- ⚠️ **bit 4 (response_error) の自動設定が欠落**

### 3. エラークリア機能

**正常完了時のエラークリア**: [l_slin_core_CC2340R53.c:1323](l_slin_core_CC2340R53.c#L1323)

```c
/* 正常完了の場合 */
else {
    xng_lin_bus_sts.st_bit.u2g_lin_ok_resp = U2G_LIN_BIT_SET;
    xng_lin_frm_buf[ slot ].un_state.st_err.u2g_lin_err = U2G_LIN_BYTE_CLR;  // ← エラーフラグクリア
}
```

**実装状況**:
- ✅ 正常完了時に**自動的に**エラーフラグをクリア
- ⚠️ **ステータス管理フレーム送信完了時の自動クリア機能が欠落**

---

## 実装完了 ✅

### 1. response_errorシグナルの自動設定

**実装箇所**: [l_slin_nmc.c:290-308](l_slin_nmc.c#L290-L308)

**実装内容**:

```c
/* response_error シグナルの集約 (LIN 2.0 Status Management) */
/* 全フレームのエラーフラグをチェックし、いずれかでエラーがあればresponse_errorをセット */
{
    l_u8 u1a_slot;
    l_bool u1a_has_response_error = U2G_LIN_NG;

    for( u1a_slot = U1G_LIN_0; u1a_slot < U1G_LIN_MAX_SLOT; u1a_slot++ ) {
        /* フレームバッファのエラーフラグ(4bit)をチェック */
        if( xng_lin_frm_buf[ u1a_slot ].un_state.st_err.u2g_lin_err != U2G_LIN_BYTE_CLR ) {
            u1a_has_response_error = U2G_LIN_OK;
            break;
        }
    }

    /* response_errorビットのセット (bit 4) */
    if( u1a_has_response_error == U2G_LIN_OK ) {
        u1a_lin_tmp_nm_dat |= U1L_LIN_RESPERR_SET;
    }
}
```

**定数定義**: [l_slin_nmc.c:24](l_slin_nmc.c#L24)
```c
#define U1L_LIN_RESPERR_SET  ((l_u8)0x10)  /* RESPONSE_ERRORビットのセット用 (bit 4) LIN 2.0 */
```

**動作説明**:
- `l_nm_tick_ch1()` 関数内でNM情報を更新する際に実行
- 全フレーム（`U1G_LIN_MAX_SLOT`）のエラーフラグをスキャン
- いずれか1つでもエラーが検出されていれば `response_error` (bit 4) をセット
- エラー種別（No Response / BIT / Checksum / Too Short）は問わず、エラーの有無のみ判定

### 2. ステータス管理フレーム送信完了時の自動クリア

**実装箇所**: [l_slin_core_CC2340R53.c:1339-1350](l_slin_core_CC2340R53.c#L1339-L1350)

**実装内容**:

```c
/* ステータス管理フレーム送信完了時の自動クリア (LIN 2.0 Status Management) */
/* NM使用設定フレームで、送信フレームで、正常完了の場合 */
if( (xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_nm_use == U1G_LIN_NM_USE) &&
    (xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND) &&
    (u1a_lin_err == U1G_LIN_ERR_OFF) ) {

    /* 全フレームのエラーフラグをクリア（LIN 2.0仕様: response_errorの自動クリア） */
    l_u8 u1a_slot;
    for( u1a_slot = U1G_LIN_0; u1a_slot < U1G_LIN_MAX_SLOT; u1a_slot++ ) {
        xng_lin_frm_buf[ u1a_slot ].un_state.st_err.u2g_lin_err = U2G_LIN_BYTE_CLR;
    }
}
```

**動作説明**:
- `l_vol_lin_set_frm_complete()` 関数内でフレーム送信完了時に実行
- 条件1: `u1g_lin_nm_use == U1G_LIN_NM_USE` - ステータス管理フレームである
- 条件2: `u1g_lin_sndrcv == U1G_LIN_CMD_SND` - 送信フレームである（受信フレームではない）
- 条件3: `u1a_lin_err == U1G_LIN_ERR_OFF` - 正常完了した
- 3条件を満たす場合、全フレームのエラーフラグを一括クリア
- これによりマスターノードへのエラー報告後、自動的に`response_error`がクリアされる

---

## 実装推奨事項

### オプション1: 完全なLIN 2.0準拠（推奨）

**メリット**:
- ✅ LIN 2.0仕様に完全準拠
- ✅ マスターノードがエラー発生を検知可能
- ✅ 診断機能の向上

**実装内容**:
1. `l_slin_nmc.c`に`response_error`シグナル設定機能を追加
2. `l_vol_lin_set_frm_complete()`にステータス管理フレーム送信完了時の自動クリア機能を追加
3. 定数定義を追加:
   ```c
   #define U1L_LIN_RESPERR_SET  ((l_u8)0x10)  /* response_error (bit 4) */
   ```

### オプション2: 最小限実装（現状維持）

**メリット**:
- ✅ 実装がシンプル
- ✅ 既存コードへの影響最小

**デメリット**:
- ❌ LIN 2.0ステータスマネジメント非対応
- ❌ マスターノードがエラーを検知できない

**現状の動作**:
- エラーフラグは各フレームバッファに記録される
- アプリケーション層で`l_ifc_read_status_ch1()`を使用してエラーを確認可能
- ただし、マスターノードへの自動報告はされない

---

## 他プラットフォームとの比較

### H850実装

**分析結果**: H850も同様に`response_error`シグナルの自動設定機能は**未実装**

**理由推定**:
- トヨタ自動車殿標準LIN仕様（独自仕様）ベース
- LIN 2.0の一部機能のみ採用
- ステータスマネジメントはアプリケーション層で実装する設計思想

### F24実装

**分析結果**: F24も同様に基本的なNM機能のみ実装

**共通点**:
- Sleep.ind, Wakeup.ind, Data.indは実装済み
- `response_error`は未実装

---

## まとめ

### 現状

| 機能 | 実装状況 | 備考 |
|-----|---------|------|
| **エラーフラグ管理** | ✅ 実装済み | 各フレームバッファに記録 |
| **エラー自動設定** | ✅ 実装済み | LINドライバが自動設定 |
| **NM基本機能** | ✅ 実装済み | Sleep/Wakeup/Data indication |
| **response_error設定** | ❌ 未実装 | bit 4への反映なし |
| **自動クリア** | ⚠️ 部分実装 | 正常完了時のみ、ステータス管理フレーム送信完了時は未対応 |

### 結論

CC2340R53のLINスタックは、**基本的なエラー検出機能は完全に実装済み**ですが、**LIN 2.0ステータスマネジメント（response_errorシグナル）は未実装**です。

**対応の必要性**:
- **LIN 2.0完全準拠が必要な場合**: 上記「オプション1」の実装が必要
- **既存の互換性維持が優先の場合**: 現状維持でも動作可能（エラーは個別に確認できる）

---

## 参考資料

### 内部ドキュメント
- [l_slin_nmc.c](l_slin_nmc.c) - ネットワーク管理コンポーネント
- [l_slin_core_CC2340R53.c](l_slin_core_CC2340R53.c) - エラー検出実装
- [l_slin_tbl.h](l_slin_tbl.h) - エラーフラグ定義

### LIN仕様
- LIN 2.0 Specification - Status Management
- LIN 2.2a Specification - Network Management

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Status:** ✅ 分析完了
