/*
 * Copyright (c) 2025, Custom Implementation
 * Based on Texas Instruments UART2 driver structure
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!****************************************************************************
 *  @file       UART_RegInt.h
 *
 *  @brief      Register Interrupt based UART driver (DMA-free)
 *
 *  @anchor ti_drivers_UART_RegInt_Overview
 *  # Overview
 *
 *  The UART_RegInt driver provides interrupt-driven UART communication without
 *  using DMA. It is API-compatible with TI's UART2 driver for easy migration.
 *
 *  This driver is optimized for LIN communication on CC2340R53, supporting
 *  low to moderate baudrates (2400-19200 bps) with efficient FIFO usage.
 *
 *  @anchor ti_drivers_UART_RegInt_Usage
 *  # Usage
 *
 *  ## Initialization #
 *  @code
 *  UART_RegInt_init();
 *
 *  UART_RegInt_Params params;
 *  UART_RegInt_Params_init(&params);
 *  params.baudRate = 9600;
 *  params.readMode = UART_REGINT_MODE_CALLBACK;
 *  params.readCallback = myReadCallback;
 *
 *  UART_RegInt_Handle handle = UART_RegInt_open(0, &params);
 *  if (handle == NULL) {
 *      // Error handling
 *  }
 *  @endcode
 *
 *  ## Transmit Data #
 *  @code
 *  uint8_t txBuf[8] = {0x55, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
 *  int_fast16_t result = UART_RegInt_write(handle, txBuf, 8);
 *  @endcode
 *
 *  ## Receive Data #
 *  @code
 *  uint8_t rxBuf[8];
 *  int_fast16_t result = UART_RegInt_read(handle, rxBuf, 8);
 *  @endcode
 *
 ******************************************************************************
 */

#ifndef ti_drivers_UART_RegInt__include
#define ti_drivers_UART_RegInt__include

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  @brief      A handle that is returned from a UART_RegInt_open() call
 */
typedef struct UART_RegInt_Config_ *UART_RegInt_Handle;

/* Flow control modes */
#define UART_REGINT_FLOWCTRL_NONE     (0)  /*!< No flow control */
#define UART_REGINT_FLOWCTRL_HARDWARE (1)  /*!< Hardware flow control (CTS/RTS) */

/* Status codes */
#define UART_REGINT_STATUS_SUCCESS    (0)   /*!< Success */
#define UART_REGINT_STATUS_ERROR      (-1)  /*!< General error */
#define UART_REGINT_STATUS_EINUSE     (-2)  /*!< Resource in use */
#define UART_REGINT_STATUS_ETIMEOUT   (-3)  /*!< Operation timed out */
#define UART_REGINT_STATUS_ECANCELLED (-4)  /*!< Operation cancelled */
#define UART_REGINT_STATUS_EFRAMING   (-5)  /*!< Framing error */
#define UART_REGINT_STATUS_EPARITY    (-6)  /*!< Parity error */
#define UART_REGINT_STATUS_EBREAK     (-7)  /*!< Break error */
#define UART_REGINT_STATUS_EOVERRUN   (-8)  /*!< Overrun error */
#define UART_REGINT_STATUS_EINVALID   (-9)  /*!< Invalid parameters */
#define UART_REGINT_STATUS_ENOTOPEN   (-10) /*!< UART not opened */

/* Event flags */
#define UART_REGINT_EVENT_OVERRUN     (0x01) /*!< Overrun error occurred */
#define UART_REGINT_EVENT_BREAK       (0x02) /*!< Break error occurred */
#define UART_REGINT_EVENT_PARITY      (0x04) /*!< Parity error occurred */
#define UART_REGINT_EVENT_FRAMING     (0x08) /*!< Framing error occurred */
#define UART_REGINT_EVENT_TX_BEGIN    (0x10) /*!< Transmission started */
#define UART_REGINT_EVENT_TX_FINISHED (0x20) /*!< Transmission finished */

/* Wait forever timeout */
#define UART_REGINT_WAIT_FOREVER      (~(0U))

/*!
 *  @brief      UART operating modes
 */
typedef enum
{
    UART_REGINT_MODE_BLOCKING = 0, /*!< Blocking mode - function waits until complete */
    UART_REGINT_MODE_CALLBACK,     /*!< Callback mode - function returns immediately, callback invoked on completion */
} UART_RegInt_Mode;

/*!
 *  @brief      UART read return modes
 */
typedef enum
{
    UART_REGINT_ReadReturnMode_FULL = 0, /*!< Return only when buffer is full */
    UART_REGINT_ReadReturnMode_PARTIAL,  /*!< Return on timeout even if buffer not full */
} UART_RegInt_ReadReturnMode;

/*!
 *  @brief      UART data length
 */
