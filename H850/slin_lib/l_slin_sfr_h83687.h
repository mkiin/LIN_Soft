/*""FILE COMMENT""*******************************************/
/* System Name : LSLib3687T                                 */
/* File Name   : l_slin_sfr_h83687.h                        */
/* Version     : 2.00                                       */
/* Contents    :                                            */
/* Customer    : SUNNY GIKEN INC.                           */
/* Model       :                                            */
/* Order       :                                            */
/* CPU         : H8/3687                                    */
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
/*""FILE COMMENT END""***************************************/

#ifndef __L_SLIN_SFR_H83687__
#define __L_SLIN_SFR_H83687__

/*** 定数定義 ***/
/* デバイス依存部分の定数定義 */
#define  U1G_LIN_SCI1               (0)
#define  U1G_LIN_SCI2               (1)

#define  U1G_LIN_BRGSRC_F1          (0)
#define  U1G_LIN_BRGSRC_F4          (1)

#define  U1G_LIN_TIMERZ0            (0)
#define  U1G_LIN_TIMERZ1            (1)

#define  U1G_LIN_WP_INT0            (0)
#define  U1G_LIN_WP_INT1            (1)
#define  U1G_LIN_WP_INT2            (2)
#define  U1G_LIN_WP_INT3            (3)

/*** 構造体、共用体定義 ***/
/* デバイス依存部分のSFR定義 */
typedef union {
    l_u8    u1l_lin_byte;
    struct {
        l_u8    u1l_lin_b7:1;
        l_u8    u1l_lin_b6:1;
        l_u8    u1l_lin_b5:1;
        l_u8    u1l_lin_b4:1;
        l_u8    u1l_lin_b3:1;
        l_u8    u1l_lin_b2:1;
        l_u8    u1l_lin_b1:1;
        l_u8    u1l_lin_b0:1;
    } st_bit;
} un_lin_byte_def_type;

typedef union {
    l_u16   u2l_lin_word;
    struct {
        l_u16   u2l_lin_b15:1;
        l_u16   u2l_lin_b14:1;
        l_u16   u2l_lin_b13:1;
        l_u16   u2l_lin_b12:1;
        l_u16   u2l_lin_b11:1;
        l_u16   u2l_lin_b10:1;
        l_u16   u2l_lin_b9:1;
        l_u16   u2l_lin_b8:1;
        l_u16   u2l_lin_b7:1;
        l_u16   u2l_lin_b6:1;
        l_u16   u2l_lin_b5:1;
        l_u16   u2l_lin_b4:1;
        l_u16   u2l_lin_b3:1;
        l_u16   u2l_lin_b2:1;
        l_u16   u2l_lin_b1:1;
        l_u16   u2l_lin_b0:1;
    } st_bit;
} un_lin_word_def_type;

/***** I/Oポート *****/
/* ポートモードレジスタ1 端子機能の切替えに必要 */
extern   un_lin_byte_def_type           U1G_LIN_REG_PMR1;

/***** SCI *****/
/* SCI1使用 */
#if U1G_LIN_UART == U1G_LIN_SCI1
    /* レシーブデータレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_RDR_1;
    #define  U1G_LIN_REG_RDR        U1G_LIN_REG_RDR_1
    /* トランスミットデータレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TDR_1;
    #define  U1G_LIN_REG_TDR        U1G_LIN_REG_TDR_1
    /* シリアルモードレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_SMR_1;
    #define U1G_LIN_REG_SMR         U1G_LIN_REG_SMR_1
    /* シリアルコントロールレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_SCR3_1;
    #define  U1G_LIN_REG_SCR3       U1G_LIN_REG_SCR3_1
    #define  U1G_LIN_FLG_STIC       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b7
    #define  U1G_LIN_FLG_SRIC       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b6
    #define  U1G_LIN_FLG_STEN       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b5
    #define  U1G_LIN_FLG_SREN       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b4
    /* シリアルステータスレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_SSR_1;
    #define  U1G_LIN_REG_SSR        U1G_LIN_REG_SSR_1
    #define  U1G_LIN_FLG_SRF        U1G_LIN_REG_SSR.st_bit.u1l_lin_b6
    #define  U1G_LIN_FLG_SOER       U1G_LIN_REG_SSR.st_bit.u1l_lin_b5
    #define  U1G_LIN_FLG_SFER       U1G_LIN_REG_SSR.st_bit.u1l_lin_b4
    #define  U1G_LIN_FLG_SPER       U1G_LIN_REG_SSR.st_bit.u1l_lin_b3
    /* ビットレートレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_BRR_1;
    #define  U1G_LIN_REG_BRR        U1G_LIN_REG_BRR_1
    /* ポートモードレジスタ1 */
    #define  U1G_LIN_FLG_PMTXD      U1G_LIN_REG_PMR1.st_bit.u1l_lin_b1
