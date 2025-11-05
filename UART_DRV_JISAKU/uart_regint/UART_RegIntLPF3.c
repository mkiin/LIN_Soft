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

/*
 *  ======== UART_RegIntLPF3.c ========
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>

#include <ti/drivers/uart_regint/UART_RegIntLPF3.h>
#include <ti/drivers/uart_regint/UART_RegIntSupport.h>

/* driverlib header files */
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_uart.h)
#include DeviceFamily_constructPath(driverlib/uart.h)

/* FIFO trigger levels */
#define TX_FIFO_INT_LEVEL UART_FIFO_TX2_8  /* Trigger when FIFO is 2/8 full */
#define RX_FIFO_INT_LEVEL UART_FIFO_RX6_8  /* Trigger when FIFO is 6/8 full */

/* Map UART_RegInt data length to driverlib data length */
static const uint8_t dataLength[] = {
    UART_CONFIG_WLEN_5, /* UART_REGINT_DataLen_5 */
    UART_CONFIG_WLEN_6, /* UART_REGINT_DataLen_6 */
    UART_CONFIG_WLEN_7, /* UART_REGINT_DataLen_7 */
    UART_CONFIG_WLEN_8  /* UART_REGINT_DataLen_8 */
};

/* Map UART_RegInt stop bits to driverlib stop bits */
static const uint8_t stopBits[] = {
    UART_CONFIG_STOP_ONE, /* UART_REGINT_StopBits_1 */
    UART_CONFIG_STOP_TWO  /* UART_REGINT_StopBits_2 */
};

/* Map UART_RegInt parity type to driverlib parity type */
static const uint8_t parityType[] = {
    UART_CONFIG_PAR_NONE, /* UART_REGINT_Parity_NONE */
    UART_CONFIG_PAR_EVEN, /* UART_REGINT_Parity_EVEN */
    UART_CONFIG_PAR_ODD,  /* UART_REGINT_Parity_ODD */
    UART_CONFIG_PAR_ZERO, /* UART_REGINT_Parity_ZERO */
    UART_CONFIG_PAR_ONE   /* UART_REGINT_Parity_ONE */
};

/* Static function prototypes */
static void UART_RegIntLPF3_hwiIntFxn(uintptr_t arg);
static void UART_RegIntLPF3_hwiIntRead(uintptr_t arg, uint32_t status);
static void UART_RegIntLPF3_hwiIntWrite(uintptr_t arg);
static void UART_RegIntLPF3_initHw(UART_RegInt_Handle handle);
static void UART_RegIntLPF3_initIO(UART_RegInt_Handle handle);
static void UART_RegIntLPF3_finalizeIO(UART_RegInt_Handle handle);
static int UART_RegIntLPF3_postNotifyFxn(unsigned int eventType, 
                                         uintptr_t eventArg, 
                                         uintptr_t clientArg);

/*
 *  ======== UART_RegInt_close ========
 */
void UART_RegInt_close(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uintptr_t key;

    key = HwiP_disable();

    if (!object->state.opened)
    {
        HwiP_restore(key);
        return;
    }

    /* Cancel any pending transactions */
    if (object->state.rxInProgress)
    {
        UART_RegInt_readCancel(handle);
    }
    if (object->state.txInProgress)
    {
        UART_RegInt_writeCancel(handle);
    }

    /* Disable all UART interrupts */
    UARTDisableInt(hwAttrs->baseAddr,
                   UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE |
                   UART_INT_RT | UART_INT_TX | UART_INT_RX);

    /* Disable UART */
    UARTDisable(hwAttrs->baseAddr);

    /* Unregister power notification */
    Power_unregisterNotify(&object->postNotify);

    /* Delete HWI */
    HwiP_destruct(&object->hwi);

    /* Release power dependency */
    Power_releaseDependency(hwAttrs->powerID);

    /* Restore pins to default state */
    UART_RegIntLPF3_finalizeIO(handle);

    /* Mark as closed */
    object->state.opened = false;

    HwiP_restore(key);
}

/*
 *  ======== UART_RegInt_open ========
 */
