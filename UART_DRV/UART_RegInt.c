/**
 * @file        UART_RegInt.c
 * @brief       UART Register Interrupt Driver - Core Implementation
 */

#include "UART_RegInt.h"
#include "UART_RegInt_Priv.h"
#include "UART_RegInt_Config.h"
#include <string.h>

static bool gModuleInitialized = false;

static const UART_RegInt_Params defaultParams = {
    .readMode = UART_REGINT_MODE_BLOCKING,
    .writeMode = UART_REGINT_MODE_BLOCKING,
    .readCallback = NULL,
    .writeCallback = NULL,
    .eventCallback = NULL,
    .eventMask = 0,
    .readReturnMode = UART_REGINT_ReadReturnMode_FULL,
    .baudRate = UART_REGINT_DEFAULT_BAUDRATE,
    .dataLength = UART_REGINT_DataLen_8,
    .stopBits = UART_REGINT_StopBits_1,
    .parityType = UART_REGINT_Parity_NONE,
    .userArg = NULL,
};

static int_fast16_t convertErrorToStatus(uint32_t errorFlags);
static void processRxInterrupt(UART_RegInt_Handle handle);
static void processTxInterrupt(UART_RegInt_Handle handle);
static void processErrorInterrupt(UART_RegInt_Handle handle, uint32_t intStatus);

void UART_RegInt_init(void)
{
    if (!gModuleInitialized) {
        gModuleInitialized = true;
    }
}

void UART_RegInt_Params_init(UART_RegInt_Params *params)
{
    if (params != NULL) {
        *params = defaultParams;
    }
}

UART_RegInt_Handle UART_RegInt_open(uint_least8_t index, UART_RegInt_Params *params)
{
    UART_RegInt_Handle handle;
    UART_RegInt_Params localParams;
    HwiP_Params hwiParams;
    SemaphoreP_Params semParams;
    uintptr_t key;
    
    if (index >= UART_RegInt_count) {
        return NULL;
    }
    
    handle = (UART_RegInt_Handle)UART_RegInt_config[index].object;
    
    key = HwiP_disable();
    if (handle->state.opened) {
        HwiP_restore(key);
        return NULL;
    }
    handle->state.opened = true;
    HwiP_restore(key);
    
    if (params == NULL) {
        localParams = defaultParams;
    } else {
        localParams = *params;
    }
    
    memset(&handle->state, 0, sizeof(handle->state));
    handle->state.opened = true;
    handle->params = localParams;
    handle->baseAddr = UART_REGINT_BASE_ADDR;
    handle->overrunCount = 0;
    
    handle->rxBuf = NULL;
    handle->rxSize = 0;
    handle->rxCount = 0;
    handle->rxStatus = UART_REGINT_STATUS_SUCCESS;
    
    handle->txBuf = NULL;
    handle->txSize = 0;
    handle->txCount = 0;
    handle->txStatus = UART_REGINT_STATUS_SUCCESS;
    
    Power_setDependency(UART_REGINT_POWER_ID);
    
    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;
    SemaphoreP_construct(&handle->readSem, 0, &semParams);
    SemaphoreP_construct(&handle->writeSem, 0, &semParams);
    
    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = UART_REGINT_DEFAULT_INT_PRIORITY;
    HwiP_construct(&handle->hwi, UART_REGINT_INT_NUM, UART_RegInt_hwiIntFxn, &hwiParams);
    
    UART_RegInt_initHw(handle);
    
    Power_registerNotify(&handle->preNotify, PowerLPF3_ENTERING_STANDBY,
                        UART_RegInt_preNotifyFxn, (uintptr_t)handle);
    Power_registerNotify(&handle->postNotify, PowerLPF3_AWAKE_STANDBY,
                        UART_RegInt_postNotifyFxn, (uintptr_t)handle);
    
    return handle;
}

