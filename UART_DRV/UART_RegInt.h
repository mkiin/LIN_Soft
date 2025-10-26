/**
 * @file        UART_RegInt.h
 *
 * @brief       UART Register Interrupt Driver - Public API
 *
 * @details     レジスタ割り込みベースのUARTドライバ公開API
 *              DMA機能を使用せず、レジスタ直接アクセスと割り込み処理で
 *              UART通信を実現します。
 *
 * @note        TI UART2ドライバとAPI互換性を持たせた設計
 *
 * @par Usage Example
 * @code
 * UART_RegInt_Handle handle;
 * UART_RegInt_Params params;
 * 
 * UART_RegInt_init();
 * UART_RegInt_Params_init(&params);
 * params.baudRate = 9600;
 * params.readMode = UART_REGINT_MODE_CALLBACK;
 * params.readCallback = myReadCallback;
 * 
 * handle = UART_RegInt_open(0, &params);
 * if (handle == NULL) {
 *     // Error handling
 * }
 * 
 * UART_RegInt_read(handle, rxBuffer, sizeof(rxBuffer));
 * UART_RegInt_write(handle, txBuffer, sizeof(txBuffer));
 * @endcode
 */

#ifndef UART_REGINT_H
#define UART_REGINT_H

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================================*/
/* Includes                                                                 */
/*==========================================================================*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*==========================================================================*/
/* Type Definitions                                                        */
/*==========================================================================*/

/**
 * @brief UART_RegInt Handle
 * 
 * UARTドライバインスタンスへの不透明ポインタ
 */
typedef struct UART_RegInt_Object_ *UART_RegInt_Handle;

/*==========================================================================*/
/* Status Codes                                                            */
/*==========================================================================*/

/**
 * @brief Status Codes
 * 
 * API関数の戻り値として使用されるステータスコード
 */
#define UART_REGINT_STATUS_SUCCESS      (0)     /**< 成功 */
#define UART_REGINT_STATUS_ERROR        (-1)    /**< 一般エラー */
#define UART_REGINT_STATUS_EINUSE       (-2)    /**< リソース使用中 */
#define UART_REGINT_STATUS_ETIMEOUT     (-3)    /**< タイムアウト */
#define UART_REGINT_STATUS_ECANCELLED   (-4)    /**< キャンセル */
#define UART_REGINT_STATUS_EFRAMING     (-5)    /**< フレーミングエラー */
#define UART_REGINT_STATUS_EPARITY      (-6)    /**< パリティエラー */
#define UART_REGINT_STATUS_EBREAK       (-7)    /**< ブレークエラー */
#define UART_REGINT_STATUS_EOVERRUN     (-8)    /**< オーバーランエラー */
#define UART_REGINT_STATUS_EINVALID     (-9)    /**< 無効なパラメータ */

/*==========================================================================*/
/* Event Flags                                                             */
/*==========================================================================*/

/**
 * @brief Event Flags
 * 
 * イベントコールバックで通知されるイベントフラグ
 */
#define UART_REGINT_EVENT_OVERRUN       (0x01U) /**< オーバーランエラー発生 */
#define UART_REGINT_EVENT_BREAK         (0x02U) /**< ブレークエラー発生 */
#define UART_REGINT_EVENT_PARITY        (0x04U) /**< パリティエラー発生 */
#define UART_REGINT_EVENT_FRAMING       (0x08U) /**< フレーミングエラー発生 */
#define UART_REGINT_EVENT_TX_BEGIN      (0x10U) /**< 送信開始 */
#define UART_REGINT_EVENT_TX_FINISHED   (0x20U) /**< 送信完了 */

/*==========================================================================*/
/* Operating Modes                                                         */
/*==========================================================================*/

/**
 * @brief UART Operating Mode
 * 
 * UART動作モード
 */
typedef enum
{
    UART_REGINT_MODE_BLOCKING = 0,  /**< ブロッキングモード（完了まで待機） */
    UART_REGINT_MODE_CALLBACK,      /**< コールバックモード（非ブロッキング） */
} UART_RegInt_Mode;

/*==========================================================================*/
/* Data Configuration                                                      */
/*==========================================================================*/

/**
 * @brief Data Length
 * 
 * UARTデータ長
 */
typedef enum
{
    UART_REGINT_DataLen_5 = 0,      /**< 5ビット */
    UART_REGINT_DataLen_6,          /**< 6ビット */
    UART_REGINT_DataLen_7,          /**< 7ビット */
    UART_REGINT_DataLen_8,          /**< 8ビット (デフォルト) */
} UART_RegInt_DataLen;

/**
 * @brief Stop Bits
 * 
 * UARTストップビット
 */
typedef enum
{
    UART_REGINT_StopBits_1 = 0,     /**< 1ストップビット (デフォルト) */
    UART_REGINT_StopBits_2,         /**< 2ストップビット */
} UART_RegInt_StopBits;

/**
 * @brief Parity Type
 * 
 * UARTパリティ設定
 */
