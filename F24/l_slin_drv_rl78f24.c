/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_drv_rl78f24.c                        */
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
#include "l_slin_sfr.h"
#include "l_slin_core_rl78f24.h"
#include "l_slin_drv_rl78f24.h"
#include "aipf_cpu.h"


/***** 関数プロトタイプ宣言 *****/
/*-- その他 MCU依存関数(static) --*/
static l_u8  l_u1l_lin_linmodule_set_reset(void);

/*** 変数(static) ***/


/**************************************************/

/********************************/
/* MCU依存のAPI関数処理         */
/********************************/
/**************************************************/
/*  LINモジュールの初期化 処理                    */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8  l_u1g_lin_linmodule_init(void)
{
    l_u16   u2a_lin_lp_cnt;
    l_u8    u1a_lin_result;
    l_u8    u1a_lin_tmp_lmst;

    u1a_lin_result = U1G_LIN_OK;

    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_CSC & AIPF_CPU_SFRGRD_ID_INT );/* SFRガード機能 無効化(GCSC、GINT) */

    /* クロックを供給 */
    U1G_LIN_SFR_BIT_LINEN   = U1G_LIN_BIT_SET;                      /* LINnモジュールへのクロック供給 */
    U1G_LIN_SFR_BIT_LINMCK  = U1G_LIN_LINMCK_SET;                   /* LINnモジュールへ供給するクロック選択 */
    U1G_LIN_SFR_BIT_LINMCKE = U1G_LIN_BIT_SET;                      /* LINnエンジンへのクロック供給 */

    U1G_LIN_SFR_LCHSEL = U1G_LIN_LCHSEL;                            /* LINチャネル選択設定 */

    u1a_lin_tmp_lmst = U1G_LIN_SFR_LMST;
    if( U1G_LIN_LMST_RESET != (u1a_lin_tmp_lmst & U1G_LIN_LMST_RESET_MASK) ) {
        /* リセットモードでなければ、リセットモードへ遷移 */
        U1G_LIN_SFR_LCUC    = U1G_LIN_LCUC_RESET;
        u2a_lin_lp_cnt      = U2G_LIN_0;

        /* リセットモード遷移待ち */
        while( (U2G_LIN_MODE_WAIT2_CNT > u2a_lin_lp_cnt)
            && ( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) ) {
            u2a_lin_lp_cnt++;
        }
        if( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) {
            /* モード遷移失敗 */
            u1a_lin_result = U1G_LIN_NG;
        }
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
        /* オート・ボー・レート時は処理なし */
#else
        else{
            if( U1G_LIN_LMST_RUN == (u1a_lin_tmp_lmst & U1G_LIN_LMST_MASK) ) {
                U1G_LIN_SFR_BIT_LINEN = U1G_LIN_BIT_CLR;            /* LINnモジュールクロック停止 */
                U1G_LIN_SFR_BIT_LINEN = U1G_LIN_BIT_SET;            /* LINnモジュールクロック供給 */
            }
        }
#endif /* U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON */
    }

    if( U1G_LIN_OK == u1a_lin_result ) {
        U1G_LIN_SFR_LWBR = U1G_LIN_LWBR_INIT;                       /* ウェイクアップ、ボーレート設定  2byteのLBRPレジスタの分周で対応可能なため、分周しない */
        U2G_LIN_SFR_LBRP = U2G_LIN_LBRP_INIT;                       /* ボーレートプリスケーラ（リセットモード時に設定） */
        U1G_LIN_SFR_LMD  = U1G_LIN_LMD_INIT;                        /* LINモード スレーブ（固定ボーレート） */
        U1G_LIN_SFR_LBFC = U1G_LIN_LBFC_INIT;                       /* Break検出幅選択 9.5Tbits */
        U1G_LIN_SFR_LSC  = U1G_LIN_LSC_INIT;                        /* LINスペース設定（RS、BS） */
        U1G_LIN_SFR_LWUP = U1G_LIN_SFR_LWUP_INIT;                   /* ウェイクアップ送信幅設定 */
        U1G_LIN_SFR_LIE  = U1G_LIN_LIE_INIT;                        /* 割り込み禁止（リセットモード時に設定） */
        U1G_LIN_SFR_LEDE = U1G_LIN_LEDE_INIT;                       /* エラー検出許可設定（リセットモード時に設定） */
    }

    U1G_LIN_SFR_BIT_LINTRMIF   = U1G_LIN_BIT_CLR;                   /* LINn送信割り込み要求フラグクリア */
    U1G_LIN_SFR_BIT_LINTRMMK   = U1G_LIN_BIT_SET;                   /* LINn送信割り込み禁止(マスク) */
    U1G_LIN_SFR_BIT_LINRVCIF   = U1G_LIN_BIT_CLR;                   /* LINn受信割り込み要求フラグクリア */
    U1G_LIN_SFR_BIT_LINRVCMK   = U1G_LIN_BIT_SET;                   /* LINn受信割り込み割り込み禁止(マスク) */
    U1G_LIN_SFR_BIT_LINSTAIF   = U1G_LIN_BIT_CLR;                   /* LINnステータスエラー割り込み要求フラグクリア */
    U1G_LIN_SFR_BIT_LINSTAMK   = U1G_LIN_BIT_SET;                   /* LINnステータスエラー 割り込み禁止(マスク) */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */
    
    l_vod_lin_EI();                                                 /* 割り込み許可 */

    return( u1a_lin_result );
}

