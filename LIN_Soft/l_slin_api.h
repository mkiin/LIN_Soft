/**
 * @file        l_slin_api.h
 *
 * @brief       SLIN APIå±¤
 *
 * @attention   ç·¨é›†ç¦æ­¢
 *
 */

#ifndef __L_SLIN_API_H_INCLUDE__
#define __L_SLIN_API_H_INCLUDE__


#include <stddef.h>
#include <stdint.h>
#include "../UART_DRV/UART_RegInt.h"
#include <ti/drivers/timer/LGPTimerLPF3.h>



/* ãƒ•ãƒ¬ãƒ¼ãƒ åã®å®šç¾© */
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

/* ã‚·ã‚°ãƒŠãƒ«ã®åå‰(Table Type) */
/* ãƒ“ãƒƒãƒˆã‚·ãƒ³ãƒœãƒ«ãƒ†ãƒ¼ãƒ–ãƒ«æ§‹é€ (LDF: Frames, Signals) */
typedef union {
    l_u8    u1g_lin_byte[U1G_LIN_MAX_DL];    /******    !!!ã“ã®è¡Œã¯å¤‰æ›´å‰Šé™¤ã‚’è¡Œã‚ãªã„ã§ãã ã•ã„!!!    ******/
	struct	{
		l_u16	u1g_lin_frame28_01:8;

		l_u8	u1g_flb_kpwu:1;			/* ã‚­ãƒ¼é€£å‹•P/W UPä¿¡å· */
		l_u8	u1g_flb_kpwd:1;			/* ã‚­ãƒ¼é€£å‹•P/W DOWNä¿¡å· */
		l_u8	u1g_flb_wpwu:1;			/* ãƒ¯ã‚¤ãƒ¤ãƒ¬ã‚¹é€£å‹•P/W UPä¿¡å· */
		l_u8	u1g_flb_wpwd:1;			/* ãƒ¯ã‚¤ãƒ¤ãƒ¬ã‚¹é€£å‹•P/W DOWNä¿¡å· */

		l_u8	u1g_flb_spwu:1;			/* ã‚¹ãƒžãƒ¼ãƒˆé€£å‹•P/W UPä¿¡å· */
		l_u8	reserved1   :1;
		l_u8	u1g_flb_pws :1;			/* P/Wä½œå‹•è¨±å¯ä¿¡å· */
		l_u8	u1g_flb_ig  :1;			/* ãƒœãƒ‡ã‚£ãƒ¼ECU IG SWä¿¡å· */

		l_u8	u1g_flb_dcty:1;			/* Då¸­ã‚«ãƒ¼ãƒ†ã‚·SWä¿¡å·	*/
		l_u8	reserved2:7;

		l_u8	u1g_lin_frame28_04:8;
		l_u8	reserved3:8;
		l_u8	reserved4:8;
		l_u8	reserved5:8;
		l_u8	reserved6:8;
	} st_frame_bdb1s01;
	struct	{
		l_u8	reserved1:6;
		l_u8	u1g_dat_rl:2;			/* ãƒãƒ³ãƒ‰ãƒ«æƒ…å ± */
		l_u8	u1g_dat_cmt:8;			/* ä»•å‘æƒ…å ± */
		l_u8	u1g_dat_spd:8;			/* è»Šé€Ÿæƒ…å ±	*/
		l_u8	u1g_dat_amd:8;			/* å¤–æ°—æ¸©	*/

		l_u8	reserved2:8;
		l_u8	reserved3:8;
		l_u8	reserved4:8;
		l_u8	reserved5:8;
	} st_frame_bdb1s02;
	struct	{
		l_u8	reserved1:4;
		l_u8	u1g_flb_warn_srbz:1;	/* S/Rã‚¦ã‚©ãƒ¼ãƒ‹ãƒ³ã‚°ä¿¡å·	*/
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

		l_u8	u1g_flb_kupc:1;			/* ã‚«ã‚¹ã‚¿ãƒžã‚¤ã‚ºæƒ…å ±(ã‚­ãƒ¼é€£å‹•P/W UPæœ‰ç„¡)	*/
		l_u8	u1g_flb_kdnc:1;			/* ã‚«ã‚¹ã‚¿ãƒžã‚¤ã‚ºæƒ…å ±(ã‚­ãƒ¼é€£å‹•P/W DNæœ‰ç„¡)	*/
		l_u8	reserved1   :1;			/* ãƒªã‚¶ãƒ¼ãƒ–	*/
		l_u8	u1g_flb_jpc :1;			/* ã‚«ã‚¹ã‚¿ãƒžã‚¤ã‚ºæƒ…å ±(æŒŸã¿è¾¼ã¿é˜²æ­¢ãƒ­ã‚¸ãƒƒã‚¯)	*/

		l_u8	u1g_flb_wupc:1;			/* ã‚«ã‚¹ã‚¿ãƒžã‚¤ã‚ºæƒ…å ±(ãƒ¯ã‚¤ãƒ¤ãƒ¬ã‚¹é€£å‹•P/W UPæœ‰ç„¡)*/
		l_u8	u1g_flb_wdnc:1;			/* ã‚«ã‚¹ã‚¿ãƒžã‚¤ã‚ºæƒ…å ±(ãƒ¯ã‚¤ãƒ¤ãƒ¬ã‚¹é€£å‹•P/W DNæœ‰ç„¡)*/
		l_u8	u1g_flb_supc:1;			/* ã‚«ã‚¹ã‚¿ãƒžã‚¤ã‚ºæƒ…å ±(ã‚¹ãƒžãƒ¼ãƒˆé€£å‹•P/W UPæœ‰ç„¡)  */
		l_u8	reserved2	:1;			/* ãƒªã‚¶ãƒ¼ãƒ– */

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

#define  U1G_LIN_FRAME_10H_D01H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[0]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D01 */
#define  U1G_LIN_FRAME_10H_D02H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[1]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D02 */
#define  U1G_LIN_FRAME_10H_D03H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[2]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D03 */
#define  U1G_LIN_FRAME_10H_D04H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[3]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D04 */
#define  U1G_LIN_FRAME_10H_D05H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[4]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D05 */
#define  U1G_LIN_FRAME_10H_D06H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[5]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D06 */
#define  U1G_LIN_FRAME_10H_D07H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[6]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D07 */
#define  U1G_LIN_FRAME_10H_D08H       xng_lin_frm_buf[U1G_LIN_FRAME_ID10H].xng_lin_data.u1g_lin_byte[7]    /**< @brief 10Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D08 */

#define  U1G_LIN_FRAME_11H_D01H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[0]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D01 */
#define  U1G_LIN_FRAME_11H_D02H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[1]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D02 */
#define  U1G_LIN_FRAME_11H_D03H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[2]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D03 */
#define  U1G_LIN_FRAME_11H_D04H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[3]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D04 */
#define  U1G_LIN_FRAME_11H_D05H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[4]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D05 */
#define  U1G_LIN_FRAME_11H_D06H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[5]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D06 */
#define  U1G_LIN_FRAME_11H_D07H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[6]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D07 */
#define  U1G_LIN_FRAME_11H_D08H       xng_lin_frm_buf[U1G_LIN_FRAME_ID11H].xng_lin_data.u1g_lin_byte[7]    /**< @brief 11Hãƒ•ãƒ¬ãƒ¼ãƒ ã®è¨­å®š D08 */


/* åŒæœŸãƒ•ãƒ©ã‚° : ãƒ•ãƒ¬ãƒ¼ãƒ é€ä¿¡å®Œäº†ãƒ•ãƒ©ã‚° */
#define  b1g_LIN_TxFrameComplete_ID30h  xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot2

#define  F1g_lin_txframe_id23h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot6
#define  F1g_lin_txframe_id3dh          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot9

/* åŒæœŸãƒ•ãƒ©ã‚° : ãƒ•ãƒ¬ãƒ¼ãƒ å—ä¿¡å®Œäº†ãƒ•ãƒ©ã‚° */
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

/* åŒæœŸãƒ•ãƒ©ã‚° : ãƒ•ãƒ¬ãƒ¼ãƒ ã‚¨ãƒ©ãƒ¼ã®ç™ºç”ŸçŠ¶æ³ãƒ•ãƒ©ã‚° */
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
/*              !!!ä»¥é™ã¯ å¤‰æ›´ã€å‰Šé™¤ã‚’ãŠã“ãªã‚ãªã„ã§ä¸‹ã•ã„!!!              */
/***************************************************************************/
/***************************************************************************/

/* åŒæœŸãƒ•ãƒ©ã‚°: Physical Busã‚¨ãƒ©ãƒ¼ç™ºç”ŸçŠ¶æ³ãƒ•ãƒ©ã‚° */
#define  F1g_lin_p_bus_err_ch1          xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err

/* åŒæœŸãƒ•ãƒ©ã‚°ï¼šãƒ˜ãƒƒãƒ€ã‚¨ãƒ©ãƒ¼ã®ç™ºç”ŸçŠ¶æ³ãƒ•ãƒ©ã‚°ï¼ˆã‚¨ãƒ©ãƒ¼å€‹åˆ¥ï¼‰ */
#define  F1g_lin_header_err_timeout_ch1 xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time
#define  F1g_lin_header_err_uart_ch1    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart
#define  F1g_lin_header_err_synch_ch1   xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_synch
#define  F1g_lin_header_err_parity_ch1  xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_pari

/* åŒæœŸãƒ•ãƒ©ã‚°ï¼šãƒ˜ãƒƒãƒ€ã‚¨ãƒ©ãƒ¼ã®ç™ºç”ŸçŠ¶æ³ãƒ•ãƒ©ã‚°ï¼ˆã‚¨ãƒ©ãƒ¼ä¸€æ‹¬ï¼‰ */
#define  F4g_lin_header_err_ch1         xng_lin_sts_buf.un_state.st_err.u2g_lin_e_head


/********************************/
/* LIN API MACRO/EXTERN         */
/********************************/
/***** APIé–¢æ•° å¤–éƒ¨å‚ç…§å®£è¨€ *****/
extern void     l_ifc_init_ch1(void);
extern void     l_ifc_init_com_ch1(void);
extern void     l_ifc_init_drv_ch1(void);
extern l_bool   l_ifc_connect_ch1(void);
extern l_bool   l_ifc_disconnect_ch1(void);
extern void     l_ifc_wake_up_ch1(void);
extern l_u16    l_ifc_read_status_ch1(void);
extern void     l_ifc_sleep_ch1(void);
extern void     l_ifc_run_ch1(void);
extern void     l_ifc_rx_ch1(UART_RegInt_Handle handle, void *u1a_lin_rx_data, size_t count, void *userArg, int_fast16_t u1a_lin_rx_status);
extern void     l_ifc_tx_ch1(UART_RegInt_Handle handle, void *u1a_lin_tx_data, size_t count, void *userArg, int_fast16_t u1a_lin_tx_status);
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
