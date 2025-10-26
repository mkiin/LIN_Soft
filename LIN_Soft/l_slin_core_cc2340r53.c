/**
 * @file        l_slin_core_CC2340R53.c
 *
 * @brief       SLIN COREå±¤
 *
 * @attention   ç·¨é›†ç¦æ­¢
 *
 */

#pragma	section	lin

/***** ãƒ˜ãƒƒãƒ€ ã‚¤ãƒ³ã‚¯ãƒ«ãƒ¼ãƒ‰ *****/
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_stddef.h"
#include "l_slin_core_cc2340r53.h"
#include "l_slin_drv_cc2340r53.h"
#if 1   /* ç¶™ç¶šãƒ•ãƒ¬ãƒ¼ãƒ é€ä¿¡æ™‚ã«é€ä¿¡å®Œäº†ãƒ•ãƒ©ã‚°ã§èª¤æ¤œçŸ¥ã—ã¦ã„ã‚‹å¯èƒ½æ€§ãŒã‚ã‚Šã€æœŸå¾…ã™ã‚‹ãƒ•ãƒ¬ãƒ¼ãƒ ãŒé€ä¿¡ã•ã‚ŒãŸã‹ã‚’ç¢ºèªã™ã‚‹ãŸã‚ã€LINé€šä¿¡ç®¡ç†ã¸ã®ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯ã‚’å®Ÿè¡Œã™ã‚‹ */
#include "LIN_Manager.h"
#endif

/***** é–¢æ•°ãƒ—ãƒ­ãƒˆã‚¿ã‚¤ãƒ—å®£è¨€ *****/
/*-- APIé–¢æ•°å®šç¾©(extern) --*/
void   l_ifc_init_ch1(void);
void   l_ifc_init_com_ch1(void);
void   l_ifc_init_drv_ch1(void);
l_bool l_ifc_connect_ch1(void);
l_bool l_ifc_disconnect_ch1(void);
void   l_ifc_wake_up_ch1(void);
l_u16  l_ifc_read_status_ch1(void);
void   l_ifc_sleep_ch1(void);
void   l_ifc_run_ch1(void);
l_u16  l_ifc_read_lb_status_ch1(void);
void   l_slot_enable_ch1(l_frame_handle  u1a_lin_frm);
void   l_slot_disable_ch1(l_frame_handle  u1a_lin_frm);
void   l_slot_rd_ch1(l_frame_handle  u1a_lin_frm, l_u8 u1a_lin_cnt, l_u8 * pta_lin_data);
void   l_slot_wr_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, const l_u8 * pta_lin_data);
void   l_slot_set_default_ch1(l_frame_handle u1a_lin_frm);
void   l_slot_set_fail_ch1(l_frame_handle  u1a_lin_frm);
l_u8   l_vog_lin_check_header( l_u8 u1a_lin_data[] ,l_u8 u1a_lin_err );
/*-- é–¢æ•°å®šç¾©(extern) --*/
void   l_vog_lin_rx_int(l_u8 u1a_lin_data[], l_u8 u1a_lin_err);
void   l_vog_lin_tm_int(void);
void   l_vog_lin_irq_int(void);
void   l_vog_lin_set_nm_info(l_u8  u1a_lin_nm_info);
l_u8   l_u1g_lin_tbl_chk(void);
l_u8   l_vog_lin_checksum(l_u8 u1a_lin_pid ,const l_u8* u1a_lin_data, l_u8 u1a_lin_data_length, U1G_LIN_ChecksumType type);

/*-- é–¢æ•°å®šç¾©(static) --*/
static void   l_vol_lin_set_wakeup(void);
static void   l_vol_lin_set_synchbreak(void);
static void   l_vol_lin_set_frm_complete(l_u8  u1a_lin_err);

/*** å¤‰æ•°å®šç¾©(static) ***/
static st_lin_id_slot_type  xnl_lin_id_sl;                     /* ä½¿ç”¨ä¸­ã®ãƒ•ãƒ¬ãƒ¼ãƒ ã®IDã¨ã‚¹ãƒ­ãƒƒãƒˆç•ªå·ã®ç®¡ç†ãƒ†ãƒ¼ãƒ–ãƒ« */
static l_u8   u1l_lin_slv_sts;                                 /* ã‚¹ãƒ¬ãƒ¼ãƒ–ã‚¿ã‚¹ã‚¯ç”¨ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ */
static l_u8   u1l_lin_frm_sz;                                  /* ãƒ‡ãƒ¼ã‚¿ã‚µã‚¤ã‚º */
static l_u8   u1l_lin_rs_cnt;                                  /* é€å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚«ã‚¦ãƒ³ã‚¿ */
static l_u8   u1l_lin_rs_tmp[ U1G_LIN_DATA_SUM_LEN ];          /* é€å—ä¿¡ãƒ‡ãƒ¼ã‚¿ç”¨tmpãƒãƒƒãƒ•ã‚¡(Data + Checksum) 9ãƒã‚¤ãƒˆ */
static l_u8   u1l_lin_chksum;                                  /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ æ ¼ç´ */
static l_u16  u2l_lin_herr_cnt;                                /* ãƒ˜ãƒƒãƒ€ã‚¿ã‚¤ãƒ ã‚¢ã‚¦ãƒˆã‚¨ãƒ©ãƒ¼å›žæ•°ã‚«ã‚¦ãƒ³ã‚¿ï¼ˆPhysical Busã‚¨ãƒ©ãƒ¼æ¤œå‡ºç”¨ï¼‰ */

extern l_u8   u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];       /**< @brief å—ä¿¡ãƒãƒƒãƒ•ã‚¡ */
extern l_u8   u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE];      /**< @brief é€ä¿¡ãƒãƒƒãƒ•ã‚¡ */
/*===========================================================================================*/

/**********************************/
/* MCUéžä¾å­˜APIé–¢æ•°å‡¦ç†           */
/**********************************/
/**************************************************/
/*  ãƒãƒƒãƒ•ã‚¡ã€ãƒ‰ãƒ©ã‚¤ãƒã®åˆæœŸåŒ–å‡¦ç†(API)           */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_ifc_init_ch1(void)
{
    l_ifc_init_com_ch1();                                               /* LINãƒãƒƒãƒ•ã‚¡ã®åˆæœŸåŒ–(API) */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RESET;    /* RESETçŠ¶æ…‹ã«ã™ã‚‹ */
    l_ifc_init_drv_ch1();                                               /* LINãƒ‰ãƒ©ã‚¤ãƒã®åˆæœŸåŒ–(API) */
    u1g_lin_syserr = U1G_LIN_BYTE_CLR;                                  /* ã‚·ã‚¹ãƒ†ãƒ ç•°å¸¸ãƒ•ãƒ©ã‚°ã®ã‚¯ãƒªã‚¢ */
}

/**************************************************/
/*  LINãƒãƒƒãƒ•ã‚¡ã®åˆæœŸåŒ– å‡¦ç†(API)                 */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_ifc_init_com_ch1(void)
{
    l_u8    u1a_lin_lp1;

  /*--- LINã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ãƒãƒƒãƒ•ã‚¡åˆæœŸåŒ– ---*/
    /* NADã®ã‚»ãƒƒãƒˆ */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_nad = U2G_LIN_NAD;

    /* Physical Busã‚¨ãƒ©ãƒ¼ãƒ•ãƒ©ã‚°ã®ã‚¯ãƒªã‚¢ */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_CLR;

    /* ãƒ˜ãƒƒãƒ€ãƒ¼ã‚¨ãƒ©ãƒ¼ãƒ•ãƒ©ã‚°ã®ã‚¯ãƒªã‚¢ */
    xng_lin_sts_buf.un_state.st_err.u2g_lin_e_head = U2G_LIN_BYTE_CLR;

    /* é€å—ä¿¡å‡¦ç†å®Œäº†ãƒ•ãƒ©ã‚°ã®ã‚¯ãƒªã‚¢ */
    xng_lin_sts_buf.un_rs_flg1.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg2.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg3.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg4.u2g_lin_word = U2G_LIN_WORD_CLR;

    /*--- LINãƒ•ãƒ¬ãƒ¼ãƒ ãƒãƒƒãƒ•ã‚¡åˆæœŸåŒ– ---*/
    for( u1a_lin_lp1 = U1G_LIN_0; u1a_lin_lp1 < U1G_LIN_MAX_SLOT; u1a_lin_lp1++ ){

        l_u1g_lin_irq_dis();                                            /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

        /* ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹éƒ¨åˆ†(ã‚¨ãƒ©ãƒ¼ãƒ•ãƒ©ã‚°ç­‰)ã‚¯ãƒªã‚¢ */
        xng_lin_frm_buf[ u1a_lin_lp1 ].un_state.u2g_lin_word = U2G_LIN_BYTE_CLR;

        /* LINãƒãƒƒãƒ•ã‚¡ã®ãƒ‡ãƒ¼ã‚¿éƒ¨ã‚’ãƒ‡ãƒ•ã‚©ãƒ«ãƒˆå€¤ã§åˆæœŸåŒ–ã™ã‚‹ */
        /* ãƒ‡ãƒ•ã‚©ãƒ«ãƒˆå€¤è¨­å®š */
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_0 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_1 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_2 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_3 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_4 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_5 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_6 ];
        xng_lin_frm_buf[ u1a_lin_lp1 ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ]
                                = xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_def[ U1G_LIN_7 ];

        l_vog_lin_irq_res();                                            /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
    }
}