/**************************************************/
/*  TAUモジュールの初期化 処理                    */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_timer_init(void)
{
    l_u16   u2a_lin_tau_tps;

    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_CSC & AIPF_CPU_SFRGRD_ID_INT );/* SFRガード機能 無効化(GCSC、GINT) */

    U1G_LIN_SFR_BIT_TAUEN   = U1G_LIN_BIT_SET;                      /* TAUnモジュールへのクロック供給し、TAUmENを有効化する */

    U1G_LIN_TAU_TT          = U1G_LIN_TT_STOP;                      /* カウント動作を停止する */

    u2a_lin_tau_tps         = U2G_LIN_TAU_TPS;                      /* 自動変数にTPSレジスタの値をコピーする */
    u2a_lin_tau_tps         &= (~U2G_LIN_TPS_CLR);                  /* 使用チャネル用設定ビット 明示クリア */
    u2a_lin_tau_tps         |= U2G_LIN_TPS_INIT;                    /* チャネル n の設定値を書き込む */
    U2G_LIN_TAU_TPS         = u2a_lin_tau_tps;                      /* クロック周波数を設定する */

    U2G_LIN_TAU_TMR = U2G_LIN_TMR_INIT;                             /* タイマ・モードを設定する */

    /* カウンタ値設定 */
    U2G_LIN_TAU_TDR = U2G_LIN_TDR_COUNT_VAL;                        /* TDR レジスタへの格納 */

    U1G_LIN_TAU_TO  &= (l_u8)(~U1G_LIN_TO_INIT);                    /* タイマ出力値を 0 にする */
    U1G_LIN_TAU_TOE &= (l_u8)(~U1G_LIN_TOE_INIT);                   /* タイマの出力を禁止する */

    U1G_LIN_TAU_BIT_TMPR0 =  U1G_LIN_TMPR_PR0;                      /* PR01L.TMPR000 bit, 割り込み優先順位指定フラグ XXPR0X */
    U1G_LIN_TAU_BIT_TMPR1 =  U1G_LIN_TMPR_PR1;                      /* PR11L.TMPR100 bit, 割り込み優先順位指定フラグ XXPR1X */
    U1G_LIN_TAU_BIT_TMIF    = U1G_LIN_BIT_CLR;                      /* 割り込み要求フラグをクリアする */
    U1G_LIN_TAU_BIT_TMMK    = U1G_LIN_BIT_SET;                      /* 割り込み処理を禁止する */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */

    l_vod_lin_EI();                                                 /* 割り込み許可 */

}

/**************************************************/
/*  TAUモジュールのタイマ開始 処理                */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_start_timer(void)
{
    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    U2G_LIN_TAU_TDR         = U2G_LIN_TDR_COUNT_VAL;                /* TDR レジスタへの格納 */

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );              /* SFRガード機能 無効化(GINT) */

    U1G_LIN_TAU_BIT_TMIF    = U1G_LIN_BIT_CLR;                      /* 割り込み要求フラグをクリアする */
    U1G_LIN_TAU_BIT_TMMK    = U1G_LIN_BIT_CLR;                      /* 割り込み処理を許可する */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */

    U1G_LIN_TAU_TS          = U1G_LIN_TS_START;                     /* タイマを開始する */

    l_vod_lin_EI();                                                 /* 割り込み許可 */

}

/**************************************************/
/*  TAUモジュールのタイマ終了 処理                */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_stop_timer(void)
{
    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    /* タイマ開始処理関数(l_vog_lin_start_timer())と対称的な処理順番になるよう設計 */
    U1G_LIN_TAU_TT          = U1G_LIN_TT_STOP;                      /* タイマを停止する */

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );              /* SFRガード機能 無効化(GINT) */

    U1G_LIN_TAU_BIT_TMMK    = U1G_LIN_BIT_SET;                      /* 割り込み処理を禁止する */
    U1G_LIN_TAU_BIT_TMIF    = U1G_LIN_BIT_CLR;                      /* 割り込み要求フラグをクリアする */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */

    l_vod_lin_EI();                                                 /* 割り込み許可 */

}

/***********************************/
/* MCU固有のSFR設定用関数処理      */
/***********************************/

