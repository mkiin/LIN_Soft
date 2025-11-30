/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_core_rl78f24.c                       */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/
/* LINライブラリ ROMセクション定義 */
#pragma section  text   @@LNCD
#pragma section  text  @@LNCDL
#pragma section  const   @@LNCNS
#pragma section  const  @@LNCNSL
/* LINライブラリ RAMセクション定義 */
#pragma section  bss   @@LNDT
#pragma section  bss  @@LNDTL

/***** ヘッダ インクルード *****/
#include "l_slin_user.h"
#include "l_slin_core_rl78f24.h"
#include "l_slin_drv_rl78f24.h"

/***** 関数プロトタイプ宣言 *****/
/*-- 関数定義(static) --*/
static void   l_vol_lin_set_frm_complete(l_u8  u1a_lin_err);
static void   l_vol_lin_sub_hdr_rcv(l_u8 u1a_lin_id);

/*** 変数定義(static) ***/
static st_lin_bus_status_type  xnl_lin_bus_sts;        /* LIN Bus ステータス */
static l_u8   u1l_lin_nm_info;                         /* NM 情報テーブル */
static st_lin_id_slot_type  xnl_lin_id_sl;
static l_u16  u2l_lin_bus_inactive_cnt;                /* Bus inactive検出用カウンタ */
static l_u8   u1l_lin_hdr_rcved;                       /* ヘッダ受信/未受信 */
static l_u8   u1l_lin_wup_det;                         /* ウェイクアップ検出フラグ */
static l_u8   u1l_lin_lst_d1rc_state;                  /* データ1受信フラグの状態(LIN/UART ステータス・レジスタ LSTn.D1RC ) */


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
void  l_ifc_init(void)
{
    xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_RESET;                    /* RESET状態にする */

    u1g_lin_syserr = U1G_LIN_SYSERR_NON;                                /* システム異常フラグの初期化 */

    l_ifc_init_drv();                                                   /* LINドライバの初期化(API) */
    l_ifc_init_com();                                                   /* LINバッファの初期化(API) */

    xnl_lin_bus_sts.u1g_lin_err_resp = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_ok_resp = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_ovr_run = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_goto_sleep = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_bus_err = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_head_err = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_last_id = U1G_LIN_BYTE_CLR;

    u1l_lin_nm_info             = U1G_LIN_BYTE_CLR;                     /* NM情報の初期化 */
    xnl_lin_id_sl.u1g_lin_id    = U1G_LIN_BYTE_CLR;                     /* 処理するフレームのIDの初期化 */
    xnl_lin_id_sl.u1g_lin_slot  = U1G_LIN_BYTE_CLR;                     /* 処理するフレームのスロット番号の初期化 */
    u2l_lin_bus_inactive_cnt    = U2G_LIN_WORD_CLR;                     /* Bus inactive検出用カウンタの初期化 */

    u1l_lin_hdr_rcved           = U1G_LIN_HDR_NOT_RCVED;                /* ヘッダ未受信 */
    u1l_lin_wup_det             = U1G_LIN_WUP_NOT_DET;                  /* ウェイクアップ未検出 */
    u1l_lin_lst_d1rc_state      = U1G_LIN_LST_D1RC_INCOMPLETE;          /* データ1受信完了の状態をクリアする (LIN/UART ステータス・レジスタ LSTn.D1RC) */

}


/**************************************************/
/*  LINバッファの初期化 処理(API)                 */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_init_com(void)
{
    l_u8    u1a_lin_lp1;

  /*--- LINステータスバッファ初期化 ---*/
    /* NADのセット */
    xng_lin_sts_buf.u1g_lin_nad = U1G_LIN_NAD;

    /* Bus inactiveフラグのクリア */
    xng_lin_sts_buf.u1g_lin_bus_inactive = U1G_LIN_BIT_CLR;

    /* ヘッダーエラーフラグのクリア */
    xng_lin_sts_buf.u1g_lin_e_uart = U1G_LIN_BYTE_CLR;
    xng_lin_sts_buf.u1g_lin_e_sync = U1G_LIN_BYTE_CLR;
    xng_lin_sts_buf.u1g_lin_e_pari = U1G_LIN_BYTE_CLR;

    /* 送受信処理完了フラグのクリア */
    xng_lin_sts_buf.un_rs_flg1.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg2.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg3.u2g_lin_word = U2G_LIN_WORD_CLR;
    xng_lin_sts_buf.un_rs_flg4.u2g_lin_word = U2G_LIN_WORD_CLR;

  /*--- LINフレームバッファ初期化 ---*/
    for( u1a_lin_lp1 = U1G_LIN_0; U1G_LIN_MAX_SLOT > u1a_lin_lp1; u1a_lin_lp1++ ) {

        l_vod_lin_DI();                                                /* 割り込み禁止設定 */

        /* ステータス部分(エラーフラグ等)クリア */
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_e_nores = U1G_LIN_BYTE_CLR;
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_e_uart = U1G_LIN_BYTE_CLR;
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_e_bit = U1G_LIN_BYTE_CLR;
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_e_sum = U1G_LIN_BYTE_CLR;
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_e_res_sht = U1G_LIN_BYTE_CLR;
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_status = U1G_LIN_BYTE_CLR;
        xng_lin_frm_buf[ u1a_lin_lp1 ].u1g_lin_no_use = U1G_LIN_BYTE_CLR;

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

        l_vod_lin_EI();                                                /* 割り込み許可     */

    }
}


