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

/* 9.5bit Break検出用定数 */
#define U4L_LIN_BREAK_9_5BIT_US     (495U)      /**< @brief 9.5bit時間(μs) @19200bps: 9.5*52.08≒495μs 型: l_u32 */
#define U4L_LIN_BREAK_MARGIN_US     (5U)        /**< @brief Break検出マージン(μs) 型: l_u32 */
#define U4L_LIN_BREAK_THRESHOLD_US  (U4L_LIN_BREAK_9_5BIT_US - U4L_LIN_BREAK_MARGIN_US)  /**< @brief Break検出閾値490μs 型: l_u32 */
#define U4L_LIN_BREAK_TIMER_COUNT   (U4L_LIN_BREAK_THRESHOLD_US * 48U) /**< @brief Break検出タイマーカウント値(48MHz) 型: l_u32 */

/* 9.5bit Break検出状態 */
#define U1L_BREAK_STATE_IDLE        ((l_u8)0x00)    /**< @brief Break検出アイドル状態 型: l_u8 */
#define U1L_BREAK_STATE_WAIT        ((l_u8)0x01)    /**< @brief Break待機状態 型: l_u8 */
#define U1L_BREAK_STATE_TIMING      ((l_u8)0x02)    /**< @brief Break計測中 型: l_u8 */
#define U1L_BREAK_STATE_DETECTED    ((l_u8)0x03)    /**< @brief Break検出完了 型: l_u8 */

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
void   l_vog_lin_rx_enb(void);
void   l_vog_lin_rx_dis(void);
void   l_vog_lin_int_enb(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
void  l_vog_lin_int_enb_wakeup(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
void   l_vog_lin_int_dis(void);
void   l_vog_lin_tx_char(const l_u8 u1a_lin_data);
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data);

/*** 変数(static) ***/
static l_u16            u2l_lin_tm_bit;             /* 1bitタイム値 */
static l_u16            u2l_lin_tm_maxbit;          /* 0xFFFFカウント分のビット長 */
static l_u16            u2l_lin_tm_cnt;             /* タイマカウンタ数 (分割割り込みにした際に残割り込み回数を導出するのに使用  */
static l_u32            u4l_lin_tm_target;          /* タイマカウンタターゲット値 */
static LGPTimerLPF3_Handle  xnl_lin_timer_handle;       /**< @brief LGPTimerハンドル */
static UART2_Handle         xnl_lin_uart_handle;        /**< @brief UART2ハンドル */
l_u8             u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];       /**< @brief 受信バッファ */
l_u8             u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE];      /**< @brief 送信バッファ */

/* 9.5bit Break検出用変数 */
static l_u8     u1l_break_state;                    /**< @brief Break検出状態 型: l_u8 */
static l_bool   b1l_break_timer_expired;            /**< @brief 9.5bitタイマー満了フラグ 型: l_bool */
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
        xna_lin_uart_params.eventMask       = 0xf;
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
 * @brief LINタイマー初期化
 *
 * @cond DETAIL
 * @par 処理内容
 *      - 1ビットあたりのカウント値を設定
 *      - 16ビットタイマーで表現可能な最大ビット数を計算
 *      - LGPTimerをワンショットモード用に初期化
 *
 * @par タイマー設定
 *      - クロック: 48MHz（プリスケーラなし）
 *      - 1ビット: 2500カウント（@19200bps）
 *      - 最大ビット: 26ビット（65535/2500）
 * @endcond
 */
