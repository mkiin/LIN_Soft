/**
 * @file        l_slin_drv_cc2340r53.c
 *
 * @brief       SLIN DRVå±¤
 *
 * @attention   ç·¨é›†ç¦æ­¢
 *
 */
#pragma	section	lin

/*=== MCUä¾å­˜éƒ¨åˆ† ===*/
/***** ãƒ˜ãƒƒãƒ€ ã‚¤ãƒ³ã‚¯ãƒ«ãƒ¼ãƒ‰ *****/
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_stddef.h"
#include "l_slin_sfr_cc2340r53.h"
#include "l_slin_drv_cc2340r53.h"
/* Driver Header files */
#include <ti/drivers/UART2.h>
#include <ti/drivers/uart2/UART2LPF3.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/timer/LGPTimerLPF3.h>
/* Driver configuration */
#include "ti_drivers_config.h"
#include <ti\devices\cc23x0r5\inc\hw_uart.h>
/********************************************************************************/
/* éžå…¬é–‹ãƒžã‚¯ãƒ­å®šç¾©                                                             */
/********************************************************************************/
#if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_2400
#define U4L_LIN_BAUDRATE ( 2400  )  /**< @brief LGPTimerã§1msè¨ˆæ¸¬ã™ã‚‹éš›ã«å…¥åŠ›ã™ã‚‹å€¤   åž‹: l_u32 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
#define U4L_LIN_BAUDRATE ( 9600  )  /**< @brief LGPTimerã§1msè¨ˆæ¸¬ã™ã‚‹éš›ã«å…¥åŠ›ã™ã‚‹å€¤   åž‹: l_u32 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
#define U4L_LIN_BAUDRATE ( 19200  )  /**< @brief LGPTimerã§1msè¨ˆæ¸¬ã™ã‚‹éš›ã«å…¥åŠ›ã™ã‚‹å€¤   åž‹: l_u32 */
#else
#error "config failure[ U1G_LIN_BAUDRATE ]"
#endif
#define U4L_LIN_1MS_TIMERVAL ( 48000  )  /**< @brief LGPTimerã§1msè¨ˆæ¸¬ã™ã‚‹éš›ã«å…¥åŠ›ã™ã‚‹å€¤   åž‹: l_u32 */
#define U4L_LIN_1BIT_TIMERVAL ( 1000 * U4L_LIN_1MS_TIMERVAL / U4L_LIN_BAUDRATE  )  /**< @brief 1bité€ã‚‹ã®ã«å¿…è¦ãªæ™‚é–“ã‚’LGPTimerã‚’ç”¨ã„ã¦è¨ˆæ¸¬ã™ã‚‹éš›ã«å…¥åŠ›ã™ã‚‹å€¤   åž‹: l_u32 */

/********************************************************************************/
/* éžå…¬é–‹ãƒžã‚¯ãƒ­é–¢æ•°å®šç¾©                                                         */
/********************************************************************************/
// Word (32 bit) access to address x
// Read example  : my32BitVar = HWREG(base_addr + offset) ;
// Write example : HWREG(base_addr + offset) = my32BitVar ;
#define U4L_LIN_HWREG(x)                                                        \
        (*((volatile unsigned long *)(x)))

/***** é–¢æ•°ãƒ—ãƒ­ãƒˆã‚¿ã‚¤ãƒ—å®£è¨€ *****/
/*-- APIé–¢æ•°(extern) --*/
void  l_ifc_tm_ch1(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask);
void  l_ifc_aux_ch1(uint_least8_t index);

/*-- ãã®ä»– MCUä¾å­˜é–¢æ•°(extern) --*/
void   l_vog_lin_uart_init(void);
void   l_vog_lin_int_init(void);
void   l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size);
void   l_vog_lin_rx_dis(void);
void   l_vog_lin_int_enb(void);
/* Ver 2.00 è¿½åŠ :ã‚¦ã‚§ã‚¤ã‚¯ã‚¢ãƒƒãƒ—ä¿¡å·æ¤œå‡ºã‚¨ãƒƒã‚¸ã®æ¥µæ€§åˆ‡ã‚Šæ›¿ãˆã¸ã®å¯¾å¿œ */
void  l_vog_lin_int_enb_wakeup(void);
/* Ver 2.00 è¿½åŠ :ã‚¦ã‚§ã‚¤ã‚¯ã‚¢ãƒƒãƒ—ä¿¡å·æ¤œå‡ºã‚¨ãƒƒã‚¸ã®æ¥µæ€§åˆ‡ã‚Šæ›¿ãˆã¸ã®å¯¾å¿œ */
void   l_vog_lin_int_dis(void);
void   l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size);
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size);

