/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_cmn.c                                */
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
    l_bool u2a_lin_result;
    
    /* コンソーシアム仕様にて定義されている */
    /* 最初に実行するAPI。                  */
    /* 本製品では行うべき処理が無いため、   */
    /* 戻り値でOKを返すのみとします         */

    u2a_lin_result = U2G_LIN_OK;

    return( u2a_lin_result );
}


/***** End of File *****/



