/* File Name   : l_slin_cmn.h                               */
/* Version     : 2.00                                       */
/* Contents    :                                            */
/* Customer    : SUNNY GIKEN INC.                           */
/* Model       :                                            */
/* Order       :                                            */
/* CPU         :                                            */
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

#ifndef __L_SLIN_CMN_H_INCLUDE__
#define __L_SLIN_CMN_H_INCLUDE__

/***** 型宣言 *****/
typedef unsigned char   l_u8;
typedef unsigned int    l_u16;
typedef unsigned long   l_u32;
typedef unsigned int    l_bool;
typedef unsigned char   l_frame_handle;

/***** 定数定義 *****/
/* Wakeup検出 */
#define  U1G_LIN_WP_UART_USE        (0)             /* WAKEUP検出にUARTを使用        */
#define  U1G_LIN_WP_INT_USE         (1)             /* WAKEUP検出にINT割り込みを使用 */

#define  U1G_LIN_ENDIAN_LITTLE      (0)             /* リトルエンディアンタイプ */
#define  U1G_LIN_ENDIAN_BIG         (1)             /* ビッグエンディアンタイプ */

/* 通信ボーレート */
#define  U1G_LIN_BAUDRATE_2400      (0)             /* 通信ボーレート2400bpsを使用  */
#define  U1G_LIN_BAUDRATE_9600      (1)             /* 通信ボーレート9600bpsを使用  */
#define  U1G_LIN_BAUDRATE_19200     (2)             /* 通信ボーレート19200bpsを使用 */

/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
#define  U1G_LIN_EDGE_INT_NON       (0)
#define  U1G_LIN_EDGE_INT_UP        (1)
#define  U1G_LIN_EDGE_INT_DOWN      (2)
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */

/* unsigned char型のOK/NG */
#define  U1G_LIN_OK                 ((l_u8)0)       /* 処理成功 */
#define  U1G_LIN_NG                 ((l_u8)1)       /* 処理失敗 */

/* bool型のOK/NG */
#define  U2G_LIN_OK                 ((l_bool)0)     /* 処理成功 */
#define  U2G_LIN_NG                 ((l_bool)1)     /* 処理失敗 */

/* LINステータス */
#define  U2G_LIN_STS_RESET          ((l_u16)0)      /* LINステータス = RESET       */
#define  U2G_LIN_STS_SLEEP          ((l_u16)1)      /* LINステータス = SLEEP       */
#define  U2G_LIN_STS_RUN_STANDBY    ((l_u16)2)      /* LINステータス = RUN STANDBY */
#define  U2G_LIN_STS_RUN            ((l_u16)3)      /* LINステータス = RUN         */

#define  U1G_LIN_MAX_SLOT_NUM       ((l_u8)64)      /* スロット最大数 */

#define  U1G_LIN_DATA_SUM_LEN       ((l_u8)9)       /* データ長(8バイト)＋チェックサム長(1バイト) */

#define  U1G_LIN_WORD_BIT           ((l_u8)16)      /* word型をbitで分割(16bit) */

/* ID Info Send/Recv Setting */
#define  U1G_LIN_CMD_SND            ((l_u8)0)       /* 送信フレーム */
#define  U1G_LIN_CMD_RCV            ((l_u8)1)       /* 受信フレーム */

/* NM使用/未使用(unsigned char型) */
#define  U1G_LIN_NM_NO_USE          ((l_u8)0)       /* NM未使用 */
#define  U1G_LIN_NM_USE             ((l_u8)1)       /* NM使用   */

#define  U1G_LIN_NO_FRAME           ((l_u8)0xFF)    /* 未登録フレーム(ID = 0xFF) */

#define  U2G_LIN_MASK_FF            ((l_u16)0x00FF) /* FFh(下位バイト)のマスク */

/* システム異常フラグ */
#define  U1G_LIN_SYSERR_STAT        ((l_u8)1)       /* LINステータスが規定値以外 */
#define  U1G_LIN_SYSERR_DIV         ((l_u8)2)       /* 0除算発生 */

