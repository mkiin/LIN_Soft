/**
 * @file        l_slin_nmc.h
 *
 * @brief       SLIN NMC層ヘッダ
 *
 * @attention   編集禁止
 *
 */

#ifndef __L_SLIN_NMC_H_INCLUDE__
#define __L_SLIN_NMC_H_INCLUDE__

/* ユーザ編集可能な定数定義 */
/* スレーブタスク用定数 */
#define U2G_LIN_TM_SLVST            ((l_u16)550)    /* ウェイクアップ信号送信許可時間 - 初期化完了時間 */
#define U2G_LIN_TM_WURTY            ((l_u16)50)     /* ウェイクアップ信号リトライ時間 */
#define U2G_LIN_TM_3BRKS            ((l_u16)1500)   /* TIME-OUT AFTER THREE BREAKS */
#define U2G_LIN_CNT_RETRY           ((l_u16)2)      /* ウェイクアップリトライ回数 */
#define U2G_LIN_TM_DATA             ((l_u16)500)    /* Data.indビット設定時間 */
#define U2G_LIN_NM_TIME_BASE        ((l_u16)5)      /* NM用タイムベース時間 */
#define U1G_LIN_CNT_MSTERR          ((l_u8)3)       /* マスタ異常検出までのウェイクアップ失敗回数 */

/* ライブラリ参照用定数定義(ユーザ編集不可) */
/* スレーブタスク用定数 */
#define U1G_LIN_SLVSTAT_PON         ((l_u8)0)       /* ON/PASSIVE/WAKE-WAIT(起動後)状態 */
#define U1G_LIN_SLVSTAT_WAKE_WAIT   ((l_u8)1)       /* ON/PASSOVE/WAKE-WAIT状態 */
#define U1G_LIN_SLVSTAT_SLEEP       ((l_u8)2)       /* ON/PASSIVE/SLEEP状態 */
#define U1G_LIN_SLVSTAT_WAKE        ((l_u8)3)       /* ON/PASSIVE/WAKE状態 */
#define U1G_LIN_SLVSTAT_OTHER_WAKE  ((l_u8)4)       /* ON/PASSIVE/他WAKE状態 */
#define U1G_LIN_SLVSTAT_ACTIVE      ((l_u8)5)       /* ON/ACTIVE状態 */

#if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_2400
 #define  U2G_LIN_TM_TIMEOUT        ((l_u16)10400)  /* NM用Bus Idle Time-out(25000bit長) ms値 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
 #define  U2G_LIN_TM_TIMEOUT        ((l_u16)2600)   /* NM用Bus Idle Time-out(25000bit長) ms値 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
 #define  U2G_LIN_TM_TIMEOUT        ((l_u16)1300)   /* NM用Bus Idle Time-out(25000bit長) ms値 */
#endif

#define U1G_LIN_SLP_REQ_OFF         ((l_u8)0)       /* スリープ要求無し,又はウェイクアップ要因有り */
#define U1G_LIN_SLP_REQ_ON          ((l_u8)1)       /* スリープ要求有り,又はウェイクアップ要因無し */
#define U1G_LIN_SLP_REQ_FORCE       ((l_u8)2)       /* 強制スリープ要因有り */

/* 外部参照定義 */
extern void l_nm_init_ch1(void);
extern void l_nm_tick_ch1(l_u8 u1a_lin_slp_req);
extern l_u8 l_nm_rd_slv_stat_ch1(void);
extern l_u8 l_nm_rd_mst_err_ch1(void);
extern void l_nm_clr_mst_err_ch1(void);

#endif

/***** End of File *****/