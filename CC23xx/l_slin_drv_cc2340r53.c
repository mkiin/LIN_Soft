/**
 * @file        l_slin_drv_cc2340r53.c
 *
 * @brief       SLIN DRV層
 *
 * @attention   編集禁止
 *
 */
#pragma	section	lin

/*=== MCU依存部分 ===*/
/***** ヘッダ インクルード *****/
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
#include "UARTDriver/UART_RegInt.h"
#include "UARTDriver/uart/UART_RegIntLPF3.h"
/* Driver configuration */
#include "ti_drivers_config.h"
#include <ti\devices\cc23x0r5\inc\hw_uart.h>
/********************************************************************************/
/* 非公開マクロ定義                                                             */
/********************************************************************************/
#if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_2400
#define U4L_LIN_BAUDRATE ( 2400  )  /**< @brief LGPTimerで1ms計測する際に入力する値   型: l_u32 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
#define U4L_LIN_BAUDRATE ( 9600  )  /**< @brief LGPTimerで1ms計測する際に入力する値   型: l_u32 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
#define U4L_LIN_BAUDRATE ( 19200  )  /**< @brief LGPTimerで1ms計測する際に入力する値   型: l_u32 */
#else
#error "config failure[ U1G_LIN_BAUDRATE ]"
#endif
#define U4L_LIN_1MS_TIMERVAL ( 48000  )  /**< @brief LGPTimerで1ms計測する際に入力する値   型: l_u32 */
#define U4L_LIN_1BIT_TIMERVAL ( 1000 * U4L_LIN_1MS_TIMERVAL / U4L_LIN_BAUDRATE  )  /**< @brief 1bit送るのに必要な時間をLGPTimerを用いて計測する際に入力する値   型: l_u32 */

/********************************************************************************/
/* 非公開マクロ関数定義                                                         */
/********************************************************************************/
// Word (32 bit) access to address x
// Read example  : my32BitVar = HWREG(base_addr + offset) ;
// Write example : HWREG(base_addr + offset) = my32BitVar ;
#define U4L_LIN_HWREG(x)                                                        \
        (*((volatile unsigned long *)(x)))

/***** 関数プロトタイプ宣言 *****/
/*-- API関数(extern) --*/
void  l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data, size_t count, void *userArg, int_fast16_t u1a_lin_rx_status);
void  l_ifc_tx_ch1(UART2_Handle handle, void *u1a_lin_tx_data, size_t count, void *userArg, int_fast16_t u1a_lin_tx_status);
void  l_ifc_tm_ch1(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask);
void  l_ifc_aux_ch1(uint_least8_t index);

