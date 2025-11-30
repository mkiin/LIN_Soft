/**
 * @file        l_limit.h
 *
 * @brief       型定義
 *
 * @attention   編集禁止
 *
 */
 
#ifndef L_LIMIT_H
#define L_LIMIT_H

/****************************************************************************/
/*																			*/
/* Header File Include 														*/
/*																			*/
/****************************************************************************/
#include "l_stddef.h"

/****************************************************************************/
/*																			*/
/* Definition section														*/
/*																			*/
/****************************************************************************/
/*------common max------------------------------------------*/
#define S1G_DAT_MAX			(               (s1)127 )   /**< @brief s1型最大値   型: s1 */
#define U1G_DAT_MAX			(              (u1)255U )   /**< @brief u1型最大値   型: u1 */
#define S2G_DAT_MAX			(             (s2)32767 )   /**< @brief s2型最大値   型: s2 */
#define U2G_DAT_MAX			(            (u2)65535U )   /**< @brief u2型最大値   型: u2 */
#define S4G_DAT_MAX			(       (s4)2147483647L )   /**< @brief s4型最大値   型: s4 */
#define U4G_DAT_MAX			(      (u4)4294967295lU )   /**< @brief u4型最大値   型: u4 */

/*------common min------------------------------------------*/
/* ISO対応のため領域を狭めた値 */
#define S1G_DAT_MIN			(              (s1)-127 )   /**< @brief s1型最小値   型: s1 */
#define U1G_DAT_MIN			(            (u1)( 0U ) )   /**< @brief i1型最小値   型: u1 */
#define S2G_DAT_MIN			(            (s2)-32767 )   /**< @brief s2型最小値   型: s2 */
#define U2G_DAT_MIN			(            (u2)( 0U ) )   /**< @brief i2型最小値   型: u2 */
#define S4G_DAT_MIN			(	   (s4)-2147483647L )   /**< @brief s4型最小値   型: s4 */
#define U4G_DAT_MIN			(            (u4)( 0U ) )   /**< @brief i4型最小値   型: u4 */

#endif /* L_LIMIT_H */

/***** End of File *****/