/*""FILE COMMENT""**********************************************
 * System Name  : S/R System
 * File Name    : p_lin_apl.h
 * Contests     : LIN通信処理
 * Model        : SRF汎用
 * Order        : Project No 
 * CPU          : Renesas H8/300H Tiny Series
 * Compiler     : Renesas H8S,H8/300 Series C/C++ Compiler Ver 4.0.03
 * OS           : no used
 * Note         : LIN通信のアプリケーション部
 ***************************************************************
 *                          Copyright(c) AISIN SEIKI CO.,LTD.
 ***************************************************************
 * History      : 2004.09.16
 *""FILE COMMENT END""*****************************************/
 
#ifndef P_LIN_APL_H
#define P_LIN_APL_H

#include	"l_stddef.h"

#define U1G_ARY_0	( (u1)(0u) )
#define U1G_ARY_1	( (u1)(1u) )
#define U1G_ARY_2	( (u1)(2u) )
#define U1G_ARY_3	( (u1)(3u) )
#define U1G_ARY_4	( (u1)(4u) )
#define U1G_ARY_5	( (u1)(5u) )
#define U1G_ARY_6	( (u1)(6u) )
#define U1G_ARY_7	( (u1)(7u) )
#define U1G_ARY_8	( (u1)(8u) )

#define	U1G_DIAG_MAX_21	( (u1)(3u) )

/*------ 仕向分類 --------------------------------*/
#define U1G_DEST_USA		( (u1)(0u) )	/* 北米仕向	*/
#define U1G_DEST_JPN		( (u1)(1u) )	/* 国内仕向	*/
#define U1G_DEST_EUR		( (u1)(2u) )	/* 欧州仕向	*/
#define U1G_DEST_AUS		( (u1)(3u) )	/* 豪州仕向	*/

/*------ 車両型式ハンドル ------------------------*/
#define U1G_HANDLE_MSK		( (u1)(0xc0u) )	/* ﾊﾝﾄﾞﾙ情報ﾏｽｸﾃﾞｰﾀ	*/
#define U1G_HANDLE_LEFT		( (u1)(0x40u) )	/* 左ﾊﾝﾄﾞﾙ($01)		*/
#define U1G_HANDLE_RIGHT	( (u1)(0x80u) )	/* 右ﾊﾝﾄﾞﾙ($10)		*/

/*------ 仕向けコード ----------------------------*/
/*------ 日本/欧州 -------*/
#define U1G_CMT_JPN	( (u1)(0x00u) )	/* 市販	*/
#define U1G_CMT_EUR	( (u1)(0x57u) ) /* 欧州 */
#define U1G_CMT_MNE	( (u1)(0x56u) ) /* 中近東 */
#define U1G_CMT_CHN	( (u1)(0x43u) ) /* 中国 */
#define U1G_CMT_GEN	( (u1)(0xF0u) ) /* 一般輸 */
#define U1G_CMT_AUS	( (u1)(0x51u) ) /* 豪州 */
/*------ 北米 ------------*/
#define U1G_CMT_USA	( (u1)(0x41u) ) /* 北米 */
#define U1G_CMT_NON	( (u1)(0xFFu) ) /* 不定 */

/*------  ノードコード(ダイアグ用) ----------------*/
#define U1G_CODE_N_TA	( (u1)(0xadu) )

/* TAG		: xn_lin_rx_com_sts_t		*/
/* ABSTRACT	: 通信途絶状態共用体	*/
/* NOTE		: 各フレームの受信途絶状態を設定する			*/
typedef union {
	u1	u1_lin_com_sts;			/* バイトアクセスタグ	*/
	struct	{
       u1 u1_flb_comdwn_bod1s01	:1;		/* ボディーECU情報①通信途絶	*/
       u1 u1_flb_comdwn_bod1s02	:1;		/* ボディーECU情報②通信途絶	*/
       u1 u1_flb_comdwn_bod1s03 :1;		/* ボディーECU情報③通信途絶	*/
       u1 u1_flb_comdwn_bod1s04	:1;		/* ボディーECU情報④通信途絶	*/

       u1 u1_flb_comdwn_msw1s01	:1;		/* Ｐ／ＷマスタＳＷ①通信途絶	*/
       u1 u1_flb_comdwn_dpw1s01 :1;    	/* Ｄ席Ｐ／Ｗ情報①通信途絶  	*/
       u1 reserve1  			:1;    	/* リザーブ     				*/
       u1 u1_flb_comdwn_diag  	:1;    	/* ダイアグ要求通信途絶			*/
    }xn_rx_com_sts;					/* ビットアクセスタグ */
}xn_lin_rx_com_sts_t;