/**************************************************/
/*  INTレジスタの初期化 処理                      */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_init(void)
{
    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );              /* SFRガード機能 無効化(GINT) */

    /* INT割り込み初期化 */
    U1G_LIN_SFR_BIT_ISC      = U1G_LIN_BIT_SET;                     /* LRxDnの入力を選択 */
    U1G_LIN_SFR_BIT_EGP      = U1G_LIN_BIT_CLR;                     /* 立ち上がりエッジ検出禁止 */
    U1G_LIN_SFR_BIT_EGN      = U1G_LIN_BIT_SET;                     /* 立ち下がりエッジ検出許可 */
    U1G_LIN_SFR_BIT_LINWUPMK = U1G_LIN_BIT_SET;                     /* INT割り込み禁止 */
    U1G_LIN_SFR_BIT_LINWUPIF = U1G_LIN_BIT_CLR;                     /* 割り込み要求フラグクリア */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */

    l_vod_lin_EI();                                                 /* 割り込み許可 */

}


/**************************************************/
/*  INT割り込み許可 処理                          */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_enb(void)
{
    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );              /* SFRガード機能 無効化(GINT) */
    
    U1G_LIN_SFR_BIT_ISC         = U1G_LIN_BIT_SET;                  /* LRxDnの入力を選択 */
    U1G_LIN_SFR_BIT_EGP         = U1G_LIN_BIT_CLR;                  /* 立ち上がりエッジ検出禁止 */
    U1G_LIN_SFR_BIT_EGN         = U1G_LIN_BIT_SET;                  /* 立ち下がりエッジ検出許可 */
    U1G_LIN_SFR_BIT_LINWUPPR0   = U1G_LIN_WUP_PR0;                  /* 割り込み優先度設定(2bit,下位) */
    U1G_LIN_SFR_BIT_LINWUPPR1   = U1G_LIN_WUP_PR1;                  /* 割り込み優先度設定(2bit,上位) */
    U1G_LIN_SFR_BIT_LINWUPIF    = U1G_LIN_BIT_CLR;                  /* 割り込み要求フラグクリア */
    U1G_LIN_SFR_BIT_LINWUPMK    = U1G_LIN_BIT_CLR;                  /* INT割り込み許可 */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */

    l_vod_lin_EI();                                                 /* 割り込み許可 */
}


/**************************************************/
/*  INT割り込み禁止 処理                          */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_dis(void)
{
    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    l_hook_wake();

    aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );              /* SFRガード機能 無効化(GINT) */

    U1G_LIN_SFR_BIT_LINWUPMK = U1G_LIN_BIT_SET;                     /* INT割り込み禁止 */
    U1G_LIN_SFR_BIT_LINWUPIF = U1G_LIN_BIT_CLR;                     /* 割り込み要求フラグクリア（明示） */

    aipf_cpu_Enable_SFRGrd();                                       /* SFRガード機能 有効化 */
    
    l_vod_lin_EI();                                                 /* 割り込み許可 */
}


/**************************************************/
/*  LINモジュールリセット状態設定処理             */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8  l_u1g_lin_set_reset(void)
{
    l_u8    u1a_lin_result;

    /* 割り込み禁止設定 */
    l_vod_lin_DI();

    u1a_lin_result = l_u1l_lin_linmodule_set_reset();       /* リセットモード遷移 */

    if( U1G_LIN_OK == u1a_lin_result ) {
        /* LIN割り込み/エラー検出禁止 */
        aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_CSC & AIPF_CPU_SFRGRD_ID_INT );/* SFRガード機能 無効化(GCSC、GINT) */
        
        U1G_LIN_SFR_LIE             = U1G_LIN_LIE_RESET;    /* 送信完了・受信完了・エラー検出割り込み禁止   */
        U1G_LIN_SFR_LEDE            = U1G_LIN_LEDE_RESET;   /* 全てのエラー検出禁止                         */
        /* LIN割り込み禁止 */
        U1G_LIN_SFR_BIT_LINTRMIF   = U1G_LIN_BIT_CLR;       /* LINn送信割り込み要求フラグクリア */
        U1G_LIN_SFR_BIT_LINTRMMK   = U1G_LIN_BIT_SET;       /* LINn送信割り込み禁止(マスク) */
        U1G_LIN_SFR_BIT_LINRVCIF   = U1G_LIN_BIT_CLR;       /* LINn受信割り込み要求フラグクリア */
        U1G_LIN_SFR_BIT_LINRVCMK   = U1G_LIN_BIT_SET;       /* LINn受信割り込み割り込み禁止(マスク) */
        U1G_LIN_SFR_BIT_LINSTAIF   = U1G_LIN_BIT_CLR;       /* LINnステータスエラー割り込み要求フラグクリア */
        U1G_LIN_SFR_BIT_LINSTAMK   = U1G_LIN_BIT_SET;       /* LINnステータスエラー 割り込み禁止(マスク) */

        /* クロックの供給を停止 */
        U1G_LIN_SFR_BIT_LINEN      = U1G_LIN_BIT_CLR;       /* LINnモジュールへのクロック供給 */
        U1G_LIN_SFR_BIT_LINMCKE    = U1G_LIN_BIT_CLR;       /* LINnエンジンへのクロック供給 */

        aipf_cpu_Enable_SFRGrd();                           /* SFRガード機能 有効化 */
    }

    /* 割り込み許可 */
    l_vod_lin_EI();

    return( u1a_lin_result );
}