/* SCI2使用 */
#elif U1G_LIN_UART == U1G_LIN_SCI2
    /* レシーブデータレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_RDR_2;
    #define  U1G_LIN_REG_RDR        U1G_LIN_REG_RDR_2
    /* トランスミットデータレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TDR_2;
    #define  U1G_LIN_REG_TDR        U1G_LIN_REG_TDR_2
    /* シリアルモードレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_SMR_2;
    #define  U1G_LIN_REG_SMR        U1G_LIN_REG_SMR_2
    /* シリアルコントロールレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_SCR3_2;
    #define  U1G_LIN_REG_SCR3       U1G_LIN_REG_SCR3_2
    #define  U1G_LIN_FLG_STIC       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b7
    #define  U1G_LIN_FLG_SRIC       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b6
    #define  U1G_LIN_FLG_STEN       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b5
    #define  U1G_LIN_FLG_SREN       U1G_LIN_REG_SCR3.st_bit.u1l_lin_b4
    /* シリアルステータスレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_SSR_2;
    #define  U1G_LIN_REG_SSR        U1G_LIN_REG_SSR_2
    #define  U1G_LIN_FLG_SRF        U1G_LIN_REG_SSR.st_bit.u1l_lin_b6
    #define  U1G_LIN_FLG_SOER       U1G_LIN_REG_SSR.st_bit.u1l_lin_b5
    #define  U1G_LIN_FLG_SFER       U1G_LIN_REG_SSR.st_bit.u1l_lin_b4
    #define  U1G_LIN_FLG_SPER       U1G_LIN_REG_SSR.st_bit.u1l_lin_b3
    /* ビットレートレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_BRR_2;
    #define U1G_LIN_REG_BRR         U1G_LIN_REG_BRR_2
    /* ポートモードレジスタ1 */
    #define U1G_LIN_FLG_PMTXD       U1G_LIN_REG_PMR1.st_bit.u1l_lin_b3
#endif

/***** Timer *****/
/* TimerZ0使用 */
#if U1G_LIN_TM == U1G_LIN_TIMERZ0
    /* タイマスタートレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TSTR_Z0;
    #define  U1G_LIN_REG_TSTR       U1G_LIN_REG_TSTR_Z0
    #define  U1G_LIN_FLG_TMST       U1G_LIN_REG_TSTR.st_bit.u1l_lin_b0
    /* タイマカウンタ */
    extern   un_lin_word_def_type   U2G_LIN_REG_TCNT_Z0;
    #define  U2G_LIN_REG_TCNT       U2G_LIN_REG_TCNT_Z0
    /* ジェネラルレジスタ */
    extern   un_lin_word_def_type   U2G_LIN_REG_GRA_Z0;
    #define  U2G_LIN_REG_GRA        U2G_LIN_REG_GRA_Z0
    /* タイマコントロールレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TCR_Z0;
    #define  U1G_LIN_REG_TCR        U1G_LIN_REG_TCR_Z0
    /* タイマI/Oコントロールレジスタ(TIORA) */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TIORA_Z0;
    #define  U1G_LIN_REG_TIORA      U1G_LIN_REG_TIORA_Z0
    /* タイマステータスレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TSR_Z0;
    #define  U1G_LIN_REG_TSR        U1G_LIN_REG_TSR_Z0
    #define  U1G_LIN_FLG_TMIR       U1G_LIN_REG_TSR.st_bit.u1l_lin_b0
    /* タイマインタラプトイネーブルレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TIER_Z0;
    #define  U1G_LIN_REG_TIER       U1G_LIN_REG_TIER_Z0
    #define  U1G_LIN_FLG_TMIC       U1G_LIN_REG_TIER.st_bit.u1l_lin_b0
/* TimerZ1使用 */
#elif U1G_LIN_TM == U1G_LIN_TIMERZ1
    /* タイマスタートレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TSTR_Z1;
    #define  U1G_LIN_REG_TSTR       U1G_LIN_REG_TSTR_Z1
    #define  U1G_LIN_FLG_TMST       U1G_LIN_REG_TSTR.st_bit.u1l_lin_b1
    /* タイマカウンタ */
    extern   un_lin_word_def_type   U2G_LIN_REG_TCNT_Z1;
    #define  U2G_LIN_REG_TCNT       U2G_LIN_REG_TCNT_Z1
    /* ジェネラルレジスタ */
    extern   un_lin_word_def_type   U2G_LIN_REG_GRA_Z1;
    #define  U2G_LIN_REG_GRA        U2G_LIN_REG_GRA_Z1
    /* タイマコントロールレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TCR_Z1;
    #define  U1G_LIN_REG_TCR        U1G_LIN_REG_TCR_Z1
    /* タイマI/Oコントロールレジスタ(TIORA) */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TIORA_Z1;
    #define  U1G_LIN_REG_TIORA      U1G_LIN_REG_TIORA_Z1
    /* タイマステータスレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TSR_Z1;
    #define  U1G_LIN_REG_TSR        U1G_LIN_REG_TSR_Z1
    #define  U1G_LIN_FLG_TMIR       U1G_LIN_REG_TSR.st_bit.u1l_lin_b0
    /* タイマインタラプトイネーブルレジスタ */
    extern   un_lin_byte_def_type   U1G_LIN_REG_TIER_Z1;
    #define  U1G_LIN_REG_TIER       U1G_LIN_REG_TIER_Z1
    #define  U1G_LIN_FLG_TMIC       U1G_LIN_REG_TIER.st_bit.u1l_lin_b0
