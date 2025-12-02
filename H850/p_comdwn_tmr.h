/*""FILE COMMENT""*************************************************************
 * System Name  : S/R system
 * File Name    : p_comdwn_tmr.h
 * Contests     : LIN通信途絶検出タイマ
 * Model        : SRF汎用
 * Order        : Project No 
 * CPU          : Renesas H8/300H Tiny Series
 * Compiler     : Renesas H8S,H8/300 Series C/C++ Compiler Ver 5.0.1
 * OS           : no used
 * Note         :
 ******************************************************************************
 *                                      Copyright(c) AISIN SEIKI CO.,LTD.
 ******************************************************************************
 * History      : 2004.07.15
 *""FILE COMMENT END""*********************************************************/

#ifndef P_COMDWN_TIM_H
#define P_COMDWN_TIM_H

/******************************************************************************
 * Header File Include
 ******************************************************************************/
#include "l_stddef.h"

/******************************************************************************
 * Definition section
 ******************************************************************************/
#define	U2L_TIM_COMDWN_ID28		( (u2)(120u) )
#define	U2L_TIM_COMDWN_ID29		( (u2)(1200u) )
#define	U2L_TIM_COMDWN_ID2A		( (u2)(1200u) )
#define	U2L_TIM_COMDWN_ID22		( (u2)(1200u) )
#define	U2L_TIM_COMDWN_ID2B		( (u2)(1200u) )
#define	U2L_TIM_COMDWN_ID24		( (u2)(1200u) )


/******************************************************************************
 * declaretion section
 ******************************************************************************/
void p_vog_tmr_start_id28_cmdwn( void );
void p_vog_tmr_start_id29_cmdwn( void );
void p_vog_tmr_start_id2a_cmdwn( void );
void p_vog_tmr_start_id22_cmdwn( void );
void p_vog_tmr_start_id2b_cmdwn( void );
void p_vog_tmr_start_id24_cmdwn( void );

u1 p_u1g_tmr_jdg_id28_cmdwn( void );
u1 p_u1g_tmr_jdg_id29_cmdwn( void );
u1 p_u1g_tmr_jdg_id2a_cmdwn( void );
u1 p_u1g_tmr_jdg_id22_cmdwn( void );
u1 p_u1g_tmr_jdg_id2b_cmdwn( void );
u1 p_u1g_tmr_jdg_id24_cmdwn( void );

#endif /* P_COMDWN_TIM_H */

/*--- end of file ---*/