/**************************************************/
/*  LINモジュールリセット状態遷移処理             */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
static l_u8  l_u1l_lin_linmodule_set_reset(void)
{
    l_u16   u2a_lin_lp_cnt;
    l_u8    u1a_lin_result;
    l_u8    u1a_lin_tmp_lmst;

    u1a_lin_result = U1G_LIN_OK;

    u1a_lin_tmp_lmst = U1G_LIN_SFR_LMST;
    if( U1G_LIN_LMST_RESET != (u1a_lin_tmp_lmst & U1G_LIN_LMST_RESET_MASK) ) {
        /* リセットモードでなければ、リセットモードへ遷移 */
        U1G_LIN_SFR_LCUC    = U1G_LIN_LCUC_RESET;
        u2a_lin_lp_cnt      = U2G_LIN_0;

        /* リセットモード遷移待ち */
        while( (U2G_LIN_MODE_WAIT2_CNT > u2a_lin_lp_cnt)
            && ( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) ) {
            u2a_lin_lp_cnt++;
        }
        if( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) {
            /* モード遷移失敗 */
            u1a_lin_result = U1G_LIN_NG;
        }
        else{
            if( U1G_LIN_LMST_RUN == (u1a_lin_tmp_lmst & U1G_LIN_LMST_MASK) ) {
                aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_CSC );  /* SFRガード機能 無効化(GCSC) */
                
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
                /* オート・ボー・レート時は処理なし */
#else
                U1G_LIN_SFR_BIT_LINEN = U1G_LIN_BIT_CLR;        /* LINnモジュールクロック停止 */
                U1G_LIN_SFR_BIT_LINEN = U1G_LIN_BIT_SET;        /* LINnモジュールクロック供給 */
#endif /* U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON */
                
                aipf_cpu_Enable_SFRGrd();                       /* SFRガード機能 有効化 */
                u1a_lin_result = l_u1g_lin_linmodule_init();    /* LINn関連レジスタを再設定 */
                l_vog_lin_timer_init();                         /* TAUモジュールの初期化処理 */
            }
        }
    }
    return u1a_lin_result;
}


/**************************************************/
/*  LINモジュールWakeup送信時設定処理             */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8  l_u1g_lin_set_sndwkup(void)
{
    l_u16   u2a_lin_lp_cnt;
    l_u8    u1a_lin_result;

    /* 割り込み禁止設定 */
    l_vod_lin_DI();

    if( (U1G_LIN_BIT_SET == U1G_LIN_SFR_BIT_LINEN)
        && (U1G_LIN_BIT_SET == U1G_LIN_SFR_BIT_LINMCKE) ) {
        u1a_lin_result = l_u1l_lin_linmodule_set_reset();       /* リセットモード遷移 */
    }
    else{
        /* クロックを供給、レジスタを再設定 */
        u1a_lin_result = l_u1g_lin_linmodule_init();            /* LINモジュールの初期化処理 */
        l_vog_lin_timer_init();                                 /* TAUモジュールの初期化処理 */
    }

    if( U1G_LIN_OK == u1a_lin_result ) {
        U1G_LIN_SFR_LIE  = U1G_LIN_LIE_SNDWUP;                  /* フレーム/ウェイクアップ送信完了割り込み 許可 */
        U1G_LIN_SFR_LEDE = U1G_LIN_LEDE_SNDWUP;                 /* エラー検出許可設定（リセットモード時に設定） */

        /* ウェイクアップモードへ遷移 */
        U1G_LIN_SFR_LCUC = U1G_LIN_LCUC_WUP;
        u2a_lin_lp_cnt   = U2G_LIN_0;
        
        /* ウェイクアップモード遷移待ち */
        while( (U2G_LIN_MODE_WAIT1_CNT > u2a_lin_lp_cnt)
            && ( U1G_LIN_LMST_WUP != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK) ) ) {
            u2a_lin_lp_cnt++;
        }
        if( U1G_LIN_LMST_WUP != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK) ) {
            /* モード遷移失敗 */
            u1a_lin_result = U1G_LIN_NG;
        }
        else{
            U1G_LIN_SFR_LST  = U1G_LIN_LST_CLR;                 /* LINステータス 明示クリア */
            U1G_LIN_SFR_LEST = U1G_LIN_LEST_CLR;                /* LINエラーレジスタ 明示クリア */

            /* FTS=1 が送信トリガとなるため、ここでは開始しない */

            aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );  /* SFRガード機能 無効化(GINT) */

            U1G_LIN_SFR_BIT_LINTRMPR0  = U1G_LIN_TRM_PR0;       /* 割り込み優先度設定(2bit,下位) */
            U1G_LIN_SFR_BIT_LINTRMPR1  = U1G_LIN_TRM_PR1;       /* 割り込み優先度設定(2bit,上位) */
            U1G_LIN_SFR_BIT_LINTRMIF   = U1G_LIN_BIT_CLR;       /* LINn送信割り込み要求フラグクリア */
            U1G_LIN_SFR_BIT_LINTRMMK   = U1G_LIN_BIT_CLR;       /* LINn送信割り込み許可 */
            U1G_LIN_SFR_BIT_LINRVCIF   = U1G_LIN_BIT_CLR;       /* LINn受信割り込み要求フラグクリア */
            U1G_LIN_SFR_BIT_LINRVCMK   = U1G_LIN_BIT_SET;       /* LINn受信割り込み禁止 */
            U1G_LIN_SFR_BIT_LINSTAIF   = U1G_LIN_BIT_CLR;       /* LINnステータスエラー割り込み要求フラグクリア */
            U1G_LIN_SFR_BIT_LINSTAMK   = U1G_LIN_BIT_SET;       /* LINnステータスエラー割り込み禁止(マスク) */

            aipf_cpu_Enable_SFRGrd();                           /* SFRガード機能 有効化 */
        }
    }

    /* 割り込み設定を復元 */
    l_vod_lin_EI();

    return( u1a_lin_result );
}


