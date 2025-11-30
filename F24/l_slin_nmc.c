/*""FILE COMMENT""********************************************/
/* System Name : NM Module for S930-LSLibRL78F24xx           */
/* File Name   : l_slin_nmc.c                                */
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

#include "l_slin_user.h"
#include "l_slin_core_rl78f24.h"
#include "l_slin_drv_rl78f24.h"

#define U1L_LIN_FLG_OFF             ((l_u8)0U)
#define U1L_LIN_FLG_ON              ((l_u8)1U)

#define U1L_LIN_SLPIND_SET          ((l_u8)0x80U)   /* SLEEP_INDビットのセット用 */
#define U1L_LIN_WUPIND_SET          ((l_u8)0x40U)   /* WAKEUP_INDビットのセット用 */

/* スレーブタスク用変数 */
static l_u8  u1l_lin_mod_slvstat;                   /* NMスレーブステータス */
static l_u16 u2l_lin_tmr_slvinit;                   /* Tslv_initカウンタ */
static l_u16 u2l_lin_tmr_wurty;                     /* Twurtyカウンタ */
static l_u16 u2l_lin_tmr_3brk;                      /* Tt3brksカウンタ */
static l_u16 u2l_lin_cnt_retry;                     /* Retryカウンタ */
static l_u8  u1l_lin_wup_ind;                       /* Wakeup.ind情報フラグ */
static l_u8  u1l_lin_cnt_msterr;                    /* マスタ監視用カウンタ */
static l_u8  u1l_lin_flg_msterr;                    /* マスタ異常発生フラグ */

static l_u8  u1l_lin_wup_seq;                       /* ウェイクアップ再送シーケンス */
static l_u8  u1l_lin_wup_en;                        /* ウェイクアップ監視許可（WAKE時用） */

/*-- 関数定義(static) --*/
static void l_vol_lin_nm_reset(void);
static void l_vol_lin_nm_sleep(l_u8 u1a_lin_slp_req);
static void l_vol_lin_nm_run_standby(l_u8 u1a_lin_slp_req);
static void l_vol_lin_nm_run_stb_ope_wake(void);
static void l_vol_lin_nm_run_stb_ope_wake_wait(l_u8 u1a_lin_slp_req);
static void l_vol_lin_nm_run(l_u8 u1a_lin_slp_req);

/****************************************************************************/
/*  NM用Tick処理API                                                         */
/*--------------------------------------------------------------------------*/
/*  引数：l_u8 u1a_lin_slp_req                                              */
/*          U1G_LIN_SLP_REQ_OFF(0)   :SLEEP要求無し、又はWAKEUP要因有り     */
/*          U1G_LIN_SLP_REQ_ON(1)    :SLEEP要求有り、又はWAKEUP要因無し     */
/*          U1G_LIN_SLP_REQ_FORCE(2) :強制SLEEP要求有り                     */
/*  戻値：なし                                                              */
/****************************************************************************/
void l_nm_tick(l_u8 u1a_lin_slp_req)
{
    l_u8  u1a_lin_stat;
    l_u8  u1a_lin_tmp_nm_dat;

    u1a_lin_tmp_nm_dat = U1G_LIN_BYTE_CLR;

    /* LINステータスのリード */
    u1a_lin_stat = (l_u8)l_ifc_read_lb_status();

    switch( u1a_lin_stat ) {
    /* LINステータスがRESET状態 */
    case U1G_LIN_STS_RESET:
        l_vol_lin_nm_reset();                           /* LINステータス=RESET時処理 */
        break;
    /* LINステータスがSLEEP状態 */
    case U1G_LIN_STS_SLEEP:
        l_vol_lin_nm_sleep(u1a_lin_slp_req);            /* LINステータス=SLEEP時処理 */
        break;
    /* LINステータスがRUN_STANDBY状態 */
    case U1G_LIN_STS_RUN_STANDBY:
        l_vol_lin_nm_run_standby(u1a_lin_slp_req);      /* LINステータス=RUN STANDBY時処理 */
        break;
    /* LINステータスがRUN状態 */
    case U1G_LIN_STS_RUN:
        l_vol_lin_nm_run(u1a_lin_slp_req);              /* LINステータス=RUN時処理 */
        break;
    default:
        /* 通常は発生しない処理（LINステータス異常） */
        u1g_lin_syserr = U1G_LIN_SYSERR_STAT;           /* システム異常(ステータス) */
        break;
    }

    /* LINライブラリのNM情報書換え処理 */
    /* Data.indについてはユーザによりセットする */
    u1a_lin_tmp_nm_dat = U1G_LIN_BYTE_CLR;
    if( U1G_LIN_SLP_REQ_ON == u1a_lin_slp_req ) {
        /* Sleep.indビットのセット */
        u1a_lin_tmp_nm_dat |= U1L_LIN_SLPIND_SET;
    }
    if( U1L_LIN_FLG_ON == u1l_lin_wup_ind ) {
        /* Wakeup.indビットのセット */
        u1a_lin_tmp_nm_dat |= U1L_LIN_WUPIND_SET;
    }
    /* NM情報書換え関数のコール */
    l_vog_lin_set_nm_info(u1a_lin_tmp_nm_dat);
}