/**************************************************/
/*  LINドライバの初期化 処理(API)                 */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_init_drv(void)
{
    l_u8    u1a_lin_result_tmp;

    /*** LINモジュールの初期化 ***/
    u1a_lin_result_tmp = l_u1g_lin_linmodule_init();                    /* LINモジュールの初期化 */

    /*** TAUモジュールの初期化 ***/
    l_vog_lin_timer_init();                                             /* TAUモジュールの初期化処理 */

    /*** INTの初期化 ***/
    l_vog_lin_int_init();                                               /* INT割り込み初期化 */

    if( U1G_LIN_OK != u1a_lin_result_tmp ) {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                         /* システム異常(MCUステータス) */
    }
    else{
        switch( xng_lin_sts_buf.u1g_lin_sts ) {
        /* SLEEP状態 */
        case ( U1G_LIN_STS_SLEEP ):
            l_vod_lin_DI();                                             /* 割り込み禁止設定 */

            l_vog_lin_int_enb();                                        /* ウェイクアップ検出許可 */
            u1a_lin_result_tmp  = l_u1g_lin_set_reset();                /* LINモジュールRESETモード設定 */
            if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                 /* システム異常(MCUステータス) */
            }
            l_hook_sleep();                                             /* Sleep時Hook処理 */

            l_vod_lin_EI();                                             /* 割り込み許可     */
            break;
        /* RUN状態 / RUN STANDBY状態 */
        case ( U1G_LIN_STS_RUN_STANDBY ):
        case ( U1G_LIN_STS_RUN ):
            xnl_lin_bus_sts.u1g_lin_err_resp = U1G_LIN_BYTE_CLR;
            xnl_lin_bus_sts.u1g_lin_ok_resp = U1G_LIN_BYTE_CLR;
            xnl_lin_bus_sts.u1g_lin_ovr_run = U1G_LIN_BYTE_CLR;
            xnl_lin_bus_sts.u1g_lin_goto_sleep = U1G_LIN_BYTE_CLR;
            xnl_lin_bus_sts.u1g_lin_bus_err = U1G_LIN_BYTE_CLR;
            xnl_lin_bus_sts.u1g_lin_head_err = U1G_LIN_BYTE_CLR;
            xnl_lin_bus_sts.u1g_lin_last_id = U1G_LIN_BYTE_CLR;
            u1l_lin_hdr_rcved             = U1G_LIN_HDR_NOT_RCVED;      /* ヘッダ未受信に戻す */
            u2l_lin_bus_inactive_cnt      = U2G_LIN_WORD_CLR;           /* Bus inactive監視カウンタのクリア  */
            u1l_lin_lst_d1rc_state        = U1G_LIN_LST_D1RC_INCOMPLETE; /* データ1受信完了の状態をクリアする (LIN/UART ステータス・レジスタ LSTn.D1RC) */

            u1a_lin_result_tmp = l_u1g_lin_set_run();                   /* LINモジュール通信可能状態設定 */
            if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                 /* システム異常(MCUステータス) */
            }

            break;
        /* 上記の状態以外の場合 */
        default:
            /* 何もしない */
            break;
        }
    }
}


/**************************************************/
/*  バスエラー監視 処理(API)                      */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_bus_err_tick(void)
{
    l_vod_lin_DI();                                                     /* 割り込み禁止設定 */

    /* RUN / RUN STANDBY状態の場合 */
    if( (U1G_LIN_STS_RUN == xng_lin_sts_buf.u1g_lin_sts)
     || (U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts) ) {
        /* Bus inactive検出フラグがセットされていない場合 */
        if( U1G_LIN_BIT_SET != xng_lin_sts_buf.u1g_lin_bus_inactive ) {
            u2l_lin_bus_inactive_cnt += U2G_LIN_TIME_BASE;              /* Bus inactive検出用カウンタをカウントアップ */

            /* カウントが超えた場合(Bus inactive) */
            if( U2G_LIN_BUS_TIMEOUT <= u2l_lin_bus_inactive_cnt ) {
                xng_lin_sts_buf.u1g_lin_bus_inactive                = U1G_LIN_BIT_SET;
                xnl_lin_bus_sts.u1g_lin_bus_err                     = U1G_LIN_BIT_SET;
                u2l_lin_bus_inactive_cnt                            = U2G_LIN_WORD_CLR; /* Bus inactive検出用カウンタ クリア */
            }
        }
        /* Bus inactive検出フラグがセットされている場合 */
        else{
            u2l_lin_bus_inactive_cnt = U2G_LIN_WORD_CLR;                /* Bus inactive検出用カウンタ クリア */
        }
    }

    l_vod_lin_EI();                                                     /* 割り込み許可     */
}


/**************************************************/
/*  LIN通信接続 処理(API)                         */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_connect(void)
{
    /*** オート変数 ***/
    l_u8    u1a_lin_result_tmp;

    l_vod_lin_DI();                                             /* 割り込み禁止設定 */

    if( U1G_LIN_STS_RESET == xng_lin_sts_buf.u1g_lin_sts ) {
        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_SLEEP;            /* SLEEP状態へ移行 */

        l_vog_lin_int_enb();                                                    /* ウェイクアップ検出許可 */
        u1a_lin_result_tmp  = l_u1g_lin_set_reset();                            /* LINモジュールRESETモード設定 */
        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                             /* システム異常(MCUステータス) */
        }
        else{
            /* 処理なし */
        }
        l_hook_sleep();                                             /* Sleep時Hook処理 */
    }
    /* RESET状態以外の場合 */
    else {
        /* 処理なし */
    }

    l_vod_lin_EI();                                             /* 割り込み許可     */
}


/**************************************************/
/*  LIN切断 処理(API)                             */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_bool  l_ifc_disconnect(void)
{
    /*** オート変数 ***/
    l_u8    u1a_lin_result_tmp;
    l_bool  u2a_lin_result;

    l_vod_lin_DI();                                             /* 割り込み禁止設定 */

    /* SLEEP状態の場合 */
    if( U1G_LIN_STS_SLEEP == xng_lin_sts_buf.u1g_lin_sts ) {
        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_RESET;
        l_vog_lin_int_dis();                                    /* INT割り込み禁止 */
        u1a_lin_result_tmp = l_u1g_lin_set_reset();             /* LINモジュールリセット状態設定 */

        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;             /* システム異常(MCUステータス) */
            u2a_lin_result = U2G_LIN_NG;
        }
        else{
            u2a_lin_result = U2G_LIN_OK;
        }
    }
    /* RESET状態の場合 */
    else if( U1G_LIN_STS_RESET == xng_lin_sts_buf.u1g_lin_sts ) {
        u2a_lin_result = U2G_LIN_OK;
    }
    /* SLEEPでもRESETでもない場合 */
    else{
        u2a_lin_result = U2G_LIN_NG;
    }

    l_vod_lin_EI();                                             /* 割り込み許可     */

    return( u2a_lin_result );
}