void l_vog_lin_timer_init(void)
{
    if (xnl_lin_timer_handle == NULL)
    {
        LGPTimerLPF3_Params params;

        /* 1ビットあたりのカウント値 */
        u2l_lin_tm_bit = U4L_LIN_1BIT_TIMERVAL;

        /* 16ビットタイマーで表現可能な最大ビット数 */
        /* ０除算のチェック */
        if (u2l_lin_tm_bit != U2G_LIN_BYTE_CLR)
        {
            u2l_lin_tm_maxbit = U2G_LIN_WORD_LIMIT / u2l_lin_tm_bit;
        }
        else
        {
            u2l_lin_tm_maxbit = U2G_LIN_1;
            u1g_lin_syserr = U1G_LIN_SYSERR_DIV;
        }

        /* 割り込み残回数を初期化 */
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;

        /* パラメーター初期化 */
        LGPTimerLPF3_Params_init(&params);
        params.hwiCallbackFxn = l_ifc_tm_ch1;
        params.intPhaseLate = true;
        params.prescalerDiv = 0U;  /* プリスケーラなし (48MHz直接) */

        /* タイマーオープン */
        xnl_lin_timer_handle = LGPTimerLPF3_open(CONFIG_LGPTIMER_1, &params);

        if (xnl_lin_timer_handle == NULL)
        {
            /* タイマーオープン失敗 */
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;
        }
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
    s2a_lin_ret = GPIO_setConfig( U1G_LIN_INTPIN, GPIO_CFG_INPUT_INTERNAL | GPIO_CFG_IN_INT_RISING ); /* GPIOピン構成を設定 */
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

    /* 受信データ取得 (1バイト目のみ) */
    u1a_lin_rx_byte = ((l_u8 *)u1a_lin_rx_data)[0];

    /* 受信エラー情報の生成 */
    u1a_lin_rx_err = U1G_LIN_BYTE_CLR;

    /* オーバーランエラーチェック */
    if( ( u1a_lin_rx_status & U1G_LIN_OVERRUN_REG_ERR ) == U1G_LIN_OVERRUN_REG_ERR )
    {
        u1a_lin_rx_err |= U1G_LIN_SOER_SET;     /* オーバーランエラー発生 */
    }
    /* フレーミングエラーチェック */
    if( ( u1a_lin_rx_status & U1G_LIN_FRAMING_REG_ERR ) == U1G_LIN_FRAMING_REG_ERR )
    {
        u1a_lin_rx_err |= U1G_LIN_SFER_SET;     /* フレーミングエラー発生 */
    }
    /* パリティエラーチェック */
    if( ( u1a_lin_rx_status & U1G_LIN_PARITY_REG_ERR ) == U1G_LIN_PARITY_REG_ERR )
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


/**
 * @brief LINタイマー割り込みコールバック
 * @param handle タイマーハンドル
 * @param interruptMask 割り込みマスク
 * ★9.5bit Break検出統合版★
 */
void l_ifc_tm_ch1(LGPTimerLPF3_Handle handle, LGPTimerLPF3_IntMask interruptMask)
{
    if (interruptMask & LGPTimerLPF3_INT_TGT)
    {
        /* 9.5bit Break計測中の場合 */
        if (u1l_break_state == U1L_BREAK_STATE_TIMING)
        {
            /* 9.5bit経過フラグをセット */
            b1l_break_timer_expired = U2G_LIN_BIT_SET;
            /* 注: GPIO立ち上がりエッジで最終判定を行う */
        }
        /* 通常のLINタイマー処理 */
        else
        {
            /* 残割り込み回数がある場合タイマー再開をしそうでなければ割込みハンドラ呼び出し */
            if (u2l_lin_tm_cnt > U2G_LIN_BIT_CLR)
            {
                u2l_lin_tm_cnt--;       /* 割込み回数をデクリメント */

                /* タイマー再開 */
                LGPTimerLPF3_setInitialCounterTarget(handle, u4l_lin_tm_target, true);
                LGPTimerLPF3_start(handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
            }
            else
            {
                /* SLIN CORE層のタイマー割り込みハンドラ呼び出し */
                l_vog_lin_tm_int();
            }
        }
    }
}


/**************************************************/
/*  外部INT割り込み制御処理(API)                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
/*  外部INT割り込みの際に呼び出されるAPI          */
/*  ★9.5bit Break検出処理統合版★                 */
/**************************************************/
void  l_ifc_aux_ch1(uint_least8_t index)
{
    /* 9.5bit Break検出モード時 */
    if (u1l_break_state == U1L_BREAK_STATE_WAIT || u1l_break_state == U1L_BREAK_STATE_TIMING)
    {
        /* 現在のGPIOピンレベルを読み取り */
        l_u32 u4a_pin_level = GPIO_read(U1G_LIN_INTPIN);

        if (u4a_pin_level == 0)  /* 立ち下がりエッジ (Break開始) */
        {
            /* Break計測開始 */
            u1l_break_state = U1L_BREAK_STATE_TIMING;
            b1l_break_timer_expired = U2G_LIN_BIT_CLR;

            /* 9.5bitタイマー起動 (490μs) */
            LGPTimerLPF3_stop(xnl_lin_timer_handle);
            LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, U4L_LIN_BREAK_TIMER_COUNT, true);
            LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);
            LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);

            /* GPIOを立ち上がりエッジ検出に切り替え */
            GPIO_setInterruptConfig(U1G_LIN_INTPIN, GPIO_CFG_IN_INT_RISING);
        }
        else  /* 立ち上がりエッジ (Break終了) */
        {
            /* タイマー停止 */
            LGPTimerLPF3_stop(xnl_lin_timer_handle);

            /* 9.5bitタイマーが満了していた場合 = Break検出成功 */
            if (b1l_break_timer_expired == U2G_LIN_BIT_SET)
            {
                u1l_break_state = U1L_BREAK_STATE_DETECTED;

                /* Break検出完了 → CORE層にUART受信開始を通知 */
                /* 既存のUART受信フローを開始 (Sync Field待ち) */
                l_vog_lin_rx_enb();
            }
            else
            {
                /* 9.5bit未満 → Break未検出、待機状態に戻る */
                u1l_break_state = U1L_BREAK_STATE_WAIT;
                GPIO_setInterruptConfig(U1G_LIN_INTPIN, GPIO_CFG_IN_INT_FALLING);
            }
        }
    }
    else
    {
        /* 通常モード(Wakeup検出など) */
        l_vog_lin_irq_int();                        /* 外部INT割り込み報告 */
    }
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
void  l_vog_lin_rx_enb(void)
{
    l_s16    s2a_lin_ret;

    l_u1g_lin_irq_dis();                                /* 割り込み禁止設定 */

    /* バッファデータを削除する処理 */
    if(1 == U1G_LIN_FLUSH_RX_USE)
    {
#if ORIGINAL_UART_DRIVER
        UART2_flushRx(xnl_lin_uart_handle);
#else
        UART_RegInt_flushRx(xnl_lin_uart_handle);
#endif
    }
#if ORIGINAL_UART_DRIVER
    UART2_rxEnable( xnl_lin_uart_handle );              /* UART受信割り込み許可 */
    s2a_lin_ret = UART2_read( xnl_lin_uart_handle,      /* UART受信開始 */
                              &u1l_lin_rx_buf,
                              u1a_lin_rx_data_size,
                              NULL );
#else
    s2a_lin_ret = UART_RegInt_read( xnl_lin_uart_handle,      /* UART受信開始 */
                                    &u1l_lin_rx_buf,
                                    U1G_LIN_1 );
#endif
    if( UART2_STATUS_SUCCESS != s2a_lin_ret )
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
    l_u1g_lin_irq_dis();                                                        /* 割り込み禁止設定 */

    // UART_RegIntLPF3_disableRxInt(xnl_lin_uart_handle->hwAttrs);

    l_vog_lin_irq_res();                                                         /* 割り込み設定復元 */
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

    GPIO_setInterruptConfig( U1G_LIN_INTPIN, GPIO_CFG_IN_INT_RISING); /* 立ち下がりエッジを検知 */
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
void  l_vog_lin_tx_char(const l_u8 u1a_lin_data)
{
    l_u8 u1a_lin_tx_data[1] = { u1a_lin_data };
    // l_vog_lin_rx_enb();
    /* 送信バッファレジスタに データを格納 */
#if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle,u1a_lin_tx_data,U1G_LIN_1,NULL);
#else
    UART_RegInt_write(xnl_lin_uart_handle, u1a_lin_tx_data, U1G_LIN_1);
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
    l_u16 u2a_lin_tmp_bit;

    /* タイマー停止 */
    l_vog_lin_frm_tm_stop();

    u2a_lin_tmp_bit = (l_u16)u1a_lin_bit;

    /* タイマ値が最大ビット数を超える場合の計算 */
    if (u2a_lin_tmp_bit > u2l_lin_tm_maxbit)
    {
        u2l_lin_tm_cnt = u2a_lin_tmp_bit / u2l_lin_tm_maxbit;
        u2a_lin_tmp_bit /= (u2l_lin_tm_cnt + U2G_LIN_1);
    }
    else
    {
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    }

    /* カウンタターゲット計算・保存 */
    u4l_lin_tm_target = (l_u32)u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4l_lin_tm_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}


/**
 * @brief 応答タイムアウトタイマー設定
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
    l_u16 u2a_lin_tmp_bit;
    l_u16 u2a_lin_rest_bit;                 /* 残りビット数 */
    l_u32 u4a_lin_current_count;
    l_u32 u4a_lin_rest_count;               /* 残りカウント回数 */

    /* タイマー停止 */
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
    }

    /* (Data Length + Checksum) × 1.4 */
    u2a_lin_tmp_bit = (((l_u16)u1a_lin_bit * U2G_LIN_14) / U2G_LIN_10);

    /* 現在のカウンタ値を取得 */
    u4a_lin_current_count = LGPTimerLPF3_getCounter(xnl_lin_timer_handle);

    /* 残りカウント値を計算 */
    if (u4l_lin_tm_target > u4a_lin_current_count)
    {
        u4a_lin_rest_count = u4l_lin_tm_target - u4a_lin_current_count;
    }
    else
    {
        u4a_lin_rest_count = U2G_LIN_WORD_CLR;
    }

    /* 残りカウント値をビット数に変換 */
    if (u2l_lin_tm_bit != U2G_LIN_BYTE_CLR)
    {
        u2a_lin_rest_bit = (l_u16)(u4a_lin_rest_count / u2l_lin_tm_bit);

        /* 残り割り込み回数分のビット数を加算 */
        u2a_lin_rest_bit += u2l_lin_tm_cnt * u2l_lin_tm_maxbit;

        /* 残時間 + 新規タイムアウト時間 */
        u2a_lin_tmp_bit = u2a_lin_rest_bit + u2a_lin_tmp_bit;
    }
    else
    {
        /* 0除算エラー */
        u2a_lin_tmp_bit = U2G_LIN_BYTE_CLR;
        u1g_lin_syserr = U1G_LIN_SYSERR_DIV;
    }

    /* タイマ値が最大ビット数を超える場合の計算 */
    if (u2a_lin_tmp_bit > u2l_lin_tm_maxbit)
    {
        u2l_lin_tm_cnt = u2a_lin_tmp_bit / u2l_lin_tm_maxbit;
        u2a_lin_tmp_bit /= (u2l_lin_tm_cnt + U2G_LIN_1);
    }
    else
    {
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    }

    /* カウンタターゲット計算・保存 */
    u4l_lin_tm_target = (l_u32)u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4l_lin_tm_target, true);

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
    l_u16 u2a_lin_tmp_bit;

    /* タイマー停止 */
    l_vog_lin_frm_tm_stop();

    u2a_lin_tmp_bit = U2G_LIN_BUS_TIMEOUT;  /* 25000 bit */

    /* タイマ値が最大ビット数を超える場合の計算 */
    if (u2a_lin_tmp_bit > u2l_lin_tm_maxbit)
    {
        u2l_lin_tm_cnt = u2a_lin_tmp_bit / u2l_lin_tm_maxbit;
        u2a_lin_tmp_bit /= (u2l_lin_tm_cnt + U2G_LIN_1);
    }
    else
    {
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    }

    /* カウンタターゲット計算・保存 */
    u4l_lin_tm_target = (l_u32)u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマー設定 */
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4l_lin_tm_target, true);

    /* ターゲット割り込み許可 */
    LGPTimerLPF3_enableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);

    /* ワンショットモードで開始 */
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}


/**************************************************/
/*  タイマー停止                                   */
/**************************************************/
void l_vog_lin_frm_tm_stop(void)
{
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);
        LGPTimerLPF3_disableInterrupt(xnl_lin_timer_handle, LGPTimerLPF3_INT_TGT);
    }
    u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    u4l_lin_tm_target = U2G_LIN_WORD_CLR;
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
#if ORIGINAL_UART_DRIVER
    UART2_close(xnl_lin_uart_handle);
#else
    UART_RegInt_close(xnl_lin_uart_handle);
#endif
    xnl_lin_uart_handle = NULL;
}

/***** End of File *****/