/*** å¤‰æ•°(static) ***/
static l_u16            u2l_lin_tm_bit;             /* 1bitã‚¿ã‚¤ãƒ å€¤ */
static l_u16            u2l_lin_tm_maxbit;          /* 0xFFFFã‚«ã‚¦ãƒ³ãƒˆåˆ†ã®ãƒ“ãƒƒãƒˆé•· */
static LGPTimerLPF3_Handle  xnl_lin_timer_handle;       /**< @brief LGPTimerãƒãƒ³ãƒ‰ãƒ« */
static UART2_Handle         xnl_lin_uart_handle;        /**< @brief UART2ãƒãƒ³ãƒ‰ãƒ« */
l_u8             u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];       /**< @brief å—ä¿¡ãƒãƒƒãƒ•ã‚¡ */
l_u8             u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE];      /**< @brief é€ä¿¡ãƒãƒƒãƒ•ã‚¡ */
/**************************************************/

/********************************/
/* MCUä¾å­˜ã®APIé–¢æ•°å‡¦ç†         */
/********************************/
/**
 * @brief   UARTãƒ¬ã‚¸ã‚¹ã‚¿ã®åˆæœŸåŒ– å‡¦ç†
 *
 *          UARTãƒ¬ã‚¸ã‚¹ã‚¿ã®åˆæœŸåŒ–
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š
 *          - UARTãƒ‘ãƒ©ãƒ¡ãƒ¼ã‚¿åˆæœŸåŒ–
 *          - UARTãƒãƒ³ãƒ‰ãƒ«ç”Ÿæˆ
 *          - UARTãƒãƒ³ãƒ‰ãƒ«ç”ŸæˆæˆåŠŸã—ãªã‹ã£ãŸå ´åˆ
 *              - ã‚·ã‚¹ãƒ†ãƒ ç•°å¸¸ãƒ•ãƒ©ã‚°ã«MCUã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ç•°å¸¸è¨­å®š
 *          - å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_lin_uart_init(void)
{
    UART2_Params xna_lin_uart_params;                           /**< @brief  UARTãƒ‘ãƒ©ãƒ¡ãƒ¼ã‚¿ åž‹: UART2_Params */

    if( NULL == xnl_lin_uart_handle )
    {
        l_u1g_lin_irq_dis();                                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

        UART2_Params_init( &xna_lin_uart_params );                  /* UARTæ§‹æˆæƒ…å ±åˆæœŸåŒ– */
        /* UARTæ§‹æˆæƒ…å ±ã«å€¤ã‚’è¨­å®š */
        xna_lin_uart_params.readMode        = UART2_Mode_CALLBACK;
        xna_lin_uart_params.writeMode       = UART2_Mode_CALLBACK;
        xna_lin_uart_params.readCallback    = l_ifc_rx_ch1;
        xna_lin_uart_params.writeCallback   = l_ifc_tx_ch1;
        xna_lin_uart_params.readReturnMode  = UART2_ReadReturnMode_FULL;
        xna_lin_uart_params.baudRate        = U4L_LIN_BAUDRATE;
        xna_lin_uart_params.dataLength      = UART2_DataLen_8;
        xna_lin_uart_params.stopBits        = UART2_StopBits_1;
        xna_lin_uart_params.parityType      = UART2_Parity_NONE;
        xna_lin_uart_params.eventMask       = 0xFFFFFFFF;
        xnl_lin_uart_handle = UART2_open( CONFIG_UART_INDEX,&xna_lin_uart_params );   /* UARTãƒãƒ³ãƒ‰ãƒ«ç”Ÿæˆ */
        if( NULL == xnl_lin_uart_handle )
        {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                 /* MCUã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ç•°å¸¸ */
        }

        l_vog_lin_irq_res();                                        /* å‰²ã‚Šè¾¼ã¿è¨­å®šã‚’å¾©å…ƒ */
    }

}