/**************************************************/
/*  LINモジュール通信可能状態設定処理             */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8  l_u1g_lin_set_run(void)
{
    l_u16   u2a_lin_lp_cnt;
    l_u8    u1a_lin_result;

    u1a_lin_result = U1G_LIN_OK;

    l_vod_lin_DI();                                                 /* 割り込み禁止設定 */

    /* 動作モードでない場合のみ */
    if( U1G_LIN_LMST_RUN != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK) ) {
        if( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) {
            /* リセットモードでなければ、リセットモードへ遷移 */
            U1G_LIN_SFR_LCUC    = U1G_LIN_LCUC_RESET;
            u2a_lin_lp_cnt      = U2G_LIN_0;

            /* リセットモード遷移待ち */
            while( (U2G_LIN_MODE_WAIT2_CNT > u2a_lin_lp_cnt)
                && ( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) ) {
                u2a_lin_lp_cnt++;
            }
            if( U1G_LIN_LMST_RESET != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_RESET_MASK) ) {
                /* モード遷移失敗 */
                u1a_lin_result = U1G_LIN_NG;
            }
        }

        if( U1G_LIN_OK == u1a_lin_result ) {
            U1G_LIN_SFR_LIE  = U1G_LIN_LIE_RUN;                 /* 割り込み許可（リセットモード時に設定） */
            U1G_LIN_SFR_LEDE = U1G_LIN_LEDE_RUN;                /* エラー検出許可設定（リセットモード時に設定） */

            /* 動作モードへ遷移 */
            u2a_lin_lp_cnt = U2G_LIN_0;
            U1G_LIN_SFR_LCUC = U1G_LIN_LCUC_RUN;
            
            /* 動作モード遷移待ち */
            while((U2G_LIN_MODE_WAIT1_CNT > u2a_lin_lp_cnt)
                && ( U1G_LIN_LMST_RUN != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK) ) ) {
                u2a_lin_lp_cnt++;
            }
            if( U1G_LIN_LMST_RUN != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK ) ) {
                /* モード遷移失敗 */
                u1a_lin_result = U1G_LIN_NG;
            }
            else{
                U1G_LIN_SFR_LST             = U1G_LIN_LST_CLR;      /* LINステータス 明示クリア */
                U1G_LIN_SFR_LEST            = U1G_LIN_LEST_CLR;     /* LINエラーレジスタ 明示クリア */

                aipf_cpu_Disable_SFRGrd( AIPF_CPU_SFRGRD_ID_INT );  /* SFRガード機能 無効化(GINT) */

                U1G_LIN_SFR_BIT_LINTRMPR0  = U1G_LIN_TRM_PR0;       /* 割り込み優先度設定(2bit,下位) */
                U1G_LIN_SFR_BIT_LINTRMPR1  = U1G_LIN_TRM_PR1;       /* 割り込み優先度設定(2bit,上位) */
                U1G_LIN_SFR_BIT_LINTRMIF   = U1G_LIN_BIT_CLR;       /* LINn送信割り込み要求フラグクリア */
                U1G_LIN_SFR_BIT_LINTRMMK   = U1G_LIN_BIT_CLR;       /* LINn送信割り込み許可 */
                U1G_LIN_SFR_BIT_LINRVCPR0  = U1G_LIN_RVC_PR0;       /* 割り込み優先度設定(2bit,下位) */
                U1G_LIN_SFR_BIT_LINRVCPR1  = U1G_LIN_RVC_PR1;       /* 割り込み優先度設定(2bit,上位) */
                U1G_LIN_SFR_BIT_LINRVCIF   = U1G_LIN_BIT_CLR;       /* LINn受信割り込み要求フラグクリア */
                U1G_LIN_SFR_BIT_LINRVCMK   = U1G_LIN_BIT_CLR;       /* LINn受信割り込み許可 */
                U1G_LIN_SFR_BIT_LINSTAPR0  = U1G_LIN_ERR_PR0;       /* 割り込み優先度設定(2bit,下位) */
                U1G_LIN_SFR_BIT_LINSTAPR1  = U1G_LIN_ERR_PR1;       /* 割り込み優先度設定(2bit,上位) */
                U1G_LIN_SFR_BIT_LINSTAIF   = U1G_LIN_BIT_CLR;       /* LINnステータスエラー割り込み要求フラグクリア */
                U1G_LIN_SFR_BIT_LINSTAMK   = U1G_LIN_BIT_CLR;       /* LINnステータスエラー割り込み許可 */

                aipf_cpu_Enable_SFRGrd();                           /* SFRガード機能 有効化 */

                U1G_LIN_SFR_LTRC   |= U1G_LIN_LTRC_FTS;             /* LIN通信開始 */
            }
        }
    }

    l_vod_lin_EI();                                                 /* 割り込み許可 */

    return( u1a_lin_result );
}


