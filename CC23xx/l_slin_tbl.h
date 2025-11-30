/**
 * @file        l_slin_tbl.h
 *
 * @brief       SLIN テーブル定義
 *
 * @attention   編集禁止
 *
 */

#ifndef __L_SLIN_TBL_H_INCLUDE__
#define __L_SLIN_TBL_H_INCLUDE__


/*** 構造体、共用体定義 ***/

/*=============== エンディアンタイプによるコンパイルスイッチの開始 ===============*/

/*=============== リトルエンディアンタイプ ===============*/
#if  U1G_LIN_ENDIAN_TYPE == U1G_LIN_ENDIAN_LITTLE

/* LIN ステータスバッファの定義 */
typedef struct {
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_nad:8;
            l_u16   u2g_lin_sts:2;
            l_u16   reserve:1;
            l_u16   u2g_lin_phy_bus_err:1;
            l_u16   u2g_lin_e_time:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_synch:1;
            l_u16   u2g_lin_e_pari:1;
        } st_bit;
        struct {
            l_u16   reserve:12;
            l_u16   u2g_lin_e_head:4;
        } st_err;
    } un_state;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_node1:1;
            l_u16   u2g_lin_node2:1;
            l_u16   u2g_lin_node3:1;
            l_u16   u2g_lin_node4:1;
            l_u16   u2g_lin_node5:1;
            l_u16   u2g_lin_node6:1;
            l_u16   u2g_lin_node7:1;
            l_u16   u2g_lin_node8:1;
            l_u16   u2g_lin_node9:1;
            l_u16   u2g_lin_node10:1;
            l_u16   u2g_lin_node11:1;
            l_u16   u2g_lin_node12:1;
            l_u16   u2g_lin_node13:1;
            l_u16   u2g_lin_node14:1;
            l_u16   u2g_lin_node15:1;
            l_u16   u2g_lin_node16:1;
        } st_bit;
    } un_node_flg;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot1:1;
            l_u16   u2g_lin_slot2:1;
            l_u16   u2g_lin_slot3:1;
            l_u16   u2g_lin_slot4:1;
            l_u16   u2g_lin_slot5:1;
            l_u16   u2g_lin_slot6:1;
            l_u16   u2g_lin_slot7:1;
            l_u16   u2g_lin_slot8:1;
            l_u16   u2g_lin_slot9:1;
            l_u16   u2g_lin_slot10:1;
            l_u16   u2g_lin_slot11:1;
            l_u16   u2g_lin_slot12:1;
            l_u16   u2g_lin_slot13:1;
            l_u16   u2g_lin_slot14:1;
            l_u16   u2g_lin_slot15:1;
            l_u16   u2g_lin_slot16:1;
        } st_bit;
    } un_rs_flg1;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot17:1;
            l_u16   u2g_lin_slot18:1;
            l_u16   u2g_lin_slot19:1;
            l_u16   u2g_lin_slot20:1;
            l_u16   u2g_lin_slot21:1;
            l_u16   u2g_lin_slot22:1;
            l_u16   u2g_lin_slot23:1;
            l_u16   u2g_lin_slot24:1;
            l_u16   u2g_lin_slot25:1;
            l_u16   u2g_lin_slot26:1;
            l_u16   u2g_lin_slot27:1;
            l_u16   u2g_lin_slot28:1;
            l_u16   u2g_lin_slot29:1;
            l_u16   u2g_lin_slot30:1;
            l_u16   u2g_lin_slot31:1;
            l_u16   u2g_lin_slot32:1;
        } st_bit;
    } un_rs_flg2;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot33:1;
            l_u16   u2g_lin_slot34:1;
            l_u16   u2g_lin_slot35:1;
            l_u16   u2g_lin_slot36:1;
            l_u16   u2g_lin_slot37:1;
            l_u16   u2g_lin_slot38:1;
            l_u16   u2g_lin_slot39:1;
            l_u16   u2g_lin_slot40:1;
            l_u16   u2g_lin_slot41:1;
            l_u16   u2g_lin_slot42:1;
            l_u16   u2g_lin_slot43:1;
            l_u16   u2g_lin_slot44:1;
            l_u16   u2g_lin_slot45:1;
            l_u16   u2g_lin_slot46:1;
            l_u16   u2g_lin_slot47:1;
            l_u16   u2g_lin_slot48:1;
        } st_bit;
    } un_rs_flg3;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot49:1;
            l_u16   u2g_lin_slot50:1;
            l_u16   u2g_lin_slot51:1;
            l_u16   u2g_lin_slot52:1;
            l_u16   u2g_lin_slot53:1;
            l_u16   u2g_lin_slot54:1;
            l_u16   u2g_lin_slot55:1;
            l_u16   u2g_lin_slot56:1;
            l_u16   u2g_lin_slot57:1;
            l_u16   u2g_lin_slot58:1;
            l_u16   u2g_lin_slot59:1;
            l_u16   u2g_lin_slot60:1;
            l_u16   u2g_lin_slot61:1;
            l_u16   u2g_lin_slot62:1;
            l_u16   u2g_lin_slot63:1;
            l_u16   u2g_lin_slot64:1;
        } st_bit;
    } un_rs_flg4;
} st_lin_state_type;


