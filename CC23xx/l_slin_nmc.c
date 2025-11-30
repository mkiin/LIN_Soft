/**
 * @file        l_slin_nmc.c
 *
 * @brief       SLIN NMC層
 *
 * @attention   編集禁止
 *
 */

#pragma	section	lin

#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_slin_nmc.h"

#define U1L_LIN_FLG_OFF             ((l_u8)0)
#define U1L_LIN_FLG_ON              ((l_u8)1)

#define U1L_LIN_SLPIND_SET          ((l_u8)0x80)    /* SLEEP_INDビットのセット用 (bit 7) */
#define U1L_LIN_WUPIND_SET          ((l_u8)0x40)    /* WAKEUP_INDビットのセット用 (bit 6) */
#define U1L_LIN_DATIND_SET          ((l_u8)0x20)    /* DATA_INDビットのセット用 (bit 5) */
#define U1L_LIN_RESPERR_SET         ((l_u8)0x10)    /* RESPONSE_ERRORビットのセット用 (bit 4) LIN 2.0 */

/* スレーブタスク用変数 */
static l_u16 u2a_lin_stat;
static l_u8  u1l_lin_mod_slvstat;                   /* NMスレーブステータス */
static l_u16 u2l_lin_tmr_slvst;                     /* Tslv_stカウンタ */
static l_u16 u2l_lin_tmr_wurty;                     /* Twurtyカウンタ */
static l_u16 u2l_lin_tmr_3brk;                      /* T3brksカウンタ */
static l_u16 u2l_lin_tmr_timeout;                   /* Ttimeoutカウンタ */
static l_u16 u2l_lin_tmr_data;                      /* Data.indカウンタ */
static l_u16 u2l_lin_cnt_retry;                     /* Retryカウンタ */
static l_u8  u1l_lin_wup_ind;                       /* Wakeup.ind情報フラグ */
static l_u8  u1l_lin_data_ind;                      /* Data.ind情報フラグ */
static l_u8  u1l_lin_cnt_msterr;                    /* マスタ監視用カウンタ */
static l_u8  u1l_lin_flg_msterr;                    /* マスタ異常発生フラグ */

/* 外部参照定義 */
extern void  l_vog_lin_set_nm_info(l_u8 u1a_lin_nm_info);

/* NM用APIプロトタイプ宣言 */
void l_nm_init_ch1(void);
void l_nm_tick_ch1(l_u8 u1a_lin_slp_req);
l_u8 l_nm_rd_slv_stat_ch1(void);
l_u8 l_nm_rd_mst_err_ch1(void);
void l_nm_clr_mst_err_ch1(void);

