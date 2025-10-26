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

/*
 *  ======== UART_RegInt.c ========
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/ClockP.h>

#include "UART_RegInt.h"
#include "uart_regint/UART_RegIntSupport.h"

/* External configuration */
extern const UART_RegInt_Config UART_RegInt_config[];
extern const uint_least8_t UART_RegInt_count;

/* Default UART parameters */
const UART_RegInt_Params UART_RegInt_defaultParams = {
    UART_REGINT_MODE_BLOCKING,          /* readMode */
    UART_REGINT_MODE_BLOCKING,          /* writeMode */
    NULL,                                /* readCallback */
    NULL,                                /* writeCallback */
    NULL,                                /* eventCallback */
    0,                                   /* eventMask */
    UART_REGINT_ReadReturnMode_FULL,    /* readReturnMode */
    9600,                                /* baudRate */
    UART_REGINT_DataLen_8,               /* dataLength */
    UART_REGINT_StopBits_1,              /* stopBits */
    UART_REGINT_Parity_NONE,             /* parityType */
    NULL                                 /* userArg */
};

/* Forward declarations of internal functions */
static int_fast16_t UART_RegInt_readBlocking(UART_RegInt_Handle handle,
                                             void *buf,
                                             size_t size,
                                             uint32_t timeout);

static int_fast16_t UART_RegInt_readCallback(UART_RegInt_Handle handle,
                                             void *buf,
                                             size_t size);

static int_fast16_t UART_RegInt_writeBlocking(UART_RegInt_Handle handle,
                                              const void *buf,
                                              size_t size,
                                              uint32_t timeout);

static int_fast16_t UART_RegInt_writeCallback(UART_RegInt_Handle handle,
                                              const void *buf,
                                              size_t size);

/*
 *  ======== UART_RegInt_close ========
 */
void UART_RegInt_close(UART_RegInt_Handle handle)
{
    /* Device-specific close implementation is required */
    /* This should be implemented in the device-specific file (e.g., UART_RegIntLPF3.c) */
}

/*
 *  ======== UART_RegInt_init ========
 */
void UART_RegInt_init(void)
{
    /* Nothing to do - initialization is done at open time */
}

/*
 *  ======== UART_RegInt_open ========
 */
UART_RegInt_Handle UART_RegInt_open(uint_least8_t index, UART_RegInt_Params *params)
{
    /* Device-specific open implementation is required */
    /* This should be implemented in the device-specific file (e.g., UART_RegIntLPF3.c) */
    return NULL;
}

/*
 *  ======== UART_RegInt_Params_init ========
 */
void UART_RegInt_Params_init(UART_RegInt_Params *params)
{
    *params = UART_RegInt_defaultParams;
}

/*
 *  ======== UART_RegInt_read ========
 */
int_fast16_t UART_RegInt_read(UART_RegInt_Handle handle, void *buf, size_t size)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;

    if (!object->state.opened)
    {
        return UART_REGINT_STATUS_ENOTOPEN;
    }

    if (size == 0)
    {
        return 0;
    }

    if (buf == NULL)
    {
        return UART_REGINT_STATUS_EINVALID;
    }

    /* Call mode-specific read function */
    if (object->params.readMode == UART_REGINT_MODE_BLOCKING)
    {
        return UART_RegInt_readBlocking(handle, buf, size, UART_REGINT_WAIT_FOREVER);
    }
    else
    {
        return UART_RegInt_readCallback(handle, buf, size);
    }
}

/*
 *  ======== UART_RegInt_readTimeout ========
 */
int_fast16_t UART_RegInt_readTimeout(UART_RegInt_Handle handle,
                                     void *buf,
                                     size_t size,
                                     uint32_t timeout)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;

    if (!object->state.opened)
    {
        return UART_REGINT_STATUS_ENOTOPEN;
    }

    if (size == 0)
    {
        return 0;
    }

    if (buf == NULL)
    {
        return UART_REGINT_STATUS_EINVALID;
    }

    return UART_RegInt_readBlocking(handle, buf, size, timeout);
}