/**************************************************/
/*  LIN送信完了割り込み 処理（API）               */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void   l_ifc_tx(void)
{
    /* 送信完了 */
    U1G_LIN_SFR_LST = U1G_LIN_LST_CLR_TX;                   /* 送信完了情報クリア */
    l_vog_lin_tx_int();                                     /* 送信完了割り込み報告 */
}


/**************************************************/
/*  LIN受信完了割り込み 処理（API）               */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void   l_ifc_rx(void)
{
    l_u8    u1a_lin_lst_data;
    l_u8    u1a_lin_id;

    u1a_lin_lst_data    = U1G_LIN_SFR_LST;                  /* LINステータス取得 */
    U1G_LIN_SFR_LST     = U1G_LIN_LST_CLR_RX;               /* 受信完了情報クリア */
    u1a_lin_id          = (l_u8)(U1G_LIN_SFR_LIDB & U1G_LIN_LIDB_ID_MASK); /* IDを取得 */
    l_vog_lin_rx_int( u1a_lin_lst_data, u1a_lin_id );       /* 受信割り込み報告 */
}


/**************************************************/
/*  LINステータスエラー割り込み 処理（API）       */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void   l_ifc_err(void)
{
    l_u8    u1a_lin_lst_data;
    l_u8    u1a_lin_err;

    u1a_lin_lst_data    = U1G_LIN_SFR_LST;                  /* LINステータス取得 */
    u1a_lin_err         = U1G_LIN_SFR_LEST;                 /* LINエラー情報取得*/
    U1G_LIN_SFR_LEST    = U1G_LIN_LEST_CLR;                 /* 全エラーフラグクリア */

    l_vog_lin_err_int( u1a_lin_err, u1a_lin_lst_data );     /* エラー割り込み報告 */
}


/**************************************************/
/*  INT割り込み制御処理(API)                      */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_aux(void)
{
    l_vog_lin_aux_int();                                    /* INT割り込み報告 */
}


/**************************************************/
/*  タイマ満了時実行処理 (API)                    */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_ifc_tau_timeout(void)
{
    l_vog_lin_stop_timer();                                 /* タイマのカウンタを停止する */

    /* Framing エラー確定 */
    l_vog_lin_determine_FramingError();
}