void UART_RegInt_close(UART_RegInt_Handle handle)
{
    uintptr_t key;
    
    if (handle == NULL) return;
    
    UART_RegInt_disableInt(handle->baseAddr, 
                          UART_REGINT_INT_RX | UART_REGINT_INT_TX | 
                          UART_REGINT_INT_RT | UART_REGINT_INT_ALL_ERRORS);
    
    UART_RegInt_readCancel(handle);
    UART_RegInt_writeCancel(handle);
    UART_RegInt_disable(handle->baseAddr);
    
    HwiP_destruct(&handle->hwi);
    SemaphoreP_destruct(&handle->readSem);
    SemaphoreP_destruct(&handle->writeSem);
    
    Power_unregisterNotify(&handle->preNotify);
    Power_unregisterNotify(&handle->postNotify);
    Power_releaseDependency(UART_REGINT_POWER_ID);
    
    key = HwiP_disable();
    handle->state.opened = false;
    HwiP_restore(key);
}

int_fast16_t UART_RegInt_read(UART_RegInt_Handle handle, void *buf, size_t size)
{
    uintptr_t key;
    int_fast16_t status;
    size_t count;
    
    if (handle == NULL || buf == NULL || size == 0) {
        return UART_REGINT_STATUS_EINVALID;
    }
    
    key = HwiP_disable();
    if (handle->state.rxInProgress) {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }
    
    handle->rxBuf = (uint8_t *)buf;
    handle->rxSize = size;
    handle->rxCount = 0;
    handle->rxStatus = UART_REGINT_STATUS_SUCCESS;
    handle->state.rxInProgress = true;
    handle->state.rxCancelled = false;
    
    UART_RegInt_enableInt(handle->baseAddr, 
                         UART_REGINT_INT_RX | UART_REGINT_INT_RT | UART_REGINT_INT_ALL_ERRORS);
    
    HwiP_restore(key);
    
    if (handle->params.readMode == UART_REGINT_MODE_CALLBACK) {
        return UART_REGINT_STATUS_SUCCESS;
    }
    
    SemaphoreP_pend(&handle->readSem, SemaphoreP_WAIT_FOREVER);
    
    key = HwiP_disable();
    status = handle->rxStatus;
    count = handle->rxCount;
    HwiP_restore(key);
    
    return (status == UART_REGINT_STATUS_SUCCESS) ? (int_fast16_t)count : status;
}

int_fast16_t UART_RegInt_readTimeout(UART_RegInt_Handle handle, void *buf, 
                                     size_t size, uint32_t timeout)
{
    uintptr_t key;
    int_fast16_t status;
    bool semStatus;
    size_t count;
    
    if (handle == NULL || buf == NULL || size == 0) {
        return UART_REGINT_STATUS_EINVALID;
    }
    
    key = HwiP_disable();
    if (handle->state.rxInProgress) {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }
    
    handle->rxBuf = (uint8_t *)buf;
    handle->rxSize = size;
    handle->rxCount = 0;
    handle->rxStatus = UART_REGINT_STATUS_SUCCESS;
    handle->state.rxInProgress = true;
    handle->state.rxCancelled = false;
    
    UART_RegInt_enableInt(handle->baseAddr, 
                         UART_REGINT_INT_RX | UART_REGINT_INT_RT | UART_REGINT_INT_ALL_ERRORS);
    HwiP_restore(key);
    
    semStatus = (timeout == UART_REGINT_WAIT_FOREVER) ?
                SemaphoreP_pend(&handle->readSem, SemaphoreP_WAIT_FOREVER) :
                SemaphoreP_pend(&handle->readSem, timeout);
    
    key = HwiP_disable();
    if (!semStatus) {
        handle->state.rxInProgress = false;
        UART_RegInt_disableInt(handle->baseAddr, UART_REGINT_INT_RX | UART_REGINT_INT_RT);
        count = handle->rxCount;
        HwiP_restore(key);
        
        if (handle->params.readReturnMode == UART_REGINT_ReadReturnMode_PARTIAL && count > 0) {
            return (int_fast16_t)count;
        }
        return UART_REGINT_STATUS_ETIMEOUT;
    }
    
    status = handle->rxStatus;
    count = handle->rxCount;
    HwiP_restore(key);
    
    return (status == UART_REGINT_STATUS_SUCCESS) ? (int_fast16_t)count : status;
}