/*
 *  ======== UART_RegInt_readCancel ========
 */
void UART_RegInt_readCancel(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    uintptr_t key;

    key = HwiP_disable();

    if (!object->state.rxInProgress)
    {
        HwiP_restore(key);
        return;
    }

    /* Set cancelled flag */
    object->state.rxCancelled = true;
    object->readStatus = UART_REGINT_STATUS_ECANCELLED;

    /* Disable RX interrupts */
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    UART_RegIntSupport_disableRx(hwAttrs);

    /* Clear in-progress flag */
    object->state.rxInProgress = false;

    HwiP_restore(key);

    /* Wake up blocked thread or invoke callback */
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
                                   UART_REGINT_STATUS_ECANCELLED);
    }
}

/*
 *  ======== UART_RegInt_write ========
 */
int_fast16_t UART_RegInt_write(UART_RegInt_Handle handle,
                               const void *buf,
                               size_t size)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;

    if (!object->state.opened)
    {
        return UART_REGINT_STATUS_ENOTOPEN;
    }

    if (size == 0)
    {
        return 0;
    }

    if (buf == NULL)
    {
        return UART_REGINT_STATUS_EINVALID;
    }

    /* Call mode-specific write function */
    if (object->params.writeMode == UART_REGINT_MODE_BLOCKING)
    {
        return UART_RegInt_writeBlocking(handle, buf, size, UART_REGINT_WAIT_FOREVER);
    }
    else
    {
        return UART_RegInt_writeCallback(handle, buf, size);
    }
}

/*
 *  ======== UART_RegInt_writeTimeout ========
 */
int_fast16_t UART_RegInt_writeTimeout(UART_RegInt_Handle handle,
                                      const void *buf,
                                      size_t size,
                                      uint32_t timeout)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;

    if (!object->state.opened)
    {
        return UART_REGINT_STATUS_ENOTOPEN;
    }

    if (size == 0)
    {
        return 0;
    }

    if (buf == NULL)
    {
        return UART_REGINT_STATUS_EINVALID;
    }

    return UART_RegInt_writeBlocking(handle, buf, size, timeout);
}

/*
 *  ======== UART_RegInt_writeCancel ========
 */
void UART_RegInt_writeCancel(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    uintptr_t key;

    key = HwiP_disable();

    if (!object->state.txInProgress)
    {
        HwiP_restore(key);
        return;
    }

    /* Set cancelled flag */
    object->state.txCancelled = true;
    object->writeStatus = UART_REGINT_STATUS_ECANCELLED;

    /* Disable TX interrupts */
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    UART_RegIntSupport_disableTx(hwAttrs);

    /* Clear in-progress flag */
    object->state.txInProgress = false;

    HwiP_restore(key);

    /* Wake up blocked thread or invoke callback */
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
                                    UART_REGINT_STATUS_ECANCELLED);
    }
}

/*
 *  ======== UART_RegInt_rxEnable ========
 */
void UART_RegInt_rxEnable(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    uintptr_t key;

    key = HwiP_disable();

    if (!object->state.rxEnabled)
    {
        object->state.rxEnabled = true;
        
        /* Set power constraint to prevent standby */
        UART_RegIntSupport_powerSetConstraint(handle);
    }

    HwiP_restore(key);
}

/*
 *  ======== UART_RegInt_rxDisable ========
 */
void UART_RegInt_rxDisable(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    uintptr_t key;

    key = HwiP_disable();

    if (object->state.rxEnabled)
    {
        object->state.rxEnabled = false;
        
        /* Release power constraint */
        UART_RegIntSupport_powerRelConstraint(handle);
    }

    HwiP_restore(key);
}

/*
 *  ======== UART_RegInt_getOverrunCount ========
 */