/* Ver 2.00 追加:意味のあるデファイン名に変更 */
/* LINレスポンスバッファのレジスタ */
#define  U1G_LIN_MAX_DL             ((l_u8)8)       /* 最大データサイズ */
/* Ver 2.00 追加:意味のあるデファイン名に変更 */

/* ID Info DL Setting */
#define  U1G_LIN_DL_1               ((l_u8)0x01)
#define  U1G_LIN_DL_2               ((l_u8)0x02)
#define  U1G_LIN_DL_3               ((l_u8)0x03)
#define  U1G_LIN_DL_4               ((l_u8)0x04)
#define  U1G_LIN_DL_5               ((l_u8)0x05)
#define  U1G_LIN_DL_6               ((l_u8)0x06)
#define  U1G_LIN_DL_7               ((l_u8)0x07)
#define  U1G_LIN_DL_8               ((l_u8)0x08)
#define  U1G_LIN_DL_n               ((l_u8)0x0F)


/* unsigned char型 */
#define  U1G_LIN_BIT_CLR            ((l_u8)0)
#define  U1G_LIN_BIT_SET            ((l_u8)1)
#define  U1G_LIN_BYTE_CLR           ((l_u8)0x00)

#define  U1G_LIN_0                  ((l_u8)0)
#define  U1G_LIN_1                  ((l_u8)1)
#define  U1G_LIN_2                  ((l_u8)2)
#define  U1G_LIN_3                  ((l_u8)3)
#define  U1G_LIN_4                  ((l_u8)4)
#define  U1G_LIN_5                  ((l_u8)5)
#define  U1G_LIN_6                  ((l_u8)6)
#define  U1G_LIN_7                  ((l_u8)7)
#define  U1G_LIN_8                  ((l_u8)8)
#define  U1G_LIN_16                 ((l_u8)16)
#define  U1G_LIN_32                 ((l_u8)32)
#define  U1G_LIN_48                 ((l_u8)48)
#define  U1G_LIN_64                 ((l_u8)64)


/* unsigned int型 */
#define  U2G_LIN_BIT_CLR            ((l_u16)0)
#define  U2G_LIN_BIT_SET            ((l_u16)1)
#define  U2G_LIN_BYTE_CLR           ((l_u16)0x00)
#define  U2G_LIN_WORD_CLR           ((l_u16)0x0000)

#define  U2G_LIN_1                  ((l_u16)1)
#define  U2G_LIN_2                  ((l_u16)2)
#define  U2G_LIN_4                  ((l_u16)4)
#define  U2G_LIN_8                  ((l_u16)8)
#define  U2G_LIN_10                 ((l_u16)10)
#define  U2G_LIN_12                 ((l_u16)12)
#define  U2G_LIN_14                 ((l_u16)14)


/* unsigned long型 */
#define  U4G_LIN_LONG_CLR           ((l_u32)0x00000000)

#define  U4G_LIN_1                  ((l_u32)1)

/* NULL */
#define  U1G_LIN_NULL               (0u)

/***** API関数 外部参照宣言(extern) *****/
extern l_bool l_sys_init(void);

/***** その他の関数 外部参照宣言(extern) *****/
extern l_u8  l_u1g_lin_para_chk(void);
extern l_u8  l_u1g_lin_tbl_chk(void);
/***** APIマクロ宣言 *****/
#define  l_bool_rd(x)       (x)
#define  l_u8_rd(x)         (x)
#define  l_u16_rd(x)        (x)
#define  l_bool_wr(x, v)    ( (x) = (v) )
#define  l_u8_wr(x, v)      ( (x) = (v) )
#define  l_u16_wr(x, v)     ( (x) = (v) )
#define  l_flg_tst(x)       (x)
#define  l_flg_clr(x)       ( (x) = 0 )

#endif

/***** End of File *****/




/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
