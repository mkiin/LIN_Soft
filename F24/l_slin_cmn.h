/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_cmn.h                                */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_CMN_H_INCLUDE
#define L_SLIN_CMN_H_INCLUDE

/***** 型宣言 *****/
typedef unsigned char   l_u8;
typedef unsigned short  l_u16;
typedef unsigned long   l_u32;
typedef unsigned short  l_bool;
typedef unsigned char   l_frame_handle;

/***** 定数定義 *****/

/* エンディアンタイプ */
#define  U1G_LIN_ENDIAN_LITTLE      (0U)            /* リトルエンディアンタイプ */
#define  U1G_LIN_ENDIAN_BIG         (1U)            /* ビッグエンディアンタイプ */

/* 通信ボーレート */
#define  U1G_LIN_BAUDRATE_9600      (0U)            /* 通信ボーレート9600bpsを使用  */
#define  U1G_LIN_BAUDRATE_10400     (1U)            /* 通信ボーレート10400bpsを使用 */
#define  U1G_LIN_BAUDRATE_19200     (2U)            /* 通信ボーレート19200bpsを使用 */
#define  U1G_LIN_BAUDRATE_10417     (3U)            /* 通信ボーレート10417bpsを使用 */

/* LINチャネル */
#define  U1G_LIN_LINCH_0            (0U)            /* LINチャネル0を使用 */
#define  U1G_LIN_LINCH_1            (1U)            /* LINチャネル1を使用 */

/* デバイス依存部分の定数定義 */

#define  U1G_LIN_CLKSRC_FCLK        (0U)            /* LINクロックソースにfCLKを選択 */
#define  U1G_LIN_CLKSRC_FMX         (1U)            /* LINクロックソースにfMXを選択 */

/* RUN遷移時のヘッダエラークリア実施/不実施 */
#define  U1G_LIN_ERRCLR_ON          (0U)            /* RUN遷移時のヘッダエラークリア実施   */
#define  U1G_LIN_ERRCLR_OFF         (1U)            /* RUN遷移時のヘッダエラークリア不実施 */

/* ROM削減(未使用APIの削除)実施/不実施 */
#define  U1G_LIN_ROM_SHRINK_ON      (1U)            /* ROM削減実施 */
#define  U1G_LIN_ROM_SHRINK_OFF     (0U)            /* ROM削減不実施 */

/* オートボーレート on/off */
#define  U1G_LIN_BAUD_RATE_DETECT_ON   (1U)         /* オートボーレート 有効 */
#define  U1G_LIN_BAUD_RATE_DETECT_OFF  (0U)         /* オートボーレート 無効 */

/* Breakの長さ */
#define  U1G_LIN_BREAK_LENGTH_10BITS   (10U)        /* Breakの長さ 10ビット(固定ボーレートの場合9.5)  */
#define  U1G_LIN_BREAK_LENGTH_11BITS   (11U)        /* Breakの長さ 11ビット(固定ボーレートの場合10.5) */

/* unsigned char型のOK/NG */
#define  U1G_LIN_OK                 ((l_u8)0U)      /* 処理成功 */
#define  U1G_LIN_NG                 ((l_u8)1U)      /* 処理失敗 */

/* bool型のOK/NG */
#define  U2G_LIN_OK                 ((l_bool)0U)    /* 処理成功 */
#define  U2G_LIN_NG                 ((l_bool)1U)    /* 処理失敗 */

/* LINステータス */
#define  U1G_LIN_STS_RESET          ((l_u8)0U)      /* LINステータス = RESET       */
#define  U1G_LIN_STS_SLEEP          ((l_u8)1U)      /* LINステータス = SLEEP       */
#define  U1G_LIN_STS_RUN_STANDBY    ((l_u8)2U)      /* LINステータス = RUN STANDBY */
#define  U1G_LIN_STS_RUN            ((l_u8)3U)      /* LINステータス = RUN         */

#define  U1G_LIN_MAX_SLOT_NUM       ((l_u8)64U)     /* スロット最大数 */

#define  U1G_LIN_WORD_BIT           ((l_u8)16U)     /* word型をbitで分割(16bit) */

/* ID Info Send/Recv Setting */
#define  U1G_LIN_CMD_SND            ((l_u8)0U)      /* 送信フレーム */
#define  U1G_LIN_CMD_RCV            ((l_u8)1U)      /* 受信フレーム */

