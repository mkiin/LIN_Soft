# LGPTimerを使用したLINタイムアウトエラー検出の実装ガイド

## 目次

1. [LGPTimerLPF3の概要](#lgptimerlpf3の概要)
2. [LGPTimerの基本的な使用方法](#lgptimerの基本的な使用方法)
3. [H850のタイマー割り込み処理](#h850のタイマー割り込み処理)
4. [LINタイムアウトエラーの種類](#linタイムアウトエラーの種類)
5. [CC23xxでの実装方法](#cc23xxでの実装方法)
6. [完全な実装例](#完全な実装例)

---

## LGPTimerLPF3の概要

### 特徴

- **16/24/32ビットカウンタ** (ペリフェラルインスタンスにより異なる)
  - LGPT0/1/2: 16ビット
  - LGPT3: 24ビット (CC2340R5) / 32ビット (CC27XX)

- **3つのカウントモード**
  - **Oneshot Up**: ターゲット値に達したら自動停止
  - **Periodic Up**: ターゲット値に達したら0から再スタート
  - **Periodic Up/Down**: ターゲット値とゼロを往復

- **高精度タイミング**
  - クロックソース: CLKSVT (システムクロック: 48 MHz)
  - プリスケーラによる分周可能

- **割り込みサポート**
  - Target interrupt (`LGPTimerLPF3_INT_TGT`): カウンタがターゲット値に達した時
  - Zero interrupt (`LGPTimerLPF3_INT_ZERO`): カウンタが0に戻った時
  - Channel compare/capture interrupts

---

## LGPTimerの基本的な使用方法

### 初期化と周期タイマーの例

```c
#include <ti/drivers/timer/LGPTimerLPF3.h>

/* タイマーハンドル */
LGPTimerLPF3_Handle lgptHandle;

/**
 * @brief タイマー割り込みコールバック
 */
void timerCallback(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask)
{
    if (interruptMask & LGPTimerLPF3_INT_TGT)
    {
        /* ターゲット値に達した時の処理 */
        // LINのタイムアウトチェックを実行
        l_vog_lin_tm_int();
    }
}

/**
 * @brief タイマー初期化
 */
void initLINTimer(void)
{
    LGPTimerLPF3_Params params;
    uint32_t counterTarget;

    /* パラメータ初期化 */
    LGPTimerLPF3_Params_init(&params);
    params.hwiCallbackFxn = timerCallback;

    /* タイマーオープン（LGPT0を使用） */
    lgptHandle = LGPTimerLPF3_open(0, &params);

    if (lgptHandle == NULL)
    {
        /* エラー処理 */
        while (1);
    }

    /* カウンタターゲット設定 */
    /* 例: 1ms周期 @ 48MHz
     * counterTarget = (48MHz / 1000) - 1 = 47999
     */
    counterTarget = 48000 - 1;
    LGPTimerLPF3_setInitialCounterTarget(lgptHandle, counterTarget, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(lgptHandle, LGPTimerLPF3_INT_TGT);

    /* カウントアップ周期モードで開始 */
    LGPTimerLPF3_start(lgptHandle, LGPTimerLPF3_CTL_MODE_UP_PER);
}
```

### ワンショットタイマーの例

```c
/**
 * @brief ワンショットタイマー設定
 * @param timeout_us タイムアウト時間（マイクロ秒）
 */
void setOneshotTimer(uint32_t timeout_us)
{
    uint32_t counterTarget;

    /* タイマー停止 */
    LGPTimerLPF3_stop(lgptHandle);

    /* カウンタターゲット計算 */
    /* 48MHz = 48カウント/us */
    counterTarget = (timeout_us * 48) - 1;

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(lgptHandle, counterTarget, true);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(lgptHandle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

### タイマー停止

```c
/**
 * @brief タイマー停止
 */
void stopTimer(void)
{
    LGPTimerLPF3_stop(lgptHandle);
}
```

### タイマークローズ

```c
/**
 * @brief タイマークローズ
 */
void closeTimer(void)
{
    LGPTimerLPF3_close(lgptHandle);
}
```

---

## H850のタイマー割り込み処理

### H850のタイマー割り込み仕様

H850では、タイマー割り込み(`l_vog_lin_tm_int`)で以下のエラー検出を行っています：

**[H850/slin_lib/l_slin_core_h83687.c:1105-1240](H850/slin_lib/l_slin_core_h83687.c#L1105-L1240)**

### タイムアウトエラーの種類と検出条件

| エラー種類 | 検出状態 | 処理内容 |
|-----------|---------|---------|
| **Header Timeout** | `BREAK_IRQ_WAIT`<br>`SYNCHFIELD_WAIT`<br>`IDENTFIELD_WAIT` | ヘッダータイムアウトエラー設定<br>Physical Busエラーカウント |
| **No Response (Slave Not Responding)** | `RCVDATA_WAIT` | No Responseエラー設定<br>フレーム完了（エラー） |
| **Physical Bus Error** | `BREAK_UART_WAIT` | Physical Busエラー設定 |

### H850のタイマー割り込み処理フロー

```
[タイマー割り込み発生]
    ↓
┌────────────────────────────────────┐
│ LINステータス確認                   │
│ - SLEEP                             │
│ - RUN_STANDBY                       │
│ - RUN                               │
└────────────────────────────────────┘
    ↓
┌────────────────────────────────────┐
│ スレーブタスクステータス確認         │
│ - BREAK_UART_WAIT                   │
│ - BREAK_IRQ_WAIT                    │
│ - SYNCHFIELD_WAIT                   │
│ - IDENTFIELD_WAIT                   │
│ - RCVDATA_WAIT                      │
│ - SNDDATA_WAIT                      │
└────────────────────────────────────┘
    ↓
┌────────────────────────────────────┐
│ エラー判定・処理                    │
└────────────────────────────────────┘
```

---

## LINタイムアウトエラーの種類

### 1. Header Timeout Error (ヘッダータイムアウトエラー)

**発生条件:**
- Synch Break受信後、Synch Fieldが受信されない
- Synch Field受信後、Protected IDが受信されない

**検出状態:**
- `U1G_LIN_SLSTS_BREAK_IRQ_WAIT`
- `U1G_LIN_SLSTS_SYNCHFIELD_WAIT`
- `U1G_LIN_SLSTS_IDENTFIELD_WAIT`

**タイムアウト時間:**
```c
/* H850の定義 */
#define U1G_LIN_HEADER_MAX_TIME     ((l_u16)149)  /* ヘッダータイムアウト時間 (bit長) */

/* @9600bps: 1ビット = 104.17us */
/* 149ビット = 15.52ms */
```

**処理内容:**
```c
/* Header Timeout エラー */
xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;
xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time = U2G_LIN_BIT_SET;

/* Physical Busエラーカウント */
if (u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT)
{
    /* Physical Busエラー */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
    xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
    u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;
}
else
{
    u2l_lin_herr_cnt++;
}

/* Synch Break待ち状態へ戻る */
l_vol_lin_set_synchbreak();
```

### 2. No Response Error (Slave Not Responding Error)

**発生条件:**
- Protected ID受信後、データフィールドが受信されない
- マスターがスレーブからの応答を期待しているが、応答がない

**検出状態:**
- `U1G_LIN_SLSTS_RCVDATA_WAIT`

**タイムアウト時間:**
```c
/* データフィールド最大時間 */
/* フレームサイズに応じて可変 */
/* 例: 8バイトデータ + チェックサム = 9バイト */
/* 9バイト × 10ビット/バイト = 90ビット */
/* @9600bps: 90ビット = 9.375ms */
```

**処理内容:**
```c
/* No Response エラー */
xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;

/* エラーありレスポンス完了 */
l_vol_lin_set_frm_complete(U1G_LIN_ERR_ON);

/* Synch Break待ち状態へ戻る */
l_vol_lin_set_synchbreak();
```

### 3. Physical Bus Error (物理バスエラー)

**発生条件:**
- Synch Break待ち状態でタイムアウト発生
- 複数回のHeader Timeoutエラー発生（連続3回以上）

**検出状態:**
- `U1G_LIN_SLSTS_BREAK_UART_WAIT`

**処理内容:**
```c
/* Physical Bus エラー */
xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
```

---

## CC23xxでの実装方法

### l_slin_drv_cc2340r53.c への実装

#### 1. グローバル変数定義

```c
/* タイマーハンドル */
static LGPTimerLPF3_Handle xnl_lin_timer_handle = NULL;

/* タイマー割り込み発生フラグ */
static volatile bool b1l_lin_timer_expired = false;
```

#### 2. タイマーコールバック関数

```c
/**
 * @brief LINタイマー割り込みコールバック
 */
static void linTimerCallback(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask)
{
    if (interruptMask & LGPTimerLPF3_INT_TGT)
    {
        /* タイマー期限切れフラグ設定 */
        b1l_lin_timer_expired = true;

        /* SLIN CORE層のタイマー割り込みハンドラ呼び出し */
        l_vog_lin_tm_int();
    }
}
```

#### 3. タイマー初期化関数

```c
/**
 * @brief LINタイマー初期化
 */
void l_vog_lin_timer_init(void)
{
    LGPTimerLPF3_Params params;

    /* パラメータ初期化 */
    LGPTimerLPF3_Params_init(&params);
    params.hwiCallbackFxn = linTimerCallback;

    /* タイマーオープン（LGPT0を使用） */
    xnl_lin_timer_handle = LGPTimerLPF3_open(0, &params);

    if (xnl_lin_timer_handle == NULL)
    {
        /* エラー処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_TIMER;
    }
}
```

#### 4. ビットタイマー設定関数

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
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    /* ビット長をマイクロ秒に変換 */
    /* @9600bps: 1ビット = 104.17us */
    /* 計算式: timeout_us = u1a_lin_bit * 10000000 / 9600 */
    /*                    = u1a_lin_bit * 1041667 / 10000 */
    timeout_us = ((uint32_t)u1a_lin_bit * 1041667UL) / 10000UL;

    /* カウンタターゲット計算 */
    /* 48MHz = 48カウント/us */
    counterTarget = (timeout_us * 48UL) - 1UL;

    /* オーバーフロー対策（16ビットタイマーの場合） */
    if (counterTarget > 0xFFFFUL)
    {
        counterTarget = 0xFFFFUL;
    }

    /* タイマー期限切れフラグクリア */
    b1l_lin_timer_expired = false;

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, counterTarget, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

#### 5. フレームタイマー停止関数

```c
/**
 * @brief フレームタイマ停止
 */
void l_vog_lin_frm_tm_stop(void)
{
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    /* タイマー期限切れフラグクリア */
    b1l_lin_timer_expired = false;
}
```

#### 6. タイマー期限切れ確認関数（オプション）

```c
/**
 * @brief タイマー期限切れ確認
 * @return true: 期限切れ, false: 期限内
 */
bool l_u1g_lin_timer_expired(void)
{
    return b1l_lin_timer_expired;
}
```

---

## 完全な実装例

### l_slin_drv_cc2340r53.c の修正

```c
/**
 * @file        l_slin_drv_cc2340r53.c (修正版)
 */

#include <ti/drivers/timer/LGPTimerLPF3.h>

/* タイマーハンドル */
static LGPTimerLPF3_Handle xnl_lin_timer_handle = NULL;

/* タイマー割り込み発生フラグ */
static volatile bool b1l_lin_timer_expired = false;

/* ボーレート依存定数（マイクロ秒/ビット × 10000） */
static uint32_t u4l_lin_us_per_bit_x10000;

/**
 * @brief LINタイマー割り込みコールバック
 */
static void linTimerCallback(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask)
{
    if (interruptMask & LGPTimerLPF3_INT_TGT)
    {
        /* タイマー期限切れフラグ設定 */
        b1l_lin_timer_expired = true;

        /* SLIN CORE層のタイマー割り込みハンドラ呼び出し */
        l_vog_lin_tm_int();
    }
}

/**
 * @brief LINタイマー初期化
 */
void l_vog_lin_timer_init(void)
{
    LGPTimerLPF3_Params params;

    /* ボーレート依存定数設定 */
    #if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_2400
        /* 2400bps: 1ビット = 416.67us */
        u4l_lin_us_per_bit_x10000 = 4166667UL;
    #elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
        /* 9600bps: 1ビット = 104.17us */
        u4l_lin_us_per_bit_x10000 = 1041667UL;
    #elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
        /* 19200bps: 1ビット = 52.08us */
        u4l_lin_us_per_bit_x10000 = 520833UL;
    #else
        #error "Unsupported baudrate"
    #endif

    /* パラメータ初期化 */
    LGPTimerLPF3_Params_init(&params);
    params.hwiCallbackFxn = linTimerCallback;
    params.prescalerDiv = 0;  /* プリスケーラなし */

    /* タイマーオープン（LGPT0を使用） */
    xnl_lin_timer_handle = LGPTimerLPF3_open(0, &params);

    if (xnl_lin_timer_handle == NULL)
    {
        /* エラー処理 */
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
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    /* ビット長をマイクロ秒に変換 */
    timeout_us = ((uint32_t)u1a_lin_bit * u4l_lin_us_per_bit_x10000) / 10000UL;

    /* カウンタターゲット計算 */
    /* システムクロック48MHz = 48カウント/us */
    counterTarget = (timeout_us * 48UL);

    /* 0始まりなので-1 */
    if (counterTarget > 0)
    {
        counterTarget -= 1UL;
    }

    /* オーバーフロー対策（16ビットタイマーの場合） */
    if (counterTarget > 0xFFFFUL)
    {
        counterTarget = 0xFFFFUL;
    }

    /* タイマー期限切れフラグクリア */
    b1l_lin_timer_expired = false;

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, counterTarget, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}

/**
 * @brief フレームタイマ停止
 */
void l_vog_lin_frm_tm_stop(void)
{
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    /* タイマー期限切れフラグクリア */
    b1l_lin_timer_expired = false;
}

/**
 * @brief タイマークローズ
 */
void l_vog_lin_timer_close(void)
{
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_close(xnl_lin_timer_handle);
        xnl_lin_timer_handle = NULL;
    }
}
```

### l_slin_drv_cc2340r53.h の修正

```c
/**
 * @file        l_slin_drv_cc2340r53.h (修正版)
 */

/* タイマー関連関数プロトタイプ */
extern void l_vog_lin_timer_init(void);
extern void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit);
extern void l_vog_lin_frm_tm_stop(void);
extern void l_vog_lin_timer_close(void);
```

### ドライバ初期化シーケンスの修正

```c
/**
 * @brief LINドライバ初期化
 */
void l_ifc_init_drv_ch1(void)
{
    /* UART初期化 */
    l_vog_lin_uart_init();

    /* タイマー初期化 */
    l_vog_lin_timer_init();

    /* GPIO初期化 */
    l_vog_lin_int_init();

    /* 受信許可 */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE);
}
```

---

## タイムアウト時間の計算例

### @9600bps の場合

| 項目 | ビット長 | 時間 (ms) |
|------|---------|-----------|
| **1バイト** | 10ビット | 1.04 ms |
| **ヘッダータイムアウト** | 149ビット | 15.52 ms |
| **8バイトデータ** | 80ビット | 8.33 ms |
| **9バイト（データ+CS）** | 90ビット | 9.38 ms |

### @19200bps の場合

| 項目 | ビット長 | 時間 (ms) |
|------|---------|-----------|
| **1バイト** | 10ビット | 0.52 ms |
| **ヘッダータイムアウト** | 149ビット | 7.76 ms |
| **8バイトデータ** | 80ビット | 4.17 ms |
| **9バイト（データ+CS）** | 90ビット | 4.69 ms |

---

## デバッグのヒント

### タイマー動作確認

```c
/**
 * @brief タイマー動作確認テスト
 */
void testTimer(void)
{
    /* 1msタイマー設定 */
    l_vog_lin_bit_tm_set(10);  /* 10ビット = 1.04ms @ 9600bps */

    /* 少し待つ */
    ClockP_usleep(2000);

    /* タイマー期限切れ確認 */
    if (b1l_lin_timer_expired)
    {
        /* タイマー正常動作 */
    }
    else
    {
        /* タイマー異常 */
    }
}
```

### タイムアウトエラー発生確認

```c
/**
 * @brief タイムアウトエラー発生確認
 */
void checkTimeoutError(void)
{
    uint16_t status = l_ifc_read_status_ch1();

    if (status & U2G_LIN_E_TIME)
    {
        /* Header Timeout Error 発生 */
    }

    if (xng_lin_frm_buf[slot].un_state.st_bit.u2g_lin_e_nores)
    {
        /* No Response Error 発生 */
    }

    if (status & U2G_LIN_PHY_BUS_ERR)
    {
        /* Physical Bus Error 発生 */
    }
}
```

---

## まとめ

LGPTimerLPF3を使用したLINタイムアウトエラー検出の実装により、以下が実現できます：

1. **Header Timeout Error検出**
   - Synch Break後のヘッダー受信タイムアウト検出
   - Physical Busエラーのカウント・検出

2. **No Response Error検出**
   - データフィールド受信タイムアウト検出
   - スレーブ無応答の検出

3. **高精度タイミング**
   - 48MHzシステムクロックによる高精度タイミング
   - ボーレートに応じた柔軟なタイマー設定

4. **H850互換動作**
   - H850のタイマー割り込み処理と同じ動作
   - SLIN API層の変更不要

これにより、CC23xxでもH850と同等のLINタイムアウトエラー検出が可能になります。

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: LIN Timer Implementation Guide
