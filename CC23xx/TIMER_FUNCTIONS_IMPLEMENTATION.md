# タイマー関数の実装 (l_vog_lin_rcv_tm_set / l_vog_lin_bus_tm_set)

## 実装日
2025-12-01

## 概要

H850互換のタイマー関数2つを実装しました：
1. `l_vog_lin_rcv_tm_set()` - 受信タイムアウトタイマー
2. `l_vog_lin_bus_tm_set()` - Physical Busエラー検出タイマー

---

## 実装内容

### 1. l_vog_lin_rcv_tm_set() - 受信タイムアウトタイマー

#### 目的
データ受信時のタイムアウトを検出するためのタイマー

#### 計算式

**LIN規格の定義:**
```
受信タイムアウト時間 = (Data Length + Checksum) × 1.4
```

**実装:**
```c
void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    l_u32 u4a_lin_timeout_us;
    l_u32 u4a_lin_counter_target;

    /* タイムアウト時間の計算: (bit数 × 1.4) */
    /* 1.4倍 = 14/10 で計算 */
    l_u16 u2a_lin_tmp_bit = (((l_u16)u1a_lin_bit * 14U) / 10U);

    /* マイクロ秒単位の時間に変換 (@19200bps: 1bit = 52.08us) */
    u4a_lin_timeout_us = ((l_u32)u2a_lin_tmp_bit * U4L_LIN_1BIT_TIMERVAL) / 1000UL;

    /* 48MHzクロックでのカウンタターゲット値計算 */
    u4a_lin_counter_target = u4a_lin_timeout_us * 48UL - 1UL;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4a_lin_counter_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

#### 使用例

**フレームサイズ8の場合:**
```c
// SLEEP ID受信時
u1l_lin_frm_sz = U1G_LIN_DL_8;  // 8バイト
l_vog_lin_rcv_tm_set( (l_u8)((u1l_lin_frm_sz + U1G_LIN_1) * U1G_LIN_BYTE_LENGTH) );
//                            ↑                             ↑
//                            8 + 1 = 9                     10bit
//                            (Data + Checksum)
// = l_vog_lin_rcv_tm_set(90)
```

**計算過程:**
```
入力: 90 bit

1. タイムアウト時間計算 (1.4倍)
   90 × 14 / 10 = 126 bit

2. マイクロ秒換算 (@19200bps)
   126 × 52.08us = 6,562us = 6.56ms

3. 48MHzカウンタ値
   6,562us × 48 = 314,976 カウント
