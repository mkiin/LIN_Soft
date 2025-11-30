/**
 * @file        l_slin_core_CC2340R53.c
 *
 * @brief       SLIN CORE層
 *
 * @attention   編集禁止
 *
 */

#pragma	section	lin

/***** ヘッダ インクルード *****/
#include <ti/drivers/GPIO.h>
#include <stdio.h>
#include "ti_drivers_config.h"
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_stddef.h"
#include "l_slin_core_cc2340r53.h"
#include "l_slin_drv_cc2340r53.h"
#if 1   /* 継続フレーム送信時に送信完了フラグで誤検知している可能性があり、期待するフレームが送信されたかを確認するため、LIN通信管理へのコールバックを実行する */
#include "LIN_Manager.h"
#endif

/***** 関数プロトタイプ宣言 *****/
/*-- API関数定義(extern) --*/
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
/*-- 関数定義(extern) --*/
void   l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err);  /* H850互換: 1バイト受信 */
void   l_vog_lin_tm_int(void);
void   l_vog_lin_irq_int(void);
void   l_vog_lin_set_nm_info(l_u8  u1a_lin_nm_info);
l_u8   l_u1g_lin_tbl_chk(void);
l_u8   l_vog_lin_checksum(l_u8 u1a_lin_pid ,const l_u8* u1a_lin_data, l_u8 u1a_lin_data_length, U1G_LIN_ChecksumType type);

/*-- 関数定義(static) --*/
static void   l_vol_lin_set_wakeup(void);
static void   l_vol_lin_set_synchbreak(void);
static void   l_vol_lin_set_frm_complete(l_u8  u1a_lin_err);

/*** 変数定義(static) ***/
static st_lin_id_slot_type  xnl_lin_id_sl;                     /* 使用中のフレームのIDとスロット番号の管理テーブル */
static l_u8   u1l_lin_slv_sts;                                 /* スレーブタスク用ステータス */
static l_u8   u1l_lin_frm_sz;                                  /* データサイズ */
static l_u8   u1l_lin_rs_cnt;                                  /* 送受信データカウンタ */
static l_u8   u1l_lin_rs_tmp[ U1G_LIN_DATA_SUM_LEN ];          /* 送受信データ用tmpバッファ(Data + Checksum) 9バイト */
static l_u8   u1l_lin_chksum;                                  /* チェックサム格納 (受信用) */
static l_u16  u2l_lin_chksum;                                  /* チェックサム演算用変数 (送信用、H850互換) */
static l_u16  u2l_lin_herr_cnt;                                /* ヘッダタイムアウトエラー回数カウンタ（Physical Busエラー検出用） */

extern l_u8   u1l_lin_rx_buf[U4L_LIN_UART_MAX_READSIZE];       /**< @brief 受信バッファ */
extern l_u8   u1l_lin_tx_buf[U4L_LIN_UART_MAX_WRITESIZE];      /**< @brief 送信バッファ */
/*===========================================================================================*/

/**********************************/
/* MCU非依存API関数処理           */
/**********************************/
/**************************************************/
/*  バッファ、ドライバの初期化処理(API)           */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_init_ch1(void)
{
    l_ifc_init_com_ch1();                                               /* LINバッファの初期化(API) */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RESET;    /* RESET状態にする */
    l_ifc_init_drv_ch1();                                               /* LINドライバの初期化(API) */
    u1g_lin_syserr = U1G_LIN_BYTE_CLR;                                  /* システム異常フラグのクリア */
}

/**************************************************/
/*  LINバッファの初期化 処理(API)                 */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_init_com_ch1(void)
{
    l_u8    u1a_lin_lp1;

  /*--- LINステータスバッファ初期化 ---*/
    /* NADのセット */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_nad = U2G_LIN_NAD;

    /* Physical Busエラーフラグのクリア */
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_CLR;

    /* ヘッダーエラーフラグのクリア */
    xng_lin_sts_buf.un_state.st_err.u2g_lin_e_head = U2G_LIN_BYTE_CLR;

    /* 送受信処理完了フラグのクリア */
    xng_lin_sts_buf.un_rs_flg1.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg2.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg3.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg4.u2g_lin_word = U2G_LIN_WORD_CLR;

    /*--- LINフレームバッファ初期化 ---*/
    for( u1a_lin_lp1 = U1G_LIN_0; u1a_lin_lp1 < U1G_LIN_MAX_SLOT; u1a_lin_lp1++ ){

        l_u1g_lin_irq_dis();                                            /* 割り込み禁止設定 */

        /* ステータス部分(エラーフラグ等)クリア */
        xng_lin_frm_buf[ u1a_lin_lp1 ].un_state.u2g_lin_word = U2G_LIN_BYTE_CLR;

        /* LINバッファのデータ部をデフォルト値で初期化する */
        /* デフォルト値設定 */
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

        l_vog_lin_irq_res();                                            /* 割り込み設定復元 */
    }
}


/**************************************************/
/*  LINドライバの初期化 処理(API)                 */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_init_drv_ch1(void)
{
    /*** UARTの初期化 ***/
    l_vog_lin_uart_init();

    /*** タイマーの初期化 ***/
    l_vog_lin_timer_init();

    /*** 外部INTの初期化 ***/
    l_vog_lin_int_init();

    u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                                /* Physical Busエラー検出カウンタクリア */

    /* スレーブタスクのステータスを確認 */
    switch( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts ){
    /* SLEEP状態 */
    case ( U2G_LIN_STS_SLEEP ):
        l_vol_lin_set_wakeup();                                         /* Wakeup待機設定 */
        break;
    /* RUN状態 / RUN STANDBY状態 */
    case ( U2G_LIN_STS_RUN_STANDBY ):
    case ( U2G_LIN_STS_RUN ):
        /* SynchBreak受信待ち状態に設定 */
        xng_lin_bus_sts.u2g_lin_word &= U2G_LIN_STSBUF_CLR;             /* リードステータスバッファのクリア */

        l_vol_lin_set_synchbreak();                                     /* Synch Break受信待ち設定 */

        break;
    default:
        /* 上記以外は、何もしない */
        break;
    }
}