/*-- その他 MCU依存関数(extern) --*/
void   l_vog_lin_uart_init(void);
void   l_vog_lin_int_init(void);
void   l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size);
void   l_vog_lin_rx_dis(void);
void   l_vog_lin_int_enb(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
void  l_vog_lin_int_enb_wakeup(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
void   l_vog_lin_int_dis(void);
void   l_vog_lin_tx_char(l_u8 u1a_lin_data);
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data);

/*** 変数(static) ***/
static l_u16            u2l_lin_tm_bit;             /* 1bitタイム値 */
static l_u16            u2l_lin_tm_maxbit;          /* 0xFFFFカウント分のビット長 */
static LGPTimerLPF3_Handle  xnl_lin_timer_handle;       /**< @brief LGPTimerハンドル */
static UART2_Handle         xnl_lin_uart_handle;        /**< @brief UART2ハンドル */
l_u8             u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];       /**< @brief 受信バッファ */
l_u8             u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE];      /**< @brief 送信バッファ */
/**************************************************/
#define ORIGINAL_UART_DRIVER    (0)
/********************************/
/* MCU依存のAPI関数処理         */
/********************************/
/**
 * @brief   UARTレジスタの初期化 処理
 *
 *          UARTレジスタの初期化
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 割り込み禁止設定
 *          - UARTパラメータ初期化
 *          - UARTハンドル生成
 *          - UARTハンドル生成成功しなかった場合
 *              - システム異常フラグにMCUステータス異常設定
 *          - 割り込み設定復元
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void l_vog_lin_uart_init(void)
{
    UART2_Params xna_lin_uart_params;                           /**< @brief  UARTパラメータ 型: UART2_Params */
    
    if( NULL == xnl_lin_uart_handle )
    {
        l_u1g_lin_irq_dis();                                        /* 割り込み禁止設定 */

        UART_RegInt_Params_init( &xna_lin_uart_params );                  /* UART構成情報初期化 */
        /* UART構成情報に値を設定 */
        xna_lin_uart_params.readMode        = UART2_Mode_CALLBACK;
        xna_lin_uart_params.writeMode       = UART2_Mode_CALLBACK;
        xna_lin_uart_params.readCallback    = l_ifc_rx_ch1;
        xna_lin_uart_params.writeCallback   = l_ifc_tx_ch1;
        xna_lin_uart_params.readReturnMode  = UART2_ReadReturnMode_FULL;
        xna_lin_uart_params.baudRate        = U4L_LIN_BAUDRATE;
        xna_lin_uart_params.dataLength      = UART2_DataLen_8;
        xna_lin_uart_params.stopBits        = UART2_StopBits_1;
        xna_lin_uart_params.parityType      = UART2_Parity_NONE;
        // #define UART_RXERROR_OVERRUN 0x00000008
        // #define UART_RXERROR_BREAK   0x00000004
        // #define UART_RXERROR_PARITY  0x00000002
        // #define UART_RXERROR_FRAMING 0x00000001
        xna_lin_uart_params.eventMask       = 0xFFFFFFFF;
        xna_lin_uart_params.userArg         = NULL;
        #if ORIGINAL_UART_DRIVER
        xnl_lin_uart_handle = UART2_open( CONFIG_UART_INDEX,&xna_lin_uart_params );   /* UARTハンドル生成 */
        #else
        xnl_lin_uart_handle = UART_RegInt_open( CONFIG_UART_INDEX,&xna_lin_uart_params );   /* UARTハンドル生成 */
        #endif
        if( NULL == xnl_lin_uart_handle )
        {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                 /* MCUステータス異常 */
        }

        l_vog_lin_irq_res();                                        /* 割り込み設定を復元 */
    }

}

/**
 * @brief   外部INTレジスタの初期化 処理
 *
 *          外部INTレジスタの初期化
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 割り込み禁止設定
 *          - GPIOモジュール初期化
 *          - GPIOピン構成を設定
 *          - GPIOピン構成設定に成功しなかった場合
 *              - システム異常フラグにMCUステータス異常設定
 *          - GPIOピンによるコールバック関数登録
 *          - 割り込み設定復元
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
    /*** INTの初期化 ***/
    l_u1g_lin_irq_dis();                            /* 割り込み禁止設定 */

    GPIO_init();                                    /* GPIOモジュール初期化 */
    s2a_lin_ret = GPIO_setConfig( U1G_LIN_INTPIN, GPIO_CFG_INPUT_INTERNAL | GPIO_CFG_IN_INT_FALLING ); /* GPIOピン構成を設定 */
    if( GPIO_STATUS_SUCCESS != s2a_lin_ret )
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;     /* MCUステータス異常 */
    }
    GPIO_setCallback( U1G_LIN_INTPIN, l_ifc_aux_ch1 ); /* GPIOピンによるコールバック関数登録 */

    l_vog_lin_irq_res();                            /* 割り込み設定を復元 */
}
/**
 * @brief   データの受信処理(API)
 *
 *          データの受信処理(API)
 *
 * @cond DETAIL
 * @par     処理内容
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
    l_u8    u1a_lin_rx_err;
    l_u8    u1a_lin_rx_byte;
    l_u32   u4a_lin_error_status;

    /* 受信データ取得 (1バイト目のみ) */
    u1a_lin_rx_byte = ((l_u8 *)u1a_lin_rx_data)[0];

    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status = U4L_LIN_HWREG(((( UART2_HWAttrs const * )( xnl_lin_uart_handle->hwAttrs ))->baseAddr + UART_O_RSR_ECR ));

    /* 受信エラー情報の生成 */
    u1a_lin_rx_err = U1G_LIN_BYTE_CLR;

    /* オーバーランエラーチェック */
    if( ( u4a_lin_error_status & U1G_LIN_OVERRUN_REG_ERR ) == U1G_LIN_OVERRUN_REG_ERR )
    {
        u1a_lin_rx_err |= U1G_LIN_SOER_SET;     /* オーバーランエラー発生 */
    }

    /* フレーミングエラーチェック */
    if( ( u4a_lin_error_status & U1G_LIN_FRAMING_REG_ERR ) == U1G_LIN_FRAMING_REG_ERR )
    {
        u1a_lin_rx_err |= U1G_LIN_SFER_SET;     /* フレーミングエラー発生 */
    }

    /* パリティエラーチェック */
    if( ( u4a_lin_error_status & U1G_LIN_PARITY_REG_ERR ) == U1G_LIN_PARITY_REG_ERR )
    {
        u1a_lin_rx_err |= U1G_LIN_SPER_SET;     /* パリティエラー発生 */
    }

    /* スレーブタスクに受信報告 (1バイト) */
    l_vog_lin_rx_int( u1a_lin_rx_byte, u1a_lin_rx_err );
}
/**
 * @brief   データの送信完了割り込み(API)
 *
 *          データの送信完了割り込み
 *
 * @cond DETAIL
 * @par     処理内容
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
/*  外部INT割り込み制御処理(API)                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
/*  外部INT割り込みの際に呼び出されるAPI          */
/**************************************************/
void  l_ifc_aux_ch1(uint_least8_t index)
{
    l_vog_lin_irq_int();                        /* 外部INT割り込み報告 */
}