/**************************************************/
/*  LINãƒ‰ãƒ©ã‚¤ãƒã®åˆæœŸåŒ– å‡¦ç†(API)                 */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_ifc_init_drv_ch1(void)
{
    /*** UARTã®åˆæœŸåŒ– ***/
    l_vog_lin_uart_init();

    /*** å¤–éƒ¨INTã®åˆæœŸåŒ– ***/
    l_vog_lin_int_init();

    u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                                /* Physical Busã‚¨ãƒ©ãƒ¼æ¤œå‡ºã‚«ã‚¦ãƒ³ã‚¿ã‚¯ãƒªã‚¢ */

    /* ã‚¹ãƒ¬ãƒ¼ãƒ–ã‚¿ã‚¹ã‚¯ã®ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚’ç¢ºèª */
    switch( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts ){
    /* SLEEPçŠ¶æ…‹ */
    case ( U2G_LIN_STS_SLEEP ):
        l_vol_lin_set_wakeup();                                         /* Wakeupå¾…æ©Ÿè¨­å®š */
        break;
    /* RUNçŠ¶æ…‹ / RUN STANDBYçŠ¶æ…‹ */
    case ( U2G_LIN_STS_RUN_STANDBY ):
    case ( U2G_LIN_STS_RUN ):
        /* SynchBreakå—ä¿¡å¾…ã¡çŠ¶æ…‹ã«è¨­å®š */
        xng_lin_bus_sts.u2g_lin_word &= U2G_LIN_STSBUF_CLR;             /* ãƒªãƒ¼ãƒ‰ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ãƒãƒƒãƒ•ã‚¡ã®ã‚¯ãƒªã‚¢ */

        l_vol_lin_set_synchbreak();                                     /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */

        break;
    default:
        /* ä¸Šè¨˜ä»¥å¤–ã¯ã€ä½•ã‚‚ã—ãªã„ */
        break;
    }
}


/**************************************************/
/*  LINé€šä¿¡æŽ¥ç¶š å‡¦ç†(API)                         */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š å‡¦ç†çµæžœ                               */
/*         (0 / 1) : å‡¦ç†æˆåŠŸ / å‡¦ç†å¤±æ•—          */
/**************************************************/
l_bool  l_ifc_connect_ch1(void)
{
    l_bool  u2a_lin_result;

    /* ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ãŒRESETçŠ¶æ…‹ã®å ´åˆ */
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RESET ) {
        /* SLEEPã¸ç§»è¡Œ */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_SLEEP;        /* SLEEPçŠ¶æ…‹ã¸ç§»è¡Œ */
        l_vol_lin_set_wakeup();                                                 /* Wakeupå¾…æ©Ÿè¨­å®š */

        u2a_lin_result = U2G_LIN_OK;
    }
    /* RESETçŠ¶æ…‹ä»¥å¤–ã®å ´åˆ */
    else {
        u2a_lin_result = U2G_LIN_NG;
    }

    return( u2a_lin_result );
}


/**************************************************/
/*  LINåˆ‡æ–­ å‡¦ç†(API)                             */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š å‡¦ç†çµæžœ                               */
/*         (0 / 1) : å‡¦ç†æˆåŠŸ / å‡¦ç†å¤±æ•—          */
/**************************************************/
l_bool  l_ifc_disconnect_ch1(void)
{
    l_bool  u2a_lin_result;

    l_u1g_lin_irq_dis();                                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    /* SLEEPçŠ¶æ…‹ã®å ´åˆ */
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP )
    {
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RESET;
      /* UARTã«ã‚ˆã‚‹WAKEUPæ¤œå‡ºã®å ´åˆ */
      #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
        l_vog_lin_rx_dis();                                     /* å—ä¿¡å‰²ã‚Šè¾¼ã¿ã‚’ç¦æ­¢ã™ã‚‹ */
      /* INTå‰²ã‚Šè¾¼ã¿ã«ã‚ˆã‚‹WAKEUPæ¤œå‡ºã®å ´åˆ */
      #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
        l_vog_lin_int_dis();                                    /* INTå‰²ã‚Šè¾¼ã¿ã‚’ç¦æ­¢ã™ã‚‹ */
      #endif

        u2a_lin_result = U2G_LIN_OK;
    }
    /* RESETçŠ¶æ…‹ã®å ´åˆ */
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RESET ){
        /* ãªã«ã‚‚ã›ãšã€å‡¦ç†æˆåŠŸã‚’è¿”ã™ */
        u2a_lin_result = U2G_LIN_OK;
    }
    /* SLEEPã§ã‚‚RESETã§ã‚‚ãªã„å ´åˆ */
    else{
        u2a_lin_result = U2G_LIN_NG;
    }

    l_vog_lin_irq_res();                                        /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */

    return( u2a_lin_result );
}


/**************************************************/
/*  wakeupã‚³ãƒžãƒ³ãƒ‰é€ä¿¡ å‡¦ç†(API)                  */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_ifc_wake_up_ch1(void)
{
    l_u16 u2a_lin_sts;
    l_u8 u1a_lin_wakeup_data;
    l_u1g_lin_irq_dis();                                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */
    u1a_lin_wakeup_data = U1G_LIN_SND_WAKEUP_DATA;
    u2a_lin_sts = xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts;

    l_vog_lin_irq_res();                                    /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */

    /* RUN STANDBYçŠ¶æ…‹ã®å ´åˆ */
    if( u2a_lin_sts == U2G_LIN_STS_RUN_STANDBY ){
        /* wakeupãƒ‘ãƒ«ã‚¹ã‚’é€ä¿¡ */
        l_vog_lin_tx_char( &u1a_lin_wakeup_data,U1G_LIN_DL_1 );
    }
    /* SLEEPçŠ¶æ…‹å ´åˆ */
    else if(u2a_lin_sts == U2G_LIN_STS_SLEEP){
      /* UARTã«ã‚ˆã‚‹WAKEUPæ¤œå‡ºã®å ´åˆ */
      #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
        l_vog_lin_rx_dis();                                     /* å—ä¿¡å‰²ã‚Šè¾¼ã¿ã‚’ç¦æ­¢ã™ã‚‹ */
      /* INTå‰²ã‚Šè¾¼ã¿ã«ã‚ˆã‚‹WAKEUPæ¤œå‡ºã®å ´åˆ */
      #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
        l_vog_lin_int_dis();                                    /* INTå‰²ã‚Šè¾¼ã¿ã‚’ç¦æ­¢ã™ã‚‹ */
      #endif
        /* wakeupãƒ‘ãƒ«ã‚¹ã‚’é€ä¿¡ */
        l_vog_lin_tx_char( &u1a_lin_wakeup_data ,U1G_LIN_DL_1);
    }
    /* RUN STANDBY / SLEEPçŠ¶æ…‹ä»¥å¤–ã®å ´åˆ */
    else{
        /* ãªã«ã‚‚ã—ãªã„ */
    }
/* Ver 2.10 å¤‰æ›´ï¼ˆç®¡ç†ç•ªå·15ï¼‰:Wakeupãƒ‘ãƒ«ã‚¹é€ä¿¡å¾Œã€ç«‹ä¸‹ã‚Šã‚¨ãƒƒã‚¸ã§æ¤œå‡ºã—ãŸæ™‚ã®Wakeupãƒ‘ãƒ«ã‚¹é€ä¿¡ä¸­æ–­å¯¾ç­– */
}


