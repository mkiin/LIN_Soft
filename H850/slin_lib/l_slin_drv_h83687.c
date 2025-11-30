/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
/* File Name   : l_slin_drv_h83687.c                        */
/* Version     : 2.00                                       */
/* Contents    :                                            */
/* Customer    : SUNNY GIKEN INC.                           */
/* Model       :                                            */
/* Order       :                                            */
/* CPU         : H8/3687                                    */
/* Compiler    :                                            */
/* OS          : Not used                                   */
/* LIN         : トヨタ自動車殿標準LIN仕様                  */
/* Programmer  :                                            */
/* Note        :                                            */
/************************************************************/
/* Copyright 2004 - 2005 SUNNY GIKEN INC.                   */
/************************************************************/
/* History : 2004.08.03 Ver 1.00                            */
/*         : 2004.11.04 Ver 1.01                            */
/*         : 2005.02.10 Ver 2.00                            */
/*""FILE COMMENT END""***************************************/

#pragma	section	lin

/*=== MCU依存部分 ===*/
/***** ヘッダ インクルード *****/
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_slin_sfr_h83687.h"
#include "l_slin_drv_h83687.h"

/***** 関数プロトタイプ宣言 *****/
/*-- API関数(extern) --*/
void  l_ifc_rx_ch1(void);
void  l_ifc_tm_ch1(void);
void  l_ifc_aux_ch1(void);

/*-- その他 MCU依存関数(extern) --*/
void   l_vog_lin_uart_init(void);
void   l_vog_lin_timer_init(void);
void   l_vog_lin_int_init(void);
void   l_vog_lin_rx_enb(void);
void   l_vog_lin_rx_dis(void);
void   l_vog_lin_int_enb(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
void  l_vog_lin_int_enb_wakeup(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
void   l_vog_lin_int_dis(void);
void   l_vog_lin_tx_char(l_u8 u1a_lin_data);
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data);
void   l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit);
void   l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit);
void   l_vog_lin_bus_tm_set(void);
void   l_vog_lin_frm_tm_stop(void);
l_u8   l_u1g_lin_para_chk(void);

/*** 変数(static) ***/
static l_u16  u2l_lin_tm_bit;           /* 1bitタイム値 */
static l_u16  u2l_lin_tm_maxbit;        /* 0xFFFFカウント分のビット長 */
static l_u16  u2l_lin_tm_cnt;           /* タイマカウンタ */

/**************************************************/

/********************************/
/* MCU依存のAPI関数処理         */
/********************************/
/**************************************************/
/*  UARTレジスタの初期化 処理                     */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_uart_init(void)
{
    l_u8    u1a_lin_flg;
    l_u8    u1a_lin_errflg;
    l_u8    u1a_lin_rdata;

    /*** SCIの初期化 ***/
    u1a_lin_flg = l_u1g_lin_irq_dis();                      /* 割り込み禁止設定 */

    U1G_LIN_REG_SCR3.u1l_lin_byte = U1G_LIN_SCR3_INIT;      /* シリアルコントロールレジスタ3の設定 */
                                                            /* シリアルモードレジスタの設定 */
  #if U1G_LIN_BRGSRC == U1G_LIN_BRGSRC_F1
    U1G_LIN_REG_SMR.u1l_lin_byte = U1G_LIN_SMR_INIT | ( U1G_LIN_SMR_MASK & U1G_LIN_0 );
  #elif U1G_LIN_BRGSRC == U1G_LIN_BRGSRC_F4
    U1G_LIN_REG_SMR.u1l_lin_byte = U1G_LIN_SMR_INIT | ( U1G_LIN_SMR_MASK & U1G_LIN_1 );
  #endif

    U1G_LIN_REG_BRR.u1l_lin_byte = U1G_LIN_BRG - U1G_LIN_1; /* ビットレートの設定 */
    U1G_LIN_FLG_PMTXD = U1G_LIN_BIT_SET;                    /* ポートモード:TXD端子出力機能の選択 */
    if( U1G_LIN_FLG_SRF == U1G_LIN_BIT_SET ) {              /* 受信データ空読み */
        u1a_lin_rdata = U1G_LIN_REG_RDR.u1l_lin_byte;
    }
    u1a_lin_errflg = U1G_LIN_FLG_SOER;                      /* オーバーランエラーのクリア */
    U1G_LIN_FLG_SOER = U1G_LIN_BIT_CLR;
    u1a_lin_errflg = U1G_LIN_FLG_SFER;                      /* フレーミングエラーのクリア */
    U1G_LIN_FLG_SFER = U1G_LIN_BIT_CLR;
    u1a_lin_errflg = U1G_LIN_FLG_SPER;                      /* パリティーエラーのクリア */
    U1G_LIN_FLG_SPER = U1G_LIN_BIT_CLR;
    U1G_LIN_FLG_SREN = U1G_LIN_BIT_SET;                     /* 受信イネーブル */
    U1G_LIN_FLG_STEN = U1G_LIN_BIT_SET;                     /* 送信イネーブル */

    l_vog_lin_irq_res( u1a_lin_flg );                       /* 割り込み設定を復元 */

}


