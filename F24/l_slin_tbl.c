/*""FILE COMMENT""********************************************/
/* System Name : S930-LSLibRL78F24xx                         */
/* File Name   : l_slin_tbl.c                                */
/* Note        :                                             */
/*""FILE COMMENT END""****************************************/
/* LINライブラリ ROMセクション定義 */
#pragma section  text   @@LNCD
#pragma section  text  @@LNCDL
#pragma section  const   @@LNCNS
#pragma section  const  @@LNCNSL
/* LINライブラリ RAMセクション定義 */
#pragma section  bss   @@LNDT
#pragma section  bss  @@LNDTL

#include "l_slin_user.h"

/********************************/
/* スレーブタスク用テーブル設定 */
/********************************/
/* ID情報テーブル 実体 (LDF: Frames) */

const l_u8  u1g_lin_id_tbl[ U1G_LIN_MAX_SLOT_NUM ] = {
    (l_u8)0xFFU, (l_u8)0x01U, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0x00U, (l_u8)0xFFU  /* Identifier : 0x00 - 0x07 */
   ,(l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU  /* Identifier : 0x08 - 0x0F */
   ,(l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0x02U  /* Identifier : 0x10 - 0x17 */
   ,(l_u8)0xFFU, (l_u8)0x04U, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU  /* Identifier : 0x18 - 0x1F */
   ,(l_u8)0xFFU, (l_u8)0x03U, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU  /* Identifier : 0x20 - 0x27 */
   ,(l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU  /* Identifier : 0x28 - 0x2F */
   ,(l_u8)0xFFU, (l_u8)0x05U, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU  /* Identifier : 0x30 - 0x37 */
   ,(l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0xFFU, (l_u8)0x06U, (l_u8)0x07U, (l_u8)0xFFU, (l_u8)0xFFU  /* Identifier : 0x38 - 0x3F */
};

/* スロット情報テーブル 実体 */
const st_lin_slot_info_type  xng_lin_slot_tbl[ U1G_LIN_MAX_SLOT ] = {
    { (l_u8)0x06U, U1G_LIN_DL_2, U1G_LIN_CMD_SND, U1G_LIN_NM_USE, U1G_LIN_SUM_ENHANCED
       ,{ (l_u8)0x8AU, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x01U, U1G_LIN_DL_2, U1G_LIN_CMD_RCV, U1G_LIN_NM_NO_USE, U1G_LIN_SUM_ENHANCED
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x17U, U1G_LIN_DL_4, U1G_LIN_CMD_SND, U1G_LIN_NM_USE, U1G_LIN_SUM_ENHANCED
       ,{ (l_u8)0x8AU, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x21U, U1G_LIN_DL_4, U1G_LIN_CMD_RCV, U1G_LIN_NM_NO_USE, U1G_LIN_SUM_ENHANCED
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x19U, U1G_LIN_DL_8, U1G_LIN_CMD_SND, U1G_LIN_NM_USE, U1G_LIN_SUM_ENHANCED
       ,{ (l_u8)0x8AU, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x31U, U1G_LIN_DL_8, U1G_LIN_CMD_RCV, U1G_LIN_NM_NO_USE, U1G_LIN_SUM_ENHANCED
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x3CU, U1G_LIN_DL_8, U1G_LIN_CMD_RCV, U1G_LIN_NM_NO_USE, U1G_LIN_SUM_CLASSIC
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
   ,{ (l_u8)0x3DU, U1G_LIN_DL_8, U1G_LIN_CMD_SND, U1G_LIN_NM_NO_USE, U1G_LIN_SUM_CLASSIC
       ,{ (l_u8)0xAAU, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
       ,{ (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U, (l_u8)0x00U }
    }
};


/***************************************************************************/
/***************************************************************************/
/*           !!!以降は絶対に 変更、削除をおこなわないで下さい!!!           */
/***************************************************************************/
/***************************************************************************/

/* 保護IDテーブル 実体 */
const l_u8  u1g_lin_protid_tbl[ U1G_LIN_MAX_SLOT_NUM ] = {
     (l_u8)0x80U, (l_u8)0xC1U, (l_u8)0x42U, (l_u8)0x03U, (l_u8)0xC4U, (l_u8)0x85U, (l_u8)0x06U, (l_u8)0x47U  /* 0x00 - 0x07 */
    ,(l_u8)0x08U, (l_u8)0x49U, (l_u8)0xCAU, (l_u8)0x8BU, (l_u8)0x4CU, (l_u8)0x0DU, (l_u8)0x8EU, (l_u8)0xCFU  /* 0x08 - 0x0F */
    ,(l_u8)0x50U, (l_u8)0x11U, (l_u8)0x92U, (l_u8)0xD3U, (l_u8)0x14U, (l_u8)0x55U, (l_u8)0xD6U, (l_u8)0x97U  /* 0x10 - 0x17 */
    ,(l_u8)0xD8U, (l_u8)0x99U, (l_u8)0x1AU, (l_u8)0x5BU, (l_u8)0x9CU, (l_u8)0xDDU, (l_u8)0x5EU, (l_u8)0x1FU  /* 0x18 - 0x1F */
    ,(l_u8)0x20U, (l_u8)0x61U, (l_u8)0xE2U, (l_u8)0xA3U, (l_u8)0x64U, (l_u8)0x25U, (l_u8)0xA6U, (l_u8)0xE7U  /* 0x20 - 0x27 */
    ,(l_u8)0xA8U, (l_u8)0xE9U, (l_u8)0x6AU, (l_u8)0x2BU, (l_u8)0xECU, (l_u8)0xADU, (l_u8)0x2EU, (l_u8)0x6FU  /* 0x28 - 0x2F */
    ,(l_u8)0xF0U, (l_u8)0xB1U, (l_u8)0x32U, (l_u8)0x73U, (l_u8)0xB4U, (l_u8)0xF5U, (l_u8)0x76U, (l_u8)0x37U  /* 0x30 - 0x37 */
    ,(l_u8)0x78U, (l_u8)0x39U, (l_u8)0xBAU, (l_u8)0xFBU, (l_u8)0x3CU, (l_u8)0x7DU, (l_u8)0xFEU, (l_u8)0xBFU  /* 0x38 - 0x3F */
};


/* フラグセットテーブル(16bit) 実体 */
const l_u16  u2g_lin_flg_set_tbl[ U1G_LIN_WORD_BIT ] = {
     (l_u16)0x0001U, (l_u16)0x0002U, (l_u16)0x0004U, (l_u16)0x0008U
    ,(l_u16)0x0010U, (l_u16)0x0020U, (l_u16)0x0040U, (l_u16)0x0080U
    ,(l_u16)0x0100U, (l_u16)0x0200U, (l_u16)0x0400U, (l_u16)0x0800U
    ,(l_u16)0x1000U, (l_u16)0x2000U, (l_u16)0x4000U, (l_u16)0x8000U
};


/* LIN STATUS BUF 実体 */
st_lin_state_type  xng_lin_sts_buf;

/* LIN FRAME BUF 実体 */
st_lin_buf_type  xng_lin_frm_buf[ U1G_LIN_MAX_SLOT ];

/* システム異常フラグ */
l_u8  u1g_lin_syserr;

/***** End of File *****/