/**************************************************/
/*  LINãƒã‚¹çŠ¶æ…‹ãƒªãƒ¼ãƒ‰ å‡¦ç†(API)                   */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š LINãƒã‚¹çŠ¶æ…‹                            */
/**************************************************/
l_u16  l_ifc_read_status_ch1(void)
{
    l_u16  u2a_lin_result;

    l_u1g_lin_irq_dis();                                    /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    u2a_lin_result = xng_lin_bus_sts.u2g_lin_word;
    xng_lin_bus_sts.u2g_lin_word &= U2G_LIN_STSBUF_CLR;     /* ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚¯ãƒªã‚¢ */

    l_vog_lin_irq_res();                                    /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */

    return( u2a_lin_result );
}


/**************************************************/
/*  SLEEPçŠ¶æ…‹ç§»è¡Œ å‡¦ç†(API)                       */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_ifc_sleep_ch1(void)
{
    l_u1g_lin_irq_dis();                                    /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    /* RUN / RUN STANDBYçŠ¶æ…‹ã®å ´åˆ */
    if( (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN)
     || (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY) ){

        /* SLEEPã¸ç§»è¡Œ */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_SLEEP;    /* SLEEPçŠ¶æ…‹ã«ç§»è¡Œ */
        l_vol_lin_set_wakeup();                                             /* Wakeupå¾…æ©Ÿè¨­å®š */
    }
    /* RUN / RUN STANDBYä»¥å¤–ã®å ´åˆ ãªã«ã‚‚ã—ãªã„ */

    l_vog_lin_irq_res();                                    /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */

}


/**************************************************/
/*  LINã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã®ãƒªãƒ¼ãƒ‰ å‡¦ç†(API)               */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š LINã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹                          */
/**************************************************/
l_u16  l_ifc_read_lb_status_ch1(void)
{
    l_u16  u2a_lin_result;

    /* ç¾åœ¨ã®LINã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚’æ ¼ç´ */
    u2a_lin_result = xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts;

    return( u2a_lin_result );
}


/**************************************************/
/*  ã‚¹ãƒ­ãƒƒãƒˆæœ‰åŠ¹ãƒ•ãƒ©ã‚°ã‚¯ãƒªã‚¢å‡¦ç†(API)             */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š frame: ãƒ•ãƒ¬ãƒ¼ãƒ å                      */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_slot_enable_ch1(l_frame_handle  u1a_lin_frm)
{
    /* å¼•æ•°å€¤ãŒç¯„å›²å†…ã®å ´åˆ */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){
        /* LINãƒãƒƒãƒ•ã‚¡ ã‚¹ãƒ­ãƒƒãƒˆæœ‰åŠ¹ */
        xng_lin_frm_buf[ u1a_lin_frm ].un_state.st_bit.u2g_lin_no_use = U2G_LIN_BIT_CLR;
    }
    /* å¼•æ•°å€¤ãŒç¯„å›²å¤–ã®å ´åˆ ãªã«ã‚‚ã—ãªã„ */
}


/**************************************************/
/*  ã‚¹ãƒ­ãƒƒãƒˆç„¡åŠ¹ãƒ•ãƒ©ã‚°ã‚»ãƒƒãƒˆå‡¦ç†(API)             */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š frame: ãƒ•ãƒ¬ãƒ¼ãƒ å                      */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_slot_disable_ch1(l_frame_handle  u1a_lin_frm)
{
    /* å¼•æ•°å€¤ãŒç¯„å›²å†…ã®å ´åˆ */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){
        /* LINãƒãƒƒãƒ•ã‚¡ ã‚¹ãƒ­ãƒƒãƒˆç„¡åŠ¹ */
        xng_lin_frm_buf[ u1a_lin_frm ].un_state.st_bit.u2g_lin_no_use = U2G_LIN_BIT_SET;
    }
    /* å¼•æ•°å€¤ãŒç¯„å›²å¤–ã®å ´åˆ ãªã«ã‚‚ã—ãªã„ */
}


