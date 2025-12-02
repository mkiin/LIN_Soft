/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
/* File Name   : l_slin_def.h                               */
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
/*           ･U1G_LIN_EDGE_INTを追加                        */
/*           ･ライブラリリリース用のデフォルト設定に変更    */
/*""FILE COMMENT END""***************************************/

#ifndef __L_SLIN_DEF_H_INCLUDE__
#define __L_SLIN_DEF_H_INCLUDE__

#define  U2G_LIN_NAD                    ((l_u16)0x01)
#define  U1G_LIN_MAX_SLOT               ((l_u8)11)
#define  U1G_LIN_UART                   (U1G_LIN_SCI2)
#define  U1G_LIN_BAUDRATE               (U1G_LIN_BAUDRATE_9600)
#define  U1G_LIN_BRGSRC                 (U1G_LIN_BRGSRC_F1)
#define  U1G_LIN_BRG                    ((l_u8)39)                /* 12MHz,9600bps +1した値を定義 */
#define  U1G_LIN_TM                     (U1G_LIN_TIMERZ0)
#define  U1G_LIN_RSSP                   ((l_u8)1)
#define  U1G_LIN_BTSP                   ((l_u8)1)
#define  U1G_LIN_WAKEUP                 (U1G_LIN_WP_INT_USE)
#define  U1G_LIN_WP_INT                 (U1G_LIN_WP_INT0)
#define  U1G_LIN_EDGE_INT               (U1G_LIN_EDGE_INT_NON)
#define  U1G_LIN_ENDIAN_TYPE            (U1G_LIN_ENDIAN_BIG)

#endif

/***** End of File *****/