/**************************************************/
/*  wakeupコマンド送信 処理(API)                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_wake_up(void)
{
    l_u8    u1a_lin_result_tmp;

    l_vod_lin_DI();                                         /* 割り込み禁止設定 */

    if( (U1G_LIN_STS_SLEEP == xng_lin_sts_buf.u1g_lin_sts)
     || (U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts) ) {
        u1a_lin_result_tmp = l_u1g_lin_set_sndwkup();       /* LINモジュールWakeup送信設定 */

        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;         /* システム異常(MCUステータス) */
        }
        else{
            l_vog_lin_int_dis();                            /* INT割り込み禁止 */

            /* wakeupパルスを送信 */
            u1a_lin_result_tmp = l_u1g_lin_snd_wakeup();

            if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;     /* システム異常(MCUステータス) */
            }
        }
    }
    /* SLEEP / RUN STANDBY状態以外の場合 なにもしない */
    l_vod_lin_EI();                                         /* 割り込み許可     */
}


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/**************************************************/
/*  LINバス状態リード 処理(API)                   */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： LINバス状態                            */
/**************************************************/
l_u16  l_ifc_read_status(void)
{
    l_u16  u2a_lin_result;

    l_vod_lin_DI();                                         /* 割り込み禁止設定 */

    u2a_lin_result = xnl_lin_bus_sts.u1g_lin_err_resp;
	u2a_lin_result |= (l_u16)xnl_lin_bus_sts.u1g_lin_ok_resp << U1G_LIN_BUS_STS_OK_RESP;
	u2a_lin_result |= (l_u16)xnl_lin_bus_sts.u1g_lin_ovr_run << U1G_LIN_BUS_STS_OVR_RUN;
    u2a_lin_result |= (l_u16)xnl_lin_bus_sts.u1g_lin_goto_sleep << U1G_LIN_BUS_STS_GOTO_SLEEP;
    u2a_lin_result |= (l_u16)xnl_lin_bus_sts.u1g_lin_bus_err << U1G_LIN_BUS_STS_BUS_ERR;
    u2a_lin_result |= (l_u16)xnl_lin_bus_sts.u1g_lin_head_err << U1G_LIN_BUS_STS_HEAD_ERR;
    u2a_lin_result |= (l_u16)xnl_lin_bus_sts.u1g_lin_last_id << U1G_LIN_BUS_STS_LAST_ID;

    xnl_lin_bus_sts.u1g_lin_head_err = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_bus_err = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_goto_sleep = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_ovr_run = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_ok_resp = U1G_LIN_BYTE_CLR;
    xnl_lin_bus_sts.u1g_lin_err_resp = U1G_LIN_BYTE_CLR;

    l_vod_lin_EI();                                         /* 割り込み許可     */

    return( u2a_lin_result );
}
#endif


/**************************************************/
/*  SLEEP状態移行 処理(API)                       */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_sleep(void)
{
    l_u8    u1a_lin_result_tmp;

    l_vod_lin_DI();                                                         /* 割り込み禁止設定 */

    /* RUN / RUN STANDBY状態の場合 */
    if( (U1G_LIN_STS_RUN == xng_lin_sts_buf.u1g_lin_sts)
     || (U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts) ) {

        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_SLEEP;                    /* SLEEP状態に移行 */
        l_vog_lin_int_enb();                                                /* ウェイクアップ検出許可 */
        u1a_lin_result_tmp  = l_u1g_lin_set_reset();                        /* LINモジュールRESETモード設定 */

        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                         /* システム異常(MCUステータス) */
        }
        l_hook_sleep();                                                     /* Sleep時Hook処理 */
    }
    /* RUN / RUN STANDBY以外の場合 なにもしない */

    l_vod_lin_EI();                                                         /* 割り込み許可     */
}


/**************************************************/
/*  LINステータスのリード 処理(API)               */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： LINステータス                          */
/**************************************************/
l_u16  l_ifc_read_lb_status(void)
{
    l_u16  u2a_lin_result;

    l_vod_lin_DI();                                                         /* 割り込み禁止設定 */

    /* 現在のLINステータスを格納 */
    u2a_lin_result = (l_u16)xng_lin_sts_buf.u1g_lin_sts;

    l_vod_lin_EI();                                                         /* 割り込み許可     */

    return( u2a_lin_result );
}


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/**************************************************/
/*  スロット無効フラグクリア処理(API)             */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm: フレーム名                */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_enable(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( U1G_LIN_MAX_SLOT > u1a_lin_frm ) {

        l_vod_lin_DI();                                                         /* 割り込み禁止設定 */

        /* LINバッファ スロット有効 */
        xng_lin_frm_buf[ u1a_lin_frm ].u1g_lin_no_use = U1G_LIN_BIT_CLR;

        l_vod_lin_EI();                                                         /* 割り込み許可     */
    }
    /* 引数値が範囲外の場合 なにもしない */
}
#endif


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/**************************************************/
/*  スロット無効フラグセット処理(API)             */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm: フレーム名                */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_disable(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( U1G_LIN_MAX_SLOT > u1a_lin_frm ) {

        l_vod_lin_DI();                                                         /* 割り込み禁止設定 */

        /* LINバッファ スロット無効 */
        xng_lin_frm_buf[ u1a_lin_frm ].u1g_lin_no_use = U1G_LIN_BIT_SET;

        l_vod_lin_EI();                                                         /* 割り込み許可     */
    }
    /* 引数値が範囲外の場合 なにもしない */
}
#endif


