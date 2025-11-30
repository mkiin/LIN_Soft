/**
 * @file        l_slin_drv_cc2340r53.h
 *
 * @brief       SLIN DRV層
 *
 * @attention   編集禁止
 *
 */

#ifndef __L_SLIN_DRV_CC2340R53_H_INCLUDE__
#define __L_SLIN_DRV_CC2340R53_H_INCLUDE__

/********************************************************************************/
/* ヘッダインクルード                                                           */
/********************************************************************************/
#include "device.h"
#include <ti/devices/DeviceFamily.h>
#include "l_slin_cmn.h"
/********************************************************************************/
/* 公開マクロ定義                                                               */
/********************************************************************************/
#define U4G_LIN_REG_PMR_MASK        ( (l_u32)0x00000001 )   /**< @brief PRIMASKフラグマスク値   型: u4 */
#define U4G_LIN_REG_PMR_EN_VAL      ( (l_u32)0x00000000 )   /**< @brief PRIMASK割り込み許可値   型: u4 */

#define  U1G_LIN_ERROR_STATUS_REG   ((l_u32)0x0000000F )    /* エラーステータス取得用マスク */
#define  U1G_LIN_OVERRUN_REG_ERR    ((l_u8)0x08)            /* オーバーランエラー発生 */
#define  U1G_LIN_FRAMING_REG_ERR    ((l_u8)0x01)            /* フレーミングエラー発生 */
#define  U1G_LIN_PARITY_REG_ERR     ((l_u8)0x02)            /* パリティエラー発生 */

#define U4L_LIN_UART_MAX_READSIZE   ( 9U )                  /**< @brief 受信サイズ   型: l_u32 */
#define U4L_LIN_UART_MAX_WRITESIZE  ( 9U )                  /**< @brief 送信サイズ   型: l_u32 */

/* Physical Busエラー検出タイマ 設定値計算用 */
#define  U2G_LIN_BUS_TIMEOUT        ( (l_u16)25000 )        /**< @brief  25000bit   型: u2 */

/* 受信ステータスのエラーセット値 */
#define  U1G_LIN_SOER_SET           ((l_u8)0x09)             /* オーバーランエラー */    
#define  U1G_LIN_SFER_SET           ((l_u8)0x0a)             /* フレーミングエラー */
#define  U1G_LIN_SPER_SET           ((l_u8)0x0c)             /* パリティエラー */

// チェックサムタイプ
typedef enum 
{
    U1G_LIN_CHECKSUM_CLASSIC,
    U1G_LIN_CHECKSUM_ENHANCED
}U1G_LIN_ChecksumType;

#define  U1G_LIN_CHECKSUM_CARRY_ADD ((l_u8)0x01)             /* チェックサム用キャリー加算値 */

/********************************************************************************/
/* 公開マクロ関数定義                                                           */
/********************************************************************************/
/**
 * @brief   割り込み禁止
 *
 *          ネスト付き割り込み禁止状態にする。
 *
 * @attention
 *          - 必ず同一スコープで、割り込み禁止/許可をセットで実施すること
 *              - 理由: 同一ブロック内からセットで使用しない場合は動作保証しないため
 *              - 対応例: l_u1g_lin_irq_dis() と l_vog_lin_irq_res() を同一ブロック内からセットで使用する。
 *          - 特権モードでのみ呼び出すこと
 *              - 理由: 特権モードでのみ実行可能のため(CMSIS留意点)
 *              - 対応例: 特権モードでのみ呼び出す
 *
 * @par     使用例
 *          割り込みされたくないタイミングで適宜呼び出す。
 *
 * @cond DETAIL
 * @par     処理内容
 *          - PRIMASK値を退避する。
 *          - __enable_irq()を呼び出し割り込み禁止状態にする。
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
#define l_u1g_lin_irq_dis()                                                         \
do                                                                                  \
{                                                                                   \
    l_u16 u4a_lin_primask;                            /* PRIMASK値        */        \
    u4a_lin_primask = __get_PRIMASK();             /* PRIMASK 退避     */           \
    __disable_irq()                                /* 即時割り込み禁止 */           
/*""End of Function""************************************************************/
/**
 * @brief   割り込み許可
 *
 *          ネスト付き割り込み許可状態にする。
 *
 * @attention
 *          - 必ず同一スコープで、割り込み禁止/許可をセットで実施すること。
 *              - 理由: 同一ブロック内からセットで使用しない場合は動作保証しないため
 *              - 対応例: l_u1g_lin_irq_dis() と l_vog_lin_irq_res() を同一ブロック内からセットで使用する。
 *          - 特権モードでのみ呼び出すこと
 *              - 理由: 特権モードでのみ実行可能のため(CMSIS留意点)
 *              - 対応例: 特権モードでのみ呼び出す
 *
 * @par     使用例
 *          割り込み禁止後に割り込みされても良いタイミングで適宜呼び出す。
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 割込み禁止時に割り込み許可状態であった場合のみ、__enable_irq()を呼び出す。
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
#define l_vog_lin_irq_res()                                                      \
    /* 割込み禁止時に割り込み許可状態であった場合のみ、__enable_irqを実施 */      \
    if ( U4G_LIN_REG_PMR_EN_VAL == ( U4G_LIN_REG_PMR_MASK & u4a_lin_primask ) )  \
    {                                                                            \
        __enable_irq();                           /* 割り込み許可 */             \
    }                                                                            \
    else{}                                                                       \
}                                                                                \
while( 0 )
/*""End of Function""************************************************************/

/********************************************************************************/
/* 公開関数(SLIN外部)プロトタイプ宣言                                        */
/********************************************************************************/
extern void  l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err);  /* H850互換: 1バイト受信 */
extern void  l_vog_lin_tm_int(void);
extern void  l_vog_lin_irq_int(void);
extern l_u8  l_vog_lin_checksum(l_u8 u1a_lin_pid ,const l_u8* u1a_lin_data, l_u8 u1a_lin_data_length, U1G_LIN_ChecksumType type);
extern void  l_vog_lin_tx_char(l_u8 u1a_lin_data);
extern l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data);
extern void  l_ifc_uart_close(void);
extern void  l_vog_lin_timer_init(void);
extern void  l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit);
extern void  l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit);
extern void  l_vog_lin_bus_tm_set(void);
extern void  l_vog_lin_frm_tm_stop(void);
extern void  l_vog_lin_timer_close(void);
#endif

/***** End of File *****/