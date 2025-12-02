# LIN 1.3 vs LIN 2.x プラットフォーム比較分析

## 目次

1. [概要](#概要)
2. [プラットフォーム比較表](#プラットフォーム比較表)
3. [レスポンススペース（Response Space）の実装比較](#レスポンススペースresponse-spaceの実装比較)
4. [インターバイトスペース（Inter-byte Space）の実装比較](#インターバイトスペースinter-byte-spaceの実装比較)
5. [エラー処理の違い](#エラー処理の違い)
6. [タイムアウト処理の違い](#タイムアウト処理の違い)
7. [CC23xx向けの実装推奨事項](#cc23xx向けの実装推奨事項)
8. [まとめ](#まとめ)

---

## 概要

本ドキュメントでは、以下の3つのプラットフォームにおけるLIN通信実装を比較し、LIN 1.3系からLIN 2.x系への移行に必要な機能を抽出します。

| プラットフォーム | MCU | LIN仕様 | ハードウェア実装 |
|-----------------|-----|---------|-----------------|
| **H850** | H8/3687 | LIN 1.3 | UART（ソフトウェア実装） |
| **F24** | RL78/F24 | LIN 2.x | 専用LINモジュール（ハードウェア） |
| **CC23xx** | CC2340R53 | LIN 1.3ベース | UART（ソフトウェア実装） |

**目的:**
- H850のアーキテクチャを尊重しつつ、LIN 2.xの機能をCC23xxに追加する
- F24の専用LINモジュール機能をソフトウェアで実現する方法を明確にする

---

## プラットフォーム比較表

### 基本仕様比較

| 項目 | F24（LIN 2.x） | H850（LIN 1.3） | CC23xx（現状） |
|------|---------------|----------------|---------------|
| **LINバージョン** | LIN 2.x | LIN 1.3 | LIN 1.3ベース |
| **ハードウェア実装** | 専用LINモジュール | UART | UART2 |
| **ボーレート** | 9600/10400/10417/19200 bps | 9600 bps | 2400/9600/19200 bps |
| **Auto Baudrate** | サポート（オプション） | 非サポート | 非サポート |
| **Break検出** | ハードウェア（11ビット/9.5ビット） | ソフトウェア | ソフトウェア |
| **レスポンススペース** | ハードウェア制御（LSCレジスタ） | ソフトウェアタイマー | ソフトウェアタイマー |
| **インターバイトスペース** | ハードウェア制御（LSCレジスタ） | ソフトウェアタイマー | ソフトウェアタイマー |
| **チェックサム方式** | Classic/Enhanced（設定可能） | Classic | Classic |

### レジスタ/タイマー実装比較

| 機能 | F24（LIN 2.x） | H850（LIN 1.3） | CC23xx（現状） |
|------|---------------|----------------|---------------|
| **Response Space設定** | `LSC.RS[2:0]` (0-7ビット) | タイマー設定 `l_vog_lin_bit_tm_set(U1G_LIN_RSSP)` | タイマー設定（未実装） |
| **Inter-byte Space設定** | `LSC.IBS[5:4]` (0-3ビット) | タイマー設定 `U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP` | タイマー設定（未実装） |
| **タイマー種類** | TAUモジュール（16ビット） | Timer Z0（16ビット） | LGPTimer（16/24/32ビット） |
| **タイマー精度** | fCLK/4分周（40MHz） | φ/8（12MHz） | 48MHz（分周可能） |

### エラー検出機能比較

| エラー種類 | F24（LIN 2.x） | H850（LIN 1.3） | CC23xx（現状） |
|-----------|---------------|----------------|---------------|
| **Framing Error** | ハードウェア検出（LEDEレジスタ） | UART SSRレジスタ | UART RSR_ECRレジスタ |
| **Overrun Error** | ハードウェア検出 | UART SSRレジスタ | UART RSR_ECRレジスタ |
| **Parity Error** | ハードウェア検出 | UART SSRレジスタ | UART RSR_ECRレジスタ |
| **Checksum Error** | ハードウェア検出 | ソフトウェア計算 | ソフトウェア計算 |
| **Response Too Short** | ハードウェア検出（D1RC） | タイマー検出 | タイマー検出（未実装） |
| **No Response** | ハードウェア検出 | タイマー検出 | タイマー検出（未実装） |
| **Header Timeout** | タイマー検出 | タイマー検出 | タイマー検出（未実装） |
| **Physical Bus Error** | タイマー検出 | タイマー検出（連続3回） | タイマー検出（未実装） |

---

## レスポンススペース（Response Space）の実装比較

### LIN 2.x仕様の要件

**LIN仕様書より:**
- レスポンススペースは、Protected IDとレスポンスフィールドの間に挿入される
- スレーブがデータ準備をするための時間
- 設定範囲: 0～7ビット長（LIN 2.xでは1ビットが推奨）

### F24のハードウェア実装

**[F24/l_slin_def.h:18](F24/l_slin_def.h#L18)**
```c
#define  U1G_LIN_RSSP               ((l_u8)1U)   /* Response space */
```

**[F24/l_slin_drv_rl78f24.h:93-94](F24/l_slin_drv_rl78f24.h#L93-L94)**
```c
/* LSCレジスタ */
#define  U1G_LIN_LSC_RS     ((l_u8)(U1G_LIN_RSSP & (l_u8)0x07U))  /* レスポンススペース設定値 RS b2-b0*/
```

**[F24/l_slin_drv_rl78f24.c:94](F24/l_slin_drv_rl78f24.c#L94)**
```c
U1G_LIN_SFR_LSC  = U1G_LIN_LSC_INIT;  /* LINスペース設定（RS、BS） */
```

**ハードウェアレジスタ設定:**
- `LSC.RS[2:0]`: レスポンススペース設定（0～7ビット）
- LINモジュールが自動的にレスポンススペースを挿入
- ソフトウェアでのタイミング制御は不要

### H850のソフトウェア実装

**[H850/slin_lib/l_slin_def.h:35](H850/slin_lib/l_slin_def.h#L35)**
```c
#define  U1G_LIN_RSSP               ((l_u8)1)    /* Response space */
```

**[H850/slin_lib/l_slin_core_h83687.c:944](H850/slin_lib/l_slin_core_h83687.c#L944)**
```c
/* 送信フレーム時 - Protected ID受信後 */
l_vog_lin_bit_tm_set( U1G_LIN_RSSP );  /* レスポンススペース待ちタイマセット */

u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;  /* データ送信待ち状態 */
```

**ソフトウェアタイマー実装:**
1. Protected ID受信完了時にタイマー設定
2. `U1G_LIN_RSSP`（1ビット）分のタイマーを起動
3. タイマー割り込みで送信処理開始

**タイマー計算:**
```c
/* H850 - l_slin_drv_h83687.c:436-459 */
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    l_u16 u2a_lin_tmp_bit = (l_u16)u1a_lin_bit;

    /* タイマカウンタ、GRAの設定 */
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマカウント動作開始 */
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_SET;
}
```

### CC23xxでの必要な実装

**[CC23xx/l_slin_def.h:17](CC23xx/l_slin_def.h#L17)**
```c
#define  U1G_LIN_RSSP               ((l_u8)1)    /* Response space */
```

**現状の問題:**
- レスポンススペースのタイマー設定が未実装
- H850と同じ実装方式を採用すべき

**推奨実装（DRV層）:**

```c
/**
 * @brief ビットタイマ設定（レスポンススペース用）
 * @param u1a_lin_bit タイマ設定値（ビット長）
 */
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    uint32_t counterTarget;
    uint32_t timeout_us;

    /* タイマー停止 */
    LGPTimerLPF3_stop(xnl_lin_timer_handle);

    /* ビット長をマイクロ秒に変換 */
    /* @9600bps: 1ビット = 104.17us */
    /* @19200bps: 1ビット = 52.08us */
    #if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
        timeout_us = ((uint32_t)u1a_lin_bit * 1041667UL) / 10000UL;
    #elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
        timeout_us = ((uint32_t)u1a_lin_bit * 520833UL) / 10000UL;
    #endif

    /* カウンタターゲット計算（48MHz = 48カウント/us） */
    counterTarget = (timeout_us * 48UL);
    if (counterTarget > 0) {
        counterTarget -= 1UL;
    }

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, counterTarget, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

**推奨実装（CORE層）:**

```c
/* 送信フレーム時 - Protected ID受信後 */
/* H850と同じタイミングでレスポンススペースタイマー設定 */
l_vog_lin_bit_tm_set( U1G_LIN_RSSP );  /* レスポンススペース待ちタイマセット */

u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;  /* データ送信待ち状態 */
```

---

## インターバイトスペース（Inter-byte Space）の実装比較

### LIN 2.x仕様の要件

**LIN仕様書より:**
- インターバイトスペースは、レスポンスフィールド内のバイト間に挿入される
- 設定範囲: 0～3ビット長（LIN 2.xでは1ビットが推奨）

### F24のハードウェア実装

**[F24/l_slin_def.h:19](F24/l_slin_def.h#L19)**
```c
#define  U1G_LIN_BTSP               ((l_u8)1U)   /* Inter-byte space */
```

**[F24/l_slin_drv_rl78f24.h:94](F24/l_slin_drv_rl78f24.h#L94)**
```c
#define  U1G_LIN_LSC_IBS    ((l_u8)((U1G_LIN_BTSP & (l_u8)0x03U) << U1G_LIN_BIT_SFT_4))  /* インタバイトスペース設定値 IBS b5-b4 */
```

**[F24/l_slin_drv_rl78f24.c:94](F24/l_slin_drv_rl78f24.c#L94)**
```c
U1G_LIN_SFR_LSC  = U1G_LIN_LSC_INIT;  /* LINスペース設定（RS、BS） */
```

**ハードウェアレジスタ設定:**
- `LSC.IBS[5:4]`: インターバイトスペース設定（0～3ビット）
- LINモジュールが自動的にバイト間スペースを挿入
- ソフトウェアでのタイミング制御は不要

### H850のソフトウェア実装

**[H850/slin_lib/l_slin_def.h:36](H850/slin_lib/l_slin_def.h#L36)**
```c
#define  U1G_LIN_BTSP               ((l_u8)1)    /* Inter-byte space */
```

**[H850/slin_lib/l_slin_core_h83687.c:1205,1216](H850/slin_lib/l_slin_core_h83687.c#L1205)**
```c
/* データ送信時 */
l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  /* 1バイト送信 */
l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  /* データ10bit + Inter-Byte Space */
u1l_lin_rs_cnt++;
```

**ソフトウェアタイマー実装:**
1. 1バイト送信後、タイマー設定
2. `U1G_LIN_BYTE_LENGTH`（10ビット：1スタート + 8データ + 1ストップ）+ `U1G_LIN_BTSP`（1ビット）= 11ビット
3. タイマー割り込みで次のバイト送信処理

**タイミングシーケンス:**
```
[1バイト目送信] → [タイマー11ビット] → [割り込み] → [2バイト目送信] → [タイマー11ビット] → ...
                    ^^^^^^^^^^^^^^^^
                    10ビット送信時間 + 1ビットスペース
```

### CC23xxでの必要な実装

**[CC23xx/l_slin_def.h:18](CC23xx/l_slin_def.h#L18)**
```c
#define  U1G_LIN_BTSP               ((l_u8)1)    /* Inter-byte space */
```

**現状の問題:**
- インターバイトスペースのタイマー設定が未実装
- 現在の実装は複数バイト一括送信のため、バイト間スペースが制御できない
- H850の1バイト送信方式への変更が必要

**推奨実装（DRV層）:**

H850と同じ`l_vog_lin_bit_tm_set()`を使用（上記参照）

**推奨実装（CORE層）:**

```c
/* データ送信処理（H850互換） */
case ( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    /* データ送信2バイト目以降 */
    if( u1l_lin_rs_cnt > U1G_LIN_0 ){
        /* 前回送信バイトのリードバックチェック */
        l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );

        if( l_u1a_lin_read_back != U1G_LIN_OK ){
            /* BITエラー処理 */
        }
        else{
            if( u1l_lin_rs_cnt > u1l_lin_frm_sz ){
                /* 送信完了 */
            }
            else{
                /* 次のバイト送信 */
                l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  /* 1バイト送信 */
                l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  /* データ10bit + Inter-Byte Space */
                u1l_lin_rs_cnt++;
            }
        }
    }
    /* データ送信1バイト目 */
    else{
        l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  /* 1バイト送信 */
        l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  /* データ10bit + Inter-Byte Space */
        u1l_lin_rs_cnt++;
    }
    break;
```

**重要:**
- `l_vog_lin_tx_char()` は1バイト送信に変更必要（[TX_READBACK_ISSUE_ANALYSIS.md](TX_READBACK_ISSUE_ANALYSIS.md)参照）
- 複数バイト一括送信方式では、インターバイトスペースの制御が不可能

---

## エラー処理の違い

### LIN 1.3のエラー種別

**H850（LIN 1.3）で検出するエラー:**

| エラー種類 | 検出方法 | フラグ | 説明 |
|-----------|---------|-------|------|
| **UART Overrun** | SSRレジスタ | `u1g_lin_e_uart` | 受信オーバーラン |
| **UART Framing** | SSRレジスタ | `u1g_lin_e_uart` | フレーミングエラー |
| **UART Parity** | SSRレジスタ | `u1g_lin_e_uart` | パリティエラー |
| **Synch Field Error** | データ比較 | `u2g_lin_e_synch` | Synchフィールド不一致（0x55以外） |
| **Parity Error** | パリティ計算 | `u2g_lin_e_pari` | Protected IDパリティ不一致 |
| **Checksum Error** | チェックサム計算 | `u2g_lin_e_sum` | チェックサムエラー |
| **No Response** | タイマータイムアウト | `u2g_lin_e_nores` | スレーブ無応答 |
| **BIT Error** | リードバック比較 | `u2g_lin_e_bit` | 送信データ不一致 |
| **Header Timeout** | タイマータイムアウト | `u2g_lin_e_time` | ヘッダータイムアウト |
| **Physical Bus Error** | 連続エラー検出 | `u2g_lin_phy_bus_err` | 物理バスエラー（Header Timeout 3回） |

**エラー検出処理（H850）:**

**[H850/slin_lib/l_slin_drv_h83687.c:191-217](H850/slin_lib/l_slin_drv_h83687.c#L191-L217)**
```c
void l_ifc_rx_ch1(void)
{
    l_u8 u1a_lin_rx_status = U1G_LIN_REG_SSR.u1l_lin_byte;
    l_u8 u1a_lin_rx_data = U1G_LIN_REG_RDR.u1l_lin_byte;
    l_u8 u1a_lin_rx_err = U1G_LIN_BYTE_CLR;

    /* 受信エラー情報の生成 */
    if( (u1a_lin_rx_status & U1G_LIN_SOER_MASK) != 0 ) {
        u1a_lin_rx_err |= U1G_LIN_SOER_SET;  /* オーバランエラー */
    }
    if( (u1a_lin_rx_status & U1G_LIN_SFER_MASK) != 0 ) {
        u1a_lin_rx_err |= U1G_LIN_SFER_SET;  /* フレーミングエラー */
    }
    if( (u1a_lin_rx_status & U1G_LIN_SPER_MASK) != 0 ) {
        u1a_lin_rx_err |= U1G_LIN_SPER_SET;  /* パリティエラー */
    }

    l_vog_lin_rx_int( u1a_lin_rx_data, u1a_lin_rx_err );
}
```

### LIN 2.xで追加されたエラー種別

**F24（LIN 2.x）で追加検出するエラー:**

| エラー種類 | 検出方法 | フラグ | LIN 1.3との違い |
|-----------|---------|-------|----------------|
| **Bit Error** | ハードウェア（LINモジュール） | `LEST.BTER` | LIN 1.3: リードバック比較 |
| **Response Too Short** | ハードウェア（D1RC判定） | タイマー + D1RC | LIN 1.3: タイマーのみ |
| **Timeout Error** | ハードウェア（LINモジュール） | `LEST.FTER` | LIN 1.3: ソフトウェアタイマー |
| **Identifier Parity Error** | ハードウェア（LINモジュール） | `LEST.IPER` | LIN 1.3: ソフトウェア計算 |
| **Checksum Error** | ハードウェア（LINモジュール） | `LEST.CSER` | LIN 1.3: ソフトウェア計算 |

**エラー検出処理（F24）:**

**[F24/l_slin_drv_rl78f24.c:570-580](F24/l_slin_drv_rl78f24.c#L570-L580)**
```c
void l_ifc_err(void)
{
    l_u8 u1a_lin_lst_data = U1G_LIN_SFR_LST;   /* LINステータス取得 */
    l_u8 u1a_lin_err = U1G_LIN_SFR_LEST;       /* LINエラー情報取得*/
    U1G_LIN_SFR_LEST = U1G_LIN_LEST_CLR;       /* 全エラーフラグクリア */

    l_vog_lin_err_int( u1a_lin_err, u1a_lin_lst_data );
}
```

**F24のLESTレジスタ（エラー検出許可）:**

**[F24/l_slin_drv_rl78f24.h:110](F24/l_slin_drv_rl78f24.h#L110)**
```c
#define  U1G_LIN_LEDE_RUN   ((l_u8)0x59U)  /* LEDEレジスタレスポンス送受信時設定値 */
                                           /* bit0: FTER (Timeout Error Detection Enable) */
                                           /* bit3: BTER (Bit Error Detection Enable) */
                                           /* bit4: CSER (Checksum Error Detection Enable) */
                                           /* bit6: IPER (Identifier Parity Error Detection Enable) */
```

### CC23xxでの実装状況とギャップ

**現状実装済み:**

| エラー種類 | 実装状況 | 実装場所 |
|-----------|---------|---------|
| **UART Overrun** | ✅ 実装済み | `UART_RSR_ECR.OE` |
| **UART Framing** | ✅ 実装済み | `UART_RSR_ECR.FE` |
| **UART Parity** | ✅ 実装済み | `UART_RSR_ECR.PE` |
| **Synch Field Error** | ✅ 実装済み | CORE層ソフトウェア |
| **Parity Error** | ✅ 実装済み | CORE層ソフトウェア |
| **Checksum Error** | ✅ 実装済み | CORE層ソフトウェア |

**未実装（要対応）:**

| エラー種類 | 実装方法 | 優先度 |
|-----------|---------|-------|
| **No Response** | LGPTimerでタイムアウト検出 | **高** |
| **BIT Error** | リードバック比較（1バイト送信化必要） | **高** |
| **Header Timeout** | LGPTimerでタイムアウト検出 | **高** |
| **Response Too Short** | LGPTimerでタイムアウト検出 + バイトカウント | **中** |
| **Physical Bus Error** | Header Timeoutカウンタ（3回） | **中** |

---

## タイムアウト処理の違い

### F24のタイムアウト処理

**タイマー仕様:**
- TAUモジュール（16ビットタイマー）使用
- クロック: fCLK/4 (40MHz / 4 = 10MHz)
- タイムベース: 5ms

**[F24/l_slin_def.h:25-26](F24/l_slin_def.h#L25-L26)**
```c
#define  U2G_LIN_TIME_BASE      ((l_u16)5U)    /* タイムベース時間の設定(ms) */
#define  U2G_LIN_BUS_TIMEOUT    ((l_u16)4100U) /* Bus inactive時間の設定（ms）*/
```

**タイムアウトエラー確定処理:**

**[F24/l_slin_drv_rl78f24.c:824-853](F24/l_slin_drv_rl78f24.c#L824-L853)**
```c
void l_vog_lin_determine_ResTooShort_or_NoRes(l_u8 u1a_lst_d1rc_state)
{
    if( U1G_LIN_BYTE_CLR != (U1G_LIN_TAU_TE & U1G_LIN_TE_ENABLE) ) {
        /* 汎用タイマ動作許可状態 */
        l_vog_lin_stop_timer();  /* タイマのカウンタ停止 */

        if( u1a_lst_d1rc_state == U1G_LIN_LST_D1RC_COMPLETE ) {
            /* データ1受信フラグが 1 の場合、Response Too Shortを確定 */
            l_vog_lin_ResTooShort_error();
        }
        else if( u1a_lst_d1rc_state == U1G_LIN_LST_D1RC_INCOMPLETE ) {
            /* データ1受信フラグが 0 の場合、No Response を確定 */
            l_vog_lin_NoRes_error();
        }

        U1G_LIN_SFR_LST &= (~U1G_LIN_LST_D1RC_MASK);  /* LSTレジスタD1RC クリア */
    }
}
```

**特徴:**
- ハードウェアの`D1RC`フラグ（Data 1 Receive Complete）を利用
- Response Too Short と No Response を区別可能

### H850のタイムアウト処理

**タイマー仕様:**
- Timer Z0（16ビットタイマー）使用
- クロック: φ/8 (12MHz / 8 = 1.5MHz)
- タイマーカウント方式: デクリメント（複数回割り込み対応）

**[H850/slin_lib/l_slin_drv_h83687.c:117-154](H850/slin_lib/l_slin_drv_h83687.c#L117-L154)**
```c
void l_vog_lin_timer_init(void)
{
    /* 1Bitタイマ値のセット */
    #if U1G_LIN_BRGSRC == U1G_LIN_BRGSRC_F1
        u2l_lin_tm_bit = ( (l_u16)U1G_LIN_BRG * U2G_LIN_FRM_TM_SRC_1 ) * U2G_LIN_4;
    #elif U1G_LIN_BRGSRC == U1G_LIN_BRGSRC_F4
        u2l_lin_tm_bit = ( (l_u16)U1G_LIN_BRG * U2G_LIN_FRM_TM_SRC_4 ) * U2G_LIN_4;
    #endif

    /* 0xFFFFHカウント分のビット長のセット */
    if(u2l_lin_tm_bit != U2G_LIN_BYTE_CLR){
        u2l_lin_tm_maxbit = U2G_LIN_WORD_LIMIT / u2l_lin_tm_bit;
    }
}
```

**タイマー割り込み処理:**

**[H850/slin_lib/l_slin_drv_h83687.c:228-246](H850/slin_lib/l_slin_drv_h83687.c#L228-L246)**
```c
void l_ifc_tm_ch1(void)
{
    l_u8 u1a_lin_rdata;

    /* タイマステータスレジスタのクリア */
    u1a_lin_rdata = U1G_LIN_REG_TSR.u1l_lin_byte;
    U1G_LIN_REG_TSR.u1l_lin_byte = U1G_LIN_BYTE_CLR;

    /* タイマカウントが"0"以上の場合 */
    if( u2l_lin_tm_cnt > U2G_LIN_WORD_CLR ){
        u2l_lin_tm_cnt--;  /* デクリメントして抜ける */
    }
    /* タイマカウントが"0"の場合 */
    else{
        U1G_LIN_FLG_TMST = U1G_LIN_BIT_CLR;  /* タイマ停止 */

        l_vog_lin_tm_int();  /* タイマ割り込み報告 */
    }
}
```

**特徴:**
- 16ビットタイマーのオーバーフロー対策でカウンタ変数使用
- 長時間タイムアウト対応（複数回割り込み）

### CC23xxでの実装推奨

**タイマー仕様:**
- LGPTimer（LGPT0: 16ビット）使用
- クロック: 48MHz（分周なし）
- ワンショットモード使用

**実装方針:**
- H850のアーキテクチャを踏襲
- F24のタイムアウトエラー検出機能を追加
- 詳細は[LGPTIMER_LIN_TIMEOUT.md](CC23xx/LGPTIMER_LIN_TIMEOUT.md)を参照

**タイムアウト値計算例（@9600bps）:**

| 項目 | ビット長 | 時間 (ms) | カウンタ値（48MHz） |
|------|---------|-----------|-------------------|
| **1バイト** | 10ビット | 1.04 ms | 49,920 |
| **レスポンススペース** | 1ビット | 0.104 ms | 4,992 |
| **インターバイトスペース** | 1ビット | 0.104 ms | 4,992 |
| **ヘッダータイムアウト** | 149ビット | 15.52 ms | 745,344 ※ |
| **8バイトデータ** | 80ビット | 8.33 ms | 399,360 ※ |

※ 16ビットタイマーのため、オーバーフロー対策が必要（65,535カウント = 1.365ms以上）

**推奨実装:**

```c
/**
 * @brief ビットタイマ設定
 * @param u1a_lin_bit タイマ設定値（ビット長）
 */
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    uint32_t counterTarget;
    uint32_t timeout_us;

    /* タイマー停止 */
    LGPTimerLPF3_stop(xnl_lin_timer_handle);

    /* ビット長をマイクロ秒に変換 */
    #if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
        /* 9600bps: 1ビット = 104.17us */
        timeout_us = ((uint32_t)u1a_lin_bit * 1041667UL) / 10000UL;
    #elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
        /* 19200bps: 1ビット = 52.08us */
        timeout_us = ((uint32_t)u1a_lin_bit * 520833UL) / 10000UL;
    #endif

    /* カウンタターゲット計算（48MHz = 48カウント/us） */
    counterTarget = (timeout_us * 48UL);
    if (counterTarget > 0) {
        counterTarget -= 1UL;
    }

    /* 16ビットオーバーフロー対策 */
    if (counterTarget > 0xFFFFUL) {
        /* プリスケーラ使用または複数回割り込み対応が必要 */
        /* 詳細はLGPTIMER_LIN_TIMEOUT.mdを参照 */
        counterTarget = 0xFFFFUL;
    }

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, counterTarget, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

---

## CC23xx向けの実装推奨事項

### 1. アーキテクチャ方針

**基本方針:**
- **H850のアーキテクチャを尊重**
  - 1バイト単位の送受信処理を維持
  - ソフトウェアタイマーによるタイミング制御
  - リードバック処理による送信確認

- **LIN 2.x機能の追加**
  - レスポンススペースのソフトウェア実装
  - インターバイトスペースのソフトウェア実装
  - タイムアウトエラー検出の強化

- **CC23xxハードウェア特性の活用**
  - UART2のFIFO機能（受信のみ）
  - LGPTimerの高精度タイミング（48MHz）
  - レジスタ直接アクセスによるエラー検出

### 2. UARTドライバレベルで実装すべき機能（DRV層）

#### 2-1. 1バイト送信処理

**現状の問題:**
- 複数バイト一括送信方式（配列受け取り）
- インターバイトスペース制御不可
- リードバックが正しく動作しない

**推奨実装:**

```c
/**
 * @brief 1バイト送信処理（H850互換）
 * @param u1a_lin_data 送信データ（1バイト）
 */
void l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    /* リードバック用に1バイト受信設定 */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE);

    /* 送信バッファに1バイトコピー */
    u1l_lin_tx_buf[0] = u1a_lin_data;

    /* 1バイト送信 */
    UART_RegInt_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1);
}
```

**参照:**
- [TX_READBACK_ISSUE_ANALYSIS.md](TX_READBACK_ISSUE_ANALYSIS.md)

#### 2-2. リードバック処理

**現状の問題:**
- 複数バイト前提の実装
- 受信完了確認なし
- エラー検出が無効化されている

**推奨実装:**

```c
/**
 * @brief リードバック処理（H850互換）
 * @param u1a_lin_data 送信データ（比較用）
 * @return U1G_LIN_OK/U1G_LIN_NG
 */
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    l_u8 u1a_lin_readback_data;
    l_u8 u1a_lin_result;
    l_u32 u4a_lin_error_status;
    int_fast16_t rxCount;

    /* 受信完了を待つ */
    rxCount = UART2_getRxCount(xnl_lin_uart_handle);
    if (rxCount <= 0) {
        return U1G_LIN_NG;  /* 受信データなし */
    }

    /* エラーステータス取得 */
    u4a_lin_error_status = U4L_LIN_HWREG(
        ((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR
    );

    /* エラーチェック */
    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        /* 受信バッファから1バイト取得 */
        u1a_lin_readback_data = u1l_lin_rx_buf[0];

        /* データ比較 */
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

#### 2-3. タイマー処理

**推奨実装:**

```c
/**
 * @brief タイマー初期化
 */
void l_vog_lin_timer_init(void)
{
    LGPTimerLPF3_Params params;

    /* パラメータ初期化 */
    LGPTimerLPF3_Params_init(&params);
    params.hwiCallbackFxn = linTimerCallback;
    params.prescalerDiv = 0;  /* プリスケーラなし（48MHz） */

    /* タイマーオープン（LGPT0使用） */
    xnl_lin_timer_handle = LGPTimerLPF3_open(0, &params);

    if (xnl_lin_timer_handle == NULL) {
        u1g_lin_syserr = U1G_LIN_SYSERR_TIMER;
    }
}

/**
 * @brief ビットタイマ設定
 * @param u1a_lin_bit タイマ設定値（ビット長）
 */
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    uint32_t counterTarget;
    uint32_t timeout_us;

    /* タイマー停止 */
    LGPTimerLPF3_stop(xnl_lin_timer_handle);

    /* ビット長をマイクロ秒に変換 */
    #if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
        timeout_us = ((uint32_t)u1a_lin_bit * 1041667UL) / 10000UL;
    #elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
        timeout_us = ((uint32_t)u1a_lin_bit * 520833UL) / 10000UL;
    #endif

    /* カウンタターゲット計算 */
    counterTarget = (timeout_us * 48UL);
    if (counterTarget > 0) {
        counterTarget -= 1UL;
    }

    /* 16ビットオーバーフロー対策 */
    if (counterTarget > 0xFFFFUL) {
        counterTarget = 0xFFFFUL;
    }

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, counterTarget, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}

/**
 * @brief タイマー停止
 */
void l_vog_lin_frm_tm_stop(void)
{
    if (xnl_lin_timer_handle != NULL) {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }
}
```

**参照:**
- [LGPTIMER_LIN_TIMEOUT.md](CC23xx/LGPTIMER_LIN_TIMEOUT.md)

### 3. CORE層で実装すべき機能

#### 3-1. レスポンススペース処理

**推奨実装:**

```c
/* 送信フレーム時 - Protected ID受信後 */
/* H850と同じタイミング */
l_vog_lin_bit_tm_set( U1G_LIN_RSSP );  /* レスポンススペース待ちタイマセット */

u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;  /* データ送信待ち状態 */
```

#### 3-2. インターバイトスペース処理

**推奨実装:**

```c
/* データ送信時 */
l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  /* 1バイト送信 */
l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  /* 10bit + 1bit */
u1l_lin_rs_cnt++;
```

#### 3-3. タイムアウトエラー処理

**推奨実装:**

```c
/**
 * @brief タイマー割り込み処理（l_vog_lin_tm_int）
 */
void l_vog_lin_tm_int(void)
{
    /* スレーブタスクステータス確認 */
    switch( u1l_lin_slv_sts ) {

    /* Header Timeout Error検出 */
    case ( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
    case ( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
    case ( U1G_LIN_SLSTS_IDENTFIELD_WAIT ):
        /* Header Timeout エラー */
        xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time = U2G_LIN_BIT_SET;

        /* Physical Busエラーカウント */
        if (u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT) {
            /* Physical Busエラー（3回連続） */
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
            xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
            u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;
        }
        else {
            u2l_lin_herr_cnt++;
        }

        /* Synch Break待ち状態へ戻る */
        l_vol_lin_set_synchbreak();
        break;

    /* No Response Error検出 */
    case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
        /* No Response エラー */
        xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;

        /* エラーありレスポンス完了 */
        l_vol_lin_set_frm_complete(U1G_LIN_ERR_ON);

        /* Synch Break待ち状態へ戻る */
        l_vol_lin_set_synchbreak();
        break;

    /* Physical Bus Error検出 */
    case ( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
        /* Physical Bus エラー */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
        xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
        break;

    /* その他のステータス */
    default:
        /* 処理なし */
        break;
    }
}
```

### 4. 実装優先度

| 優先度 | 機能 | 実装場所 | 理由 |
|--------|------|---------|------|
| **最高** | 1バイト送信処理 | DRV層 | 現在の複数バイト方式では他の機能が実装不可 |
| **最高** | リードバック処理 | DRV層 | 送信エラー検出に必須 |
| **高** | タイマー初期化・設定 | DRV層 | タイムアウト検出の基盤 |
| **高** | Header Timeout検出 | CORE層 | LIN通信の基本エラー検出 |
| **高** | No Response検出 | CORE層 | スレーブ無応答検出に必須 |
| **中** | レスポンススペース | CORE層 | LIN 2.x仕様対応 |
| **中** | インターバイトスペース | CORE層 | LIN 2.x仕様対応 |
| **中** | Physical Bus Error | CORE層 | バス異常検出 |
| **低** | Response Too Short | CORE層 | 詳細エラー分類（F24互換） |

---

## まとめ

### H850（LIN 1.3）とF24（LIN 2.x）の主な違い

| 項目 | H850（LIN 1.3） | F24（LIN 2.x） | 移行の影響 |
|------|----------------|---------------|-----------|
| **Response Space** | ソフトウェアタイマー | ハードウェア（LSC.RS） | CC23xxでソフトウェア実装 |
| **Inter-byte Space** | ソフトウェアタイマー | ハードウェア（LSC.IBS） | CC23xxでソフトウェア実装 |
| **Checksum方式** | Classic | Classic/Enhanced | Classicのみでも互換性あり |
| **エラー検出** | ソフトウェア主体 | ハードウェア主体 | ソフトウェアで対応可能 |
| **Auto Baudrate** | 非サポート | サポート（オプション） | CC23xxでは不要 |

### CC23xxでの実装方針まとめ

1. **H850のアーキテクチャを踏襲**
   - 1バイト単位の送受信処理を維持
   - ソフトウェアタイマーによるタイミング制御
   - エラー検出はソフトウェア主体

2. **LIN 2.x機能の追加**
   - レスポンススペース: ソフトウェアタイマーで実装
   - インターバイトスペース: ソフトウェアタイマーで実装
   - タイムアウトエラー検出の強化

3. **CC23xxハードウェアの活用**
   - LGPTimer（48MHz）による高精度タイミング
   - UART2レジスタによるエラー検出
   - FIFO機能の活用（受信のみ）

### 実装ロードマップ

**Phase 1: 基本機能の修正（最高優先度）**
1. 1バイト送信処理への変更
2. リードバック処理の修正
3. タイマー初期化・設定の実装

**Phase 2: タイムアウトエラー検出（高優先度）**
1. Header Timeout検出
2. No Response検出
3. Physical Bus Error検出

**Phase 3: LIN 2.x機能追加（中優先度）**
1. レスポンススペース実装
2. インターバイトスペース実装
3. Response Too Short検出

### 参考ドキュメント

- **送信・リードバック処理:** [TX_READBACK_ISSUE_ANALYSIS.md](TX_READBACK_ISSUE_ANALYSIS.md)
- **タイマー実装:** [LGPTIMER_LIN_TIMEOUT.md](CC23xx/LGPTIMER_LIN_TIMEOUT.md)
- **ネットワーク管理層比較:** [NM_LAYER_COMPARISON.md](NM_LAYER_COMPARISON.md)

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Author:** LIN Platform Comparison Analysis Tool