/**************************************************/
/*  バッファ スロット読み出し処理(API)            */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm  : 読み出すフレーム名      */
/*         u1a_lin_cnt  : 読み出しデータ長        */
/*         pta_lin_data : データ格納先ポインタ    */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_rd(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, l_u8 pta_lin_data[])
{
    /* ポインタ(引数値)がNULL以外の場合 */
    if( U1G_LIN_NULL != pta_lin_data ) {
        /* 引数値が範囲内の場合 */
        if( U1G_LIN_MAX_SLOT > u1a_lin_frm ) {

            l_vod_lin_DI();                                         /* 割り込み禁止設定 */

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

            l_vod_lin_EI();                                         /* 割り込み許可     */

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
void  l_slot_wr(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, const l_u8 pta_lin_data[])
{
    /* ポインタ(引数値)がNULL以外の場合 */
    if( U1G_LIN_NULL != pta_lin_data ) {
        /* 引数値が範囲内の場合 */
        if( U1G_LIN_MAX_SLOT > u1a_lin_frm ) {

            l_vod_lin_DI();                                         /* 割り込み禁止設定 */

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

            l_vod_lin_EI();                                         /* 割り込み許可     */

        }
        /* 引数値が範囲外の場合 何もしない */
    }
    /* ポインタ(引数値)がNULLの場合 何もしない */
}


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/**************************************************/
/*  バッファスロット初期値設定処理(API)           */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm: フレーム名                */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_set_default(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( U1G_LIN_MAX_SLOT > u1a_lin_frm ) {

    	l_vod_lin_DI();                                         /* 割り込み禁止設定 */

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

    	l_vod_lin_EI();                                         /* 割り込み許可     */

    }
    /* 引数値が範囲外の場合 なにもしない */
}
#endif


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/**************************************************/
/*  バッファスロットFAIL値設定処理(API)           */
/*------------------------------------------------*/
/*  引数： u1a_lin_frm: フレーム名                */
/*  戻値： なし                                   */
/**************************************************/
void  l_slot_set_fail(l_frame_handle  u1a_lin_frm)
{
    /* 引数値が範囲内の場合 */
    if( U1G_LIN_MAX_SLOT > u1a_lin_frm ) {

        l_vod_lin_DI();                                         /* 割り込み禁止設定 */

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

        l_vod_lin_EI();                                         /* 割り込み許可     */

    }
    /* 引数値が範囲外の場合 なにもしない */
}
#endif


/**************************************************/
/*  RUN状態移行 処理(API)                         */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_run(void)
{
    l_vod_lin_DI();                                                             /* 割り込み禁止設定 */

    /* SLEEP状態の場合 */
    if( U1G_LIN_STS_SLEEP == xng_lin_sts_buf.u1g_lin_sts ) {
        /* RUN STANDBY状態へ移行 */
        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_RUN_STANDBY;
        l_ifc_init_drv();                                                       /* ドライバ部の初期化 */
        l_vog_lin_int_dis();                                                    /* INT割り込み禁止 */
    }
    /* SLEEP以外の場合 なにもしない */

    l_vod_lin_EI();                                                             /* 割り込み許可     */
}


/**************************************************/
/*  受信割り込み発生                              */
/*------------------------------------------------*/
/*  引数： u1a_lin_data : LINステータス情報       */
/*         u1a_lin_id   : 受信したID              */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_id)
{
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
    l_u8    u1a_lin_is_valid;
#endif
    l_u8    u1a_lin_result_tmp;
    l_u8    u1a_lin_rs_tmp[U1G_LIN_DL_8];
    
    /** RUN_STANDBY状態 **/
    if( U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts ) {
        if( U1G_LIN_BYTE_CLR != (u1a_lin_data & U1G_LIN_HDR_RCVED_MASK) ) {
            /* エラーを確定する */
            l_vog_lin_determine_ResTooShort_or_NoRes(u1l_lin_lst_d1rc_state);       /* Response too short / No response を確定する */
            u1l_lin_lst_d1rc_state = U1G_LIN_LST_D1RC_INCOMPLETE;                   /* データ1受信完了の内部状態をクリアする */

#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
            u1a_lin_is_valid = l_u1g_lin_is_valid_baud_rate();                      /* ヘッダ受信時のボー・レート妥当性判断処理 */
            /* 検出されたボー・レートが指定されたボー・レートの範囲に収まっているか */
            if( U1G_LIN_OK == u1a_lin_is_valid ) {
#endif
                /* ヘッダの受信完了 */
                xng_lin_sts_buf.u1g_lin_sts    = U1G_LIN_STS_RUN;                   /* RUN状態へ移行 */
#if U1G_LIN_ERRCLR_PRE_RUN == U1G_LIN_ERRCLR_ON
                xnl_lin_bus_sts.u1g_lin_head_err               = U1G_LIN_BIT_CLR;   /* LINバスステータス Headerエラークリア */
                xng_lin_sts_buf.u1g_lin_e_uart = U1G_LIN_BYTE_CLR;
                xng_lin_sts_buf.u1g_lin_e_sync = U1G_LIN_BYTE_CLR;
                xng_lin_sts_buf.u1g_lin_e_pari = U1G_LIN_BYTE_CLR;
#endif
                l_vog_lin_int_dis();                                                /* INT割り込み禁止 */
                l_vol_lin_sub_hdr_rcv( u1a_lin_id );                                /* ヘッダ受信時処理 */
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
            }
            /* 検出されたボー・レートが指定されたボー・レートの範囲に収まっているない場合 */
            else {
                u1a_lin_result_tmp = l_u1g_lin_resp_noreq();                        /* レスポンスなし要求 */
                if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                    u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                         /* システム異常(MCUステータス) */
                }
                xnl_lin_bus_sts.u1g_lin_head_err                = U1G_LIN_BIT_SET;  /* Headerエラー */
                xng_lin_sts_buf.u1g_lin_e_sync                  = U1G_LIN_BIT_SET;  /* Synch Fieldエラー */
                l_hook_err(U1G_LIN_ID_DUMMY, U2G_LIN_ERRINFO_HDR_SYNC);             /* エラー時フック処理 */
            }
#endif
        }
    }
    /** RUN状態 **/
    else if( U1G_LIN_STS_RUN == xng_lin_sts_buf.u1g_lin_sts ) {
        if( U1G_LIN_BYTE_CLR != (u1a_lin_data & U1G_LIN_RESP_RCVED_MASK) ) {
            /* 受信完了 */
            /* SLEEPコマンドID(=3Ch)の場合 */
            if( U1G_LIN_SLEEP_ID == xnl_lin_id_sl.u1g_lin_id ) {
                /* フレーム[定義あり]の場合 */
                if( U1G_LIN_NO_FRAME != xnl_lin_id_sl.u1g_lin_slot ) {
                    /* LINバッファにデータを格納(取得は8byte固定) */
                    l_vog_lin_get_rcvdata( xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte );

                    /* 1バイト目のデータが00hの場合 */
                    if( U1G_LIN_SLEEP_DATA == xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ] ) {
                        /* SLEEPコマンドとして受信 */
                        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_SLEEP;                    /* SLEEP状態に移行 */
                        xnl_lin_bus_sts.u1g_lin_goto_sleep = U1G_LIN_BIT_SET;               /*GoToSleepコマンドフラグセット*/
                        l_vog_lin_int_enb();                                                /* ウェイクアップ検出許可 */

                        /* 送受信ステータスをreadyに設定する（フレーム定義ありの場合） */
                        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_status =  U1G_LIN_BIT_CLR;

                        u1a_lin_result_tmp = l_u1g_lin_set_reset();                         /* LINモジュールRESETモード設定 */
                        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                         /* システム異常(MCUステータス) */
                        }
                        l_hook_sleep();                                                     /* Sleep時Hook処理 */
                    }
                    /* 1バイト目のデータが00h以外の場合 */
                    else{
                        /* 通常フレームとして受信 */
                        l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );              /* 転送成功 */
                        l_hook_rcv_comp(xnl_lin_id_sl.u1g_lin_slot);                /* 受信完了時フック処理 */
                    }
                }
                /* フレーム[未定義]の場合 */
                else{
                    /* テンポラリバッファにデータを格納(取得は8byte固定) */
                    l_vog_lin_get_rcvdata( u1a_lin_rs_tmp );

                    /* 1バイト目のデータが00hの場合 */
                    if( U1G_LIN_SLEEP_DATA == u1a_lin_rs_tmp[ U1G_LIN_0 ] ) {
                        /* SLEEPコマンドとして受信 */
                        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_SLEEP;                    /* SLEEP状態に移行 */
                        xnl_lin_bus_sts.u1g_lin_goto_sleep = U1G_LIN_BIT_SET;               /*GoToSleepコマンドフラグセット*/
                        l_vog_lin_int_enb();                                                /* ウェイクアップ検出許可 */

                        u1a_lin_result_tmp = l_u1g_lin_set_reset();                         /* LINモジュールRESETモード設定 */
                        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                         /* システム異常(MCUステータス) */
                        }
                        l_hook_sleep();                                                     /* Sleep時Hook処理 */
                    }
                    /* 1バイト目のデータが00h以外の場合 */
                    else{
                        /* 何もせずに Break Field受信待ち状態にする */
                    }
                }
                u1l_lin_hdr_rcved = U1G_LIN_HDR_NOT_RCVED;                  /* ヘッダ未受信に戻す */
            }
             /* SLEEPコマンド以外のID の場合 */
            else{
                /* フレーム定義ありの場合 */
                if( U1G_LIN_NO_FRAME != xnl_lin_id_sl.u1g_lin_slot ) {
                    /* 通常フレームとして受信 */
                    /* LINバッファにデータを格納(取得は8byte固定) */
                    l_vog_lin_get_rcvdata( xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte );
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );              /* 転送成功 */
                    l_hook_rcv_comp(xnl_lin_id_sl.u1g_lin_slot);                /* 受信完了時フック処理 */
                }
                /* フレーム未定義の場合 */
                else{
                    /* 何もせずに Break Field受信待ち状態にする */
                }
            }
        }
        else if( U1G_LIN_BYTE_CLR != (u1a_lin_data & U1G_LIN_HDR_RCVED_MASK) ) {
            /* エラーを確定する */
            l_vog_lin_determine_ResTooShort_or_NoRes(u1l_lin_lst_d1rc_state);       /* Response too short / No response を確定する */
            u1l_lin_lst_d1rc_state = U1G_LIN_LST_D1RC_INCOMPLETE;                   /* データ1受信完了の内部状態をクリアする */

#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
            u1a_lin_is_valid = l_u1g_lin_is_valid_baud_rate();                      /* ヘッダ受信時のボー・レート妥当性判断処理 */
            /* 検出されたボー・レートが指定されたボー・レートの範囲に収まっているか */
            if( U1G_LIN_OK == u1a_lin_is_valid ) {
#endif
                /* ヘッダの受信完了 */
                l_vol_lin_sub_hdr_rcv( u1a_lin_id );                                /* ヘッダ受信時処理 */
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
            }
            /* 検出されたボー・レートが指定されたボー・レートの範囲に収まっているない場合 */
            else {
                u1a_lin_result_tmp = l_u1g_lin_resp_noreq();                        /* レスポンスなし要求 */
                if( U1G_LIN_OK != u1a_lin_result_tmp ) {
                    u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                         /* システム異常(MCUステータス) */
                }
                xnl_lin_bus_sts.u1g_lin_head_err                = U1G_LIN_BIT_SET;  /* Headerエラー */
                xng_lin_sts_buf.u1g_lin_e_sync                  = U1G_LIN_BIT_SET;  /* Synch Fieldエラー */
                l_hook_err(U1G_LIN_ID_DUMMY, U2G_LIN_ERRINFO_HDR_SYNC);             /* エラー時フック処理 */
            }
#endif
        }
        else{
            /* 通常は発生しない処理 */
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                             /* システム異常(ドライバ) */
        }
    }
    /** RESET状態、異常値 **/
    else{
        /* 通常は発生しない処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                                   /* システム異常(ステータス) */
    }

}