void UART_RegInt_readCancel(UART_RegInt_Handle handle)
{
    uintptr_t key;
    
    if (handle == NULL) return;
    
    key = HwiP_disable();
    if (handle->state.rxInProgress) {
        handle->state.rxCancelled = true;
        handle->state.rxInProgress = false;
        UART_RegInt_disableInt(handle->baseAddr, UART_REGINT_INT_RX | UART_REGINT_INT_RT);
        handle->rxStatus = UART_REGINT_STATUS_ECANCELLED;
        
        if (handle->params.readMode == UART_REGINT_MODE_CALLBACK && 
            handle->params.readCallback != NULL) {
            HwiP_restore(key);
            handle->params.readCallback(handle, handle->rxBuf, handle->rxCount,
                                       handle->params.userArg, UART_REGINT_STATUS_ECANCELLED);
            return;
        }
        SemaphoreP_post(&handle->readSem);
    }
    HwiP_restore(key);
}

int_fast16_t UART_RegInt_write(UART_RegInt_Handle handle, const void *buf, size_t size)
{
    uintptr_t key;
    int_fast16_t status;
    size_t count;
    
    if (handle == NULL || buf == NULL || size == 0) {
        return UART_REGINT_STATUS_EINVALID;
    }
    
    key = HwiP_disable();
    if (handle->state.txInProgress) {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }
    
    handle->txBuf = (const uint8_t *)buf;
    handle->txSize = size;
    handle->txCount = 0;
    handle->txStatus = UART_REGINT_STATUS_SUCCESS;
    handle->state.txInProgress = true;
    handle->state.txCancelled = false;
    
    if (handle->params.eventCallback != NULL && 
        (handle->params.eventMask & UART_REGINT_EVENT_TX_BEGIN)) {
        handle->params.eventCallback(handle, UART_REGINT_EVENT_TX_BEGIN, 0, handle->params.userArg);
    }
    
    while (handle->txCount < handle->txSize && !UART_REGINT_TX_FIFO_FULL(handle->baseAddr)) {
        UART_REGINT_WRITE_DATA(handle->baseAddr, handle->txBuf[handle->txCount]);
        handle->txCount++;
    }
    
    if (handle->txCount < handle->txSize) {
        UART_RegInt_enableInt(handle->baseAddr, UART_REGINT_INT_TX);
    } else {
        handle->state.txInProgress = false;
        if (handle->params.eventCallback != NULL && 
            (handle->params.eventMask & UART_REGINT_EVENT_TX_FINISHED)) {
            handle->params.eventCallback(handle, UART_REGINT_EVENT_TX_FINISHED, 0, handle->params.userArg);
        }
    }
    
    HwiP_restore(key);
    
    if (handle->params.writeMode == UART_REGINT_MODE_CALLBACK) {
        if (handle->txCount >= handle->txSize && handle->params.writeCallback != NULL) {
            handle->params.writeCallback(handle, (void *)handle->txBuf, handle->txCount,
                                        handle->params.userArg, UART_REGINT_STATUS_SUCCESS);
        }
        return UART_REGINT_STATUS_SUCCESS;
    }
    
    if (handle->txCount < handle->txSize) {
        SemaphoreP_pend(&handle->writeSem, SemaphoreP_WAIT_FOREVER);
    }
    
    key = HwiP_disable();
    status = handle->txStatus;
    count = handle->txCount;
    HwiP_restore(key);
    
    return (status == UART_REGINT_STATUS_SUCCESS) ? (int_fast16_t)count : status;
}

