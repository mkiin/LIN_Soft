/*""FILE COMMENT""*************************************************************
 * System Name  : S/R system
 * File Name    : p_get_msg_lin.h
 * Contests     : LIN通信データ取得
 * Model        : SRF汎用
 * Order        : Project No 
 * CPU          : Renesas H8/300H Tiny Series
 * Compiler     : Renesas H8S,H8/300 Series C/C++ Compiler Ver 5.0.1
 * OS           : no used
 * Note         :
 ******************************************************************************
 *                                      Copyright(c) AISIN SEIKI CO.,LTD.
 ******************************************************************************
 * History      : 2004.09.20
 *""FILE COMMENT END""*********************************************************/

#ifndef P_GET_MSG_LIN_H
#define P_GET_MSG_LIN_H

/******************************************************************************
 * Header File Include
 ******************************************************************************/
#include "l_stddef.h"
#include "p_lin_apl.h"

/******************************************************************************
 * Definition section
 ******************************************************************************/


/******************************************************************************
 * declaretion section
 ******************************************************************************/


/****************************************************************************/
/* Prototype declaration section										 	*/
/****************************************************************************/
void p_vog_lin_info_get_msg( xn_lin_info_sig_t * );
void p_vog_lin_rmt_get_msg( xn_lin_info_rmt_t * );
void p_vog_lin_sts_get_msg( xn_lin_rx_com_sts_t * );
void p_vog_lin_fst_get_msg( xn_lin_rx_com_fst_t * );
void p_vog_lin_cmt_get_msg( xn_lin_info_cmt_t * );
void p_vog_lin_mst_get_msg( xn_lin_info_mst_t * );
void p_vog_lin_spd_get_msg( xn_lin_info_spd_t * );

#endif /* P_GET_MSG_LIN_H */

/*--- end of file ---*/