/***********************************/
/* MCU固有のSFR設定用関数処理      */
/***********************************/
/**
 * @brief   UART受信割り込み許可 処理
 *
 *          UART受信割り込み許可
 * 
 * @param 　受信データサイズ
 * 
 * @cond DETAIL
 * @par     処理内容
 *          - 割り込み禁止設定
 *          - UART受信割り込み許可
 *          - UART受信開始
 *          - UART受信開始成功しなかった場合
 *              - システム異常フラグにMCUステータス異常設定
 *          - 割り込み設定復元
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
#if ORIGINAL_UART_DRIVER
void  l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size)
{
    l_s16    s2a_lin_ret;
    l_u1g_lin_irq_dis();                                /* 割り込み禁止設定 */
    /* バッファデータを削除する処理 */
    if(u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE)
    {
        UART2_flushRx(xnl_lin_uart_handle);
    }

    UART2_rxEnable( xnl_lin_uart_handle );              /* UART受信割り込み許可 */
    s2a_lin_ret = UART2_read( xnl_lin_uart_handle,      /* UART受信開始 */
                              &u1l_lin_rx_buf,
                              u1a_lin_rx_data_size,
                              NULL );
    if( UART2_STATUS_SUCCESS != s2a_lin_ret )
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;         /* MCUステータス異常 */
    }
    l_vog_lin_irq_res();                                /* 割り込み設定復元 */
}
#else
void  l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size)
{
    l_s16    s2a_lin_ret;
    l_u1g_lin_irq_dis();                                /* 割り込み禁止設定 */
    /* バッファデータを削除する処理 */
    if(u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE)
    {
        UART_RegInt_flushRx(xnl_lin_uart_handle);
    }
    UART_RegInt_rxEnable( xnl_lin_uart_handle );              /* UART受信割り込み許可 */
    s2a_lin_ret = UART_RegInt_read( xnl_lin_uart_handle,
                      &u1l_lin_rx_buf,
                      u1a_lin_rx_data_size);
    if( UART2_STATUS_SUCCESS != s2a_lin_ret )
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;         /* MCUステータス異常 */
    }
    l_vog_lin_irq_res();                                /* 割り込み設定復元 */
}
#endif

/**
 * @brief   UART受信割り込み禁止 処理
 *
 *          UART受信割り込み禁止
 *
 * @cond DETAIL
 * @par     処理内容
 *          受信コールバックが帰ってきた時点でUART受信割り込みが停止するため、禁止処理は不要。記録のため関数のみ残す。
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_rx_dis(void)
{
    /* 受信コールバックが帰ってきた時点でUART受信割り込みが停止するため、禁止処理は不要。 */