int_fast16_t UART_RegInt_writeTimeout(UART_RegInt_Handle handle, const void *buf,
                                      size_t size, uint32_t timeout)
{
    uintptr_t key;
    int_fast16_t status;
    bool semStatus;
    bool needWait;
    size_t count;
    
    if (handle == NULL || buf == NULL || size == 0) {
        return UART_REGINT_STATUS_EINVALID;
    }
    
    key = HwiP_disable();
    if (handle->state.txInProgress) {
        HwiP_restore(key);
        return UART_REGINT_STATUS_EINUSE;
    }
    
    handle->txBuf = (const uint8_t *)buf;
    handle->txSize = size;
    handle->txCount = 0;
    handle->txStatus = UART_REGINT_STATUS_SUCCESS;
    handle->state.txInProgress = true;
    handle->state.txCancelled = false;
    
    if (handle->params.eventCallback != NULL && 
        (handle->params.eventMask & UART_REGINT_EVENT_TX_BEGIN)) {
        handle->params.eventCallback(handle, UART_REGINT_EVENT_TX_BEGIN, 0, handle->params.userArg);
    }
    
    while (handle->txCount < handle->txSize && !UART_REGINT_TX_FIFO_FULL(handle->baseAddr)) {
        UART_REGINT_WRITE_DATA(handle->baseAddr, handle->txBuf[handle->txCount]);
        handle->txCount++;
    }
    
    needWait = (handle->txCount < handle->txSize);
    if (needWait) {
        UART_RegInt_enableInt(handle->baseAddr, UART_REGINT_INT_TX);
    } else {
        handle->state.txInProgress = false;
        if (handle->params.eventCallback != NULL && 
            (handle->params.eventMask & UART_REGINT_EVENT_TX_FINISHED)) {
            handle->params.eventCallback(handle, UART_REGINT_EVENT_TX_FINISHED, 0, handle->params.userArg);
        }
    }
    HwiP_restore(key);
    
    if (!needWait) {
        return (int_fast16_t)size;
    }
    
    semStatus = (timeout == UART_REGINT_WAIT_FOREVER) ?
                SemaphoreP_pend(&handle->writeSem, SemaphoreP_WAIT_FOREVER) :
                SemaphoreP_pend(&handle->writeSem, timeout);
    
    key = HwiP_disable();
    if (!semStatus) {
        handle->state.txInProgress = false;
        UART_RegInt_disableInt(handle->baseAddr, UART_REGINT_INT_TX);
        status = UART_REGINT_STATUS_ETIMEOUT;
    } else {
        status = handle->txStatus;
    }
    count = handle->txCount;
    HwiP_restore(key);
    
    return (status == UART_REGINT_STATUS_SUCCESS) ? (int_fast16_t)count : status;
}

void UART_RegInt_writeCancel(UART_RegInt_Handle handle)
{
    uintptr_t key;
    
    if (handle == NULL) return;
    
    key = HwiP_disable();
    if (handle->state.txInProgress) {
        handle->state.txCancelled = true;
        handle->state.txInProgress = false;
        UART_RegInt_disableInt(handle->baseAddr, UART_REGINT_INT_TX);
        handle->txStatus = UART_REGINT_STATUS_ECANCELLED;
        
        if (handle->params.writeMode == UART_REGINT_MODE_CALLBACK && 
            handle->params.writeCallback != NULL) {
            HwiP_restore(key);
            handle->params.writeCallback(handle, (void *)handle->txBuf, handle->txCount,
                                        handle->params.userArg, UART_REGINT_STATUS_ECANCELLED);
            return;
        }
        SemaphoreP_post(&handle->writeSem);
    }
    HwiP_restore(key);
}

void UART_RegInt_rxEnable(UART_RegInt_Handle handle)
{
    uintptr_t key;
    
    if (handle == NULL) return;
    
    key = HwiP_disable();
    if (!handle->state.rxEnabled) {
        handle->state.rxEnabled = true;
        Power_setConstraint(PowerLPF3_DISALLOW_STANDBY);
    }
    HwiP_restore(key);
}

void UART_RegInt_rxDisable(UART_RegInt_Handle handle)
{
    uintptr_t key;
    
    if (handle == NULL) return;
    
    key = HwiP_disable();
    if (handle->state.rxEnabled) {
        handle->state.rxEnabled = false;
        Power_releaseConstraint(PowerLPF3_DISALLOW_STANDBY);
    }
    HwiP_restore(key);
}

uint32_t UART_RegInt_getOverrunCount(UART_RegInt_Handle handle)
{
    return (handle != NULL) ? handle->overrunCount : 0;
}

