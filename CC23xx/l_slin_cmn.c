/**
 * @file        l_slin_cmn.c
 *
 * @brief       SLIN 共通処理層
 *
 * @details     LINシステムの初期化処理とパラメータ検証を行う
 *
 * @note        CC2340R53向けに新規作成（H850互換）
 */

/********************************************************************************/
/* ヘッダインクルード                                                           */
/********************************************************************************/
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_slin_drv_cc2340r53.h"

/********************************************************************************/
/* 関数のプロトタイプ宣言(内部)                                                */
/********************************************************************************/
static l_u8 l_u1g_lin_para_chk(void);

/********************************************************************************/
/* 公開関数実装                                                                 */
/********************************************************************************/

/**
 * @brief システム初期化処理(API)
 *
 * @details LINシステムの初期化を行う。パラメータチェックとテーブルチェックを実施。
 *
 * @return l_bool 処理結果
 * @retval U2G_LIN_OK   処理成功
 * @retval U2G_LIN_NG   処理失敗
 *
 * @note
 *   - アプリケーション起動時に1回だけ呼び出すこと
 *   - l_ifc_init_ch1()より前に呼び出すこと
 */
l_bool l_sys_init(void)
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

/********************************************************************************/
/* 内部関数実装                                                                 */
/********************************************************************************/

/**
 * @brief パラメータチェック
 *
 * @details l_slin_def.hで定義されたパラメータの妥当性を検証
 *
 * @return l_u8 チェック結果
 * @retval U1G_LIN_OK   チェックOK
 * @retval U1G_LIN_NG   チェックNG
 *
 * @note 以下のパラメータをチェック：
 *   - ボーレート設定（2400/9600/19200 bps）
 *   - Response Space（0-10 bit）
 *   - Inter-byte Space（1-4 bit）※LIN 2.2a仕様準拠
 */
static l_u8 l_u1g_lin_para_chk(void)
{
    l_u8 u1a_lin_result = U1G_LIN_OK;

    /* ボーレート設定チェック */
    if( (U1G_LIN_BAUDRATE != U1G_LIN_BAUDRATE_2400) &&
        (U1G_LIN_BAUDRATE != U1G_LIN_BAUDRATE_9600) &&
        (U1G_LIN_BAUDRATE != U1G_LIN_BAUDRATE_19200) ){
        u1a_lin_result = U1G_LIN_NG;
    }

    /* Response Space チェック（0 - 10 bit）*/
    if( (U1G_LIN_RSSP < U1G_LIN_RSSP_MIN) || (U1G_LIN_RSSP_MAX < U1G_LIN_RSSP) ){
        u1a_lin_result = U1G_LIN_NG;
    }

    /* Inter-byte Space チェック（1 - 4 bit）※LIN 2.2a仕様 */
    if( (U1G_LIN_BTSP < U1G_LIN_BTSP_MIN) || (U1G_LIN_BTSP_MAX < U1G_LIN_BTSP) ){
        u1a_lin_result = U1G_LIN_NG;
    }

    return( u1a_lin_result );
}

/***** End of File *****/