/**
 * @brief   å¤–éƒ¨INTãƒ¬ã‚¸ã‚¹ã‚¿ã®åˆæœŸåŒ– å‡¦ç†
 *
 *          å¤–éƒ¨INTãƒ¬ã‚¸ã‚¹ã‚¿ã®åˆæœŸåŒ–
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š
 *          - GPIOãƒ¢ã‚¸ãƒ¥ãƒ¼ãƒ«åˆæœŸåŒ–
 *          - GPIOãƒ”ãƒ³æ§‹æˆã‚’è¨­å®š
 *          - GPIOãƒ”ãƒ³æ§‹æˆè¨­å®šã«æˆåŠŸã—ãªã‹ã£ãŸå ´åˆ
 *              - ã‚·ã‚¹ãƒ†ãƒ ç•°å¸¸ãƒ•ãƒ©ã‚°ã«MCUã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ç•°å¸¸è¨­å®š
 *          - GPIOãƒ”ãƒ³ã«ã‚ˆã‚‹ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯é–¢æ•°ç™»éŒ²
 *          - å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_int_init(void)
{
    l_s16    s2a_lin_ret;
    /*** INTã®åˆæœŸåŒ– ***/
    l_u1g_lin_irq_dis();                            /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    GPIO_init();                                    /* GPIOãƒ¢ã‚¸ãƒ¥ãƒ¼ãƒ«åˆæœŸåŒ– */
    s2a_lin_ret = GPIO_setConfig( U1G_LIN_INTPIN, GPIO_CFG_INPUT_INTERNAL | GPIO_CFG_IN_INT_FALLING ); /* GPIOãƒ”ãƒ³æ§‹æˆã‚’è¨­å®š */
    if( GPIO_STATUS_SUCCESS != s2a_lin_ret )
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;     /* MCUã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ç•°å¸¸ */
    }
    GPIO_setCallback( U1G_LIN_INTPIN, l_ifc_aux_ch1 ); /* GPIOãƒ”ãƒ³ã«ã‚ˆã‚‹ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯é–¢æ•°ç™»éŒ² */

    l_vog_lin_irq_res();                            /* å‰²ã‚Šè¾¼ã¿è¨­å®šã‚’å¾©å…ƒ */
}
/**
 * @brief   ãƒ‡ãƒ¼ã‚¿ã®å—ä¿¡å‡¦ç†(API)
 *
 *          ãƒ‡ãƒ¼ã‚¿ã®å—ä¿¡å‡¦ç†(API)
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          -

 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data, size_t count, void *userArg, int_fast16_t u1a_lin_rx_status)
{
    l_u8    u1a_lin_rx_set_err;

    /* å—ä¿¡ã‚¨ãƒ©ãƒ¼æƒ…å ±ã®ç”Ÿæˆ */
    u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;//ç¾åœ¨ã‚¨ãƒ©ãƒ¼æ¤œå‡ºæœªå®Ÿè£…
    l_vog_lin_rx_int( (l_u8 *)u1a_lin_rx_data, u1a_lin_rx_set_err );     /* ã‚¹ãƒ¬ãƒ¼ãƒ–ã‚¿ã‚¹ã‚¯ã«å—ä¿¡å ±å‘Š */

}
/**
 * @brief   ãƒ‡ãƒ¼ã‚¿ã®é€ä¿¡å®Œäº†å‰²ã‚Šè¾¼ã¿(API)
 *
 *          ãƒ‡ãƒ¼ã‚¿ã®é€ä¿¡å®Œäº†å‰²ã‚Šè¾¼ã¿
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          -

 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_ifc_tx_ch1(UART2_Handle handle, void *u1a_lin_tx_data, size_t count, void *userArg, int_fast16_t u1a_lin_tx_status)
{
}

/**************************************************/
/*  å¤–éƒ¨INTå‰²ã‚Šè¾¼ã¿åˆ¶å¾¡å‡¦ç†(API)                  */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
/*  å¤–éƒ¨INTå‰²ã‚Šè¾¼ã¿ã®éš›ã«å‘¼ã³å‡ºã•ã‚Œã‚‹API          */
/**************************************************/
void  l_ifc_aux_ch1(uint_least8_t index)
{
    l_vog_lin_irq_int();                        /* å¤–éƒ¨INTå‰²ã‚Šè¾¼ã¿å ±å‘Š */
}

