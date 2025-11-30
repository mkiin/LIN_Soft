/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_core_rl78f24.h                       */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_CORE_RL78F24_H_INCLUDE
#define L_SLIN_CORE_RL78F24_H_INCLUDE

/*** 定数定義 ***/

#define  U1G_LIN_SLEEP_ID               ((l_u8)0x3CU)   /* SLEEPコマンドID = 3Ch  */
#define  U1G_LIN_SLEEP_DATA             ((l_u8)0x00U)   /* SLEEPコマンドのData0 = 0x00 */

#define  U2G_LIN_STSBUF_CLR             ((l_u16)0xFF00U)/* リード用ステータスバッファのクリア値 */

#define  U1G_LIN_ID_PARITY_MASK         ((l_u8)0x3FU)   /* 保護IDパリティビットマスク(3Fh) */

#define  U2G_LIN_BUS_STS_CMP_SET        ((l_u16)0x0003U)/* 転送完了かエラー応答完了がセットされている */

#define  U1G_LIN_BUF_NM_CLR_MASK        ((l_u8)0x2FU)   /* LINフレームバッファのNM部分(Data1 bit4-7)クリアマスク  ※data.ind(b5)は触らない */
#define  U1G_LIN_NM_INFO_MASK           ((l_u8)0xD0U)   /* NM情報テーブルのreserve部 クリア用マスク */

#define  U1G_LIN_ERR_OFF                ((l_u8)0U)      /* 正常に転送完了 */
#define  U1G_LIN_ERR_ON                 ((l_u8)1U)      /* エラーありレスポンス完了 */
#define  U1G_LIN_ERR_NORES              ((l_u8)2U)      /* No response完了 */
#define  U1G_LIN_ERR_READY              ((l_u8)3U)      /* レスポンス準備エラー発生 */

#define  U1G_LIN_HDR_NOT_RCVED          ((l_u8)0U)      /* ヘッダ未受信 */
#define  U1G_LIN_HDR_RCVED              ((l_u8)1U)      /* ヘッダ受信後（有効なID） */

#define  U1G_LIN_BIT_ERR_MASK           ((l_u8)0x01U)   /* Bitエラーマスク */
#define  U1G_LIN_FRA_ERR_MASK           ((l_u8)0x08U)   /* Framingエラーマスク */
#define  U1G_LIN_SF_ERR_MASK            ((l_u8)0x10U)   /* SyncFieldエラーマスク */
#define  U1G_LIN_CKSUM_ERR_MASK         ((l_u8)0x20U)   /* Checksumエラーマスク */
#define  U1G_LIN_ID_ERR_MASK            ((l_u8)0x40U)   /* ID parityエラーマスク */
#define  U1G_LIN_RESP_ERR_MASK          ((l_u8)0x80U)   /* レスポンス準備エラーマスク */

#define  U1G_LIN_HDR_RCVED_MASK         ((l_u8)0x80U)   /* ヘッダ受信完了値マスク */
#define  U1G_LIN_RESP_RCVED_MASK        ((l_u8)0x02U)   /* レスポンス受信完了値マスク */

#define  U1G_LIN_BUS_STS_OK_RESP        ((l_u8)0x01U)   /* 正常終了データのビットシフト数 */
#define  U1G_LIN_BUS_STS_OVR_RUN        ((l_u8)0x02U)   /* オーバーランのビットシフト数 */
#define  U1G_LIN_BUS_STS_GOTO_SLEEP     ((l_u8)0x03U)   /* GoTo Sleepのビットシフト数 */
#define  U1G_LIN_BUS_STS_BUS_ERR        ((l_u8)0x06U)   /* バスエラーのビットシフト数 */
#define  U1G_LIN_BUS_STS_HEAD_ERR       ((l_u8)0x07U)   /* ヘッダエラーのビットシフト数 */
#define  U1G_LIN_BUS_STS_LAST_ID        ((l_u8)0x08U)   /* LastIDのビットシフト数 */

#define  U1G_LIN_LST_D1RC_INCOMPLETE    ((l_u8)0x00U)   /* データ1受信 未完了 */
#define  U1G_LIN_LST_D1RC_COMPLETE      ((l_u8)0x01U)   /* データ1受信 完了 */

/*** テーブル構造定義 ***/
/* 現在処理しているフレームのIDとスロット番号を管理 */
typedef struct {
    l_u8  u1g_lin_id;                       /* ID (00h-3Fh)            */
    l_u8  u1g_lin_slot;                     /* SLOT No. (00h-3Fh、FFh) */
} st_lin_id_slot_type;


/*** 関数のプロトタイプ宣言(global) ***/
void  l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_id);
void  l_vog_lin_tx_int(void);
void  l_vog_lin_err_int(l_u8 u1a_lin_err,l_u8 u1a_lin_lst_data);
void  l_vog_lin_aux_int(void);
l_u8  l_u1g_lin_get_wupflg(void);
void  l_vog_lin_clr_wupflg(void);
void  l_vog_lin_set_int_enb(void);
void  l_vog_lin_set_nm_info( l_u8  u1a_lin_nm_info );
void  l_vog_lin_determine_FramingError(void);
void  l_vog_lin_ResTooShort_error(void);
void  l_vog_lin_NoRes_error(void);

#endif

/***** End of File *****/



