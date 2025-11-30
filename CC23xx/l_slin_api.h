/**
 * @file        l_slin_api.h
 *
 * @brief       SLIN API層
 *
 * @attention   編集禁止
 *
 */

#ifndef __L_SLIN_API_H_INCLUDE__
#define __L_SLIN_API_H_INCLUDE__


#include <stddef.h>
#include <stdint.h>
#include <ti/drivers/UART2.h>
#include <ti/drivers/timer/LGPTimerLPF3.h>



/* フレーム名の定義 */
#define  U1G_LIN_FRAME_ID20H			( (u1)(0x00u) )
#define  U1G_LIN_FRAME_ID30H			( (u1)(0x01u) )
#define  U1G_LIN_FRAME_ID33H           	( (u1)(0x02u) )
#define  U1G_LIN_FRAME_ID34H           	( (u1)(0x03u) )

#define  U1G_LIN_FRAME_ID28H           	( (u1)(0x00u) )
#define  U1G_LIN_FRAME_ID29H           	( (u1)(0x01u) )
#define  U1G_LIN_FRAME_ID2AH           	( (u1)(0x02u) )
#define  U1G_LIN_FRAME_ID22H           	( (u1)(0x03u) )
#define  U1G_LIN_FRAME_ID2BH           	( (u1)(0x04u) )
#define  U1G_LIN_FRAME_ID23H           	( (u1)(0x05u) )
#define  U1G_LIN_FRAME_ID24H         	( (u1)(0x06u) )
#define  U1G_LIN_FRAME_ID3CH           	( (u1)(0x07u) )
#define  U1G_LIN_FRAME_ID3DH           	( (u1)(0x08u) )

/* シグナルの名前(Table Type) */
/* ビットシンボルテーブル構造(LDF: Frames, Signals) */
typedef union {
    l_u8    u1g_lin_byte[U1G_LIN_MAX_DL];    /******    !!!この行は変更削除を行わないでください!!!    ******/
	struct	{
		l_u16	u1g_lin_frame28_01:8;

		l_u8	u1g_flb_kpwu:1;			/* キー連動P/W UP信号 */
		l_u8	u1g_flb_kpwd:1;			/* キー連動P/W DOWN信号 */
		l_u8	u1g_flb_wpwu:1;			/* ワイヤレス連動P/W UP信号 */
		l_u8	u1g_flb_wpwd:1;			/* ワイヤレス連動P/W DOWN信号 */

		l_u8	u1g_flb_spwu:1;			/* スマート連動P/W UP信号 */
		l_u8	reserved1   :1;
		l_u8	u1g_flb_pws :1;			/* P/W作動許可信号 */
		l_u8	u1g_flb_ig  :1;			/* ボディーECU IG SW信号 */

		l_u8	u1g_flb_dcty:1;			/* D席カーテシSW信号	*/
		l_u8	reserved2:7;

		l_u8	u1g_lin_frame28_04:8;
		l_u8	reserved3:8;
		l_u8	reserved4:8;
		l_u8	reserved5:8;
		l_u8	reserved6:8;
	} st_frame_bdb1s01;
	struct	{
		l_u8	reserved1:6;
		l_u8	u1g_dat_rl:2;			/* ハンドル情報 */
		l_u8	u1g_dat_cmt:8;			/* 仕向情報 */
		l_u8	u1g_dat_spd:8;			/* 車速情報	*/
		l_u8	u1g_dat_amd:8;			/* 外気温	*/

		l_u8	reserved2:8;
		l_u8	reserved3:8;
		l_u8	reserved4:8;
		l_u8	reserved5:8;
	} st_frame_bdb1s02; 
	struct	{
		l_u8	reserved1:4;
		l_u8	u1g_flb_warn_srbz:1;	/* S/Rウォーニング信号	*/
		l_u8	reserved2:1;
		l_u8	reserved3:1;
		l_u8	reserved4:1;

		l_u8	u1g_lin_frame23_02:8;
		l_u8	u1g_lin_frame23_03:8;
		l_u8	u1g_lin_frame23_04:8;
		l_u8	reserved5:8;
		l_u8	reserved6:8;
		l_u8	reserved7:8;
		l_u8	reserved8:8;
	} st_frame_srf1s01;
	struct	{
		l_u8	u1g_lin_frame24_01:8;

		l_u8	u1g_flb_kupc:1;			/* カスタマイズ情報(キー連動P/W UP有無)	*/
		l_u8	u1g_flb_kdnc:1;			/* カスタマイズ情報(キー連動P/W DN有無)	*/
		l_u8	reserved1   :1;			/* リザーブ	*/
		l_u8	u1g_flb_jpc :1;			/* カスタマイズ情報(挟み込み防止ロジック)	*/

		l_u8	u1g_flb_wupc:1;			/* カスタマイズ情報(ワイヤレス連動P/W UP有無)*/
		l_u8	u1g_flb_wdnc:1;			/* カスタマイズ情報(ワイヤレス連動P/W DN有無)*/
		l_u8	u1g_flb_supc:1;			/* カスタマイズ情報(スマート連動P/W UP有無)  */
		l_u8	reserved2	:1;			/* リザーブ */

		l_u8	u1g_lin_frame24_03:8;
		l_u8	u1g_lin_frame24_04:8;
		l_u8	reserved4:8;
		l_u8	reserved5:8;
		l_u8	reserved6:8;
		l_u8	reserved7:8;
	} st_frame_dpw1s01;
	struct	{
		l_u8	u1g_lin_frame3c_01:8;
		l_u8	u1g_lin_frame3c_02:8;
		l_u8	u1g_lin_frame3c_03:8;
		l_u8	u1g_lin_frame3c_04:8;
		l_u8	u1g_lin_frame3c_05:8;
		l_u8	u1g_lin_frame3c_06:8;
		l_u8	u1g_lin_frame3c_07:8;
		l_u8	u1g_lin_frame3c_08:8;
	} st_frame_srfdiag; 
} un_lin_data_type;