/* NM使用/未使用(unsigned char型) */
#define  U1G_LIN_NM_NO_USE          ((l_u8)0U)      /* NM未使用 */
#define  U1G_LIN_NM_USE             ((l_u8)1U)      /* NM使用   */

/* Checksum type */
#define  U1G_LIN_SUM_CLASSIC        ((l_u8)0U)      /* Classicチェックサム */
#define  U1G_LIN_SUM_ENHANCED       ((l_u8)1U)      /* Enhancedチェックサム */
#define  U1G_LIN_SUM_TYPE_MAX       ((l_u8)1U)      /* 異常判定用 */

#define  U1G_LIN_NO_FRAME           ((l_u8)0xFFU)   /* 未登録フレーム(ID = 0xFF) */

/* ウェイクアップ検出 */
#define  U1G_LIN_WUP_NOT_DET        ((l_u8)0U)      /* ウェイクアップ未検出 */
#define  U1G_LIN_WUP_DET            ((l_u8)1U)      /* ウェイクアップ検出 */

/* システム異常フラグ */
#define  U1G_LIN_SYSERR_NON         ((l_u8)0U)      /* 初期値 */
#define  U1G_LIN_SYSERR_STAT        ((l_u8)1U)      /* LINステータスが規定値以外 */
#define  U1G_LIN_SYSERR_DRIVER      ((l_u8)3U)      /* MCUステータス異常 */

/* LINレスポンスバッファ */
#define  U1G_LIN_MAX_DL             ((l_u8)8U)      /* 最大データサイズ */

/* エラーフック処理通知用定数 */
#define  U1G_LIN_ID_DUMMY           ((l_u8)0xFFU)   /* ヘッダエラーでのエラーフック発生時用 */

/* タイマ・アレイ・ユニット(TAU) ユニット */
#define  U1G_LIN_TIMER_UNIT_0       (0x00U)   /* TAUユニット 0 を使用 */
#define  U1G_LIN_TIMER_UNIT_1       (0x01U)   /* TAUユニット 1 を使用 */

/* タイマ・アレイ・ユニット(TAU) チャネル */
#define  U1G_LIN_TIMER_CH_0         (0x00U)   /* TAUチャネル 0 を使用 */
#define  U1G_LIN_TIMER_CH_1         (0x01U)   /* TAUチャネル 1 を使用 */
#define  U1G_LIN_TIMER_CH_2         (0x02U)   /* TAUチャネル 2 を使用 */
#define  U1G_LIN_TIMER_CH_3         (0x03U)   /* TAUチャネル 3 を使用 */
#define  U1G_LIN_TIMER_CH_4         (0x04U)   /* TAUチャネル 4 を使用 */
#define  U1G_LIN_TIMER_CH_5         (0x05U)   /* TAUチャネル 5 を使用 */
#define  U1G_LIN_TIMER_CH_6         (0x06U)   /* TAUチャネル 6 を使用 */
#define  U1G_LIN_TIMER_CH_7         (0x07U)   /* TAUチャネル 7 を使用 */

/* タイマ・アレイ・ユニット(TAU) 動作クロック (fMCK) */
#define  U1G_LIN_TIMER_CKMK_M0      (0x00U)   /* TAU 動作クロック(fMCK) CKm0 を使用 */
#define  U1G_LIN_TIMER_CKMK_M1      (0x01U)   /* TAU 動作クロック(fMCK) CKm1 を使用 */
#define  U1G_LIN_TIMER_CKMK_M2      (0x02U)   /* TAU 動作クロック(fMCK) CKm2 を使用 */
#define  U1G_LIN_TIMER_CKMK_M3      (0x03U)   /* TAU 動作クロック(fMCK) CKm3 を使用 */

