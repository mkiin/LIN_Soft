/**
 * @file        l_slin_break_detect_temp.h
 *
 * @brief       LIN 9.5ビットBreak検出モジュール (プレビュー版)
 *
 * @attention   これはテスト実装です。本番環境には適用しないでください。
 *
 */

#ifndef __L_SLIN_BREAK_DETECT_TEMP_H_INCLUDE__
#define __L_SLIN_BREAK_DETECT_TEMP_H_INCLUDE__

#include "l_type.h"

/********************************************************************************/
/* 定数定義                                                                     */
/********************************************************************************/

/* LINボーレート関連 */
#define U4G_LIN_BREAK_BAUDRATE          (19200U)        /**< @brief LINボーレート [bps] */
#define U4G_LIN_BREAK_BIT_TIME_US       (52U)           /**< @brief 1ビット時間 [μs] */

/* Break検出閾値 */
#define U4G_LIN_BREAK_9_5_BIT_US        (495U)          /**< @brief 9.5ビット時間 [μs] */
#define U4G_LIN_BREAK_MARGIN_US         (5U)            /**< @brief マージン [μs] */
#define U4G_LIN_BREAK_THRESHOLD_US      (U4G_LIN_BREAK_9_5_BIT_US - U4G_LIN_BREAK_MARGIN_US)
                                                        /**< @brief 実際の閾値: 490μs */

/* Break検出状態 */
#define U1G_BREAK_STATE_IDLE            ((l_u8)0x00)    /**< @brief アイドル状態 */
#define U1G_BREAK_STATE_WAIT            ((l_u8)0x01)    /**< @brief Break待機中 (GPIO監視) */
#define U1G_BREAK_STATE_TIMING          ((l_u8)0x02)    /**< @brief タイマー計測中 */
#define U1G_BREAK_STATE_DETECTED        ((l_u8)0x03)    /**< @brief Break検出完了 */

/* タイマー状態 */
#define U1G_TIMER_STATE_STOPPED         ((l_u8)0x00)    /**< @brief タイマー停止中 */
#define U1G_TIMER_STATE_RUNNING         ((l_u8)0x01)    /**< @brief タイマー動作中 */
#define U1G_TIMER_STATE_EXPIRED         ((l_u8)0x02)    /**< @brief タイマー満了 */

/* エッジタイプ */
#define U1G_EDGE_TYPE_FALLING           ((l_u8)0x00)    /**< @brief 立下りエッジ */
#define U1G_EDGE_TYPE_RISING            ((l_u8)0x01)    /**< @brief 立上りエッジ */

/********************************************************************************/
/* 外部関数プロトタイプ                                                         */
/********************************************************************************/

/**
 * @brief   Break検出モジュール初期化
 * @param   なし
 * @return  なし
 */
extern void l_vog_break_detect_init(void);

/**
 * @brief   Break検出モジュール終了
 * @param   なし
 * @return  なし
 */
extern void l_vog_break_detect_deinit(void);

/**
 * @brief   Break検出開始
 * @param   なし
 * @return  なし
 */
extern void l_vog_break_detect_start(void);

/**
 * @brief   Break検出停止
 * @param   なし
 * @return  なし
 */
extern void l_vog_break_detect_stop(void);

/**
 * @brief   Break検出状態取得
 * @param   なし
 * @return  現在のBreak検出状態
 */
extern l_u8 l_u1g_break_detect_get_state(void);

/**
 * @brief   GPIO割り込みハンドラ (内部使用)
 * @param   index : GPIOインデックス
 * @return  なし
 */
extern void l_vog_break_detect_gpio_isr(uint_least8_t index);

/**
 * @brief   タイマー割り込みハンドラ (内部使用)
 * @param   handle : タイマーハンドル
 * @param   interruptMask : 割り込みマスク
 * @return  なし
 */
extern void l_vog_break_detect_timer_isr(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask);

#endif /* __L_SLIN_BREAK_DETECT_TEMP_H_INCLUDE__ */

/***** End of File *****/