#ifndef ORIGINAL_UART_DRIVER
    return;
#else
    UART_RegInt_rxDisable(xnl_lin_uart_handle);
#endif
}


/**************************************************/
/*  INT割り込み許可 処理                          */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_enb(void)
{
    l_u1g_lin_irq_dis();                                                        /* 割り込み禁止設定 */

    GPIO_setInterruptConfig( U1G_LIN_INTPIN, GPIO_CFG_IN_INT_RISING);   /* 立ち上がりエッジを検知 */
    GPIO_enableInt( U1G_LIN_INTPIN );                                     /* INT割り込み許可 */

    l_vog_lin_irq_res();                                                         /* 割り込み設定復元 */
}


/**
 * @brief   INT割り込み許可(Wakeupパルス検出用) 処理
 *
 *          INT割り込み許可(Wakeupパルス検出用)
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 割り込み禁止設定
 *          - UART受信割り込み許可
 *          - 割り込み設定復元
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_int_enb_wakeup(void)
{
    l_u1g_lin_irq_dis();                                                       /* 割り込み禁止設定 */

    GPIO_setInterruptConfig( U1G_LIN_INTPIN, GPIO_CFG_IN_INT_FALLING); /* 立ち下がりエッジを検知 */
    GPIO_enableInt( U1G_LIN_INTPIN );                                    /* INT割り込み許可 */

    l_vog_lin_irq_res();                                                        /* 割り込み設定復元 */
}


/**
 * @brief   INT割り込み禁止 処理
 *
 *          INT割り込み禁止
 *
 * @cond DETAIL
 * @par     処理内容
 *          - 割り込み禁止設定
 *          - INT割り込み禁止
 *          - 割り込み設定復元
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_int_dis(void)
{
    l_u1g_lin_irq_dis();                                /* 割り込み禁止設定 */

    GPIO_disableInt( U1G_LIN_INTPIN );            /* INT割り込み禁止 */

    l_vog_lin_irq_res();                                 /* 割り込み設定復元 */
}

/**
 * @brief   送信レジスタにデータをセットする 処理
 *
 *          送信レジスタにデータをセットする
 *
 * @param   送信データ
 * @return  なし
 * @cond DETAIL
 * @par     処理内容
 *          - 送信バッファレジスタに データを格納
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
void  l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    /* 1バイト受信許可（Half-Duplex用リードバック） */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE, U1G_LIN_1);

    /* 送信バッファに1バイト格納 */
    u1l_lin_tx_buf[U1G_LIN_0] = u1a_lin_data;

    /* 1バイト送信 */
#if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle, u1l_lin_tx_buf, U1G_LIN_1, NULL);
#else
    UART_RegInt_write(xnl_lin_uart_handle, u1l_lin_tx_buf, U1G_LIN_1);
#endif
}
/**
 * @brief   リードバック 処理
 *
 *          リードバック 処理
 *
 * @param リードバック比較用データ
 * @return (0 / 1) : 読込み成功 / 読込み失敗
 *
 * @cond DETAIL
 * @par     処理内容
 *          - エラーステータスをレジスタから取得
 *          - 受信データをレジスタから取得
 *          - 受信データの有無をチェック
 *              - エラーが発生している場合
 *                  - 戻り値=読込み失敗
 *              - エラーが発生していない場合
 *                  - 受信バッファの内容と引数を比較し違う場合
 *                      - 戻り値=読込み失敗
 *                  - 受信バッファの内容と引数を比較し正しい場合
 *                      - 戻り値=読込み成功
 *
 * @par     ID
 *          - ID;;
 *          - REF;;
 * @endcond
 *
 */
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;

    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status  = U4L_LIN_HWREG(((( UART2_HWAttrs const * )( xnl_lin_uart_handle->hwAttrs ))->baseAddr +  UART_O_RSR_ECR ));
    u4a_lin_error_status  = U4G_DAT_ZERO; /*一旦エラー検出機能オフ*/

    /* エラーが発生しているかをチェック */
    if(    ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN ) == U4G_LIN_UART_ERR_OVERRUN )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY  ) == U4G_LIN_UART_ERR_PARITY  )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING ) == U4G_LIN_UART_ERR_FRAMING ) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        /* 受信バッファの1バイト目と引数を比較 */
        if(u1l_lin_rx_buf[U1G_LIN_0] != u1a_lin_data)
        {
            u1a_lin_result = U1G_LIN_NG;
        }
        else
        {
            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return( u1a_lin_result );
}