/***********************************/
/* MCUå›ºæœ‰ã®SFRè¨­å®šç”¨é–¢æ•°å‡¦ç†      */
/***********************************/
/**
 * @brief   UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯ å‡¦ç†
 *
 *          UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯
 *
 * @param ã€€å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚µã‚¤ã‚º
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š
 *          - UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯
 *          - UARTå—ä¿¡é–‹å§‹
 *          - UARTå—ä¿¡é–‹å§‹æˆåŠŸã—ãªã‹ã£ãŸå ´åˆ
 *              - ã‚·ã‚¹ãƒ†ãƒ ç•°å¸¸ãƒ•ãƒ©ã‚°ã«MCUã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ç•°å¸¸è¨­å®š
 *          - å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size)
{
    l_s16    s2a_lin_ret;
    l_u1g_lin_irq_dis();                                /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */
    /* ãƒãƒƒãƒ•ã‚¡ãƒ‡ãƒ¼ã‚¿ã‚’å‰Šé™¤ã™ã‚‹å‡¦ç† */
    if(u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE)
    {
        UART2_flushRx(xnl_lin_uart_handle);
    }

    UART2_rxEnable( xnl_lin_uart_handle );              /* UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯ */
    s2a_lin_ret = UART2_read( xnl_lin_uart_handle,      /* UARTå—ä¿¡é–‹å§‹ */
                              &u1l_lin_rx_buf,
                              u1a_lin_rx_data_size,
                              NULL );
    if( UART2_STATUS_SUCCESS != s2a_lin_ret )
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;         /* MCUã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ç•°å¸¸ */
    }
    l_vog_lin_irq_res();                                /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
}


/**
 * @brief   UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿ç¦æ­¢ å‡¦ç†
 *
 *          UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿ç¦æ­¢
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          å—ä¿¡ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯ãŒå¸°ã£ã¦ããŸæ™‚ç‚¹ã§UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿ãŒåœæ­¢ã™ã‚‹ãŸã‚ã€ç¦æ­¢å‡¦ç†ã¯ä¸è¦ã€‚è¨˜éŒ²ã®ãŸã‚é–¢æ•°ã®ã¿æ®‹ã™ã€‚
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_rx_dis(void)
{
    /* å—ä¿¡ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯ãŒå¸°ã£ã¦ããŸæ™‚ç‚¹ã§UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿ãŒåœæ­¢ã™ã‚‹ãŸã‚ã€ç¦æ­¢å‡¦ç†ã¯ä¸è¦ã€‚ */
}


/**************************************************/
/*  INTå‰²ã‚Šè¾¼ã¿è¨±å¯ å‡¦ç†                          */
/*------------------------------------------------*/
/*  å¼•æ•°ï¼š ãªã—                                   */
/*  æˆ»å€¤ï¼š ãªã—                                   */
/**************************************************/
void  l_vog_lin_int_enb(void)
{
    l_u1g_lin_irq_dis();                                                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    GPIO_setInterruptConfig( U1G_LIN_INTPIN, GPIO_CFG_IN_INT_RISING);   /* ç«‹ã¡ä¸ŠãŒã‚Šã‚¨ãƒƒã‚¸ã‚’æ¤œçŸ¥ */
    GPIO_enableInt( U1G_LIN_INTPIN );                                     /* INTå‰²ã‚Šè¾¼ã¿è¨±å¯ */

    l_vog_lin_irq_res();                                                         /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
}


/**
 * @brief   INTå‰²ã‚Šè¾¼ã¿è¨±å¯(Wakeupãƒ‘ãƒ«ã‚¹æ¤œå‡ºç”¨) å‡¦ç†
 *
 *          INTå‰²ã‚Šè¾¼ã¿è¨±å¯(Wakeupãƒ‘ãƒ«ã‚¹æ¤œå‡ºç”¨)
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š
 *          - UARTå—ä¿¡å‰²ã‚Šè¾¼ã¿è¨±å¯
 *          - å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_int_enb_wakeup(void)
{
    l_u1g_lin_irq_dis();                                                       /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    GPIO_setInterruptConfig( U1G_LIN_INTPIN, GPIO_CFG_IN_INT_FALLING); /* ç«‹ã¡ä¸‹ãŒã‚Šã‚¨ãƒƒã‚¸ã‚’æ¤œçŸ¥ */
    GPIO_enableInt( U1G_LIN_INTPIN );                                    /* INTå‰²ã‚Šè¾¼ã¿è¨±å¯ */

    l_vog_lin_irq_res();                                                        /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
}


