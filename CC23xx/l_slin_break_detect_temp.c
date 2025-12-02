/**
 * @file        l_slin_break_detect_temp.c
 *
 * @brief       LIN 9.5ビットBreak検出モジュール (プレビュー版)
 *
 * @attention   これはテスト実装です。本番環境には適用しないでください。
 *
 */

#pragma section lin

/**** ヘッダ インクルード ****/
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_drv_cc2340r53.h"
#include "l_slin_break_detect_temp.h"

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/timer/LGPTimerLPF3.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/********************************************************************************/
/* 静的変数                                                                     */
/********************************************************************************/

/**
 * @brief Break検出制御構造体
 */
typedef struct {
    volatile l_u8  u1_state;            /**< @brief Break検出状態 */
    volatile l_u8  u1_timer_state;      /**< @brief タイマー状態 */
    volatile l_u8  u1_edge_type;        /**< @brief 現在の検出エッジ (0:立下り, 1:立上り) */
    volatile l_u8  u1_reserved;         /**< @brief 予約 (アライメント用) */
} st_BreakDetectCtrl;

/* Break検出制御構造体インスタンス */
static st_BreakDetectCtrl stl_break_ctrl;

/* Break検出用タイマーハンドル */
static LGPTimerLPF3_Handle xnl_break_timer_handle = NULL;

/* 外部参照 (core層の状態変数) */
extern l_u8 u1l_lin_slv_sts;                    /* スレーブタスク用ステータス */

/********************************************************************************/
/* 内部関数プロトタイプ                                                         */
/********************************************************************************/

static void l_vsl_set_gpio_falling_edge(void);
static void l_vsl_set_gpio_rising_edge(void);
static void l_vsl_start_break_timer(void);
static void l_vsl_stop_break_timer(void);
static void l_vsl_break_detected_callback(void);

/********************************************************************************/
/* 外部関数                                                                     */
/********************************************************************************/

/**
 * @brief   Break検出モジュール初期化
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 制御構造体の初期化
 *          - Break検出用タイマーの初期化
 *          - GPIO設定（エッジ検出、コールバック登録）
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_break_detect_init(void)
{
    LGPTimerLPF3_Params timerParams;

    if (xnl_break_timer_handle == NULL)
    {
        /*=== 制御構造体初期化 ===*/
        stl_break_ctrl.u1_state       = U1G_BREAK_STATE_IDLE;
        stl_break_ctrl.u1_timer_state = U1G_TIMER_STATE_STOPPED;
        stl_break_ctrl.u1_edge_type   = U1G_EDGE_TYPE_FALLING;
        stl_break_ctrl.u1_reserved    = 0U;

        /*=== Break検出タイマー初期化 ===*/
        LGPTimerLPF3_Params_init(&timerParams);
        timerParams.hwiCallbackFxn = l_vog_break_detect_timer_isr;
        timerParams.intPhaseLate = true;
        timerParams.prescalerDiv = 0U;  /* プリスケーラなし (48MHz直接) */

        /* タイマーオープン */
        xnl_break_timer_handle = LGPTimerLPF3_open(CONFIG_LGPTIMER_0, &timerParams);

        if (xnl_break_timer_handle == NULL)
        {
            /* タイマーオープン失敗 */
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;
        }

        /*=== GPIO設定 ===*/
        /* GPIO_init()は既に呼ばれている前提 */

        /* LIN RXピンを立下りエッジ割り込みで設定 */
        GPIO_setConfig(U1G_LIN_INTPIN,
                      GPIO_CFG_INPUT_INTERNAL | GPIO_CFG_IN_INT_FALLING);

        /* コールバック登録 */
        GPIO_setCallback(U1G_LIN_INTPIN, l_vog_break_detect_gpio_isr);
    }
}

/**
 * @brief   Break検出モジュール終了
 *
 * @cond DETAIL
 * @par     処理内容
 *          - タイマー停止・クローズ
 *          - GPIO割り込み無効化
 *          - 状態リセット
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_break_detect_deinit(void)
{
    /* タイマー停止・クローズ */
    if (xnl_break_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_break_timer_handle);
        LGPTimerLPF3_close(xnl_break_timer_handle);
        xnl_break_timer_handle = NULL;
    }

    /* GPIO割り込み無効化 */
    GPIO_disableInt(U1G_LIN_INTPIN);

    /* 状態リセット */
    stl_break_ctrl.u1_state = U1G_BREAK_STATE_IDLE;
}

