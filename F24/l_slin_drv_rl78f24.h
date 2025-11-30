/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_drv_rl78f24.h                        */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_DRV_RL78F24_H_INCLUDE
#define L_SLIN_DRV_RL78F24_H_INCLUDE

/*** MCU依存部分の定数定義 ***/

/*** LINモジュールモード遷移待ちカウント数 ************************************************/
/* 理論最大所要は [3*fclk + 4*LIN通信クロック源]、前提として [LIN通信クロック源 ≦ fCLK]  */
/*  → より長い [7 * LIN通信クロック源] cycleウェイトが保証できれば良い                   */
/* LIN通信クロック源、メインクロックを4MHz以上（4～40）の前提とする。                     */
/* 最大所要cycleは LIN通信クロック源が最小、メインクロックが最大パターンなので、          */
/*   (((40/4 * メイン・システム・クロック) * 7) ⇒ 70cycle                                */
/* ウェイクアップモード、動作モード遷移待ちループが一周で10cycleのため、7回ループとする   */
/* リセットモード遷移待ちループが一周で9cycleのため、8回ループとする                      */

#define U2G_LIN_MODE_WAIT1_CNT          ((l_u16)7U)     /* ウェイクアップモード、動作モード遷移待ちループカウント */
#define U2G_LIN_MODE_WAIT2_CNT          ((l_u16)8U)     /* リセットモード遷移待ちループカウント */

/* LCHSELレジスタ */
#if U1G_LIN_CH == U1G_LIN_LINCH_0
#define  U1G_LIN_LCHSEL                 ((l_u8)0x00U)   /* LINチャネル0を選択 */
#elif U1G_LIN_CH == U1G_LIN_LINCH_1
#define  U1G_LIN_LCHSEL                 ((l_u8)0x01U)   /* LINチャネル1を選択 */
#endif

/* LINCKSELレジスタ LINnMCKビット */
#if U1G_LIN_CLK_SRC == U1G_LIN_CLKSRC_FCLK
#define  U1G_LIN_LINMCK_SET            ((l_u8)(0x00U)) /* LINクロック選択レジスタ LINnMCKビット設定値 */
#elif U1G_LIN_CLK_SRC == U1G_LIN_CLKSRC_FMX
#define  U1G_LIN_LINMCK_SET            ((l_u8)(0x01U)) /* LINクロック選択レジスタ LINnMCKビット設定値 */
#endif

/* LWBRレジスタ */
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
#if U1G_LIN_CLK <= 12U
#define  U1G_LIN_LWBR_INIT              ((l_u8)0x70U)   /* LINウェイクアップボーレート選択レジスタ初期値（設定値） */
#define  U4G_LIN_PRESCALER              ((l_u32)8UL)
#elif U1G_LIN_CLK <= 24U
#define  U1G_LIN_LWBR_INIT              ((l_u8)0x72U)   /* LINウェイクアップボーレート選択レジスタ初期値（設定値） */
#define  U4G_LIN_PRESCALER              ((l_u32)16UL)
#else
#define  U1G_LIN_LWBR_INIT              ((l_u8)0x74U)   /* LINウェイクアップボーレート選択レジスタ初期値（設定値） */
#define  U4G_LIN_PRESCALER              ((l_u32)32UL)
#endif
#else
#define  U1G_LIN_LWBR_INIT              ((l_u8)0x00U)   /* LINウェイクアップボーレート選択レジスタ初期値（設定値） */
#define  U4G_LIN_PRESCALER              ((l_u32)16UL)
#endif