/**************************************************/
/*  ãƒãƒƒãƒ•ã‚¡ ã‚¹ãƒ­ãƒƒãƒˆèª­ã¿å‡ºã—å‡¦ç†(API)            */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š u1a_lin_frm  : èª­ã¿å‡ºã™ãƒ•ãƒ¬ãƒ¼ãƒ å      */
/*         u1a_lin_cnt  : èª­ã¿å‡ºã—ãƒ‡ãƒ¼ã‚¿é•·        */
/*         pta_lin_data : ãƒ‡ãƒ¼ã‚¿æ ¼ç´å…ˆãƒã‚¤ãƒ³ã‚¿    */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_slot_rd_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, l_u8 * pta_lin_data)
{
    /* ãƒã‚¤ãƒ³ã‚¿(å¼•æ•°å€¤)ãŒNULLä»¥å¤–ã®å ´åˆ */
    if( pta_lin_data != U1G_LIN_NULL){
        /* å¼•æ•°å€¤ãŒç¯„å›²å†…ã®å ´åˆ */
        if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

            l_u1g_lin_irq_dis();                      /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

            switch( u1a_lin_cnt ) {
            case ( U1G_LIN_8 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                pta_lin_data[ U1G_LIN_2 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                pta_lin_data[ U1G_LIN_3 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ];
                pta_lin_data[ U1G_LIN_4 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ];
                pta_lin_data[ U1G_LIN_5 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ];
                pta_lin_data[ U1G_LIN_6 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ];
                pta_lin_data[ U1G_LIN_7 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ];
                break;
            case ( U1G_LIN_7 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                pta_lin_data[ U1G_LIN_2 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                pta_lin_data[ U1G_LIN_3 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ];
                pta_lin_data[ U1G_LIN_4 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ];
                pta_lin_data[ U1G_LIN_5 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ];
                pta_lin_data[ U1G_LIN_6 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ];
                break;
            case ( U1G_LIN_6 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                pta_lin_data[ U1G_LIN_2 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                pta_lin_data[ U1G_LIN_3 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ];
                pta_lin_data[ U1G_LIN_4 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ];
                pta_lin_data[ U1G_LIN_5 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ];
                break;
            case ( U1G_LIN_5 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                pta_lin_data[ U1G_LIN_2 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                pta_lin_data[ U1G_LIN_3 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ];
                pta_lin_data[ U1G_LIN_4 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ];
                break;
            case ( U1G_LIN_4 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                pta_lin_data[ U1G_LIN_2 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                pta_lin_data[ U1G_LIN_3 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ];
                break;
            case ( U1G_LIN_3 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                pta_lin_data[ U1G_LIN_2 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                break;
            case ( U1G_LIN_2 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                pta_lin_data[ U1G_LIN_1 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                break;
            case ( U1G_LIN_1 ):
                pta_lin_data[ U1G_LIN_0 ] = xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                break;
            default:
                /* æŒ‡å®šã‚µã‚¤ã‚ºç•°å¸¸ã®ç‚ºã€ä½•ã‚‚ã—ãªã„ */
                break;
            }

            l_vog_lin_irq_res();                       /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
        }
        /* å¼•æ•°ãŒç¯„å›²å¤–ã®å ´åˆ ä½•ã‚‚ã—ãªã„ */
    }
    /* ãƒã‚¤ãƒ³ã‚¿(å¼•æ•°å€¤)ãŒNULLã®å ´åˆ ä½•ã‚‚ã—ãªã„ */
}


/**************************************************/
/*  ãƒãƒƒãƒ•ã‚¡ ã‚¹ãƒ­ãƒƒãƒˆæ›¸ãè¾¼ã¿å‡¦ç†(API)            */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š u1a_lin_frm  : æ›¸ãè¾¼ã‚€ãƒ•ãƒ¬ãƒ¼ãƒ å      */
/*         u1a_lin_cnt  : æ›¸ãè¾¼ã¿ãƒ‡ãƒ¼ã‚¿é•·        */
/*         pta_lin_data : æ›¸ãè¾¼ã¿æ ¼ç´å…ƒãƒã‚¤ãƒ³ã‚¿  */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_slot_wr_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, const l_u8 * pta_lin_data)
{
    /* ãƒã‚¤ãƒ³ã‚¿(å¼•æ•°å€¤)ãŒNULLä»¥å¤–ã®å ´åˆ */
    if( pta_lin_data != U1G_LIN_NULL){
        /* å¼•æ•°å€¤ãŒç¯„å›²å†…ã®å ´åˆ */
        if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

            l_u1g_lin_irq_dis();                      /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

            switch( u1a_lin_cnt ) {
            case ( U1G_LIN_8 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] = pta_lin_data[ U1G_LIN_2 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] = pta_lin_data[ U1G_LIN_3 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] = pta_lin_data[ U1G_LIN_4 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] = pta_lin_data[ U1G_LIN_5 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ] = pta_lin_data[ U1G_LIN_6 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ] = pta_lin_data[ U1G_LIN_7 ];
                break;
            case ( U1G_LIN_7 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] = pta_lin_data[ U1G_LIN_2 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] = pta_lin_data[ U1G_LIN_3 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] = pta_lin_data[ U1G_LIN_4 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] = pta_lin_data[ U1G_LIN_5 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ] = pta_lin_data[ U1G_LIN_6 ];
                break;
            case ( U1G_LIN_6 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] = pta_lin_data[ U1G_LIN_2 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] = pta_lin_data[ U1G_LIN_3 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] = pta_lin_data[ U1G_LIN_4 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] = pta_lin_data[ U1G_LIN_5 ];
                break;
            case ( U1G_LIN_5 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] = pta_lin_data[ U1G_LIN_2 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] = pta_lin_data[ U1G_LIN_3 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] = pta_lin_data[ U1G_LIN_4 ];
                break;
            case ( U1G_LIN_4 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] = pta_lin_data[ U1G_LIN_2 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] = pta_lin_data[ U1G_LIN_3 ];
                break;
            case ( U1G_LIN_3 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] = pta_lin_data[ U1G_LIN_2 ];
                break;
            case ( U1G_LIN_2 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] = pta_lin_data[ U1G_LIN_1 ];
                break;
            case ( U1G_LIN_1 ):
                xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] = pta_lin_data[ U1G_LIN_0 ];
                break;
            default:
                /* æŒ‡å®šã‚µã‚¤ã‚ºç•°å¸¸ã®ç‚ºã€ä½•ã‚‚ã—ãªã„ */
                break;
            }

            l_vog_lin_irq_res();                       /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
        }
        /* å¼•æ•°å€¤ãŒç¯„å›²å¤–ã®å ´åˆ ä½•ã‚‚ã—ãªã„ */
    }
    /* ãƒã‚¤ãƒ³ã‚¿(å¼•æ•°å€¤)ãŒNULLã®å ´åˆ ä½•ã‚‚ã—ãªã„ */
}


/**************************************************/
/*  ãƒãƒƒãƒ•ã‚¡ã‚¹ãƒ­ãƒƒãƒˆåˆæœŸå€¤è¨­å®šå‡¦ç†(API)           */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãƒ•ãƒ¬ãƒ¼ãƒ å                             */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_slot_set_default_ch1(l_frame_handle  u1a_lin_frm)
{
    /* å¼•æ•°å€¤ãŒç¯„å›²å†…ã®å ´åˆ */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

        l_u1g_lin_irq_dis();                      /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

        /* ãƒ‡ãƒ•ã‚©ãƒ«ãƒˆå€¤è¨­å®š */
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_0 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_1 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_2 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_3 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_4 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_5 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_6 ];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ]
                                = xng_lin_slot_tbl[ u1a_lin_frm ].u1g_lin_def[ U1G_LIN_7 ];

        l_vog_lin_irq_res();                       /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
    }
    /* å¼•æ•°å€¤ãŒç¯„å›²å¤–ã®å ´åˆ ãªã«ã‚‚ã—ãªã„ */
}


/**************************************************/
/*  ãƒãƒƒãƒ•ã‚¡ã‚¹ãƒ­ãƒƒãƒˆFAILå€¤è¨­å®šå‡¦ç†(API)           */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãƒ•ãƒ¬ãƒ¼ãƒ å                             */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_slot_set_fail_ch1(l_frame_handle  u1a_lin_frm)
{
    /* å¼•æ•°å€¤ãŒç¯„å›²å†…ã®å ´åˆ */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

        l_u1g_lin_irq_dis();                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

        /* ãƒ•ã‚§ã‚¤ãƒ«å€¤è¨­å®š */
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_0];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_1];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_2];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_3];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_4];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_5];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_6];
        xng_lin_frm_buf[ u1a_lin_frm ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ]
                                = xng_lin_slot_tbl[u1a_lin_frm].u1g_lin_fail[U1G_LIN_7];

        l_vog_lin_irq_res();                        /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
    }
    /* å¼•æ•°å€¤ãŒç¯„å›²å¤–ã®å ´åˆ ãªã«ã‚‚ã—ãªã„ */
}


/**************************************************/
/*  RUNçŠ¶æ…‹ç§»è¡Œ å‡¦ç†(API)                         */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_ifc_run_ch1(void)
{
    l_u16 u2a_lin_sts;

    l_u1g_lin_irq_dis();                                /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */
    u2a_lin_sts = xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts;
    l_vog_lin_irq_res();                                /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */

    /* SLEEPçŠ¶æ…‹ã®å ´åˆ */
    if( u2a_lin_sts == U2G_LIN_STS_SLEEP )
    {
        l_u1g_lin_irq_dis();                            /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */
        /* RUN STANDBYçŠ¶æ…‹ã¸ç§»è¡Œ */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;
        l_vog_lin_irq_res();                            /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */

        l_ifc_init_drv_ch1();                           /* ãƒ‰ãƒ©ã‚¤ãƒéƒ¨ã®åˆæœŸåŒ– */
    /* SLEEPä»¥å¤–ã®å ´åˆ ãªã«ã‚‚ã—ãªã„ */
    }
    else
    {
    }
}


