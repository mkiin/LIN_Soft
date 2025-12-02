# CC23xx LIN実装 状態遷移整合性分析

## 実装日: 2025-01-22
## 対象ボーレート: 19200bps

---

## 1. NMC層の状態定義 (l_slin_nmc.c)

### NMスレーブステータス (u1l_lin_mod_slvstat)

| 状態 | 定義値 | 説明 |
|------|-------|------|
| `U1G_LIN_SLVSTAT_PON` | - | 起動後 (Power ON) |
| `U1G_LIN_SLVSTAT_WAKE_WAIT` | - | Wakeup待ち |
| `U1G_LIN_SLVSTAT_SLEEP` | - | スリープ |
| `U1G_LIN_SLVSTAT_WAKE` | - | Wakeup送信中 |
| `U1G_LIN_SLVSTAT_OTHER_WAKE` | - | 他ノードWakeup検出 |
| `U1G_LIN_SLVSTAT_ACTIVE` | - | アクティブ (通信中) |

### LINステータス (u2a_lin_stat)

| 状態 | 定義値 | 説明 |
|------|-------|------|
| `U2G_LIN_STS_SLEEP` | - | スリープ状態 |
| `U2G_LIN_STS_RUN_STANDBY` | - | RUNスタンバイ状態 |
| `U2G_LIN_STS_RUN` | - | RUN状態 (通常通信) |

---

## 2. CORE層のスレーブタスクステータス (l_slin_core_CC2340R53.h/c)

### スレーブタスクステータス (u1l_lin_slv_sts)

| 状態 | 定義値 | 説明 | タイマー使用 |
|------|-------|------|------------|
| `U1G_LIN_SLSTS_BREAK_UART_WAIT` | 0 | Synch Break(UART)待ち | ✅ Bus Timeout |
| `U1G_LIN_SLSTS_BREAK_IRQ_WAIT` | 1 | Synch Break(IRQ)待ち | ✅ Header Timeout |
| `U1G_LIN_SLSTS_SYNCHFIELD_WAIT` | 2 | Synch Field待ち | ✅ Header Timeout |
| `U1G_LIN_SLSTS_IDENTFIELD_WAIT` | 3 | Ident Field待ち | ✅ Header Timeout |
| `U1G_LIN_SLSTS_RCVDATA_WAIT` | 4 | データ受信待ち | ✅ No Response Timeout |
| `U1G_LIN_SLSTS_SNDDATA_WAIT` | 5 | データ送信待ち | ✅ Inter-byte Timer |

---

## 3. 状態遷移フロー

### 3.1 正常な受信フレームの状態遷移

```
[初期状態]
BREAK_UART_WAIT (0)
    ↓ Break検出 (UART受信割り込み)
BREAK_IRQ_WAIT (1) または SYNCHFIELD_WAIT (2)
    ↓ Synch Field受信 (0x55)
IDENTFIELD_WAIT (3)
    ↓ Protected ID受信
RCVDATA_WAIT (4)
    ↓ データフィールド受信完了 (データ + チェックサム)
[フレーム完了]
BREAK_UART_WAIT (0) へ戻る
```

**タイマー使用:**
- `BREAK_UART_WAIT`: Bus Timeout (25000bit)
- `BREAK_IRQ_WAIT`, `SYNCHFIELD_WAIT`, `IDENTFIELD_WAIT`: Header Timeout (149bit)
- `RCVDATA_WAIT`: Response Timeout (データ長 × 1.4)

### 3.2 正常な送信フレームの状態遷移（H850互換実装）

```
[初期状態]
BREAK_UART_WAIT (0)
    ↓ Break検出
SYNCHFIELD_WAIT (2)
    ↓ Synch Field受信
IDENTFIELD_WAIT (3)
    ↓ Protected ID受信（送信フレーム）
    ↓ レスポンススペース待ちタイマー設定 (1bit)
SNDDATA_WAIT (5)  ← **タイマー割り込みで処理**
    ↓ タイマー割り込み (Response Space経過)
    ↓ 1バイト目送信
    ↓ Inter-byte Spaceタイマー設定 (11bit = 10bit + 1bit)
    ↓ タイマー割り込み
    ↓ 1バイト目リードバックチェック → OK
    ↓ 2バイト目送信
    ↓ Inter-byte Spaceタイマー設定 (11bit)
    ↓ ... (全バイト送信まで繰り返し)
    ↓ 全バイト送信完了
[フレーム完了]
BREAK_UART_WAIT (0) へ戻る
```

**タイマー使用:**
- IDフィールド受信後: Response Space (1bit = 52.08us @ 19200bps)
- 各バイト送信後: Inter-byte Space (11bit = 572.9us @ 19200bps)

---

## 4. 追加実装の整合性検証