/* LINフレームバッファの定義 */
typedef struct {
    un_lin_data_type  xng_lin_data;
    union {
       l_u16    u2g_lin_word;
        struct {
            l_u16   reserve1:8;
            l_u16   u2g_lin_err:4;
            l_u16   reserve2:4;
        } st_err;
        struct {
            l_u16   u2g_lin_chksum:8;
            l_u16   u2g_lin_e_nores:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_bit:1;
            l_u16   u2g_lin_e_sum:1;
            l_u16   reserve:3;
            l_u16   u2g_lin_no_use:1;
        } st_bit;
    } un_state;
} st_lin_buf_type;


/* LIN Busステータス */
typedef union {
    l_u16  u2g_lin_word;
    struct {
        l_u16  u2g_lin_err_resp:1;
        l_u16  u2g_lin_ok_resp:1;
        l_u16  u2g_lin_ovr_run:1;
        l_u16  u2g_lin_goto_sleep:1;
        l_u16  reserve:2;
        l_u16  u2g_lin_bus_err:1;
        l_u16  u2g_lin_head_err:1;
        l_u16  u2g_lin_last_id:8;
    } st_bit;
} un_lin_bus_status_type;


/*=============== ビッグエンディアンタイプ ===============*/
#elif  U1G_LIN_ENDIAN_TYPE == U1G_LIN_ENDIAN_BIG
/* LIN ステータスバッファの定義 */
typedef struct {
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_e_pari:1;
            l_u16   u2g_lin_e_synch:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_time:1;
            l_u16   u2g_lin_phy_bus_err:1;
            l_u16   reserve:1;
            l_u16   u2g_lin_sts:2;
            l_u16   u2g_lin_nad:8;
        } st_bit;
        struct {
            l_u16   u2g_lin_e_head:4;
            l_u16   reserve:12;
        } st_err;
    } un_state;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_node16:1;
            l_u16   u2g_lin_node15:1;
            l_u16   u2g_lin_node14:1;
            l_u16   u2g_lin_node13:1;
            l_u16   u2g_lin_node12:1;
            l_u16   u2g_lin_node11:1;
            l_u16   u2g_lin_node10:1;
            l_u16   u2g_lin_node9:1;
            l_u16   u2g_lin_node8:1;
            l_u16   u2g_lin_node7:1;
            l_u16   u2g_lin_node6:1;
            l_u16   u2g_lin_node5:1;
            l_u16   u2g_lin_node4:1;
            l_u16   u2g_lin_node3:1;
            l_u16   u2g_lin_node2:1;
            l_u16   u2g_lin_node1:1;
        } st_bit;
    } un_node_flg;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot16:1;
            l_u16   u2g_lin_slot15:1;
            l_u16   u2g_lin_slot14:1;
            l_u16   u2g_lin_slot13:1;
            l_u16   u2g_lin_slot12:1;
            l_u16   u2g_lin_slot11:1;
            l_u16   u2g_lin_slot10:1;
            l_u16   u2g_lin_slot9:1;
            l_u16   u2g_lin_slot8:1;
            l_u16   u2g_lin_slot7:1;
            l_u16   u2g_lin_slot6:1;
            l_u16   u2g_lin_slot5:1;
            l_u16   u2g_lin_slot4:1;
            l_u16   u2g_lin_slot3:1;
            l_u16   u2g_lin_slot2:1;
            l_u16   u2g_lin_slot1:1;
        } st_bit;
    } un_rs_flg1;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot32:1;
            l_u16   u2g_lin_slot31:1;
            l_u16   u2g_lin_slot30:1;
            l_u16   u2g_lin_slot29:1;
            l_u16   u2g_lin_slot28:1;
            l_u16   u2g_lin_slot27:1;
            l_u16   u2g_lin_slot26:1;
            l_u16   u2g_lin_slot25:1;
            l_u16   u2g_lin_slot24:1;
            l_u16   u2g_lin_slot23:1;
            l_u16   u2g_lin_slot22:1;
            l_u16   u2g_lin_slot21:1;
            l_u16   u2g_lin_slot20:1;
            l_u16   u2g_lin_slot19:1;
            l_u16   u2g_lin_slot18:1;
            l_u16   u2g_lin_slot17:1;
        } st_bit;
    } un_rs_flg2;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot48:1;
            l_u16   u2g_lin_slot47:1;
            l_u16   u2g_lin_slot46:1;
            l_u16   u2g_lin_slot45:1;
            l_u16   u2g_lin_slot44:1;
            l_u16   u2g_lin_slot43:1;
            l_u16   u2g_lin_slot42:1;
            l_u16   u2g_lin_slot41:1;
            l_u16   u2g_lin_slot40:1;
            l_u16   u2g_lin_slot39:1;
            l_u16   u2g_lin_slot38:1;
            l_u16   u2g_lin_slot37:1;
            l_u16   u2g_lin_slot36:1;
            l_u16   u2g_lin_slot35:1;
            l_u16   u2g_lin_slot34:1;
            l_u16   u2g_lin_slot33:1;
        } st_bit;
    } un_rs_flg3;
    union {
        l_u16   u2g_lin_word;
        struct {
            l_u16   u2g_lin_slot64:1;
            l_u16   u2g_lin_slot63:1;
            l_u16   u2g_lin_slot62:1;
            l_u16   u2g_lin_slot61:1;
            l_u16   u2g_lin_slot60:1;
            l_u16   u2g_lin_slot59:1;
            l_u16   u2g_lin_slot58:1;
            l_u16   u2g_lin_slot57:1;
            l_u16   u2g_lin_slot56:1;
            l_u16   u2g_lin_slot55:1;
            l_u16   u2g_lin_slot54:1;
            l_u16   u2g_lin_slot53:1;
            l_u16   u2g_lin_slot52:1;
            l_u16   u2g_lin_slot51:1;
            l_u16   u2g_lin_slot50:1;
            l_u16   u2g_lin_slot49:1;
        } st_bit;
    } un_rs_flg4;
} st_lin_state_type;