/* LBRPレジスタ */
#if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
#define  U2G_LIN_LBRP_INIT              ((l_u16)((((( (l_u32)1000000UL * (l_u32)U1G_LIN_CLK ) / U4G_LIN_PRESCALER) + ( (l_u32)9600UL / (l_u32)2UL ) ) / (l_u32)9600UL ) - (l_u32)1UL )) /* LINボーレートプリスケーラ初期値（設定値） */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_10400
#define  U2G_LIN_LBRP_INIT              ((l_u16)((((( (l_u32)1000000UL * (l_u32)U1G_LIN_CLK ) / U4G_LIN_PRESCALER) + ( (l_u32)10400UL / (l_u32)2UL ) ) / (l_u32)10400UL ) - (l_u32)1UL )) /* LINボーレートプリスケーラ初期値（設定値） */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_10417
#define  U2G_LIN_LBRP_INIT              ((l_u16)((((( (l_u32)1000000UL * (l_u32)U1G_LIN_CLK ) / U4G_LIN_PRESCALER) + ( (l_u32)10417UL / (l_u32)2UL ) ) / (l_u32)10417UL ) - (l_u32)1UL )) /* LINボーレートプリスケーラ初期値（設定値） */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
#define  U2G_LIN_LBRP_INIT              ((l_u16)((((( (l_u32)1000000UL * (l_u32)U1G_LIN_CLK ) / U4G_LIN_PRESCALER) + ( (l_u32)19200UL / (l_u32)2UL ) ) / (l_u32)19200UL ) - (l_u32)1UL )) /* LINボーレートプリスケーラ初期値（設定値） */
#endif

/* LMDレジスタ */
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
#define  U1G_LIN_LMD_INIT               ((l_u8)0x12U)   /* LINモードレジスタ初期値（設定値） */
#else
#define  U1G_LIN_LMD_INIT               ((l_u8)0x13U)   /* LINモードレジスタ初期値（設定値） */
#endif

/* LBFCレジスタ */
#if U1G_LIN_BREAK_LENGTH == U1G_LIN_BREAK_LENGTH_11BITS
#define  U1G_LIN_LBFC_INIT              ((l_u8)0x01U)   /* LINブレークフィールド設定レジスタ初期値（設定値） */
#else
#define  U1G_LIN_LBFC_INIT              ((l_u8)0x00U)   /* LINブレークフィールド設定レジスタ初期値（設定値） */
#endif

/* LCUCレジスタ */
#define  U1G_LIN_LCUC_RESET             ((l_u8)0x00U)  /* LINモジュールリセット        */
#define  U1G_LIN_LCUC_WUP               ((l_u8)0x01U)  /* LINモジュールウェイクアップ  */
#define  U1G_LIN_LCUC_RUN               ((l_u8)0x03U)  /* LINモジュール動作            */

/* LMSTレジスタ */
#define  U1G_LIN_LMST_RESET             ((l_u8)0x00U)  /* LINモジュールリセット        */
#define  U1G_LIN_LMST_WUP               ((l_u8)0x01U)  /* LINモジュールウェイクアップ  */
#define  U1G_LIN_LMST_RUN               ((l_u8)0x03U)  /* LINモジュール動作            */
#define  U1G_LIN_LMST_MASK              ((l_u8)0x03U)  /* LINモードステータス確認用マスク値 */
#define  U1G_LIN_LMST_RESET_MASK        ((l_u8)0x01U)  /* LINモードステータス確認用マスク値(RESET) */

/* LSCレジスタ */
#define  U1G_LIN_LSC_RS                 ((l_u8)(U1G_LIN_RSSP & (l_u8)0x07U))                           /* レスポンススペース設定値 RS b2-b0*/
#define  U1G_LIN_LSC_IBS                ((l_u8)((U1G_LIN_BTSP & (l_u8)0x03U) << U1G_LIN_BIT_SFT_4))    /* インタバイトスペース設定値 IBS b5-b4 */
#define  U1G_LIN_LSC_INIT               ((l_u8)(U1G_LIN_LSC_RS | U1G_LIN_LSC_IBS))              /* スペース設定（RS、BS） */

/* LWUPレジスタ */
#define  U1G_LIN_SFR_LWUP_INIT          ((l_u8)((U1G_LIN_WUP - (l_u8)1U) << U1G_LIN_BIT_SFT_4))    /* Wakeup送信幅  WUTL b7-b4 */

/* LIEレジスタ */
#define  U1G_LIN_LIE_INIT               ((l_u8)0x00U)  /* LIEレジスタ初期値   */
#define  U1G_LIN_LIE_RESET              ((l_u8)0x00U)  /* LIEレジスタRESET時設定値   */
#define  U1G_LIN_LIE_SNDWUP             ((l_u8)0x01U)  /* LIEレジスタウェイクアップ送信時設定値   */
#define  U1G_LIN_LIE_RUN                ((l_u8)0x0FU)  /* LIEレジスタレスポンス送受信時設定値     */