/**************************************************/
/*  LIN通信接続 処理(API)                         */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_bool  l_ifc_connect_ch1(void)
{
    l_bool  u2a_lin_result;

    /* ステータスがRESET状態の場合 */
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RESET ) {
        /* SLEEPへ移行 */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_SLEEP;        /* SLEEP状態へ移行 */
        l_vol_lin_set_wakeup();                                                 /* Wakeup待機設定 */

        u2a_lin_result = U2G_LIN_OK;
    }
    /* RESET状態以外の場合 */
    else {
        u2a_lin_result = U2G_LIN_NG;
    }

    return( u2a_lin_result );
}


/**************************************************/
/*  LIN切断 処理(API)                             */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_bool  l_ifc_disconnect_ch1(void)
{
    l_bool  u2a_lin_result;

    l_u1g_lin_irq_dis();                                        /* 割り込み禁止設定 */

    /* SLEEP状態の場合 */
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP )
    {
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RESET;
      /* UARTによるWAKEUP検出の場合 */
      #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
        l_vog_lin_rx_dis();                                     /* 受信割り込みを禁止する */
      /* INT割り込みによるWAKEUP検出の場合 */
      #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
        l_vog_lin_int_dis();                                    /* INT割り込みを禁止する */
      #endif

        u2a_lin_result = U2G_LIN_OK;
    }
    /* RESET状態の場合 */
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RESET ){
        /* なにもせず、処理成功を返す */
        u2a_lin_result = U2G_LIN_OK;
    }
    /* SLEEPでもRESETでもない場合 */
    else{
        u2a_lin_result = U2G_LIN_NG;
    }

    l_vog_lin_irq_res();                                        /* 割り込み設定復元 */

    return( u2a_lin_result );
}


/**************************************************/
/*  wakeupコマンド送信 処理(API)                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_wake_up_ch1(void)
{
    l_u16 u2a_lin_sts;
    l_u8 u1a_lin_wakeup_data;
    l_u1g_lin_irq_dis();                                        /* 割り込み禁止設定 */
    u1a_lin_wakeup_data = U1G_LIN_SND_WAKEUP_DATA;
    u2a_lin_sts = xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts;
    
    l_vog_lin_irq_res();                                    /* 割り込み設定復元 */

    /* RUN STANDBY状態の場合 */
    if( u2a_lin_sts == U2G_LIN_STS_RUN_STANDBY ){
        /* wakeupパルスを送信 */
        l_vog_lin_tx_char( &u1a_lin_wakeup_data,U1G_LIN_DL_1 );
    }
    /* SLEEP状態場合 */
    else if(u2a_lin_sts == U2G_LIN_STS_SLEEP){
      /* UARTによるWAKEUP検出の場合 */
      #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
        l_vog_lin_rx_dis();                                     /* 受信割り込みを禁止する */
      /* INT割り込みによるWAKEUP検出の場合 */
      #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
        l_vog_lin_int_dis();                                    /* INT割り込みを禁止する */
      #endif
        /* wakeupパルスを送信 */
        l_vog_lin_tx_char( &u1a_lin_wakeup_data ,U1G_LIN_DL_1);
    }
    /* RUN STANDBY / SLEEP状態以外の場合 */
    else{
        /* なにもしない */
    }
/* Ver 2.10 変更（管理番号15）:Wakeupパルス送信後、立下りエッジで検出した時のWakeupパルス送信中断対策 */
}


/**************************************************/
/*  LINバス状態リード 処理(API)                   */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： LINバス状態                            */
/**************************************************/
l_u16  l_ifc_read_status_ch1(void)
{
    l_u16  u2a_lin_result;

    l_u1g_lin_irq_dis();                                    /* 割り込み禁止設定 */

    u2a_lin_result = xng_lin_bus_sts.u2g_lin_word;
    xng_lin_bus_sts.u2g_lin_word &= U2G_LIN_STSBUF_CLR;     /* ステータスクリア */

    l_vog_lin_irq_res();                                    /* 割り込み設定復元 */

    return( u2a_lin_result );
}


/**************************************************/
/*  SLEEP状態移行 処理(API)                       */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_sleep_ch1(void)
{
    l_u1g_lin_irq_dis();                                    /* 割り込み禁止設定 */

    /* RUN / RUN STANDBY状態の場合 */
    if( (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN)
     || (xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY) ){

        /* SLEEPへ移行 */ 
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_SLEEP;    /* SLEEP状態に移行 */
        l_vol_lin_set_wakeup();                                             /* Wakeup待機設定 */
    }
    /* RUN / RUN STANDBY以外の場合 なにもしない */

    l_vog_lin_irq_res();                                    /* 割り込み設定復元 */

}


/**************************************************/
/*  LINステータスのリード 処理(API)               */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： LINステータス                          */
/**************************************************/
l_u16  l_ifc_read_lb_status_ch1(void)
{
    l_u16  u2a_lin_result;

    /* 現在のLINステータスを格納 */
    u2a_lin_result = xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts;

    return( u2a_lin_result );
}


/**************************************************/
/*  スロット有効フラグクリア処理(API)             */
/*------------------------------------------------*/
/*  引数： frame: フレーム名                      */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_enable_ch1(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){
        /* LINバッファ スロット有効 */
        xng_lin_frm_buf[ u1a_lin_frm ].un_state.st_bit.u2g_lin_no_use = U2G_LIN_BIT_CLR;
    }
    /* 引数値が範囲外の場合 なにもしない */
}


/**************************************************/
/*  スロット無効フラグセット処理(API)             */
/*------------------------------------------------*/
/*  引数： frame: フレーム名                      */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_disable_ch1(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){
        /* LINバッファ スロット無効 */
        xng_lin_frm_buf[ u1a_lin_frm ].un_state.st_bit.u2g_lin_no_use = U2G_LIN_BIT_SET;
    }
    /* 引数値が範囲外の場合 なにもしない */
}