/**************************************************/
/*  å—ä¿¡å‰²ã‚Šè¾¼ã¿ç™ºç”Ÿ                              */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š data : å—ä¿¡ãƒ‡ãƒ¼ã‚¿                      */
/*         err  : å—ä¿¡ã‚¨ãƒ©ãƒ¼ãƒ•ãƒ©ã‚°                */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_vog_lin_rx_int(l_u8 u1a_lin_data[], l_u8 u1a_lin_err)
{
    l_u8  u1a_lin_protid;
    l_u8  l_u1a_lin_read_back;
    /*** SLEEPçŠ¶æ…‹ã®å ´åˆ ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP )
    {
        /* WAKEUPã‚³ãƒžãƒ³ãƒ‰å—ä¿¡(UARTã«ã‚ˆã‚‹WAKEUPæ¤œå‡º) */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;      /* RUN STANDBYçŠ¶æ…‹ã¸ç§»è¡Œ */
        l_ifc_init_drv_ch1();                                                       /* ãƒ‰ãƒ©ã‚¤ãƒéƒ¨ã®åˆæœŸåŒ– */
    }
    /*** RUN STANDBYçŠ¶æ…‹ã®å ´åˆ ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY )
    {
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;              /* RUNçŠ¶æ…‹ã¸ç§»è¡Œ */
        l_vog_lin_check_header(u1a_lin_data,u1a_lin_err);
    }
    /*** RUNçŠ¶æ…‹ã®å ´åˆ ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN )
    {
        /* ã‚¹ãƒ¬ãƒ¼ãƒ–ã‚¿ã‚¹ã‚¯ã®ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚’ç¢ºèª */
        switch( u1l_lin_slv_sts )
        {
        /*** Synch Break(UART)å¾…ã¡çŠ¶æ…‹ ***/
        case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
            l_vog_lin_check_header(u1a_lin_data,u1a_lin_err);
            break;

        /*** ãƒ‡ãƒ¼ã‚¿å—ä¿¡å¾…ã¡çŠ¶æ…‹ ***/
        case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            /* UARTã‚¨ãƒ©ãƒ¼ç™ºç”Ÿ */
            if( (u1a_lin_err & U1G_LIN_UART_ERR_ON) != U1G_LIN_BYTE_CLR ){
                /* SLEEPã‚³ãƒžãƒ³ãƒ‰IDã§ã€ãƒ•ãƒ¬ãƒ¼ãƒ æœªå®šç¾©ã®å ´åˆ */
                if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
                    /* ä½•ã‚‚ã›ãš Synch Breakå¾…ã¡çŠ¶æ…‹ã¸ */
                    l_vol_lin_set_synchbreak();                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                }
                /* SLEEPã‚³ãƒžãƒ³ãƒ‰IDã§ã¯ãªã„ã€ã‚‚ã—ãã¯ãƒ•ãƒ¬ãƒ¼ãƒ å®šç¾©ã‚ã‚Šã®å ´åˆ */
                else{
                    /* UARTã‚¨ãƒ©ãƒ¼ */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;

                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );          /* ã‚¨ãƒ©ãƒ¼ã‚ã‚Šãƒ¬ã‚¹ãƒãƒ³ã‚¹å®Œäº† */

                    l_vol_lin_set_synchbreak();                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                }
            }
            /* UARTã‚¨ãƒ©ãƒ¼ãªã— */
            else
            {
                /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ è¨ˆç®— */
                u1l_lin_chksum = l_vog_lin_checksum(u1g_lin_protid_tbl[xnl_lin_id_sl.u1g_lin_id], u1a_lin_data, u1l_lin_frm_sz, U1G_LIN_CHECKSUM_ENHANCED);

                /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ ã‚¨ãƒ©ãƒ¼ç™ºç”Ÿã®å ´åˆ */
                if( u1l_lin_chksum != u1a_lin_data[u1l_lin_frm_sz] )
                {
                    /* SLEEPã‚³ãƒžãƒ³ãƒ‰IDã§ã€ãƒ•ãƒ¬ãƒ¼ãƒ æœªå®šç¾©ã®å ´åˆ */
                    if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) )
                    {
                        /* ä½•ã‚‚ã›ãš Synch Breakå¾…ã¡çŠ¶æ…‹ã¸ */
                        l_vol_lin_set_synchbreak();                                    /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                    }
                    /* SLEEPã‚³ãƒžãƒ³ãƒ‰IDã§ã¯ãªã„ã€ã‚‚ã—ãã¯ãƒ•ãƒ¬ãƒ¼ãƒ å®šç¾©ã‚ã‚Šã®å ´åˆ */
                    else
                    {
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_sum = U2G_LIN_BIT_SET;

                        l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );      /* ã‚¨ãƒ©ãƒ¼ã‚ã‚Šãƒ¬ã‚¹ãƒãƒ³ã‚¹å®Œäº† */

                        l_vol_lin_set_synchbreak();                                    /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                    }
                }
                /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ ã‚¨ãƒ©ãƒ¼ãªã— */
                else{
                    /* SLEEPã‚³ãƒžãƒ³ãƒ‰ID(=3Ch)ã®å ´åˆ */
                    if( xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID ){
                        /* 1ãƒã‚¤ãƒˆç›®ã®ãƒ‡ãƒ¼ã‚¿ãŒ00hã®å ´åˆ */
                        if( u1l_lin_rs_tmp[ U1G_LIN_0 ] == U1G_LIN_SLEEP_DATA ){
                            /* SLEEPã‚³ãƒžãƒ³ãƒ‰ã¨ã—ã¦å—ä¿¡ */
                            xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_SLEEP;    /* SLEEPçŠ¶æ…‹ã«ç§»è¡Œ */
                            l_vol_lin_set_wakeup();                                             /* Wakeupå¾…æ©Ÿè¨­å®š */
                        }
                        /* 1ãƒã‚¤ãƒˆç›®ã®ãƒ‡ãƒ¼ã‚¿ãŒ00hä»¥å¤–ã®å ´åˆ */
                        else{
                            /* ãƒ•ãƒ¬ãƒ¼ãƒ å®šç¾©ã‚ã‚Šã®å ´åˆ */
                            if( xnl_lin_id_sl.u1g_lin_slot != U1G_LIN_NO_FRAME ){
                                /* é€šå¸¸ãƒ•ãƒ¬ãƒ¼ãƒ ã¨ã—ã¦å—ä¿¡ */
                                /* LINãƒãƒƒãƒ•ã‚¡ã«ãƒ‡ãƒ¼ã‚¿ã‚’æ ¼ç´(ãƒ•ãƒ¬ãƒ¼ãƒ ã‚µã‚¤ã‚º - 1) */
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_0 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_1 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_2 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_3 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_4 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_5 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_6 ];
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ] =
                                    u1l_lin_rx_buf[ U1G_LIN_7 ];

                                /* LINãƒãƒƒãƒ•ã‚¡ã«ãƒã‚§ãƒƒã‚¯ã‚µãƒ ã‚’æ ¼ç´ */
                                xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].un_state.st_bit.u2g_lin_chksum = (l_u16)(u1l_lin_chksum);

                                l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );      /* è»¢é€æˆåŠŸ */
                                l_vol_lin_set_synchbreak();                                     /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                            }
                            /* ãƒ•ãƒ¬ãƒ¼ãƒ æœªå®šç¾©ã®å ´åˆ */
                            else{
                                /* ä½•ã‚‚ã›ãšã« Synch Breakå—ä¿¡å¾…ã¡çŠ¶æ…‹ã«ã™ã‚‹ */
                                l_vol_lin_set_synchbreak();                                     /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                            }
                        }
                    }
                    /* SLEEPã‚³ãƒžãƒ³ãƒ‰ä»¥å¤–ã®ID ã®å ´åˆ */
                    else{
                        /* LINãƒãƒƒãƒ•ã‚¡ã«ãƒ‡ãƒ¼ã‚¿ã‚’æ ¼ç´(ãƒ•ãƒ¬ãƒ¼ãƒ ã‚µã‚¤ã‚º - 1) */
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] =
                            u1l_lin_rx_buf[ U1G_LIN_0 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] =
                            u1l_lin_rx_buf[ U1G_LIN_1 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] =
                            u1l_lin_rx_buf[ U1G_LIN_2 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] =
                            u1l_lin_rx_buf[ U1G_LIN_3 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] =
                            u1l_lin_rx_buf[ U1G_LIN_4 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] =
                            u1l_lin_rx_buf[ U1G_LIN_5 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ] =
                            u1l_lin_rx_buf[ U1G_LIN_6 ];
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ] =
                            u1l_lin_rx_buf[ U1G_LIN_7 ];

                        /* LINãƒãƒƒãƒ•ã‚¡ã«ãƒã‚§ãƒƒã‚¯ã‚µãƒ ã‚’æ ¼ç´ */
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_chksum = (l_u16)(u1l_lin_chksum);

                        l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );              /* è»¢é€æˆåŠŸ */

                        l_vol_lin_set_synchbreak();                                          /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                    }
                }
            }
        break;

        /*** ãƒ‡ãƒ¼ã‚¿é€ä¿¡å¾ŒçŠ¶æ…‹ ***/
        case( U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT ):
#if 1   /* ç¶™ç¶šãƒ•ãƒ¬ãƒ¼ãƒ é€ä¿¡æ™‚ã«é€ä¿¡å®Œäº†ãƒ•ãƒ©ã‚°ã§èª¤æ¤œçŸ¥ã—ã¦ã„ã‚‹å¯èƒ½æ€§ãŒã‚ã‚Šã€æœŸå¾…ã™ã‚‹ãƒ•ãƒ¬ãƒ¼ãƒ ãŒé€ä¿¡ã•ã‚ŒãŸã‹ã‚’ç¢ºèªã™ã‚‹ãŸã‚ã€LINé€šä¿¡ç®¡ç†ã¸ã®ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯ã‚’å®Ÿè¡Œã™ã‚‹ */
                f_LIN_Manager_Callback_TxComplete( xnl_lin_id_sl.u1g_lin_slot, u1l_lin_frm_sz, u1l_lin_rs_tmp );    /* é€ä¿¡å®Œäº†ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯ */
