/**
 * @file        l_slin_def.hl_slin_def.h
 *
 * @brief       SLIN コンフィグレーション定義
 *
 * @attention   編集禁止
 *
 */
#ifndef __L_SLIN_DEF_H_INCLUDE__
#define __L_SLIN_DEF_H_INCLUDE__

/*** 定数定義 ***/
#define  U2G_LIN_NAD                    ((l_u16)0x01)                /* ノードアドレス */
#define  U1G_LIN_MAX_SLOT               ((l_u8)5)                    /* 最大LINバッファスロット数 */
#define  U1G_LIN_UART                   (U1G_LIN_SCI2)
#define  U1G_LIN_BAUDRATE               (U1G_LIN_BAUDRATE_19200)     /* 設定ボーレートの選択 */
#define  U1G_LIN_RSSP                   ((l_u8)1)                    /* Response space */
#define  U1G_LIN_BTSP                   ((l_u8)1)                    /* Inter-byte space */
#define  U1G_LIN_WAKEUP                 (U1G_LIN_WP_INT_USE)         /* WAKEUP検出にINT割り込みを使用 */
#define  U1G_LIN_WP_INT                 (U1G_LIN_WP_INT0)
#define  U1G_LIN_EDGE_INT               (U1G_LIN_EDGE_INT_NON)       /* ウェイクアップ信号検出エッジの極性切り替え */
#define  U1G_LIN_ENDIAN_TYPE            (U1G_LIN_ENDIAN_LITTLE)      /* エンディアンタイプの選択 */
/* syscfgで既に定義済みのため削除 */
/* #define  U1G_LIN_INTPIN                 ((l_u8)24)              *< @brief INT割り込み用PIN番号    型: l_u8 */

#endif

/***** End of File *****/