/**************************************************/
/*  バッファ スロット読み出し処理(API)            */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm  : 読み出すフレーム名      */
/*         u1a_lin_cnt  : 読み出しデータ長        */
/*         pta_lin_data : データ格納先ポインタ    */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_rd_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, l_u8 * pta_lin_data)
{
    /* ポインタ(引数値)がNULL以外の場合 */
    if( pta_lin_data != U1G_LIN_NULL){
        /* 引数値が範囲内の場合 */
        if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

            l_u1g_lin_irq_dis();                      /* 割り込み禁止設定 */

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
                /* 指定サイズ異常の為、何もしない */
                break;
            }

            l_vog_lin_irq_res();                       /* 割り込み設定復元 */
        }
        /* 引数が範囲外の場合 何もしない */
    }
    /* ポインタ(引数値)がNULLの場合 何もしない */
}


/**************************************************/
/*  バッファ スロット書き込み処理(API)            */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm  : 書き込むフレーム名      */
/*         u1a_lin_cnt  : 書き込みデータ長        */
/*         pta_lin_data : 書き込み格納元ポインタ  */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_wr_ch1(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, const l_u8 * pta_lin_data)
{
    /* ポインタ(引数値)がNULL以外の場合 */
    if( pta_lin_data != U1G_LIN_NULL){
        /* 引数値が範囲内の場合 */
        if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

            l_u1g_lin_irq_dis();                      /* 割り込み禁止設定 */

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
                /* 指定サイズ異常の為、何もしない */
                break;
            }

            l_vog_lin_irq_res();                       /* 割り込み設定復元 */
        }
        /* 引数値が範囲外の場合 何もしない */
    }
    /* ポインタ(引数値)がNULLの場合 何もしない */
}


/**************************************************/
/*  バッファスロット初期値設定処理(API)           */
/*------------------------------------------------*/
/*  引数： フレーム名                             */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_set_default_ch1(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

        l_u1g_lin_irq_dis();                      /* 割り込み禁止設定 */

        /* デフォルト値設定 */
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

        l_vog_lin_irq_res();                       /* 割り込み設定復元 */
    }
    /* 引数値が範囲外の場合 なにもしない */
}


/**************************************************/
/*  バッファスロットFAIL値設定処理(API)           */
/*------------------------------------------------*/
/*  引数： フレーム名                             */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_set_fail_ch1(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( u1a_lin_frm < U1G_LIN_MAX_SLOT ){

        l_u1g_lin_irq_dis();                        /* 割り込み禁止設定 */

        /* フェイル値設定 */
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

        l_vog_lin_irq_res();                        /* 割り込み設定復元 */
    }
    /* 引数値が範囲外の場合 なにもしない */
}


/**************************************************/
/*  RUN状態移行 処理(API)                         */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_run_ch1(void)
{
    l_u16 u2a_lin_sts;
    
    l_u1g_lin_irq_dis();                                /* 割り込み禁止設定 */
    u2a_lin_sts = xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts;
    l_vog_lin_irq_res();                                /* 割り込み設定復元 */

    /* SLEEP状態の場合 */
    if( u2a_lin_sts == U2G_LIN_STS_SLEEP )
    {
        l_u1g_lin_irq_dis();                            /* 割り込み禁止設定 */
        /* RUN STANDBY状態へ移行 */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;
        l_vog_lin_irq_res();                            /* 割り込み設定復元 */

        l_ifc_init_drv_ch1();                           /* ドライバ部の初期化 */
    /* SLEEP以外の場合 なにもしない */                          
    }
    else
    {
    }
}