#endif
                l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp ,u1l_lin_frm_sz + U1G_LIN_1);
                /* ãƒªãƒ¼ãƒ‰ãƒãƒƒã‚¯å¤±æ•—(å—ä¿¡å‰²ã‚Šè¾¼ã¿ãŒã‹ã‹ã‚‰ãªã„å ´åˆã€ã‚‚ã—ãã¯å—ä¿¡ãƒãƒƒãƒ•ã‚¡ã®å†…å®¹ãŒå¼•æ•°ã¨ç•°ãªã‚‹) */
                if( l_u1a_lin_read_back != U1G_LIN_OK )
                {
                    /* BITã‚¨ãƒ©ãƒ¼ */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                           /* ã‚¨ãƒ©ãƒ¼ã‚ã‚Šãƒ¬ã‚¹ãƒãƒ³ã‚¹å®Œäº† */
                    l_vol_lin_set_synchbreak();                                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                }
                /* ãƒªãƒ¼ãƒ‰ãƒãƒƒã‚¯æˆåŠŸ */
                else
                {
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );                          /* è»¢é€æˆåŠŸ */
                    l_vol_lin_set_synchbreak();                                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                }
            break;
        /*** é€šå¸¸å‡¦ç†ã•ã‚Œã‚‹äº‹ã¯ãªã„ ***/
        default:
            l_vol_lin_set_synchbreak();                                                 /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
            break;
        }
    }
    /*** ä¸Šè¨˜ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ä»¥å¤–(RESET, ãã®ä»–)ã®å ´åˆ ***/
    else
    {
        /* é€šå¸¸ã¯ç™ºç”Ÿã—ãªã„å‡¦ç† */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                                           /* ã‚·ã‚¹ãƒ†ãƒ ç•°å¸¸(LINã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹) */
        l_vol_lin_set_synchbreak();                                                 /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
    }
}

/**************************************************/
/*  å¤–éƒ¨INTå‰²ã‚Šè¾¼ã¿ç™ºç”Ÿ                           */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_vog_lin_irq_int(void)
{
    /*** SLEEPçŠ¶æ…‹ã®å ´åˆ ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP )
    {
        /* RUN STANDBYçŠ¶æ…‹ã¸ç§»è¡Œ */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;
        l_ifc_init_drv_ch1();                                                   /* ãƒ‰ãƒ©ã‚¤ãƒéƒ¨ã®åˆæœŸåŒ– */
    }
    /*** RUN STANDBY,RUNçŠ¶æ…‹çŠ¶æ…‹ã®å ´åˆ ***/
    else
    {
        l_vog_lin_int_dis();                                                    /* INTå‰²ã‚Šè¾¼ã¿ã‚’ç¦æ­¢ã™ã‚‹ */
    }
}


/**************************************************/
/*  Wakeupæ¤œå‡ºå¾…ã¡ã¸ã®è¨­å®šå‡¦ç†                    */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
static void  l_vol_lin_set_wakeup(void)
{
  /* WAKEUPæ¤œå‡ºæ–¹æ³•ãŒUARTã®å ´åˆ */
  #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
    l_vog_lin_int_dis();                                /* INTå‰²ã‚Šè¾¼ã¿ã‚’ç¦æ­¢ */
    l_vog_lin_rx_enb();                                 /* å—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯ */
  /* WAKEUPæ¤œå‡ºæ–¹æ³•ãŒINTã®å ´åˆ */
  #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
    l_vog_lin_rx_dis();                                 /* å—ä¿¡å‰²ã‚Šè¾¼ã¿ç¦æ­¢ */
    l_vog_lin_int_enb_wakeup();                         /* INTå‰²ã‚Šè¾¼ã¿ã‚’è¨±å¯ */
/* Ver 2.00 å¤‰æ›´:ã‚¦ã‚§ã‚¤ã‚¯ã‚¢ãƒƒãƒ—ä¿¡å·æ¤œå‡ºã‚¨ãƒƒã‚¸ã®æ¥µæ€§åˆ‡ã‚Šæ›¿ãˆã¸ã®å¯¾å¿œ */
  #endif
}


/**************************************************/
/*  Synch Breakå—ä¿¡å¾…ã¡è¨­å®šå‡¦ç†                   */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
static void  l_vol_lin_set_synchbreak(void)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE, U1L_LIN_UART_HEADER_RXSIZE);   /* å—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯ */
    l_vog_lin_int_enb();                                                  /* INTå‰²ã‚Šè¾¼ã¿ã‚’è¨±å¯ */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT;                      /* Synch Break(UART)å¾…ã¡çŠ¶æ…‹ã«ç§»è¡Œ */
}


/**************************************************/
/*  å®Œäº†ãƒ•ãƒ©ã‚°ã®è¨­å®šå‡¦ç†                          */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š err_stat : å®Œäº†çŠ¶æ…‹                    */
/*         (0 / 1) : æ­£å¸¸å®Œäº† / ç•°å¸¸å®Œäº†          */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
static void  l_vol_lin_set_frm_complete(l_u8  u1a_lin_err)
{
    /* è»¢é€æˆåŠŸã€ã‚‚ã—ãã¯è»¢é€ã‚¨ãƒ©ãƒ¼ã®ãƒ•ãƒ©ã‚°ãŒç«‹ã£ã¦ã„ã‚‹å ´åˆ(ã‚ªãƒ¼ãƒãƒ¼ãƒ©ãƒ³) */
    if( (xng_lin_bus_sts.u2g_lin_word & U2G_LIN_BUS_STS_CMP_SET) != U2G_LIN_WORD_CLR ){
        xng_lin_bus_sts.st_bit.u2g_lin_ovr_run = U2G_LIN_BIT_SET;               /* ã‚ªãƒ¼ãƒãƒ¼ãƒ©ãƒ³ */
    }
    /* ç•°å¸¸å®Œäº†ã®å ´åˆ */
    if ( u1a_lin_err == U1G_LIN_ERR_ON ) {
        xng_lin_bus_sts.st_bit.u2g_lin_err_resp = U2G_LIN_BIT_SET;              /* ã‚¨ãƒ©ãƒ¼æœ‰ã‚Šãƒ¬ã‚¹ãƒãƒ³ã‚¹ */
    }
    /* æ­£å¸¸å®Œäº†ã®å ´åˆ */
    else {
        xng_lin_bus_sts.st_bit.u2g_lin_ok_resp = U2G_LIN_BIT_SET;               /* æ­£å¸¸å®Œäº† */
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_err.u2g_lin_err = U2G_LIN_BYTE_CLR;  /* ã‚¨ãƒ©ãƒ¼ãƒ•ãƒ©ã‚°ã®ã‚¯ãƒªã‚¢ */
/* Ver 2.00 å¤‰æ›´:ç„¡åŠ¹ã«ã—ãŸãƒ•ãƒ¬ãƒ¼ãƒ ãŒç„¡åŠ¹ã«ãªã‚‰ãªã„å ´åˆãŒã‚ã‚‹ã€‚ */
    }

    /* ä¿è­·IDã®ã‚»ãƒƒãƒˆ */
    xng_lin_bus_sts.st_bit.u2g_lin_last_id = (l_u16)u1g_lin_protid_tbl[ xnl_lin_id_sl.u1g_lin_id ];

    /* é€å—ä¿¡å‡¦ç†å®Œäº†ãƒ•ãƒ©ã‚°ã®ã‚»ãƒƒãƒˆ */
    if( xnl_lin_id_sl.u1g_lin_slot < U1G_LIN_16 ){
        xng_lin_sts_buf.un_rs_flg1.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot ];
    }
    else if( xnl_lin_id_sl.u1g_lin_slot < U1G_LIN_32 ){
        xng_lin_sts_buf.un_rs_flg2.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot - U1G_LIN_16 ];
    }
    else if( xnl_lin_id_sl.u1g_lin_slot < U1G_LIN_48 ){
        xng_lin_sts_buf.un_rs_flg3.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot - U1G_LIN_32 ];
    }
    else if( xnl_lin_id_sl.u1g_lin_slot < U1G_LIN_64 ){
        xng_lin_sts_buf.un_rs_flg4.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot - U1G_LIN_48 ];
    }
    else {
        /* é€šå¸¸ç™ºç”Ÿã—ãªã„ã®ã§å‡¦ç†ãªã— */
    }
}


