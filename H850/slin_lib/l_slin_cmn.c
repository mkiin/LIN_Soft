/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
/* File Name   : l_slin_cmn.c                               */
/* Version     : 2.00                                       */
/* Contents    :                                            */
/* Customer    : SUNNY GIKEN INC.                           */
/* Model       :                                            */
/* Order       :                                            */
/* CPU         :                                            */
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

/***** ヘッダ インクルード *****/
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"

/*** 関数のプロトタイプ宣言(API) ***/
l_bool l_sys_init(void);

/*===========================================================================================*/

/**************************************************/
/*  システム初期化 処理(API)                      */
/*------------------------------------------------*/
/*  引数： なし                                   */
/*  戻値： 処理結果                               */
/*         (0 / 1) : 処理成功 / 処理失敗          */
/**************************************************/
l_bool  l_sys_init(void)
{
    l_u8   u1a_lin_parachk;
    l_u8   u1a_lin_tblchk;
    l_bool u2a_lin_result;

    /* パラメータチェック */
    u1a_lin_parachk = l_u1g_lin_para_chk();

    if( u1a_lin_parachk != U1G_LIN_OK ){
        u2a_lin_result = U2G_LIN_NG;
    }
    else{
        /* テーブルパラメータチェック */
        u1a_lin_tblchk = l_u1g_lin_tbl_chk();

        if( u1a_lin_tblchk != U1G_LIN_OK ){
            u2a_lin_result = U2G_LIN_NG;
        }
        else{
            u2a_lin_result = U2G_LIN_OK;
        }
    }

    return( u2a_lin_result );
}

/***** End of File *****/




/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