/**********************************************************/
/*  レスポンス送信開始処理                                */
/*--------------------------------------------------------*/
/*  引数： u1a_lin_dl       : 送信するデータ長            */
/*         u1a_sum_type     : 0:classic 1:enhanced        */
/*         pta_lin_snd_data : 送信データ格納元ポインタ    */
/*  戻値： 処理結果                                       */
/*         (0 / 1) : 処理成功 / 処理失敗                  */
/**********************************************************/
l_u8  l_u1g_lin_resp_snd_start(l_u8 u1a_lin_dl, l_u8 u1a_sum_type, const l_u8 pta_lin_snd_data[])
{
    l_u8    u1a_lin_result;

    u1a_lin_result = U1G_LIN_NG;

    if( (U1G_LIN_DL_8 < u1a_lin_dl) || (U1G_LIN_NULL == pta_lin_snd_data) || (U1G_LIN_SUM_TYPE_MAX < u1a_sum_type) ) {
        /* 引数異常 */
    }
    else{
        /* LINモジュールモード確認 */
        if( U1G_LIN_LMST_RUN != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK) ) {
            /* 動作モードでなければ異常 */
        }
        else{
            /* LINフレーム/ウェイクアップ送信開始ビット確認 */
            if( U1G_LIN_BYTE_CLR == (U1G_LIN_SFR_LTRC & U1G_LIN_LTRC_FTS) ) {
                /* 送信開始ビット異常 */
            }
            else{
                /* LINレスポンスフィールド設定レジスタ設定  送信 */
                U1G_LIN_SFR_LDFC    = (l_u8)(u1a_lin_dl | U1G_LIN_LDFC_RCDS | (u1a_sum_type << U1G_LIN_BIT_SFT_5));  /* 送信、フレームサイズセット */

                /* 送信データ設定 */
                U1G_LIN_SFR_LDB1 = pta_lin_snd_data[ U1G_LIN_0 ];
                U1G_LIN_SFR_LDB2 = pta_lin_snd_data[ U1G_LIN_1 ];
                U1G_LIN_SFR_LDB3 = pta_lin_snd_data[ U1G_LIN_2 ];
                U1G_LIN_SFR_LDB4 = pta_lin_snd_data[ U1G_LIN_3 ];
                U1G_LIN_SFR_LDB5 = pta_lin_snd_data[ U1G_LIN_4 ];
                U1G_LIN_SFR_LDB6 = pta_lin_snd_data[ U1G_LIN_5 ];
                U1G_LIN_SFR_LDB7 = pta_lin_snd_data[ U1G_LIN_6 ];
                U1G_LIN_SFR_LDB8 = pta_lin_snd_data[ U1G_LIN_7 ];

                U1G_LIN_SFR_LTRC = U1G_LIN_LTRC_RTS;        /* レスポンス送信/受信開始（送信） */

                u1a_lin_result = U1G_LIN_OK;
            }
        }
    }

    return( u1a_lin_result );
}


/**********************************************************/
/*  レスポンス受信開始処理                                */
/*--------------------------------------------------------*/
/*  引数： u1a_lin_dl       : 受信するデータ長            */
/*         u1a_sum_type     : 0:classic 1:enhanced        */
/*  戻値： 処理結果                                       */
/*         (0 / 1) : 処理成功 / 処理失敗                  */
/**********************************************************/
l_u8  l_u1g_lin_resp_rcv_start(l_u8 u1a_lin_dl, l_u8 u1a_sum_type)
{
    l_u8    u1a_lin_result;

    u1a_lin_result = U1G_LIN_NG;

    if( (U1G_LIN_DL_8 < u1a_lin_dl) || (U1G_LIN_SUM_TYPE_MAX < u1a_sum_type) ) {
        /* 引数異常 */
    }
    else{
        /* LINモジュールモード確認 */
        if( U1G_LIN_LMST_RUN != (U1G_LIN_SFR_LMST &  U1G_LIN_LMST_MASK) ) {
            /* 動作モードでなければ異常 */
        }
        else{
            /* LINフレーム/ウェイクアップ送信開始ビット確認 */
            if( U1G_LIN_BYTE_CLR == (U1G_LIN_SFR_LTRC & U1G_LIN_LTRC_FTS) ) {
                /* 送信開始ビット異常 */
            }
            else{
                /* LINレスポンスフィールド設定レジスタ設定  受信 */
                U1G_LIN_SFR_LDFC    = (l_u8)(u1a_lin_dl | (u1a_sum_type << U1G_LIN_BIT_SFT_5));  /* 受信、フレームサイズセット */
                U1G_LIN_SFR_LTRC    = U1G_LIN_LTRC_RTS;     /* レスポンス送信/受信開始（受信） */
                u1a_lin_result      = U1G_LIN_OK;
            }
        }
    }

    return( u1a_lin_result );
}


