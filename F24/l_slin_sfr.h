/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_sfr.h                                */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_SFR_RL78F24_H_INCLUDE
#define L_SLIN_SFR_RL78F24_H_INCLUDE

#include "aipf_sfr_rl78_f24.h"


/**** Clock ****/
#define U1G_LIN_SFR_PER2                PER2                                            /* 周辺イネーブル・レジスタ2 PER2 */
#define U1G_LIN_SFR_LINCKSEL            LINCKSEL                                        /* LINクロック選択レジスタ LINCKSEL */

#if U1G_LIN_CH == U1G_LIN_LINCH_0
#define U1G_LIN_SFR_BIT_LINEN           LIN0EN                                          /* LIN0EN bit   LIN0の入力クロック供給の制御 */
#define U1G_LIN_SFR_BIT_LINMCKE         LIN0MCKE                                        /* LIN0MCKE bit   LIN0のエンジンクロック供給/停止制御 */
#define U1G_LIN_SFR_BIT_LINMCK          LIN0MCK                                         /* LIN0MCK bit   LIN0のエンジンクロック選択制御 */
#elif U1G_LIN_CH == U1G_LIN_LINCH_1
#define U1G_LIN_SFR_BIT_LINEN           LIN1EN                                          /* LIN1EN bit   LIN1の入力クロック供給の制御 */
#define U1G_LIN_SFR_BIT_LINMCKE         LIN1MCKE                                        /* LIN1MCKE bit   LIN1のエンジンクロック供給/停止制御 */
#define U1G_LIN_SFR_BIT_LINMCK          LIN1MCK                                         /* LIN1MCK bit   LIN1のエンジンクロック選択制御 */
#endif

#if U1G_LIN_TIMER_UNIT == U1G_LIN_TIMER_UNIT_0
#define U1G_LIN_SFR_BIT_TAUEN           TAU0EN
#elif U1G_LIN_TIMER_UNIT == U1G_LIN_TIMER_UNIT_1
#define U1G_LIN_SFR_BIT_TAUEN           TAU1EN
#endif

/***** LIN *****/
#define U1G_LIN_SFR_IF0H                IF0H                                            /* 割り込み要求フラグ・レジスタ IF0H */
#define U1G_LIN_SFR_MK0H                MK0H                                            /* 割り込みマスク・フラグ・レジスタ MK0H */
#define U1G_LIN_SFR_PR_0X0H             PR00H                                           /* 優先順位指定フラグ・レジスタ PR00H */
#define U1G_LIN_SFR_PR_1X0H             PR10H                                           /* 優先順位指定フラグ・レジスタ PR10H */

#define U1G_LIN_SFR_IF1L                IF1L                                            /* 割り込み要求フラグ・レジスタ IF1L */
#define U1G_LIN_SFR_MK1L                MK1L                                            /* 割り込みマスク・フラグ・レジスタ MK1L */
#define U1G_LIN_SFR_PR_0X1L             PR01L                                           /* 優先順位指定フラグ・レジスタ PR01L */
#define U1G_LIN_SFR_PR_1X1L             PR11L                                           /* 優先順位指定フラグ・レジスタ PR11L */

#define U1G_LIN_SFR_IF3L                IF3L                                            /* 割り込み要求フラグ・レジスタ IF3L */
#define U1G_LIN_SFR_MK3L                MK3L                                            /* 割り込みマスク・フラグ・レジスタ MK3L */
#define U1G_LIN_SFR_PR_0X3L             PR03L                                           /* 優先順位指定フラグ・レジスタ PR03L */
#define U1G_LIN_SFR_PR_1X3L             PR13L                                           /* 優先順位指定フラグ・レジスタ PR13L */

#define U1G_LIN_SFR_LCHSEL              LCHSEL                                          /* LINチャネル選択レジスタ LCHSEL */