/* TAG		: xn_lin_rx_com_fst_t		*/
/* ABSTRACT	: フレーム受信状態共用体	*/
/* NOTE		: 各フレームの受信状態を設定する			*/
typedef union {
	u1	u1_lin_com_fst;			/* バイトアクセスタグ	*/
	struct	{
       u1 u1_flb_id28_first_rx 	:1;    	/* ID28初回受信	*/
       u1 u1_flb_id29_first_rx 	:1;    	/* ID29初回受信	*/
       u1 u1_flb_id2a_first_rx 	:1;    	/* ID2a初回受信	*/
       u1 u1_flb_id22_first_rx 	:1;    	/* ID22初回受信	*/

       u1 u1_flb_id2b_first_rx 	:1;    	/* ID2b初回受信	*/
       u1 u1_flb_id24_first_rx 	:1;    	/* ID24初回受信	*/
       u1 reserve1  			:1;    	/* リザーブ     */
       u1 reserve2  			:1;    	/* リザーブ     */
    }xn_rx_com_fst;					/* ビットアクセスタグ */
}xn_lin_rx_com_fst_t;


/* TAG		: xn_lin_info_rmt_t		*/
/* ABSTRACT	: リモート信号情報共用体	*/
/* NOTE		: 各リモート信号の状態を設定する			*/
typedef union {
	u1	u1_lin_info_rmt;		/* バイトアクセスタグ	*/
	struct	{
	   u1 reserved1	 		:1;			/* リザーブ						*/
	   u1 reserved2	 		:1;			/* リザーブ						*/
	   u1 u1_flb_kpwu		:1;    		/* キー連動Ｐ／Ｗ ＵＰ信号 		*/
	   u1 u1_flb_kpwd		:1;   		/* キー連動Ｐ／Ｗ ＤＯＷＮＵＰ信号 */

	   u1 u1_flb_wpwu		:1;    		/* ワイアレス連動Ｐ／Ｗ ＵＰ信号 */
	   u1 u1_flb_wpwd		:1;    		/* ワイアレス連動Ｐ／Ｗ ＤＯＷＮ信号 */
	   u1 u1_flb_spwu		:1;    		/* スマート連動Ｐ／Ｗ ＵＰ信号	*/
	   u1 reserved3	 		:1;			/* リザーブ						*/
	} xn_lin_rmt;					/* ビットアクセスタグ */
}xn_lin_info_rmt_t;


/* TAG		: xn_lin_info_sig_t		*/
/* ABSTRACT	: ＳＷ信号情報共用体	*/
/* NOTE		: 各ＳＷ信号の状態を設定する			*/
typedef union {
	u1	u1_lin_sig_info;		/* バイトアクセスタグ	*/
	struct	{
	   u1 u1_flb_pws  	 	:1;			/* Ｐ／Ｗ作動許可信号 			*/
	   u1 u1_flb_ig   	 	:1;			/* ボディーＥＣＵ ＩＧ ＳＷ信号 */
	   u1 u1_flb_dcty     	:1;			/* Ｄ席カーテシＳＷ信号			*/
	   u1 reserved1	 		:1;			/* リザーブ						*/

	   u1 reserved2	 		:1;			/* リザーブ						*/
	   u1 reserved3	 		:1;			/* リザーブ						*/
	   u1 reserved4	 		:1;			/* リザーブ						*/
	   u1 reserved5			:1;			/* リザーブ						*/
	} xn_lin_sig;					/* ビットアクセスタグ */
}xn_lin_info_sig_t;


/* TAG		: xn_lin_info_cmt_t		*/
/* ABSTRACT	: 仕向け情報構造体		*/
/* NOTE		: 						*/
typedef struct
{
	u1	u1_lin_cmt;
}xn_lin_info_cmt_t;


/* TAG		: xn_lin_info_spd_t		*/
/* ABSTRACT	: 車速情報構造体		*/
/* NOTE		: 						*/
typedef struct
{
	u1	u1_lin_spd;
}xn_lin_info_spd_t;

#if 0
/* TAG		: xn_lin_info_ctm_t			*/
/* ABSTRACT	: カスタマイズ情報共用体	*/
/* NOTE		: 各カスタマイズ信号の状態を設定する		*/
typedef union {
	u1	u1_lin_info_cstm;		/* バイトアクセスタグ	*/
	struct	{
       u1 u1_flb_kupc 	 	:1;			/* カスタマイズ情報(キー連動Ｐ／Ｗ ＵＰ有無)	*/
       u1 u1_flb_kdnc 	 	:1;			/* カスタマイズ情報(キー連動Ｐ／Ｗ ＤＮ有無)	*/
	   u1 u1_flb_jpc	 	:1;			/* カスタマイズ情報(挟み込み防止ロジック)		*/
	   u1 u1_flb_wupc	 	:1;			/* カスタマイズ情報(ワイアレス連動Ｐ／Ｗ ＵＰ有無)	*/

	   u1 u1_flb_wdnc	 	:1;			/* カスタマイズ情報(ワイアレス連動Ｐ／Ｗ ＤＮ有無)	*/
	   u1 u1_flb_supc	 	:1;			/* カスタマイズ情報(スマート連動Ｐ／Ｗ ＵＰ有無)*/
	   u1 reserved1	 		:1;			/* リザーブ			*/
	   u1 reserved2	 		:1;			/* リザーブ			*/
	} xn_lin_cstm;					/* ビットアクセスタグ */
}xn_lin_info_cstm_t;
#endif