typedef enum
{
    UART_REGINT_Parity_NONE = 0,    /**< パリティなし (デフォルト) */
    UART_REGINT_Parity_EVEN,        /**< 偶数パリティ */
    UART_REGINT_Parity_ODD,         /**< 奇数パリティ */
    UART_REGINT_Parity_ZERO,        /**< 固定0パリティ */
    UART_REGINT_Parity_ONE,         /**< 固定1パリティ */
} UART_RegInt_Parity;

/**
 * @brief Read Return Mode
 * 
 * 受信データ返却モード
 */
typedef enum
{
    UART_REGINT_ReadReturnMode_FULL = 0,    /**< 要求バイト数完全受信まで待機 */
    UART_REGINT_ReadReturnMode_PARTIAL,     /**< 部分受信でも返却 */
} UART_RegInt_ReadReturnMode;

/*==========================================================================*/
/* Callback Function Types                                                */
/*==========================================================================*/

/**
 * @brief Read/Write Callback Function
 * 
 * 送受信完了時に呼び出されるコールバック関数
 *
 * @param[in] handle    UARTハンドル
 * @param[in] buf       送受信バッファへのポインタ
 * @param[in] count     実際に送受信されたバイト数
 * @param[in] userArg   ユーザー引数 (Paramsで設定)
 * @param[in] status    ステータスコード
 *
 * @note コールバックは割り込みコンテキストで実行されます
 *       ブロッキング処理を行わないでください
 */
typedef void (*UART_RegInt_Callback)(UART_RegInt_Handle handle,
                                     void *buf,
                                     size_t count,
                                     void *userArg,
                                     int_fast16_t status);

/**
 * @brief Event Callback Function
 * 
 * UARTイベント発生時に呼び出されるコールバック関数
 *
 * @param[in] handle    UARTハンドル
 * @param[in] event     イベントフラグ (UART_REGINT_EVENT_*)
 * @param[in] data      イベント固有データ
 * @param[in] userArg   ユーザー引数 (Paramsで設定)
 *
 * @note コールバックは割り込みコンテキストで実行されます
 */
typedef void (*UART_RegInt_EventCallback)(UART_RegInt_Handle handle,
                                          uint32_t event,
                                          uint32_t data,
                                          void *userArg);

/*==========================================================================*/
/* Parameter Structure                                                     */
/*==========================================================================*/

/**
 * @brief UART Parameters
 * 
 * UART設定パラメータ構造体
 */
typedef struct
{
    UART_RegInt_Mode            readMode;           /**< 受信モード */
    UART_RegInt_Mode            writeMode;          /**< 送信モード */
    UART_RegInt_Callback        readCallback;       /**< 受信コールバック */
    UART_RegInt_Callback        writeCallback;      /**< 送信コールバック */
    UART_RegInt_EventCallback   eventCallback;      /**< イベントコールバック */
    uint32_t                    eventMask;          /**< イベントマスク */
    UART_RegInt_ReadReturnMode  readReturnMode;     /**< 受信返却モード */
    uint32_t                    baudRate;           /**< ボーレート */
    UART_RegInt_DataLen         dataLength;         /**< データ長 */
    UART_RegInt_StopBits        stopBits;           /**< ストップビット */
    UART_RegInt_Parity          parityType;         /**< パリティ */
    void                        *userArg;           /**< ユーザー引数 */
} UART_RegInt_Params;

/*==========================================================================*/
/* Configuration Structure                                                 */
/*==========================================================================*/

/**
 * @brief UART Configuration
 * 
 * UART設定構造体（静的設定用）
 */
typedef struct
{
    void                        *object;            /**< オブジェクトインスタンス */
    void const                  *hwAttrs;           /**< ハードウェア属性 */
} UART_RegInt_Config;

/**
 * @brief UART設定テーブルの要素数
 * 
 * アプリケーションで定義される必要があります
 */
extern const uint_least8_t UART_RegInt_count;

/**
 * @brief UART設定テーブル
 * 
 * アプリケーションで定義される必要があります
 */
extern const UART_RegInt_Config UART_RegInt_config[];

/*==========================================================================*/
/* API Functions                                                           */
/*==========================================================================*/

/**
 * @brief モジュール初期化
 * 
 * UARTドライバモジュールを初期化します。
 * 他のAPI関数を呼び出す前に1回だけ呼び出してください。
 *
 * @note この関数は複数回呼び出しても安全です（冪等）
 */
void UART_RegInt_init(void);

/**
 * @brief パラメータ初期化
 * 
 * UART_RegInt_Params構造体をデフォルト値で初期化します。
 *
 * @param[out] params   初期化するパラメータ構造体
 *
 * @par デフォルト値
 * - readMode: UART_REGINT_MODE_BLOCKING
 * - writeMode: UART_REGINT_MODE_BLOCKING
 * - readCallback: NULL
 * - writeCallback: NULL
 * - eventCallback: NULL
 * - eventMask: 0
 * - readReturnMode: UART_REGINT_ReadReturnMode_FULL
 * - baudRate: 9600
 * - dataLength: UART_REGINT_DataLen_8
 * - stopBits: UART_REGINT_StopBits_1
 * - parityType: UART_REGINT_Parity_NONE
 * - userArg: NULL
 */
