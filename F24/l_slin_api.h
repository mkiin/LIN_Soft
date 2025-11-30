/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_api.h                                */
/* Note        : 一部、ユーザ編集可                          */
/*""FILE COMMENT END""****************************************/

#ifndef L_SLIN_API_H_INCLUDE
#define L_SLIN_API_H_INCLUDE


/* ↓↓↓ ユーザ編集許可ここから ↓↓↓ */
/* フレーム名の定義 */
#define  U1G_LIN_FRAME_ID06H            ((l_u8)0x00)
#define  U1G_LIN_FRAME_ID01H            ((l_u8)0x01)
#define  U1G_LIN_FRAME_ID17H            ((l_u8)0x02)
#define  U1G_LIN_FRAME_ID21H            ((l_u8)0x03)
#define  U1G_LIN_FRAME_ID19H            ((l_u8)0x04)
#define  U1G_LIN_FRAME_ID31H            ((l_u8)0x05)
#define  U1G_LIN_FRAME_ID3CH            ((l_u8)0x06)
#define  U1G_LIN_FRAME_ID3DH            ((l_u8)0x07)

/* シグナルの名前(Table Type) */
/* ビットシンボルテーブル構造(LDF: Frames, Signals) */
typedef union {
    l_u8    u1g_lin_byte[U1G_LIN_MAX_DL];    /******    !!!この行は変更削除を行わないでください!!!    ******/
} un_lin_data_type;

/* 3DHフレームの設定 */
#define  U1G_LIN_FRAME_3DH_BRG          xng_lin_frm_buf[U1G_LIN_FRAME_ID3DH].xng_lin_data.u1g_lin_byte[2U]

/* 同期フラグ : フレーム送信完了フラグ */
#define  F1g_lin_txframe_id06h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot2

/* 同期フラグ : フレーム受信完了フラグ */
#define  F1g_lin_rxframe_id01h          xng_lin_sts_buf.un_rs_flg1.st_bit.u2g_lin_slot1

/* 同期フラグ : フレームエラーの発生状況フラグ */
#define  F4g_lin_errframe_e_nores_id01h        xng_lin_frm_buf[ U1G_LIN_FRAME_ID01H ].u1g_lin_e_nores
#define  F4g_lin_errframe_e_uart_id01h         xng_lin_frm_buf[ U1G_LIN_FRAME_ID01H ].u1g_lin_e_uart
#define  F4g_lin_errframe_e_bit_id01h          xng_lin_frm_buf[ U1G_LIN_FRAME_ID01H ].u1g_lin_e_bit
#define  F4g_lin_errframe_e_sum_id01h          xng_lin_frm_buf[ U1G_LIN_FRAME_ID01H ].u1g_lin_e_sum
#define  F4g_lin_errframe_e_res_sht_id01h      xng_lin_frm_buf[ U1G_LIN_FRAME_ID01H ].u1g_lin_e_res_sht

#define  F4g_lin_errframe_e_nores_id06h        xng_lin_frm_buf[ U1G_LIN_FRAME_ID06H ].u1g_lin_e_nores
#define  F4g_lin_errframe_e_uart_id06h         xng_lin_frm_buf[ U1G_LIN_FRAME_ID06H ].u1g_lin_e_uart
#define  F4g_lin_errframe_e_bit_id06h          xng_lin_frm_buf[ U1G_LIN_FRAME_ID06H ].u1g_lin_e_bit
#define  F4g_lin_errframe_e_sum_id06h          xng_lin_frm_buf[ U1G_LIN_FRAME_ID06H ].u1g_lin_e_sum
#define  F4g_lin_errframe_e_res_sht_id06h      xng_lin_frm_buf[ U1G_LIN_FRAME_ID06H ].u1g_lin_e_res_sht

/* 同期フラグ : フレームのbusy状態フラグ */
#define  F1g_lin_busyflag_id3Ch         xng_lin_frm_buf[U1G_LIN_FRAME_ID3CH].un_state.st_bit.u2g_lin_status

/* ↑↑↑ ユーザ編集許可ここまで ↑↑↑ */

/***************************************************************************/
/*              !!!以降は 変更、削除をおこなわないで下さい!!!              */
/***************************************************************************/
/***************************************************************************/

/* 同期フラグ: Bus inactive発生状況フラグ */
#define  F1g_lin_bus_inactive           xng_lin_sts_buf.u1g_lin_bus_inactive

/* 同期フラグ：ヘッダエラーの発生状況フラグ（エラー個別） */
#define  F1g_lin_header_err_uart        xng_lin_sts_buf.u1g_lin_e_uart
#define  F1g_lin_header_err_sync        xng_lin_sts_buf.u1g_lin_e_sync
#define  F1g_lin_header_err_parity      xng_lin_sts_buf.u1g_lin_e_pari

/********************************/
/* LIN API EXTERN               */
/********************************/
/***** API関数 外部参照宣言 *****/
void     l_ifc_init(void);
void     l_ifc_init_com(void);
void     l_ifc_init_drv(void);
void     l_ifc_bus_err_tick(void);
void     l_ifc_connect(void);
l_bool   l_ifc_disconnect(void);
void     l_ifc_wake_up(void);
#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
l_u16    l_ifc_read_status(void);
#endif
void     l_ifc_sleep(void);

void     l_ifc_run(void);

void     l_ifc_tx(void);
void     l_ifc_rx(void);
void     l_ifc_err(void);
void     l_ifc_aux(void);
void     l_ifc_tau_timeout(void);

l_u16    l_ifc_read_lb_status(void);
#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
void     l_slot_enable(l_frame_handle  u1a_lin_frm);
void     l_slot_disable(l_frame_handle  u1a_lin_frm);
#endif
void     l_slot_rd(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, l_u8 pta_lin_data[]);
void     l_slot_wr(l_frame_handle u1a_lin_frm, l_u8 u1a_lin_cnt, const l_u8 pta_lin_data[]);
#if U1G_LIN_ROM_SHRINK == U1G_LIN_ROM_SHRINK_OFF
void     l_slot_set_default(l_frame_handle u1a_lin_frm);
void     l_slot_set_fail(l_frame_handle  u1a_lin_frm);
#endif

#endif

/***** End of File *****/