UART_RegInt_Handle UART_RegInt_open(uint_least8_t index, UART_RegInt_Params *params)
{
    UART_RegInt_Handle handle;
    UART_RegInt_Config *config;
    UART_RegIntLPF3_Object *object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs;
    HwiP_Params hwiParams;
    uintptr_t key;

    /* Check index */
    if (index >= UART_RegInt_count)
    {
        return NULL;
    }

    /* Get handle */
    handle = (UART_RegInt_Handle)&UART_RegInt_config[index];
    config = (UART_RegInt_Config *)handle;
    object = (UART_RegIntLPF3_Object *)config->object;
    hwAttrs = (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;

    key = HwiP_disable();

    /* Check if already opened */
    if (object->state.opened)
    {
        HwiP_restore(key);
        return NULL;
    }

    /* Mark as opened */
    object->state.opened = true;

    HwiP_restore(key);

    /* Initialize object with parameters */
    if (params == NULL)
    {
        params = (UART_RegInt_Params *)&UART_RegInt_defaultParams;
    }
    object->params = *params;

    /* Initialize state */
    object->state.rxEnabled = false;
    object->state.txEnabled = false;
    object->state.rxInProgress = false;
    object->state.txInProgress = false;
    object->state.rxCancelled = false;
    object->state.txCancelled = false;
    object->overrunCount = 0;

    /* Initialize transaction state */
    object->readBuf = NULL;
    object->readSize = 0;
    object->readCount = 0;
    object->readStatus = UART_REGINT_STATUS_SUCCESS;
    object->writeBuf = NULL;
    object->writeSize = 0;
    object->writeCount = 0;
    object->writeStatus = UART_REGINT_STATUS_SUCCESS;

    /* Create semaphores */
    SemaphoreP_constructBinary(&object->readSem, 0);
    SemaphoreP_constructBinary(&object->writeSem, 0);

    /* Set power dependency */
    Power_setDependency(hwAttrs->powerID);

    /* Initialize GPIO pins */
    UART_RegIntLPF3_initIO(handle);

    /* Initialize UART hardware */
    UART_RegIntLPF3_initHw(handle);

    /* Create hardware interrupt */
    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = (hwAttrs->intPriority == 0xFF) ? 0x20 : hwAttrs->intPriority;
    
    if (HwiP_construct(&object->hwi, hwAttrs->intNum, 
                       UART_RegIntLPF3_hwiIntFxn, &hwiParams) == NULL)
    {
        UART_RegInt_close(handle);
        return NULL;
    }

    /* Register power post-notify callback */
    Power_registerNotify(&object->postNotify,
                        PowerLPF3_AWAKE_STANDBY,
                        UART_RegIntLPF3_postNotifyFxn,
                        (uintptr_t)handle);

    return handle;
}

/*
 *  ======== UART_RegIntLPF3_initHw ========
 *  Initialize UART hardware
 */
static void UART_RegIntLPF3_initHw(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    ClockP_FreqHz freq;
    uint32_t config_value;

    /* Disable UART */
    UARTDisable(hwAttrs->baseAddr);

    /* Get CPU clock frequency */
    ClockP_getCpuFreq(&freq);

    /* Configure baud rate and data format */
    config_value = dataLength[object->params.dataLength] |
                   stopBits[object->params.stopBits] |
                   parityType[object->params.parityType];

    UARTConfigSetExpClk(hwAttrs->baseAddr,
                        freq.lo,
                        object->params.baudRate,
                        config_value);

    /* Enable FIFO */
    UARTEnableFifo(hwAttrs->baseAddr);

    /* Set FIFO trigger levels */
    UARTSetFifoLevel(hwAttrs->baseAddr, TX_FIFO_INT_LEVEL, RX_FIFO_INT_LEVEL);

    /* Clear all interrupts */
    UARTClearInt(hwAttrs->baseAddr,
                 UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE |
                 UART_INT_RT | UART_INT_TX | UART_INT_RX);

    /* Enable UART */
    HWREG(hwAttrs->baseAddr + UART_O_CTL) |= UART_CTL_UARTEN;

    /* Enable TX */
    HWREG(hwAttrs->baseAddr + UART_O_CTL) |= UART_CTL_TXE;
}

/*
 *  ======== UART_RegIntLPF3_initIO ========
 *  Initialize GPIO pins
 */
static void UART_RegIntLPF3_initIO(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;

    /* Configure TX pin */
    GPIO_setConfigAndMux(hwAttrs->txPin, GPIO_CFG_NO_DIR, hwAttrs->txPinMux);

    /* Configure RX pin */
    GPIO_setConfigAndMux(hwAttrs->rxPin, GPIO_CFG_INPUT, hwAttrs->rxPinMux);

    /* Configure flow control pins if used */
    if (hwAttrs->flowControl == UART_REGINT_FLOWCTRL_HARDWARE)
    {
        if (hwAttrs->ctsPin != GPIO_INVALID_INDEX)
        {
            GPIO_setConfigAndMux(hwAttrs->ctsPin, GPIO_CFG_INPUT, hwAttrs->ctsPinMux);
        }
        if (hwAttrs->rtsPin != GPIO_INVALID_INDEX)
        {
            GPIO_setConfigAndMux(hwAttrs->rtsPin, GPIO_CFG_NO_DIR, hwAttrs->rtsPinMux);
        }
    }
}

/*
 *  ======== UART_RegIntLPF3_finalizeIO ========
 *  Restore GPIO pins to default state
 */
static void UART_RegIntLPF3_finalizeIO(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;

    /* Reset pins to GPIO mode */
    GPIO_resetConfig(hwAttrs->txPin);
    GPIO_resetConfig(hwAttrs->rxPin);

    if (hwAttrs->flowControl == UART_REGINT_FLOWCTRL_HARDWARE)
    {
        if (hwAttrs->ctsPin != GPIO_INVALID_INDEX)
        {
            GPIO_resetConfig(hwAttrs->ctsPin);
        }
        if (hwAttrs->rtsPin != GPIO_INVALID_INDEX)
        {
            GPIO_resetConfig(hwAttrs->rtsPin);
        }
    }
}

/*
 *  ======== UART_RegIntLPF3_postNotifyFxn ========
 *  Power notification callback after waking from standby
 */
static int UART_RegIntLPF3_postNotifyFxn(unsigned int eventType,
                                         uintptr_t eventArg,
                                         uintptr_t clientArg)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)clientArg;

    /* Re-initialize hardware after waking from standby */
    if (eventType == PowerLPF3_AWAKE_STANDBY)
    {
        UART_RegIntLPF3_initHw(handle);
    }

    return Power_NOTIFYDONE;
}