/**************************************************/
/*  Timerレジスタの初期化 処理                    */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_timer_init(void)
{
    l_u8    u1a_lin_flg;
    l_u8    u1a_tmp_rdata;

    /*** 1Bitタイマ値のセット ***/
  #if U1G_LIN_BRGSRC == U1G_LIN_BRGSRC_F1
    u2l_lin_tm_bit = ( (l_u16)U1G_LIN_BRG * U2G_LIN_FRM_TM_SRC_1 ) * U2G_LIN_4;
  #elif U1G_LIN_BRGSRC == U1G_LIN_BRGSRC_F4
    u2l_lin_tm_bit = ( (l_u16)U1G_LIN_BRG * U2G_LIN_FRM_TM_SRC_4 ) * U2G_LIN_4;
  #endif

    /* 0xFFFFHカウント分のビット長のセット */
    /* ０除算のチェック */
    if(u2l_lin_tm_bit != U2G_LIN_BYTE_CLR){
        u2l_lin_tm_maxbit = U2G_LIN_WORD_LIMIT / u2l_lin_tm_bit;
    }else{
        /* 通常ありえない処理 */
        u2l_lin_tm_maxbit = U2G_LIN_1;
        u1g_lin_syserr = U1G_LIN_SYSERR_DIV;                /* 0除算発生 */
    }

    /*** Timerの初期化 ***/
    u1a_lin_flg = l_u1g_lin_irq_dis();                      /* 割り込み禁止設定 */

    U1G_LIN_FLG_TMST = U2G_LIN_BIT_CLR;                     /* タイマ停止 */
    U1G_LIN_REG_TCR.u1l_lin_byte = U1G_LIN_FRM_TCR_INIT;    /* タイマコントロールレジスタの設定 */
                                                            /* GRAのコンペアマッチ */
                                                            /* 内部クロックのφ/8でカウント */
    U1G_LIN_REG_TIORA.u1l_lin_byte = U1G_LIN_FRM_TIORA_INIT;/* タイマI/Oコントロールレジスタの設定 */

    u1a_tmp_rdata = U1G_LIN_REG_TSR.u1l_lin_byte;           /* クリアのためのダミー読み込み */
    U1G_LIN_REG_TSR.u1l_lin_byte = U2G_LIN_BYTE_CLR;        /* タイマステータスレジスタのクリア */
    U1G_LIN_REG_TIER.u1l_lin_byte = U1G_LIN_FRM_TIER_INIT;  /* タイマインタラプトイネーブルレジスタの設定 */

    l_vog_lin_irq_res( u1a_lin_flg );                       /* 割り込み設定を復元 */

}