void UART_RegInt_Params_init(UART_RegInt_Params *params);

/**
 * @brief UARTオープン
 * 
 * 指定されたインデックスのUARTインスタンスを開きます。
 *
 * @param[in] index     UART設定テーブルのインデックス
 * @param[in] params    UARTパラメータ（NULLの場合デフォルト値）
 * @return UARTハンドル（失敗時はNULL）
 *
 * @note 同じインデックスを複数回開くことはできません
 */
UART_RegInt_Handle UART_RegInt_open(uint_least8_t index, 
                                    UART_RegInt_Params *params);

/**
 * @brief UARTクローズ
 * 
 * UARTインスタンスを閉じ、リソースを解放します。
 *
 * @param[in] handle    UARTハンドル
 *
 * @note 実行中の送受信処理はキャンセルされます
 */
void UART_RegInt_close(UART_RegInt_Handle handle);

/**
 * @brief データ受信
 * 
 * UARTからデータを受信します。
 * - BLOCKING モード: 受信完了まで待機
 * - CALLBACK モード: 即座に戻り、完了時にコールバック呼び出し
 *
 * @param[in]  handle   UARTハンドル
 * @param[out] buf      受信バッファ
 * @param[in]  size     受信バイト数
 * @return ステータスコード
 *         - BLOCKING: 受信バイト数 or エラーコード
 *         - CALLBACK: UART_REGINT_STATUS_SUCCESS or エラーコード
 */
int_fast16_t UART_RegInt_read(UART_RegInt_Handle handle,
                              void *buf,
                              size_t size);

/**
 * @brief データ受信（タイムアウト指定）
 * 
 * タイムアウト付きでUARTからデータを受信します。
 *
 * @param[in]  handle   UARTハンドル
 * @param[out] buf      受信バッファ
 * @param[in]  size     受信バイト数
 * @param[in]  timeout  タイムアウト（マイクロ秒、UART_REGINT_WAIT_FOREVERで無限待機）
 * @return 受信バイト数 or エラーコード
 */
int_fast16_t UART_RegInt_readTimeout(UART_RegInt_Handle handle,
                                     void *buf,
                                     size_t size,
                                     uint32_t timeout);

/**
 * @brief 受信キャンセル
 * 
 * 実行中の受信処理をキャンセルします。
 *
 * @param[in] handle    UARTハンドル
 */
void UART_RegInt_readCancel(UART_RegInt_Handle handle);

/**
 * @brief データ送信
 * 
 * UARTへデータを送信します。
 * - BLOCKING モード: 送信完了まで待機
 * - CALLBACK モード: 即座に戻り、完了時にコールバック呼び出し
 *
 * @param[in] handle    UARTハンドル
 * @param[in] buf       送信バッファ
 * @param[in] size      送信バイト数
 * @return ステータスコード
 *         - BLOCKING: 送信バイト数 or エラーコード
 *         - CALLBACK: UART_REGINT_STATUS_SUCCESS or エラーコード
 */
int_fast16_t UART_RegInt_write(UART_RegInt_Handle handle,
                               const void *buf,
                               size_t size);

/**
 * @brief データ送信（タイムアウト指定）
 * 
 * タイムアウト付きでUARTへデータを送信します。
 *
 * @param[in] handle    UARTハンドル
 * @param[in] buf       送信バッファ
 * @param[in] size      送信バイト数
 * @param[in] timeout   タイムアウト（マイクロ秒、UART_REGINT_WAIT_FOREVERで無限待機）
 * @return 送信バイト数 or エラーコード
 */
int_fast16_t UART_RegInt_writeTimeout(UART_RegInt_Handle handle,
                                      const void *buf,
                                      size_t size,
                                      uint32_t timeout);

/**
 * @brief 送信キャンセル
 * 
 * 実行中の送信処理をキャンセルします。
 *
 * @param[in] handle    UARTハンドル
 */
void UART_RegInt_writeCancel(UART_RegInt_Handle handle);

/**
 * @brief 受信有効化
 * 
 * UART受信を有効化します。
 *
 * @param[in] handle    UARTハンドル
 */
void UART_RegInt_rxEnable(UART_RegInt_Handle handle);

/**
 * @brief 受信無効化
 * 
 * UART受信を無効化します。
 *
 * @param[in] handle    UARTハンドル
 */
void UART_RegInt_rxDisable(UART_RegInt_Handle handle);

/**
 * @brief オーバーランカウント取得
 * 
 * 発生したオーバーランエラーの累積カウントを取得します。
 *
 * @param[in] handle    UARTハンドル
 * @return オーバーランカウント
 */
uint32_t UART_RegInt_getOverrunCount(UART_RegInt_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* UART_REGINT_H */
