/**
 * @file        UART_RegInt_Config.h
 *
 * @brief       UART Register Interrupt Driver - Device Specific Configuration
 *
 * @details     CC2340R53 (CC23X0R5) specific configuration constants
 *              for the register-based UART interrupt driver.
 *
 * @note        このファイルはCC2340R53デバイス固有の設定を含みます
 */

#ifndef UART_REGINT_CONFIG_H
#define UART_REGINT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================================*/
/* Includes                                                                 */
/*==========================================================================*/

#include <ti/devices/cc23x0r5/inc/hw_memmap.h>
#include <ti/devices/cc23x0r5/inc/hw_ints.h>

/*==========================================================================*/
/* Device Specific Constants                                               */
/*==========================================================================*/

/**
 * @brief UART0 Base Address
 * 
 * CC2340R53のUART0ペリフェラルベースアドレス
 */
#define UART_REGINT_BASE_ADDR           UART0_BASE      /* 0x40034000 */

/**
 * @brief UART0 Combined Interrupt Number
 * 
 * CC2340R53のUART0統合割り込み番号
 */
#define UART_REGINT_INT_NUM             INT_UART0_COMB  /* 27 */

/**
 * @brief UART FIFO Size
 * 
 * CC2340R53のUART送受信FIFO深度
 */
#define UART_REGINT_FIFO_SIZE           (8U)

/**
 * @brief Maximum Transfer Size
 * 
 * 1回の送受信で扱える最大バイト数
 */
#define UART_REGINT_MAX_TRANSFER_SIZE   (256U)

/*==========================================================================*/
/* Baud Rate Selection                                                     */
/*==========================================================================*/

/**
 * @brief Baud Rate Options
 * 
 * 利用可能なボーレート設定値
 */
#define UART_REGINT_BAUDRATE_2400       (2400U)
#define UART_REGINT_BAUDRATE_9600       (9600U)
#define UART_REGINT_BAUDRATE_19200      (19200U)
#define UART_REGINT_BAUDRATE_38400      (38400U)
#define UART_REGINT_BAUDRATE_57600      (57600U)
#define UART_REGINT_BAUDRATE_115200     (115200U)

/**
 * @brief Default Baud Rate
 * 
 * デフォルトのボーレート設定
 */
#define UART_REGINT_DEFAULT_BAUDRATE    UART_REGINT_BAUDRATE_9600

/*==========================================================================*/
/* Power Management                                                        */
/*==========================================================================*/

/**
 * @brief Power Domain for UART0
 * 
 * CC23X0デバイスのUART0電源ドメイン識別子
 * 
 * @note CC27XXとは異なるマクロを使用する必要があります
 */
#include <ti/drivers/power/PowerCC23X0.h>
#define UART_REGINT_POWER_ID            PowerLPF3_PERIPH_UART0

/*==========================================================================*/
/* Interrupt Priority                                                      */
/*==========================================================================*/

/**
 * @brief Default Interrupt Priority
 * 
 * デフォルトの割り込み優先度
 * CC23X0は3ビット優先度 (0-7, 0は使用不可)
 * 
 * @note 優先度0はゼロレイテンシ割り込み用で本ドライバでは非対応
 */
#define UART_REGINT_DEFAULT_INT_PRIORITY    (5U << 5)

/**
 * @brief Low Priority
 */
#define UART_REGINT_LOW_PRIORITY            (7U << 5)

/**
 * @brief High Priority
 */
#define UART_REGINT_HIGH_PRIORITY           (1U << 5)

/*==========================================================================*/
/* Clock Configuration                                                     */
/*==========================================================================*/

/**
 * @brief Clock Division for CC23X0
 * 
 * CC23X0ファミリでは、UARTクロックはCPUクロックと同じです。
 * CC27XXのようにfreq/2を行う必要はありません。
 * 
 * @note この定義はドキュメント目的で、実際のコードではfreq.loをそのまま使用
 */
#define UART_REGINT_CLOCK_DIVIDER           (1U)    /* No division for CC23X0 */

/*==========================================================================*/
/* Timeout Configuration                                                   */
/*==========================================================================*/

/**
 * @brief Default Read Timeout (microseconds)
 * 
 * デフォルトの受信タイムアウト値
 */
#define UART_REGINT_DEFAULT_READ_TIMEOUT_US     (100000U)   /* 100ms */

/**
 * @brief Default Write Timeout (microseconds)
 * 
 * デフォルトの送信タイムアウト値
 */
#define UART_REGINT_DEFAULT_WRITE_TIMEOUT_US    (100000U)   /* 100ms */

/**
 * @brief Wait Forever
 * 
 * タイムアウトなし（無限待機）
 */
#define UART_REGINT_WAIT_FOREVER                (~(0U))

/*==========================================================================*/
/* Buffer Configuration                                                    */
/*==========================================================================*/

/**
 * @brief Default RX Buffer Size
 * 
 * デフォルトの受信バッファサイズ
 */
#define UART_REGINT_DEFAULT_RX_BUF_SIZE     (64U)

/**
 * @brief Default TX Buffer Size
 * 
 * デフォルトの送信バッファサイズ
 */
#define UART_REGINT_DEFAULT_TX_BUF_SIZE     (64U)

/**
 * @brief Minimum Buffer Size
 * 
 * 最小バッファサイズ
 */
#define UART_REGINT_MIN_BUF_SIZE            (8U)

#ifdef __cplusplus
}
#endif

#endif /* UART_REGINT_CONFIG_H */