/**************************************************/
/*  Response too short時確定処理                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void l_vog_lin_ResTooShort_error(void)
{
    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_res_sht = U1G_LIN_BIT_SET;
    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                           /* エラーありレスポンス完了 */
    l_hook_err(xnl_lin_id_sl.u1g_lin_slot, U2G_LIN_ERRINFO_RSP_RESSHT);     /* エラー時フック処理 */
}

/**************************************************/
/*  No response時確定処理                         */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void l_vog_lin_NoRes_error(void)
{
    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_nores = U1G_LIN_BIT_SET;
    l_vol_lin_set_frm_complete( U1G_LIN_ERR_NORES );                       /* No response完了 */
    l_hook_err(xnl_lin_id_sl.u1g_lin_slot, U2G_LIN_ERRINFO_RSP_NORES);     /* エラー時フック処理 */
}

/**************************************************/
/*  LINヘッダ受信完了時処理                       */
/*------------------------------------------------*/
/*  引数： u1a_lin_id   : 受信したID              */
/*  戻値： なし                                   */
/**************************************************/
static void  l_vol_lin_sub_hdr_rcv(l_u8 u1a_lin_id)
{
    l_u8    u1a_lin_result;

    u2l_lin_bus_inactive_cnt                             = U2G_LIN_WORD_CLR;    /* Bus inactive検出用カウンタ クリア */
    xng_lin_sts_buf.u1g_lin_bus_inactive                 = U1G_LIN_BIT_CLR;     /* Bus inactive検出フラグクリア */
    xnl_lin_bus_sts.u1g_lin_bus_err                      = U1G_LIN_BIT_CLR;     /* Bus異常フラグクリア */

    xnl_lin_id_sl.u1g_lin_id   = (l_u8)( u1a_lin_id & U1G_LIN_ID_PARITY_MASK ); /* ID取得 */
    xnl_lin_id_sl.u1g_lin_slot = u1g_lin_id_tbl[ xnl_lin_id_sl.u1g_lin_id ];

    /* SLEEPコマンドIDを受信した場合(必ずレスポンス受信動作となる) */
    if( U1G_LIN_SLEEP_ID == xnl_lin_id_sl.u1g_lin_id ) {
        /* フレームが[定義] かつ LINバッファのスロットが[未使用設定]の場合 */
        if( (U1G_LIN_NO_FRAME != xnl_lin_id_sl.u1g_lin_slot)
         && (U1G_LIN_BIT_SET == xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_no_use) ) {
            xnl_lin_id_sl.u1g_lin_slot = U1G_LIN_NO_FRAME;              /* フレーム[未定義]に変更 */
        }

        u1a_lin_result = l_u1g_lin_resp_rcv_start( U1G_LIN_DL_8, U1G_LIN_SUM_CLASSIC ); /* レスポンス受信開始 */
        if( U1G_LIN_OK != u1a_lin_result ) {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                     /* システム異常(MCUステータス) */
        }
        else{
            /* フレームが[定義]の場合のみ */
            /* 送受信ステータスをbusyに設定する */
            if( U1G_LIN_NO_FRAME != xnl_lin_id_sl.u1g_lin_slot ) {
                xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_status =  U1G_LIN_BIT_SET;
            }
            else{
                /* 何もしない（未定義/未使用のため、エラー検出対象としない） */
            }
            u1l_lin_hdr_rcved = U1G_LIN_HDR_RCVED;                      /* 有効ヘッダ受信済み */
        }
    }
    /* SLEEPコマンド以外の場合 */
    else{
        /* フレームが[未定義] もしくは LINバッファのスロットが[未使用設定]の場合 */
        if( (U1G_LIN_NO_FRAME == xnl_lin_id_sl.u1g_lin_slot)
         || (U1G_LIN_BIT_SET == xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_no_use) ) {

            u1a_lin_result = l_u1g_lin_resp_noreq();                    /* レスポンスなし要求 */

            if( U1G_LIN_OK != u1a_lin_result ) {
                u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;                 /* システム異常(MCUステータス) */
            }
        }
        /* 処理対象フレームの場合 */
        else{
            /*-- [受信フレーム時] --*/
            if( U1G_LIN_CMD_RCV == xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv ) {
                u1a_lin_result = l_u1g_lin_resp_rcv_start( xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_frm_sz,
                                            xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_sum_type );    /* レスポンス受信開始 */
                if( U1G_LIN_OK != u1a_lin_result ) {
                    u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;             /* システム異常(MCUステータス) */
                }
                else{
                    /* 送受信ステータスをbusyに設定する */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_status =  U1G_LIN_BIT_SET;
                    u1l_lin_hdr_rcved = U1G_LIN_HDR_RCVED;              /* 有効ヘッダ受信済み */
                }
            }
            /*-- [送信フレーム時] --*/
            else if( U1G_LIN_CMD_SND == xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv ) {
            
                /* NM使用設定フレームの場合 */
                if( U1G_LIN_NM_USE == xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_nm_use ) {
                    /* LINフレームバッファのNM部分(データ1のbit4-7)をクリア */
                    /* NM部分のクリア */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                     &= U1G_LIN_BUF_NM_CLR_MASK;
                    /* LINフレームに レスポンス送信ステータスをセット */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte[ U1G_LIN_0 ]
                     |= (l_u8)( u1l_lin_nm_info & U1G_LIN_NM_INFO_MASK );
                }

                u1a_lin_result = l_u1g_lin_resp_snd_start( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_frm_sz,
                                            xng_lin_slot_tbl[xnl_lin_id_sl.u1g_lin_slot].u1g_lin_sum_type,
                                            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].xng_lin_data.u1g_lin_byte );  /* レスポンス送信開始 */

                if( U1G_LIN_OK != u1a_lin_result ) {
                    u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;             /* システム異常(MCUステータス) */
                }
                else{
                    /* 送受信ステータスをbusyに設定する */
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_status =  U1G_LIN_BIT_SET;
                    u1l_lin_hdr_rcved = U1G_LIN_HDR_RCVED;              /* 有効ヘッダ受信済み */
                }
            }
            /*-- [送信でも受信でもない] --*/
            else{
                /* 登録エラー */
                u1a_lin_result = l_u1g_lin_resp_noreq();                /* レスポンスなし要求 */

                if( U1G_LIN_OK != u1a_lin_result ) {
                    u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;             /* システム異常(ドライバ */
                }
            }
        }
    }
}