#if U1G_LIN_CH == U1G_LIN_LINCH_0
#define U1G_LIN_SFR_LWBR                LWBR0                                           /* LINウェイクアップボーレート選択レジスタ LWBR */
#define U2G_LIN_SFR_LBRP                LBRP0                                           /* LINボーレートプリスケーラレジスタ LBRP */
#define U1G_LIN_SFR_LMD                 LMD0                                            /* LINモードレジスタ LMD */
#define U1G_LIN_SFR_LBFC                LBFC0                                           /* LINブレークフィールド設定レジスタ LBFC */
#define U1G_LIN_SFR_LSC                 LSC0                                            /* LINスペース設定レジスタ LSC */
#define U1G_LIN_SFR_LWUP                LWUP0                                           /* LINウェイクアップ設定レジスタ LWUP */
#define U1G_LIN_SFR_LIE                 LIE0                                            /* LIN割り込み許可レジスタ LIE */
#define U1G_LIN_SFR_LEDE                LEDE0                                           /* LINエラー検出許可レジスタ LEDE */
#define U1G_LIN_SFR_LCUC                LCUC0                                           /* LIN制御レジスタ LCUC */
#define U1G_LIN_SFR_LTRC                LTRC0                                           /* LIN送信制御レジスタ LTRC */
#define U1G_LIN_SFR_LMST                LMST0                                           /* LINモードステータスレジスタ LMST */
#define U1G_LIN_SFR_LST                 LST0                                            /* LINステータスレジスタ LST */
#define U1G_LIN_SFR_LEST                LEST0                                           /* LINエラーステータスレジスタ LEST */
#define U1G_LIN_SFR_LDFC                LDFC0                                           /* LINレスポンスフィールド設定レジスタ LDFC */
#define U1G_LIN_SFR_LIDB                LIDB0                                           /* LIN IDバッファレジスタ LIDB */
#define U1G_LIN_SFR_LDB1                LDB01                                           /* LINデータ1バッファレジスタ LDB1 */
#define U1G_LIN_SFR_LDB2                LDB02                                           /* LINデータ2バッファレジスタ LDB2 */
#define U1G_LIN_SFR_LDB3                LDB03                                           /* LINデータ3バッファレジスタ LDB3 */
#define U1G_LIN_SFR_LDB4                LDB04                                           /* LINデータ4バッファレジスタ LDB4 */
#define U1G_LIN_SFR_LDB5                LDB05                                           /* LINデータ5バッファレジスタ LDB5 */
#define U1G_LIN_SFR_LDB6                LDB06                                           /* LINデータ6バッファレジスタ LDB6 */
#define U1G_LIN_SFR_LDB7                LDB07                                           /* LINデータ7バッファレジスタ LDB7 */
#define U1G_LIN_SFR_LDB8                LDB08                                           /* LINデータ8バッファレジスタ LDB8 */
#elif U1G_LIN_CH == U1G_LIN_LINCH_1
#define U1G_LIN_SFR_LWBR                LWBR1                                           /* LINウェイクアップボーレート選択レジスタ LWBR */
#define U2G_LIN_SFR_LBRP                LBRP1                                           /* LINボーレートプリスケーラレジスタ LBRP */
#define U1G_LIN_SFR_LMD                 LMD1                                            /* LINモードレジスタ LMD */
#define U1G_LIN_SFR_LBFC                LBFC1                                           /* LINブレークフィールド設定レジスタ LBFC */
#define U1G_LIN_SFR_LSC                 LSC1                                            /* LINスペース設定レジスタ LSC */
#define U1G_LIN_SFR_LWUP                LWUP1                                           /* LINウェイクアップ設定レジスタ LWUP */
#define U1G_LIN_SFR_LIE                 LIE1                                            /* LIN割り込み許可レジスタ LIE */
#define U1G_LIN_SFR_LEDE                LEDE1                                           /* LINエラー検出許可レジスタ LEDE */
#define U1G_LIN_SFR_LCUC                LCUC1                                           /* LIN制御レジスタ LCUC */
#define U1G_LIN_SFR_LTRC                LTRC1                                           /* LIN送信制御レジスタ LTRC */
#define U1G_LIN_SFR_LMST                LMST1                                           /* LINモードステータスレジスタ LMST */
#define U1G_LIN_SFR_LST                 LST1                                            /* LINステータスレジスタ LST */
#define U1G_LIN_SFR_LEST                LEST1                                           /* LINエラーステータスレジスタ LEST */
#define U1G_LIN_SFR_LDFC                LDFC1                                           /* LINレスポンスフィールド設定レジスタ LDFC */
#define U1G_LIN_SFR_LIDB                LIDB1                                           /* LIN IDバッファレジスタ LIDB */
#define U1G_LIN_SFR_LDB1                LDB11                                           /* LINデータ1バッファレジスタ LDB1 */
#define U1G_LIN_SFR_LDB2                LDB12                                           /* LINデータ2バッファレジスタ LDB2 */
#define U1G_LIN_SFR_LDB3                LDB13                                           /* LINデータ3バッファレジスタ LDB3 */
#define U1G_LIN_SFR_LDB4                LDB14                                           /* LINデータ4バッファレジスタ LDB4 */
#define U1G_LIN_SFR_LDB5                LDB15                                           /* LINデータ5バッファレジスタ LDB5 */
#define U1G_LIN_SFR_LDB6                LDB16                                           /* LINデータ6バッファレジスタ LDB6 */
#define U1G_LIN_SFR_LDB7                LDB17                                           /* LINデータ7バッファレジスタ LDB7 */
#define U1G_LIN_SFR_LDB8                LDB18                                           /* LINデータ8バッファレジスタ LDB8 */
#endif