/****************************************************************************/
/*  NM用Tick処理API                                                         */
/*--------------------------------------------------------------------------*/
/*  引数：l_u8 slp_req                                                      */
/*          U1G_LIN_SLP_REQ_OFF(0)   :SLEEP要求無し、又はWAKEUP要因有り     */
/*          U1G_LIN_SLP_REQ_ON(1)    :SLEEP要求有り、又はWAKEUP要因無し     */
/*          U1G_LIN_SLP_REQ_FORCE(2) :強制SLEEP要求有り                     */
/*  戻値：なし                                                              */
/****************************************************************************/
void l_nm_tick_ch1(l_u8 u1a_lin_slp_req)
{
    l_u8  u1a_lin_tmp_nm_dat;

    /* LINステータスのリード */
    u2a_lin_stat = l_ifc_read_lb_status_ch1();

    /* スレーブタスク用NMステータス管理処理 */
    switch (u2a_lin_stat) {
    /* LINステータスがSLEEP状態 */
    case (U2G_LIN_STS_SLEEP):
        switch (u1l_lin_mod_slvstat) {
        /* LINステータスがSLEEP状態 / NMスレーブステータスが起動後 */
        case (U1G_LIN_SLVSTAT_PON):
            /* 起動時は、スケジュール開始待ち状態にする */
            u2l_lin_tmr_slvst += U2G_LIN_NM_TIME_BASE;
            l_ifc_run_ch1();
            break;
        /* LINステータスがSLEEP状態 / NMスレーブステータスがWAKE_WAIT状態 */
        case (U1G_LIN_SLVSTAT_WAKE_WAIT):
            /* ウェイクアップ要因なし */
            if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_ON ) {
                u2l_lin_tmr_timeout += U2G_LIN_NM_TIME_BASE;
                if( u2l_lin_tmr_timeout >= U2G_LIN_TM_TIMEOUT ) {
                    /* NMスレーブステータスをSLEEP状態に移行 */
                    u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_SLEEP;
                }
            }
            /* ウェイクアップ要因あり */
            else if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_OFF ) {
                /* ウェイクアップ送信 */
                l_ifc_wake_up_ch1();
                u1l_lin_wup_ind = U1L_LIN_FLG_ON;
                u2l_lin_tmr_wurty = U2G_LIN_WORD_CLR;
                u2l_lin_cnt_retry = U2G_LIN_WORD_CLR;
                u2l_lin_tmr_timeout = U2G_LIN_WORD_CLR;
                u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_WAKE;
            }
            /* 強制スリープ要因あり */
            else if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_FORCE ) {
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_SLEEP;
            }
            /* 上記以外 */
            else {
                /* 引数が範囲外の為、何もしない */
            }
            break;
        /* LINステータスがSLEEP状態 / NMスレーブステータスがSLEEP状態 */
        case (U1G_LIN_SLVSTAT_SLEEP):
            /* ウェイクアップ要因あり */
            if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_OFF ) {
                l_ifc_wake_up_ch1();
                u1l_lin_wup_ind = U1L_LIN_FLG_ON;
                u2l_lin_tmr_wurty = U2G_LIN_WORD_CLR;
                u2l_lin_cnt_retry = U2G_LIN_WORD_CLR;
                u2l_lin_tmr_timeout = U2G_LIN_WORD_CLR;
                u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_WAKE;
            }
            break;
        /* LINステータスがSLEEP状態 / NMスレーブステータスがWAKE状態 */
        case (U1G_LIN_SLVSTAT_WAKE):
            /* Twurty時間経過後、ウェイクアップ再送する。*/
            u2l_lin_tmr_timeout += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_3brk += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_wurty += U2G_LIN_NM_TIME_BASE;
            if( u2l_lin_tmr_wurty >= U2G_LIN_TM_WURTY ) {
                if( u2l_lin_cnt_retry < U2G_LIN_CNT_RETRY ) {
                    /* ウェイクアップ再送 */
                    l_ifc_wake_up_ch1();
                    u2l_lin_cnt_retry++;
                    u2l_lin_tmr_wurty = U2G_LIN_WORD_CLR;
/* Ver 2.00 追加:T T3BRKの起点を変更 */
                    u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
/* Ver 2.00 追加:T T3BRKの起点を変更 */
                }
                else {
                    /* 2回ウェイクアップ再送後は、他WAKE状態に移行する。*/
                    u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_OTHER_WAKE;
                }
            }
            break;
        /* LINステータスがSLEEP状態 / NMスレーブステータスが他WAKE状態 */
        case (U1G_LIN_SLVSTAT_OTHER_WAKE):
            /* T3brks時間経過後、WAKE-WAIT状態に移行する */
            u2l_lin_tmr_timeout += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_3brk += U2G_LIN_NM_TIME_BASE;
            if( u2l_lin_tmr_3brk >= U2G_LIN_TM_3BRKS ) {
                u1l_lin_cnt_msterr++;
                if( u1l_lin_cnt_msterr >= U1G_LIN_CNT_MSTERR ) {
                    u1l_lin_flg_msterr = U1L_LIN_FLG_ON;
                }
                u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_WAKE_WAIT;
            }
            break;
        /* LINステータスがSLEEP状態 / NMスレーブステータスがACTIVE状態 */
        case (U1G_LIN_SLVSTAT_ACTIVE):
            /* SLEEP状態に移行する */
            u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_SLEEP;
            break;
        /* LINステータスがSLEEP状態 / NMスレーブステータスが上記以外 */
        default:
            /* 通常ありえないが、フェール処理としてSLEEP状態に移行 */
            u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_SLEEP;
            break;
        }
        break;
    /* LINステータスがRUN STANDBY状態 */
    case (U2G_LIN_STS_RUN_STANDBY):
        switch (u1l_lin_mod_slvstat) {
        /* LINステータスがRUN STANDBY状態 / NMスレーブステータスが起動後 */
        case (U1G_LIN_SLVSTAT_PON):
            /* Tslv_st時間経過後にWAKE-WAIT状態に移行する */
            u2l_lin_tmr_timeout += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_slvst += U2G_LIN_NM_TIME_BASE;
            if( u2l_lin_tmr_slvst >= U2G_LIN_TM_SLVST ) {
                l_ifc_sleep_ch1();
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_WAKE_WAIT;
            }
            break;
        /* LINステータスがRUN STANDBY状態 / NMスレーブステータスがWAKE_WAIT状態、又はSLEEP状態 */
        case (U1G_LIN_SLVSTAT_WAKE_WAIT):
        case (U1G_LIN_SLVSTAT_SLEEP):
            /* ウェイクアップ検出 */
            u2l_lin_tmr_timeout = U2G_LIN_WORD_CLR;
            u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_OTHER_WAKE;
            break;
        /* LINステータスがRUN STANDBY状態 / NMスレーブステータスがWAKE状態 */
        case (U1G_LIN_SLVSTAT_WAKE):
            u2l_lin_tmr_timeout += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_3brk += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_wurty += U2G_LIN_NM_TIME_BASE;
            if( u2l_lin_tmr_wurty >= U2G_LIN_TM_WURTY ) {
                if( u2l_lin_cnt_retry < U2G_LIN_CNT_RETRY ) {
                    l_ifc_wake_up_ch1();
                    u2l_lin_cnt_retry++;
                    u2l_lin_tmr_wurty = U2G_LIN_WORD_CLR;
/* Ver 2.00 追加:T T3BRKの起点を変更 */
                    u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
/* Ver 2.00 追加:T T3BRKの起点を変更 */
                }
                else {
                    u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_OTHER_WAKE;
                }
            }
            break;
        /* LINステータスがRUN STANDBY状態 / NMスレーブステータスが他WAKE状態 */
        case (U1G_LIN_SLVSTAT_OTHER_WAKE):
            /* T3brks時間経過後、WAKE-WAIT状態に移行する */
            u2l_lin_tmr_timeout += U2G_LIN_NM_TIME_BASE;
            u2l_lin_tmr_3brk += U2G_LIN_NM_TIME_BASE;
            if( u2l_lin_tmr_3brk >= U2G_LIN_TM_3BRKS ) {
                u1l_lin_cnt_msterr++;
                if( u1l_lin_cnt_msterr >= U1G_LIN_CNT_MSTERR ) {
                    u1l_lin_flg_msterr = U1L_LIN_FLG_ON;
                }
                l_ifc_sleep_ch1();
                u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_WAKE_WAIT;
            }
            break;
        /* LINステータスがRUN STANDBY状態 / NMスレーブステータスがACTIVE状態 */
        case (U1G_LIN_SLVSTAT_ACTIVE):
            /* SLEEP状態への移行 & 他ノードのウェイクアップ検出 */
            u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
            u2l_lin_tmr_timeout = U2G_LIN_WORD_CLR;
            u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_OTHER_WAKE;
            break;
        /* LINステータスがRUN STANDBY状態 / NMスレーブステータスが上記以外 */
        default:
            /* 通常ありえないが、フェール処理として他WAKE状態に移行 */
            u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
            u2l_lin_tmr_timeout = U2G_LIN_WORD_CLR;
            u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_OTHER_WAKE;
            break;
        }
        break;
    /* LINステータスがRUN状態 */
    case (U2G_LIN_STS_RUN):
        switch (u1l_lin_mod_slvstat) {
        /* LINステータスがRUN状態 / NMスレーブステータスがPASSIVE状態 */
        case (U1G_LIN_SLVSTAT_PON):
        case (U1G_LIN_SLVSTAT_WAKE_WAIT):
        case (U1G_LIN_SLVSTAT_SLEEP):
        case (U1G_LIN_SLVSTAT_WAKE):
        case (U1G_LIN_SLVSTAT_OTHER_WAKE):
            u1l_lin_cnt_msterr = U1G_LIN_BYTE_CLR;
            u1l_lin_flg_msterr = U1L_LIN_FLG_OFF;
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_ACTIVE;
            break;
        /* LINステータスがRUN状態 / NMスレーブステータスがACTIVE状態 */
        case (U1G_LIN_SLVSTAT_ACTIVE):
            /* 強制スリープ要因あり */
            if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_FORCE ) {
                l_ifc_sleep_ch1();
                u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
                u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_SLEEP;
            }
            break;
        /* LINステータスがRUN状態 / NMスレーブステータスが上記以外 */
        default:
            /* 通常ありえないが、フェール処理としてACTIVE状態に移行 */
            u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_ACTIVE;
            break;
        }
        break;
    /* LINステータスが上記以外 */
    default:
        /* 通常は発生しない処理 */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;               /* システム異常(ステータス) */
        break;
    }

    /* Data.indの管理 */
    if( u1l_lin_data_ind == U1L_LIN_FLG_OFF ) {
        u2l_lin_tmr_data += U2G_LIN_NM_TIME_BASE;
        if( u2l_lin_tmr_data >= U2G_LIN_TM_DATA ) {
            u1l_lin_data_ind = U1L_LIN_FLG_ON;
        }
    }

    /* LINライブラリのNM情報書換え処理 */
    u1a_lin_tmp_nm_dat = U1G_LIN_BYTE_CLR;

    /* response_error シグナルの集約 (LIN 2.0 Status Management) */
    /* 全フレームのエラーフラグをチェックし、いずれかでエラーがあればresponse_errorをセット */
    {
        l_u8 u1a_slot;
        l_bool u1a_has_response_error = U2G_LIN_NG;

        for( u1a_slot = U1G_LIN_0; u1a_slot < U1G_LIN_MAX_SLOT; u1a_slot++ ) {
            /* フレームバッファのエラーフラグ(4bit)をチェック */
            if( xng_lin_frm_buf[ u1a_slot ].un_state.st_err.u2g_lin_err != U2G_LIN_BYTE_CLR ) {
                u1a_has_response_error = U2G_LIN_OK;
                break;
            }
        }

        /* response_errorビットのセット (bit 4) */
        if( u1a_has_response_error == U2G_LIN_OK ) {
            u1a_lin_tmp_nm_dat |= U1L_LIN_RESPERR_SET;
        }
    }

    if( u1a_lin_slp_req == U1G_LIN_SLP_REQ_ON ) {
        /* Sleep.indビットのセット */
        u1a_lin_tmp_nm_dat |= U1L_LIN_SLPIND_SET;
    }
    if( u1l_lin_wup_ind == U1L_LIN_FLG_ON ) {
        /* Wakeup.indビットのセット */
        u1a_lin_tmp_nm_dat |= U1L_LIN_WUPIND_SET;
    }
    if( u1l_lin_data_ind == U1L_LIN_FLG_ON ) {
        /* Data.indビットのセット */
        u1a_lin_tmp_nm_dat |= U1L_LIN_DATIND_SET;
    }
    /* NM情報書換え関数のコール */
    l_vog_lin_set_nm_info(u1a_lin_tmp_nm_dat);
}