### 4.1 CORE層タイマー割り込みハンドラ (l_vog_lin_tm_int)

**ファイル:** `l_slin_core_CC2340R53.c:867-995`

**実装した状態分岐:**

```c
void l_vog_lin_tm_int(void)
{
    /*** RUN STANDBY状態の場合 ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY ){
        if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT ){
            /* Physical Busエラー */
        }
    }
    /*** RUN状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN ){
        switch( u1l_lin_slv_sts ){
        case ( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
            /* Physical Busエラー */
            break;
        case ( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
        case ( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
        case ( U1G_LIN_SLSTS_IDENTFIELD_WAIT ):
            /* Header Timeoutエラー */
            break;
        case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            /* No Responseエラー */
            break;
        case ( U1G_LIN_SLSTS_SNDDATA_WAIT ):
            /* ← 新規追加！1バイト単位送信処理 */
            break;
        }
    }
    /*** SLEEP状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP ){
        /* WAKEUPコマンド送信完了 */
    }
}
```

**整合性チェック結果:** ✅ **OK**
- LINステータス (`U2G_LIN_STS_*`) の分岐がNMC層と一致
- スレーブタスクステータス (`U1G_LIN_SLSTS_*`) の分岐が全て定義済み
- 新規追加した `SNDDATA_WAIT` 処理がH850互換

### 4.2 CORE層受信割り込みハンドラ (l_vog_lin_rx_int)

**ファイル:** `l_slin_core_CC2340R53.c:672-843`

**送信フレーム時の状態遷移:**

```c
// IDフィールド受信時 (l_vog_lin_check_header内)
if( xng_lin_slot_tbl[slot].u1g_lin_sndrcv == U1G_LIN_CMD_SND )
{
    // データをtmpバッファにコピー
    u1l_lin_rs_tmp[0..7] = xng_lin_frm_buf[slot].data[0..7];

    // レスポンススペース待ちタイマ設定
    l_vog_lin_bit_tm_set( U1G_LIN_RSSP );  // 1bit

    // データ送信待ち状態へ遷移
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;  // ← ここで状態遷移
}
```

**整合性チェック結果:** ✅ **OK**
- 状態遷移先 `U1G_LIN_SLSTS_SNDDATA_WAIT` が定義済み
- タイマー設定関数 `l_vog_lin_bit_tm_set()` が実装済み
- NMC層のLINステータスと独立して動作

### 4.3 DRV層タイマー関数

**ファイル:** `l_slin_drv_cc2340r53.c:549-684`

**実装した関数:**

1. **l_ifc_tm_ch1()** - タイマー割り込みコールバック
   - LGPTimerLPF3の割り込みを受けて `l_vog_lin_tm_int()` を呼び出し

2. **l_vog_lin_timer_init()** - タイマー初期化
   - LGPT0使用、48MHz駆動、コールバック登録

3. **l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)** - ビットタイマー設定
   - @19200bps: 1bit = 52.08us
   - ワンショットモードで起動

4. **l_vog_lin_frm_tm_stop()** - タイマー停止
   - LGPTimerを停止

**整合性チェック結果:** ✅ **OK**
- H850の `l_vog_lin_bit_tm_set()` と同じインターフェース
- CORE層から呼び出される関数がすべて実装済み
- 19200bpsの計算式が正しい

---

## 5. NMC層との整合性確認

### 5.1 NMC層が使用するCORE層API

NMC層 (`l_nm_tick_ch1`) が呼び出すCORE層関数:

| 関数 | 呼び出し条件 | 影響するステータス |
|------|------------|------------------|
| `l_ifc_run_ch1()` | 起動後、スケジュール開始 | `U2G_LIN_STS_RUN` へ遷移 |
| `l_ifc_wake_up_ch1()` | Wakeup送信 | `U2G_LIN_STS_SLEEP` 維持 |
| `l_ifc_sleep_ch1()` | スリープ移行 | `U2G_LIN_STS_SLEEP` へ遷移 |
| `l_ifc_read_lb_status_ch1()` | ステータス読み取り | 読み取りのみ |

**整合性チェック結果:** ✅ **OK**
- NMC層の状態遷移とCORE層の状態遷移は独立
- NMC層がCORE層のスレーブタスクステータスを直接操作しない
- CORE層のタイマー処理がNMC層に影響しない

### 5.2 LINステータスの整合性

| NMC層のチェック | CORE層の状態 | タイマー割り込み処理 |
|---------------|-------------|-------------------|
| `U2G_LIN_STS_SLEEP` | 任意 | WAKEUPコマンド送信完了処理 |
| `U2G_LIN_STS_RUN_STANDBY` | `BREAK_UART_WAIT` | Physical Busエラー |
| `U2G_LIN_STS_RUN` | `BREAK_UART_WAIT` | Physical Busエラー |
| `U2G_LIN_STS_RUN` | `BREAK_IRQ_WAIT` 等 | Header Timeoutエラー |
| `U2G_LIN_STS_RUN` | `RCVDATA_WAIT` | No Responseエラー |
| `U2G_LIN_STS_RUN` | `SNDDATA_WAIT` | **1バイト送信処理** ← 新規 |

