/*
 * Copyright (c) 2025, Custom Implementation
 * Based on Texas Instruments UART2Support structure
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
 *  @file       UART_RegIntSupport.h
 *
 *  @brief      Internal helper functions for UART_RegInt driver
 *
 *  # Overview
 *
 *  This file contains internal support functions used by the UART_RegInt
 *  driver implementation. These functions are not part of the public API
 *  and should not be called directly by applications.
 *
 ******************************************************************************
 */

#ifndef ti_drivers_uart_regint_UART_RegIntSupport__include
#define ti_drivers_uart_regint_UART_RegIntSupport__include

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../UART_RegInt.h"
#include "UART_RegIntLPF3.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  @brief  Function to enable receive and error interrupts
 *
 *  Enables the RX, RX timeout, and error interrupts for the UART.
 *
 *  @param[in]  handle    A UART_RegInt_Handle returned from UART_RegInt_open()
 */
extern void UART_RegIntSupport_enableInts(UART_RegInt_Handle handle);

/*!
 *  @brief  Function to disable the receive and receive interrupts
 *
 *  Disables the RX and RX timeout interrupts for the UART.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 */
extern void UART_RegIntSupport_disableRx(const UART_RegIntLPF3_HWAttrs *hwAttrs);

/*!
 *  @brief  Function to disable the transmit interrupt
 *
 *  Disables the TX interrupt for the UART.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 */
extern void UART_RegIntSupport_disableTx(const UART_RegIntLPF3_HWAttrs *hwAttrs);

/*!
 *  @brief  Function to enable the receive
 *
 *  Enables the receive function by setting the RXE control bit.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 */
extern void UART_RegIntSupport_enableRx(const UART_RegIntLPF3_HWAttrs *hwAttrs);

/*!
 *  @brief  Function to enable the transmit interrupt
 *
 *  Enables the TX interrupt for the UART.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 */
extern void UART_RegIntSupport_enableTx(const UART_RegIntLPF3_HWAttrs *hwAttrs);

/*!
 *  @brief  Function to send data
 *
 *  Writes data directly to the UART FIFO. This function writes as much data
 *  as possible without blocking.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 *  @param[in]  size       The number of bytes in the buffer that should be
 *                         written to the UART
 *  @param[in]  buf        A pointer to buffer containing data to be written
 *                         to the UART
 *
 *  @return Returns the number of bytes written
 */
extern uint32_t UART_RegIntSupport_sendData(const UART_RegIntLPF3_HWAttrs *hwAttrs,
                                            size_t size,
                                            const uint8_t *buf);

/*!
 *  @brief  Function to determine if TX is in progress
 *
 *  Checks if the UART is currently transmitting data.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 *
 *  @return Returns true if there is no TX in progress, otherwise false
 */
extern bool UART_RegIntSupport_txDone(const UART_RegIntLPF3_HWAttrs *hwAttrs);

/*!
 *  @brief  Function to clear receive errors
 *
 *  Reads and clears the UART error status register.
 *
 *  @param[in]  hwAttrs    A pointer to a UART_RegIntLPF3_HWAttrs structure
 *
 *  @return Returns a status indicating success or failure of the read.
 *
 *  @retval UART_REGINT_STATUS_SUCCESS  No errors.
 *  @retval UART_REGINT_STATUS_EOVERRUN A FIFO overrun occurred.
 *  @retval UART_REGINT_STATUS_EFRAMING A framing error occurred.
 *  @retval UART_REGINT_STATUS_EBREAK   A break error occurred.
 *  @retval UART_REGINT_STATUS_EPARITY  A parity error occurred.
 */
extern int UART_RegIntSupport_uartRxError(const UART_RegIntLPF3_HWAttrs *hwAttrs);

/*!
 *  @brief  Function to convert RX error status to a UART_RegInt error code
 *
 *  Converts raw error status bits to UART_RegInt status codes.
 *
 *  @param[in]  errorData    Data indicating the UART RX error status
 *
 *  @return Returns a status indicating the type of error.
 *
 *  @retval UART_REGINT_STATUS_SUCCESS  No errors.
 *  @retval UART_REGINT_STATUS_EOVERRUN A FIFO overrun occurred.
 *  @retval UART_REGINT_STATUS_EFRAMING A framing error occurred.
 *  @retval UART_REGINT_STATUS_EBREAK   A break error occurred.
 *  @retval UART_REGINT_STATUS_EPARITY  A parity error occurred.
 */
extern int_fast16_t UART_RegIntSupport_rxStatus2ErrorCode(uint32_t errorData);

/*!
 *  @brief  Function to set power constraints
 *
 *  Sets power constraints to prevent the device from entering low power modes
 *  that would interfere with UART operation.
 *
 *  @param[in]  handle    A UART_RegInt_Handle returned from UART_RegInt_open()
 */
extern void UART_RegIntSupport_powerSetConstraint(UART_RegInt_Handle handle);

/*!
 *  @brief  Function to release power constraints
 *
 *  Releases power constraints previously set by UART_RegIntSupport_powerSetConstraint().
 *
 *  @param[in]  handle    A UART_RegInt_Handle returned from UART_RegInt_open()
 */
extern void UART_RegIntSupport_powerRelConstraint(UART_RegInt_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_uart_regint_UART_RegIntSupport__include */