/**************************************************/
/*  LIN送信完了割り込み発生                       */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void l_vog_lin_tx_int(void)
{
    l_u8 u1a_lin_result_tmp;

    /** RUN状態 **/
    if( U1G_LIN_STS_RUN == xng_lin_sts_buf.u1g_lin_sts ) {
        /* レスポンス送信完了 */
        l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );          /* 転送成功 */
        l_hook_snd_comp(xnl_lin_id_sl.u1g_lin_slot);            /* 送信完了時フック処理 */
    }
    /** SLEEP状態 **/
    else if( U1G_LIN_STS_SLEEP == xng_lin_sts_buf.u1g_lin_sts ) {
        /* ウェイクアップ送信完了 */
        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_RUN_STANDBY;
        l_ifc_init_drv();                                       /* LINドライバ部の初期化 */
        /* ※ウェイクアップ送信開始時にl_vog_lin_int_dis()実行済みであるため、ここでは不要  */
    }
    /** RUN_STANDBY状態 **/
    else if( U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts ) {
        /* ウェイクアップ送信完了 */
        u1a_lin_result_tmp = l_u1g_lin_set_run();               /* LINモジュール通信可能状態設定 */
        if( U1G_LIN_OK != u1a_lin_result_tmp ) {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;             /* システム異常(MCUステータス) */
        }
    }
    /** RESET状態、異常値 **/
    else{
        /* 通常は発生しない処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                   /* システム異常(ステータス) */
    }
}