void UART_RegInt_initHw(UART_RegInt_Handle handle)
{
    ClockP_FreqHz freq;
    uint32_t config = 0;
    
    UART_RegInt_disable(handle->baseAddr);
    ClockP_getCpuFreq(&freq);
    
    switch (handle->params.dataLength) {
        case UART_REGINT_DataLen_5: config |= UART_CONFIG_WLEN_5; break;
        case UART_REGINT_DataLen_6: config |= UART_CONFIG_WLEN_6; break;
        case UART_REGINT_DataLen_7: config |= UART_CONFIG_WLEN_7; break;
        case UART_REGINT_DataLen_8:
        default: config |= UART_CONFIG_WLEN_8; break;
    }
    
    config |= (handle->params.stopBits == UART_REGINT_StopBits_2) ? 
              UART_CONFIG_STOP_TWO : UART_CONFIG_STOP_ONE;
    
    switch (handle->params.parityType) {
        case UART_REGINT_Parity_EVEN: config |= UART_CONFIG_PAR_EVEN; break;
        case UART_REGINT_Parity_ODD: config |= UART_CONFIG_PAR_ODD; break;
        case UART_REGINT_Parity_ZERO: config |= UART_CONFIG_PAR_ZERO; break;
        case UART_REGINT_Parity_ONE: config |= UART_CONFIG_PAR_ONE; break;
        case UART_REGINT_Parity_NONE:
        default: config |= UART_CONFIG_PAR_NONE; break;
    }
    
    UARTConfigSetExpClk(handle->baseAddr, freq.lo, handle->params.baudRate, config);
    UARTFIFOEnable(handle->baseAddr);
    UARTFIFOLevelSet(handle->baseAddr, UART_FIFO_TX2_8, UART_FIFO_RX6_8);
    UART_REGINT_CLEAR_ERROR_STATUS(handle->baseAddr);
    UART_RegInt_enable(handle->baseAddr);
    handle->state.txEnabled = true;
}

void UART_RegInt_hwiIntFxn(uintptr_t arg)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)arg;
    uint32_t intStatus = UART_RegInt_getIntStatus(handle->baseAddr);
    
    if (intStatus & (UART_REGINT_INT_RX | UART_REGINT_INT_RT)) {
        UART_RegInt_clearInt(handle->baseAddr, UART_REGINT_INT_RX | UART_REGINT_INT_RT);
        processRxInterrupt(handle);
    }
    
    if (intStatus & UART_REGINT_INT_TX) {
        UART_RegInt_clearInt(handle->baseAddr, UART_REGINT_INT_TX);
        processTxInterrupt(handle);
    }
    
    if (intStatus & UART_REGINT_INT_ALL_ERRORS) {
        UART_RegInt_clearInt(handle->baseAddr, UART_REGINT_INT_ALL_ERRORS);
        processErrorInterrupt(handle, intStatus);
    }
}

static void processRxInterrupt(UART_RegInt_Handle handle)
{
    uint8_t data;
    uint32_t errorStatus;
    bool completed = false;
    
    if (!handle->state.rxInProgress) return;
    
    while (!UART_REGINT_RX_FIFO_EMPTY(handle->baseAddr) && handle->rxCount < handle->rxSize) {
        errorStatus = UART_REGINT_READ_ERROR_STATUS(handle->baseAddr);
        if (errorStatus != 0) {
            UART_REGINT_CLEAR_ERROR_STATUS(handle->baseAddr);
            handle->rxStatus = convertErrorToStatus(errorStatus);
            completed = true;
            break;
        }
        
        data = UART_REGINT_READ_DATA(handle->baseAddr);
        handle->rxBuf[handle->rxCount] = data;
        handle->rxCount++;
        
        if (handle->rxCount >= handle->rxSize) {
            completed = true;
            break;
        }
    }
    
    if (completed) {
        handle->state.rxInProgress = false;
        UART_RegInt_disableInt(handle->baseAddr, UART_REGINT_INT_RX | UART_REGINT_INT_RT);
        
        if (handle->params.readMode == UART_REGINT_MODE_CALLBACK &&
            handle->params.readCallback != NULL) {
            handle->params.readCallback(handle, handle->rxBuf, handle->rxCount,
                                       handle->params.userArg, handle->rxStatus);
        } else {
            SemaphoreP_post(&handle->readSem);
        }
    }
}

