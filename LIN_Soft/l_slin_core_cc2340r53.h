/**
 * @file        l_slin_core_CC2340R53.h
 *
 * @brief       SLIN CORE層
 *
 * @attention   編集禁止
 *
 */

#ifndef __L_SLIN_CORE_CC2340R53_H_INCLUDE__
#define __L_SLIN_CORE_CC2340R53_H_INCLUDE__


/*** 定数定義 ***/
#define U1L_LIN_UART_HEADER_RXSIZE          ( 3U )          /* ヘッダー受信サイズ   型: l_u32 */
#define U1L_LIN_UART_RXSIZE                 ( 9U )          /* 受信サイズ   型: l_u32 */
#define U1L_LIN_UART_TXSIZE                 ( 9U )          /* 送信サイズ   型: l_u32 */

/* LINヘッダーの各フィールド（位置） */
#define U1G_LIN_HEADER_SYNCHBREAK           ( 0U )
#define U1G_LIN_HEADER_SYNCHFIELD           ( 1U )
#define U1G_LIN_HEADER_PID                  ( 2U )
#define U1G_LIN_HEADER_PARITY               ( 2U )

#define  U1G_LIN_SYNCH_BREAK_DATA           ((l_u8)0x00)    /* SYNCH BREAKのデータ = 00h */
#define  U1G_LIN_SYNCH_FIELD_DATA           ((l_u8)0x55)    /* SYNCH FIELDのデータ = 55h */

#if U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_2400
 #define  U1G_LIN_SND_WAKEUP_DATA           ((l_u8)0xFF)    /* WAKEUPデータ = FFh */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_9600
/* Ver 2.00 変更:ウェイクアップ信号の標準値を"0x80"に変更 */
#if 0  /********** ↓↓↓ 削除コード ↓↓↓ **********/
 #define  U1G_LIN_SND_WAKEUP_DATA            ((l_u8)0xF8)    /* WAKEUPデータ = F8h */
#endif /********** ↑↑↑ 削除コード ↑↑↑ **********/
 #define  U1G_LIN_SND_WAKEUP_DATA           ((l_u8)0x80)    /* WAKEUPデータ = 80h */
/* Ver 2.00 変更:ウェイクアップ信号の標準値を"0x80"に変更 */
#elif U1G_LIN_BAUDRATE == U1G_LIN_BAUDRATE_19200
 #define  U1G_LIN_SND_WAKEUP_DATA           ((l_u8)0x80)    /* WAKEUPデータ = 80h */
#endif

#define  U1G_LIN_SLEEP_ID                   ((l_u8)0x3C)    /* SLEEPコマンドID = 3Ch  */
#define  U1G_LIN_SLEEP_DATA                 ((l_u8)0x00)    /* SLEEPコマンドのData0 = 0x00 */

#define  U1G_LIN_HEADER_MAX_TIME            ((l_u8)49)      /* ヘッダータイムアウトMAX値 */
#define  U1G_LIN_BYTE_LENGTH                ((l_u8)10)      /* 1バイト長(10bit) */

#define  U2G_LIN_STSBUF_CLR                 ((l_u16)0xFF00) /* リード用ステータスバッファのクリア値 */
#define  U1G_LIN_UART_ERR_ON                ((l_u8)0x0F)    /* 受信バッファレジスタの上位(エラー)ビットON */

#define  U1G_LIN_FRAMING_ERR_ON             ((l_u8)0x02)    /* 受信バッファレジスタのフレーミングエラービットON */
#define  U1G_LIN_FRAMING_ERR                ((l_u8)0x0A)    /* フレーミングエラーチェック */
#define  U1G_LIN_OVERRUN_ERR                ((l_u8)0x09)    /* オーバーランエラーチェック */
#define  U1G_LIN_ID_PARITY_MASK             ((l_u8)0x3F)    /* 保護IDパリティビットマスク(3Fh) */

#define  U2G_LIN_BUS_STS_CMP_SET            ((l_u16)0x0003) /* 転送完了かエラー応答完了がセットされている */

#define  U1G_LIN_BUF_NM_CLR_MASK            ((l_u8)0x0F)    /* LINフレームバッファのNM部分(Data1 bit4-7)クリアマスク */
#define  U1G_LIN_NM_INFO_MASK               ((l_u8)0xF0)    /* NM情報テーブルのreserve部 クリア用マスク */

#define  U1G_LIN_SLSTS_BREAK_UART_WAIT      ((l_u8)0)       /* スレーブタスクのステータス = Synch Break(UART)待ち */
#define  U1G_LIN_SLSTS_BREAK_IRQ_WAIT       ((l_u8)1)       /* スレーブタスクのステータス = Synch Break(IRQ)待ち */
#define  U1G_LIN_SLSTS_SYNCHFIELD_WAIT      ((l_u8)2)       /* スレーブタスクのステータス = Synch Field待ち */
#define  U1G_LIN_SLSTS_IDENTFIELD_WAIT      ((l_u8)3)       /* スレーブタスクのステータス = Ident Field待ち */
#define  U1G_LIN_SLSTS_RCVDATA_WAIT         ((l_u8)4)       /* スレーブタスクのステータス = データ受信待ち */
#define  U1G_LIN_SLSTS_BEFORE_SNDDATA_WAIT  ((l_u8)5)       /* スレーブタスクのステータス = データ送信待ち */
#define  U1G_LIN_SLSTS_AFTER_SNDDATA_WAIT   ((l_u8)6)       /* スレーブタスクのステータス = データ送信後 */

#define  U1G_LIN_ERR_OFF                    ((l_u8)0)       /* 正常に転送完了 */
#define  U1G_LIN_ERR_ON                     ((l_u8)1)       /* エラーありレスポンス完了 */

#define  U2G_LIN_HERR_LIMIT                 ((l_u16)510)       /* 25000bitタイム分のヘッダタイム回数(25000 / 49) */

/*** テーブル構造定義 ***/
/* 現在処理しているフレームのIDとスロット番号を管理 */
typedef struct {
    l_u8  u1g_lin_id;                       /* ID (00h-3Fh) */
    l_u8  u1g_lin_slot;                     /* SLOT No. (1-64) */
} st_lin_id_slot_type;


/*** 関数のプロトタイプ宣言(extern) ***/
extern void   l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size);
extern l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size);
/* Ver 1.01 変更:トヨタコーディングルール対応 <- end */
extern void   l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size);
extern void   l_vog_lin_rx_dis(void);
extern void   l_vog_lin_uart_init(void);
extern void   l_vog_lin_int_init(void);
extern void   l_vog_lin_int_enb(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
extern void  l_vog_lin_int_enb_wakeup(void);
/* Ver 2.00 追加:ウェイクアップ信号検出エッジの極性切り替えへの対応 */
extern void   l_vog_lin_int_dis(void);

#endif

/***** End of File *****/