/* LEDEレジスタ */
#define  U1G_LIN_LEDE_INIT              ((l_u8)0x00U)  /* LEDEレジスタ初期値   */
#define  U1G_LIN_LEDE_RESET             ((l_u8)0x00U)  /* LEDEレジスタRESET時設定値   */
#define  U1G_LIN_LEDE_SNDWUP            ((l_u8)0x00U)  /* LEDEレジスタウェイクアップ送信時設定値   */
#define  U1G_LIN_LEDE_RUN               ((l_u8)0x59U)   /* LEDEレジスタレスポンス送受信時設定値     */

/* LSTレジスタ */
#define  U1G_LIN_LST_CLR                ((l_u8)0x00U)  /* LSTレジスタクリア値   */
#define  U1G_LIN_LST_CLR_TX             ((l_u8)0xCAU)  /* LSTレジスタバッファ送信完了フラグ(FTC)クリア   */
#define  U1G_LIN_LST_CLR_RX             ((l_u8)0x09U)  /* LST b1,6,7 クリア（受信関連）   */
#define  U1G_LIN_LST_D1RC_MASK          ((l_u8)0x40U)  /* LSTレジスタデータ1受信完了マスク値 */

/* LESTレジスタ */
#define  U1G_LIN_LEST_CLR               ((l_u8)0x00U)  /* LESTレジスタクリア値   */

/* LDFCレジスタ */
#define  U1G_LIN_LDFC_RCDS              ((l_u8)0x10U)  /* レスポンスフィールド通信方向選択ビット 0:受信 1:送信 */

/* LTRCレジスタ */
#define  U1G_LIN_LTRC_RTS               ((l_u8)0x02U)  /* LTRCレジスタレスポンス送信/受信開始値(MOV書き込み限定) */
#define  U1G_LIN_LTRC_LNRR              ((l_u8)0x04U)  /* LTRCレジスタレスポンスなし要求値(MOV書き込み限定)      */
#define  U1G_LIN_LTRC_FTS               ((l_u8)0x01U)  /* LTRCレジスタフレーム送信／ウェイクアップ送受信開始 */

/* LIDBレジスタ */
#define  U1G_LIN_LIDB_ID_MASK           ((l_u8)0x3FU)  /* LIN IDバッファレジスタ IDマスク */

/* TPS レジスタ */
#if   (U1G_LIN_TIMER_CKMK_M0 == U1G_LIN_TIMER_CKMK)
#define  U2G_LIN_TPS_CLR                ((l_u16)((l_u16)0x0FU << 0x00U))                   /* プリスケーラ設定クリア値 */
#define  U2G_LIN_TPS_INIT               ((l_u16)((l_u16)U1G_LIN_FMCK_PRESCALER << 0x00U))  /* タイマ・クロック選択レジスタ プリスケーラ設定(CKm0) (fCLK/(2^U1G_LIN_FMCK_PRESCALER)) */
#elif (U1G_LIN_TIMER_CKMK_M1 == U1G_LIN_TIMER_CKMK)
#define  U2G_LIN_TPS_CLR                ((l_u16)((l_u16)0x0FU << 0x04U))                   /* プリスケーラ設定クリア値 */
#define  U2G_LIN_TPS_INIT               ((l_u16)((l_u16)U1G_LIN_FMCK_PRESCALER << 0x04U))  /* タイマ・クロック選択レジスタ プリスケーラ設定(CKm1) (fCLK/(2^U1G_LIN_FMCK_PRESCALER)) */
#elif (U1G_LIN_TIMER_CKMK_M2 == U1G_LIN_TIMER_CKMK)
#define  U2G_LIN_TPS_CLR                ((l_u16)((l_u16)0x0FU << 0x08U))                   /* プリスケーラ設定クリア値 */
#define  U2G_LIN_TPS_INIT               ((l_u16)((l_u16)U1G_LIN_FMCK_PRESCALER << 0x08U))  /* タイマ・クロック選択レジスタ プリスケーラ設定(CKm2) (fCLK/(2^U1G_LIN_FMCK_PRESCALER)) */
#elif (U1G_LIN_TIMER_CKMK_M3 == U1G_LIN_TIMER_CKMK)
#define  U2G_LIN_TPS_CLR                ((l_u16)((l_u16)0x0FU << 0x0CU))                   /* プリスケーラ設定クリア値 */
#define  U2G_LIN_TPS_INIT               ((l_u16)((l_u16)U1G_LIN_FMCK_PRESCALER << 0x0CU))  /* タイマ・クロック選択レジスタ プリスケーラ設定(CKm3) (fCLK/(2^U1G_LIN_FMCK_PRESCALER)) */
#endif

