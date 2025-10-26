/**
 * @file        UART_RegInt_Priv.h
 *
 * @brief       UART Register Interrupt Driver - Private Definitions
 *
 * @details     Internal data structures, macros, and function prototypes
 *              for the register-based UART interrupt driver.
 *
 * @note        このファイルはドライバ内部でのみ使用されます
 *              アプリケーションから直接インクルードしないでください
 */

#ifndef UART_REGINT_PRIV_H
#define UART_REGINT_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================================*/
/* Includes                                                                 */
/*==========================================================================*/

#include "UART_RegInt.h"
#include "UART_RegInt_Config.h"

#include <ti/devices/cc23x0r5/inc/hw_uart.h>
#include <ti/devices/cc23x0r5/driverlib/uart.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/Power.h>

/*==========================================================================*/
/* Internal State Flags                                                    */
/*==========================================================================*/

/**
 * @brief Internal State Flags
 * 
 * ドライバの内部状態を管理するビットフラグ
 */
typedef struct
{
    uint8_t opened          : 1;    /**< ドライバが開かれているか */
    uint8_t rxEnabled       : 1;    /**< 受信が有効か */
    uint8_t txEnabled       : 1;    /**< 送信が有効か */
    uint8_t rxInProgress    : 1;    /**< 受信処理中か */
    uint8_t txInProgress    : 1;    /**< 送信処理中か */
    uint8_t rxCancelled     : 1;    /**< 受信がキャンセルされたか */
    uint8_t txCancelled     : 1;    /**< 送信がキャンセルされたか */
    uint8_t reserved        : 1;    /**< 予約 */
} UART_RegInt_StateFlags;

/*==========================================================================*/
/* Internal Object Structure                                               */
/*==========================================================================*/

/**
 * @brief UART Register Interrupt Object
 * 
 * ドライバのランタイム状態を保持する構造体
 * 
 * @note この構造体はアプリケーションから直接アクセスしてはいけません
 */
struct UART_RegInt_Object_
{
    /* Configuration */
    UART_RegInt_Params          params;             /**< ユーザー設定パラメータ */
    uint32_t                    baseAddr;           /**< UARTベースアドレス */
    
    /* State Management */
    UART_RegInt_StateFlags      state;              /**< 状態フラグ */
    uint32_t                    overrunCount;       /**< オーバーランカウント */
    
    /* Hardware Resources */
    HwiP_Struct                 hwi;                /**< 割り込みオブジェクト */
    SemaphoreP_Struct           readSem;            /**< 読み込み同期セマフォ */
    SemaphoreP_Struct           writeSem;           /**< 書き込み同期セマフォ */
    
    /* RX Transaction Data */
    uint8_t                     *rxBuf;             /**< 受信バッファポインタ */
    size_t                      rxSize;             /**< 受信要求サイズ */
    size_t                      rxCount;            /**< 受信済みバイト数 */
    int_fast16_t                rxStatus;           /**< 受信ステータス */
    
    /* TX Transaction Data */
    const uint8_t               *txBuf;             /**< 送信バッファポインタ */
    size_t                      txSize;             /**< 送信要求サイズ */
    size_t                      txCount;            /**< 送信済みバイト数 */
    int_fast16_t                txStatus;           /**< 送信ステータス */
    
    /* Power Management */
    Power_NotifyObj             preNotify;          /**< スタンバイ前通知 */
    Power_NotifyObj             postNotify;         /**< スタンバイ後通知 */
};

/*==========================================================================*/
/* Internal Macros                                                         */
/*==========================================================================*/

/**
 * @brief レジスタアクセスマクロ
 */
#define UART_REGINT_HWREG(x)    (*((volatile uint32_t *)(x)))

/**
 * @brief 受信データレジスタ読み取り
 */
#define UART_REGINT_READ_DATA(baseAddr) \
    ((uint8_t)UART_REGINT_HWREG((baseAddr) + UART_O_DR))

/**
 * @brief 送信データレジスタ書き込み
 */
#define UART_REGINT_WRITE_DATA(baseAddr, data) \
    UART_REGINT_HWREG((baseAddr) + UART_O_DR) = ((uint32_t)(data))

/**
 * @brief フラグレジスタ読み取り
 */
#define UART_REGINT_READ_FLAGS(baseAddr) \
    UART_REGINT_HWREG((baseAddr) + UART_O_FR)

/**
 * @brief 受信FIFO空チェック
 */
#define UART_REGINT_RX_FIFO_EMPTY(baseAddr) \
    (UART_REGINT_READ_FLAGS(baseAddr) & UART_FR_RXFE)

/**
 * @brief 送信FIFO満杯チェック
 */
#define UART_REGINT_TX_FIFO_FULL(baseAddr) \
    (UART_REGINT_READ_FLAGS(baseAddr) & UART_FR_TXFF)

/**
 * @brief エラーステータス読み取り
 */
#define UART_REGINT_READ_ERROR_STATUS(baseAddr) \
    (UART_REGINT_HWREG((baseAddr) + UART_O_RSR_ECR) & 0x0FU)

/**
 * @brief エラーステータスクリア
 */
#define UART_REGINT_CLEAR_ERROR_STATUS(baseAddr) \
    UART_REGINT_HWREG((baseAddr) + UART_O_RSR_ECR) = 0x00U

/*==========================================================================*/
/* Error Status Bits                                                       */
/*==========================================================================*/

/**
 * @brief UART Error Status Bits (RSR/ECR Register)
 */