/**
 * @brief LINチェックサム計算
 * @param u1a_lin_pid           pidデータ
 * @param u1a_lin_data          チェックサム対象のデータ
 * @param u1a_lin_data_length   データの長さ（バイト数）
 * @param type                  チェックサムの種類（クラシック or 拡張）
 * @return                      チェックサム値
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
    #ifdef ORIGINAL_UART_DRIVER
    UART2_close(xnl_lin_uart_handle);
    #else
    UART_RegInt_close(xnl_lin_uart_handle);
    #endif
    xnl_lin_uart_handle = NULL;
}

/**
 * @brief LINタイマー割り込みコールバック
 * @param handle タイマーハンドル
 * @param interruptMask 割り込みマスク
 */
void l_ifc_tm_ch1(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask)
{
    if (interruptMask & LGPTimerLPF3_INT_TGT)
    {
        /* SLIN CORE層のタイマー割り込みハンドラ呼び出し */
        l_vog_lin_tm_int();
    }
}

/**
 * @brief LINタイマー初期化
 *
 * @cond DETAIL
 * @par 処理内容
 *      - LGPTimerのパラメータ初期化
 *      - タイマーコールバック設定
 *      - LGPTimer0をオープン
 *      - エラー時はシステム異常フラグ設定
 *
 * @par 19200bps設定
 *      - 1ビット = 52.08us
 *      - u4l_lin_us_per_bit_x10000 = 520833UL
 * @endcond
 */