/****************************************************************************/
/*  NM用Tick処理（LINステータス=RESET時）                                   */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：なし                                                              */
/****************************************************************************/
static void l_vol_lin_nm_reset(void)
{
    switch( u1l_lin_mod_slvstat ) {
    /* LINステータスがRESET状態 / NMスレーブステータスがINI_WAIT状態 */
    case U1G_LIN_NM_INI_WAIT:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            l_ifc_connect();                            /* LIN接続  LINステータス→SLEEP */
            l_ifc_run();                                /* RUN状態移行 LINステータス→RUN_STANDBY */

            /***  INITIALIZING -> OPERATIONAL  ACTIVE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_ACTIVE;
        }
        break;
    /* LINステータスがRESET状態 / NMスレーブステータスが */
    /* INI_WAKE、INI_OTH_WAKE、OPE_WAKE_WAIT、OPE_WAKE   */
    /* OPE_OTH_WAKE、OPE_ACTIVE状態                      */
    case U1G_LIN_NM_INI_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_INI_OTH_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_WAKE_WAIT:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_OTH_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_ACTIVE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
        u1l_lin_wup_ind = U1L_LIN_FLG_OFF;

        /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
        break;
    /* LINステータスがRESET状態 / NMスレーブステータスがBS_SLEEP状態 */
    case U1G_LIN_NM_BS_SLEEP:
        break;
    default:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
        u1l_lin_wup_ind = U1L_LIN_FLG_OFF;

        /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
        break;
    }
}