/**
 * @brief   INTå‰²ã‚Šè¾¼ã¿ç¦æ­¢ å‡¦ç†
 *
 *          INTå‰²ã‚Šè¾¼ã¿ç¦æ­¢
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š
 *          - INTå‰²ã‚Šè¾¼ã¿ç¦æ­¢
 *          - å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_int_dis(void)
{
    l_u1g_lin_irq_dis();                                /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢è¨­å®š */

    GPIO_disableInt( U1G_LIN_INTPIN );            /* INTå‰²ã‚Šè¾¼ã¿ç¦æ­¢ */

    l_vog_lin_irq_res();                                 /* å‰²ã‚Šè¾¼ã¿è¨­å®šå¾©å…ƒ */
}

/**
 * @brief   é€ä¿¡ãƒ¬ã‚¸ã‚¹ã‚¿ã«ãƒ‡ãƒ¼ã‚¿ã‚’ã‚»ãƒƒãƒˆã™ã‚‹ å‡¦ç†
 *
 *          é€ä¿¡ãƒ¬ã‚¸ã‚¹ã‚¿ã«ãƒ‡ãƒ¼ã‚¿ã‚’ã‚»ãƒƒãƒˆã™ã‚‹
 *
 * @param   é€ä¿¡ãƒ‡ãƒ¼ã‚¿
 * @return  ãªã—
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - é€ä¿¡ãƒãƒƒãƒ•ã‚¡ãƒ¬ã‚¸ã‚¹ã‚¿ã« ãƒ‡ãƒ¼ã‚¿ã‚’æ ¼ç´
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE,u1a_lin_data_size);
    /* é€ä¿¡ãƒãƒƒãƒ•ã‚¡ãƒ¬ã‚¸ã‚¹ã‚¿ã« ãƒ‡ãƒ¼ã‚¿ã‚’æ ¼ç´ */
    UART2_write(xnl_lin_uart_handle,u1a_lin_data,u1a_lin_data_size,NULL);
}