#endif

/***** INT *****/
    extern   un_lin_byte_def_type   U1G_LIN_REG_IEGR1;
    extern   un_lin_byte_def_type   U1G_LIN_REG_IENR1;
    extern   un_lin_byte_def_type   U1G_LIN_REG_IRR1;
/* IRQ0使用 */
#if U1G_LIN_WP_INT == U1G_LIN_WP_INT0
    /* 割り込みエッジセレクトレジスタ1 */
    #define  U1G_LIN_FLG_INTEG      U1G_LIN_REG_IEGR1.st_bit.u1l_lin_b0
    /* 割り込みイネーブルレジスタ1 */
    #define  U1G_LIN_FLG_INTIC      U1G_LIN_REG_IENR1.st_bit.u1l_lin_b0
    /* 割り込みフラグレジスタ1 */
    #define  U1G_LIN_FLG_INTIR      U1G_LIN_REG_IRR1.st_bit.u1l_lin_b0
    /* ポートモードレジスタ1 */
    #define U1G_LIN_FLG_PMIRQ       U1G_LIN_REG_PMR1.st_bit.u1l_lin_b4
/* IRQ1使用 */
#elif U1G_LIN_WP_INT == U1G_LIN_WP_INT1
    /* 割り込みエッジセレクトレジスタ1 */
    #define  U1G_LIN_FLG_INTEG      U1G_LIN_REG_IEGR1.st_bit.u1l_lin_b1
    /* 割り込みイネーブルレジスタ1 */
    #define  U1G_LIN_FLG_INTIC      U1G_LIN_REG_IENR1.st_bit.u1l_lin_b1
    /* 割り込みフラグレジスタ1 */
    #define  U1G_LIN_FLG_INTIR      U1G_LIN_REG_IRR1.st_bit.u1l_lin_b1
    /* ポートモードレジスタ1 */
    #define  U1G_LIN_FLG_PMIRQ      U1G_LIN_REG_PMR1.st_bit.u1l_lin_b5
/* IRQ2使用 */
#elif U1G_LIN_WP_INT == U1G_LIN_WP_INT2
    /* 割り込みエッジセレクトレジスタ1 */
    #define  U1G_LIN_FLG_INTEG      U1G_LIN_REG_IEGR1.st_bit.u1l_lin_b2
    /* 割り込みイネーブルレジスタ1 */
    #define  U1G_LIN_FLG_INTIC      U1G_LIN_REG_IENR1.st_bit.u1l_lin_b2
    /* 割り込みフラグレジスタ1 */
    #define  U1G_LIN_FLG_INTIR      U1G_LIN_REG_IRR1.st_bit.u1l_lin_b2
    /* ポートモードレジスタ1 */
    #define  U1G_LIN_FLG_PMIRQ      U1G_LIN_REG_PMR1.st_bit.u1l_lin_b6
/* IRQ3使用 */
#elif U1G_LIN_WP_INT == U1G_LIN_WP_INT3
    /* 割り込みエッジセレクトレジスタ1 */
    #define  U1G_LIN_FLG_INTEG      U1G_LIN_REG_IEGR1.st_bit.u1l_lin_b3
    /* 割り込みイネーブルレジスタ1 */
    #define  U1G_LIN_FLG_INTIC      U1G_LIN_REG_IENR1.st_bit.u1l_lin_b3
    /* 割り込みフラグレジスタ1 */
    #define  U1G_LIN_FLG_INTIR      U1G_LIN_REG_IRR1.st_bit.u1l_lin_b3
    /* ポートモードレジスタ1 */
    #define  U1G_LIN_FLG_PMIRQ      U1G_LIN_REG_PMR1.st_bit.u1l_lin_b7
#endif

#endif

/***** End of File *****/