/**************************************************/
/*  受信割り込み発生                              */
/*------------------------------------------------*/
/*  引数： data : 受信データ                      */
/*         err  : 受信エラーフラグ                */
/*  戻値： なし                                   */
/**************************************************/
/**************************************************/
/*  受信割り込み発生 (H850互換実装)               */
/*------------------------------------------------*/
/*  引数： data : 受信データ (1バイト)            */
/*         err  : 受信エラーフラグ                */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err)
{
    l_u8  u1a_lin_protid;

    /*** SLEEP状態の場合 ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP )
    {
        /* WAKEUPコマンド受信(UARTによるWAKEUP検出) */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;      /* RUN STANDBY状態へ移行 */
        l_ifc_init_drv_ch1();                                                       /* ドライバ部の初期化 */
    }
    /*** RUN STANDBY状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY )
    {
        /* Synch Break(UART)待ち状態 */
        if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT )
        {
            /* Framingエラー発生 */
            if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON )
            {
                /* 受信データが00hならば、SynchBreak受信 */
                if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
                {
                    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;          /* RUN状態へ移行 */
                    /* ヘッダタイムアウトタイマセット */
                    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
                    /* SynchField待ち状態に移行 (CC23xxではIRQピンなしのため直接遷移) */
                    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
                    /* UART受信は有効のまま (IRQ待ちをスキップ) */
                }
                /* 受信データが00h以外ならば */
                else
                {
                    /* エラーフラグのリセット */
                    l_vog_lin_rx_enb();                                            /* Synch Break待ち */
                }
            }
            /* Over Runエラーの発生 */
            else if( (u1a_lin_err & U1G_LIN_OVERRUN_ERR) != U1G_LIN_BYTE_CLR )
            {
                /* エラーフラグのリセット */
                l_vog_lin_rx_enb();                                            /* Synch Break待ち */
            }
            /* エラー未発生 */
            else
            {
                /* エラー発生していない場合は何もしない */
            }
        }
    }
    /*** RUN状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN )
    {
        /* スレーブタスクのステータスを確認 */
        switch( u1l_lin_slv_sts )
        {
        /*** Synch Break(UART)待ち状態 ***/
        case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
            /* Framingエラー発生 */
            if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON )
            {
                /* 受信データが00hならば、SynchBreak受信 */
                if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
                {
                    /* ヘッダタイムアウトタイマセット */
                    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
                    /* SynchField待ち状態に移行 (CC23xxではIRQピンなしのため直接遷移) */
                    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
                    /* UART受信は有効のまま (IRQ待ちをスキップ) */
                }
                /* 受信データが00h以外ならば */
                else
                {
                    /* エラーフラグのリセット */
                    l_vog_lin_rx_enb();                                            /* Synch Break待ち */
                }
            }
            /* Over Runエラーの発生 */
            else if( (u1a_lin_err & U1G_LIN_OVERRUN_ERR) != U1G_LIN_BYTE_CLR )
            {
                /* エラーフラグのリセット */
                l_vog_lin_rx_enb();                                            /* Synch Break待ち */
            }
            /* エラー未発生 */
            else
            {
                /* エラー発生していない場合は何もしない */
            }
            break;
        /*** Synch Break(IRQ)待ち状態 ***/
        /* CC23xxではIRQピンが未接続のため、この状態は使用しない */
        /* Break Delimiter検出のための外部IRQ割り込みが利用できないため、 */
        /* Break検出後は直接SYNCHFIELD_WAITへ遷移する */
        // case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
        //     /* 何もしない */
        //     break;
        /*** Synch Field待ち状態 ***/
        case( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
            /* UARTエラーが発生した場合 */
            if( (u1a_lin_err & U1G_LIN_UART_ERR_ON) != U1G_LIN_BYTE_CLR )
            {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;       /* UARTエラー */
                xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;              /* Headerエラー */

                l_vol_lin_set_synchbreak();                                             /* Synch Break受信待ち設定 */
            }
            /* UARTエラーなしの場合 */
            else
            {
                /* 受信データが55h以外の場合 */
                if( u1a_lin_data != U1G_LIN_SYNCH_FIELD_DATA )
                {
                    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_synch = U2G_LIN_BIT_SET;  /* Synch Fieldエラー */
                    xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerエラー */

                    l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
                }
                /* 正常Synch Field受信 */
                else
                {
                    u1l_lin_slv_sts = U1G_LIN_SLSTS_IDENTFIELD_WAIT;                    /* Ident Field待ち状態 */
                }
            }
            break;
        /*** Ident Field待ち状態 ***/
        case( U1G_LIN_SLSTS_IDENTFIELD_WAIT ):
            /* UARTエラーが発生した場合 */
            if( (u1a_lin_err & U1G_LIN_UART_ERR_ON) != U1G_LIN_BYTE_CLR )
            {
                xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;              /* Headerエラー */
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;       /* UARTエラー */

                l_vol_lin_set_synchbreak();                                             /* Synch Break受信待ち設定 */
            }
            /* UARTエラーなし */
            else
            {
                /* PARITYエラーが発生した場合 */
                u1a_lin_protid = u1g_lin_protid_tbl[ (u1a_lin_data & U1G_LIN_ID_PARITY_MASK) ];
                if( u1a_lin_data != u1a_lin_protid )
                {
                    xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerエラー */
                    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_pari = U2G_LIN_BIT_SET;   /* PARITYエラー */

                    l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
                }
                /* PARITYエラーなし */
                else
                {
                    /* ID,フレームモードを管理変数に格納 */
                    xnl_lin_id_sl.u1g_lin_id = (l_u8)( u1a_lin_data & U1G_LIN_ID_PARITY_MASK );   /* パリティを省く */
                    xnl_lin_id_sl.u1g_lin_slot = u1g_lin_id_tbl[ xnl_lin_id_sl.u1g_lin_id ];

                    /* SLEEPコマンドIDを受信した場合(必ずレスポンス受信動作となる) */
                    if( xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID )
                    {
                        /* フレームが[定義] かつ LINバッファのスロットが[未使用設定]の場合 */
                        if( (xnl_lin_id_sl.u1g_lin_slot != U1G_LIN_NO_FRAME)
                         && (xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_no_use == U2G_LIN_BIT_SET) )
                        {
                            xnl_lin_id_sl.u1g_lin_slot = U1G_LIN_NO_FRAME;              /* フレーム[未定義]に変更 */
                        }
                        u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                            /* Physical Busエラー検出カウンタクリア */

                        u2l_lin_chksum = U2G_LIN_WORD_CLR;                              /* チェックサム演算用変数の初期化 */
                        u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                              /* データ送信カウンタの初期化 */
                        u1l_lin_frm_sz = U1G_LIN_DL_8;                                  /* SLEEP時のフレームサイズは8固定 */
                        /* 応答タイムアウトタイマの起動 */
                        l_vog_lin_rcv_tm_set( (l_u8)((u1l_lin_frm_sz + U1G_LIN_1) * U1G_LIN_BYTE_LENGTH) );

                        u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;                   /* データ受信待ち状態 */
                    }
                    /* SLEEPコマンド以外の場合 */
                    else
                    {
                        /* フレームが[未定義] もしくは LINバッファのスロットが[未使用設定]の場合 */
                        if( (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME)
                         || (xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_no_use == U2G_LIN_BIT_SET) )
                        {

                            l_vol_lin_set_synchbreak();                                 /* Synch Break受信待ち設定 */
                        }
                        /* 処理対象フレームの場合 */
                        else
                        {
                            u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                        /* Physical Busエラー検出カウンタクリア */

                            /* 現在のフレームデータ長 */
                            u1l_lin_frm_sz = xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_frm_sz;

                            /*-- [受信フレーム時] --*/
                            if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_RCV )
                            {
                                u2l_lin_chksum = U2G_LIN_WORD_CLR;                      /* チェックサム演算用変数の初期化 */
                                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                      /* データ受信カウンタの初期化 */

                                /* 応答タイムアウトタイマの起動 */
                                l_vog_lin_rcv_tm_set( (l_u8)((u1l_lin_frm_sz + U1G_LIN_1) * U1G_LIN_BYTE_LENGTH) );

                                u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;           /* データ受信待ち状態 */
                            }
                            /*-- [送信フレーム時] --*/
                            else if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND )
                            {
                                l_vog_lin_frm_tm_stop();                                /* ヘッダタイムアウトタイマの停止 */
                                u2l_lin_chksum = U2G_LIN_WORD_CLR;                      /* チェックサム演算用変数の初期化 */
                                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                      /* データ送信カウンタの初期化 */

                                l_vog_lin_rx_dis();                                     /* 送信時は受信割り込み禁止 */

                                /* NM使用設定フレームの場合 */
                                if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_nm_use == U1G_LIN_NM_USE )
                                {
                                    /* LINフレームバッファのNM部分(データ1のbit4-7)をクリア */
                                    /* NM部分のクリア */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                     &= U1G_LIN_BUF_NM_CLR_MASK;
                                    /* LINフレームに レスポンス送信ステータスをセット */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                     |= ( u1g_lin_nm_info & U1G_LIN_NM_INFO_MASK );
                                }

                                /* LINバッファのデータを 送信用tmpバッファにコピー */
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

                                l_vog_lin_bit_tm_set( U1G_LIN_RSSP );                   /* レスポンススペース待ちタイマセット */

                                u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;           /* データ送信待ち状態 */
                            }
                            /*-- [送信でも受信でもない] --*/
                            else
                            {
                                /* 登録エラー */
                                l_vol_lin_set_synchbreak();                             /* Synch Break受信待ち設定 */
                            }
                        }
                    }
                }
            }
            break;
        /*** データ受信待ち状態 ***/
        case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            /* UARTエラー発生 */
            if( (u1a_lin_err & U1G_LIN_UART_ERR_ON) != U1G_LIN_BYTE_CLR )
            {
                /* SLEEPコマンドIDで、フレーム未定義の場合 */
                if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) )
                {
                    /* 何もせず Synch Break待ち状態へ */
                    l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
                }
                /* SLEEPコマンドIDではない、もしくはフレーム定義ありの場合 */
                else
                {
                    /* UARTエラー */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;

                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                       /* エラーありレスポンス完了 */

                    l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
                }
            }
            /* UARTエラーなし */
            else
            {
                /* データ部の受信割り込み処理 */
                if( u1l_lin_rs_cnt < u1l_lin_frm_sz )                                   /* DL + 1 */
                {
                    u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] = u1a_lin_data;                    /* データを tmp に格納 */
                    u1l_lin_rs_cnt++;
                    u2l_lin_chksum += (l_u16)u1a_lin_data;
                    u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );
                }
                /* チェックサム部の受信割り込み処理 */
                else
                {

                    u2l_lin_chksum = ~u2l_lin_chksum;                                   /* チェックサム反転 */

                    /* チェックサムエラー発生の場合 */
                    if( (l_u8)u2l_lin_chksum != u1a_lin_data )
                    {
                        /* SLEEPコマンドIDで、フレーム未定義の場合 */
                        if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) )
                        {
                            /* 何もせず Synch Break待ち状態へ */
                            l_vol_lin_set_synchbreak();                                 /* Synch Break受信待ち設定 */
                        }
                        /* SLEEPコマンドIDではない、もしくはフレーム定義ありの場合 */
                        else
                        {
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_sum = U2G_LIN_BIT_SET;

                            l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );               /* エラーありレスポンス完了 */

                            l_vol_lin_set_synchbreak();                                 /* Synch Break受信待ち設定 */
                        }
                    }
                    /* チェックサムエラーなし */
                    else
                    {
                        /* SLEEPコマンドID(=3Ch)の場合 */
                        if( xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID )
                        {
                            /* 1バイト目のデータが00hの場合 */
                            if( u1l_lin_rs_tmp[ U1G_LIN_0 ] == U1G_LIN_SLEEP_DATA )
                            {
                                /* SLEEPコマンドとして受信 */
                                l_vog_lin_frm_tm_stop();                                            /* タイマ停止 */
                                xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_SLEEP;    /* SLEEP状態に移行 */
                                l_vol_lin_set_wakeup();                                             /* Wakeup待機設定 */
                            }
                            /* 1バイト目のデータが00h以外の場合 */
                            else
                            {
                                /* フレーム定義ありの場合 */
                                if( xnl_lin_id_sl.u1g_lin_slot != U1G_LIN_NO_FRAME )
                                {
                                    /* 通常フレームとして受信 */
                                    /* LINバッファにデータを格納(フレームサイズ - 1) */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_0 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_1 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_2 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_3 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_4 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_5 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_6 ];
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ] =
                                     u1l_lin_rs_tmp[ U1G_LIN_7 ];

                                    /* LINバッファにチェックサムを格納 */
                                    xng_lin_frm_buf[xnl_lin_id_sl.u1g_lin_slot].un_state.st_bit.u2g_lin_chksum = (l_u16)(u1a_lin_data);

                                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );                  /* 転送成功 */
                                    l_vol_lin_set_synchbreak();                                     /* Synch Break受信待ち設定 */
                                }
                                /* フレーム未定義の場合 */
                                else
                                {
                                    /* 何もせずに Synch Break受信待ち状態にする */
                                    l_vol_lin_set_synchbreak();                                     /* Synch Break受信待ち設定 */
                                }
                            }
                        }
                        /* SLEEPコマンド以外のID の場合 */
                        else
                        {
                            /* LINバッファにデータを格納(フレームサイズ - 1) */
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_0 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_1 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_1 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_2 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_2 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_3 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_3 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_4 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_4 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_5 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_5 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_6 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_6 ];
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_7 ] =
                             u1l_lin_rs_tmp[ U1G_LIN_7 ];

                            /* LINバッファにチェックサムを格納 */
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_chksum = (l_u16)(u1a_lin_data);

                            l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );              /* 転送成功 */

                            l_vol_lin_set_synchbreak();                                 /* Synch Break受信待ち設定 */
                        }
                    }
                }
            }
            break;

        /*** データ送信後状態 (削除: タイマー割り込みで処理) ***/
        /* case( U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT ): は削除 */
        /*** 通常処理される事はない ***/
        default:
            l_vol_lin_set_synchbreak();                                                 /* Synch Break受信待ち設定 */
            break;
        }
    }
    /*** 上記ステータス以外(RESET, その他)の場合 ***/
    else
    {
        /* 通常は発生しない処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                                           /* システム異常(LINステータス) */
        l_vol_lin_set_synchbreak();                                                 /* Synch Break受信待ち設定 */
    }
}