/**********************************************************/
/*  レスポンス要求なし設定 処理                           */
/*--------------------------------------------------------*/
/*  引数： なし                                           */
/*  戻値： 処理結果                                       */
/*         (0 / 1) : 処理成功 / 処理失敗                  */
/**********************************************************/
l_u8  l_u1g_lin_resp_noreq(void)
{
    l_u8    u1a_lin_result;

    u1a_lin_result = U1G_LIN_NG;

    /* LINモジュールモード確認 */
    if( U1G_LIN_LMST_RUN != (U1G_LIN_SFR_LMST &  U1G_LIN_LMST_MASK) ) {
        /* 動作モードでなければ異常 */
    }
    else{
        /* LINフレーム/ウェイクアップ送信開始ビット確認 */
        if( U1G_LIN_BYTE_CLR == (U1G_LIN_SFR_LTRC & U1G_LIN_LTRC_FTS) ) {
            /* 送信開始ビット異常 */
        }
        else{
            /* フレーム通信開始 */
            U1G_LIN_SFR_LTRC = U1G_LIN_LTRC_LNRR;       /* レスポンスなし要求 */

            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return( u1a_lin_result );
}



/**********************************************************/
/*  受信データ取得処理                                    */
/*--------------------------------------------------------*/
/*  引数： pta_lin_rcv_data : 受信データ格納先ポインタ    */
/*  戻値： なし                                           */
/**********************************************************/
void  l_vog_lin_get_rcvdata(l_u8 pta_lin_rcv_data[])
{
    if( U1G_LIN_NULL == pta_lin_rcv_data ) {
        /* 引数異常  ※戻り値での通知は行いません */
    }
    else{
        /* 受信データ格納 */
        pta_lin_rcv_data[ U1G_LIN_0 ] = U1G_LIN_SFR_LDB1;
        pta_lin_rcv_data[ U1G_LIN_1 ] = U1G_LIN_SFR_LDB2;
        pta_lin_rcv_data[ U1G_LIN_2 ] = U1G_LIN_SFR_LDB3;
        pta_lin_rcv_data[ U1G_LIN_3 ] = U1G_LIN_SFR_LDB4;
        pta_lin_rcv_data[ U1G_LIN_4 ] = U1G_LIN_SFR_LDB5;
        pta_lin_rcv_data[ U1G_LIN_5 ] = U1G_LIN_SFR_LDB6;
        pta_lin_rcv_data[ U1G_LIN_6 ] = U1G_LIN_SFR_LDB7;
        pta_lin_rcv_data[ U1G_LIN_7 ] = U1G_LIN_SFR_LDB8;
    }
}


/**************************************************/
/*  Wakeupパルス送信処理                          */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_u8  l_u1g_lin_snd_wakeup(void)
{
    l_u8    u1a_lin_result;

    u1a_lin_result = U1G_LIN_NG;

    /* LINモジュールモード確認 */
    if( U1G_LIN_LMST_WUP != (U1G_LIN_SFR_LMST & U1G_LIN_LMST_MASK) ) {
            /* ウェイクアップモードでなければ異常 */
    }
    else{
        /* LINフレーム/ウェイクアップ送信開始ビット確認 */
        if( U1G_LIN_BYTE_CLR != (U1G_LIN_SFR_LTRC & U1G_LIN_LTRC_FTS) ) {
            /* 送信開始ビット異常 */
        }
        else{
            U1G_LIN_SFR_LDFC    |= U1G_LIN_LDFC_RCDS;   /* 通信方向設定(送信) */
            U1G_LIN_SFR_LTRC    |= U1G_LIN_LTRC_FTS;    /* ウェイクアップ送信開始 */

            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return( u1a_lin_result );
}


/**************************************************/
/*  ボーレート妥当性確認処理                      */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 妥当性                                 */
/*         (0 / 1) : 妥当 / 不当                  */
/**************************************************/
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
l_u8 l_u1g_lin_is_valid_baud_rate(void)
{
    l_u16 u2a_lin_lbrp;
    l_u8 u1a_lin_result;

    u2a_lin_lbrp = U2G_LIN_SFR_LBRP;
    u1a_lin_result = U1G_LIN_OK;

    if( ( U2G_LIN_MINIMUM_LBRP > u2a_lin_lbrp ) ||
        ( U2G_LIN_MAXIMUM_LBRP < u2a_lin_lbrp ) ) {
        u1a_lin_result = U1G_LIN_NG;
    }

    return u1a_lin_result;
}
#endif /* U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON */


/*************************************************************************/
/*  Response too short / No response 確定処理                            */
/*-----------------------------------------------------------------------*/
/*  引数： u1a_lst_d1rc_state : 0:データ1受信未完了 1:データ1受信完了    */
/*  戻値： なし                                                          */
/*************************************************************************/
void l_vog_lin_determine_ResTooShort_or_NoRes(l_u8 u1a_lst_d1rc_state)
{
    if( U1G_LIN_BYTE_CLR != (U1G_LIN_TAU_TE & U1G_LIN_TE_ENABLE) ) {
        /* 汎用タイマ動作許可状態 */
        l_vog_lin_stop_timer();                             /* タイマのカウンタ停止 */
        
        if( u1a_lst_d1rc_state == U1G_LIN_LST_D1RC_COMPLETE ) {
            /* データ1受信フラグが 1 の場合、Response Too Shortを確定 */
            l_vog_lin_ResTooShort_error();                  /* Response too short エラー確定処理 */
        }
        else if( u1a_lst_d1rc_state == U1G_LIN_LST_D1RC_INCOMPLETE ) {
            /* データ1受信フラグが 0 の場合、No Response を確定 */
            l_vog_lin_NoRes_error();                        /* No response エラー確定処理 */
        }
        else{
            /* 引数異常 */
        }
        
        U1G_LIN_SFR_LST &= (~U1G_LIN_LST_D1RC_MASK);        /* LSTレジスタデータ1受信完了値 明示クリア */
    }
    else{
        /* 汎用タイマ動作禁止の場合、処理なし */
    }

}

/***** End of File *****/