/**************************************************/
/*  LINエラー割り込み発生                         */
/*------------------------------------------------*/
/*  引数： u1a_lin_err       : エラー情報         */
/*         u1a_lin_lst_data  : LINステータス情報  */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_err_int(l_u8 u1a_lin_err, l_u8 u1a_lin_lst_data )
{
    /** RUN状態 または RUN_STANDBY状態 **/
    if(   (U1G_LIN_STS_RUN == xng_lin_sts_buf.u1g_lin_sts)
       || (U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts) ) {

        l_vog_lin_determine_ResTooShort_or_NoRes(u1l_lin_lst_d1rc_state);               /* Response too short / No response を確定する */
        u1l_lin_lst_d1rc_state = U1G_LIN_LST_D1RC_INCOMPLETE;                           /* データ1受信完了の内部状態をクリアする */

        if( U1G_LIN_HDR_NOT_RCVED == u1l_lin_hdr_rcved ) {
            /* Synch Field Error */
            if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_SF_ERR_MASK) ) {
                xnl_lin_bus_sts.u1g_lin_head_err                = U1G_LIN_BIT_SET;      /* Headerエラー */
                xng_lin_sts_buf.u1g_lin_e_sync                  = U1G_LIN_BIT_SET;      /* Synch Fieldエラー */
                l_hook_err(U1G_LIN_ID_DUMMY, U2G_LIN_ERRINFO_HDR_SYNC);                 /* エラー時フック処理 */
            }
            /* ID parity Error */
            else if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_ID_ERR_MASK) ) {
                xnl_lin_bus_sts.u1g_lin_head_err                = U1G_LIN_BIT_SET;      /* Headerエラー */
                xng_lin_sts_buf.u1g_lin_e_pari                  = U1G_LIN_BIT_SET;      /* PARITYエラー */
                l_hook_err(U1G_LIN_ID_DUMMY, U2G_LIN_ERRINFO_HDR_PARI);                 /* エラー時フック処理 */
            }
            /* Framing Error */
            else if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_FRA_ERR_MASK) ) {
                xnl_lin_bus_sts.u1g_lin_head_err                = U1G_LIN_BIT_SET;      /* Headerエラー */
                xng_lin_sts_buf.u1g_lin_e_uart                  = U1G_LIN_BIT_SET;      /* UARTエラー */
                l_hook_err(U1G_LIN_ID_DUMMY, U2G_LIN_ERRINFO_HDR_UART);                 /* エラー時フック処理 */
            }
            else{
                /* 何もしない  ヘッダ受信フラグ異常、エラーフラグ異常、未定義Sleepコマンド時のいずれか */
            }
        }
        else if( U1G_LIN_HDR_RCVED == u1l_lin_hdr_rcved ) {
            /* SLEEPコマンドIDで、フレーム未定義の場合 */
            if( (U1G_LIN_SLEEP_ID == xnl_lin_id_sl.u1g_lin_id) && (U1G_LIN_NO_FRAME == xnl_lin_id_sl.u1g_lin_slot) ) {
                u1l_lin_hdr_rcved = U1G_LIN_HDR_NOT_RCVED;      /* ヘッダ未受信に戻す */
            }
            /* SLEEPコマンドIDではない、もしくはフレーム定義ありの場合 */
            else{
                /* Bit Error */
                if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_BIT_ERR_MASK) ) {
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_bit = U1G_LIN_BIT_SET;
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                       /* エラーありレスポンス完了 */
                    l_hook_err(xnl_lin_id_sl.u1g_lin_slot, U2G_LIN_ERRINFO_RSP_BIT);       /* エラー時フック処理 */
                }
                /* Framing Error */
                else if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_FRA_ERR_MASK) ) {
                    l_vog_lin_start_timer();                                                /* タイマのカウンタを開始する */
                    /* LINエラー情報からデータ1受信状態を判定し、状態変数を更新する */
                    if(U1G_LIN_BYTE_CLR != (u1a_lin_lst_data & U1G_LIN_LST_D1RC_MASK)){
                        u1l_lin_lst_d1rc_state = U1G_LIN_LST_D1RC_COMPLETE;    
                    }
                }
                /* Check sum Error */
                else if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_CKSUM_ERR_MASK) ) {
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_sum = U1G_LIN_BIT_SET;
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                       /* 転送エラー */
                    l_hook_err(xnl_lin_id_sl.u1g_lin_slot, U2G_LIN_ERRINFO_RSP_SUM);       /* エラー時フック処理 */
                }
                /* レスポンス準備エラー */
                else if( U1G_LIN_BYTE_CLR != (u1a_lin_err & U1G_LIN_RESP_ERR_MASK) ) {
                    /* 通常到達しない見込み   */
                    u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                               /* システム異常(ステータス) */
                    l_vol_lin_set_frm_complete( U1G_LIN_ERR_READY );                      /* レスポンス準備エラー発生 */
                    l_hook_err(xnl_lin_id_sl.u1g_lin_slot, U2G_LIN_ERRINFO_RSP_READY);       /* エラー時フック処理 */
                }
                else{
                    /* 何もしない  ヘッダ受信フラグ異常、エラーフラグ異常のどちらか */
                }
            }
        }
        else{
            /* 何もしない  ヘッダ受信状態異常 */
        }
    }
    /** RESET状態、SLEEP状態、異常値 **/
    else{
        /* 通常は発生しない処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;                   /* システム異常(ステータス) */
    }
}

/**************************************************/
/*  Framing エラー確定処理                        */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_determine_FramingError(void)
{
    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_uart = U1G_LIN_BIT_SET;
    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );                       /* エラーありレスポンス完了 */
    l_hook_err(xnl_lin_id_sl.u1g_lin_slot, U2G_LIN_ERRINFO_RSP_UART);      /* エラー時フック処理 */
}


