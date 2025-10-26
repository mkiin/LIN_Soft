/**
 * @file        l_slin_cmn.c
 *
 * @brief       SLIN CMN層
 *
 * @attention   編集禁止
 *
 */

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
    l_u8   u1a_lin_tblchk;
    l_bool u2a_lin_result;

    /* テーブルパラメータチェック */
    u1a_lin_tblchk = l_u1g_lin_tbl_chk();

    if( u1a_lin_tblchk != U1G_LIN_OK ){
        u2a_lin_result = U2G_LIN_NG;
    }
    else{
        u2a_lin_result = U2G_LIN_OK;
    }

    return( u2a_lin_result );
}

/***** End of File *****/