typedef enum
{
    UART_REGINT_DataLen_5 = 0, /*!< 5 data bits */
    UART_REGINT_DataLen_6,     /*!< 6 data bits */
    UART_REGINT_DataLen_7,     /*!< 7 data bits */
    UART_REGINT_DataLen_8,     /*!< 8 data bits (most common) */
} UART_RegInt_DataLen;

/*!
 *  @brief      UART stop bits
 */
typedef enum
{
    UART_REGINT_StopBits_1 = 0, /*!< 1 stop bit */
    UART_REGINT_StopBits_2,     /*!< 2 stop bits */
} UART_RegInt_StopBits;

/*!
 *  @brief      UART parity type
 */
typedef enum
{
    UART_REGINT_Parity_NONE = 0, /*!< No parity */
    UART_REGINT_Parity_EVEN,     /*!< Even parity */
    UART_REGINT_Parity_ODD,      /*!< Odd parity */
    UART_REGINT_Parity_ZERO,     /*!< Parity bit always 0 */
    UART_REGINT_Parity_ONE,      /*!< Parity bit always 1 */
} UART_RegInt_Parity;

/*!
 *  @brief      Callback function invoked on read/write completion
 *
 *  @param[in]  handle      UART handle
 *  @param[in]  buf         Pointer to buffer
 *  @param[in]  count       Number of bytes transferred
 *  @param[in]  userArg     User argument
 *  @param[in]  status      Status code (UART_REGINT_STATUS_SUCCESS or error)
 */
typedef void (*UART_RegInt_Callback)(UART_RegInt_Handle handle,
                                     void *buf,
                                     size_t count,
                                     void *userArg,
                                     int_fast16_t status);

/*!
 *  @brief      Callback function invoked on UART events (errors, etc.)
 *
 *  @param[in]  handle      UART handle
 *  @param[in]  event       Event flags (UART_REGINT_EVENT_*)
 *  @param[in]  data        Event-specific data
 *  @param[in]  userArg     User argument
 */
typedef void (*UART_RegInt_EventCallback)(UART_RegInt_Handle handle,
                                          uint32_t event,
                                          uint32_t data,
                                          void *userArg);

/*!
 *  @brief      UART parameters
 *
 *  These parameters are used with UART_RegInt_open() to configure the UART
 *  instance.
 */
typedef struct
{
    UART_RegInt_Mode readMode;              /*!< Read operation mode */
    UART_RegInt_Mode writeMode;             /*!< Write operation mode */
    UART_RegInt_Callback readCallback;      /*!< Read completion callback */
    UART_RegInt_Callback writeCallback;     /*!< Write completion callback */
    UART_RegInt_EventCallback eventCallback; /*!< Event callback for errors */
    uint32_t eventMask;                     /*!< Event mask (which events to report) */
    UART_RegInt_ReadReturnMode readReturnMode; /*!< Read return behavior */
    uint32_t baudRate;                      /*!< Baud rate (e.g., 9600, 19200) */
    UART_RegInt_DataLen dataLength;         /*!< Data length (bits per character) */
    UART_RegInt_StopBits stopBits;          /*!< Number of stop bits */
    UART_RegInt_Parity parityType;          /*!< Parity type */
    void *userArg;                          /*!< User argument passed to callbacks */
} UART_RegInt_Params;

/*!
 *  @brief      UART configuration structure
 *
 *  This structure contains the hardware and software configuration for a
 *  UART instance. It is typically defined in the board configuration file.
 */
typedef struct UART_RegInt_Config_
{
    void *object;    /*!< Pointer to UART object (device-specific) */
    void const *hwAttrs; /*!< Pointer to hardware attributes (device-specific) */
} UART_RegInt_Config;

/*!
 *  @brief      Close a UART instance
 *
 *  Closes the UART instance and releases all resources. Any pending transactions
 *  are cancelled.
 *
 *  @param[in]  handle      UART handle returned from UART_RegInt_open()
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern void UART_RegInt_close(UART_RegInt_Handle handle);

/*!
 *  @brief      Initialize the UART driver module
 *
 *  This function must be called once before any other UART_RegInt functions.
 *  Typically called from the main() function before BIOS_start().
 */
extern void UART_RegInt_init(void);

/*!
 *  @brief      Open a UART instance
 *
 *  Opens a UART instance with the specified parameters.
 *
 *  @param[in]  index       Index in the UART_RegInt_config array
 *  @param[in]  params      Pointer to parameters, or NULL for default
 *
 *  @return     UART_RegInt_Handle on success, NULL on error
 *
 *  @pre        UART_RegInt_init() has been called
 */
extern UART_RegInt_Handle UART_RegInt_open(uint_least8_t index,
                                           UART_RegInt_Params *params);