/* TMR レジスタ */
#if   ( (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_2) || (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_4) || (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_6) )
#define  U2G_LIN_TMR_INIT               ((l_u16)((l_u16)U1G_LIN_TIMER_CKMK << 0x0EU))          /* タイマ・モードレジスタ 初期値 */
#elif ( (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_1) || (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_3) )
#define  U2G_LIN_TMR_INIT               ((l_u16)((l_u16)U1G_LIN_TIMER_CKMK << 0x0EU))          /* タイマ・モードレジスタ 初期値 */
#elif ( (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_0) || (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_5) || (U1G_LIN_TIMER_CH == U1G_LIN_TIMER_CH_7) )
#define  U2G_LIN_TMR_INIT               ((l_u16)((l_u16)U1G_LIN_TIMER_CKMK << 0x0EU))          /* タイマ・モードレジスタ 初期値 */
#endif

/* TO レジスタ */
#define  U1G_LIN_TO_INIT                ((l_u8)((l_u8)0x01U << U1G_LIN_TIMER_CH))  /* タイマ出力レジスタ 初期値(タイマ出力値を0とする), この値と&=演算子を用いて定義体に代入すること */

/* TE レジスタ */
#define  U1G_LIN_TE_ENABLE              ((l_u8)((l_u8)0x01U << U1G_LIN_TIMER_CH))  /* タイマ・チャネル許可ステータスレジスタ 動作許可状態 */

/* TOE レジスタ */
#define  U1G_LIN_TOE_INIT               ((l_u8)((l_u8)0x01U << U1G_LIN_TIMER_CH))  /* タイマ出力許可レジスタ 初期値(0: タイマ出力を禁止), この値と&=演算子を用いて定義体に代入すること */

/* TS レジスタ */
#define  U1G_LIN_TS_START               ((l_u8)((l_u8)0x01U << U1G_LIN_TIMER_CH))  /* タイマ・チャネル開始レジスタ 開始トリガ */

/* TT レジスタ */
#define  U1G_LIN_TT_STOP                ((l_u8)((l_u8)0x01U << U1G_LIN_TIMER_CH))  /* タイマ・チャネル停止レジスタ 停止トリガ */

/* xxPR0/xxPR1レジスタ（割り込みレベル設定） */
#define  U1G_LIN_WUP_PR0                ((l_u8)( U1G_LIN_WUP_INTLEVEL & U1G_LIN_BIT0))                  /* LIN受信端子入力割り込みレベル 下位 */
#define  U1G_LIN_WUP_PR1                ((l_u8)((U1G_LIN_WUP_INTLEVEL & U1G_LIN_BIT1) >> U1G_LIN_1))    /* LIN受信端子入力割り込みレベル 上位 */
#define  U1G_LIN_TRM_PR0                ((l_u8)( U1G_LIN_TRM_INTLEVEL & U1G_LIN_BIT0))                  /* LIN送信完了割り込みレベル 下位 */
#define  U1G_LIN_TRM_PR1                ((l_u8)((U1G_LIN_TRM_INTLEVEL & U1G_LIN_BIT1) >> U1G_LIN_1))    /* LIN送信完了割り込みレベル 上位 */
#define  U1G_LIN_RVC_PR0                ((l_u8)( U1G_LIN_RVC_INTLEVEL & U1G_LIN_BIT0))                  /* LIN受信完了割り込みレベル 下位 */
#define  U1G_LIN_RVC_PR1                ((l_u8)((U1G_LIN_RVC_INTLEVEL & U1G_LIN_BIT1) >> U1G_LIN_1))    /* LIN受信完了割り込みレベル 上位 */
#define  U1G_LIN_ERR_PR0                ((l_u8)( U1G_LIN_STA_INTLEVEL & U1G_LIN_BIT0))                  /* LINステータスエラー割り込みレベル 下位 */
#define  U1G_LIN_ERR_PR1                ((l_u8)((U1G_LIN_STA_INTLEVEL & U1G_LIN_BIT1) >> U1G_LIN_1))    /* LINステータスエラー割り込みレベル 上位 */
#define  U1G_LIN_TMPR_PR0               ((l_u8)( U1G_LIN_TIMER_INTLEVEL & U1G_LIN_BIT0))                /* TAU優先順位指定フラグレベル 下位 */
#define  U1G_LIN_TMPR_PR1               ((l_u8)((U1G_LIN_TIMER_INTLEVEL & U1G_LIN_BIT1) >> U1G_LIN_1))  /* TAU優先順位指定フラグレベル 上位 */