/* TAG		: xn_lin_info_mst_t		*/
/* ABSTRACT	: マスタＳＷ情報共用体	*/
/* NOTE		: マスタＳＷ信号の状態を設定する		*/
typedef union {
	u1	u1_lin_info_mst;	/* バイトアクセスタグ	*/
	struct	{
       u1 u1_flb_wl	 	 	:1;		/* マスタＳＷ情報	*/
	   u1 reserved1			:1;		/* リザーブ			*/
	   u1 reserved2			:1;		/* リザーブ			*/
	   u1 reserved3	 		:1;		/* リザーブ			*/

	   u1 reserved4			:4;		/* リザーブ			*/
	} xn_lin_mst;				/* ビットアクセスタグ */
}xn_lin_info_mst_t;

/*------ 受信情報 ------------------------*/
extern	xn_lin_rx_com_sts_t 	xng_lin_rx_com_sts;
extern	xn_lin_rx_com_fst_t		xng_lin_rx_com_fst;
extern	xn_lin_info_sig_t		xng_lin_info_sig;
extern  xn_lin_info_rmt_t 		xng_lin_info_rmt;
extern  xn_lin_info_cmt_t 		xng_lin_info_cmt;
extern  xn_lin_info_mst_t 		xng_lin_info_mst;
extern  xn_lin_info_spd_t 		xng_lin_info_spd;
#if 0
extern  xn_lin_info_cstm_t 		xng_lin_info_cstm;
#endif

#define	F_comdwn_01		xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_bod1s01
#define	F_comdwn_02		xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_bod1s02
#define	F_comdwn_03		xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_bod1s03
#define	F_comdwn_04		xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_bod1s04
#define	F_comdwn_05		xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_msw1s01
#define	F_comdwn_06		xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_dpw1s01

#define	F_comdwn_diag	xng_lin_rx_com_sts.xn_rx_com_sts.u1_flb_comdwn_diag

#define	F_id28_first	xng_lin_rx_com_fst.xn_rx_com_fst.u1_flb_id28_first_rx
#define	F_id29_first	xng_lin_rx_com_fst.xn_rx_com_fst.u1_flb_id29_first_rx
#define	F_id2a_first	xng_lin_rx_com_fst.xn_rx_com_fst.u1_flb_id2a_first_rx
#define	F_id22_first	xng_lin_rx_com_fst.xn_rx_com_fst.u1_flb_id22_first_rx
#define	F_id2b_first	xng_lin_rx_com_fst.xn_rx_com_fst.u1_flb_id2b_first_rx
#define	F_id24_first	xng_lin_rx_com_fst.xn_rx_com_fst.u1_flb_id24_first_rx

#define F_lin_ig		xng_lin_info_sig.xn_lin_sig.u1_flb_ig
#define F_lin_pws		xng_lin_info_sig.xn_lin_sig.u1_flb_pws
#define F_lin_dcty		xng_lin_info_sig.xn_lin_sig.u1_flb_dcty

#define F_lin_kpwu		xng_lin_info_rmt.xn_lin_rmt.u1_flb_kpwu
#define F_lin_kpwd		xng_lin_info_rmt.xn_lin_rmt.u1_flb_kpwd
#define F_lin_wpwu		xng_lin_info_rmt.xn_lin_rmt.u1_flb_wpwu
#define F_lin_wpwd		xng_lin_info_rmt.xn_lin_rmt.u1_flb_wpwd
#define F_lin_spwu		xng_lin_info_rmt.xn_lin_rmt.u1_flb_spwu

#define F_lin_kupc		xng_lin_info_cstm.xn_lin_cstm.u1_flb_kupc
#define F_lin_kdnc		xng_lin_info_cstm.xn_lin_cstm.u1_flb_kdnc
#define F_lin_jpc		xng_lin_info_cstm.xn_lin_cstm.u1_flb_jpc
#define F_lin_wupc		xng_lin_info_cstm.xn_lin_cstm.u1_flb_wupc
#define F_lin_wdnc		xng_lin_info_cstm.xn_lin_cstm.u1_flb_wdnc
#define F_lin_supc		xng_lin_info_cstm.xn_lin_cstm.u1_flb_supc

#define F_lin_wl		xng_lin_info_mst.xn_lin_mst.u1_flb_wl

#define U1g_lin_cmt		xng_lin_info_cmt.u1_lin_cmt
#define U1g_lin_spd		xng_lin_info_spd.u1_lin_spd

#endif /* P_LIN_APL_H */

/* End of p_lin_apl.h ***************************************/