/**
 * @brief   Break検出開始
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 状態を待機中に設定
 *          - 立下りエッジ検出を設定
 *          - GPIO割り込み有効化
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_break_detect_start(void)
{
    /* 状態を待機中に設定 */
    stl_break_ctrl.u1_state       = U1G_BREAK_STATE_WAIT;
    stl_break_ctrl.u1_timer_state = U1G_TIMER_STATE_STOPPED;

    /* 立下りエッジ検出を設定 */
    l_vsl_set_gpio_falling_edge();

    /* GPIO割り込み有効化 */
    GPIO_enableInt(U1G_LIN_INTPIN);
}

/**
 * @brief   Break検出停止
 *
 * @cond DETAIL
 * @par     処理内容
 *          - GPIO割り込み無効化
 *          - タイマー停止
 *          - 状態をアイドルに設定
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_break_detect_stop(void)
{
    /* GPIO割り込み無効化 */
    GPIO_disableInt(U1G_LIN_INTPIN);

    /* タイマー停止 */
    l_vsl_stop_break_timer();

    /* 状態をアイドルに設定 */
    stl_break_ctrl.u1_state       = U1G_BREAK_STATE_IDLE;
    stl_break_ctrl.u1_timer_state = U1G_TIMER_STATE_STOPPED;
}

/**
 * @brief   Break検出状態取得
 *
 * @return  現在のBreak検出状態
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 現在のBreak検出状態を返す
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
l_u8 l_u1g_break_detect_get_state(void)
{
    return stl_break_ctrl.u1_state;
}

/********************************************************************************/
/* 割り込みハンドラ                                                             */
/********************************************************************************/

/**
 * @brief   GPIO割り込みハンドラ
 *
 * @param   index : GPIOインデックス
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 立下りエッジ検出時:
 *              - Break開始の可能性
 *              - タイマー開始
 *              - 立上りエッジ検出に切り替え
 *          - 立上りエッジ検出時:
 *              - Break終了（Delimiter検出）
 *              - タイマー停止
 *              - タイマー状態確認
 *                  - 9.5ビット以上経過 → Break確定
 *                  - 9.5ビット未満 → Breakではない
 *              - 立下りエッジ検出に戻す
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_break_detect_gpio_isr(uint_least8_t index)
{
    (void)index;  /* 未使用パラメータ */

    /*=== 立下りエッジ検出時 ===*/
    if (stl_break_ctrl.u1_edge_type == U1G_EDGE_TYPE_FALLING)
    {
        /* Break開始の可能性 */

        /* タイマー状態をリセット */
        stl_break_ctrl.u1_timer_state = U1G_TIMER_STATE_RUNNING;

        /* 状態を計測中に変更 */
        stl_break_ctrl.u1_state = U1G_BREAK_STATE_TIMING;

        /* 9.5ビットタイマー開始 */
        l_vsl_start_break_timer();

        /* 立上りエッジ検出に切り替え */
        l_vsl_set_gpio_rising_edge();
    }
    /*=== 立上りエッジ検出時 ===*/
    else
    {
        /* Break終了（Delimiter検出） */

        /* タイマー停止 */
        l_vsl_stop_break_timer();

        /* タイマー状態確認 */
        if (stl_break_ctrl.u1_timer_state == U1G_TIMER_STATE_EXPIRED)
        {
            /*=== 9.5ビット以上経過 → Break確定 ===*/
            stl_break_ctrl.u1_state = U1G_BREAK_STATE_DETECTED;

            /* Break検出コールバック呼び出し */
            l_vsl_break_detected_callback();
        }
        else
        {
            /*=== 9.5ビット未満 → Breakではない ===*/
            /* 待機状態に戻る */
            stl_break_ctrl.u1_state       = U1G_BREAK_STATE_WAIT;
            stl_break_ctrl.u1_timer_state = U1G_TIMER_STATE_STOPPED;
        }

        /* 立下りエッジ検出に戻す */
        l_vsl_set_gpio_falling_edge();
    }
}