#define  U1G_LIN_BIT_SFT_4              ((l_u8)4U)
#define  U1G_LIN_BIT_SFT_5              ((l_u8)5U)

#define  U1G_LIN_PRSC_BASE              ((l_u8)0x01U)       /* プリスケーラ設定用底, 使用例: U1G_LIN_PRSC_BASE << U1G_LIN_FMCK_PRESCALER */
#define  U1G_LIN_FMCK_PRESCALER         ((l_u8)2U)          /* プリスケーラ設定, fCLK/2^n の n を決定する */
#define  U2G_LIN_COND_ERROR_TIME        ((l_u16)2343U)      /* Framingエラー確定に必要なタイマカウント値[us] */
#define  U4G_LIN_FCLK_FREQ              ((l_u32)((l_u32)U1G_LIN_TIMER_CLK / ((l_u32)U1G_LIN_PRSC_BASE << (l_u32)U1G_LIN_FMCK_PRESCALER)))
                                                            /* カウンタ・クロックの周波数 [MHz] */
#define  U2G_LIN_TDR_COUNT_VAL          ((l_u16)(((l_u32)U2G_LIN_COND_ERROR_TIME * U4G_LIN_FCLK_FREQ)))  /* カウンタ値の算出 */


/*** マクロ定義 ***/
#define U1G_LIN_REG_PSW_IE_MASK        ( (l_u8)0x80U)   /* IEフラグマスク値 */

/*** マクロ関数 ***/
#pragma inline_asm __inline_asm_func_DI
static void __inline_asm_func_DI(void)
{
    DI
}

#pragma inline_asm __inline_asm_func_EI
static void __inline_asm_func_EI(void)
{
    EI
}

#define l_vod_lin_DI()                                                              \
do                                                                                  \
{                                                                                   \
    l_u8 aipf_psw;                              /* PSW値 */                         \
    aipf_psw = __get_psw();                     /* PSW(アドレス：0xFFFFA) 退避 */   \
    __inline_asm_func_DI()                      /* 即時割り込み禁止 */

#define l_vod_lin_EI()                                                              \
    if( U1G_LIN_REG_PSW_IE_MASK == ( U1G_LIN_REG_PSW_IE_MASK & aipf_psw ) )        \
    {                                                                               \
        __inline_asm_func_EI();                 /* 即時割り込み許可 */              \
    }                                                                               \
    else{}                                                                          \
}                                                                                   \
while( 0 )


/*** 関数のプロトタイプ宣言(global) ***/
l_u8   l_u1g_lin_linmodule_init(void);
void   l_vog_lin_timer_init(void);
void   l_vog_lin_start_timer(void);
void   l_vog_lin_stop_timer(void);
void   l_vog_lin_int_init(void);
void   l_vog_lin_int_enb(void);
void   l_vog_lin_int_dis(void);
l_u8   l_u1g_lin_set_reset(void);
l_u8   l_u1g_lin_set_sndwkup(void);
l_u8   l_u1g_lin_set_run(void);
l_u8   l_u1g_lin_resp_snd_start(l_u8 u1a_lin_dl, l_u8 u1a_sum_type, const l_u8 pta_lin_snd_data[]);
l_u8   l_u1g_lin_resp_rcv_start(l_u8 u1a_lin_dl, l_u8 u1a_sum_type);
l_u8   l_u1g_lin_resp_noreq(void);
l_u8   l_u1g_lin_snd_wakeup(void);
void   l_vog_lin_get_rcvdata(l_u8 pta_lin_rcv_data[]);
#if U1G_LIN_BAUD_RATE_DETECT == U1G_LIN_BAUD_RATE_DETECT_ON
l_u8   l_u1g_lin_is_valid_baud_rate(void);
#endif
void l_vog_lin_determine_ResTooShort_or_NoRes(l_u8 u1a_lst_d1rc_state);

#endif

/***** End of File *****/