#define  U2G_LIN_ERRINFO_HDR_SYNC   ((l_u16)0x0002U)   /* ヘッダエラー Synch field error */
#define  U2G_LIN_ERRINFO_HDR_PARI   ((l_u16)0x0004U)   /* ヘッダエラー ID parity error */
#define  U2G_LIN_ERRINFO_HDR_UART   ((l_u16)0x0008U)   /* ヘッダエラー Framing error */
#define  U2G_LIN_ERRINFO_RSP_BIT    ((l_u16)0x0010U)   /* レスポンスエラー Bit error */
#define  U2G_LIN_ERRINFO_RSP_NORES  ((l_u16)0x0020U)   /* レスポンスエラー No response */
#define  U2G_LIN_ERRINFO_RSP_UART   ((l_u16)0x0040U)   /* レスポンスエラー Framing error */
#define  U2G_LIN_ERRINFO_RSP_SUM    ((l_u16)0x0080U)   /* レスポンスエラー Checksum error */
#define  U2G_LIN_ERRINFO_RSP_RESSHT ((l_u16)0x0100U)   /* レスポンスエラー Response too short */
#define  U2G_LIN_ERRINFO_RSP_READY  ((l_u16)0x0200U)   /* レスポンスエラー Response ready error */

/* ID Info DL Setting */
#define  U1G_LIN_DL_1               ((l_u8)0x01U)
#define  U1G_LIN_DL_2               ((l_u8)0x02U)
#define  U1G_LIN_DL_3               ((l_u8)0x03U)
#define  U1G_LIN_DL_4               ((l_u8)0x04U)
#define  U1G_LIN_DL_5               ((l_u8)0x05U)
#define  U1G_LIN_DL_6               ((l_u8)0x06U)
#define  U1G_LIN_DL_7               ((l_u8)0x07U)
#define  U1G_LIN_DL_8               ((l_u8)0x08U)
#define  U1G_LIN_DL_n               ((l_u8)0x0FU)


/* unsigned char型 */
#define  U1G_LIN_BIT_CLR            ((l_u8)0U)
#define  U1G_LIN_BIT_SET            ((l_u8)1U)
#define  U1G_LIN_BYTE_CLR           ((l_u8)0x00U)

#define  U1G_LIN_0                  ((l_u8)0U)
#define  U1G_LIN_1                  ((l_u8)1U)
#define  U1G_LIN_2                  ((l_u8)2U)
#define  U1G_LIN_3                  ((l_u8)3U)
#define  U1G_LIN_4                  ((l_u8)4U)
#define  U1G_LIN_5                  ((l_u8)5U)
#define  U1G_LIN_6                  ((l_u8)6U)
#define  U1G_LIN_7                  ((l_u8)7U)
#define  U1G_LIN_8                  ((l_u8)8U)
#define  U1G_LIN_16                 ((l_u8)16U)
#define  U1G_LIN_32                 ((l_u8)32U)
#define  U1G_LIN_48                 ((l_u8)48U)
#define  U1G_LIN_64                 ((l_u8)64U)

#define  U1G_LIN_BIT0               ((l_u8)0x01U)
#define  U1G_LIN_BIT1               ((l_u8)0x02U)

/* unsigned short型 */
#define  U2G_LIN_BIT_CLR            ((l_u16)0U)
#define  U2G_LIN_BIT_SET            ((l_u16)1U)
#define  U2G_LIN_BYTE_CLR           ((l_u16)0x00U)
#define  U2G_LIN_WORD_CLR           ((l_u16)0x0000U)

#define  U2G_LIN_0                  ((l_u16)0U)

/* NULL */
#define  U1G_LIN_NULL               ((void *)0U)

/*** 関数のプロトタイプ宣言(global) ***/
l_bool l_sys_init(void);

/***** その他の関数 外部参照宣言(global) *****/

/***** APIマクロ宣言 *****/
#define  l_bool_rd(x)       (x)
#define  l_u8_rd(x)         (x)
#define  l_u16_rd(x)        (x)
#define  l_bool_wr(x, v)    ( (x) = (v) )
#define  l_u8_wr(x, v)      ( (x) = (v) )
#define  l_u16_wr(x, v)     ( (x) = (v) )
#define  l_flg_tst(x)       (x)
#define  l_flg_clr(x)       ( (x) = 0U )


/***** 関数プロトタイプ宣言 *****/
void  l_hook_sleep(void);
void  l_hook_wake(void);
void  l_hook_snd_comp(l_frame_handle  u1a_lin_frm);
void  l_hook_rcv_comp(l_frame_handle  u1a_lin_frm);
void  l_hook_err(l_frame_handle  u1a_lin_frm, l_u16 u2a_lin_err);
#endif

/***** End of File *****/