/**************************************************/
/*  外部INTレジスタの初期化 処理                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_init(void)
{
    l_u8    u1a_lin_flg;

    /*** INTの初期化 ***/
    u1a_lin_flg = l_u1g_lin_irq_dis();              /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_CLR;            /* 割り込み要求ディセーブル */
    U1G_LIN_FLG_INTEG = U1G_LIN_BIT_SET;            /* 立上りエッジ検出を選択 */
    U1G_LIN_FLG_PMIRQ = U1G_LIN_BIT_SET;            /* ポートモード:IRQ入力端子の選択 */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();
    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;            /* 割り込み要求フラグクリア */

    l_vog_lin_irq_res( u1a_lin_flg );               /* 割り込み設定を復元 */

}


/**************************************************/
/*  データの一文字受信処理(API)                   */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
/*  UART受信割り込みの際に呼び出されるAPI         */
/**************************************************/
void  l_ifc_rx_ch1(void)
{
    l_u8    u1a_lin_rx_status;
    l_u8    u1a_lin_rx_data;
    l_u8    u1a_lin_rx_err;

    /* 受信データ取得 */
    u1a_lin_rx_data = U1G_LIN_REG_RDR.u1l_lin_byte;

    /* 受信ステータス取得 */
    u1a_lin_rx_status = U1G_LIN_REG_SSR.u1l_lin_byte;

    /* 受信エラー情報の生成 */
    u1a_lin_rx_err = U1G_LIN_BYTE_CLR;
    if( (u1a_lin_rx_status & U1G_LIN_SOER_MASK) != 0 ) {
        u1a_lin_rx_err |= U1G_LIN_SOER_SET;                 /* オーバランエラー発生 */
    }
    if( (u1a_lin_rx_status & U1G_LIN_SFER_MASK) != 0 ) {
        u1a_lin_rx_err |= U1G_LIN_SFER_SET;                 /* フレーミングエラー発生 */
    }
    if( (u1a_lin_rx_status & U1G_LIN_SPER_MASK) != 0 ) {
        u1a_lin_rx_err |= U1G_LIN_SPER_SET;                 /* パリティエラー発生 */
    }

    l_vog_lin_rx_int( u1a_lin_rx_data, u1a_lin_rx_err );     /* スレーブタスクに受信報告 */

}


/**************************************************/
/*  フレームタイマ制御処理(API)                   */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
/*  タイマ割り込みの際に呼び出されるAPI           */
/**************************************************/
void  l_ifc_tm_ch1(void)
{
    l_u8    u1a_lin_rdata;

    /* タイマステータスレジスタのクリア */
    u1a_lin_rdata = U1G_LIN_REG_TSR.u1l_lin_byte;   /* ダミーリード */
    U1G_LIN_REG_TSR.u1l_lin_byte = U1G_LIN_BYTE_CLR;

    /* タイマカウントが"0"以上の場合 */
    if( u2l_lin_tm_cnt > U2G_LIN_WORD_CLR ){
        u2l_lin_tm_cnt--;                                           /* デクリメントして抜ける */
    }
    /* タイマカウントが"0"の場合 */
    else{
        U1G_LIN_FLG_TMST = U1G_LIN_BIT_CLR;                       /* タイマ停止 */

        l_vog_lin_tm_int();                                         /* タイマ割り込み報告 */
    }
}


/**************************************************/
/*  外部INT割り込み制御処理(API)                  */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
/*  外部INT割り込みの際に呼び出されるAPI          */
/**************************************************/
void  l_ifc_aux_ch1(void)
{
    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;        /* IRQ割り込み要求フラグクリア */
    l_vog_lin_irq_int();                        /* 外部INT割り込み報告 */
}