/**
 * @brief   ãƒªãƒ¼ãƒ‰ãƒãƒƒã‚¯ å‡¦ç†
 *
 *          ãƒªãƒ¼ãƒ‰ãƒãƒƒã‚¯ å‡¦ç†
 *
 * @param ãƒªãƒ¼ãƒ‰ãƒãƒƒã‚¯æ¯”è¼ƒç”¨ãƒ‡ãƒ¼ã‚¿
 * @return (0 / 1) : èª­è¾¼ã¿æˆåŠŸ / èª­è¾¼ã¿å¤±æ•—
 *
 * @cond DETAIL
 * @par     å‡¦ç†å†…å®¹
 *          - ã‚¨ãƒ©ãƒ¼ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚’ãƒ¬ã‚¸ã‚¹ã‚¿ã‹ã‚‰å–å¾—
 *          - å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚’ãƒ¬ã‚¸ã‚¹ã‚¿ã‹ã‚‰å–å¾—
 *          - å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã®æœ‰ç„¡ã‚’ãƒã‚§ãƒƒã‚¯
 *              - ã‚¨ãƒ©ãƒ¼ãŒç™ºç”Ÿã—ã¦ã„ã‚‹å ´åˆ
 *                  - æˆ»ã‚Šå€¤=èª­è¾¼ã¿å¤±æ•—
 *              - ã‚¨ãƒ©ãƒ¼ãŒç™ºç”Ÿã—ã¦ã„ãªã„å ´åˆ
 *                  - å—ä¿¡ãƒãƒƒãƒ•ã‚¡ã®å†…å®¹ã¨å¼•æ•°ã‚’æ¯”è¼ƒã—é•ã†å ´åˆ
 *                      - æˆ»ã‚Šå€¤=èª­è¾¼ã¿å¤±æ•—
 *                  - å—ä¿¡ãƒãƒƒãƒ•ã‚¡ã®å†…å®¹ã¨å¼•æ•°ã‚’æ¯”è¼ƒã—æ­£ã—ã„å ´åˆ
 *                      - æˆ»ã‚Šå€¤=èª­è¾¼ã¿æˆåŠŸ
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    l_u8    u1a_lin_index;
    /* ã‚¨ãƒ©ãƒ¼ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ã‚’ãƒ¬ã‚¸ã‚¹ã‚¿ã‹ã‚‰å–å¾— */
    u4a_lin_error_status  = U4L_LIN_HWREG(((( UART2_HWAttrs const * )( xnl_lin_uart_handle->hwAttrs ))->baseAddr +  UART_O_RSR_ECR ));
    u4a_lin_error_status  = U4G_DAT_ZERO; /*ä¸€æ—¦ã‚¨ãƒ©ãƒ¼æ¤œå‡ºæ©Ÿèƒ½ã‚ªãƒ•*/

    /* ã‚¨ãƒ©ãƒ¼ãŒç™ºç”Ÿã—ã¦ã„ã‚‹ã‹ã‚’ãƒã‚§ãƒƒã‚¯ */
    if(    ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN ) == U4G_LIN_UART_ERR_OVERRUN )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY  ) == U4G_LIN_UART_ERR_PARITY  )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING ) == U4G_LIN_UART_ERR_FRAMING ) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        u1a_lin_result = U1G_LIN_OK;
        /* å—ä¿¡ãƒãƒƒãƒ•ã‚¡ã®å†…å®¹ã¨å¼•æ•°ã‚’æ¯”è¼ƒ */
        for (u1a_lin_index = U1G_DAT_ZERO; u1a_lin_index < u1a_lin_data_size; u1a_lin_index++)
        {
            if(u1l_lin_rx_buf[u1a_lin_index] != u1a_lin_data[u1a_lin_index])
            {
                u1a_lin_result = U1G_LIN_NG;
            }
        }
    }

    return( u1a_lin_result );
}


/**
 * @brief LINãƒã‚§ãƒƒã‚¯ã‚µãƒ è¨ˆç®—
 * @param u1a_lin_pid           pidãƒ‡ãƒ¼ã‚¿
 * @param u1a_lin_data          ãƒã‚§ãƒƒã‚¯ã‚µãƒ å¯¾è±¡ã®ãƒ‡ãƒ¼ã‚¿
 * @param u1a_lin_data_length   ãƒ‡ãƒ¼ã‚¿ã®é•·ã•ï¼ˆãƒã‚¤ãƒˆæ•°ï¼‰
 * @param type                  ãƒã‚§ãƒƒã‚¯ã‚µãƒ ã®ç¨®é¡žï¼ˆã‚¯ãƒ©ã‚·ãƒƒã‚¯ or æ‹¡å¼µï¼‰
 * @return                      ãƒã‚§ãƒƒã‚¯ã‚µãƒ å€¤
 */
l_u8 l_vog_lin_checksum(l_u8 u1a_lin_pid ,const l_u8* u1a_lin_data, l_u8 u1a_lin_data_length, U1G_LIN_ChecksumType type)
{
    l_u16 u2a_lin_sum = U1G_DAT_ZERO;
    l_u8  u1a_lin_data_index;
    if(type == U1G_LIN_CHECKSUM_ENHANCED)
    {
        u2a_lin_sum += u1a_lin_pid;
    }
    for (u1a_lin_data_index = U1G_DAT_ZERO; u1a_lin_data_index < u1a_lin_data_length ; u1a_lin_data_index++)
    {
        u2a_lin_sum += u1a_lin_data[u1a_lin_data_index];
        if (u2a_lin_sum > U2G_LIN_MASK_FF)
        {
            u2a_lin_sum = (u2a_lin_sum & U2G_LIN_MASK_FF) + U1G_LIN_CHECKSUM_CARRY_ADD;
        }
    }

    return (l_u8)(~u2a_lin_sum);
}

void l_ifc_uart_close(void)
{
    UART2_close(xnl_lin_uart_handle);
    xnl_lin_uart_handle = NULL;
}
/***** End of File *****/