#define UART_REGINT_ERR_OVERRUN     (0x08U)     /**< Overrun Error (OE) */
#define UART_REGINT_ERR_BREAK       (0x04U)     /**< Break Error (BE) */
#define UART_REGINT_ERR_PARITY      (0x02U)     /**< Parity Error (PE) */
#define UART_REGINT_ERR_FRAMING     (0x01U)     /**< Framing Error (FE) */

/*==========================================================================*/
/* Interrupt Mask Bits                                                     */
/*==========================================================================*/

/**
 * @brief UART Interrupt Mask Bits
 * 
 * 使用する割り込みソース
 */
#define UART_REGINT_INT_RX          UART_INT_RX     /**< 受信割り込み */
#define UART_REGINT_INT_TX          UART_INT_TX     /**< 送信割り込み */
#define UART_REGINT_INT_RT          UART_INT_RT     /**< 受信タイムアウト */
#define UART_REGINT_INT_OE          UART_INT_OE     /**< オーバーラン */
#define UART_REGINT_INT_BE          UART_INT_BE     /**< ブレークエラー */
#define UART_REGINT_INT_PE          UART_INT_PE     /**< パリティエラー */
#define UART_REGINT_INT_FE          UART_INT_FE     /**< フレーミングエラー */

/**
 * @brief All Error Interrupts
 */
#define UART_REGINT_INT_ALL_ERRORS  (UART_REGINT_INT_OE | \
                                     UART_REGINT_INT_BE | \
                                     UART_REGINT_INT_PE | \
                                     UART_REGINT_INT_FE)

/**
 * @brief Default Interrupt Mask (RX + Errors)
 */
#define UART_REGINT_INT_DEFAULT     (UART_REGINT_INT_RX | \
                                     UART_REGINT_INT_RT | \
                                     UART_REGINT_INT_ALL_ERRORS)

/*==========================================================================*/
/* Internal Function Prototypes                                            */
/*==========================================================================*/

/**
 * @brief UART割り込みハンドラ
 * 
 * @param[in] arg   ハンドルへのポインタ (uintptr_t形式)
 */
void UART_RegInt_hwiIntFxn(uintptr_t arg);

/**
 * @brief ハードウェア初期化
 * 
 * @param[in] handle    UARTハンドル
 */
void UART_RegInt_initHw(UART_RegInt_Handle handle);

/**
 * @brief 電源管理 - スタンバイ前通知
 * 
 * @param[in] eventType     イベントタイプ
 * @param[in] eventArg      イベント引数
 * @param[in] clientArg     クライアント引数
 * @return Power_NOTIFYDONE on success
 */
int UART_RegInt_preNotifyFxn(unsigned int eventType, 
                             uintptr_t eventArg, 
                             uintptr_t clientArg);

/**
 * @brief 電源管理 - スタンバイ後通知
 * 
 * @param[in] eventType     イベントタイプ
 * @param[in] eventArg      イベント引数
 * @param[in] clientArg     クライアント引数
 * @return Power_NOTIFYDONE on success
 */
int UART_RegInt_postNotifyFxn(unsigned int eventType, 
                              uintptr_t eventArg, 
                              uintptr_t clientArg);

/**
 * @brief 受信処理完了（コールバック呼び出し）
 * 
 * @param[in] handle    UARTハンドル
 * @param[in] status    受信ステータス
 */
void UART_RegInt_completeRead(UART_RegInt_Handle handle, int_fast16_t status);

/**
 * @brief 送信処理完了（コールバック呼び出し）
 * 
 * @param[in] handle    UARTハンドル
 * @param[in] status    送信ステータス
 */
void UART_RegInt_completeWrite(UART_RegInt_Handle handle, int_fast16_t status);

/**
 * @brief UART有効化
 * 
 * @param[in] baseAddr  UARTベースアドレス
 */
static inline void UART_RegInt_enable(uint32_t baseAddr)
{
    UARTEnable(baseAddr);
}

/**
 * @brief UART無効化
 * 
 * @param[in] baseAddr  UARTベースアドレス
 */
static inline void UART_RegInt_disable(uint32_t baseAddr)
{
    UARTDisable(baseAddr);
}

/**
 * @brief 割り込み有効化
 * 
 * @param[in] baseAddr  UARTベースアドレス
 * @param[in] intFlags  割り込みフラグ
 */
static inline void UART_RegInt_enableInt(uint32_t baseAddr, uint32_t intFlags)
{
    UARTEnableInt(baseAddr, intFlags);
}

/**
 * @brief 割り込み無効化
 * 
 * @param[in] baseAddr  UARTベースアドレス
 * @param[in] intFlags  割り込みフラグ
 */
static inline void UART_RegInt_disableInt(uint32_t baseAddr, uint32_t intFlags)
{
    UARTDisableInt(baseAddr, intFlags);
}

/**
 * @brief 割り込みステータス取得
 * 
 * @param[in] baseAddr  UARTベースアドレス
 * @return 割り込みステータス
 */
static inline uint32_t UART_RegInt_getIntStatus(uint32_t baseAddr)
{
    return UARTGetIntStatus(baseAddr, true);
}

/**
 * @brief 割り込みステータスクリア
 * 
 * @param[in] baseAddr  UARTベースアドレス
 * @param[in] intFlags  割り込みフラグ
 */
static inline void UART_RegInt_clearInt(uint32_t baseAddr, uint32_t intFlags)
{
    UARTClearInt(baseAddr, intFlags);
}

#ifdef __cplusplus
}
#endif

#endif /* UART_REGINT_PRIV_H */
