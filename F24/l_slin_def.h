/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_def.h                                */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_DEF_H_INCLUDE
#define L_SLIN_DEF_H_INCLUDE

/*** 定数定義 ***/
#define  U1G_LIN_NAD                ((l_u8)0x01U)                 /* ノードアドレス */

#define  U1G_LIN_MAX_SLOT           ((l_u8)8U)                    /* 最大LINバッファスロット数 */
#define  U1G_LIN_CLK_SRC            (U1G_LIN_CLKSRC_FCLK)         /*  LINモジュールへ供給するクロックソースの選択 */
#define  U1G_LIN_CLK                (40U)                         /*  LINモジュールへ供給するクロック周波数  MHz単位  */
#define  U1G_LIN_BAUDRATE           (U1G_LIN_BAUDRATE_19200)      /* 設定ボーレートの選択 */
#define  U1G_LIN_CH                 (U1G_LIN_LINCH_0)             /* 使用するLINチャネルの選択 */
#define  U1G_LIN_RSSP               ((l_u8)1U)                    /* Response space */
#define  U1G_LIN_BTSP               ((l_u8)1U)                    /* Inter-byte space */
#define  U1G_LIN_WUP                ((l_u8)7U)                    /* ウェイクアップ信号送出幅(Tbits) */
#define  U1G_LIN_TRM_INTLEVEL       ((l_u8)2U)                    /* LIN送信完了割り込みレベル設定値 */
#define  U1G_LIN_RVC_INTLEVEL       ((l_u8)2U)                    /* LIN受信完了割り込みレベル設定値 */
#define  U1G_LIN_STA_INTLEVEL       ((l_u8)2U)                    /* LINステータスエラー割り込みレベル設定値 */
#define  U1G_LIN_WUP_INTLEVEL       ((l_u8)2U)                    /* LIN受信端子入力割り込みレベル設定値 */
#define  U2G_LIN_TIME_BASE          ((l_u16)5U)                   /* タイムベース時間の設定(ms) */
#define  U2G_LIN_BUS_TIMEOUT        ((l_u16)4100U)                /* Bus inactive時間の設定（ms）*/
#define  U1G_LIN_ENDIAN_TYPE        (U1G_LIN_ENDIAN_LITTLE)       /* エンディアンタイプの選択 */
#define  U1G_LIN_ERRCLR_PRE_RUN     (U1G_LIN_ERRCLR_ON)           /* RUN状態遷移時のヘッダエラークリア実施/不実施 */
#define  U1G_LIN_ROM_SHRINK         (U1G_LIN_ROM_SHRINK_OFF)      /* ROM削減(未使用APIの削除)実施/不実施 */
#define  U1G_LIN_BAUD_RATE_DETECT   (U1G_LIN_BAUD_RATE_DETECT_ON) /* オートボーレート機能の有効/無効 */
#define  U2G_LIN_MINIMUM_LBRP       ((l_u16)56U)                  /* オートボーレート時における許容されるLBRPの最小値 */
#define  U2G_LIN_MAXIMUM_LBRP       ((l_u16)75U)                  /* オートボーレート時における許容されるLBRPの最大値 */
/* TLIN仕様では、狙いのボーレート±14%以内のボーレートを許容することが求められるため19200の±14%以内の値をdefault値とする、 */
/* (40[MHz](U1G_LIN_CLK) ÷ 4(LPRS) ÷ 19200[bps](ボーレート) ÷ 8(NSPB) ) ±14% (小数点は範囲が広がる方向へ切り上げ・切り捨て) -1(LBRPは設定値+1されるため) */
/* ※LPRS値はU1G_LIN_CLKの設定値に依存することに注意する */
#define  U1G_LIN_BREAK_LENGTH       (U1G_LIN_BREAK_LENGTH_11BITS) /* Break検出幅 */
#define  U1G_LIN_TIMER_CLK          ((l_u8)40U)                   /* TAUモジュールへ供給するクロック周波数(fCLK)  MHz単位  */
#define  U1G_LIN_TIMER_UNIT         (U1G_LIN_TIMER_UNIT_1)        /* TAUユニット設定, 0~1 から選択する */
#define  U1G_LIN_TIMER_CH           (U1G_LIN_TIMER_CH_7)          /* TAUチャネル設定, 0~7 から選択する */
#define  U1G_LIN_TIMER_CKMK         (U1G_LIN_TIMER_CKMK_M3)       /* TAU動作クロック(fMCK)設定, CKm0~CKm3 から選択する */
#define  U1G_LIN_TIMER_INTLEVEL     ((l_u8)2U)                    /* TAUカウント完了割り込みレベル設定値 */

#endif

/***** End of File *****/