#define  U1G_LIN_FRAME_10H_D01H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[0]    /**< @brief 10Hフレームの設定 D01 */
#define  U1G_LIN_FRAME_10H_D02H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[1]    /**< @brief 10Hフレームの設定 D02 */
#define  U1G_LIN_FRAME_10H_D03H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[2]    /**< @brief 10Hフレームの設定 D03 */
#define  U1G_LIN_FRAME_10H_D04H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[3]    /**< @brief 10Hフレームの設定 D04 */
#define  U1G_LIN_FRAME_10H_D05H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[4]    /**< @brief 10Hフレームの設定 D05 */
#define  U1G_LIN_FRAME_10H_D06H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[5]    /**< @brief 10Hフレームの設定 D06 */
#define  U1G_LIN_FRAME_10H_D07H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[6]    /**< @brief 10Hフレームの設定 D07 */
#define  U1G_LIN_FRAME_10H_D08H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[7]    /**< @brief 10Hフレームの設定 D08 */

#define  U1G_LIN_FRAME_11H_D01H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[0]    /**< @brief 11Hフレームの設定 D01 */
#define  U1G_LIN_FRAME_11H_D02H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[1]    /**< @brief 11Hフレームの設定 D02 */
#define  U1G_LIN_FRAME_11H_D03H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[2]    /**< @brief 11Hフレームの設定 D03 */
#define  U1G_LIN_FRAME_11H_D04H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[3]    /**< @brief 11Hフレームの設定 D04 */
#define  U1G_LIN_FRAME_11H_D05H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[4]    /**< @brief 11Hフレームの設定 D05 */
#define  U1G_LIN_FRAME_11H_D06H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[5]    /**< @brief 11Hフレームの設定 D06 */
#define  U1G_LIN_FRAME_11H_D07H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[6]    /**< @brief 11Hフレームの設定 D07 */
#define  U1G_LIN_FRAME_11H_D08H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[7]    /**< @brief 11Hフレームの設定 D08 */


/* 同期フラグ : フレーム送信完了フラグ */
#define  b1g_LIN_TxFrameComplete_ID30h  xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot2