/***********************************/
/* MCU固有のSFR設定用関数処理      */
/***********************************/
/**************************************************/
/*  UART受信割り込み許可 処理                     */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_rx_enb(void)
{
    l_u8    u1a_lin_flg;
    l_u8    u1a_lin_errflg;
    l_u8    u1a_lin_rdata;

    u1a_lin_flg = l_u1g_lin_irq_dis();                  /* 割り込み禁止設定 */

    U1G_LIN_FLG_SREN = U1G_LIN_BIT_CLR;                 /* 受信ディセーブル */

    if( U1G_LIN_FLG_SRF == U1G_LIN_BIT_SET ) {          /* 受信データ空読み */
        u1a_lin_rdata = U1G_LIN_REG_RDR.u1l_lin_byte;
    }

    u1a_lin_errflg = U1G_LIN_FLG_SOER;                  /* オーバーランエラーのクリア */
    U1G_LIN_FLG_SOER = U1G_LIN_BIT_CLR;
    u1a_lin_errflg = U1G_LIN_FLG_SFER;                  /* フレーミングエラーのクリア */
    U1G_LIN_FLG_SFER = U1G_LIN_BIT_CLR;
    u1a_lin_errflg = U1G_LIN_FLG_SPER;                  /* パリティーエラーのクリア */
    U1G_LIN_FLG_SPER = U1G_LIN_BIT_CLR;

    U1G_LIN_FLG_SREN = U1G_LIN_BIT_SET;                 /* 受信イネーブル */
    U1G_LIN_FLG_SRIC = U1G_LIN_BIT_SET;                 /* 割り込み要求イネーブル */

    l_vog_lin_irq_res( u1a_lin_flg );                   /* 割り込み設定復元 */
}


/**************************************************/
/*  UART受信割り込み禁止 処理                     */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_rx_dis(void)
{
    l_u8    u1a_lin_flg;

    u1a_lin_flg = l_u1g_lin_irq_dis();                  /* 割り込み禁止設定 */

    U1G_LIN_FLG_SRIC = U1G_LIN_BIT_CLR;                 /* 割り込み要求ディセーブル */

    l_vog_lin_irq_res( u1a_lin_flg );                   /* 割り込み設定復元 */
}


/**************************************************/
/*  INT割り込み許可 処理                          */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_enb(void)
{
    l_u8    u1a_lin_flg;

    u1a_lin_flg = l_u1g_lin_irq_dis();                  /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_SET;                /* 割り込み要求イネーブル */
    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;                /* 割り込み要求フラグクリア */

    l_vog_lin_irq_res( u1a_lin_flg );                   /* 割り込み設定復元 */
}


/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
/**************************************************/
/*  INT割り込み許可(Wakeupパルス検出用) 処理      */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_enb_wakeup(void)
{
    l_u8    u1a_lin_flg;

    u1a_lin_flg = l_u1g_lin_irq_dis();                  /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTEG = U1G_LIN_INTEG_WPINI;            /* エッジ検出方向を設定 */
    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_SET;                /* 割り込み要求イネーブル */
    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;                /* 割り込み要求フラグクリア */

    l_vog_lin_irq_res( u1a_lin_flg );                   /* 割り込み設定復元 */
}
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */


/**************************************************/
/*  INT割り込み禁止 処理                          */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_int_dis(void)
{
    l_u8    u1a_lin_flg;

    u1a_lin_flg = l_u1g_lin_irq_dis();                  /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_CLR;                /* 割り込み要求ディセーブル */

    l_vog_lin_irq_res( u1a_lin_flg );                   /* 割り込み設定復元 */
}


/**************************************************/
/*  送信レジスタにデータをセットする 処理         */
/*------------------------------------------------*/
/*  引数： 送信データ                             */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    /* 送信バッファレジスタに (1byte)データを入れる */
    U1G_LIN_REG_TDR.u1l_lin_byte = u1a_lin_data;
}