#if U1G_LIN_CH == U1G_LIN_LINCH_0
#define U1G_LIN_SFR_BIT_LINTRMIF        LIN0TRMIF                                       /* LIN0TRMIF bit   LIN0送信割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINTRMMK        LIN0TRMMK                                       /* LIN0TRMMK bit   LIN0送信割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINTRMPR0       LIN0TRMPR0                                      /* LIN0TRMPR0 bit  LIN0送信割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINTRMPR1       LIN0TRMPR1                                      /* LIN0TRMPR1 bit  LIN0送信割り込み優先順位指定フラグ1 */

#define U1G_LIN_SFR_BIT_LINRVCIF        LIN0RVCIF                                       /* LIN0RVCIF bit   LIN0受信割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINRVCMK        LIN0RVCMK                                       /* LIN0RVCMK bit   LIN0受信割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINRVCPR0       LIN0RVCPR0                                      /* LIN0RVCPR0 bit  LIN0受信割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINRVCPR1       LIN0RVCPR1                                      /* LIN0RVCPR1 bit  LIN0受信割り込み優先順位指定フラグ1 */

#define U1G_LIN_SFR_BIT_LINSTAIF        LIN0STAIF                                       /* LIN0STAIF bit   LIN0ステータスエラー割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINSTAMK        LIN0STAMK                                       /* LIN0STAMK bit   LIN0ステータスエラー割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINSTAPR0       LIN0STAPR0                                      /* LIN0STAPR0 bit  LIN0ステータスエラー割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINSTAPR1       LIN0STAPR1                                      /* LIN0STAPR1 bit  LIN0ステータスエラー割り込み優先順位指定フラグ1 */
#elif U1G_LIN_CH == U1G_LIN_LINCH_1
#define U1G_LIN_SFR_BIT_LINTRMIF        LIN1TRMIF                                       /* LIN1TRMIF bit   LIN1送信割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINTRMMK        LIN1TRMMK                                       /* LIN1TRMMK bit   LIN1送信割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINTRMPR0       LIN1TRMPR0                                      /* LIN1TRMPR0 bit  LIN1送信割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINTRMPR1       LIN1TRMPR1                                      /* LIN1TRMPR1 bit  LIN1送信割り込み優先順位指定フラグ1 */

#define U1G_LIN_SFR_BIT_LINRVCIF        LIN1RVCIF                                       /* LIN1RVCIF bit   LIN1受信割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINRVCMK        LIN1RVCMK                                       /* LIN1RVCMK bit   LIN1受信割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINRVCPR0       LIN1RVCPR0                                      /* LIN1RVCPR0 bit  LIN1受信割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINRVCPR1       LIN1RVCPR1                                      /* LIN1RVCPR1 bit  LIN1受信割り込み優先順位指定フラグ1 */

#define U1G_LIN_SFR_BIT_LINSTAIF        LIN1STAIF                                       /* LIN1STAIF bit   LIN1ステータスエラー割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINSTAMK        LIN1STAMK                                       /* LIN1STAMK bit   LIN1ステータスエラー割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINSTAPR0       LIN1STAPR0                                      /* LIN1STAPR0 bit  LIN1ステータスエラー割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINSTAPR1       LIN1STAPR1                                      /* LIN1STAPR1 bit  LIN1ステータスエラー割り込み優先順位指定フラグ1 */
#endif