/****************************************************************************/
/*  NM用初期化処理API                                                       */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：なし                                                              */
/****************************************************************************/
void l_nm_init_ch1(void)
{
/* スレーブタスク用変数 */
    u1l_lin_mod_slvstat = U1G_LIN_SLVSTAT_PON;
    u2l_lin_tmr_slvst = U2G_LIN_WORD_CLR;
    u2l_lin_tmr_wurty = U2G_LIN_WORD_CLR;
    u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;
    u2l_lin_tmr_timeout = U2G_LIN_WORD_CLR;
    u2l_lin_tmr_data = U2G_LIN_WORD_CLR;
    u2l_lin_cnt_retry = U2G_LIN_WORD_CLR;
    u1l_lin_wup_ind = U1L_LIN_FLG_OFF;
    u1l_lin_data_ind = U1L_LIN_FLG_OFF;
    u1l_lin_cnt_msterr = U1G_LIN_BYTE_CLR;
    u1l_lin_flg_msterr = U1L_LIN_FLG_OFF;
}

/****************************************************************************/
/*  NMスレーブステータスリードAPI                                           */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：l_u8  NMスレーブステータス                                        */
/****************************************************************************/
l_u8 l_nm_rd_slv_stat_ch1(void)
{
    return (u1l_lin_mod_slvstat);
}

/****************************************************************************/
/*  マスタ異常フラグのリードAPI                                             */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：l_u8  0：マスタ異常なし, 1:マスタ異常あり                         */
/****************************************************************************/
l_u8 l_nm_rd_mst_err_ch1(void)
{
    return (u1l_lin_flg_msterr);
}

/****************************************************************************/
/*  マスタ異常フラグのクリアAPI                                             */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：なし                                                              */
/****************************************************************************/
void l_nm_clr_mst_err_ch1(void)
{
    u1l_lin_flg_msterr = U1L_LIN_FLG_OFF;
}

/***** End of File *****/