/*
 *  ======== UART_RegIntLPF3_hwiIntFxn ========
 *  Main interrupt handler
 */
static void UART_RegIntLPF3_hwiIntFxn(uintptr_t arg)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)arg;
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uint32_t intStatus;
    uint32_t errStatus;

    /* Get and clear interrupt status */
    intStatus = UARTIntStatus(hwAttrs->baseAddr, true);
    UARTClearInt(hwAttrs->baseAddr, intStatus);

    /* Handle RX interrupt or RX timeout */
    if (intStatus & (UART_INT_RX | UART_INT_RT))
    {
        UART_RegIntLPF3_hwiIntRead(arg, intStatus);
    }

    /* Handle TX interrupt */
    if (intStatus & UART_INT_TX)
    {
        UART_RegIntLPF3_hwiIntWrite(arg);
    }

    /* Handle error interrupts */
    if (intStatus & (UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE))
    {
        UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
        
        /* Read error status */
        errStatus = UARTGetRxError(hwAttrs->baseAddr);
        
        /* Update overrun count */
        if (errStatus & UART_RXERROR_OVERRUN)
        {
            object->overrunCount++;
        }
        
        /* Clear error */
        UARTClearRxError(hwAttrs->baseAddr);
        
        /* Invoke event callback if registered */
        if (object->params.eventCallback != NULL)
        {
            uint32_t events = 0;
            
            if (errStatus & UART_RXERROR_OVERRUN)
                events |= UART_REGINT_EVENT_OVERRUN;
            if (errStatus & UART_RXERROR_BREAK)
                events |= UART_REGINT_EVENT_BREAK;
            if (errStatus & UART_RXERROR_PARITY)
                events |= UART_REGINT_EVENT_PARITY;
            if (errStatus & UART_RXERROR_FRAMING)
                events |= UART_REGINT_EVENT_FRAMING;
            
            if (events & object->params.eventMask)
            {
                object->params.eventCallback(handle, events, errStatus, 
                                           object->params.userArg);
            }
        }
    }
}

/*
 *  ======== UART_RegIntLPF3_hwiIntRead ========
 *  RX interrupt handler
 */
static void UART_RegIntLPF3_hwiIntRead(uintptr_t arg, uint32_t status)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)arg;
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uint32_t errStatus;
    uint8_t data;

    if (!object->state.rxInProgress)
    {
        return;
    }

    /* Read data from FIFO */
    while (UARTCharAvailable(hwAttrs->baseAddr) && 
           (object->readCount < object->readSize))
    {
        /* Check for errors before reading */
        errStatus = HWREG(hwAttrs->baseAddr + UART_O_RSR_ECR);
        
        /* Read data */
        data = (uint8_t)HWREG(hwAttrs->baseAddr + UART_O_DR);
        
        /* Check for errors */
        if (errStatus & (UART_RSR_ECR_OE | UART_RSR_ECR_BE | 
                        UART_RSR_ECR_PE | UART_RSR_ECR_FE))
        {
            object->readStatus = UART_RegIntSupport_rxStatus2ErrorCode(errStatus);
            UARTClearRxError(hwAttrs->baseAddr);
            break;
        }
        
        /* Store data */
        object->readBuf[object->readCount] = data;
        object->readCount++;
    }

    /* Check if read complete or timeout occurred */
    if ((object->readCount >= object->readSize) || (status & UART_INT_RT))
    {
        /* Disable RX interrupts */
        UART_RegIntSupport_disableRx(hwAttrs);
        HWREG(hwAttrs->baseAddr + UART_O_CTL) &= ~UART_CTL_RXE;
        
        /* Clear in-progress flag */
        object->state.rxInProgress = false;
        
        /* Notify or callback */
        if (object->params.readMode == UART_REGINT_MODE_BLOCKING)
        {
            SemaphoreP_post(&object->readSem);
        }
        else if (object->params.readCallback != NULL)
        {
            object->params.readCallback(handle,
                                       object->readBuf,
                                       object->readCount,
                                       object->params.userArg,
                                       object->readStatus);
        }
    }
}

