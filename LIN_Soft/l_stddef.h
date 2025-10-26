/**
 * @file        l_stddef.h
 *
 * @brief       標準定数定義
 *
 * @attention   編集禁止
 *
 */

#ifndef L_STDDEF_H
#define L_STDDEF_H

/****************************************************************************/
/*																			*/
/* Header File Include 														*/
/*																			*/
/****************************************************************************/
#include "l_type.h"
#include "l_limit.h"

/****************************************************************************/
/*																			*/
/* Definition section														*/
/*																			*/
/****************************************************************************/
/*------TRUE/FALSE---------------------------------------------*/
#define U1G_FLB_TRUE		( (u1)( 1u ) )
#define U1G_FLB_FALSE		( (u1)( 0u ) )
#define U2G_FLB_TRUE		( (u2)( 1u ) )
#define U2G_FLB_FALSE		( (u2)( 0u ) )

/*------Signal Hi/Lo-------------------------------------------*/
#define U1G_FLB_HI			( (u1)( 1u ) )
#define U1G_FLB_LW			( (u1)( 0u ) )

/*------ZERO---------------------------------------------------*/
#define U1G_DAT_ZERO		( (u1)( 0u ) )
#define U2G_DAT_ZERO		( (u2)( 0u ) )
#define U4G_DAT_ZERO		( (u4)( 0u ) )
#define S1G_DAT_ZERO		( (s1)0 )
#define S2G_DAT_ZERO		( (s2)0 )
#define S4G_DAT_ZERO		( (s4)0 )

/*----- 汎用定義 -------------------------------------------*/
#ifndef NULL
#define NULL                    ( 0 )                   /**< @brief NULL */
#endif

#define AIPF_OFF                ( 0 )                   /**< @brief OFF */
#define AIPF_ON                 ( 1 )                   /**< @brief ON */

#define AIPF_OK                 ( 0 )                   /**< @brief OK */
#define AIPF_NG                 ( 1 )                   /**< @brief NG */

#define AIPF_FALSE              ( 0 )                   /**< @brief FALSE */
#define AIPF_TRUE               ( 1 )                   /**< @brief TRUE */

#endif /* L_STDDEF_H */

/***** End of File *****/
