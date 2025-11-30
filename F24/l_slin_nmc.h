/*""FILE COMMENT""********************************************/
/* System Name : NM Module for S930-LSLibRL78F24xx           */
/* File Name   : l_slin_nmc.h                                */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_NMC_H_INCLUDE
#define L_SLIN_NMC_H_INCLUDE

/* ユーザ編集可能な定数定義 */
/* スレーブタスク用定数 */
#define U2G_LIN_TM_SLVINIT          ((l_u16)0U)     /* 初期化完了時間 ～100ms （起動時） */
#define U2G_LIN_TM_SLVINIT_WAKE     ((l_u16)0U)     /* 初期化完了時間 ～100ms （Wakeup時） */

#define U2G_LIN_TM_WURTY            ((l_u16)155U)   /* ウェイクアップ信号リトライ時間 “150 + U2G_LIN_NM_TIME_BASE”以上の値を設定してください。 */
                                                    /* U2G_LIN_TM_WURTYは、前回のWakeup信号の立下がりエッジから、次回のWakeup信号の立下がりエッジまでの間隔で、 */
                                                    /* LIN規格では、前回のWakeup信号の立上がりエッジから、次のWakeup信号の立下がりエッジまでの間隔を150ms以上と規定しており、 */
                                                    /* Wakeup信号の長さ分、表す時間に差があるため。*/
#define U2G_LIN_TM_3BRKS            ((l_u16)1500U)  /* TIME-OUT AFTER THREE BREAKS */
#define U2G_LIN_CNT_RETRY           ((l_u16)2U)     /* ウェイクアップリトライ回数 */
#define U2G_LIN_NM_TIME_BASE        ((l_u16)5U)     /* NM用タイムベース時間 */
#define U1G_LIN_CNT_MSTERR          ((l_u8)3U)      /* マスタ異常検出までのウェイクアップ失敗回数 */

/* ライブラリ参照用定数定義(ユーザ編集不可) */
/* スレーブタスク用定数 */
#define U1G_LIN_NM_INI_WAIT         ((l_u8)0U)      /* NM INITIALIZING状態 起動時 */
#define U1G_LIN_NM_INI_WAKE         ((l_u8)1U)      /* NM INITIALIZING状態 自ノードWakeup時 */
#define U1G_LIN_NM_INI_OTH_WAKE     ((l_u8)2U)      /* NM INITIALIZING状態 他ノードWakeup時 */
#define U1G_LIN_NM_OPE_WAKE_WAIT    ((l_u8)3U)      /* NM OPERATIONAL状態 Wakeup待ち状態 */
#define U1G_LIN_NM_OPE_WAKE         ((l_u8)4U)      /* NM OPERATIONAL状態 自ノードWakeup時 */
#define U1G_LIN_NM_OPE_OTH_WAKE     ((l_u8)5U)      /* NM OPERATIONAL状態 他ノードWakeup時 */
#define U1G_LIN_NM_OPE_ACTIVE       ((l_u8)6U)      /* NM OPERATIONAL状態 スケジュール検出時 */
#define U1G_LIN_NM_BS_SLEEP         ((l_u8)7U)      /* NM BUS SLEEP MODE状態 */


/* ウェイクアップ再送制御用定数 */
#define U1L_LIN_WUP_NONE            ((l_u8)0U)      /* Wakeup再送制御 停止 */
#define U1L_LIN_WUP_T3BRK           ((l_u8)1U)      /* Wakeup再送制御 TT3BRKカウント中 */
#define U1L_LIN_WUP_WUARTY          ((l_u8)2U)      /* Wakeup再送制御 TWUARTYカウント中 */

#define U1G_LIN_SLP_REQ_OFF         ((l_u8)0U)      /* スリープ要求無し,又はウェイクアップ要因有り */
#define U1G_LIN_SLP_REQ_ON          ((l_u8)1U)      /* スリープ要求有り,又はウェイクアップ要因無し */
#define U1G_LIN_SLP_REQ_FORCE       ((l_u8)2U)      /* 強制スリープ要因有り */

/*** 関数のプロトタイプ宣言(global) ***/
void l_nm_init(void);
void l_nm_tick(l_u8 u1a_lin_slp_req);
l_u8 l_nm_rd_slv_stat(void);
#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
l_u8 l_nm_rd_mst_err(void);
void l_nm_clr_mst_err(void);
#endif

#endif

/***** End of File *****/





