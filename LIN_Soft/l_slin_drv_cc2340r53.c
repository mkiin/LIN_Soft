/**
 * @file        l_slin_drv_cc2340r53.c
 *
 * @brief       SLIN DRV層
 *
 * @attention   編集禁止
 *
 * @note        UART2ドライバからUART_RegIntドライバへ移植済み
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
#include "../UART_DRV/UART_RegInt.h"
#include <ti/drivers/GPIO.h>
#include <ti/drivers/timer/LGPTimerLPF3.h>
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
void   l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size);
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size);

/*** 変数(static) ***/
static l_u16            u2l_lin_tm_bit;             /* 1bitタイム値 */
static l_u16            u2l_lin_tm_maxbit;          /* 0xFFFFカウント分のビット長 */
static LGPTimerLPF3_Handle  xnl_lin_timer_handle;       /**< @brief LGPTimerハンドル */
static UART_RegInt_Handle   xnl_lin_uart_handle;        /**< @brief UART_RegIntハンドル */
l_u8             u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];       /**< @brief 受信バッファ */
l_u8             u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE];      /**< @brief 送信バッファ */
/**************************************************/

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
    UART_RegInt_Params xna_lin_uart_params;                           /**< @brief  UARTパラメータ 型: UART_RegInt_Params */

    if( NULL == xnl_lin_uart_handle )
    {
        l_u1g_lin_irq_dis();                                        /* 割り込み禁止設定 */

        UART_RegInt_Params_init( &xna_lin_uart_params );                  /* UART構成情報初期化 */
        /* UART構成情報に値を設定 */
        xna_lin_uart_params.readMode        = UART_REGINT_MODE_CALLBACK;
        xna_lin_uart_params.writeMode       = UART_REGINT_MODE_CALLBACK;
        xna_lin_uart_params.readCallback    = l_ifc_rx_ch1;
        xna_lin_uart_params.writeCallback   = l_ifc_tx_ch1;
        xna_lin_uart_params.readReturnMode  = UART_REGINT_RETURN_FULL;
        xna_lin_uart_params.baudRate        = U4L_LIN_BAUDRATE;
        xna_lin_uart_params.dataLength      = UART_REGINT_LEN_8;
        xna_lin_uart_params.stopBits        = UART_REGINT_STOP_ONE;
        xna_lin_uart_params.parityType      = UART_REGINT_PAR_NONE;
        xnl_lin_uart_handle = UART_RegInt_open( CONFIG_UART_INDEX,&xna_lin_uart_params );   /* UARTハンドル生成 */
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
void  l_ifc_rx_ch1(UART_RegInt_Handle handle, void *u1a_lin_rx_data, size_t count, void *userArg, int_fast16_t u1a_lin_rx_status)
{
    l_u8    u1a_lin_rx_set_err;

    /* 受信エラー情報の生成 */
    u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;//現在エラー検出未実装
    l_vog_lin_rx_int( (l_u8 *)u1a_lin_rx_data, u1a_lin_rx_set_err );     /* スレーブタスクに受信報告 */

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
void  l_ifc_tx_ch1(UART_RegInt_Handle handle, void *u1a_lin_tx_data, size_t count, void *userArg, int_fast16_t u1a_lin_tx_status)
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
    s2a_lin_ret = UART_RegInt_read( xnl_lin_uart_handle,      /* UART受信開始 */
                              &u1l_lin_rx_buf,
                              u1a_lin_rx_data_size,
                              NULL );
    if( UART_REGINT_STATUS_SUCCESS != s2a_lin_ret )
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;         /* MCUステータス異常 */
    }
    l_vog_lin_irq_res();                                /* 割り込み設定復元 */
}


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
void  l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE,u1a_lin_data_size);
    /* 送信バッファレジスタに データを格納 */
    UART_RegInt_write(xnl_lin_uart_handle,u1a_lin_data,u1a_lin_data_size,NULL);
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
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    l_u8    u1a_lin_index;
    const UART_RegIntLPF3_HWAttrs *hwAttrs;
    
    /* ハードウェア属性取得 */
    hwAttrs = (const UART_RegIntLPF3_HWAttrs *)xnl_lin_uart_handle->hwAttrs;
    
    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status  = U4L_LIN_HWREG(hwAttrs->baseAddr +  UART_O_RSR_ECR);
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
        u1a_lin_result = U1G_LIN_OK;
        /* 受信バッファの内容と引数を比較 */
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
 * @brief LINチェックサム計算
 * @param u1a_lin_pid           pidデータ
 * @param u1a_lin_data          チェックサム対象のデータ
 * @param u1a_lin_data_length   データの長さ(バイト数)
 * @param type                  チェックサムの種類(クラシック or 拡張)
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
    UART_RegInt_close(xnl_lin_uart_handle);
    xnl_lin_uart_handle = NULL;
}
/***** End of File *****/