#define  F1g_lin_txframe_id23h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot6
#define  F1g_lin_txframe_id3dh          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot9

/* 同期フラグ : フレーム受信完了フラグ */
#define  b1g_LIN_RxFrameComplete_ID20h  xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot1
#define  b1g_LIN_RxFrameComplete_ID33h  xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot3
#define  b1g_LIN_RxFrameComplete_ID34h  xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot4

#define  F1g_lin_rxframe_id28h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot1
#define  F1g_lin_rxframe_id29h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot2
#define  F1g_lin_rxframe_id2ah          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot3
#define  F1g_lin_rxframe_id22h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot4
#define  F1g_lin_rxframe_id2bh          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot5
#define  F1g_lin_rxframe_id24h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot7
#define  F1g_lin_rxframe_id3ch          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot8

/* 同期フラグ : フレームエラーの発生状況フラグ */
#define  F4g_lin_errframe_id28h         xng_lin_frm_buf[0].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id29h         xng_lin_frm_buf[1].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id2ah         xng_lin_frm_buf[2].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id22h         xng_lin_frm_buf[3].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id2bh         xng_lin_frm_buf[4].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id23h         xng_lin_frm_buf[5].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id24h         xng_lin_frm_buf[6].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id3ch         xng_lin_frm_buf[7].un_state.st_err.u2g_lin_err
#define  F4g_lin_errframe_id3dh         xng_lin_frm_buf[8].un_state.st_err.u2g_lin_err



/***************************************************************************/
/***************************************************************************/
/*              !!!以降は 変更、削除をおこなわないで下さい!!!              */
/***************************************************************************/
/***************************************************************************/

/* 同期フラグ: Physical Busエラー発生状況フラグ */
#define  F1g_lin_p_bus_err_ch1          xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err

/* 同期フラグ：ヘッダエラーの発生状況フラグ（エラー個別） */
#define  F1g_lin_header_err_timeout_ch1 xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time
#define  F1g_lin_header_err_uart_ch1    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart
#define  F1g_lin_header_err_synch_ch1   xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_synch
#define  F1g_lin_header_err_parity_ch1  xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_pari

/* 同期フラグ：ヘッダエラーの発生状況フラグ（エラー一括） */
#define  F4g_lin_header_err_ch1         xng_lin_sts_buf.un_state.st_err.u2g_lin_e_head


/********************************/
/* LIN API MACRO/EXTERN         */
/********************************/
/***** API関数 外部参照宣言 *****/
extern void     l_ifc_init_ch1(void);
extern void     l_ifc_init_com_ch1(void);
extern void     l_ifc_init_drv_ch1(void);
extern l_bool   l_ifc_connect_ch1(void);
extern l_bool   l_ifc_disconnect_ch1(void);
extern void     l_ifc_wake_up_ch1(void);
extern l_u16    l_ifc_read_status_ch1(void);
extern void     l_ifc_sleep_ch1(void);
extern void     l_ifc_run_ch1(void);
extern void     l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data, size_t count, void *userArg, int_fast16_t u1a_lin_rx_status);
extern void     l_ifc_tx_ch1(UART2_Handle handle, void *u1a_lin_tx_data, size_t count, void *userArg, int_fast16_t u1a_lin_tx_status);
extern void     l_ifc_tm_ch1(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask);
extern void     l_ifc_aux_ch1(uint_least8_t index);
extern l_u16    l_ifc_read_lb_status_ch1(void);
extern void     l_slot_enable_ch1(l_frame_handle  u1a_lin_frm);
extern void     l_slot_disable_ch1(l_frame_handle  u1a_lin_frm);
extern void     l_slot_rd_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, l_u8* pta_lin_data);
extern void     l_slot_wr_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, const l_u8* pta_lin_data);
extern void     l_slot_set_default_ch1(l_frame_handle u1a_lin_frm);
extern void     l_slot_set_fail_ch1(l_frame_handle  u1a_lin_frm);

#endif

/***** End of File *****/