/*!
 *  @brief      Initialize UART parameters to default values
 *
 *  @param[out] params      Pointer to parameters structure
 *
 *  Default values:
 *  - readMode: UART_REGINT_MODE_BLOCKING
 *  - writeMode: UART_REGINT_MODE_BLOCKING
 *  - readCallback: NULL
 *  - writeCallback: NULL
 *  - eventCallback: NULL
 *  - eventMask: 0
 *  - readReturnMode: UART_REGINT_ReadReturnMode_FULL
 *  - baudRate: 9600
 *  - dataLength: UART_REGINT_DataLen_8
 *  - stopBits: UART_REGINT_StopBits_1
 *  - parityType: UART_REGINT_Parity_NONE
 *  - userArg: NULL
 */
extern void UART_RegInt_Params_init(UART_RegInt_Params *params);

/*!
 *  @brief      Read data from UART
 *
 *  Reads data from the UART. Behavior depends on the read mode:
 *  - BLOCKING: Blocks until the requested bytes are received
 *  - CALLBACK: Returns immediately, readCallback invoked on completion
 *
 *  @param[in]  handle      UART handle
 *  @param[out] buf         Buffer to store received data
 *  @param[in]  size        Number of bytes to read
 *
 *  @return     BLOCKING mode: Number of bytes read, or negative error code
 *              CALLBACK mode: UART_REGINT_STATUS_SUCCESS (0) or negative error code
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern int_fast16_t UART_RegInt_read(UART_RegInt_Handle handle,
                                     void *buf,
                                     size_t size);

/*!
 *  @brief      Read data from UART with timeout
 *
 *  Same as UART_RegInt_read() but with timeout support.
 *
 *  @param[in]  handle      UART handle
 *  @param[out] buf         Buffer to store received data
 *  @param[in]  size        Number of bytes to read
 *  @param[in]  timeout     Timeout in microseconds, UART_REGINT_WAIT_FOREVER for no timeout
 *
 *  @return     Number of bytes read, or negative error code
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern int_fast16_t UART_RegInt_readTimeout(UART_RegInt_Handle handle,
                                            void *buf,
                                            size_t size,
                                            uint32_t timeout);

/*!
 *  @brief      Cancel ongoing read operation
 *
 *  Cancels the current read operation. The read callback (if set) will be
 *  invoked with status UART_REGINT_STATUS_ECANCELLED.
 *
 *  @param[in]  handle      UART handle
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern void UART_RegInt_readCancel(UART_RegInt_Handle handle);

/*!
 *  @brief      Write data to UART
 *
 *  Writes data to the UART. Behavior depends on the write mode:
 *  - BLOCKING: Blocks until all data is transmitted
 *  - CALLBACK: Returns immediately, writeCallback invoked on completion
 *
 *  @param[in]  handle      UART handle
 *  @param[in]  buf         Buffer containing data to transmit
 *  @param[in]  size        Number of bytes to write
 *
 *  @return     BLOCKING mode: Number of bytes written, or negative error code
 *              CALLBACK mode: UART_REGINT_STATUS_SUCCESS (0) or negative error code
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern int_fast16_t UART_RegInt_write(UART_RegInt_Handle handle,
                                      const void *buf,
                                      size_t size);

/*!
 *  @brief      Write data to UART with timeout
 *
 *  Same as UART_RegInt_write() but with timeout support.
 *
 *  @param[in]  handle      UART handle
 *  @param[in]  buf         Buffer containing data to transmit
 *  @param[in]  size        Number of bytes to write
 *  @param[in]  timeout     Timeout in microseconds, UART_REGINT_WAIT_FOREVER for no timeout
 *
 *  @return     Number of bytes written, or negative error code
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern int_fast16_t UART_RegInt_writeTimeout(UART_RegInt_Handle handle,
                                             const void *buf,
                                             size_t size,
                                             uint32_t timeout);

/*!
 *  @brief      Cancel ongoing write operation
 *
 *  Cancels the current write operation. The write callback (if set) will be
 *  invoked with status UART_REGINT_STATUS_ECANCELLED.
 *
 *  @param[in]  handle      UART handle
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern void UART_RegInt_writeCancel(UART_RegInt_Handle handle);

/*!
 *  @brief      Enable UART receive
 *
 *  Enables the receive function and sets power constraints to prevent standby.
 *
 *  @param[in]  handle      UART handle
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern void UART_RegInt_rxEnable(UART_RegInt_Handle handle);

/*!
 *  @brief      Disable UART receive
 *
 *  Disables the receive function and releases power constraints.
 *
 *  @param[in]  handle      UART handle
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern void UART_RegInt_rxDisable(UART_RegInt_Handle handle);

/*!
 *  @brief      Get receive overrun count
 *
 *  Returns the cumulative count of receive overrun errors since the UART
 *  was opened.
 *
 *  @param[in]  handle      UART handle
 *
 *  @return     Overrun error count
 *
 *  @pre        UART_RegInt_open() called successfully
 */
extern uint32_t UART_RegInt_getOverrunCount(UART_RegInt_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_UART_RegInt__include */