**整合性チェック結果:** ✅ **OK**
- タイマー割り込みハンドラがLINステータスをチェック
- 各状態で適切な処理を実行
- NMC層とCORE層の状態が矛盾しない

---

## 6. 潜在的な問題点と対策

### 問題1: AFTER_SNDDATA_WAIT状態の削除

**旧実装:**
```c
#define U1G_LIN_SLSTS_BEFORE_SNDDATA_WAIT  ((l_u8)5)
#define U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT   ((l_u8)6)  // 削除
```

**新実装:**
```c
#define U1G_LIN_SLSTS_SNDDATA_WAIT  ((l_u8)5)  // H850互換
```

**影響分析:**
- `AFTER_SNDDATA_WAIT` を使用していたコードは削除済み
- 受信割り込みハンドラで `AFTER_SNDDATA_WAIT` の処理を削除
- タイマー割り込みハンドラで `SNDDATA_WAIT` として処理

**対策:** ✅ **完了**
- ヘッダーファイルの定義を修正
- 不要な状態定義を削除

### 問題2: チェックサム変数の型

**旧実装:**
```c
static l_u8  u1l_lin_chksum;  // 8ビット
```

**新実装:**
```c
static l_u16 u2l_lin_chksum;  // 16ビット (H850互換)
```

**影響分析:**
- チェックサム計算でキャリーオーバーが発生
- 8ビット変数ではキャリーが失われる

**対策:** ⚠️ **要確認**
- CORE層の変数定義を確認
- H850と同じ16ビット変数を使用しているか確認

### 問題3: タイマー初期化のタイミング

**実装:**
```c
void l_ifc_init_drv_ch1(void)
{
    l_vog_lin_uart_init();   // UART初期化
    l_vog_lin_timer_init();  // タイマー初期化 ← 追加
    l_vog_lin_int_init();    // GPIO初期化
}
```

**影響分析:**
- タイマー初期化がUART初期化の後
- GPIO初期化の前

**対策:** ✅ **OK**
- 初期化順序は問題なし
- タイマーはUART初期化後でも動作可能

---

## 7. 最終確認項目

### ✅ 完了項目

1. **状態定義の整合性**
   - `U1G_LIN_SLSTS_SNDDATA_WAIT` をヘッダーに追加
   - 不要な `BEFORE_SNDDATA_WAIT`, `AFTER_SNDDATA_WAIT` を削除

2. **タイマー割り込みハンドラ**
   - `l_vog_lin_tm_int()` を実装
   - 全ての状態分岐を実装
   - H850互換の処理を実装

3. **DRV層タイマー関数**
   - `l_vog_lin_timer_init()` を実装
   - `l_vog_lin_bit_tm_set()` を実装 (19200bps対応)
   - `l_vog_lin_frm_tm_stop()` を実装

4. **ドライバ初期化シーケンス**
   - `l_ifc_init_drv_ch1()` にタイマー初期化を追加

5. **送信フレーム処理**
   - IDフィールド受信時にレスポンススペース待ちタイマー設定
   - `SNDDATA_WAIT` 状態へ遷移
   - タイマー割り込みで1バイト送信

### ⚠️ 要確認項目

1. **チェックサム変数の型**
   - `u1l_lin_chksum` → `u2l_lin_chksum` へ変更されているか確認

2. **受信タイムアウトタイマー**
   - `l_vog_lin_rcv_tm_set()` が実装されているか確認
   - H850では使用されているが、CC23xxでの実装状況を確認

3. **バスタイムアウトタイマー**
   - `l_vog_lin_bus_tm_set()` が実装されているか確認

---

## 8. 結論

### 整合性の評価: ✅ **概ね良好**

実装した機能:
1. タイマー割り込みハンドラ (`l_vog_lin_tm_int`) - H850互換
2. DRV層タイマー関数 (19200bps対応)
3. 状態定義の修正 (`SNDDATA_WAIT`)
4. 1バイト単位送信処理

発見した問題と対策:
1. 状態定義の不一致 → ✅ 修正完了
2. チェックサム変数の型 → ⚠️ 要確認
3. 受信タイムアウトタイマー → ⚠️ 実装状況確認必要

### 次のステップ

1. チェックサム変数の型を確認・修正
2. 受信タイムアウトタイマーの実装確認
3. バスタイムアウトタイマーの実装確認
4. 統合テストの実施

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: State Transition Analysis