uint32_t UART_RegInt_getOverrunCount(UART_RegInt_Handle handle)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;

    return object->overrunCount;
}

/*
 *  ======== UART_RegInt_readBlocking ========
 *  Internal blocking read implementation
 */
static int_fast16_t UART_RegInt_readBlocking(UART_RegInt_Handle handle,
                                             void *buf,
                                             size_t size,
                                             uint32_t timeout)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uintptr_t key;
    SemaphoreP_Status semStatus;

    key = HwiP_disable();

    /* Check if read already in progress */
    if (object->state.rxInProgress)
    {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }

    /* Initialize read transaction */
    object->readBuf = (uint8_t *)buf;
    object->readSize = size;
    object->readCount = 0;
    object->readStatus = UART_REGINT_STATUS_SUCCESS;
    object->state.rxInProgress = true;
    object->state.rxCancelled = false;

    /* Enable RX interrupts */
    UART_RegIntSupport_enableRx(hwAttrs);
    UART_RegIntSupport_enableInts(handle);

    HwiP_restore(key);

    /* Wait for completion with timeout */
    if (timeout == UART_REGINT_WAIT_FOREVER)
    {
        SemaphoreP_pend(&object->readSem, SemaphoreP_WAIT_FOREVER);
    }
    else
    {
        /* Convert microseconds to system ticks */
        uint32_t ticks = (timeout * ClockP_getSystemTickPeriod()) / ClockP_TICK_PERIOD;
        if (ticks == 0) ticks = 1;
        
        semStatus = SemaphoreP_pend(&object->readSem, ticks);
        
        if (semStatus == SemaphoreP_TIMEOUT)
        {
            key = HwiP_disable();
            
            /* Disable interrupts */
            UART_RegIntSupport_disableRx(hwAttrs);
            object->state.rxInProgress = false;
            
            /* Check return mode */
            if (object->params.readReturnMode == UART_REGINT_ReadReturnMode_PARTIAL)
            {
                /* Return partial data */
                size_t count = object->readCount;
                HwiP_restore(key);
                return (int_fast16_t)count;
            }
            else
            {
                HwiP_restore(key);
                return UART_REGINT_STATUS_ETIMEOUT;
            }
        }
    }

    /* Return status or count */
    if (object->readStatus != UART_REGINT_STATUS_SUCCESS)
    {
        return object->readStatus;
    }
    
    return (int_fast16_t)object->readCount;
}

/*
 *  ======== UART_RegInt_readCallback ========
 *  Internal callback read implementation
 */
static int_fast16_t UART_RegInt_readCallback(UART_RegInt_Handle handle,
                                             void *buf,
                                             size_t size)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uintptr_t key;

    key = HwiP_disable();

    /* Check if read already in progress */
    if (object->state.rxInProgress)
    {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }

    /* Initialize read transaction */
    object->readBuf = (uint8_t *)buf;
    object->readSize = size;
    object->readCount = 0;
    object->readStatus = UART_REGINT_STATUS_SUCCESS;
    object->state.rxInProgress = true;
    object->state.rxCancelled = false;

    /* Enable RX interrupts */
    UART_RegIntSupport_enableRx(hwAttrs);
    UART_RegIntSupport_enableInts(handle);

    HwiP_restore(key);

    return UART_REGINT_STATUS_SUCCESS;
}

/*
 *  ======== UART_RegInt_writeBlocking ========
 *  Internal blocking write implementation
 */