/**************************************************/
/*  外部INT割り込み発生                           */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_irq_int(void)
{
    /*** SLEEP状態の場合 ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP )
    {
        /* RUN STANDBY状態へ移行 */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;
        l_ifc_init_drv_ch1();                                                   /* ドライバ部の初期化 */
    }
    /*** RUN STANDBY,RUN状態状態の場合 ***/
    else
    {
        l_vog_lin_int_dis();                                                    /* INT割り込みを禁止する */
    }
}


/**************************************************/
/*  タイマ割り込み発生                            */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_tm_int(void)
{
    l_u8  l_u1a_lin_read_back;

    /*** RUN STANDBY状態の場合 ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY ){
        /*** Synch Break(UART)待ち状態 ***/
        if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT ){
            /* Physical Busエラー */
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
            xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
        }
    }
    /*** RUN状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN ){
        /* スレーブタスクのステータスを確認 */
        switch( u1l_lin_slv_sts ){
        /*** Synch Break(UART)待ち状態 ***/
        case ( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
            /* Physical Busエラー */
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
            xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
            break;
        /*** Synch Break(IRQ)待ち状態 / Synch Field待ち状態 / Ident Field待ち状態 ***/
        case ( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
        case ( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
        case ( U1G_LIN_SLSTS_IDENTFIELD_WAIT ):
            /* header timeoutエラー */
            xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;                      /* Headerエラー */
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_time = U2G_LIN_BIT_SET;               /* Header Timeout */
            /* Physical Busエラーチェック */
            if(u2l_lin_herr_cnt >= U2G_LIN_HERR_LIMIT){
                /* Physical Busエラー */
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = U2G_LIN_BIT_SET;
                xng_lin_bus_sts.st_bit.u2g_lin_bus_err = U2G_LIN_BIT_SET;
                u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                                        /* Physical Busエラー検出カウンタクリア */
            }else{
                u2l_lin_herr_cnt++;                                                         /* Physical Busエラー検出カウンタ増加 */
            }
            l_vol_lin_set_synchbreak();                                                     /* Synch Break受信待ち設定 */
            break;
        /*** データ受信待ち状態 ***/
        case ( U1G_LIN_SLSTS_RCVDATA_WAIT ):
            /* SLEEPコマンドIDで、フレーム定義なしの場合 */
            if( (xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID) && (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME) ){
                /* 何もせず Synch Break待ち状態へ */
                l_vol_lin_set_synchbreak();                                                 /* Synch Break受信待ち設定 */
            }
            else{
                /* 1バイト目を受信済みかチェック (LIN 2.x Response Too Short検出) */
                if( u1l_lin_rs_cnt > U1G_LIN_0 ){
                    /* Response Too Short エラー (1バイト目受信後、残りのバイトが受信できなかった) */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_tooshort = U2G_LIN_BIT_SET;
                }
                else{
                    /* No Responseエラー (1バイト目すら受信できなかった) */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_nores = U2G_LIN_BIT_SET;
                }

                l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                               /* エラーありレスポンス完了 */
                l_vol_lin_set_synchbreak();                                                 /* Synch Break受信待ち設定 */
            }
            break;
        /*** データ送信待ち状態 ***/
        case ( U1G_LIN_SLSTS_SNDDATA_WAIT ):
            /* データ送信2バイト目以降 */
            if( u1l_lin_rs_cnt > U1G_LIN_0 ){
                l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );
                /* リードバック失敗(受信割り込みがかからない場合、もしくは受信バッファの内容が引数と異なる) */
                if( l_u1a_lin_read_back != U1G_LIN_OK ){
                    /* BITエラー */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                           /* エラーありレスポンス完了 */

                    l_vol_lin_set_synchbreak();                                             /* Synch Break受信待ち設定 */
                }
                /* リードバック成功 */
                else{
                    /* 最後まで送信 */
                    if( u1l_lin_rs_cnt > u1l_lin_frm_sz ){                                  /* DL + 1 */

                        l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );                      /* 転送成功 */

                        l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
                    }
                    /* まだ全てのデータを送信完了していない場合 */
                    else{
                        /* データの送信(チェックサムまで送信していない) */
                        if( u1l_lin_rs_cnt < u1l_lin_frm_sz ){
                            u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
                            u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );
                        }
                        /* チェックサムの送信 */
                        else{
                            /* 送信用tmpバッファにコピー */
                            u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = (l_u8)(~( (l_u8)u2l_lin_chksum ));
                            /* LINバッファにもコピー */
                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_chksum
                             = (l_u16)u1l_lin_rs_tmp[ u1l_lin_frm_sz ];
                        }

                        l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );              /* データ送信 */
                        l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );         /* データ10bit + Inter-Byte Space */
                        u1l_lin_rs_cnt++;
                    }
                }
            }
            /* データ送信1バイト目 */
            else{
                u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
                u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );

                l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );                      /* データ送信 */
                l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );                 /* データ10bit + Inter-Byte Space */
                u1l_lin_rs_cnt++;
            }
            break;
        /*** 通常処理される事はない ***/
        default:
            l_vol_lin_set_synchbreak();                                                     /* Synch Break受信待ち設定 */
            break;
        }

    }
    /*** SLEEP状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_SLEEP ){
        /* WAKEUPコマンド送信完了時のWakeup動作 */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN_STANDBY;              /* RUN STANDBY状態へ移行 */
        l_ifc_init_drv_ch1();                                                               /* ドライバ部の初期化 */
    }
    /*** RESET状態の場合(規定外の状態を含む) ***/
    else{
        /* 通常は発生しない処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                                               /* システム異常(LINステータス) */
    }
}