void l_vog_lin_timer_init(void)
{
    LGPTimerLPF3_Params params;

    /* パラメータ初期化 */
    LGPTimerLPF3_Params_init(&params);
    params.hwiCallbackFxn = l_ifc_tm_ch1;
    params.prescalerDiv = 0;  /* プリスケーラなし (48MHz直接) */

    /* タイマーオープン (LGPT0を使用) */
    xnl_lin_timer_handle = LGPTimerLPF3_open(0, &params);

    if (xnl_lin_timer_handle == NULL)
    {
        /* エラー処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_TIMER;
    }
}

/**
 * @brief ビットタイマ設定
 * @param u1a_lin_bit タイマ設定値（ビット長）
 *
 * @cond DETAIL
 * @par 処理内容
 *      - タイマー停止
 *      - ビット長をマイクロ秒に変換
 *        @19200bps: 1ビット = 52.08us
 *        計算式: timeout_us = u1a_lin_bit * 520833 / 10000
 *      - カウンタターゲット計算 (48MHz = 48カウント/us)
 *      - カウンタターゲット設定
 *      - ワンショットモードで開始
 *
 * @par 使用例
 *      - Response Space: l_vog_lin_bit_tm_set(1)  -> 52.08us
 *      - Inter-byte Space: l_vog_lin_bit_tm_set(11) -> 572.9us (10bit + 1bit)
 * @endcond
 */
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    l_u32 u4a_lin_counter_target;
    l_u32 u4a_lin_timeout_us;

    /* タイマー停止 */
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    /* ビット長をマイクロ秒に変換 */
    /* @19200bps: 1ビット = 52.08us = 520833 / 10000 us */
    u4a_lin_timeout_us = ((l_u32)u1a_lin_bit * U4L_LIN_1BIT_TIMERVAL) / 1000UL;

    /* カウンタターゲット計算 */
    /* システムクロック48MHz = 48カウント/us */
    u4a_lin_counter_target = u4a_lin_timeout_us * 48UL;

    /* 0始まりなので-1 */
    if (u4a_lin_counter_target > 0UL)
    {
        u4a_lin_counter_target -= 1UL;
    }

    /* オーバーフロー対策（16ビットタイマーLGPT0の場合） */
    if (u4a_lin_counter_target > 0xFFFFUL)
    {
        u4a_lin_counter_target = 0xFFFFUL;
    }

    /* カウンタターゲット設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4a_lin_counter_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}

/**
 * @brief フレームタイマ停止
 *
 * @cond DETAIL
 * @par 処理内容
 *      - LGPTimerを停止
 * @endcond
 */
void l_vog_lin_frm_tm_stop(void)
{
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }
}

/**
 * @brief 受信タイムアウトタイマー設定
 *
 * @param[in] u1a_lin_bit タイマー設定ビット数
 *
 * @cond DETAIL
 * @par 処理内容
 *      - データ受信時のタイムアウトタイマーを設定
 *      - LIN規格: (Data Length + Checksum) × 1.4 で計算
 *      - @19200bps: 1 bit = 52.08μs
 *
 * @par 計算式
 *      タイムアウト時間 = u1a_lin_bit × 1.4 × 52.08μs
 *
 * @par 使用例
 *      - フレームサイズ8の場合: l_vog_lin_rcv_tm_set(90)
 *        → (8データ + 1チェックサム) × 10bit = 90bit
 *        → 90 × 1.4 = 126bit
 *        → 126 × 52.08μs = 6.56ms
 * @endcond
 */
void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    l_u32 u4a_lin_timeout_us;
    l_u32 u4a_lin_counter_target;

    /* タイムアウト時間の計算: (bit数 × 1.4) */
    /* 1.4倍 = 14/10 で計算 */
    l_u16 u2a_lin_tmp_bit = (((l_u16)u1a_lin_bit * 14U) / 10U);

    /* マイクロ秒単位の時間に変換 (@19200bps: 1bit = 52.08us) */
    u4a_lin_timeout_us = ((l_u32)u2a_lin_tmp_bit * U4L_LIN_1BIT_TIMERVAL) / 1000UL;

    /* 48MHzクロックでのカウンタターゲット値計算 */
    u4a_lin_counter_target = u4a_lin_timeout_us * 48UL - 1UL;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4a_lin_counter_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}

/**
 * @brief Physical Busエラー検出タイマー設定
 *
 * @cond DETAIL
 * @par 処理内容
 *      - Physical Busエラー検出用の長時間タイマーを設定
 *      - LIN規格: 25000 bit時間でタイムアウト
 *      - @19200bps: 25000 × 52.08μs = 1.302秒
 *
 * @par 使用目的
 *      - Break待ち状態でのバスエラー検出
 *      - 510回連続のヘッダータイムアウト相当時間
 *      - バスの物理的な異常を検出
 *
 * @par 備考
 *      - Synch Break受信待ち設定時に呼び出される
 *      - l_vol_lin_set_synchbreak() から呼び出し
 * @endcond
 */
void l_vog_lin_bus_tm_set(void)
{
    l_u32 u4a_lin_timeout_us;
    l_u32 u4a_lin_counter_target;
    l_u16 u2a_lin_tmp_bit;

    /* Physical Busエラータイムアウト: 25000 bit */
    u2a_lin_tmp_bit = 25000U;

    /* マイクロ秒単位の時間に変換 (@19200bps: 1bit = 52.08us) */
    /* 25000 bit × 52.08us = 1,302,000 us = 1.302秒 */
    u4a_lin_timeout_us = ((l_u32)u2a_lin_tmp_bit * U4L_LIN_1BIT_TIMERVAL) / 1000UL;

    /* 48MHzクロックでのカウンタターゲット値計算 */
    u4a_lin_counter_target = u4a_lin_timeout_us * 48UL - 1UL;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4a_lin_counter_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}

/**
 * @brief タイマークローズ
 *
 * @cond DETAIL
 * @par 処理内容
 *      - LGPTimerをクローズ
 *      - ハンドルをNULLに設定
 * @endcond
 */
void l_vog_lin_timer_close(void)
{
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_close(xnl_lin_timer_handle);
        xnl_lin_timer_handle = NULL;
    }
}

/***** End of File *****/