/**************************************************/
/*  NMæƒ…å ±ãƒ‡ãƒ¼ã‚¿ã®ã‚»ãƒƒãƒˆ å‡¦ç†                     */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š NMæƒ…å ±ãƒ‡ãƒ¼ã‚¿                           */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_vog_lin_set_nm_info( l_u8  u1a_lin_nm_info )
{
    u1g_lin_nm_info = u1a_lin_nm_info;
}


/**************************************************/
/*  ãƒ†ãƒ¼ãƒ–ãƒ«ã®ãƒã‚§ãƒƒã‚¯ å‡¦ç†                       */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š å‡¦ç†çµæžœ                               */
/*         (0 / 1) : å‡¦ç†æˆåŠŸ / å‡¦ç†å¤±æ•—          */
/**************************************************/
l_u8  l_u1g_lin_tbl_chk(void)
{
    l_u8  u1a_lin_result;
    l_u8  u1a_lin_lp1;

    u1a_lin_result = U1G_LIN_OK;

    for( u1a_lin_lp1 = U1G_LIN_0; u1a_lin_lp1 < U1G_LIN_MAX_SLOT_NUM; u1a_lin_lp1++ )
    {
        /* IDæƒ…å ±(0x00 - ã‚¹ãƒ­ãƒƒãƒˆæœ€å¤§å€¤, FFh) */
        if( U1G_LIN_MAX_SLOT <= u1g_lin_id_tbl[ u1a_lin_lp1 ] ){
            /* ãƒ•ãƒ¬ãƒ¼ãƒ æœªå®šç¾©(=FFh)ä»¥å¤–ã®å ´åˆ */
            if( u1g_lin_id_tbl[ u1a_lin_lp1 ] != U1G_LIN_NO_FRAME )
            {
                u1a_lin_result = U1G_LIN_NG;
            }
        }
    }
    for( u1a_lin_lp1 = U1G_LIN_0; u1a_lin_lp1 < U1G_LIN_MAX_SLOT; u1a_lin_lp1++ )
    {
        /* ãƒ•ãƒ¬ãƒ¼ãƒ ã‚µã‚¤ã‚º(1 - 8) */
        if( (xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_frm_sz < U1G_LIN_DL_1)
         || (U1G_LIN_DL_8 < xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_frm_sz) )
        {
            u1a_lin_result = U1G_LIN_NG;
        }
    }

    return( u1a_lin_result );
}