/**
 * @brief   タイマー割り込みハンドラ
 *
 * @param   handle : タイマーハンドル
 * @param   interruptMask : 割り込みマスク
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 9.5ビット時間経過で呼び出される
 *          - タイマー満了フラグをセット
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_break_detect_timer_isr(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask)
{
    (void)handle;         /* 未使用パラメータ */
    (void)interruptMask;  /* 未使用パラメータ */

    /* 9.5ビット以上Low継続 → タイマー満了フラグセット */
    stl_break_ctrl.u1_timer_state = U1G_TIMER_STATE_EXPIRED;

    /* 注: この時点ではまだBreak確定ではない */
    /* 立上りエッジ検出時に最終判定を行う */
}

/********************************************************************************/
/* 内部関数                                                                     */
/********************************************************************************/

/**
 * @brief   GPIO立下りエッジ検出設定
 *
 * @cond DETAIL
 * @par     処理内容
 *          - エッジタイプを立下りに設定
 *          - GPIO設定変更
 *          - 割り込み再有効化
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
static void l_vsl_set_gpio_falling_edge(void)
{
    /* エッジタイプを立下りに設定 */
    stl_break_ctrl.u1_edge_type = U1G_EDGE_TYPE_FALLING;

    /* GPIO設定変更 */
    GPIO_setInterruptConfig(U1G_LIN_INTPIN, GPIO_CFG_IN_INT_FALLING);

    /* 割り込み再有効化 */
    GPIO_enableInt(U1G_LIN_INTPIN);
}

/**
 * @brief   GPIO立上りエッジ検出設定
 *
 * @cond DETAIL
 * @par     処理内容
 *          - エッジタイプを立上りに設定
 *          - GPIO設定変更
 *          - 割り込み再有効化
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
static void l_vsl_set_gpio_rising_edge(void)
{
    /* エッジタイプを立上りに設定 */
    stl_break_ctrl.u1_edge_type = U1G_EDGE_TYPE_RISING;

    /* GPIO設定変更 */
    GPIO_setInterruptConfig(U1G_LIN_INTPIN, GPIO_CFG_IN_INT_RISING);

    /* 割り込み再有効化 */
    GPIO_enableInt(U1G_LIN_INTPIN);
}

/**
 * @brief   Break検出タイマー開始
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 9.5ビット時間(490μs)のタイマーを開始
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
static void l_vsl_start_break_timer(void)
{
    l_u32 u4a_lin_timer_count;

    if (xnl_break_timer_handle != NULL)
    {
        /* 9.5ビット時間を48MHzカウントに変換 */
        /* 490μs * 48 = 23520カウント */
        u4a_lin_timer_count = (l_u32)U4G_LIN_BREAK_THRESHOLD_US * 48U;

        /* タイマー設定 */
        LGPTimerLPF3_setInitialCounterTarget(xnl_break_timer_handle,
                                             u4a_lin_timer_count,
                                             true);

        /* ターゲット割り込み許可 */
        LGPTimerLPF3_enableInterrupt(xnl_break_timer_handle, LGPTimerLPF3_INT_TGT);

        /* ワンショットモードで開始 */
        LGPTimerLPF3_start(xnl_break_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
    }
}

/**
 * @brief   Break検出タイマー停止
 *
 * @cond DETAIL
 * @par     処理内容
 *          - タイマーを停止
 *          - 割り込みを無効化
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
static void l_vsl_stop_break_timer(void)
{
    if (xnl_break_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_break_timer_handle);
        LGPTimerLPF3_disableInterrupt(xnl_break_timer_handle, LGPTimerLPF3_INT_TGT);
    }
}

/**
 * @brief   Break検出コールバック関数
 *
 * @cond DETAIL
 * @par     処理内容
 *          - Break検出停止（UART受信に切り替えるため）
 *          - RUN状態でない場合はRUN状態へ移行
 *          - ヘッダタイムアウトタイマセット
 *          - UART受信有効化
 *          - Sync Field待ちへ遷移
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
static void l_vsl_break_detected_callback(void)
{
    /* Break検出停止（UART受信に切り替えるため） */
    l_vog_break_detect_stop();

    /* RUN状態でない場合はRUN状態へ移行 */
    if (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts != U2G_LIN_STS_RUN)
    {
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
    }

    /* ヘッダタイムアウトタイマセット */
    l_vog_lin_bit_tm_set(U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH);

    /* UART受信有効化 */
    l_vog_lin_rx_enb();

    /* Sync Field待ちへ遷移 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
}

/***** End of File *****/
