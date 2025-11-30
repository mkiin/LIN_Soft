/****************************************************************/
/**
 *  @file       LIN_Manager.h
 *  @brief      @b LIN通信管理 @n
 *  @details
 *      #### ユニットID
 *       ID;; @n
 *      @if false
 *           ID;;
 *          REF;;
 *          @copyright  AISIN CORPORATION
 *      @endif
 *      #### ユニット仕様
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
#define U2G_LIN_MANAGER_MAX_PAYLOAD_SIZE        ((u2)307U)  /*!< @brief 最大ペイロードサイズ */


/*!
 * @name LINフレーム関連定数
 * @brief LINフレームに関連する定数
 */
/*! @{ */
#define U1G_LINMODULE_HEADER_SIZE                ((uint8_t)3u)    /*!< @brief ヘッダ長 */
#define U1G_LINMODULE_RESPONSE_DATAFIELD_MAXSIZE ((uint8_t)8u)    /*!< @brief レスポンスのデータフィールド長の最大値 */
#define U1G_LINMODULE_LINRSP_MAXSIZE             ((uint8_t)9u)    /*!< @brief レスポンス長の最大値(データフィールド+チェックサム) */
#define U1G_LINMODULE_LINFRAMEID_INVALID         ((uint8_t)0xFFu) /*!< @brief LINフレームID無効値 */
/*! @} */

/*!
 * @brief @b LINフレームID種別列挙型 @n
 *      LINフレームIDの種類を定義する。
 */
typedef enum
{
    ENG_LINMODULE_LINFRAMEID_RSSI                     = 0x10, /*!< @brief RSSIを通知する(0x10-0x16で設定) */
    ENG_LINMODULE_LINFRAMEID_WRITE_CONNPARAM_1        = 0x21, /*!< @brief 接続パラメタ書き込み(デバイスNo,接続ハンドル値,接続ロール,アドレス) */
    ENG_LINMODULE_LINFRAMEID_WRITE_CONNPARAM_2        = 0x22, /*!< @brief 接続パラメタ書き込み(接続間隔,ホップ値,次チャンネル) */
    ENG_LINMODULE_LINFRAMEID_WRITE_CONNPARAM_3        = 0x23, /*!< @brief 接続パラメタ書き込み(チャンネルマップ,CRC初期値) */
    ENG_LINMODULE_LINFRAMEID_START_CONNECTION_MONITOR = 0x24, /*!< @brief コネクションモニタ開始(デバイスNo,開始/停止判別ビット) */
    ENG_LINMODULE_LINFRAMEID_BLEANTTEST_REQ           = 0x33, /*!< @brief BLEアンテナ試験用の要求LINフレーム */
    ENG_LINMODULE_LINFRAMEID_BLEANTTEST_RSP           = 0x34  /*!< @brief BLEアンテナ試験用の応答要求LINフレーム */
} t_LinModule_LinFrameId;

/*==============================================================*/
/*  Data Prototypes                                             */
/*==============================================================*/
/*==============================================================*/
/*  Function Prototypes                                         */
/*==============================================================*/
/*!
 * @brief @b LIN通信管理初期化処理
 * @param None
 * @return なし @n
 * @details
 *       @n
*/
void f_LIN_Manager_Initialize( void );


/*!
 * @brief @b LIN通信管理メイン周期処理
 * @param None
 * @return なし @n
 * @details
 *       @n
*/
void f_LIN_Manager_MainFunction( void );


/*!
 * @brief @b LIN受信データ取得
 * @param u2* [out] 受信データ長
 * @param u1* [out] 受信データ
 * @return なし @n
 * @details
 *       @n
*/
void f_LIN_Manager_GetRxData( u2* u2a_DataLength, u1* u1a_Data );


/*!
 * @brief @b LIN送信データ設定
 * @param u2 送信データ長
 * @param u1* 送信データ
 * @return なし @n
 * @details
 *       @n
*/
void f_LIN_Manager_SetTxData( u2 u2a_DataLength, const u1* u1a_Data );


/*!
 * @brief @b LIN送信完了コールバック
 * @param u1 送信フレーム名
 * @param u1 送信データ長
 * @param u1* 送信データ
 * @return なし @n
 * @details
 *       @n
*/
void f_LIN_Manager_Callback_TxComplete( u1 u1a_frame, u1 u1a_DataLength, u1* u1a_Data );

void f_LIN_Manager_set_slpreq( u1 u1a_LIN_Manager_slpreq );
void f_LIN_Manager_Driver_sleep( void );

#endif  /* LIN_MANAGER_H */

/**** End of File *********************************************/