/**************************************************/
/*  ãƒ˜ãƒƒãƒ€å—ä¿¡ å‡¦ç†                               */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š å—ä¿¡ãƒ‡ãƒ¼ã‚¿                             */
/*      ï¼š LINã‚¨ãƒ©ãƒ¼ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹                    */
/*  æˆ»å€¤ï¼š å‡¦ç†çµæžœ                               */
/*         (0 / 1) : å‡¦ç†æˆåŠŸ / å‡¦ç†å¤±æ•—          */
/**************************************************/
l_u8 l_vog_lin_check_header( l_u8 u1a_lin_data[U1L_LIN_UART_HEADER_RXSIZE] ,l_u8 u1a_lin_err )
{
    l_u8  u1a_lin_protid;
    l_u8  u1a_lin_result;
    l_vog_lin_int_dis(); /* æš«å®šå‡¦ç†ï¼šä»Šå¾ŒLINVer.2ã«æº–æ‹ ã•ã›ã‚‹éš›ã«å‰Šé™¤äºˆå®š */
    /* UARTã‚¨ãƒ©ãƒ¼ãŒç™ºç”Ÿã—ãŸå ´åˆ */
    if( (u1a_lin_err & U1G_LIN_UART_ERR_ON) != U1G_LIN_BYTE_CLR )
    {
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;       /* UARTã‚¨ãƒ©ãƒ¼ */
        xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;              /* Headerã‚¨ãƒ©ãƒ¼ */
        u1a_lin_result = U1G_LIN_NG;
        l_vol_lin_set_synchbreak();                                             /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
    }
    else
    {
        /* å—ä¿¡ãƒ‡ãƒ¼ã‚¿ãŒ00hä»¥å¤–ã®å ´åˆ */
        if( u1a_lin_data[U1G_LIN_HEADER_SYNCHBREAK] != U1G_LIN_SYNCH_BREAK_DATA )
        {
            /* ã‚¨ãƒ©ãƒ¼ãƒ•ãƒ©ã‚°ã®ãƒªã‚»ãƒƒãƒˆ */
                xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerã‚¨ãƒ©ãƒ¼ */
                u1a_lin_result = U1G_LIN_NG;
                l_vol_lin_set_synchbreak();                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
        }
        /* å—ä¿¡ãƒ‡ãƒ¼ã‚¿ãŒ00hãªã‚‰ã°ã€SynchBreakå—ä¿¡ */
        else
        {
            /* å—ä¿¡ãƒ‡ãƒ¼ã‚¿ãŒ55hä»¥å¤–ã®å ´åˆ */
            if( u1a_lin_data[U1G_LIN_HEADER_SYNCHFIELD] != U1G_LIN_SYNCH_FIELD_DATA )
            {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_synch = U2G_LIN_BIT_SET;  /* Synch Fieldã‚¨ãƒ©ãƒ¼ */
                xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerã‚¨ãƒ©ãƒ¼ */
                u1a_lin_result = U1G_LIN_NG;
                l_vol_lin_set_synchbreak();                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
            }
            /* æ­£å¸¸Synch Fieldå—ä¿¡ */
            else
            {
                /* PARITYã‚¨ãƒ©ãƒ¼ãŒç™ºç”Ÿã—ãŸå ´åˆ */
                u1a_lin_protid = u1g_lin_protid_tbl[ (u1a_lin_data[U1G_LIN_HEADER_PID] & U1G_LIN_ID_PARITY_MASK) ];
                if( u1a_lin_data[U1G_LIN_HEADER_PID] != u1a_lin_protid )
                {
                    xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerã‚¨ãƒ©ãƒ¼ */
                    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_pari = U2G_LIN_BIT_SET;   /* PARITYã‚¨ãƒ©ãƒ¼ */
                    u1a_lin_result = U1G_LIN_NG;
                    l_vol_lin_set_synchbreak();                                         /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                }
                /* PARITYã‚¨ãƒ©ãƒ¼ãªã— */
                else
                {
                    /* ID,ãƒ•ãƒ¬ãƒ¼ãƒ ãƒ¢ãƒ¼ãƒ‰ã‚’ç®¡ç†å¤‰æ•°ã«æ ¼ç´ */
                    xnl_lin_id_sl.u1g_lin_id = (l_u8)( u1a_lin_data[U1G_LIN_HEADER_PID] & U1G_LIN_ID_PARITY_MASK );   /* ãƒ‘ãƒªãƒ†ã‚£ã‚’çœã */
                    xnl_lin_id_sl.u1g_lin_slot = u1g_lin_id_tbl[ xnl_lin_id_sl.u1g_lin_id ];

                    /* SLEEPã‚³ãƒžãƒ³ãƒ‰IDã‚’å—ä¿¡ã—ãŸå ´åˆ(å¿…ãšãƒ¬ã‚¹ãƒãƒ³ã‚¹å—ä¿¡å‹•ä½œã¨ãªã‚‹) */
                    if( xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID ){
                        /* ãƒ•ãƒ¬ãƒ¼ãƒ ãŒ[å®šç¾©] ã‹ã¤ LINãƒãƒƒãƒ•ã‚¡ã®ã‚¹ãƒ­ãƒƒãƒˆãŒ[æœªä½¿ç”¨è¨­å®š]ã®å ´åˆ */
                        if( (xnl_lin_id_sl.u1g_lin_slot != U1G_LIN_NO_FRAME)
                         && (xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_no_use == U2G_LIN_BIT_SET) )
                        {
                            xnl_lin_id_sl.u1g_lin_slot = U1G_LIN_NO_FRAME;              /* ãƒ•ãƒ¬ãƒ¼ãƒ [æœªå®šç¾©]ã«å¤‰æ›´ */
                        }
                        u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                            /* Physical Busã‚¨ãƒ©ãƒ¼æ¤œå‡ºã‚«ã‚¦ãƒ³ã‚¿ã‚¯ãƒªã‚¢ */

                        u1l_lin_chksum = U1G_LIN_BYTE_CLR;                              /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ æ¼”ç®—ç”¨å¤‰æ•°ã®åˆæœŸåŒ– */
                        u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                              /* ãƒ‡ãƒ¼ã‚¿é€ä¿¡ã‚«ã‚¦ãƒ³ã‚¿ã®åˆæœŸåŒ– */
                        u1l_lin_frm_sz = U1G_LIN_DL_8;                                  /* SLEEPæ™‚ã®ãƒ•ãƒ¬ãƒ¼ãƒ ã‚µã‚¤ã‚ºã¯8å›ºå®š */

                        u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;                   /* ãƒ‡ãƒ¼ã‚¿å—ä¿¡å¾…ã¡çŠ¶æ…‹ */
                        l_vog_lin_rx_enb( U1G_LIN_FLUSH_RX_NO_USE,u1l_lin_frm_sz + U1G_LIN_1 );
                    }
                    /* SLEEPã‚³ãƒžãƒ³ãƒ‰ä»¥å¤–ã®å ´åˆ */
                    else
                    {
                        /* ãƒ•ãƒ¬ãƒ¼ãƒ ãŒ[æœªå®šç¾©] ã‚‚ã—ãã¯ LINãƒãƒƒãƒ•ã‚¡ã®ã‚¹ãƒ­ãƒƒãƒˆãŒ[æœªä½¿ç”¨è¨­å®š]ã®å ´åˆ */
                        if( (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME)
                         || (xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_no_use == U2G_LIN_BIT_SET) )
                        {
                            l_vol_lin_set_synchbreak();                                 /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                        }
                        /* å‡¦ç†å¯¾è±¡ãƒ•ãƒ¬ãƒ¼ãƒ ã®å ´åˆ */
                        else
                        {
                            u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                        /* Physical Busã‚¨ãƒ©ãƒ¼æ¤œå‡ºã‚«ã‚¦ãƒ³ã‚¿ã‚¯ãƒªã‚¢ */

                            /* ç¾åœ¨ã®ãƒ•ãƒ¬ãƒ¼ãƒ ãƒ‡ãƒ¼ã‚¿é•· */
                            u1l_lin_frm_sz = xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_frm_sz;

                            /*-- [å—ä¿¡ãƒ•ãƒ¬ãƒ¼ãƒ æ™‚] --*/
                            if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_RCV )
                            {
                                u1l_lin_chksum = U1G_LIN_BYTE_CLR;                      /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ æ¼”ç®—ç”¨å¤‰æ•°ã®åˆæœŸåŒ– */
                                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                      /* ãƒ‡ãƒ¼ã‚¿å—ä¿¡ã‚«ã‚¦ãƒ³ã‚¿ã®åˆæœŸåŒ– */
                                u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;           /* ãƒ‡ãƒ¼ã‚¿å—ä¿¡å¾…ã¡çŠ¶æ…‹ */
                                u1a_lin_result = U1G_LIN_OK;
                                l_vog_lin_rx_enb( U1G_LIN_FLUSH_RX_NO_USE,u1l_lin_frm_sz + U1G_LIN_1 );
                            }
                            /*-- [é€ä¿¡ãƒ•ãƒ¬ãƒ¼ãƒ æ™‚] --*/
                            else if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND )
                            {
                                u1l_lin_chksum = U1G_LIN_BYTE_CLR;                      /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ æ¼”ç®—ç”¨å¤‰æ•°ã®åˆæœŸåŒ– */
                                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                      /* ãƒ‡ãƒ¼ã‚¿é€ä¿¡ã‚«ã‚¦ãƒ³ã‚¿ã®åˆæœŸåŒ– */

                                l_vog_lin_rx_dis();                                     /* é€ä¿¡æ™‚ã¯å—ä¿¡å‰²ã‚Šè¾¼ã¿ç¦æ­¢ */

                                /* NMä½¿ç”¨è¨­å®šãƒ•ãƒ¬ãƒ¼ãƒ ã®å ´åˆ */
                                if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_nm_use == U1G_LIN_NM_USE )
                                {
                                    /* LINãƒ•ãƒ¬ãƒ¼ãƒ ãƒãƒƒãƒ•ã‚¡ã®NMéƒ¨åˆ†(ãƒ‡ãƒ¼ã‚¿1ã®bit4-7)ã‚’ã‚¯ãƒªã‚¢ */
                                    /* NMéƒ¨åˆ†ã®ã‚¯ãƒªã‚¢ */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                     &= U1G_LIN_BUF_NM_CLR_MASK;
                                    /* LINãƒ•ãƒ¬ãƒ¼ãƒ ã« ãƒ¬ã‚¹ãƒãƒ³ã‚¹é€ä¿¡ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚’ã‚»ãƒƒãƒˆ */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                     |= ( u1g_lin_nm_info & U1G_LIN_NM_INFO_MASK );
                                }

                                /* LINãƒãƒƒãƒ•ã‚¡ã®ãƒ‡ãƒ¼ã‚¿ã‚’ é€ä¿¡ç”¨tmpãƒãƒƒãƒ•ã‚¡ã«ã‚³ãƒ”ãƒ¼ */
                                u1l_lin_rs_tmp[ U1G_LIN_0 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ];
                                u1l_lin_rs_tmp[ U1G_LIN_1 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ];
                                u1l_lin_rs_tmp[ U1G_LIN_2 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ];
                                u1l_lin_rs_tmp[ U1G_LIN_3 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ];
                                u1l_lin_rs_tmp[ U1G_LIN_4 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ];
                                u1l_lin_rs_tmp[ U1G_LIN_5 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ];
                                u1l_lin_rs_tmp[ U1G_LIN_6 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ];
                                u1l_lin_rs_tmp[ U1G_LIN_7 ] =
                                 xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ];

                                /* ãƒã‚§ãƒƒã‚¯ã‚µãƒ ã®é€ä¿¡ */
                                /* é€ä¿¡ç”¨tmpãƒãƒƒãƒ•ã‚¡ã«ã‚³ãƒ”ãƒ¼ */
                                u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = l_vog_lin_checksum(u1g_lin_protid_tbl[xnl_lin_id_sl.u1g_lin_id],u1l_lin_rs_tmp, u1l_lin_frm_sz, U1G_LIN_CHECKSUM_ENHANCED);
                                /* LINãƒãƒƒãƒ•ã‚¡ã«ã‚‚ã‚³ãƒ”ãƒ¼ */
                                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_chksum
                                    = (l_u16)u1l_lin_rs_tmp[ u1l_lin_frm_sz ];

                                l_vog_lin_tx_char( u1l_lin_rs_tmp, u1l_lin_frm_sz + U1G_LIN_1 );              /* ãƒ‡ãƒ¼ã‚¿é€ä¿¡ */

                                u1l_lin_slv_sts = U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT;
                                u1a_lin_result = U1G_LIN_OK;
                            }
                            /*-- [é€ä¿¡ã§ã‚‚å—ä¿¡ã§ã‚‚ãªã„] --*/
                            else
                            {
                                /* ç™»éŒ²ã‚¨ãƒ©ãƒ¼ */
                                l_vol_lin_set_synchbreak();                             /* Synch Breakå—ä¿¡å¾…ã¡è¨­å®š */
                                u1a_lin_result = U1G_LIN_NG;
                            }
                        }
                    }
                }
            }
        }
    }
    return( u1a_lin_result );
}
/***** End of File *****/
