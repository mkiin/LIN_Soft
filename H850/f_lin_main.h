/*""FILE COMMENT""*************************************************************
 * System Name  : S/R system
 * File Name    : f_lin_main.h
 * Contests     : LIN ʐM   C       w b _
 * Model        : SRF ėp
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

#ifndef F_LIN_MAIN_H
#define F_LIN_MAIN_H

/******************************************************************************
 * Header File Include
 ******************************************************************************/
#include "l_stddef.h"

/******************************************************************************
 * Definition section
 ******************************************************************************/

/******************************************************************************
 * declaretion section
 ******************************************************************************/
void f_vog_lin_init( u1 u1a_mode );
void f_vog_lin_rcv_main(void);
void f_vog_lin_snd_main(void);
void f_vog_lin_mng_ctrl(void);
void f_vogi_inthdr_irq0(void);
void f_vogi_inthdr_tmr_z0(void);
void f_vogi_inthdr_sci3_2( void );

#endif /* F_LIN_MAIN_H */

/*--- end of file ---*/