/* INT */
#define U1G_LIN_SFR_ISC                 ISC                                             /* 入力切り替え制御レジスタ ISC */
#define U1G_LIN_SFR_EGP1                EGP1                                            /* 外部割り込み立ち上がりエッジ許可レジスタ EGP1 */
#define U1G_LIN_SFR_EGN1                EGN1                                            /* 外部割り込み立ち下がりエッジ許可レジスタ EGN1 */
#define U1G_LIN_SFR_IF2L                IF2L                                            /* 割り込み要求フラグ・レジスタ IF2L */
#define U1G_LIN_SFR_MK2L                MK2L                                            /* 割り込みマスク・フラグ・レジスタ MK2L */
#define U1G_LIN_SFR_PR_0X2L             PR02L                                           /* 優先順位指定フラグ・レジスタ PR02L */
#define U1G_LIN_SFR_PR_1X2L             PR12L                                           /* 優先順位指定フラグ・レジスタ PR12L */

#if U1G_LIN_CH == U1G_LIN_LINCH_0
#define U1G_LIN_SFR_BIT_ISC             ISC_bit.no2                                     /* ISC2 bit   外部割り込み（INTP11）の入力選択 */
#define U1G_LIN_SFR_BIT_LINWUPIF        LIN0WUPIF                                       /* LIN0WUPIF bit   LIN0受信端子入力割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINWUPMK        LIN0WUPMK                                       /* LIN0WUPMK bit   LIN0受信端子入力割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINWUPPR0       LIN0WUPPR0                                      /* LIN0WUPPR0 bit  LIN0受信端子入力割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINWUPPR1       LIN0WUPPR1                                      /* LIN0WUPPR1 bit  LIN0受信端子入力割り込み優先順位指定フラグ1 */
#define U1G_LIN_SFR_BIT_EGP             EGP1_bit.no3                                    /* EGP11 bit   INTPn端子の有効エッジの選択（立ち上がり許可） */
#define U1G_LIN_SFR_BIT_EGN             EGN1_bit.no3                                    /* EGN11 bit   INTPn端子の有効エッジの選択（立ち下がり許可） */
#elif U1G_LIN_CH == U1G_LIN_LINCH_1
#define U1G_LIN_SFR_BIT_ISC             ISC_bit.no3                                     /* ISC3 bit   外部割り込み（INTP12）の入力選択 */
#define U1G_LIN_SFR_BIT_LINWUPIF        LIN1WUPIF                                       /* LIN1WUPIF bit   LIN1受信端子入力割り込み要求フラグ */
#define U1G_LIN_SFR_BIT_LINWUPMK        LIN1WUPMK                                       /* LIN1WUPMK bit   LIN1受信端子入力割り込みマスク・フラグ */
#define U1G_LIN_SFR_BIT_LINWUPPR0       LIN1WUPPR0                                      /* LIN1WUPPR0 bit  LIN1受信端子入力割り込み優先順位指定フラグ0 */
#define U1G_LIN_SFR_BIT_LINWUPPR1       LIN1WUPPR1                                      /* LIN1WUPPR1 bit  LIN1受信端子入力割り込み優先順位指定フラグ1 */
#define U1G_LIN_SFR_BIT_EGP             EGP1_bit.no4                                    /* EGP12 bit   INTPn端子の有効エッジの選択（立ち上がり許可） */
#define U1G_LIN_SFR_BIT_EGN             EGN1_bit.no4                                    /* EGN12 bit   INTPn端子の有効エッジの選択（立ち下がり許可） */
#endif

/* SFRガード機能制御レジスタ */
#define U1G_LIN_SFR_IAWCTL              IAWCTL                                          /* 不正メモリ・アクセス検出制御レジスタ IAWCTL */