static int_fast16_t UART_RegInt_writeBlocking(UART_RegInt_Handle handle,
                                              const void *buf,
                                              size_t size,
                                              uint32_t timeout)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uintptr_t key;
    uint32_t bytesWritten;
    SemaphoreP_Status semStatus;

    key = HwiP_disable();

    /* Check if write already in progress */
    if (object->state.txInProgress)
    {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }

    /* Initialize write transaction */
    object->writeBuf = (const uint8_t *)buf;
    object->writeSize = size;
    object->writeCount = 0;
    object->writeStatus = UART_REGINT_STATUS_SUCCESS;
    object->state.txInProgress = true;
    object->state.txCancelled = false;

    /* Write as much as possible to FIFO immediately */
    bytesWritten = UART_RegIntSupport_sendData(hwAttrs, 
                                               size - object->writeCount,
                                               &object->writeBuf[object->writeCount]);
    object->writeCount += bytesWritten;

    /* Check if all data written */
    if (object->writeCount >= object->writeSize)
    {
        object->state.txInProgress = false;
        HwiP_restore(key);
        return (int_fast16_t)object->writeCount;
    }

    /* Enable TX interrupt for remaining data */
    UART_RegIntSupport_enableTx(hwAttrs);

    HwiP_restore(key);

    /* Wait for completion with timeout */
    if (timeout == UART_REGINT_WAIT_FOREVER)
    {
        SemaphoreP_pend(&object->writeSem, SemaphoreP_WAIT_FOREVER);
    }
    else
    {
        /* Convert microseconds to system ticks */
        uint32_t ticks = (timeout * ClockP_getSystemTickPeriod()) / ClockP_TICK_PERIOD;
        if (ticks == 0) ticks = 1;
        
        semStatus = SemaphoreP_pend(&object->writeSem, ticks);
        
        if (semStatus == SemaphoreP_TIMEOUT)
        {
            key = HwiP_disable();
            UART_RegIntSupport_disableTx(hwAttrs);
            object->state.txInProgress = false;
            HwiP_restore(key);
            return UART_REGINT_STATUS_ETIMEOUT;
        }
    }

    /* Return status or count */
    if (object->writeStatus != UART_REGINT_STATUS_SUCCESS)
    {
        return object->writeStatus;
    }
    
    return (int_fast16_t)object->writeCount;
}

/*
 *  ======== UART_RegInt_writeCallback ========
 *  Internal callback write implementation
 */
static int_fast16_t UART_RegInt_writeCallback(UART_RegInt_Handle handle,
                                              const void *buf,
                                              size_t size)
{
    UART_RegInt_Config *config = (UART_RegInt_Config *)handle;
    UART_RegIntLPF3_Object *object = (UART_RegIntLPF3_Object *)config->object;
    const UART_RegIntLPF3_HWAttrs *hwAttrs = 
        (const UART_RegIntLPF3_HWAttrs *)config->hwAttrs;
    uintptr_t key;
    uint32_t bytesWritten;

    key = HwiP_disable();

    /* Check if write already in progress */
    if (object->state.txInProgress)
    {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }

    /* Initialize write transaction */
    object->writeBuf = (const uint8_t *)buf;
    object->writeSize = size;
    object->writeCount = 0;
    object->writeStatus = UART_REGINT_STATUS_SUCCESS;
    object->state.txInProgress = true;
    object->state.txCancelled = false;

    /* Write as much as possible to FIFO immediately */
    bytesWritten = UART_RegIntSupport_sendData(hwAttrs,
                                               size - object->writeCount,
                                               &object->writeBuf[object->writeCount]);
    object->writeCount += bytesWritten;

    /* Check if all data written */
    if (object->writeCount >= object->writeSize)
    {
        object->state.txInProgress = false;
        HwiP_restore(key);
        
        /* Invoke callback */
        if (object->params.writeCallback != NULL)
        {
            object->params.writeCallback(handle,
                                        (void *)buf,
                                        object->writeCount,
                                        object->params.userArg,
                                        UART_REGINT_STATUS_SUCCESS);
        }
        
        return UART_REGINT_STATUS_SUCCESS;
    }

    /* Enable TX interrupt for remaining data */
    UART_RegIntSupport_enableTx(hwAttrs);

    HwiP_restore(key);

    return UART_REGINT_STATUS_SUCCESS;
}