/****************************************************************************/
/*  NM用Tick処理（LINステータス=SLEEP時）                                   */
/*--------------------------------------------------------------------------*/
/*  引数：l_u8 u1a_lin_slp_req                                              */
/*          U1G_LIN_SLP_REQ_OFF(0)   :SLEEP要求無し、又はWAKEUP要因有り     */
/*          U1G_LIN_SLP_REQ_ON(1)    :SLEEP要求有り、又はWAKEUP要因無し     */
/*          U1G_LIN_SLP_REQ_FORCE(2) :強制SLEEP要求有り                     */
/*  戻値：なし                                                              */
/****************************************************************************/
static void l_vol_lin_nm_sleep(l_u8 u1a_lin_slp_req)
{
    switch( u1l_lin_mod_slvstat ) {
    /* LINステータスがSLEEP状態 / NMスレーブステータスがINI_WAIT状態 */
    case U1G_LIN_NM_INI_WAIT:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            l_ifc_run();                                /* RUN状態移行 LINステータス→RUN_STANDBY */
            /***  INITIALIZING -> OPERATIONAL  ACTIVE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_ACTIVE;
        }
        break;
    /* LINステータスがSLEEP状態 / NMスレーブステータスが */
    /* INI_OTH_WAKE、OPE_WAKE_WAIT、OPE_WAKE、           */
    /* OPE_OTH_WAKE、OPE_ACTIVE状態                      */
    case U1G_LIN_NM_INI_OTH_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_WAKE_WAIT:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_OTH_WAKE:
        /* NMステータス異常  フェールセーフとしてBS_SLEEPに遷移 */
    case U1G_LIN_NM_OPE_ACTIVE:
        /* SLEEPコマンド受信  BS_SLEEP に遷移（処理内容は同じ） */
        u1l_lin_wup_ind = U1L_LIN_FLG_OFF;

        /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
        break;
    /* LINステータスがSLEEP状態 / NMスレーブステータスがINI_WAKE状態 */
    case U1G_LIN_NM_INI_WAKE:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT_WAKE <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            u1l_lin_wup_ind     = U1L_LIN_FLG_ON;       /* wakeup.ind ON */
            /* ウェイクアップ送信開始 */
            u2l_lin_tmr_wurty   = U2G_LIN_WORD_CLR;     /* TWURTY タイムカウントクリア */
            u2l_lin_cnt_retry   = U2G_LIN_WORD_CLR;     /* Wakeup送信回数クリア */
            u1l_lin_wup_seq     = U1L_LIN_WUP_WUARTY;   /* Wakeup再送シーケンス TWUARTYカウントへ */
            u1l_lin_wup_en      = U1L_LIN_FLG_OFF;      /* ウェイクアップの監視禁止 */

            l_ifc_wake_up();                            /* ウェイクアップ送信 1回目*/

            /***  INITIALIZING -> OPERATIONAL  WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_WAKE;
        }
        break;
    /* LINステータスがSLEEP状態 / NMスレーブステータスがBS_SLEEP状態 */
    case U1G_LIN_NM_BS_SLEEP:
        if( U1G_LIN_SLP_REQ_OFF == u1a_lin_slp_req ) {
            /** wakeup要因成立 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLVINITカウントクリア */
            u1l_lin_wup_seq     = U1L_LIN_WUP_NONE;     /* ウェイクアップ信号送信終了（明示） */

            /***  BUS SLEEP MODE -> INITIALIZING  WAKE_INI 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_INI_WAKE;
        }
        break;
    default:
        /* NMステータス異常   フェールセーフとして、Bus sleep modeに遷移 */
        u1l_lin_wup_ind = U1L_LIN_FLG_OFF;

        /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
        break;
    }
}

/****************************************************************************/
/*  NM用Tick処理（LINステータス=RUN_STANDBY時）                             */
/*--------------------------------------------------------------------------*/
/*  引数：l_u8 u1a_lin_slp_req                                              */
/*          U1G_LIN_SLP_REQ_OFF(0)   :SLEEP要求無し、又はWAKEUP要因有り     */
/*          U1G_LIN_SLP_REQ_ON(1)    :SLEEP要求有り、又はWAKEUP要因無し     */
/*          U1G_LIN_SLP_REQ_FORCE(2) :強制SLEEP要求有り                     */
/*  戻値：なし                                                              */
/****************************************************************************/
static void l_vol_lin_nm_run_standby(l_u8 u1a_lin_slp_req)
{
    switch( u1l_lin_mod_slvstat ) {
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがINI_WAIT状態 */
    case U1G_LIN_NM_INI_WAIT:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            /***  INITIALIZING -> OPERATIONAL  ACTIVE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_ACTIVE;
        }
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがINI_WAKE状態 */
    case U1G_LIN_NM_INI_WAKE:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT_WAKE <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;     /* T3BRKカウンタクリア */
            /***  INITIALIZING -> OPERATIONAL  OTH_WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
        }
        else{
            u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;     /* T3BRKカウンタクリア */
            /***  INITIALIZING INI_WAKE -> INI_OTH_WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_INI_OTH_WAKE;
        }
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがINI_OTH_WAKE状態 */
    case U1G_LIN_NM_INI_OTH_WAKE:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        u2l_lin_tmr_3brk    += U2G_LIN_NM_TIME_BASE;    /* T3BRKカウント */
        if( U2G_LIN_TM_SLVINIT_WAKE <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            /***  INITIALIZING -> OPERATIONAL  OTH_WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
        }
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがOPE_WAKE_WAIT状態 */
    case U1G_LIN_NM_OPE_WAKE_WAIT:
        l_vol_lin_nm_run_stb_ope_wake_wait(u1a_lin_slp_req);    /* OPE_WAKE_WAIT処理 */
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがOPE_WAKE状態 */
    case U1G_LIN_NM_OPE_WAKE:
        /* 定常処理 */
        l_vol_lin_nm_run_stb_ope_wake();               /* OPE_WAKE処理 */
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがOPE_OTH_WAKE状態 */
    case U1G_LIN_NM_OPE_OTH_WAKE:
        u2l_lin_tmr_3brk += U2G_LIN_NM_TIME_BASE;       /* TT3BRKカウント */

        /* T3BRK待ち継続 */
        if( U2G_LIN_TM_3BRKS <= u2l_lin_tmr_3brk ) {
            /** T3BRK経過 **/
            u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;        /* TT3BRK タイムカウントクリア */
            u1l_lin_cnt_msterr++;                       /* マスタ監視用カウンタインクリメント */
            if( U1G_LIN_CNT_MSTERR <= u1l_lin_cnt_msterr ) {
                u1l_lin_flg_msterr = U1L_LIN_FLG_ON;    /* マスタ異常フラグセット */
                u1l_lin_cnt_msterr = U1G_LIN_BYTE_CLR;  /* マスタ異常カウントクリア */
            }

            l_vog_lin_clr_wupflg();                     /* coreのウェイクアップ検出フラグクリア */
            l_vog_lin_set_int_enb();                    /* INT割り込み許可 */

            /***   -> WAKE-WAIT 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_WAKE_WAIT;
        }
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがOPE_ACTIVE状態 */
    case U1G_LIN_NM_OPE_ACTIVE:
        if( U1G_LIN_SLP_REQ_FORCE == u1a_lin_slp_req ) {
            l_vod_lin_DI();                             /* 割り込み禁止設定 */
            if( U1G_LIN_BIT_SET == F1g_lin_bus_inactive ){
                /** Bus inactive検出 **/
                l_ifc_sleep();                          /* LINステータス→Sleep */
                F1g_lin_bus_inactive = U1G_LIN_BIT_CLR; /* Bus Inactiveフラグクリア */
                u1l_lin_wup_ind = U1L_LIN_FLG_OFF;      /* wakeup.ind OFF */

                /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
                u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
            }
            l_vod_lin_EI();                             /* 割り込み許可 */
        }
        break;
    /* LINステータスがRUN_STANDBY状態 / NMスレーブステータスがBS_SLEEP状態 */
    case U1G_LIN_NM_BS_SLEEP:
        /** wakeup受信 **/
        u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;         /* TSLVINITカウントクリア */
        u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;         /* T3BRKカウンタクリア */

        /***  BUS SLEEP MODE -> INITIALIZING  INI_OTH_WAKE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_INI_OTH_WAKE;
        break;
    default:
        /* NMステータス異常  フェールセーフとしてOPE_OTH_WAKEに遷移  */
        u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;         /* TSLVINITカウントクリア */
        u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;         /* T3BRKカウンタクリア */

        /***   -> OTH_WAKE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
        break;
    }
}

/****************************************************************************/
/*  NM用Tick処理（LINステータス=RUN_STANDBY  NMステータス=OPE_WAKE_WAIT時） */
/*--------------------------------------------------------------------------*/
/*  引数：l_u8 u1a_lin_slp_req                                              */
/*          U1G_LIN_SLP_REQ_OFF(0)   :SLEEP要求無し、又はWAKEUP要因有り     */
/*          U1G_LIN_SLP_REQ_ON(1)    :SLEEP要求有り、又はWAKEUP要因無し     */
/*          U1G_LIN_SLP_REQ_FORCE(2) :強制SLEEP要求有り                     */
/*  戻値：なし                                                              */
/****************************************************************************/
static void l_vol_lin_nm_run_stb_ope_wake_wait(l_u8 u1a_lin_slp_req)
{
    l_u8    u1a_lin_wup_flg;

    u1a_lin_wup_flg = l_u1g_lin_get_wupflg();       /* coreのウェイクアップ検出フラグ取得 */
    if( U1G_LIN_WUP_DET == u1a_lin_wup_flg ) {
        /** wakeup受信 **/
        u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLVINITカウントクリア */
        u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;     /* T3BRKカウンタクリア */

        /***   -> OTH_WAKE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
    }
    else if( U1G_LIN_SLP_REQ_OFF == u1a_lin_slp_req ) {
        /** wakeup要因成立 **/
        u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLVINITカウントクリア */
        u1l_lin_wup_ind     = U1L_LIN_FLG_ON;       /* wakeup.ind ON（再送） */

        /* ウェイクアップ送信開始 */
        u2l_lin_tmr_wurty   = U2G_LIN_WORD_CLR;     /* TWURTY タイムカウントクリア */
        u2l_lin_cnt_retry   = U2G_LIN_WORD_CLR;     /* Wakeup送信回数クリア */
        u1l_lin_wup_seq     = U1L_LIN_WUP_WUARTY;   /* Wakeup再送シーケンス TWUARTYカウントへ */
        u1l_lin_wup_en      = U1L_LIN_FLG_OFF;      /* ウェイクアップの監視禁止 */

        l_ifc_wake_up();                            /* ウェイクアップ送信 1回目*/

        /***   -> WAKE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_WAKE;
    }
    else if( U1G_LIN_SLP_REQ_FORCE == u1a_lin_slp_req )
    {
        l_vod_lin_DI();                             /* 割り込み禁止設定 */
        if( U1G_LIN_BIT_SET == F1g_lin_bus_inactive ){
            /** Bus inactive検出 **/
            l_ifc_sleep();                          /* LINステータス→Sleep */
            F1g_lin_bus_inactive = U1G_LIN_BIT_CLR; /* Bus Inactiveフラグクリア */
            u1l_lin_wup_ind = U1L_LIN_FLG_OFF;      /* wakeup.ind OFF */

            /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
        }
        l_vod_lin_EI();                             /* 割り込み許可 */
    }
    else{
        /* 定常処理  なし */
    }
}

/****************************************************************************/
/*  NM用Tick処理（LINステータス=RUN_STANDBY  NMステータス=OPE_WAKE時）      */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：なし                                                              */
/****************************************************************************/
static void l_vol_lin_nm_run_stb_ope_wake(void)
{
    l_u8 u1a_lin_wup_flg;

    /*** ウェイクアップ信号送信シーケンス遂行   ***/
    if( U1L_LIN_WUP_WUARTY == u1l_lin_wup_seq ) {
        /*** 再送制御 ***/
        if( U2G_LIN_CNT_RETRY > u2l_lin_cnt_retry ) {
            u2l_lin_tmr_wurty += U2G_LIN_NM_TIME_BASE;          /* TWURTYカウンタインクリメント */
            if( U2G_LIN_TM_WURTY <= u2l_lin_tmr_wurty ) {
                l_ifc_wake_up();                                /* ウェイクアップ再送 */
                u2l_lin_cnt_retry++;                            /* ウェイクアップ再送回数インクリメント */
                u2l_lin_tmr_wurty   = U2G_LIN_WORD_CLR;
                /* 最後の送信からTWURTY後にウェイクアップ検出を監視する */
                if( U2G_LIN_CNT_RETRY <= u2l_lin_cnt_retry ) {
                    /* 最後の送信完了 T3BRK待ちへ移行 */
                    u2l_lin_cnt_retry   = U2G_LIN_WORD_CLR;     /* Wakeup送信回数クリア */
                    u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR ;    /* TT3BRK タイムカウントクリア */
                    u1l_lin_wup_seq     = U1L_LIN_WUP_T3BRK;    /* Wakeup再送シーケンス TT3BRKカウントへ */
                }
            }
        }
        else{
            /* ウェイクアップ信号送信シーケンス異常  T3BRK待ちにする */
            u2l_lin_cnt_retry   = U2G_LIN_WORD_CLR;             /* Wakeup送信回数クリア */
            u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR ;            /* TT3BRK タイムカウントクリア */
            u1l_lin_wup_seq     = U1L_LIN_WUP_T3BRK;            /* Wakeup再送シーケンス TT3BRKカウントへ */
        }
    }
    else if( U1L_LIN_WUP_T3BRK == u1l_lin_wup_seq ) {
        /*** T3BRKカウント   TWURTY経過後からウェイクアップの検出を許可。検出したらOTHER-WAKEに遷移する。 ***/
        u2l_lin_tmr_3brk += U2G_LIN_NM_TIME_BASE;
        /* ウェイクアップ検出の確認（OPE_OTH_WAKE遷移条件） */
        if( U1L_LIN_FLG_ON == u1l_lin_wup_en ) {
            u1a_lin_wup_flg = l_u1g_lin_get_wupflg();           /* coreのウェイクアップ検出フラグ取得 */
            if( U1G_LIN_WUP_DET == u1a_lin_wup_flg ) {
                /** wakeup受信 **/
                u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;         /* T3BRKカウンタクリア */
                u2l_lin_tmr_wurty   = U2G_LIN_WORD_CLR;

                /***   -> 他WAKE 遷移  ***/
                u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
            }
        }
        else{
            u2l_lin_tmr_wurty += U2G_LIN_NM_TIME_BASE;
            if( U2G_LIN_TM_WURTY <= u2l_lin_tmr_wurty ) {
                u1l_lin_wup_en = U1L_LIN_FLG_ON;                /* ウェイクアップの監視開始 */
                l_vog_lin_clr_wupflg();                         /* coreのウェイクアップ検出フラグクリア */
                l_vog_lin_set_int_enb();                        /* INT割り込み許可 */
                u2l_lin_tmr_wurty   = U2G_LIN_WORD_CLR;
            }
        }

        /* T3BRK経過判定  ※上部のウェイクアップ検出→他WAKE遷移時はT3BRKカウンタをクリアすることで排他とします */
        if( U2G_LIN_TM_3BRKS <= u2l_lin_tmr_3brk ) {
            /** T3BRK経過 **/
            u2l_lin_tmr_3brk = U2G_LIN_WORD_CLR;                /* TT3BRK タイムカウントクリア */
            u1l_lin_wup_seq  = U1L_LIN_WUP_NONE;                /* ウェイクアップ信号送信シーケンス一巡 */

            u1l_lin_cnt_msterr++;                               /* マスタ監視用カウンタインクリメント */
            if( U1G_LIN_CNT_MSTERR <= u1l_lin_cnt_msterr ) {
                u1l_lin_flg_msterr = U1L_LIN_FLG_ON;            /* マスタ異常フラグセット */
                u1l_lin_cnt_msterr = U1G_LIN_BYTE_CLR;          /* マスタ異常カウントクリア */
            }

            /***   -> WAKE-WAIT 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_WAKE_WAIT;
        }
    }
    else{
        /* ウェイクアップ信号送信シーケンス異常  T3BRK待ちにする */
        u2l_lin_cnt_retry   = U2G_LIN_WORD_CLR;                 /* Wakeup送信回数クリア */
        u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR ;                /* TT3BRK タイムカウントクリア */
        u1l_lin_wup_seq     = U1L_LIN_WUP_T3BRK;                /* Wakeup再送シーケンス TT3BRKカウントへ */
    }
}

/****************************************************************************/
/*  NM用Tick処理（LINステータス=RUN時）                                     */
/*--------------------------------------------------------------------------*/
/*  引数：l_u8 u1a_lin_slp_req                                              */
/*          U1G_LIN_SLP_REQ_OFF(0)   :SLEEP要求無し、又はWAKEUP要因有り     */
/*          U1G_LIN_SLP_REQ_ON(1)    :SLEEP要求有り、又はWAKEUP要因無し     */
/*          U1G_LIN_SLP_REQ_FORCE(2) :強制SLEEP要求有り                     */
/*  戻値：なし                                                              */
/****************************************************************************/
static void l_vol_lin_nm_run(l_u8 u1a_lin_slp_req)
{
    switch( u1l_lin_mod_slvstat ) {
    /* LINステータスがRUN状態 / NMスレーブステータスがINI_WAIT状態 */
    case U1G_LIN_NM_INI_WAIT:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            /***  INITIALIZING -> OPERATIONAL  ACTIVE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_ACTIVE;
        }
        break;
    /* LINステータスがRUN状態 / NMスレーブステータスがINI_WAKE状態 */
    case U1G_LIN_NM_INI_WAKE:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        if( U2G_LIN_TM_SLVINIT_WAKE <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;     /* T3BRKカウンタクリア */
            /***  INITIALIZING -> OPERATIONAL  OTH_WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
        }
        else{
            u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;     /* T3BRKカウンタクリア */
            /***  -> INI_OTH_WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_INI_OTH_WAKE;
        }
        break;
    /* LINステータスがRUN状態 / NMスレーブステータスがINI_OTH_WAKE状態 */
    case U1G_LIN_NM_INI_OTH_WAKE:
        u2l_lin_tmr_slvinit += U2G_LIN_NM_TIME_BASE;    /* TSLV_INITカウント */
        u2l_lin_tmr_3brk    += U2G_LIN_NM_TIME_BASE;    /* T3BRKカウント */
        if( U2G_LIN_TM_SLVINIT_WAKE <= u2l_lin_tmr_slvinit ) {
            /** TSLV_INIT経過 **/
            u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLV_INITカウンタクリア */
            /***  INITIALIZING -> OPERATIONAL  OTH_WAKE 遷移  ***/
            u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_OTH_WAKE;
        }
        break;
    /* LINステータスがRUN状態 / NMスレーブステータスが */
    /* OPE_WAKE_WAIT、OPE_WAKE、OPE_OTH_WAKE状態       */
    case U1G_LIN_NM_OPE_WAKE_WAIT:
        /* NMステータス異常  フェールセーフとしてOPE  ACTIVEに遷移（処理内容は同じ） */
    case U1G_LIN_NM_OPE_WAKE:
        /* スケジュール検出  OPE  ACTIVE に遷移 */
    case U1G_LIN_NM_OPE_OTH_WAKE:
        /* スケジュール検出  OPE  ACTIVE に遷移 */
        u1l_lin_wup_seq     = U1L_LIN_WUP_NONE;     /* ウェイクアップ信号送信終了 */

        u1l_lin_cnt_msterr = U1G_LIN_BYTE_CLR;      /* マスタ監視用カウンタクリア */
        u1l_lin_flg_msterr = U1L_LIN_FLG_OFF;       /* マスタ異常フラグクリア */
        /***   -> ACTIVE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_ACTIVE;
        break;
    /* LINステータスがRUN状態 / NMスレーブステータスがOPE_ACTIVE状態 */
    case U1G_LIN_NM_OPE_ACTIVE:
        if( U1G_LIN_SLP_REQ_FORCE == u1a_lin_slp_req ){
            l_vod_lin_DI();                             /* 割り込み禁止設定 */
            if( U1G_LIN_BIT_SET == F1g_lin_bus_inactive ){
                /** Bus inactive検出 **/
                l_ifc_sleep();                          /* LINステータス→Sleep */
                F1g_lin_bus_inactive = U1G_LIN_BIT_CLR; /* Bus Inactiveフラグクリア */
                u1l_lin_wup_ind = U1L_LIN_FLG_OFF;      /* wakeup.ind OFF */

                /***  OPERATIONAL -> BUS SLEEP MODE 遷移  ***/
                u1l_lin_mod_slvstat = U1G_LIN_NM_BS_SLEEP;
            }
            l_vod_lin_EI();                             /* 割り込み許可 */
        }
        break;
    /* LINステータスがRUN状態 / NMスレーブステータスがBS_SLEEP状態 */
    case U1G_LIN_NM_BS_SLEEP:
        /** wakeup受信 **/
        u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;     /* TSLVINITカウントクリア */
        u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;     /* T3BRKカウンタクリア */

        /***  BUS SLEEP MODE -> INITIALIZING  OTH_WAKE_INI 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_INI_OTH_WAKE;
        break;
    default:
        /* NMステータス異常  フェールセーフとしてOPE_ACTIVEに遷移 */
        u1l_lin_wup_seq     = U1L_LIN_WUP_NONE;     /* ウェイクアップ信号送信終了 */

        u1l_lin_cnt_msterr  = U1G_LIN_BYTE_CLR;     /* マスタ監視用カウンタクリア */
        u1l_lin_flg_msterr  = U1L_LIN_FLG_OFF;      /* マスタ異常フラグクリア */
        /***   -> ACTIVE 遷移  ***/
        u1l_lin_mod_slvstat = U1G_LIN_NM_OPE_ACTIVE;
        break;
    }
}


/****************************************************************************/
/*  NM用初期化処理API                                                       */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：なし                                                              */
/****************************************************************************/
void l_nm_init(void)
{
/* スレーブタスク用変数 */
    u1l_lin_mod_slvstat = U1G_LIN_NM_INI_WAIT;
    u2l_lin_tmr_slvinit = U2G_LIN_WORD_CLR;

    u2l_lin_tmr_wurty   = U2G_LIN_WORD_CLR;
    u2l_lin_tmr_3brk    = U2G_LIN_WORD_CLR;
    u2l_lin_cnt_retry   = U2G_LIN_WORD_CLR;
    u1l_lin_wup_ind     = U1L_LIN_FLG_OFF;
    u1l_lin_cnt_msterr  = U1G_LIN_BYTE_CLR;
    u1l_lin_flg_msterr  = U1L_LIN_FLG_OFF;

    u1l_lin_wup_seq     = U1L_LIN_WUP_NONE;
    u1l_lin_wup_en      = U1L_LIN_FLG_OFF;
}


/****************************************************************************/
/*  NMスレーブステータスリードAPI                                           */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：l_u8  NMスレーブステータス                                        */
/****************************************************************************/
l_u8 l_nm_rd_slv_stat(void)
{
    return (u1l_lin_mod_slvstat);
}


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/****************************************************************************/
/*  マスタ異常フラグのリードAPI                                             */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：l_u8  0：マスタ異常なし, 1:マスタ異常あり                         */
/****************************************************************************/
l_u8 l_nm_rd_mst_err(void)
{
    return (u1l_lin_flg_msterr);
}
#endif


#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
/****************************************************************************/
/*  マスタ異常フラグのクリアAPI                                             */
/*--------------------------------------------------------------------------*/
/*  引数：なし                                                              */
/*  戻値：なし                                                              */
/****************************************************************************/
void l_nm_clr_mst_err(void)
{
    u1l_lin_flg_msterr = U1L_LIN_FLG_OFF;
}
#endif

/***** End of File *****/