/**************************************************/
/*  リードバック 処理                             */
/*------------------------------------------------*/
/*  引数： リードバック比較用データ               */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 読込み成功 / 読込み失敗      */
/**************************************************/
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    l_u8    u1a_lin_readback_data;
    l_u8    u1a_lin_result;

    /* 受信データの有無をチェック */
    if( U1G_LIN_FLG_SRF == U1G_LIN_BIT_SET ) {
        /* エラーが発生しているかをチェック */
        if(    (U1G_LIN_FLG_SOER == U1G_LIN_BIT_SET)
            || (U1G_LIN_FLG_SFER == U1G_LIN_BIT_SET)
            || (U1G_LIN_FLG_SPER == U1G_LIN_BIT_SET) ) {
            u1a_lin_result = U1G_LIN_NG;
        }
        else {
            u1a_lin_readback_data = U1G_LIN_REG_RDR.u1l_lin_byte; /* 受信データ取得 */
            /* 受信バッファの内容と引数を比較 */
            if( u1a_lin_readback_data != u1a_lin_data ){
                u1a_lin_result = U1G_LIN_NG;
            }
            else{
                u1a_lin_result = U1G_LIN_OK;
            }
        }
    }
    else {
        u1a_lin_result = U1G_LIN_NG;
    }

    return( u1a_lin_result );
}


/**************************************************/
/*  Bitタイマの設定 処理                          */
/*------------------------------------------------*/
/*  引数： bit:タイマ設定値(bit長)                */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    l_u16   u2a_lin_tmp_bit;

    l_vog_lin_frm_tm_stop();                                /* タイマ停止 */

    u2a_lin_tmp_bit = (l_u16)u1a_lin_bit;

    /* タイマ値がFFFFhを超える場合の計算 */
    if( u2a_lin_tmp_bit > u2l_lin_tm_maxbit ){
        u2l_lin_tm_cnt = u2a_lin_tmp_bit / u2l_lin_tm_maxbit;
        u2a_lin_tmp_bit /= (u2l_lin_tm_cnt + U2G_LIN_1);
    }
    else{
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    } 

    /* タイマカウンタ、GRAの設定 */
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマカウント動作開始 */
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_SET;
}


/**************************************************/
/*  応答タイムアウトタイマセット 処理             */
/*------------------------------------------------*/
/*  引数： bit: レスポンス長(データ+チェックサム) */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    l_u16   u2a_lin_tmp_bit;
    l_u16   u2a_lin_rest_time;

    /* タイマカウントを停止します */
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_CLR;

    /* (Data Length + Checksum) × 1.4 */
    u2a_lin_tmp_bit = ( ((l_u16)u1a_lin_bit * U2G_LIN_14) / U2G_LIN_10 );

    /* 割り込み要求ビットチェック */
    if( U1G_LIN_FLG_TMIR != U1G_LIN_BIT_CLR ){
        /* 残時間なし */
        u2a_lin_rest_time = U2G_LIN_WORD_CLR;
    }else{
        /* タイマ値の算出 */
        u2a_lin_rest_time = U2G_LIN_REG_GRA.u2l_lin_word - U2G_LIN_REG_TCNT.u2l_lin_word;
    }

    /* 割り込み要求ビットクリア */
    U1G_LIN_FLG_TMIR = U1G_LIN_BIT_CLR;

    /* タイマ値をビット長に置き換える */
    /* ０除算のチェック */
    if(u2l_lin_tm_bit != U2G_LIN_BYTE_CLR){
        u2a_lin_tmp_bit = (u2a_lin_rest_time / u2l_lin_tm_bit) + u2a_lin_tmp_bit;
    }
    else{
        /* 通常ありえない処理 */
        u2a_lin_tmp_bit = U2G_LIN_BYTE_CLR;
        u1g_lin_syserr = U1G_LIN_SYSERR_DIV;                /* 0除算発生 */
    } 

    /* タイマ値がFFFFhを超える場合の計算 */
    if( u2a_lin_tmp_bit > u2l_lin_tm_maxbit ){
        u2l_lin_tm_cnt = u2a_lin_tmp_bit / u2l_lin_tm_maxbit;
        u2a_lin_tmp_bit /= (u2l_lin_tm_cnt + U2G_LIN_1);
    }
    else{
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    } 

    /* タイマカウンタ、GRAの設定 */
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマカウント動作開始 */
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_SET;


}


