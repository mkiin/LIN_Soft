/*
 * Copyright (c) 2025, Custom Implementation
 * Based on Texas Instruments UART2LPF3 driver structure
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
 *  @file       UART_RegIntLPF3.h
 *
 *  @brief      UART_RegInt driver implementation for CC2340R53 (LPF3)
 *
 *  # Overview
 *
 *  This is the device-specific implementation of the UART_RegInt driver for
 *  the Low Power F3 family (CC23X0R5). It uses register-level interrupt
 *  handling without DMA.
 *
 *  ## Hardware Features
 *  - 8-byte TX/RX FIFOs
 *  - Programmable baud rate generator
 *  - Hardware flow control support (CTS/RTS)
 *  - Error detection (overrun, framing, parity, break)
 *  - Low power mode integration
 *
 ******************************************************************************
 */

#ifndef ti_drivers_uart_regint_UART_RegIntLPF3__include
#define ti_drivers_uart_regint_UART_RegIntLPF3__include

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_types.h)

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>

#include "../UART_RegInt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! Size of the TX and RX FIFOs */
#define UART_REGINT_LPF3_FIFO_SIZE 8

/*!
 *  @brief      Base object to be included in UART_RegIntLPF3_Object
 *
 *  This macro should be used to define the common object fields
 */
#define UART_REGINT_BASE_OBJECT                                                \
    /* User configuration */                                                   \
    UART_RegInt_Params params;                                                 \
                                                                               \
    /* State flags */                                                          \
    struct {                                                                   \
        bool opened       : 1;  /* Driver instance opened */                  \
        bool rxEnabled    : 1;  /* Receive enabled */                         \
        bool txEnabled    : 1;  /* Transmit enabled */                        \
        bool rxInProgress : 1;  /* Read operation in progress */              \
        bool txInProgress : 1;  /* Write operation in progress */             \
        bool rxCancelled  : 1;  /* Read operation cancelled */                \
        bool txCancelled  : 1;  /* Write operation cancelled */               \
    } state;                                                                   \
                                                                               \
    /* Hardware interrupt object */                                           \
    HwiP_Struct hwi;                                                          \
                                                                               \
    /* Synchronization objects */                                             \
    SemaphoreP_Struct readSem;     /* Read semaphore (blocking mode) */       \
    SemaphoreP_Struct writeSem;    /* Write semaphore (blocking mode) */      \
                                                                               \
    /* Read transaction state */                                              \
    uint8_t *readBuf;              /* Read buffer pointer */                  \
    size_t readSize;               /* Requested read size */                  \
    size_t readCount;              /* Bytes read so far */                    \
    int_fast16_t readStatus;       /* Read status code */                     \
                                                                               \
    /* Write transaction state */                                             \
    const uint8_t *writeBuf;       /* Write buffer pointer */                 \
    size_t writeSize;              /* Requested write size */                 \
    size_t writeCount;             /* Bytes written so far */                 \
    int_fast16_t writeStatus;      /* Write status code */                    \
                                                                               \
    /* Error tracking */                                                      \
    uint32_t overrunCount;         /* Cumulative overrun error count */

/*!
 *  @brief      Base hardware attributes to be included in UART_RegIntLPF3_HWAttrs
 *
 *  This macro should be used to define the common hardware attributes
 */
#define UART_REGINT_BASE_HWATTRS                                               \
    uint32_t baseAddr;             /* UART peripheral base address */         \
    int intNum;                    /* UART interrupt number */                \
    uint8_t intPriority;           /* Interrupt priority (0xFF = default) */  \
                                                                               \
    /* Ring buffer for received data (optional) */                            \
    unsigned char *rxBufPtr;       /* RX ring buffer pointer */               \
    size_t rxBufSize;              /* RX ring buffer size */                  \
                                                                               \
    /* Transmit buffer (optional) */                                          \
    unsigned char *txBufPtr;       /* TX buffer pointer */                    \
    size_t txBufSize;              /* TX buffer size */                       \
                                                                               \
    /* GPIO pin configuration */                                              \
    uint32_t flowControl;          /* Flow control mode */                    \
    uint32_t rxPin;                /* RX GPIO pin index */                    \
    uint32_t txPin;                /* TX GPIO pin index */                    \
    uint32_t ctsPin;               /* CTS GPIO pin index (flow control) */    \
    uint32_t rtsPin;               /* RTS GPIO pin index (flow control) */

/*!
 *  @brief      UART_RegIntLPF3 Hardware Attributes
 *
 *  The fields, baseAddr and intNum are used by driverlib APIs and therefore
 *  must be populated by driverlib macro definitions. These definitions are
 *  found under the device family in:
 *      - inc/hw_memmap.h
 *      - inc/hw_ints.h
 *      - driverlib/uart.h
 *
 *  intPriority is the UART peripheral's interrupt priority, as defined by the
 *  underlying OS.
 *
 *  Setting the priority to 0 is not supported by this driver as it uses the
 *  HWI dispatcher and requires critical sections to work correctly.
 *
 *  A sample structure is shown below:
 *  @code
 *  const UART_RegIntLPF3_HWAttrs UART_RegIntLPF3_hwAttrs[] = {
 *      {
 *           .baseAddr              = UART0_BASE,
 *           .intNum                = INT_UART0_COMB,
 *           .intPriority           = 0x20,
 *           .rxPin                 = CONFIG_GPIO_UART_0_RX,
 *           .txPin                 = CONFIG_GPIO_UART_0_TX,
 *           .ctsPin                = GPIO_INVALID_INDEX,
 *           .rtsPin                = GPIO_INVALID_INDEX,
 *           .flowControl           = UART_REGINT_FLOWCTRL_NONE,
 *           .txPinMux              = GPIO_MUX_PORTCFG_PFUNC3,
 *           .rxPinMux              = GPIO_MUX_PORTCFG_PFUNC3,
 *           .ctsPinMux             = GPIO_MUX_GPIO_INTERNAL,
 *           .rtsPinMux             = GPIO_MUX_GPIO_INTERNAL,
 *           .powerID               = PowerLPF3_PERIPH_UART0,
 *      },
 *  };
 *  @endcode
 */
typedef struct
{
    UART_REGINT_BASE_HWATTRS

    /* Pin multiplexing configuration */
    int32_t txPinMux;              /*!< TX pin mux value */
    int32_t rxPinMux;              /*!< RX pin mux value */
    int32_t ctsPinMux;             /*!< CTS pin mux value for flow control */
    int32_t rtsPinMux;             /*!< RTS pin mux value for flow control */

    /* Power management */
    uint8_t powerID;               /*!< Power resource ID for this UART instance */
} UART_RegIntLPF3_HWAttrs;

/*!
 *  @brief      UART_RegIntLPF3 Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct
{
    UART_REGINT_BASE_OBJECT

    /* Power management notification objects */
    Power_NotifyObj preNotify;     /*!< For configuring IO pins before entering standby */
    Power_NotifyObj postNotify;    /*!< For configuring IO pins after returning from standby */
} UART_RegIntLPF3_Object, *UART_RegIntLPF3_Handle;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_uart_regint_UART_RegIntLPF3__include */
