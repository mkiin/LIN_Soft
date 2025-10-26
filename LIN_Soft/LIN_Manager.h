/****************************************************************/
/**
 *  @file       LIN_Manager.h
 *  @brief      @b LINé€šä¿¡ç®¡ç† @n
 *  @details
 *      #### ãƒ¦ãƒ‹ãƒƒãƒˆID
 *       ID;; @n
 *      @if false
 *           ID;;
 *          REF;;
 *          @copyright  AISIN CORPORATION
 *      @endif
 *      #### ãƒ¦ãƒ‹ãƒƒãƒˆä»•æ§˜
 *          REF;;
 */
/****************************************************************/
#ifndef LIN_MANAGER_H
#define LIN_MANAGER_H

/*==============================================================*/
/*  Includes                                                    */
/*==============================================================*/
#include "l_std_def_d.h"


/*==============================================================*/
/*  Types And Constants                                         */
/*==============================================================*/
#define U2G_LIN_MANAGER_MAX_PAYLOAD_SIZE        ((u2)307U)  /*!< @brief æœ€å¤§ãƒšã‚¤ãƒ­ãƒ¼ãƒ‰ã‚µã‚¤ã‚º */


/*!
 * @name LINãƒ•ãƒ¬ãƒ¼ãƒ é–¢é€£å®šæ•°
 * @brief LINãƒ•ãƒ¬ãƒ¼ãƒ ã«é–¢é€£ã™ã‚‹å®šæ•°
 */
/*! @{ */
#define U1G_LINMODULE_HEADER_SIZE                ((uint8_t)3u)    /*!< @brief ãƒ˜ãƒƒãƒ€é•· */
#define U1G_LINMODULE_RESPONSE_DATAFIELD_MAXSIZE ((uint8_t)8u)    /*!< @brief ãƒ¬ã‚¹ãƒãƒ³ã‚¹ã®ãƒ‡ãƒ¼ã‚¿ãƒ•ã‚£ãƒ¼ãƒ«ãƒ‰é•·ã®æœ€å¤§å€¤ */
#define U1G_LINMODULE_LINRSP_MAXSIZE             ((uint8_t)9u)    /*!< @brief ãƒ¬ã‚¹ãƒãƒ³ã‚¹é•·ã®æœ€å¤§å€¤(ãƒ‡ãƒ¼ã‚¿ãƒ•ã‚£ãƒ¼ãƒ«ãƒ‰+ãƒã‚§ãƒƒã‚¯ã‚µãƒ ) */
#define U1G_LINMODULE_LINFRAMEID_INVALID         ((uint8_t)0xFFu) /*!< @brief LINãƒ•ãƒ¬ãƒ¼ãƒ IDç„¡åŠ¹å€¤ */
/*! @} */

/*!
 * @brief @b LINãƒ•ãƒ¬ãƒ¼ãƒ IDç¨®åˆ¥åˆ—æŒ™åž‹ @n
 *      LINãƒ•ãƒ¬ãƒ¼ãƒ IDã®ç¨®é¡žã‚’å®šç¾©ã™ã‚‹ã€‚
 */