/**************************************************/
/*  INT割り込み発生                               */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_aux_int(void)
{
    /* SLEEP状態 */
    if( U1G_LIN_STS_SLEEP == xng_lin_sts_buf.u1g_lin_sts ) {
        /* ウェイクアップ検出   RUN STANDBY状態へ移行 */
        xng_lin_sts_buf.u1g_lin_sts = U1G_LIN_STS_RUN_STANDBY;
        l_ifc_init_drv();                           /* ドライバ部の初期化 */
        l_vog_lin_int_dis();                        /* INT割り込み禁止（wake_hook実行のため） */
    }

    /** RUN_STANDBY状態 **/
    else if( U1G_LIN_STS_RUN_STANDBY == xng_lin_sts_buf.u1g_lin_sts ) {
        u1l_lin_wup_det = U1G_LIN_WUP_DET;          /* NM通知のため保持 */
        l_vog_lin_int_dis();                        /* INT割り込み禁止 */
    }
    /** RUN状態、RUN_STANDBY状態、異常値 **/
    else{
        /* 通常起こらない */
        l_vog_lin_int_dis();                        /* INT割り込みを禁止する */
    }
}


/**************************************************/
/*  ウェイクアップ検出フラグ取得処理              */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： ウェイクアップ検出状態                 */
/**************************************************/
l_u8  l_u1g_lin_get_wupflg(void)
{
    l_u8    u1a_lin_result;

    u1a_lin_result  = u1l_lin_wup_det;              /* ウェイクアップ検出フラグ取得 */

    return u1a_lin_result;
}

/**************************************************/
/*  ウェイクアップ検出フラグクリア処理            */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_clr_wupflg(void)
{
    u1l_lin_wup_det = U1G_LIN_WUP_NOT_DET;          /* ウェイクアップ検出フラグクリア */
}

/**************************************************/
/*  ウェイクアップ検出許可処理                    */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_set_int_enb(void)
{
    l_vod_lin_DI();                                 /* 割り込み禁止設定 */

    l_vog_lin_int_enb();                            /* INT割り込み許可 */
    l_hook_sleep();                                 /* Sleep時Hook処理 */

    l_vod_lin_EI();                                 /* 割り込み許可     */
}


/**************************************************/
/*  完了フラグの設定処理                          */
/*------------------------------------------------*/
/*  引数： u1a_lin_err : 完了状態                 */
/*         (0 / 1) : 正常完了 / 異常完了          */
/*  戻値： なし                                   */
/**************************************************/
static void  l_vol_lin_set_frm_complete(l_u8  u1a_lin_err)
{
    /* 転送成功、もしくは転送エラーのフラグが立っている場合(オーバーラン) */
    if( U1G_LIN_BYTE_CLR != (xnl_lin_bus_sts.u1g_lin_err_resp | xnl_lin_bus_sts.u1g_lin_ok_resp) ) {
        xnl_lin_bus_sts.u1g_lin_ovr_run = U1G_LIN_BIT_SET;                      /* オーバーラン */
    }
    /* 異常完了の場合 */
    if( U1G_LIN_ERR_ON == u1a_lin_err ) {
        xnl_lin_bus_sts.u1g_lin_err_resp = U1G_LIN_BIT_SET;                     /* エラー有りレスポンス */
    }
    /* 正常完了の場合 */
    else if( U1G_LIN_ERR_OFF == u1a_lin_err ) {
        xnl_lin_bus_sts.u1g_lin_ok_resp = U1G_LIN_BIT_SET;                      /* 正常完了 */
        if( U1G_LIN_MAX_SLOT > xnl_lin_id_sl.u1g_lin_slot ) {
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_nores = U1G_LIN_BYTE_CLR;
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_uart = U1G_LIN_BYTE_CLR;
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_bit = U1G_LIN_BYTE_CLR;
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_sum = U1G_LIN_BYTE_CLR;
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_e_res_sht = U1G_LIN_BYTE_CLR;
        }
    }
    /* No Response、レスポンス準備エラー発生の場合 */
    else {
        /* Does Nothing */
    }

    /* 保護IDのセット */
    xnl_lin_bus_sts.u1g_lin_last_id = u1g_lin_protid_tbl[ xnl_lin_id_sl.u1g_lin_id ];

    if( U1G_LIN_MAX_SLOT > xnl_lin_id_sl.u1g_lin_slot ) {

        /* 正常完了の場合にのみ完了フラグをセット */
        if( U1G_LIN_ERR_OFF == u1a_lin_err ) {
            /* 送受信処理完了フラグのセット */
            if( U1G_LIN_16 > xnl_lin_id_sl.u1g_lin_slot ) {
                xng_lin_sts_buf.un_rs_flg1.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot ];
            }
            else if( U1G_LIN_32 > xnl_lin_id_sl.u1g_lin_slot ) {
                xng_lin_sts_buf.un_rs_flg2.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot - U1G_LIN_16 ];
            }
            else if( U1G_LIN_48 > xnl_lin_id_sl.u1g_lin_slot ) {
                xng_lin_sts_buf.un_rs_flg3.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot - U1G_LIN_32 ];
            }
            else if( U1G_LIN_64 > xnl_lin_id_sl.u1g_lin_slot ) {
                xng_lin_sts_buf.un_rs_flg4.u2g_lin_word |= u2g_lin_flg_set_tbl[ xnl_lin_id_sl.u1g_lin_slot - U1G_LIN_48 ];
            }
            else {
                /* 通常発生しないので処理なし */
            }
        }
        /* 送受信ステータスをreadyに設定する */
        xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_status =  U1G_LIN_BIT_CLR;
    }

    u1l_lin_hdr_rcved = U1G_LIN_HDR_NOT_RCVED;                                  /* ヘッダ未受信に戻す */
}


/**************************************************/
/*  NM情報データのセット 処理                     */
/*------------------------------------------------*/
/*  引数： u1a_lin_nm_info : NM情報データ         */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_set_nm_info( l_u8  u1a_lin_nm_info )
{
    u1l_lin_nm_info = u1a_lin_nm_info;
}


/***** End of File *****/


