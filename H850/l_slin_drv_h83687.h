/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
/* File Name   : l_slin_drv_h83687.h                        */
/* Version     : 2.00                                       */
/* Contents    :                                            */
/* Customer    : SUNNY GIKEN INC.                           */
/* Model       :                                            */
/* Order       :                                            */
/* CPU         : H8/3687                                    */
/* Compiler    :                                            */
/* OS          : Not used                                   */
/* LIN         : トヨタ自動車殿標準LIN仕様                  */
/* Programmer  :                                            */
/* Note        :                                            */
/************************************************************/
/* Copyright 2004 - 2005 SUNNY GIKEN INC.                   */
/************************************************************/
/* History : 2004.08.03 Ver 1.00                            */
/*         : 2004.11.04 Ver 1.01                            */
/*         : 2005.02.10 Ver 2.00                            */
/*""FILE COMMENT END""***************************************/

#ifndef __L_SLIN_DRV_H83687_H_INCLUDE__
#define __L_SLIN_DRV_H83687_H_INCLUDE__


/*** MCU依存部分の定数定義 ***/

/** <UARTレジスタの設定値> **/
/* UxMRレジスタ */
#define  U1G_LIN_UMR_INIT           ((l_u8)0x05)        /* 8bit長 パリティ禁止 */
/* UxC0レジスタ */
#define  U1G_LIN_UC0_INIT           ((l_u8)0x10)        /* UxC0の初期値 */
/* UxC1レジスタ */
#define  U1G_LIN_UC1_TE_SET         ((l_u8)0x01)        /* 送信許可ビットセット */
#define  U1G_LIN_UC1_RE_SET         ((l_u8)0x04)        /* 受信許可ビットセット */
#define  U1G_LIN_UC1_RE_CLR_MASK    ((l_u8)0xFB)        /* 受信許可ビットクリア用マスク */

/** <SCI3レジスタの設定値> **/
/* SCR3 */
#define  U1G_LIN_SCR3_INIT          ((l_u8)0x00)        /* SCR3の初期値 */
/* SMR */
#define  U1G_LIN_SMR_INIT           ((l_u8)0x00)        /* bit長 8,パリティなし,stop bit長 1 */
#define  U1G_LIN_SMR_MASK           ((l_u8)0x03)        /* 分周値マスク */


/** <Timerレジスタの設定値> **/
#define  U1G_LIN_FRM_TCR_INIT       ((l_u8)0x23)        /* TCRの初期値 */
#define  U1G_LIN_FRM_TIORA_INIT     ((l_u8)0x00)        /* TIORAの初期値 */
#define  U1G_LIN_FRM_TIER_INIT      ((l_u8)0x01)        /* TIERの初期値 */

/* タイマレジスタ設定値計算用 */
#define  U2G_LIN_FRM_TM_SRC_1       ((l_u16)1)          /* 文周なし */
#define  U2G_LIN_FRM_TM_SRC_4       ((l_u16)4)          /* 4分周 */

/* Physical Busエラー検出タイマ 設定値計算用 */
#define  U2G_LIN_BUS_TIMEOUT        ((l_u16)25000)      /* 25000bit */

/** <外部INTレジスタ> **/
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
#if U1G_LIN_EDGE_INT == U1G_LIN_EDGE_INT_UP
 #define U1G_LIN_INTEG_WPINI        ((l_u8)0x01)       /* INT割り込み極性 立ち上がりエッジ */
#elif U1G_LIN_EDGE_INT == U1G_LIN_EDGE_INT_DOWN
 #define U1G_LIN_INTEG_WPINI        ((l_u8)0x00)       /* INT割り込み極性 立ち下がりエッジ */
#elif U1G_LIN_EDGE_INT == U1G_LIN_EDGE_INT_NON
 #define U1G_LIN_INTEG_WPINI        ((l_u8)0x00)       /* INT割り込み極性 エッジ検出なし(立ち下がりエッジ) */
#endif
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */

/* Ver 2.00 削除:未使用の定義のため */
#if 0  /********** ↓↓↓ 削除コード ↓↓↓ **********/
#define  U1G_LIN_IC_POL_SET         ((l_u8)0x10)       /* INT割り込み極性 立ち上がりエッジ */
#define  U1G_LIN_IC_LEVEL_MASK      ((l_u8)0xF8)
#define  U1G_LIN_IC_IR_SET          ((l_u8)0x08)
#define  U1G_LIN_IC_IR_CLR_MASK     ((l_u8)0xF7)
#endif /********** ↑↑↑ 削除コード ↑↑↑ **********/
/* Ver 2.00 削除:未使用の定義のため */

#define  U2G_LIN_WORD_LIMIT         ((l_u16)0xFFFF)

/* Iフラグセット */
#define  U2G_LIN_I_FLAG_SET         ((l_u16)0x0040)

/* パラメータチェック範囲 */
#define  U2G_LIN_NAD_MIN            ((l_u16)0x01)
#define  U2G_LIN_NAD_MAX            ((l_u16)0xFF)
#define  U1G_LIN_MAX_SLOT_MIN       ((l_u8)1)
#define  U1G_LIN_MAX_SLOT_MAX       ((l_u8)64)
#define  U1G_LIN_BRG_MIN            ((l_u8)0x01)
#define  U1G_LIN_BRG_MAX            ((l_u8)0xFF)
#define  U1G_LIN_RSSP_MIN           ((l_u8)0)
#define  U1G_LIN_RSSP_MAX           ((l_u8)10)
#define  U1G_LIN_BTSP_MIN           ((l_u8)1)
#define  U1G_LIN_BTSP_MAX           ((l_u8)4)

/* CCRの割り込みマスクビット */
#define  U1G_LIN_IFLAG_CLR          ((l_u8)0)           /* CCRの割り込みマスクビット(I)=0(許可) */
#define  U1G_LIN_IFLAG_SET          ((l_u8)1)           /* CCRの割り込みマスクビット(I)=0(禁止) */

/* 受信ステータスのエラービットのマスク */
#define  U1G_LIN_SOER_MASK          ((l_u8)0x20)        /* オーバーランエラー */    
#define  U1G_LIN_SFER_MASK          ((l_u8)0x10)        /* フレーミングエラー */
#define  U1G_LIN_SPER_MASK          ((l_u8)0x08)        /* パリティエラー */

/* 受信ステータスのエラーセット値 */
#define  U1G_LIN_SOER_SET           ((l_u8)0x09)        /* オーバーランエラー */    
#define  U1G_LIN_SFER_SET           ((l_u8)0x0a)        /* フレーミングエラー */
#define  U1G_LIN_SPER_SET           ((l_u8)0x0c)        /* パリティエラー */

/*** 外部参照宣言 ***/
extern void  l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err);
extern void  l_vog_lin_tm_int(void);
extern void  l_vog_lin_irq_int(void);

/*** アセンブリ関数参照宣言 ***/
/* Ver 1.01 削除:使用していない関数のため -> start */
/* extern void  l_vog_lin_set_imask_ccr(l_u8 u1a_lin_flg); */
/* extern l_u8  l_u1g_lin_get_imask_ccr(void); */
/* Ver 1.01 削除:使用していない関数のため <- end */
extern void  l_vog_lin_nop(void);

/* Ver 1.01 追加:宣言が抜けていたため -> start */
extern l_u8   l_u1g_lin_irq_dis(void);
extern void   l_vog_lin_irq_res(l_u8 u1a_lin_flg);
/* Ver 1.01 追加:宣言が抜けていたため <- end */

#endif

/***** End of File *****/