/***** TAU *****/
#if U1G_LIN_TIMER_UNIT == U1G_LIN_TIMER_UNIT_0
#define U2G_LIN_TAU_TPS                 TPS0                                            /* タイマ・クロック選択レジスタ TPS */
#define U1G_LIN_TAU_TO                  TO0L                                            /* タイマ出力レジスタ TO */
#define U1G_LIN_TAU_TE                  TE0L                                            /* タイマ・チャネル許可ステータス・レジスタ TE */
#define U1G_LIN_TAU_TOE                 TOE0L                                           /* タイマ出力許可レジスタ TOE */
#define U1G_LIN_TAU_TS                  TS0L                                            /* タイマ・チャネル開始レジスタ TS */
#define U1G_LIN_TAU_TT                  TT0L                                            /* タイマ・チャネル停止レジスタ TT */
#if U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_0
#define U2G_LIN_TAU_TMR                 TMR00                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR00                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR000                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR100                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK00                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF00                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_1
#define U2G_LIN_TAU_TMR                 TMR01                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR01                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR001                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR101                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK01                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF01                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_2
#define U2G_LIN_TAU_TMR                 TMR02                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR02                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR002                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR102                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK02                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF02                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_3
#define U2G_LIN_TAU_TMR                 TMR03                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR03                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR003                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR103                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK03                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF03                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_4
#define U2G_LIN_TAU_TMR                 TMR04                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR04                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR004                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR104                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK04                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF04                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_5
#define U2G_LIN_TAU_TMR                 TMR05                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR05                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR005                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR105                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK05                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF05                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_6
#define U2G_LIN_TAU_TMR                 TMR06                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR06                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR006                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR106                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK06                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF06                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_7
#define U2G_LIN_TAU_TMR                 TMR07                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR07                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR007                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR107                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK07                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF07                                          /* 割り込み要求フラグ・レジスタ TMIF */
#endif
#elif U1G_LIN_TIMER_UNIT == U1G_LIN_TIMER_UNIT_1
#define U2G_LIN_TAU_TPS                 TPS1                                            /* タイマ・クロック選択レジスタ TPS */
#define U1G_LIN_TAU_TO                  TO1L                                            /* タイマ出力レジスタ TO */
#define U1G_LIN_TAU_TE                  TE1L                                            /* タイマ・チャネル許可ステータス・レジスタ TE */
#define U1G_LIN_TAU_TOE                 TOE1L                                           /* タイマ出力許可レジスタ TOE */
#define U1G_LIN_TAU_TS                  TS1L                                            /* タイマ・チャネル開始レジスタ TS */
#define U1G_LIN_TAU_TT                  TT1L                                            /* タイマ・チャネル停止レジスタ TT */
#if U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_0
#define U2G_LIN_TAU_TMR                 TMR10                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR10                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR010                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR110                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK10                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF10                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_1
#define U2G_LIN_TAU_TMR                 TMR11                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR11                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR011                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR111                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK11                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF11                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_2
#define U2G_LIN_TAU_TMR                 TMR12                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR12                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR012                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR112                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK12                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF12                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_3
#define U2G_LIN_TAU_TMR                 TMR13                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR13                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR013                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR113                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK13                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF13                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_4
#define U2G_LIN_TAU_TMR                 TMR14                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR14                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR014                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR114                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK14                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF14                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_5
#define U2G_LIN_TAU_TMR                 TMR15                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR15                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR015                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR115                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK15                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF15                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_6
#define U2G_LIN_TAU_TMR                 TMR16                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR16                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR016                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR116                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK16                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF16                                          /* 割り込み要求フラグ・レジスタ TMIF */
#elif U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_7
#define U2G_LIN_TAU_TMR                 TMR17                                           /* TAU タイマ・モードレジスタ TMR */
#define U2G_LIN_TAU_TDR                 TDR17                                           /* TAU タイマ・データレジスタ TDR */
#define U1G_LIN_TAU_BIT_TMPR0           TMPR017                                         /* 優先順位指定フラグ・レジスタ TMPR0 */
#define U1G_LIN_TAU_BIT_TMPR1           TMPR117                                         /* 優先順位指定フラグ・レジスタ TMPR1 */
#define U1G_LIN_TAU_BIT_TMMK            TMMK17                                          /* 割り込みマスク・フラグ TMMK */
#define U1G_LIN_TAU_BIT_TMIF            TMIF17                                          /* 割り込み要求フラグ・レジスタ TMIF */
#endif
#endif

#endif

/***** End of File *****/