typedef enum
{
    ENG_LINMODULE_LINFRAMEID_RSSI                     = 0x10, /*!< @brief RSSIã‚’é€šçŸ¥ã™ã‚‹(0x10-0x16ã§è¨­å®š) */
    ENG_LINMODULE_LINFRAMEID_WRITE_CONNPARAM_1        = 0x21, /*!< @brief æŽ¥ç¶šãƒ‘ãƒ©ãƒ¡ã‚¿æ›¸ãè¾¼ã¿(ãƒ‡ãƒã‚¤ã‚¹No,æŽ¥ç¶šãƒãƒ³ãƒ‰ãƒ«å€¤,æŽ¥ç¶šãƒ­ãƒ¼ãƒ«,ã‚¢ãƒ‰ãƒ¬ã‚¹) */
    ENG_LINMODULE_LINFRAMEID_WRITE_CONNPARAM_2        = 0x22, /*!< @brief æŽ¥ç¶šãƒ‘ãƒ©ãƒ¡ã‚¿æ›¸ãè¾¼ã¿(æŽ¥ç¶šé–“éš”,ãƒ›ãƒƒãƒ—å€¤,æ¬¡ãƒãƒ£ãƒ³ãƒãƒ«) */
    ENG_LINMODULE_LINFRAMEID_WRITE_CONNPARAM_3        = 0x23, /*!< @brief æŽ¥ç¶šãƒ‘ãƒ©ãƒ¡ã‚¿æ›¸ãè¾¼ã¿(ãƒãƒ£ãƒ³ãƒãƒ«ãƒžãƒƒãƒ—,CRCåˆæœŸå€¤) */
    ENG_LINMODULE_LINFRAMEID_START_CONNECTION_MONITOR = 0x24, /*!< @brief ã‚³ãƒã‚¯ã‚·ãƒ§ãƒ³ãƒ¢ãƒ‹ã‚¿é–‹å§‹(ãƒ‡ãƒã‚¤ã‚¹No,é–‹å§‹/åœæ­¢åˆ¤åˆ¥ãƒ“ãƒƒãƒˆ) */
    ENG_LINMODULE_LINFRAMEID_BLEANTTEST_REQ           = 0x33, /*!< @brief BLEã‚¢ãƒ³ãƒ†ãƒŠè©¦é¨“ç”¨ã®è¦æ±‚LINãƒ•ãƒ¬ãƒ¼ãƒ  */
    ENG_LINMODULE_LINFRAMEID_BLEANTTEST_RSP           = 0x34  /*!< @brief BLEã‚¢ãƒ³ãƒ†ãƒŠè©¦é¨“ç”¨ã®å¿œç­”è¦æ±‚LINãƒ•ãƒ¬ãƒ¼ãƒ  */
} t_LinModule_LinFrameId;

/*==============================================================*/
/*  Data Prototypes                                             */
/*==============================================================*/
/*==============================================================*/
/*  Function Prototypes                                         */
/*==============================================================*/
/*!
 * @brief @b LINé€šä¿¡ç®¡ç†åˆæœŸåŒ–å‡¦ç†
 * @param None
 * @return ãªã— @n
 * @details
 *       @n
*/
void f_LIN_Manager_Initialize( void );


/*!
 * @brief @b LINé€šä¿¡ç®¡ç†ãƒ¡ã‚¤ãƒ³å‘¨æœŸå‡¦ç†
 * @param None
 * @return ãªã— @n
 * @details
 *       @n
*/
void f_LIN_Manager_MainFunction( void );


/*!
 * @brief @b LINå—ä¿¡ãƒ‡ãƒ¼ã‚¿å–å¾—
 * @param u2* [out] å—ä¿¡ãƒ‡ãƒ¼ã‚¿é•·
 * @param u1* [out] å—ä¿¡ãƒ‡ãƒ¼ã‚¿
 * @return ãªã— @n
 * @details
 *       @n
*/
void f_LIN_Manager_GetRxData( u2* u2a_DataLength, u1* u1a_Data );


/*!
 * @brief @b LINé€ä¿¡ãƒ‡ãƒ¼ã‚¿è¨­å®š
 * @param u2 é€ä¿¡ãƒ‡ãƒ¼ã‚¿é•·
 * @param u1* é€ä¿¡ãƒ‡ãƒ¼ã‚¿
 * @return ãªã— @n
 * @details
 *       @n
*/
void f_LIN_Manager_SetTxData( u2 u2a_DataLength, const u1* u1a_Data );


/*!
 * @brief @b LINé€ä¿¡å®Œäº†ã‚³ãƒ¼ãƒ«ãƒãƒƒã‚¯
 * @param u1 é€ä¿¡ãƒ•ãƒ¬ãƒ¼ãƒ å
 * @param u1 é€ä¿¡ãƒ‡ãƒ¼ã‚¿é•·
 * @param u1* é€ä¿¡ãƒ‡ãƒ¼ã‚¿
 * @return ãªã— @n
 * @details
 *       @n
*/
void f_LIN_Manager_Callback_TxComplete( u1 u1a_frame, u1 u1a_DataLength, u1* u1a_Data );

void f_LIN_Manager_set_slpreq( u1 u1a_LIN_Manager_slpreq );
void f_LIN_Manager_Driver_sleep( void );

#endif  /* LIN_MANAGER_H */

/**** End of File *********************************************/