/*
 *  ======== UART_RegIntLPF3_hwiIntWrite ========
 *  TX interrupt handler
 */
static void UART_RegIntLPF3_hwiIntWrite(uintptr_t arg)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)arg;
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;

    if (!object->state.txInProgress)
    {
        return;
    }

    /* Write data to FIFO */
    while (UARTSpaceAvailable(hwAttrs->baseAddr) && 
           (object->writeCount < object->writeSize))
    {
        HWREG(hwAttrs->baseAddr + UART_O_DR) = object->writeBuf[object->writeCount];
        object->writeCount++;
    }

    /* Check if write complete */
    if (object->writeCount >= object->writeSize)
    {
        /* Disable TX interrupt */
        UART_RegIntSupport_disableTx(hwAttrs);
        
        /* Clear in-progress flag */
        object->state.txInProgress = false;
        
        /* Notify or callback */
        if (object->params.writeMode == UART_REGINT_MODE_BLOCKING)
        {
            SemaphoreP_post(&object->writeSem);
        }
        else if (object->params.writeCallback != NULL)
        {
            object->params.writeCallback(handle,
                                        (void *)object->writeBuf,
                                        object->writeCount,
                                        object->params.userArg,
                                        object->writeStatus);
        }
    }
}

/*
 *  ======== Support function implementations ========
 */

void UART_RegIntSupport_enableInts(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;

    UARTEnableInt(hwAttrs->baseAddr,
                  UART_INT_RX | UART_INT_RT | 
                  UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE);
}

void UART_RegIntSupport_disableRx(const UART_RegIntLPF3_HWAttrs *hwAttrs)
{
    UARTDisableInt(hwAttrs->baseAddr, UART_INT_RX | UART_INT_RT);
}

void UART_RegIntSupport_disableTx(const UART_RegIntLPF3_HWAttrs *hwAttrs)
{
    UARTDisableInt(hwAttrs->baseAddr, UART_INT_TX);
}

void UART_RegIntSupport_enableRx(const UART_RegIntLPF3_HWAttrs *hwAttrs)
{
    HWREG(hwAttrs->baseAddr + UART_O_CTL) |= UART_CTL_RXE;
}

void UART_RegIntSupport_enableTx(const UART_RegIntLPF3_HWAttrs *hwAttrs)
{
    UARTEnableInt(hwAttrs->baseAddr, UART_INT_TX);
}

uint32_t UART_RegIntSupport_sendData(const UART_RegIntLPF3_HWAttrs *hwAttrs,
                                     size_t size,
                                     const uint8_t *buf)
{
    uint32_t count = 0;

    while ((count < size) && UARTSpaceAvailable(hwAttrs->baseAddr))
    {
        HWREG(hwAttrs->baseAddr + UART_O_DR) = buf[count];
        count++;
    }

    return count;
}

bool UART_RegIntSupport_txDone(const UART_RegIntLPF3_HWAttrs *hwAttrs)
{
    return !(HWREG(hwAttrs->baseAddr + UART_O_FR) & UART_FR_BUSY);
}

int UART_RegIntSupport_uartRxError(const UART_RegIntLPF3_HWAttrs *hwAttrs)
{
    uint32_t errStatus = UARTGetRxError(hwAttrs->baseAddr);
    
    UARTClearRxError(hwAttrs->baseAddr);
    
    return UART_RegIntSupport_rxStatus2ErrorCode(errStatus);
}

int_fast16_t UART_RegIntSupport_rxStatus2ErrorCode(uint32_t errorData)
{
    if (errorData & UART_RXERROR_OVERRUN)
        return UART_REGINT_STATUS_EOVERRUN;
    if (errorData & UART_RXERROR_BREAK)
        return UART_REGINT_STATUS_EBREAK;
    if (errorData & UART_RXERROR_PARITY)
        return UART_REGINT_STATUS_EPARITY;
    if (errorData & UART_RXERROR_FRAMING)
        return UART_REGINT_STATUS_EFRAMING;
    
    return UART_REGINT_STATUS_SUCCESS;
}

void UART_RegIntSupport_powerSetConstraint(UART_RegInt_Handle handle)
{
    Power_setConstraint(PowerLPF3_DISALLOW_STANDBY);
}

void UART_RegIntSupport_powerRelConstraint(UART_RegInt_Handle handle)
{
    Power_releaseConstraint(PowerLPF3_DISALLOW_STANDBY);
}