static void processTxInterrupt(UART_RegInt_Handle handle)
{
    bool completed = false;
    
    if (!handle->state.txInProgress) return;
    
    while (handle->txCount < handle->txSize && !UART_REGINT_TX_FIFO_FULL(handle->baseAddr)) {
        UART_REGINT_WRITE_DATA(handle->baseAddr, handle->txBuf[handle->txCount]);
        handle->txCount++;
        
        if (handle->txCount >= handle->txSize) {
            completed = true;
            break;
        }
    }
    
    if (completed) {
        handle->state.txInProgress = false;
        UART_RegInt_disableInt(handle->baseAddr, UART_REGINT_INT_TX);
        
        if (handle->params.eventCallback != NULL && 
            (handle->params.eventMask & UART_REGINT_EVENT_TX_FINISHED)) {
            handle->params.eventCallback(handle, UART_REGINT_EVENT_TX_FINISHED, 0, handle->params.userArg);
        }
        
        if (handle->params.writeMode == UART_REGINT_MODE_CALLBACK &&
            handle->params.writeCallback != NULL) {
            handle->params.writeCallback(handle, (void *)handle->txBuf, handle->txCount,
                                        handle->params.userArg, UART_REGINT_STATUS_SUCCESS);
        } else {
            SemaphoreP_post(&handle->writeSem);
        }
    }
}

static void processErrorInterrupt(UART_RegInt_Handle handle, uint32_t intStatus)
{
    uint32_t event = 0;
    
    if (intStatus & UART_REGINT_INT_OE) {
        handle->overrunCount++;
        event |= UART_REGINT_EVENT_OVERRUN;
        if (handle->state.rxInProgress) {
            handle->rxStatus = UART_REGINT_STATUS_EOVERRUN;
        }
    }
    
    if (intStatus & UART_REGINT_INT_BE) {
        event |= UART_REGINT_EVENT_BREAK;
        if (handle->state.rxInProgress) {
            handle->rxStatus = UART_REGINT_STATUS_EBREAK;
        }
    }
    
    if (intStatus & UART_REGINT_INT_PE) {
        event |= UART_REGINT_EVENT_PARITY;
        if (handle->state.rxInProgress) {
            handle->rxStatus = UART_REGINT_STATUS_EPARITY;
        }
    }
    
    if (intStatus & UART_REGINT_INT_FE) {
        event |= UART_REGINT_EVENT_FRAMING;
        if (handle->state.rxInProgress) {
            handle->rxStatus = UART_REGINT_STATUS_EFRAMING;
        }
    }
    
    if (event != 0 && handle->params.eventCallback != NULL && 
        (handle->params.eventMask & event)) {
        handle->params.eventCallback(handle, event, 0, handle->params.userArg);
    }
    
    UART_REGINT_CLEAR_ERROR_STATUS(handle->baseAddr);
}

static int_fast16_t convertErrorToStatus(uint32_t errorFlags)
{
    if (errorFlags & UART_REGINT_ERR_OVERRUN) return UART_REGINT_STATUS_EOVERRUN;
    if (errorFlags & UART_REGINT_ERR_BREAK) return UART_REGINT_STATUS_EBREAK;
    if (errorFlags & UART_REGINT_ERR_PARITY) return UART_REGINT_STATUS_EPARITY;
    if (errorFlags & UART_REGINT_ERR_FRAMING) return UART_REGINT_STATUS_EFRAMING;
    return UART_REGINT_STATUS_SUCCESS;
}

int UART_RegInt_preNotifyFxn(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg)
{
    return Power_NOTIFYDONE;
}

int UART_RegInt_postNotifyFxn(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg)
{
    UART_RegInt_Handle handle = (UART_RegInt_Handle)clientArg;
    if (handle != NULL && handle->state.opened) {
        UART_RegInt_initHw(handle);
    }
    return Power_NOTIFYDONE;
}