/**************************************************/
/*  Wakeup検出待ちへの設定処理                    */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
static void  l_vol_lin_set_wakeup(void)
{
  /* WAKEUP検出方法がUARTの場合 */
  #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
    l_vog_lin_int_dis();                                /* INT割り込みを禁止 */
    l_vog_lin_rx_enb();                                 /* 受信割り込み許可 */
  /* WAKEUP検出方法がINTの場合 */
  #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
    l_vog_lin_rx_dis();                                 /* 受信割り込み禁止 */
    l_vog_lin_int_enb_wakeup();                         /* INT割り込みを許可 */
/* Ver 2.00 変更:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
  #endif
}


/**************************************************/
/*  Synch Break受信待ち設定処理                   */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
static void  l_vol_lin_set_synchbreak(void)
{
    l_vog_lin_bus_tm_set();                                               /* Physical Bus Error検出タイマ起動 */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE, U1L_LIN_UART_HEADER_RXSIZE);   /* 受信割り込み許可 */
    l_vog_lin_int_enb();                                                  /* INT割り込みを許可 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT;                      /* Synch Break(UART)待ち状態に移行 */
}


/**************************************************/
/*  完了フラグの設定処理                          */
/*------------------------------------------------*/
/*  引数： err_stat : 完了状態                    */
/*         (0 / 1) : 正常完了 / 異常完了          */
/*  戻値： なし                                   */
/**************************************************/
static void  l_vol_lin_set_frm_complete(l_u8  u1a_lin_err)
{
    /* 転送成功、もしくは転送エラーのフラグが立っている場合(オーバーラン) */
    if( (xng_lin_bus_sts.u2g_lin_word & U2G_LIN_BUS_STS_CMP_SET) != U2G_LIN_WORD_CLR ){
        xng_lin_bus_sts.st_bit.u2g_lin_ovr_run = U2G_LIN_BIT_SET;               /* オーバーラン */
    }
    /* 異常完了の場合 */
    if ( u1a_lin_err == U1G_LIN_ERR_ON ) {
        xng_lin_bus_sts.st_bit.u2g_lin_err_resp = U2G_LIN_BIT_SET;              /* エラー有りレスポンス */
    }
    /* 正常完了の場合 */
    else {
        xng_lin_bus_sts.st_bit.u2g_lin_ok_resp = U2G_LIN_BIT_SET;               /* 正常完了 */
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_err.u2g_lin_err = U2G_LIN_BYTE_CLR;  /* エラーフラグのクリア */
/* Ver 2.00 変更:無効にしたフレームが無効にならない場合がある。 */
    }

    /* 保護IDのセット */
    xng_lin_bus_sts.st_bit.u2g_lin_last_id = (l_u16)u1g_lin_protid_tbl[ xnl_lin_id_sl.u1g_lin_id ];

    /* 送信フレームの場合、LIN_Managerへコールバック通知 */
    /* 継続フレーム送信時に送信完了フラグで誤検知している可能性があり、 */
    /* 期待するフレームが送信されたかを確認するため、LIN通信管理へのコールバックを実行する */
    if( (xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND) &&
        (u1a_lin_err == U1G_LIN_ERR_OFF) ) {
        /* 送信成功時のみコールバック */
        f_LIN_Manager_Callback_TxComplete( xnl_lin_id_sl.u1g_lin_slot, u1l_lin_frm_sz, u1l_lin_rs_tmp );
    }

    /* 送受信処理完了フラグのセット */
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
        /* 通常発生しないので処理なし */
    }
}


