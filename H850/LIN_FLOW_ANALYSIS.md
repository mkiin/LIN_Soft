# H850 LINライブラリ 動作フロー解析ドキュメント

## 目次
1. [概要](#概要)
2. [アーキテクチャ](#アーキテクチャ)
3. [初期化フロー](#初期化フロー)
4. [データ送信フロー](#データ送信フロー)
5. [データ受信フロー](#データ受信フロー)
6. [割り込み処理フロー](#割り込み処理フロー)
7. [ネットワーク管理とスリープ制御](#ネットワーク管理とスリープ制御)
8. [エラーハンドリング](#エラーハンドリング)
9. [フレーム定義](#フレーム定義)
10. [状態遷移図](#状態遷移図)

---

## 概要

### プロジェクト情報
- **システム名**: S/R System (Sunroof System)
- **ターゲットMCU**: Renesas H8/300H Tiny Series (H8/3687)
- **LINライブラリ**: LSLib3687T v2.00 (SUNNY GIKEN INC.)
- **LIN仕様**: トヨタ自動車標準LIN仕様
- **コンパイラ**: Renesas H8S,H8/300 Series C/C++ Compiler Ver 4.0.03 - 5.0.1
- **開発元**: AISIN SEIKI CO., LTD.
- **開発時期**: 2003年〜2005年

### 機能概要
このLINライブラリは、車載ネットワーク(LIN)を介してサンルーフ制御ECUと他のボディECU間で通信を行うためのスタックです。

**主な機能:**
- LINスレーブノードとしての動作
- 複数フレームの送受信管理（ID: 0x22, 0x23, 0x28, 0x29, 0x2A, 0x2B, 0x3C, 0x3D）
- ネットワーク管理（スリープ/ウェイクアップ制御）
- 通信途絶検出
- ダイアグノーシス対応

---

## アーキテクチャ

### 階層構造

```
┌─────────────────────────────────────────────────────────┐
│             アプリケーション層                              │
│  - LIN通信メイン処理 (f_lin_main.c)                        │
│  - データ取得/送信 (f_lin_if.c)                            │
│  - 通信途絶管理 (p_comdwn_tmr.c)                           │
│  - メッセージ取得 (p_get_msg_lin.c)                        │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│               SLIN API層                                 │
│  - API関数群 (l_slin_api.h)                              │
│  - テーブル管理 (l_slin_tbl.c/h)                         │
│  - 共通処理 (l_slin_cmn.c/h)                             │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│             SLIN CORE層                                  │
│  - プロトコルエンジン (l_slin_core_h83687.c/h)           │
│  - NM管理 (l_slin_nmc.c/h)                               │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│             SLIN DRV層                                   │
│  - UART制御 (l_slin_drv_h83687.c/h)                      │
│  - タイマ制御                                             │
│  - 割り込み制御                                           │
│  - SFRアクセス (l_slin_sfr_h83687.h)                     │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│             ハードウェア                                  │
│  - SCI3 (UART)                                           │
│  - Timer Z0                                              │
│  - IRQ0 (外部割り込み)                                    │
└─────────────────────────────────────────────────────────┘
```

### ファイル構成

#### アプリケーション層 (H850/application/)
| ファイル | 役割 |
|---------|------|
| f_lin_main.c/h | LIN通信の初期化、メイン処理、割り込みハンドラ |
| f_lin_if.c | LINフレームのデータ取得・送信インターフェース |
| p_comdwn_tmr.c/h | 通信途絶監視タイマー管理 |
| p_get_msg_lin.c/h | LINメッセージ取得処理 |
| p_lin_apl.h | アプリケーション固有の定義・構造体 |

#### SLIN Library層 (H850/slin_lib/)
| ファイル | 役割 |
|---------|------|
| l_slin_api.h | LIN API関数プロトタイプとマクロ定義 |
| l_slin_cmn.c/h | 共通処理関数（チェックサム計算等） |
| l_slin_core_h83687.c/h | LINプロトコルコア処理（フレーム送受信制御） |
| l_slin_drv_h83687.c/h | H8/3687向けドライバ（UART/Timer/IRQ制御） |
| l_slin_nmc.c/h | ネットワーク管理コンポーネント（スリープ/ウェイクアップ） |
| l_slin_tbl.c/h | フレームテーブル定義とバッファ管理 |
| l_slin_sfr_h83687.h | H8/3687 SFR（特殊機能レジスタ）定義 |
| l_slin_def.h | LIN定数定義 |

---

## 初期化フロー

### 1. システム起動時の初期化シーケンス

```
[メイン処理]
    ↓
f_vog_lin_init(U1G_FLB_TRUE)  ← RESET時
    ↓
    ├─ 1. 同期フラグ初期化
    │    ├─ F1g_lin_txframe_id23h = CLR (送信完了フラグ)
    │    └─ F1g_lin_rxframe_idXXh = CLR (受信完了フラグ × 5フレーム)
    │
    ├─ 2. 初回受信フラグ初期化
    │    ├─ F_id28_first = SET (ボディECU情報①)
    │    ├─ F_id29_first = SET (ボディECU情報②)
    │    ├─ F_id2a_first = SET (ボディECU情報③)
    │    ├─ F_id22_first = SET (ボディECU情報④)
    │    └─ F_id2b_first = SET (P/WマスタSW情報)
    │
    ├─ 3. 通信途絶フラグクリア
    │    ├─ F_comdwn_01〜05 = CLR
    │    └─ p_vog_tmr_start_idXX_cmdwn() × 5フレーム
    │
    ├─ 4. LINシステム初期化
    │    └─ l_sys_init()
    │         ↓
    │         [SLIN API層]
    │         ├─ パラメータチェック
    │         ├─ グローバル変数初期化
    │         ├─ ステータスバッファ初期化
    │         └─ フレームバッファ初期化
    │
    ├─ 5. インターフェース初期化
    │    └─ l_ifc_init_ch1()
    │         ↓
    │         [SLIN CORE層]
    │         ├─ l_ifc_init_com_ch1()    ← 通信バッファ初期化
    │         │    ├─ ステータスバッファクリア
    │         │    ├─ フレームバッファデフォルト値設定
    │         │    └─ バスステータスクリア
    │         │
    │         └─ l_ifc_init_drv_ch1()    ← ドライバ初期化
    │              ↓
    │              [SLIN DRV層]
    │              ├─ l_vog_lin_uart_init()   ← UART初期化
    │              │    ├─ SCR3設定（送受信禁止）
    │              │    ├─ SMR設定（8bit, パリティなし, 1 stop bit）
    │              │    ├─ BRR設定（ボーレート設定）
    │              │    ├─ UMR設定（8bit, パリティ禁止）
    │              │    └─ UC0/UC1設定
    │              │
    │              ├─ l_vog_lin_timer_init()  ← タイマ初期化
    │              │    ├─ TCR設定（カウントアップ、4分周）
    │              │    ├─ TIER設定（割り込み許可）
    │              │    └─ TCNT初期化
    │              │
    │              └─ l_vog_lin_int_init()    ← 外部割り込み初期化
    │                   └─ IRQ0設定（エッジ極性設定）
    │
    ├─ 6. ネットワーク管理初期化
    │    └─ l_nm_init_ch1()
    │         ↓
    │         [SLIN NMC層]
    │         ├─ スレーブ状態 = PON (Power ON)
    │         ├─ タイマカウンタクリア
    │         ├─ リトライカウンタクリア
    │         └─ マスタエラーフラグクリア
    │
    ├─ 7. スロット有効/無効設定
    │    └─ l_slot_disable_ch1(U1G_LIN_FRAME_ID3DH)  ← ID3Dh無効化
    │
    └─ 8. LIN通信接続
         └─ l_ifc_connect_ch1()
              ↓
              [SLIN CORE層]
              ├─ スレーブタスクステータス = BREAK_UART_WAIT
              ├─ l_vog_lin_rx_enb()        ← UART受信許可
              ├─ l_vog_lin_int_enb()       ← 外部割り込み許可
              └─ l_vog_lin_frm_tm_set()    ← フレームタイマ起動
```

### 2. Wake-up時の初期化シーケンス

```
f_vog_lin_init(U1G_FLB_FALSE)  ← Wake-up時
    ↓
    ├─ F_id28_first はクリアしない（スリープ前の状態を保持）
    └─ その他は RESET時と同じ処理
```

### 初期化時の注意点

1. **二重初期化の防止**: `l_sys_init()`や`l_ifc_connect_ch1()`が失敗した場合は、CPU リセット処理(`f_vog_set_fail_cpu()`)を実行

2. **デフォルトスロット設定**:
   - 受信スロット: ID28h, ID29h, ID2Ah, ID22h, ID2Bh, ID24h, ID3Ch（デフォルト有効）
   - 送信スロット: ID23h（デフォルト有効）, ID3Dh（アプリケーションで無効化）

3. **割り込み優先順位**:
   - SCI3受信割り込み（最高優先）
   - Timer Z0割り込み
   - IRQ0外部割り込み

---

## データ送信フロー

### 1. アプリケーションからの送信要求

```
[アプリケーション層]
f_vog_lin_snd_main()  ← 周期的に呼び出し（メインループ）
    ↓
f_vog_lin_warn_send()  ← 警告情報送信処理
    ↓
    ├─ 1. 送信データ準備
    │    └─ Fll_lin_srbz = 警告状態 (S/Rブザー要求)
    │         ↓
    │         xng_tx_dat_id23.st_frame_srf1s01構造体に設定
    │
    └─ 2. LIN APIで送信
         └─ l_slot_wr_ch1(U1G_LIN_FRAME_ID23H, U1G_LIN_4, xng_tx_dat_id23.u1_tx_byte)
              ↓
              [SLIN API層]
              ├─ パラメータチェック
              │    ├─ フレームハンドルが有効範囲内か？
              │    ├─ データ長が1〜8の範囲内か？
              │    └─ データポインタがNULLでないか？
              │
              ├─ 送信バッファにコピー
              │    └─ xng_lin_frm_buf[slot].xng_lin_data = 送信データ
              │
              └─ チェックサム計算
                   └─ xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_chksum = 計算値
```

### 2. LINバス上での送信処理

```
[SLIN CORE層 - l_ifc_rx_ch1()]
マスタからID23hのヘッダを受信
    ↓
    ├─ 1. ID受信検出
    │    └─ u1a_lin_id = 受信ID (0x23)
    │
    ├─ 2. IDパリティチェック
    │    └─ l_u1g_lin_id_parity_chk(u1a_lin_id)
    │         ├─ P0 = ID0 XOR ID1 XOR ID2 XOR ID4
    │         ├─ P1 = NOT (ID1 XOR ID3 XOR ID4 XOR ID5)
    │         └─ OK → 送信処理へ / NG → エラー処理
    │
    ├─ 3. スロット検索
    │    └─ u1g_lin_id_tbl[]からID23hに対応するスロット番号を取得
    │         → slot = 5 (U1G_LIN_FRAME_ID23H)
    │
    ├─ 4. 送信/受信判定
    │    └─ xng_lin_slot_tbl[slot].u1g_lin_sndrcv == 0 → 送信フレーム
    │
    └─ 5. データ送信開始
         └─ u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT
              ↓
              ├─ u1l_lin_snd_cnt = 0 (送信カウンタ初期化)
              └─ l_vog_lin_tx_char(xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[0])
                   ↓
                   [SLIN DRV層]
                   └─ TDR = 送信データ (レジスタ直接書き込み)
```

### 3. 送信データの順次送出

```
[送信割り込み処理 - UART送信完了毎]
l_ifc_rx_ch1() (送信完了割り込み内で呼び出し)
    ↓
    ├─ u1l_lin_snd_cnt++
    │
    ├─ データ送信継続判定
    │    └─ if (u1l_lin_snd_cnt < フレームサイズ)
    │         └─ l_vog_lin_tx_char(xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[u1l_lin_snd_cnt])
    │
    └─ 全データ送信完了
         └─ チェックサム送信
              └─ l_vog_lin_tx_char(xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_chksum)
                   ↓
                   チェックサム送信完了
                   ↓
                   ├─ F1g_lin_txframe_id23h = SET  ← 送信完了フラグセット
                   ├─ u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT
                   └─ l_vog_lin_rx_enb()  ← 受信許可に戻る
```

### 4. 送信完了の確認（アプリケーション側）

```
[アプリケーション層]
送信完了確認
    ↓
    └─ if (F1g_lin_txframe_id23h == U1G_LIN_BIT_SET)
         ↓
         ├─ 送信完了
         ├─ F1g_lin_txframe_id23h = CLR (フラグクリア)
         └─ エラー確認: F4g_lin_errframe_id23h
              ├─ 0x00 → 正常完了
              ├─ 0x01 → 応答なしエラー
              ├─ 0x02 → UARTエラー
              ├─ 0x04 → ビットエラー
              └─ 0x08 → チェックサムエラー
```

---

## データ受信フロー

### 1. LINバスからの受信処理

```
[UART受信割り込み]
f_vogi_inthdr_sci3_2()  ← SCI3受信割り込みハンドラ
    ↓
l_ifc_rx_ch1()
    ↓
    [SLIN CORE層]
    ├─ UART受信データ・エラーフラグ取得
    │    ├─ u1a_lin_data = RDR (受信データレジスタ)
    │    └─ u1a_lin_err = SSR & エラービットマスク
    │
    ├─ スレーブタスクステータスに応じた処理
    │
    ├─【BREAK_UART_WAIT状態】
    │    └─ Synch Breakフィールド待ち
    │         ├─ 受信データ == 0x00 かつ フレーミングエラー検出？
    │         │    └─ YES → Synch Break検出
    │         │         ├─ u1l_lin_slv_sts = SYNCHFIELD_WAIT
    │         │         ├─ l_vog_lin_bit_tm_set(U1G_LIN_BYTE_LENGTH)
    │         │         └─ ヘッダタイムアウトタイマ起動
    │         └─ NO → 通常データとして無視
    │
    ├─【SYNCHFIELD_WAIT状態】
    │    └─ Synch Fieldフィールド待ち (0x55)
    │         ├─ 受信データ == 0x55？
    │         │    └─ YES → Synch Field検出
    │         │         ├─ u1l_lin_slv_sts = IDENTFIELD_WAIT
    │         │         └─ ビットタイミング同期
    │         └─ NO → ヘッダエラー
    │
    ├─【IDENTFIELD_WAIT状態】
    │    └─ Identifier Field (Protected ID) 待ち
    │         ├─ IDパリティチェック
    │         │    └─ l_u1g_lin_id_parity_chk(u1a_lin_data)
    │         │
    │         ├─ スロット検索
    │         │    └─ u1g_lin_id_tbl[]から該当IDのスロット番号取得
    │         │
    │         ├─ 送信/受信判定
    │         │    ├─ 受信フレーム → RCVDATA_WAIT状態へ
    │         │    └─ 送信フレーム → SNDDATA_WAIT状態へ
    │         │
    │         └─ スロット無効 or ID不一致
    │              └─ BREAK_UART_WAIT状態へ戻る
    │
    ├─【RCVDATA_WAIT状態】← 受信データフィールド処理
    │    └─ データバイト受信
    │         ├─ xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[u1l_lin_rcv_cnt] = u1a_lin_data
    │         ├─ u1l_lin_rcv_cnt++
    │         ├─ チェックサム累積計算
    │         │    └─ u2l_lin_sum += u1a_lin_data
    │         │
    │         └─ 全データ受信完了？
    │              └─ YES → チェックサム受信へ
    │                   ├─ 受信チェックサム = u1a_lin_data
    │                   ├─ 計算チェックサム = ~u2l_lin_sum (反転)
    │                   ├─ 一致判定
    │                   │    ├─ OK → 正常受信完了
    │                   │    │    ├─ F1g_lin_rxframe_idXXh = SET
    │                   │    │    └─ F4g_lin_errframe_idXXh = 0x00
    │                   │    └─ NG → チェックサムエラー
    │                   │         ├─ F1g_lin_rxframe_idXXh = SET
    │                   │         └─ F4g_lin_errframe_idXXh = 0x08
    │                   └─ u1l_lin_slv_sts = BREAK_UART_WAIT
    │
    └─ UARTエラー処理
         ├─ フレーミングエラー (0x10)
         ├─ オーバーランエラー (0x20)
         └─ パリティエラー (0x08)
```

### 2. アプリケーションでのデータ取得

```
[アプリケーション層]
f_vog_lin_rcv_main()  ← 周期的に呼び出し（メインループ）
    ↓
f_vog_get_lin_dat()
    ↓
    ├─ f_vog_ecu1st_get()   ← ボディECU情報① (ID28h) 取得
    ├─ f_vog_ecu2nd_get()   ← ボディECU情報② (ID29h) 取得
    ├─ f_vog_ecu3rd_get()   ← ボディECU情報③ (ID2Ah) 取得
    ├─ f_vog_ecu4th_get()   ← ボディECU情報④ (ID22h) 取得
    └─ f_vog_pwsw1st_get()  ← P/WマスタSW情報 (ID2Bh) 取得
```

### 3. 個別フレーム取得処理（例: ID28h）

```
f_vog_ecu1st_get()  ← ボディECU情報① (ID28h)
    ↓
    ├─ 1. 受信完了確認
    │    └─ if (F1g_lin_rxframe_id28h == U1G_LIN_BIT_SET)
    │         ↓
    │         ├─ 割り込み禁止
    │         ├─ F1g_lin_rxframe_id28h = CLR
    │         └─ 割り込み許可
    │
    ├─ 2. エラーチェック
    │    └─ if (F4g_lin_errframe_id28h != 0x00)
    │         ↓
    │         ├─ エラーフラグクリア
    │         └─ エラー処理へ
    │
    ├─ 3. 正常データ読み出し
    │    └─ l_slot_rd_ch1(U1G_LIN_FRAME_ID28H, U1G_LIN_4, xnl_rx_dat_id28.u1_rx_byte)
    │         ↓
    │         [SLIN API層]
    │         └─ 受信バッファからアプリケーションバッファへコピー
    │              └─ xnl_rx_dat_id28.u1_rx_byte[] = xng_lin_frm_buf[0].xng_lin_data.u1g_lin_byte[]
    │
    ├─ 4. 初回受信フラグクリア
    │    └─ F_id28_first = CLR
    │
    ├─ 5. 通信途絶タイマリセット
    │    ├─ F_comdwn_01 = CLR
    │    └─ p_vog_tmr_start_id28_cmdwn()
    │
    └─ 6. データ抽出
         └─ ビットフィールドから個別信号取得
              ├─ F_lin_ig = Fll_lin_ig         (IG SW信号)
              ├─ F_lin_pws = Fll_lin_pws       (P/W作動許可)
              ├─ F_lin_dcty = Fll_lin_dcty     (D席カーテシSW)
              ├─ F_lin_kpwu = Fll_lin_kpwu     (キー連動P/W UP)
              ├─ F_lin_kpwd = Fll_lin_kpwd     (キー連動P/W DOWN)
              ├─ F_lin_wpwu = Fll_lin_wpwu     (ワイヤレス連動UP)
              ├─ F_lin_wpwd = Fll_lin_wpwd     (ワイヤレス連動DOWN)
              └─ F_lin_spwu = Fll_lin_spwu     (スマート連動UP)
```

### 4. 通信途絶検出処理

```
f_vog_ecu1st_get() (受信タイムアウト時)
    ↓
    ├─ スリープ状態チェック
    │    └─ u1a_lin_sleep_ans = l_nm_rd_slv_stat_ch1()
    │         └─ if (U1G_LIN_SLVSTAT_SLEEP == u1a_lin_sleep_ans)
    │              └─ スリープ中は途絶判定しない
    │                   └─ p_vog_tmr_start_id28_cmdwn() (タイマリセット)
    │
    └─ 通信途絶判定
         └─ if (p_u1g_tmr_jdg_id28_cmdwn() == TRUE)
              ↓
              ├─ F_comdwn_01 = SET           ← 途絶フラグセット
              ├─ F_id28_first = SET          ← 初回受信フラグ再設定
              └─ デフォルト値設定
                   ├─ F_lin_ig = OFF
                   ├─ F_lin_pws = OFF
                   └─ xng_lin_info_rmt = 0x00 (全リモート信号OFF)
```

---

## 割り込み処理フロー

### 1. 割り込みの種類と優先順位

| 優先順位 | 割り込み | ハンドラ | 役割 |
|---------|---------|---------|------|
| 高 | SCI3受信割り込み | `f_vogi_inthdr_sci3_2()` | UART受信データ処理 |
| 中 | Timer Z0割り込み | `f_vogi_inthdr_tmr_z0()` | フレームタイミング管理 |
| 低 | IRQ0外部割り込み | `f_vogi_inthdr_irq0()` | Synch Break / Wake-up検出 |

### 2. UART受信割り込み処理

```
[ハードウェア] SCI3受信完了
    ↓
f_vogi_inthdr_sci3_2()  ← #pragma interrupt宣言
    ↓
l_ifc_rx_ch1()
    ↓
    [SLIN CORE層] l_vog_lin_rx_int()
    ↓
    ├─ 受信データ取得
    │    ├─ u1a_lin_data = RDR (受信データレジスタ)
    │    └─ u1a_lin_err = SSR (ステータスレジスタ)
    │
    ├─ エラーチェック
    │    ├─ フレーミングエラー (bit4)
    │    ├─ オーバーランエラー (bit5)
    │    └─ パリティエラー (bit3)
    │
    ├─ スレーブタスク状態に応じた処理
    │    ├─ BREAK_UART_WAIT    → Synch Break検出
    │    ├─ SYNCHFIELD_WAIT    → Synch Field検出
    │    ├─ IDENTFIELD_WAIT    → ID受信・パリティチェック
    │    ├─ RCVDATA_WAIT       → データ受信・チェックサム検証
    │    └─ SNDDATA_WAIT       → データ送信・リードバックチェック
    │
    └─ フレーム完了時
         ├─ 同期フラグセット (F1g_lin_rxframe_idXXh or F1g_lin_txframe_idXXh)
         ├─ エラーフラグ設定 (F4g_lin_errframe_idXXh)
         └─ スレーブタスク状態リセット (BREAK_UART_WAIT)
```

### 3. タイマ割り込み処理

```
[ハードウェア] Timer Z0 オーバーフロー (約49μs周期)
    ↓
f_vogi_inthdr_tmr_z0()  ← #pragma interrupt宣言
    ↓
l_ifc_tm_ch1()
    ↓
    [SLIN CORE層] l_vog_lin_tm_int()
    ↓
    ├─ 1. フレームタイムアウト監視
    │    └─ u1l_lin_frm_tm_cnt++
    │         ├─ ヘッダタイムアウト判定 (49μs × 49 = 2.4ms)
    │         │    └─ if (u1l_lin_frm_tm_cnt >= U1G_LIN_HEADER_MAX_TIME)
    │         │         ├─ ヘッダタイムアウトエラー
    │         │         ├─ F1g_lin_header_err_timeout_ch1 = SET
    │         │         └─ スレーブタスク状態 = BREAK_UART_WAIT
    │         │
    │         └─ レスポンスタイムアウト判定
    │              └─ データ長に応じたタイムアウト時間で監視
    │
    ├─ 2. Physical Busエラー検出
    │    └─ u2l_lin_bus_tm_cnt++
    │         └─ if (u2l_lin_bus_tm_cnt >= U2G_LIN_HERR_LIMIT)  ← 25000bitタイム
    │              ├─ F1g_lin_p_bus_err_ch1 = SET
    │              └─ ネットワーク管理へ通知
    │
    └─ 3. Wait Bit挿入タイミング管理
         └─ ビット長カウント (10bit = 1バイト分)
```

### 4. 外部割り込み処理 (IRQ0)

```
[ハードウェア] LINバス立ち下がりエッジ検出
    ↓
f_vogi_inthdr_irq0()  ← #pragma interrupt宣言
    ↓
l_ifc_aux_ch1()
    ↓
    [SLIN CORE層] l_vog_lin_irq_int()
    ↓
    ├─ スレーブタスク状態確認
    │
    ├─【BREAK_IRQ_WAIT状態】
    │    └─ Synch Break検出 (Wake-up信号検出)
    │         ├─ Synch Break立ち下がりエッジ検出
    │         ├─ u1l_lin_slv_sts = BREAK_UART_WAIT
    │         ├─ 外部割り込み禁止
    │         ├─ エッジ極性反転 (立ち上がり→立ち下がり)
    │         └─ ヘッダタイムアウトタイマ起動
    │
    └─【その他の状態】
         └─ ノイズとして無視
```

### 5. 割り込み禁止/許可制御

```
割り込み禁止 (クリティカルセクション保護)
    ↓
u1l_lin_flg = l_u1g_lin_irq_dis()  ← アセンブリ関数
    ↓
    [アセンブリレベル]
    ├─ CCRレジスタのIビット保存
    ├─ Iビット = 1 (全割り込み禁止)
    └─ 元のIビット値を返す
    ↓
クリティカルセクション処理
(例: フラグクリア、バッファアクセス)
    ↓
l_vog_lin_irq_res(u1l_lin_flg)  ← 割り込み状態復元
    ↓
    [アセンブリレベル]
    └─ CCRレジスタのIビットを復元
```

### 割り込み処理の注意点

1. **割り込みネスト対応**: `l_u1g_lin_irq_dis()`/`l_vog_lin_irq_res()`はネスト対応

2. **処理時間制約**:
   - UART受信割り込み: 次のバイト受信までに処理完了必須（オーバーラン防止）
   - タイマ割り込み: 49μs周期で発生、軽量処理が必須

3. **リエントランシー**: グローバル変数アクセスは割り込み禁止区間で保護

---

## ネットワーク管理とスリープ制御

### 1. スレーブノードの状態遷移

```
┌─────────────────────────────────────────────────────┐
│          ON/PASSIVE/WAKE-WAIT (PON)                 │
│          - システムリセット後の初期状態              │
│          - l_nm_init_ch1()で設定                    │
└─────────────────────────────────────────────────────┘
                        ↓
        l_ifc_connect_ch1() (通信接続)
                        ↓
┌─────────────────────────────────────────────────────┐
│        ON/PASSIVE/WAKE-WAIT (WAKE_WAIT)             │
│        - LINバス監視中                               │
│        - マスタからのフレーム待ち                     │
└─────────────────────────────────────────────────────┘
           ↓                           ↓
  マスタフレーム受信           Bus Idle Timeout (25000bit)
           ↓                           ↓
┌──────────────────────┐    ┌──────────────────────┐
│  ON/ACTIVE           │    │  ON/PASSIVE/SLEEP    │
│  - 通常通信中         │    │  - スリープ状態       │
│  - データ送受信可能   │    │  - 低消費電力モード   │
└──────────────────────┘    └──────────────────────┘
           ↓                           ↑
  スリープ要求受信 (ID=3Ch)              │
           └───────────────────────────┘
                        ↓
              Wake-up信号検出
                        ↓
┌─────────────────────────────────────────────────────┐
│         ON/PASSIVE/WAKE                             │
│         - Wake-up信号送信中                         │
│         - マスタ応答待ち                             │
└─────────────────────────────────────────────────────┘
```

### 2. ネットワーク管理メイン処理

```
f_vog_lin_mng_ctrl()  ← 周期的に呼び出し（メインループ）
    ↓
    ├─ 1. スリープ要因判定
    │    ├─【Physical Busエラー発生時】
    │    │    └─ u1a_slp_req = U1G_LIN_SLP_REQ_FORCE (強制スリープ)
    │    │
    │    ├─【ウェイクアップ要因あり】
    │    │    └─ モータ動作中 or 警告出力中
    │    │         └─ u1a_slp_req = U1G_LIN_SLP_REQ_OFF
    │    │
    │    └─【スリープ要求あり】
    │         └─ モータ停止 and 警告なし
    │              └─ u1a_slp_req = U1G_LIN_SLP_REQ_ON
    │
    └─ 2. NM Tick処理
         └─ l_nm_tick_ch1(u1a_slp_req)
              ↓
              [SLIN NMC層]
              ├─ スレーブ状態に応じた処理
              └─ タイマカウンタ更新
```

### 3. NM Tick処理詳細

```
l_nm_tick_ch1(u1a_lin_slp_req)
    ↓
    ├─【ON/PASSIVE/WAKE-WAIT (PON)状態】
    │    └─ u2l_lin_nm_slvst_cnt++
    │         └─ if (u2l_lin_nm_slvst_cnt >= U2G_LIN_TM_SLVST)  ← 550ms
    │              └─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_WAKE_WAIT
    │
    ├─【ON/PASSIVE/WAKE-WAIT状態】
    │    ├─ Bus Idleタイムアウト監視
    │    │    └─ u2l_lin_nm_timeout_cnt++
    │    │         └─ if (u2l_lin_nm_timeout_cnt >= U2G_LIN_TM_TIMEOUT)  ← 25000bit分
    │    │              ├─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_SLEEP
    │    │              └─ l_ifc_sleep_ch1()  ← スリープ移行
    │    │
    │    └─ フレーム受信時
    │         └─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_ACTIVE
    │
    ├─【ON/ACTIVE状態】
    │    ├─ Bus Idleタイムアウト監視
    │    │    └─ フレーム受信なし → WAKE_WAIT状態へ
    │    │
    │    ├─ スリープコマンド受信 (ID=3Ch, Data0=0x00)
    │    │    └─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_SLEEP
    │    │         └─ l_ifc_sleep_ch1()
    │    │
    │    └─ 強制スリープ要求
    │         └─ if (u1a_lin_slp_req == U1G_LIN_SLP_REQ_FORCE)
    │              └─ l_ifc_sleep_ch1()
    │
    ├─【ON/PASSIVE/SLEEP状態】
    │    └─ Wake-up要因確認
    │         └─ if (u1a_lin_slp_req == U1G_LIN_SLP_REQ_OFF)
    │              ├─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_WAKE
    │              ├─ u2l_lin_nm_wurty_cnt = 0
    │              ├─ u2l_lin_nm_wurty_rty = 0
    │              └─ Wake-up信号送信準備
    │
    └─【ON/PASSIVE/WAKE状態】
         ├─ Wake-up信号送信処理
         │    ├─ u2l_lin_nm_wurty_cnt++
         │    └─ if (u2l_lin_nm_wurty_cnt >= U2G_LIN_TM_WURTY)  ← 50ms
         │         ├─ l_ifc_wake_up_ch1()  ← Wake-up信号送信
         │         ├─ u2l_lin_nm_wurty_cnt = 0
         │         └─ u2l_lin_nm_wurty_rty++
         │
         ├─ マスタ応答確認
         │    └─ フレーム受信検出
         │         ├─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_ACTIVE
         │         └─ u1l_lin_nm_msterr = CLR
         │
         └─ Wake-upリトライ処理
              └─ if (u2l_lin_nm_wurty_rty >= U2G_LIN_CNT_RETRY)  ← 2回
                   ├─ u2l_lin_nm_3brks_cnt++
                   └─ if (u2l_lin_nm_3brks_cnt >= U2G_LIN_TM_3BRKS)  ← 1500ms
                        ├─ u1l_lin_nm_msterr_cnt++
                        ├─ マスタ異常判定
                        │    └─ if (u1l_lin_nm_msterr_cnt >= U1G_LIN_CNT_MSTERR)  ← 3回
                        │         └─ u1l_lin_nm_msterr = SET
                        └─ u1l_lin_slv_stat = U1G_LIN_SLVSTAT_SLEEP
```

### 4. Wake-up信号送信処理

```
l_ifc_wake_up_ch1()
    ↓
    [SLIN CORE層]
    ├─ 1. UART受信禁止
    │    └─ l_vog_lin_rx_dis()
    │
    ├─ 2. Wake-upデータ送信
    │    └─ l_vog_lin_tx_char(U1G_LIN_SND_WAKEUP_DATA)
    │         ├─ 9600bps: 0x80 (1000 0000b) → 250μs〜5ms のドミナントパルス
    │         ├─ 19200bps: 0x80
    │         └─ 2400bps: 0xFF
    │
    └─ 3. UART受信許可
         └─ l_vog_lin_rx_enb()
```

### 5. スリープ移行処理

```
l_ifc_sleep_ch1()
    ↓
    [SLIN CORE層]
    ├─ 1. UART受信禁止
    │    └─ l_vog_lin_rx_dis()
    │
    ├─ 2. タイマ停止
    │    └─ l_vog_lin_frm_tm_stop()
    │
    ├─ 3. 外部割り込み許可 (Wake-up検出用)
    │    └─ l_vog_lin_int_enb_wakeup()
    │         ├─ IRQ0エッジ極性設定 (立ち下がり)
    │         └─ IRQ0割り込み許可
    │
    └─ 4. スレーブタスク状態更新
         └─ u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT
```

### 6. NM用タイマ値設定

| パラメータ | 値 | 説明 |
|-----------|------|------|
| U2G_LIN_TM_SLVST | 550ms | Wake-up信号送信許可時間 - 初期化完了時間 |
| U2G_LIN_TM_WURTY | 50ms | Wake-up信号リトライ時間 |
| U2G_LIN_TM_3BRKS | 1500ms | TIME-OUT AFTER THREE BREAKS |
| U2G_LIN_CNT_RETRY | 2回 | Wake-upリトライ回数 |
| U2G_LIN_TM_DATA | 500ms | Data.indビット設定時間 |
| U2G_LIN_NM_TIME_BASE | 6ms | NM用タイムベース時間 |
| U1G_LIN_CNT_MSTERR | 3回 | マスタ異常検出までのWake-up失敗回数 |
| U2G_LIN_TM_TIMEOUT | 2600ms (9600bps) | Bus Idle Time-out (25000bit長) |

---

## エラーハンドリング

### 1. エラーの種類

#### ヘッダエラー (Header Error)

| エラービット | マクロ名 | 内容 |
|------------|---------|------|
| bit 12 | F1g_lin_header_err_timeout_ch1 | ヘッダタイムアウトエラー |
| bit 13 | F1g_lin_header_err_uart_ch1 | UARTエラー（フレーミング/オーバーラン） |
| bit 14 | F1g_lin_header_err_synch_ch1 | Synch Fieldエラー（0x55不一致） |
| bit 15 | F1g_lin_header_err_parity_ch1 | IDパリティエラー |
| bit 12-15 | F4g_lin_header_err_ch1 | ヘッダエラー一括読み取り（0x0〜0xF） |

#### レスポンスエラー (Response Error)

| エラービット | 値 | 内容 |
|------------|-----|------|
| bit 0 | 0x01 | 応答なしエラー（レスポンスタイムアウト） |
| bit 1 | 0x02 | UARTエラー（フレーミング/オーバーラン/パリティ） |
| bit 2 | 0x04 | ビットエラー（送信リードバック不一致） |
| bit 3 | 0x08 | チェックサムエラー |
| bit 0-3 | 0x0〜0xF | レスポンスエラー一括読み取り |

#### Physical Busエラー

| エラービット | マクロ名 | 内容 |
|------------|---------|------|
| bit 11 | F1g_lin_p_bus_err_ch1 | Physical Busエラー（25000bitタイム無通信） |

### 2. エラー検出フロー

```
[UART受信割り込み]
l_vog_lin_rx_int(u1a_lin_data, u1a_lin_err)
    ↓
    ├─【UARTエラー検出】
    │    ├─ フレーミングエラー (SSR bit4 = 1)
    │    │    └─ Synch Break検出以外はエラー
    │    │         ├─ ヘッダ受信中 → F1g_lin_header_err_uart_ch1 = SET
    │    │         └─ データ受信中 → F4g_lin_errframe_idXXh |= 0x02
    │    │
    │    ├─ オーバーランエラー (SSR bit5 = 1)
    │    │    ├─ ヘッダ受信中 → F1g_lin_header_err_uart_ch1 = SET
    │    │    └─ データ受信中 → F4g_lin_errframe_idXXh |= 0x02
    │    │
    │    └─ パリティエラー (SSR bit3 = 1)
    │         ├─ ヘッダ受信中 → F1g_lin_header_err_uart_ch1 = SET
    │         └─ データ受信中 → F4g_lin_errframe_idXXh |= 0x02
    │
    ├─【Synch Fieldエラー検出】
    │    └─ 受信データ != 0x55
    │         └─ F1g_lin_header_err_synch_ch1 = SET
    │
    ├─【IDパリティエラー検出】
    │    └─ l_u1g_lin_id_parity_chk(u1a_lin_id) == NG
    │         └─ F1g_lin_header_err_parity_ch1 = SET
    │
    ├─【チェックサムエラー検出】
    │    └─ 受信チェックサム != 計算チェックサム
    │         └─ F4g_lin_errframe_idXXh |= 0x08
    │
    └─【ビットエラー検出】(送信時)
         └─ l_u1g_lin_read_back(送信データ) == NG
              └─ F4g_lin_errframe_idXXh |= 0x04
```

```
[タイマ割り込み]
l_vog_lin_tm_int()
    ↓
    ├─【ヘッダタイムアウトエラー検出】
    │    └─ u1l_lin_frm_tm_cnt >= U1G_LIN_HEADER_MAX_TIME (49カウント ≈ 2.4ms)
    │         ├─ F1g_lin_header_err_timeout_ch1 = SET
    │         └─ スレーブタスク状態 = BREAK_UART_WAIT
    │
    ├─【レスポンスタイムアウトエラー検出】
    │    └─ u1l_lin_frm_tm_cnt >= レスポンス最大時間
    │         ├─ F4g_lin_errframe_idXXh |= 0x01
    │         └─ スレーブタスク状態 = BREAK_UART_WAIT
    │
    └─【Physical Busエラー検出】
         └─ u2l_lin_bus_tm_cnt >= U2G_LIN_HERR_LIMIT (510カウント ≈ 25000bitタイム)
              └─ F1g_lin_p_bus_err_ch1 = SET
```

### 3. エラーリカバリ処理

#### ヘッダエラー時

```
ヘッダエラー検出
    ↓
    ├─ エラーフラグセット
    ├─ スレーブタスク状態 = BREAK_UART_WAIT
    ├─ 次のSynch Break待ち
    └─ 該当フレームはスキップ（受信/送信中止）
```

#### レスポンスエラー時

```
レスポンスエラー検出
    ↓
    ├─ エラーフラグセット (F4g_lin_errframe_idXXh)
    ├─ 同期フラグセット (F1g_lin_rxframe_idXXh or F1g_lin_txframe_idXXh)
    │    └─ アプリケーション層へエラー通知
    ├─ スレーブタスク状態 = BREAK_UART_WAIT
    └─ 次のフレーム受信準備
```

#### Physical Busエラー時

```
Physical Busエラー検出
    ↓
    ├─ F1g_lin_p_bus_err_ch1 = SET
    ├─ ネットワーク管理へ通知
    └─ f_vog_lin_mng_ctrl()内で処理
         ↓
         ├─ u1a_slp_req = U1G_LIN_SLP_REQ_FORCE (強制スリープ要求)
         └─ l_nm_tick_ch1() → スリープ移行
```

### 4. アプリケーション層でのエラー処理

```
f_vog_ecu1st_get() (受信データ取得処理)
    ↓
    ├─ 1. 受信完了確認
    │    └─ if (F1g_lin_rxframe_id28h == SET)
    │
    ├─ 2. エラーチェック
    │    └─ if (F4g_lin_errframe_id28h != 0x00)
    │         ↓
    │         ├─ エラーフラグクリア
    │         │    └─ F4g_lin_errframe_id28h = 0x00
    │         │
    │         └─ エラー処理
    │              ├─ データは使用しない
    │              ├─ 前回値を保持
    │              └─ 通信途絶タイマは継続
    │
    └─ 3. 通信途絶判定
         └─ if (p_u1g_tmr_jdg_id28_cmdwn() == TRUE)
              ↓
              ├─ F_comdwn_01 = SET
              ├─ F_id28_first = SET
              └─ デフォルト値設定
                   ├─ F_lin_ig = OFF
                   └─ F_lin_pws = OFF
```

### 5. エラー統計情報

**エラーカウンタ (アプリケーション実装推奨)**

- ヘッダエラーカウント
- レスポンスエラーカウント（フレーム毎）
- Physical Busエラーカウント
- 通信途絶回数（フレーム毎）

**エラーログ記録 (デバッグ用)**

```c
typedef struct {
    u1 u1_error_type;      // エラー種別
    u1 u1_frame_id;        // フレームID
    u1 u1_error_code;      // エラーコード
    u4 u4_timestamp;       // タイムスタンプ
} st_lin_error_log_t;
```

---

## フレーム定義

### 1. LINフレームID一覧

| フレーム名 | ID | 方向 | データ長 | 説明 | スロット番号 |
|----------|-----|------|---------|------|------------|
| ボディECU情報① | 0x28 | 受信 | 4 byte | IG SW、P/W作動許可、リモート信号 | 0 |
| ボディECU情報② | 0x29 | 受信 | 4 byte | ハンドル情報、仕向情報、車速 | 1 |
| ボディECU情報③ | 0x2A | 受信 | 4 byte | (予約) | 2 |
| ボディECU情報④ | 0x22 | 受信 | 4 byte | (予約) | 3 |
| P/WマスタSW情報① | 0x2B | 受信 | 4 byte | ウインドロック信号 | 4 |
| S/R情報 | 0x23 | 送信 | 4 byte | S/Rウォーニング信号 | 5 |
| D席P/W情報① | 0x24 | 受信 | 4 byte | カスタマイズ情報 | 6 |
| ダイアグ要求 | 0x3C | 受信 | 8 byte | ダイアグノーシス要求 | 7 |
| ダイアグ応答 | 0x3D | 送信 | 8 byte | ダイアグノーシス応答 (デフォルト無効) | 8 |

### 2. フレームデータ構造

#### ID28h: ボディECU情報①

```
Byte 0: [予約]
Byte 1: bit 0   : u1g_flb_kpwu   (キー連動P/W UP信号)
        bit 1   : u1g_flb_kpwd   (キー連動P/W DOWN信号)
        bit 2   : u1g_flb_wpwu   (ワイヤレス連動P/W UP信号)
        bit 3   : u1g_flb_wpwd   (ワイヤレス連動P/W DOWN信号)
        bit 4   : u1g_flb_spwu   (スマート連動P/W UP信号)
        bit 5   : [予約]
        bit 6   : u1g_flb_pws    (P/W作動許可信号)
        bit 7   : u1g_flb_ig     (ボディECU IG SW信号)
Byte 2: bit 0   : u1g_flb_dcty   (D席カーテシSW信号)
        bit 1-7 : [予約]
Byte 3: [予約]

アプリケーションマクロ:
- F_lin_ig    : IGスイッチ状態
- F_lin_pws   : P/W作動許可状態
- F_lin_dcty  : D席カーテシSW状態
- F_lin_kpwu/kpwd : キー連動P/W信号
- F_lin_wpwu/wpwd : ワイヤレス連動P/W信号
- F_lin_spwu  : スマート連動P/W UP信号
```

#### ID29h: ボディECU情報②

```
Byte 0: bit 0-1 : u1g_dat_rl     (ハンドル情報)
                  0x01 = 左ハンドル
                  0x02 = 右ハンドル
        bit 2-7 : [予約]
Byte 1: u1g_dat_cmt (仕向情報)
        0x00 = 日本市販
        0x41 = 北米
        0x57 = 欧州
        0x56 = 中近東
        0x43 = 中国
        0xF0 = 一般輸出
        0x51 = 豪州
        0xFF = 不定
Byte 2: u1g_dat_spd (車速情報) [km/h]
Byte 3: u1g_dat_amd (外気温) [℃]

アプリケーションマクロ:
- U1g_lin_cmt : 仕向け情報 → U1G_DEST_JPN/USA等に変換
- U1g_lin_spd : 車速情報
```

#### ID23h: S/R情報 (送信)

```
Byte 0: bit 0-3 : [予約]
        bit 4   : u1g_flb_warn_srbz (S/Rウォーニング信号)
        bit 5-7 : [予約]
Byte 1-3: [予約]

アプリケーションマクロ:
- Fll_lin_srbz : S/Rブザー要求状態
```

#### ID2Bh: P/WマスタSW情報①

```
Byte 0-3: フレームデータ

アプリケーションマクロ:
- F_lin_wl : ウインドロック信号
```

#### ID24h: D席P/W情報① (カスタマイズ情報)

```
Byte 0: [予約]
Byte 1: bit 0   : u1g_flb_kupc (カスタマイズ: キー連動P/W UP有無)
        bit 1   : u1g_flb_kdnc (カスタマイズ: キー連動P/W DN有無)
        bit 2   : [予約]
        bit 3   : u1g_flb_jpc  (カスタマイズ: 挟み込み防止ロジック)
        bit 4   : u1g_flb_wupc (カスタマイズ: ワイヤレス連動P/W UP有無)
        bit 5   : u1g_flb_wdnc (カスタマイズ: ワイヤレス連動P/W DN有無)
        bit 6   : u1g_flb_supc (カスタマイズ: スマート連動P/W UP有無)
        bit 7   : [予約]
Byte 2-3: [予約]
```

### 3. フレームスケジュール

LINマスタ（ボディECU）が以下のスケジュールでフレームを送信すると想定：

```
LINバススケジュール (推定):
  ┌───────┬───────┬───────┬───────┬───────┬───────┐
  │ ID28h │ ID29h │ ID2Ah │ ID22h │ ID2Bh │ ID23h │ ...
  └───────┴───────┴───────┴───────┴───────┴───────┘
    10ms    10ms    10ms    10ms    10ms    10ms

  周期: 約10ms/フレーム (100Hz)
  フレーム期間: 約60ms (6フレーム)
```

### 4. チェックサム計算

**Classic Checksum (LIN 1.x仕様):**

```
チェックサム = ~(Data0 + Data1 + ... + DataN)

例: ID28h, Data = [0x12, 0x34, 0x56, 0x78]
  Sum = 0x12 + 0x34 + 0x56 + 0x78 = 0x114
  Checksum = ~0x14 = 0xEB
```

**実装 (l_slin_cmn.c):**
```c
l_u8 l_u1g_lin_checksum(l_u8* pta_lin_data, l_u8 u1a_lin_cnt) {
    l_u16 u2l_lin_sum = 0;
    for (l_u8 i = 0; i < u1a_lin_cnt; i++) {
        u2l_lin_sum += pta_lin_data[i];
    }
    return (l_u8)(~u2l_lin_sum);  // 下位8bitの反転
}
```

### 5. IDパリティ計算

**Protected ID = ID(6bit) + P1(1bit) + P0(1bit)**

```
P0 = ID0 XOR ID1 XOR ID2 XOR ID4
P1 = NOT (ID1 XOR ID3 XOR ID4 XOR ID5)

例: ID = 0x28 (0b101000)
  ID5 ID4 ID3 ID2 ID1 ID0
   1   0   1   0   0   0

  P0 = 0 XOR 0 XOR 0 XOR 0 = 0
  P1 = NOT (0 XOR 1 XOR 0 XOR 1) = NOT 0 = 1

  Protected ID = 0b10101000 = 0xA8
```

---

## 状態遷移図

### 1. スレーブタスク状態遷移

```
┌─────────────────────────────────────────────────────────┐
│  BREAK_UART_WAIT (0)                                    │
│  - Synch Break待ち（UART経由）                           │
│  - 通常運用時の待機状態                                   │
└─────────────────────────────────────────────────────────┘
         ↓ [Synch Break検出]
         ↓ (0x00 + Framing Error)
┌─────────────────────────────────────────────────────────┐
│  SYNCHFIELD_WAIT (2)                                    │
│  - Synch Field待ち (0x55)                               │
│  - ビットタイミング同期                                   │
└─────────────────────────────────────────────────────────┘
         ↓ [Synch Field受信]
         ↓ (0x55)
┌─────────────────────────────────────────────────────────┐
│  IDENTFIELD_WAIT (3)                                    │
│  - Identifier Field待ち                                 │
│  - IDパリティチェック                                    │
│  - スロット検索                                          │
└─────────────────────────────────────────────────────────┘
         ↓
    ┌────┴────┐
    │         │
[受信]     [送信]
    │         │
    ↓         ↓
┌────────┐ ┌────────┐
│RCVDATA │ │SNDDATA │
│WAIT(4) │ │WAIT(5) │
└────────┘ └────────┘
    │         │
    │         │
    └────┬────┘
         ↓ [フレーム完了]
┌─────────────────────────────────────────────────────────┐
│  BREAK_UART_WAIT (0)                                    │
│  - 次のフレーム待ち                                       │
└─────────────────────────────────────────────────────────┘

[スリープ時]
┌─────────────────────────────────────────────────────────┐
│  BREAK_IRQ_WAIT (1)                                     │
│  - Synch Break待ち（IRQ経由）                            │
│  - 外部割り込みでWake-up検出                              │
└─────────────────────────────────────────────────────────┘
         ↓ [Wake-up信号検出]
         ↓ (IRQ0割り込み)
┌─────────────────────────────────────────────────────────┐
│  BREAK_UART_WAIT (0)                                    │
│  - UART受信許可                                          │
│  - 通常動作へ復帰                                         │
└─────────────────────────────────────────────────────────┘
```

### 2. ネットワーク管理状態遷移

```
[システムリセット]
    ↓
┌─────────────────────────────────────────────────────────┐
│  ON/PASSIVE/WAKE-WAIT (PON: 0)                          │
│  - 初期化直後                                            │
│  - 550ms待機                                             │
└─────────────────────────────────────────────────────────┘
    ↓ [550ms経過]
┌─────────────────────────────────────────────────────────┐
│  ON/PASSIVE/WAKE-WAIT (WAKE_WAIT: 1)                    │
│  - LINバス監視中                                         │
│  - Bus Idleタイムアウト監視 (25000bit = 2.6秒 @ 9600bps) │
└─────────────────────────────────────────────────────────┘
    ↓                              ↓
 [フレーム受信]             [Bus Idle Timeout]
    ↓                              ↓
┌──────────────────┐    ┌──────────────────────────────┐
│  ON/ACTIVE (5)   │    │  ON/PASSIVE/SLEEP (2)        │
│  - 通常通信中     │    │  - スリープ状態               │
│  - データ送受信   │    │  - UART受信禁止               │
└──────────────────┘    │  - IRQ0割り込み待機           │
    ↓                   │  - 低消費電力モード           │
 [スリープ要求]          └──────────────────────────────┘
    └─────────────────────┘      ↑
                                 │
                          [Wake-up要因発生]
                                 ↓
                    ┌──────────────────────────────┐
                    │  ON/PASSIVE/WAKE (3)         │
                    │  - Wake-up信号送信中         │
                    │  - 50ms毎にWake-up送信       │
                    │  - マスタ応答待ち (最大2回)   │
                    └──────────────────────────────┘
                         ↓                ↓
                  [マスタ応答]      [リトライ失敗]
                         ↓                ↓
                    ON/ACTIVE        ON/PASSIVE/SLEEP
```

### 3. 通信途絶検出状態遷移

```
[通常通信中]
    ↓
    ├─ フレーム受信正常
    │   ├─ F_comdwn_XX = CLR
    │   └─ タイマリセット
    │
    └─ フレーム受信なし/エラー
         ↓
        [途絶判定タイマカウントアップ]
         ↓
         └─ タイムアウト（例: ID28h → 500ms想定）
              ↓
              ├─ F_comdwn_XX = SET
              ├─ F_idXX_first = SET
              └─ デフォルト値設定
                   ↓
                  [途絶状態]
                   ↓
              フレーム受信再開
                   ↓
              ├─ F_comdwn_XX = CLR
              ├─ F_idXX_first = CLR
              └─ タイマリセット
                   ↓
                [通常通信復帰]
```

### 4. エラー処理状態遷移

```
[フレーム受信中]
    ↓
    ├─【正常完了】
    │   ├─ F1g_lin_rxframe_idXXh = SET
    │   ├─ F4g_lin_errframe_idXXh = 0x00
    │   └─ BREAK_UART_WAIT状態へ
    │
    ├─【ヘッダエラー】
    │   ├─ Timeout / UART / Synch / Parity
    │   ├─ F1g_lin_header_err_XXX = SET
    │   ├─ BREAK_UART_WAIT状態へ
    │   └─ 該当フレームスキップ
    │
    └─【レスポンスエラー】
        ├─ No Response / UART / Bit / Checksum
        ├─ F1g_lin_rxframe_idXXh = SET
        ├─ F4g_lin_errframe_idXXh = エラービット
        └─ BREAK_UART_WAIT状態へ

[アプリケーション層]
    ↓
    ├─ エラーなし (0x00)
    │   └─ データ使用
    │
    └─ エラーあり (0x01〜0x0F)
        ├─ エラーフラグクリア
        ├─ データ破棄
        └─ 前回値保持
```

---

## まとめ

### システムの特徴

1. **トヨタ自動車標準LIN仕様準拠**: 車載ネットワークの信頼性・互換性を確保

2. **階層化アーキテクチャ**: アプリケーション層、API層、CORE層、DRV層に分離され、保守性が高い

3. **ロバストなエラーハンドリング**:
   - ヘッダエラー/レスポンスエラーの詳細検出
   - 通信途絶監視タイマ
   - Physical Busエラー検出

4. **低消費電力対応**:
   - スリープ/ウェイクアップ制御
   - Bus Idleタイムアウト監視
   - Wake-up信号送信・検出

5. **リアルタイム性**:
   - 割り込み駆動型処理
   - 最小遅延でのフレーム送受信

### 移植時の注意点

このライブラリをTexas Instruments CC2340R53等の他のマイコンに移植する場合、以下の層の変更が必要です：

#### 変更が必要な層
- **SLIN DRV層** (`l_slin_drv_*.c/h`)
  - UART制御 (SCI → UART2)
  - タイマ制御 (Timer Z0 → CC2340R53タイマ)
  - 外部割り込み制御 (IRQ0 → GPIO割り込み)
  - SFR定義 (`l_slin_sfr_*.h`)

#### 変更不要な層
- **アプリケーション層** (最小限の修正)
- **SLIN API層** (変更不要)
- **SLIN CORE層** (変更不要)
- **SLIN NMC層** (変更不要)

### 参考資料

- LIN Specification 1.3 / 2.0
- トヨタ自動車 LIN通信仕様書
- Renesas H8/3687 Hardware Manual
- LSLib3687T v2.00 Release Note

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: LIN Flow Analysis Tool