/**************************************************/
/*  Physical Busエラータイマの設定 処理           */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_bus_tm_set(void)
{
    l_u16   u2a_lin_tmp_bit;

    l_vog_lin_frm_tm_stop();                                /* タイマ停止 */

    u2a_lin_tmp_bit = U2G_LIN_BUS_TIMEOUT;

    /* タイマ値がFFFFhを超える場合の計算 */
    if( u2a_lin_tmp_bit > u2l_lin_tm_maxbit ){
        u2l_lin_tm_cnt = u2a_lin_tmp_bit / u2l_lin_tm_maxbit;
        u2a_lin_tmp_bit /= (u2l_lin_tm_cnt + U2G_LIN_1);
    }
    else{
        u2l_lin_tm_cnt = U2G_LIN_BYTE_CLR;
    } 

    /* タイマカウンタ、GRAの設定 */
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = u2a_lin_tmp_bit * u2l_lin_tm_bit;

    /* タイマカウント動作開始 */
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_SET;
}


/**************************************************/
/*  タイマ停止 処理                               */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： なし                                   */
/**************************************************/
void  l_vog_lin_frm_tm_stop(void)
{
    /* タイマカウントを停止します */
    U1G_LIN_FLG_TMST = U1G_LIN_BIT_CLR;

    /* 割り込み要求ビットをOFFします */
    U1G_LIN_FLG_TMIR = U1G_LIN_BIT_CLR;
}


/**************************************************/
/*  パラメータチェック 処理                       */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
/* 本関数内の条件判定文(５箇所)はすべて"C0004 (I) Constant as condition"のコンパイルワーニングとなる */
/* コンパイル時にユーザーが指定する定数値を式で評価しているため上記のワーニングが出る。問題なしとする。 */
l_u8  l_u1g_lin_para_chk(void)
{
    l_u8 u1a_lin_result;

    u1a_lin_result = U1G_LIN_OK;

    /* ノードアドレスのチェック(0x01 - 0xFF) */
    if( (U2G_LIN_NAD < U2G_LIN_NAD_MIN) || ( U2G_LIN_NAD_MAX < U2G_LIN_NAD) ){
        u1a_lin_result = U1G_LIN_NG;
    }
    /* 最大LINバッファスロット数(1 - 64) */
    if( (U1G_LIN_MAX_SLOT < U1G_LIN_MAX_SLOT_MIN) || (U1G_LIN_MAX_SLOT_MAX < U1G_LIN_MAX_SLOT) ){
        u1a_lin_result = U1G_LIN_NG;
    }
    /* BRGレジスタ値のチェック(0x01 - 0xFF) */
    if( (U1G_LIN_BRG < U1G_LIN_BRG_MIN) || (U1G_LIN_BRG_MAX < U1G_LIN_BRG) ){
        u1a_lin_result = U1G_LIN_NG;
    }
    /* In-Frame Responce Spaceのチェック(0 - 10) */
    if( (U1G_LIN_RSSP < U1G_LIN_RSSP_MIN) || (U1G_LIN_RSSP_MAX < U1G_LIN_RSSP) ){
        u1a_lin_result = U1G_LIN_NG;
    }
    /* Inter-Byte Spaceのチェック(1 - 4) */
    if( (U1G_LIN_BTSP < U1G_LIN_BTSP_MIN) || (U1G_LIN_BTSP_MAX < U1G_LIN_BTSP) ){
        u1a_lin_result = U1G_LIN_NG;
    }
    return( u1a_lin_result );
}

/***** End of File *****/