/**************************************************/
/*  NM情報データのセット 処理                     */
/*------------------------------------------------*/
/*  引数： NM情報データ                           */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_set_nm_info( l_u8  u1a_lin_nm_info )
{
    u1g_lin_nm_info = u1a_lin_nm_info;
}


/**************************************************/
/*  テーブルのチェック 処理                       */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8  l_u1g_lin_tbl_chk(void)
{
    l_u8  u1a_lin_result;
    l_u8  u1a_lin_lp1;

    u1a_lin_result = U1G_LIN_OK;

    for( u1a_lin_lp1 = U1G_LIN_0; u1a_lin_lp1 < U1G_LIN_MAX_SLOT_NUM; u1a_lin_lp1++ )
    {
        /* ID情報(0x00 - スロット最大値, FFh) */
        if( U1G_LIN_MAX_SLOT <= u1g_lin_id_tbl[ u1a_lin_lp1 ] ){
            /* フレーム未定義(=FFh)以外の場合 */
            if( u1g_lin_id_tbl[ u1a_lin_lp1 ] != U1G_LIN_NO_FRAME )
            {
                u1a_lin_result = U1G_LIN_NG;
            }
        }
    }
    for( u1a_lin_lp1 = U1G_LIN_0; u1a_lin_lp1 < U1G_LIN_MAX_SLOT; u1a_lin_lp1++ )
    {
        /* フレームサイズ(1 - 8) */
        if( (xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_frm_sz < U1G_LIN_DL_1)
         || (U1G_LIN_DL_8 < xng_lin_slot_tbl[ u1a_lin_lp1 ].u1g_lin_frm_sz) )
        {
            u1a_lin_result = U1G_LIN_NG;
        }
    }

    return( u1a_lin_result );
}