/* LINフレームバッファの定義 */
typedef struct {
    un_lin_data_type  xng_lin_data;
    union {
       l_u16    u2g_lin_word;
        struct {
            l_u16   reserve2:4;
            l_u16   u2g_lin_err:4;
            l_u16   reserve1:8;
        } st_err;
        struct {
            l_u16   u2g_lin_no_use:1;
            l_u16   reserve:3;
            l_u16   u2g_lin_e_sum:1;
            l_u16   u2g_lin_e_bit:1;
            l_u16   u2g_lin_e_uart:1;
            l_u16   u2g_lin_e_nores:1;
            l_u16   u2g_lin_chksum:8;
        } st_bit;
    } un_state;
} st_lin_buf_type;


/* LIN Busステータス */
typedef union {
    l_u16  u2g_lin_word;
    struct {
        l_u16  u2g_lin_last_id:8;
        l_u16  u2g_lin_head_err:1;
        l_u16  u2g_lin_bus_err:1;
        l_u16  reserve:2;
        l_u16  u2g_lin_goto_sleep:1;
        l_u16  u2g_lin_ovr_run:1;
        l_u16  u2g_lin_ok_resp:1;
        l_u16  u2g_lin_err_resp:1;
    } st_bit;
} un_lin_bus_status_type;

#endif
/*=============== エンディアンタイプによるコンパイルスイッチの終了 ===============*/

/* LIN スロット情報テーブルの定義 */
typedef struct {
    l_u8    u1g_lin_id;                 /* ID(00h～3Fh) */
    l_u8    u1g_lin_frm_sz;             /* DL(1～8) */
    l_u8    u1g_lin_sndrcv;             /* 0b:Snd, 1b:Rcv */
    l_u8    u1g_lin_nm_use;             /* NM有効 (0:no use 1:use) */
    l_u8    u1g_lin_def[U1G_LIN_MAX_DL];     /* デフォルト値 */
    l_u8    u1g_lin_fail[U1G_LIN_MAX_DL];    /* フェールセーフ値 */
} st_lin_slot_info_type;


/***** 外部参照定義 *****/
extern st_lin_state_type  xng_lin_sts_buf;
extern st_lin_buf_type  xng_lin_frm_buf[U1G_LIN_MAX_SLOT];
extern un_lin_bus_status_type  xng_lin_bus_sts;
extern l_u8  u1g_lin_nm_info;
extern l_u8  u1g_lin_syserr;                        /* システム異常フラグ */

extern const l_u8   u1g_lin_id_tbl[ U1G_LIN_MAX_SLOT_NUM ];
extern const l_u8   u1g_lin_protid_tbl[ U1G_LIN_MAX_SLOT_NUM ];
extern const l_u16  u2g_lin_flg_set_tbl[ U1G_LIN_WORD_BIT ];
extern const l_u16  u2g_lin_flg_clr_tbl[ U1G_LIN_WORD_BIT ];
extern const st_lin_slot_info_type  xng_lin_slot_tbl[ U1G_LIN_MAX_SLOT ];

#endif

/***** End of File *****/