```

**結果:** 6.56ms後にタイムアウト割り込み発生 ✅

#### 呼び出し箇所

[l_slin_core_CC2340R53.c:848](l_slin_core_CC2340R53.c#L848)
```c
/* SLEEPコマンドID受信時 */
u1l_lin_frm_sz = U1G_LIN_DL_8;
l_vog_lin_rcv_tm_set( (l_u8)((u1l_lin_frm_sz + U1G_LIN_1) * U1G_LIN_BYTE_LENGTH) );
u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;
```

[l_slin_core_CC2340R53.c:877](l_slin_core_CC2340R53.c#L877)
```c
/* 受信フレーム時 */
u1l_lin_frm_sz = xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_frm_sz;
l_vog_lin_rcv_tm_set( (l_u8)((u1l_lin_frm_sz + U1G_LIN_1) * U1G_LIN_BYTE_LENGTH) );
u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;
```

#### タイムアウト時の動作

タイマー割り込みハンドラ ([l_slin_core_CC2340R53.c:1126-1254](l_slin_core_CC2340R53.c#L1126-L1254))で処理：

```c
void l_vog_lin_tm_int(void)
{
    // RUN状態
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN )
    {
        switch( u1l_lin_slv_sts )
        {
        // データ受信待ち状態でタイムアウト
        case( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            // Physical Busエラーカウンタ更新
            u2l_lin_herr_cnt++;

            // エラー設定
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_tout = U2G_LIN_BIT_SET;
            xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;

            // Synch Break待ち状態へ
            l_vol_lin_set_synchbreak();
            break;
        }
    }
}
```

---

### 2. l_vog_lin_bus_tm_set() - Physical Busエラー検出タイマー

#### 目的
LINバス全体の物理的異常を検出するための長時間タイマー

#### LIN規格の定義

**Physical Busエラーの条件:**
- **25000 bit時間**以内にヘッダーを受信できない場合
- これは約510回のヘッダータイムアウト相当
  ```
  25000 bit / 49 bit (ヘッダー最大時間) = 510.2... ≈ 510回
  ```

#### 実装

```c
void l_vog_lin_bus_tm_set(void)
{
    l_u32 u4a_lin_timeout_us;
    l_u32 u4a_lin_counter_target;
    l_u16 u2a_lin_tmp_bit;

    /* Physical Busエラータイムアウト: 25000 bit */
    u2a_lin_tmp_bit = 25000U;

    /* マイクロ秒単位の時間に変換 (@19200bps: 1bit = 52.08us) */
    /* 25000 bit × 52.08us = 1,302,000 us = 1.302秒 */
    u4a_lin_timeout_us = ((l_u32)u2a_lin_tmp_bit * U4L_LIN_1BIT_TIMERVAL) / 1000UL;

    /* 48MHzクロックでのカウンタターゲット値計算 */
    u4a_lin_counter_target = u4a_lin_timeout_us * 48UL - 1UL;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4a_lin_counter_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

#### 計算過程

```
入力: 25000 bit (固定)

1. マイクロ秒換算 (@19200bps)
   25000 × 52.08us = 1,302,000 us = 1.302秒

2. 48MHzカウンタ値
   1,302,000us × 48 = 62,496,000 カウント
```

**結果:** 約1.3秒後にタイムアウト割り込み発生 ✅

#### 呼び出し箇所

[l_slin_core_CC2340R53.c:1286](l_slin_core_CC2340R53.c#L1286)
```c
static void l_vol_lin_set_synchbreak(void)
{
    l_vog_lin_bus_tm_set();  /* Physical Bus Error検出タイマ起動 */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE, U1L_LIN_UART_HEADER_RXSIZE);
    l_vog_lin_int_enb();
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT;
}
```

**使用目的:**
- Synch Break待ち状態でバス異常を監視
- マスタが停止した場合などを検出
- 1.3秒間Break受信がない → Physical Busエラー

#### タイムアウト時の動作

```c
void l_vog_lin_tm_int(void)
{
    // RUN_STANDBY状態
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY )
    {
        if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT )
        {
            /* Physical Busエラー */
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
            xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
        }
    }

    // RUN状態でも同様の処理
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN )
    {
        switch( u1l_lin_slv_sts )
        {
        case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
            // Physical Busエラーカウンタ更新
            u2l_lin_herr_cnt++;

            // 510回以上のヘッダータイムアウト相当でPhysical Busエラー
            if( u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT )
            {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
                xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
                u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;
            }

            // Synch Break待ちは継続
            break;
        }
    }
}
```

---

## タイマー関数の比較

| 関数 | タイムアウト時間 | 用途 | 呼び出しタイミング |
|------|----------------|------|-------------------|
| `l_vog_lin_bit_tm_set()` | 可変 (1-49 bit) | ヘッダー受信、送信間隔 | Break検出後、各バイト送信後 |
| `l_vog_lin_rcv_tm_set()` | (DL+1)×10×1.4 bit | データ受信タイムアウト | ID受信後（受信フレーム時） |
| `l_vog_lin_bus_tm_set()` | 25000 bit (固定) | Physical Busエラー検出 | Break待ち状態移行時 |

### 19200bpsでの実時間例

| 関数 | 入力 | 計算 | 実時間 |
|------|------|------|--------|
| `l_vog_lin_bit_tm_set(1)` | 1 bit | 1 × 52.08us | 52.08 μs |
| `l_vog_lin_bit_tm_set(39)` | 39 bit | 39 × 52.08us | 2.03 ms |
| `l_vog_lin_rcv_tm_set(90)` | 90 bit | 90×1.4 × 52.08us | 6.56 ms |
| `l_vog_lin_bus_tm_set()` | 25000 bit | 25000 × 52.08us | 1.302 秒 |

---

## H850との互換性

### H850の実装

[H850/slin_lib/l_slin_drv_h83687.c:468-519](../H850/slin_lib/l_slin_drv_h83687.c#L468-L519)

**l_vog_lin_rcv_tm_set (H850):**
```c
void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    l_u16 u2a_lin_tmp_bit;

    /* (Data Length + Checksum) × 1.4 */
    u2a_lin_tmp_bit = ( ((l_u16)u1a_lin_bit * U2G_LIN_14) / U2G_LIN_10 );

    // H83687固有のタイマーレジスタ設定
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = u2a_lin_tmp_bit * u2l_lin_tm_bit;
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_SET;
}
```

**l_vog_lin_bus_tm_set (H850):**
```c
void l_vog_lin_bus_tm_set(void)
{
    l_u16 u2a_lin_tmp_bit;

    u2a_lin_tmp_bit = U2G_LIN_BUS_TIMEOUT;  // 25000 bit

    // H83687固有のタイマーレジスタ設定
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = u2a_lin_tmp_bit * u2l_lin_tm_bit;
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_SET;
}
```

### CC23xxの実装の違い

| 項目 | H850 | CC23xx |
|-----|------|--------|
| **タイマーハードウェア** | H83687 TMR | LGPTimerLPF3 |
| **計算ロジック** | 同じ (1.4倍、25000bit) | 同じ |
| **タイマーモード** | フリーラン + コンペアマッチ | ワンショットモード |
| **クロック** | H83687固有 | 48MHz |

**結果:** 計算ロジックは完全互換、ハードウェア制御のみ異なる ✅

---

## 修正ファイル一覧

### 1. l_slin_drv_cc2340r53.h

**追加内容:**
```c
extern void  l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit);  /* 受信タイムアウトタイマー */
extern void  l_vog_lin_bus_tm_set(void);              /* Physical Busエラー検出タイマー */
```

### 2. l_slin_drv_cc2340r53.c

**追加内容:**
- `l_vog_lin_rcv_tm_set()` 関数実装 (709-732行目)
- `l_vog_lin_bus_tm_set()` 関数実装 (753-777行目)

### 3. l_slin_core_CC2340R53.c

**修正内容:**
- `l_vol_lin_set_synchbreak()` 関数に `l_vog_lin_bus_tm_set()` 呼び出しを追加 (1286行目)

---

## 動作確認項目

実装後、以下を確認してください：

### 1. 受信タイムアウトの確認

```
シナリオ: データ受信中にマスタが停止

1. ID Field受信
2. l_vog_lin_rcv_tm_set(90) 呼び出し
   → 6.56ms後にタイムアウト設定
3. データ受信開始
4. ★マスタ停止★ (データ途中で止まる)
5. 6.56ms経過
6. タイマー割り込み発生
   → u2g_lin_e_tout エラーフラグセット
   → Break待ち状態へ復帰

期待結果: ✅ タイムアウトエラー検出
```

### 2. Physical Busエラーの確認

```
シナリオ: マスタ完全停止

1. Break待ち状態
2. l_vog_lin_bus_tm_set() 呼び出し
   → 1.302秒後にタイムアウト設定
3. ★マスタ完全停止★ (Breakが来ない)
4. 1.302秒経過
5. タイマー割り込み発生
   → u2g_lin_phy_bus_err フラグセット
   → u2g_lin_bus_err フラグセット

期待結果: ✅ Physical Busエラー検出
```

### 3. 正常動作の確認

```
シナリオ: 正常なデータ受信

1. ID Field受信
2. l_vog_lin_rcv_tm_set(90) 呼び出し
   → 6.56ms後にタイムアウト設定
3. データ受信開始 (1バイトずつ)
4. 全データ + チェックサム受信完了
   → タイマー停止 (l_vol_lin_set_synchbreak内でl_vog_lin_bus_tm_set再起動)
5. タイムアウト発生しない

期待結果: ✅ タイムアウトなしで正常受信
```

---

## まとめ

### 実装した機能

1. ✅ **l_vog_lin_rcv_tm_set()** - 受信タイムアウト検出
   - LIN規格準拠 (1.4倍マージン)
   - データ長に応じた可変タイムアウト
   - H850完全互換

2. ✅ **l_vog_lin_bus_tm_set()** - Physical Busエラー検出
   - LIN規格準拠 (25000 bit)
   - バス物理異常の検出
   - H850完全互換

### 期待される効果

- **信頼性向上**: マスタ故障時の検出が可能
- **エラーハンドリング**: タイムアウトの適切な処理
- **H850互換性**: 既存アーキテクチャとの一貫性維持

---

## 参照

- 実装箇所:
  - [l_slin_drv_cc2340r53.c:709-777](l_slin_drv_cc2340r53.c#L709-L777)
  - [l_slin_core_CC2340R53.c:1286](l_slin_core_CC2340R53.c#L1286)
- H850参照実装:
  - [H850/slin_lib/l_slin_drv_h83687.c:468-547](../H850/slin_lib/l_slin_drv_h83687.c#L468-L547)
- 関連ドキュメント:
  - [TIMER_MODE_SELECTION_RATIONALE.md](TIMER_MODE_SELECTION_RATIONALE.md)
  - [HEADER_TIMEOUT_CALCULATION.md](HEADER_TIMEOUT_CALCULATION.md)