/**************************************************/
/*  ヘッダ受信 処理                               */
/*------------------------------------------------*/
/*  引数： 受信データ                             */
/*      ： LINエラーステータス                    */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8 l_vog_lin_check_header( l_u8 u1a_lin_data[U1L_LIN_UART_HEADER_RXSIZE] ,l_u8 u1a_lin_err )
{
    l_u8  u1a_lin_protid;
    l_u8  u1a_lin_result;
    l_vog_lin_int_dis(); /* 暫定処理：今後LINVer.2に準拠させる際に削除予定 */
    /* UARTエラーが発生した場合 */
    if( (u1a_lin_err & U1G_LIN_UART_ERR_ON) != U1G_LIN_BYTE_CLR )
    {
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;       /* UARTエラー */
        xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;              /* Headerエラー */
        u1a_lin_result = U1G_LIN_NG;
        l_vol_lin_set_synchbreak();                                             /* Synch Break受信待ち設定 */
    }
    else
    {
        /* 受信データが00h以外の場合 */
        if( u1a_lin_data[U1G_LIN_HEADER_SYNCHBREAK] != U1G_LIN_SYNCH_BREAK_DATA )
        {
            /* エラーフラグのリセット */
                xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerエラー */
                u1a_lin_result = U1G_LIN_NG;
                l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */ 
        }
        /* 受信データが00hならば、SynchBreak受信 */
        else
        {
            /* 受信データが55h以外の場合 */
            if( u1a_lin_data[U1G_LIN_HEADER_SYNCHFIELD] != U1G_LIN_SYNCH_FIELD_DATA )
            {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_synch = U2G_LIN_BIT_SET;  /* Synch Fieldエラー */
                xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerエラー */
                u1a_lin_result = U1G_LIN_NG;
                l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
            }
            /* 正常Synch Field受信 */
            else
            {
                /* PARITYエラーが発生した場合 */
                u1a_lin_protid = u1g_lin_protid_tbl[ (u1a_lin_data[U1G_LIN_HEADER_PID] & U1G_LIN_ID_PARITY_MASK) ];
                if( u1a_lin_data[U1G_LIN_HEADER_PID] != u1a_lin_protid )
                {
                    xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;          /* Headerエラー */
                    xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_pari = U2G_LIN_BIT_SET;   /* PARITYエラー */
                    u1a_lin_result = U1G_LIN_NG;
                    l_vol_lin_set_synchbreak();                                         /* Synch Break受信待ち設定 */
                }
                /* PARITYエラーなし */
                else
                {
                    /* ID,フレームモードを管理変数に格納 */
                    xnl_lin_id_sl.u1g_lin_id = (l_u8)( u1a_lin_data[U1G_LIN_HEADER_PID] & U1G_LIN_ID_PARITY_MASK );   /* パリティを省く */
                    xnl_lin_id_sl.u1g_lin_slot = u1g_lin_id_tbl[ xnl_lin_id_sl.u1g_lin_id ];

                    /* SLEEPコマンドIDを受信した場合(必ずレスポンス受信動作となる) */
                    if( xnl_lin_id_sl.u1g_lin_id == U1G_LIN_SLEEP_ID ){
                        /* フレームが[定義] かつ LINバッファのスロットが[未使用設定]の場合 */
                        if( (xnl_lin_id_sl.u1g_lin_slot != U1G_LIN_NO_FRAME)
                         && (xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_no_use == U2G_LIN_BIT_SET) )
                        {
                            xnl_lin_id_sl.u1g_lin_slot = U1G_LIN_NO_FRAME;              /* フレーム[未定義]に変更 */
                        }
                        u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                            /* Physical Busエラー検出カウンタクリア */

                        u1l_lin_chksum = U1G_LIN_BYTE_CLR;                              /* チェックサム演算用変数の初期化 */
                        u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                              /* データ送信カウンタの初期化 */
                        u1l_lin_frm_sz = U1G_LIN_DL_8;                                  /* SLEEP時のフレームサイズは8固定 */

                        u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;                   /* データ受信待ち状態 */
                        l_vog_lin_rx_enb( U1G_LIN_FLUSH_RX_NO_USE,u1l_lin_frm_sz + U1G_LIN_1 );
                    }
                    /* SLEEPコマンド以外の場合 */
                    else
                    {
                        /* フレームが[未定義] もしくは LINバッファのスロットが[未使用設定]の場合 */
                        if( (xnl_lin_id_sl.u1g_lin_slot == U1G_LIN_NO_FRAME)
                         || (xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_no_use == U2G_LIN_BIT_SET) )
                        {
                            l_vol_lin_set_synchbreak();                                 /* Synch Break受信待ち設定 */
                        }
                        /* 処理対象フレームの場合 */
                        else
                        {
                            u2l_lin_herr_cnt = U2G_LIN_WORD_CLR;                        /* Physical Busエラー検出カウンタクリア */

                            /* 現在のフレームデータ長 */
                            u1l_lin_frm_sz = xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_frm_sz;

                            /*-- [受信フレーム時] --*/
                            if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_RCV )
                            {
                                u1l_lin_chksum = U1G_LIN_BYTE_CLR;                      /* チェックサム演算用変数の初期化 */
                                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                      /* データ受信カウンタの初期化 */
                                u1l_lin_slv_sts = U1G_LIN_SLSTS_RCVDATA_WAIT;           /* データ受信待ち状態 */
                                u1a_lin_result = U1G_LIN_OK;
                                l_vog_lin_rx_enb( U1G_LIN_FLUSH_RX_NO_USE,u1l_lin_frm_sz + U1G_LIN_1 );
                            }
                            /*-- [送信フレーム時] --*/
                            else if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND )
                            {
                                u2l_lin_chksum = U2G_LIN_WORD_CLR;                      /* チェックサム演算用変数の初期化 */
                                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;                      /* データ送信カウンタの初期化 */

                                l_vog_lin_rx_dis();                                     /* 送信時は受信割り込み禁止 */

                                /* NM使用設定フレームの場合 */
                                if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_nm_use == U1G_LIN_NM_USE )
                                {
                                    /* LINフレームバッファのNM部分(データ1のbit4-7)をクリア */
                                    /* NM部分のクリア */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                     &= U1G_LIN_BUF_NM_CLR_MASK;
                                    /* LINフレームに レスポンス送信ステータスをセット */
                                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                                     |= ( u1g_lin_nm_info & U1G_LIN_NM_INFO_MASK );
                                }

                                /* LINバッファのデータを 送信用tmpバッファにコピー */
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

                                /* レスポンススペース待ちタイマ設定 (H850互換) */
                                l_vog_lin_bit_tm_set( U1G_LIN_RSSP );

                                /* データ送信待ち状態へ遷移 */
                                u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;
                                u1a_lin_result = U1G_LIN_OK;
                            }
                            /*-- [送信でも受信でもない] --*/
                            else
                            {
                                /* 登録エラー */
                                l_vol_lin_set_synchbreak();                             /* Synch Break受信待ち設定 */
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