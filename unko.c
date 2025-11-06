/*
 *  ======== UART_RegIntLPF3.h ========
 *  追加定義（ヘッダーファイルに追加）
 */

/* FIFO動作モード */
typedef enum {
    UART_REGINT_FIFO_MODE_ENABLED = 0,   /* FIFO有効（デフォルト） */
    UART_REGINT_FIFO_MODE_DISABLED = 1   /* FIFO無効（LIN通信等） */
} UART_RegInt_FifoMode;

/* HWAttrs構造体に追加するフィールド */
typedef struct {
    /* 既存のフィールド... */

    /* FIFO動作モード */
    UART_RegInt_FifoMode fifoMode;

    /* FIFO無効時の受信タイムアウト（マイクロ秒） */
    /* 0 = タイムアウト無効 */
    uint32_t rxTimeoutUs;

} UART2LPF3_HWAttrs;


/*
 *  ======== UART_RegIntLPF3.c ========
 *  改良版実装
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <ti/drivers/GPIO.h>

#include "UART_RegIntLPF3.h"
#include <ti/drivers/uart2/UART2Support.h>

/* driverlib header files */
#include DeviceFamily_constructPath(driverlib/uart.h)

/* FIFO trigger levels */
#define TX_FIFO_INT_LEVEL UART_FIFO_TX2_8  /* Trigger when FIFO is 2/8 full */
#define RX_FIFO_INT_LEVEL UART_FIFO_RX1_8  /* より低い閾値に変更（LIN用） */

/* FIFO無効時の割り込み動作 */
#define FIFO_DISABLED_RX_TRIGGER  1  /* 1バイト受信で割り込み */
#define FIFO_DISABLED_TX_TRIGGER  1  /* 1バイト送信完了で割り込み */

/* Default UART2 parameters structure */
extern const UART2_Params UART2_defaultParams;

/* Map UART2 data length to driverlib data length */
static const uint8_t dataLength[] = {
    UART_CONFIG_WLEN_5, /* UART_REGINT_DataLen_5 */
    UART_CONFIG_WLEN_6, /* UART_REGINT_DataLen_6 */
    UART_CONFIG_WLEN_7, /* UART_REGINT_DataLen_7 */
    UART_CONFIG_WLEN_8  /* UART_REGINT_DataLen_8 */
};

/* Map UART2 stop bits to driverlib stop bits */
static const uint8_t stopBits[] = {
    UART_CONFIG_STOP_ONE, /* UART_REGINT_StopBits_1 */
    UART_CONFIG_STOP_TWO  /* UART_REGINT_StopBits_2 */
};

/* Map UART2 parity type to driverlib parity type */
static const uint8_t parityType[] = {
    UART_CONFIG_PAR_NONE, /* UART_REGINT_Parity_NONE */
    UART_CONFIG_PAR_EVEN, /* UART_REGINT_Parity_EVEN */
    UART_CONFIG_PAR_ODD,  /* UART_REGINT_Parity_ODD */
    UART_CONFIG_PAR_ZERO, /* UART_REGINT_Parity_ZERO */
    UART_CONFIG_PAR_ONE   /* UART_REGINT_Parity_ONE */
};

/* Static function prototypes */
static void UART_RegIntLPF3_eventCallback(UART2_Handle handle, uint32_t event, uint32_t data, void *userArg);
static void UART_RegIntLPF3_hwiIntFxn(uintptr_t arg);
static void UART_RegIntLPF3_initHw(UART2_Handle handle);
static void UART_RegIntLPF3_initIO(UART2_Handle handle);
static void UART_RegIntLPF3_finalizeIO(UART2_Handle handle);
static int UART_RegInt_preNotifyFxn(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg);
static int UART_RegInt_postNotifyFxn(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg);
static void UART_RegIntLPF3_hwiIntWrite(uintptr_t arg);
static void UART_RegIntLPF3_hwiIntRead(uintptr_t arg, uint32_t status, bool* rxPostPtr);

/*
 *  ======== UART2LPF3_isFifoEnabled ========
 *  FIFOが有効かどうかを確認
 */
static inline bool UART2LPF3_isFifoEnabled(UART2LPF3_HWAttrs const *hwAttrs)
{
    return (hwAttrs->fifoMode == UART_REGINT_FIFO_MODE_ENABLED);
}

/*
 *  ======== UART2LPF3_getRxData ========
 *  Must be called with HWI disabled.
 *  FIFO有効/無効の両方に対応
 */
static inline size_t UART2LPF3_getRxData(UART2_Handle handle, size_t size)
{
    UART2LPF3_Object *object         = handle->object;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;
    size_t consumed                  = 0;
    uint8_t data;

    if (UART2LPF3_isFifoEnabled(hwAttrs))
    {
        /* FIFO有効時: FIFOから複数バイト読み出し可能 */
        while (UARTCharAvailable(hwAttrs->baseAddr) && size)
        {
            /* Read directly from DATA register */
            data = HWREG(hwAttrs->baseAddr + UART_O_DR);
            RingBuf_put(&object->rxBuffer, data);
            ++consumed;
            --size;
        }
    }
    else
    {
        /* FIFO無効時: 1バイトのみ読み出し */
        if (UARTCharAvailable(hwAttrs->baseAddr) && size)
        {
            data = HWREG(hwAttrs->baseAddr + UART_O_DR);
            RingBuf_put(&object->rxBuffer, data);
            consumed = 1;
        }
    }

    return consumed;
}

/*
 * Function for checking whether flow control is enabled.
 */
static inline bool UART_RegIntLPF3_isFlowControlEnabled(UART2LPF3_HWAttrs const *hwAttrs)
{
    return hwAttrs->flowControl == UART2_FLOWCTRL_HARDWARE;
}

/*
 *  ======== UART2LPF3_getRxStatus ========
 *  Get the left-most bit set in the RX error status (OE, BE, PE, FE)
 *  read from the RSR register:
 *      bit#   3   2   1   0
 *             OE  BE  PE  FE
 *  e.g., if OE and FE are both set, OE wins.  This will make it easier
 *  to convert an RX error status to a UART2 error code.
 */
static inline uint32_t UART_RegIntLPF3_getRxStatus(uint32_t bitMask)
{
#if defined(__TI_COMPILER_VERSION__)
    return ((uint32_t)(bitMask & (0x80000000 >> __clz(bitMask))));
#elif defined(__GNUC__)
    return ((uint32_t)(bitMask & (0x80000000 >> __builtin_clz(bitMask))));
#elif defined(__IAR_SYSTEMS_ICC__)
    return ((uint32_t)(bitMask & (0x80000000 >> __CLZ(bitMask))));
#else
    #error "Unsupported compiler"
#endif
}

/*
 * Function for checking whether flow control is enabled.
 */
static inline bool UART2LPF3_isFlowControlEnabled(UART2LPF3_HWAttrs const *hwAttrs)
{
    return hwAttrs->flowControl == UART2_FLOWCTRL_HARDWARE;
}

/*
 *  ======== UART_RegInt_close ========
 */
void UART_RegInt_close(UART2_Handle handle)
{
    UART2LPF3_Object *object         = handle->object;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;


    /* UART、各割り込みをすべて無効化 */
    UARTDisableInt(hwAttrs->baseAddr,
                   UART_INT_TX | UART_INT_RX | UART_INT_RT | UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE |
                       UART_INT_CTS | UART_INT_TXDMADONE | UART_INT_RXDMADONE);

    /*これらの2つの関数は、それぞれ writeInUse と readInUse の状態に応じて条件付き
      で実行される
     */
    UART_RegInt_readCancel(handle);
    UART_RegInt_writeCancel(handle);

    /* rx（受信）tx(送信)が有効になっている場合に、電源制約を解除する */
    UART_RegInt_rxDisable(handle);
    UART_RegInt_txDisable(handle);

   /* UARTを無効化します。この関数は、TX FIFO（送信バッファ）が空になるまで待ってから周辺機器をシャットダウンします。そうしないと、BUSYビットがセットされたままとなり、周辺機器がフリーズする可能性があります。*/
    UARTDisable(hwAttrs->baseAddr);
    object->state.txEnabled = false;

    /* ピンを解放する */
    UART_RegIntLPF3_finalizeIO(handle);

    HwiP_destruct(&(object->hwi));
    SemaphoreP_destruct(&(object->writeSem));
    SemaphoreP_destruct(&(object->readSem));

    /* 電源通知オブジェクトの登録を解除する */
    Power_unregisterNotify(&object->postNotify);

    /* 電源依存性を解除する — つまり、シリアルドメインを電源オフにする可能性がある。 */
    Power_releaseDependency(hwAttrs->powerID);
    object->state.opened = false;
}

/*
 *  ======== UART2_flushRx ========
 *  FIFO有効/無効の両方に対応
 */
void UART_RegInt_flushRx(UART2_Handle handle)
{
    UART2LPF3_Object *object         = handle->object;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;
    uintptr_t key;

    key = HwiP_disable();
    RingBuf_flush(&object->rxBuffer);

    if (UART2LPF3_isFifoEnabled(hwAttrs))
    {
        /* FIFO有効時: FIFO全体をクリア */
        while (UARTCharAvailable(hwAttrs->baseAddr))
        {
            UARTGetCharNonBlocking(hwAttrs->baseAddr);
        }
    }
    else
    {
        /* FIFO無効時: データレジスタのみクリア */
        if (UARTCharAvailable(hwAttrs->baseAddr))
        {
            UARTGetCharNonBlocking(hwAttrs->baseAddr);
        }
    }

    /* Clear any read errors */
    UARTClearRxError(hwAttrs->baseAddr);

    HwiP_restore(key);
}

/*
 *  ======== UART_RegInt_open ========
 */
UART2_Handle UART_RegInt_open(uint_least8_t index, UART2_Params *params)
{
    UART2_Handle handle = NULL;
    uintptr_t key;
    UART2LPF3_Object *object;
    const UART2LPF3_HWAttrs *hwAttrs;
    HwiP_Params hwiParams;

    if (index < UART2_count)
    {
        handle  = (UART2_Handle) & (UART2_config[index]);
        hwAttrs = handle->hwAttrs;
        object  = handle->object;
    }
    else
    {
        return NULL;
    }

    /* paramsがNULLの場合、デフォルト値を利用する */
    if (params == NULL)
    {
        params = (UART2_Params *)&UART2_defaultParams;
    }

    /* callbackモードの場合、readCallbackとwriteCallback
     * 設定されているかを確認
     */
    if (((params->readMode == UART2_Mode_CALLBACK) && (params->readCallback == NULL)) ||
        ((params->writeMode == UART2_Mode_CALLBACK) && (params->writeCallback == NULL)))
    {
        return NULL;
    }

    key = HwiP_disable();

    if (object->state.opened)
    {
        HwiP_restore(key);
        return NULL;
    }
    /* オープンとしてマーク */
    object->state.opened = true;

    HwiP_restore(key);

    object->state.rxEnabled       = false;
    object->state.txEnabled       = false;
    object->state.rxCancelled     = false;
    object->state.txCancelled     = false;
    object->state.overrunActive   = false;
    object->state.inReadCallback  = false;
    object->state.inWriteCallback = false;
    object->state.overrunCount    = 0;
    object->state.readMode        = params->readMode;
    object->state.writeMode       = params->writeMode;
    object->state.readReturnMode  = params->readReturnMode;
    object->readCallback          = params->readCallback;
    object->writeCallback         = params->writeCallback;
    object->eventCallback         = params->eventCallback;
    object->eventMask             = params->eventMask;
    object->baudRate              = params->baudRate;
    object->stopBits              = params->stopBits;
    object->dataLength            = params->dataLength;
    object->parityType            = params->parityType;
    object->userArg               = params->userArg;

    /* Set UART transaction variables to defaults. */
    object->writeBuf   = NULL;
    object->readBuf    = NULL;
    object->writeCount = 0;
    object->readCount  = 0;
    object->writeSize  = 0;
    object->readSize   = 0;
    object->rxStatus   = 0;
    object->txStatus   = 0;
    object->rxSize     = 0;
    object->txSize     = 0;
    object->readInUse  = false;
    object->writeInUse = false;

    /* Set the event mask to 0 if the callback is NULL to simplify checks */
    if (object->eventCallback == NULL)
    {
        object->eventCallback = UART_RegIntLPF3_eventCallback;
        object->eventMask     = 0;
    }

    /* 電源制約の設定 */
    Power_setDependency(hwAttrs->powerID);

    RingBuf_construct(&object->rxBuffer, hwAttrs->rxBufPtr, hwAttrs->rxBufSize);
    RingBuf_construct(&object->txBuffer, hwAttrs->txBufPtr, hwAttrs->txBufSize);

    /* UARTの無効化 */
    UARTDisable(hwAttrs->baseAddr);

    /* UARTのHWを初期化 UARTのモジュール、FIFOの有効化/無効化 */
    /* RX,TXは有効化を行わない */
    UART_RegIntLPF3_initHw(handle);

    /* GPIOピンの初期化 */
    UART_RegIntLPF3_initIO(handle);

    Power_registerNotify(&object->preNotify, PowerLPF3_ENTERING_STANDBY, UART_RegInt_preNotifyFxn, (uintptr_t)handle);
    Power_registerNotify(&object->postNotify, PowerLPF3_AWAKE_STANDBY, UART_RegInt_postNotifyFxn, (uintptr_t)handle);

    /* このUART周辺機器のためにHwiオブジェクトを作成 */
    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = hwAttrs->intPriority;
    HwiP_construct(&(object->hwi), hwAttrs->intNum, UART_RegIntLPF3_hwiIntFxn, &hwiParams);

    /* セマフォの作成 */
    SemaphoreP_constructBinary(&object->readSem, 0);
    SemaphoreP_constructBinary(&object->writeSem, 0);

    return handle;
}

void UART_RegIntLPF3_disableRxInt(UART2_HWAttrs const *hwAttrs)
{
    UARTDisableInt(hwAttrs->baseAddr, UART_INT_RX);
}

void UART_RegIntLPF3_enableRxInt(UART2_HWAttrs const *hwAttrs)
{
    UARTEnableInt(hwAttrs->baseAddr, UART_INT_RX);
}

void UART_RegIntLPF3_disableTxInt(UART2_HWAttrs const *hwAttrs)
{
    UARTDisableInt(hwAttrs->baseAddr, UART_INT_TX);
}

void UART_RegIntLPF3_enableTxInt(UART2_HWAttrs const *hwAttrs)
{
    UARTEnableInt(hwAttrs->baseAddr, UART_INT_TX);
}

static void UART_RegIntLPF3_eventCallback(UART2_Handle handle, uint32_t event, uint32_t data, void *userArg)
{}

/*
 *  ======== UART_RegIntLPF3_hwiIntRead ========
 *  RX interrupt handler
 *  FIFO有効/無効の両方に最適化
 */
static void UART_RegIntLPF3_hwiIntRead(uintptr_t arg, uint32_t status, bool* rxPostPtr)
{
    UART2_Handle handle              = (UART2_Handle)arg;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;
    UART2LPF3_Object *object         = handle->object;
    uint16_t count = 0;
    bool isFifoEnabled = UART2LPF3_isFifoEnabled(hwAttrs);

    /* 受信割り込み */
    if (status & UART_INT_RX)
    {
        if (isFifoEnabled)
        {
            /* FIFO有効時: 複数バイトを一度に処理 */
            while (UARTCharAvailable(hwAttrs->baseAddr))
            {
                volatile uint8_t rxData = HWREG(hwAttrs->baseAddr + UART_O_DR) & 0xFF;

                /* try to push the data into the software RX buffer */
                if (RingBuf_put(&object->rxBuffer, rxData) == -1)
                {
                    /* no space left in RX buffer, overrun */
                    object->state.overrunCount++;
                }
            }
        }
        else
        {
            /* FIFO無効時: 1バイトのみ処理（即応性重視） */
            if (UARTCharAvailable(hwAttrs->baseAddr))
            {
                volatile uint8_t rxData = HWREG(hwAttrs->baseAddr + UART_O_DR) & 0xFF;

                if (RingBuf_put(&object->rxBuffer, rxData) == -1)
                {
                    /* no space left in RX buffer, overrun */
                    object->state.overrunCount++;
                }
            }
        }

        /* コールバック処理 */
        if (object->readCallback != NULL && object->readInUse)
        {
            /* get the data from ring buffer to the application provided buffer */
            count = RingBuf_getn(&object->rxBuffer,
                                 object->readBuf + object->readCount,
                                 object->readSize - object->readCount);
            object->readCount += count;

            if (object->readSize == object->readCount)
            {
                object->readInUse = false;
                object->readCallback(handle,
                                    (void*)object->readBuf,
                                    object->readSize,
                                    object->userArg,
                                    UART2_STATUS_SUCCESS);
                object->readCount = 0;
            }
        }

        *rxPostPtr = true;
    }
}

/*
 *  ======== UART_RegIntLPF3_hwiIntWrite ========
 *  TX interrupt handler
 *  FIFO有効/無効の両方に最適化
 */
static void UART_RegIntLPF3_hwiIntWrite(uintptr_t arg)
{
    UART2_Handle handle              = (UART2_Handle)arg;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;
    UART2LPF3_Object *object         = handle->object;
    bool isFifoEnabled = UART2LPF3_isFifoEnabled(hwAttrs);
    uint8_t txData;

    if (isFifoEnabled)
    {
        /* FIFO有効時: FIFOが満杯になるまで送信 */
        while (UARTSpaceAvailable(hwAttrs->baseAddr))
        {
            if (RingBuf_get(&object->txBuffer, &txData) == -1)
            {
                /* no more data left to transmit */
                break;
            }
            HWREG(hwAttrs->baseAddr + UART_O_DR) = txData;
            object->bytesWritten++;
        }
    }
    else
    {
        /* FIFO無効時: 1バイトのみ送信 */
        if (UARTSpaceAvailable(hwAttrs->baseAddr))
        {
            if (RingBuf_get(&object->txBuffer, &txData) != -1)
            {
                HWREG(hwAttrs->baseAddr + UART_O_DR) = txData;
                object->bytesWritten++;
            }
        }
    }

    /* バッファが空になった場合の処理 */
    if (RingBuf_getCount(&object->txBuffer) == 0)
    {
        /* no more data left to transmit, disable TX interrupt, enable
         * end of transmission interrupt for low power support.
         */
        UARTDisableInt(hwAttrs->baseAddr, UART_INT_TX);
        UARTEnableInt(hwAttrs->baseAddr, UART_INT_EOT);

        if (object->state.writeMode == UART2_Mode_CALLBACK)
        {
            object->writeInUse = false;
            object->writeCallback(handle,
                                (void *)object->writeBuf,
                                object->bytesWritten,
                                object->userArg,
                                UART2_STATUS_SUCCESS);
        }
    }
}

/*
 *  ======== UART_RegIntLPF3_hwiIntFxn ========
 *  Main interrupt handler
 */
static void UART_RegIntLPF3_hwiIntFxn(uintptr_t arg)
{
    uint32_t status;
    uint32_t event;
    uint32_t errStatus = 0;
    UART2_Handle handle              = (UART2_Handle)arg;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;
    UART2LPF3_Object *object         = handle->object;

    bool rxPost = false;
    bool txPost = false;

    /* Get and clear interrupt status */
    status = UARTIntStatus(hwAttrs->baseAddr, true);
    UARTClearInt(hwAttrs->baseAddr, status);

    /* エラー処理 */
    if ((status & (UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE)) && object->eventCallback)
    {
        object->state.overrunActive = false;

        if (status & UART_INT_OE)
        {
            /*
             *  Overrun error occurred, get what we can.
             */
            UART2LPF3_getRxData(handle, RingBuf_space(&object->rxBuffer));

            /* FIFO無効時は1バイトのみなので、すぐにクリア */
            if (!UART2LPF3_isFifoEnabled(hwAttrs))
            {
                if (UARTCharAvailable(hwAttrs->baseAddr))
                {
                    volatile uint8_t data = HWREG(hwAttrs->baseAddr + UART_O_DR);
                    (void)data;
                }
            }
            else
            {
                /* FIFO有効時は全データをクリア */
                while(!(HWREG(hwAttrs->baseAddr + UART_O_FR) & UART_FR_RXFE))
                {
                    volatile uint8_t data = HWREG(hwAttrs->baseAddr + UART_O_DR);
                    (void)data;
                }
            }

            object->state.overrunCount++;
            if (object->state.overrunActive == false)
            {
                object->state.overrunActive = true;
            }
        }

        errStatus = UARTGetRxError(hwAttrs->baseAddr);
        event     = UART_RegIntLPF3_getRxStatus(errStatus & object->eventMask);
        object->rxStatus = UART2Support_rxStatus2ErrorCode(errStatus);

        if (event && object->eventCallback)
        {
            object->eventCallback(handle, event, object->state.overrunCount, object->userArg);
        }
    }

    /* Handle RX interrupt */
    if (status & (UART_INT_RX))
    {
        UART_RegIntLPF3_hwiIntRead(arg, status, &rxPost);
    }

    /* Handle TX interrupt */
    if (status & (UART_INT_TX))
    {
        UART_RegIntLPF3_hwiIntWrite(arg);
    }

    /* Handle End of Transmission interrupt */
    if (status & (UART_INT_EOT))
    {
        UARTDisableInt(hwAttrs->baseAddr, UART_INT_EOT);
        txPost = true;

        /* End of Transmission occurred. Make sure TX FIFO is truly empty before disabling TX */
        while(UARTBusy(hwAttrs->baseAddr)){}

        /* Disable TX */
        HWREG(hwAttrs->baseAddr + UART_O_CTL) &= ~UART_CTL_TXE;

        if(object->state.txEnabled)
        {
            object->state.txEnabled = false;
            /* Release standby constraint because there are no active transactions. Also release flash constraint */
            UART2Support_powerRelConstraint(handle, true);
        }
    }

    if (rxPost)
    {
        SemaphoreP_post(&object->readSem);
    }
    if (txPost)
    {
        SemaphoreP_post(&object->writeSem);
    }
}

/*
 *  ======== UART_RegIntLPF3_initHw ========
 *  UARTハードウェアの初期化
 *  FIFO有効/無効に対応
 */
static void UART_RegIntLPF3_initHw(UART2_Handle handle)
{
    ClockP_FreqHz freq;
    UART2LPF3_Object *object         = handle->object;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;

    ClockP_getCpuFreq(&freq);

    UARTConfigSetExpClk(hwAttrs->baseAddr,
                    freq.lo,
                    object->baudRate,
                    dataLength[object->dataLength] | stopBits[object->stopBits] | parityType[object->parityType]);

    /* すべての割り込みをクリア */
    UARTClearInt(hwAttrs->baseAddr,
                 UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | UART_INT_RT | UART_INT_TX | UART_INT_RX |
                     UART_INT_CTS);

    /* FIFO動作モードに応じた設定 */
    if (UART2LPF3_isFifoEnabled(hwAttrs))
    {
        /* FIFO有効: FIFOを有効化し、割り込みレベルを設定 */
        UARTEnableFifo(hwAttrs->baseAddr);
        UARTSetFifoLevel(hwAttrs->baseAddr, TX_FIFO_INT_LEVEL, RX_FIFO_INT_LEVEL);
    }
    else
    {
        /* FIFO無効: FIFOを無効化（1バイトモード） */
        UARTDisableFifo(hwAttrs->baseAddr);

        /* FIFO無効時は受信タイムアウト割り込みも無効化 */
        UARTDisableInt(hwAttrs->baseAddr, UART_INT_RT);
    }

    /* フロー制御の設定 */
    if (UART_RegIntLPF3_isFlowControlEnabled(hwAttrs) && (hwAttrs->ctsPin != GPIO_INVALID_INDEX))
    {
        UARTEnableCTS(hwAttrs->baseAddr);
    }
    else
    {
        UARTDisableCTS(hwAttrs->baseAddr);
    }

    if (UART_RegIntLPF3_isFlowControlEnabled(hwAttrs) && (hwAttrs->rtsPin != GPIO_INVALID_INDEX))
    {
        UARTEnableRTS(hwAttrs->baseAddr);
    }
    else
    {
        UARTDisableRTS(hwAttrs->baseAddr);
    }

    /* IrDA設定 */
    if (hwAttrs->codingScheme == UART2LPF3_CODING_SIR)
    {
        /* IrDA SIRエンコーダ／デコーダを有効にする */
        HWREG(hwAttrs->baseAddr + UART_O_CTL) |= UART_CTL_SIREN;
    }

    if (hwAttrs->codingScheme == UART2LPF3_CODING_SIR_LP)
    {
        /* IrDA 省電力 SIRエンコーダ／デコーダを有効にする */
        HWREG(hwAttrs->baseAddr + UART_O_CTL) |= (UART_CTL_SIREN | UART_CTL_SIRLP);
        HWREG(hwAttrs->baseAddr + UART_O_UARTILPR) = hwAttrs->irLPClkDivider;
    }

    /* Enable the UART module, but not RX or TX */
    HWREG(hwAttrs->baseAddr + UART_O_CTL) |= UART_CTL_UARTEN;
}

/*
 *  ======== UART_RegIntLPF3_initIO ========
 *  GPIOピンを初期化する
 */
static void UART_RegIntLPF3_initIO(UART2_Handle handle)
{
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* Make sure RX and CTS pins have their input buffers enabled, then apply the correct mux */
    GPIO_setConfigAndMux(hwAttrs->txPin, GPIO_CFG_NO_DIR, hwAttrs->txPinMux);
    GPIO_setConfigAndMux(hwAttrs->rxPin, GPIO_CFG_INPUT, hwAttrs->rxPinMux);

    if (UART2LPF3_isFlowControlEnabled(hwAttrs))
    {
        GPIO_setConfigAndMux(hwAttrs->ctsPin, GPIO_CFG_INPUT, hwAttrs->ctsPinMux);
        GPIO_setConfigAndMux(hwAttrs->rtsPin, GPIO_CFG_NO_DIR, hwAttrs->rtsPinMux);
    }
}

/*
 *  ======== UART_RegIntLPF3_finalizeIO ========
 *  Restore GPIO pins to default state
 */
static void UART_RegIntLPF3_finalizeIO(UART2_Handle handle)
{
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* Reset pins to GPIO mode */
    GPIO_resetConfig(hwAttrs->txPin);
    GPIO_resetConfig(hwAttrs->rxPin);

    if (UART2LPF3_isFlowControlEnabled(hwAttrs))
    {
        GPIO_resetConfig(hwAttrs->ctsPin);
        GPIO_resetConfig(hwAttrs->rtsPin);
    }
}

/*
 *  ======== UART_RegInt_preNotifyFxn ========
 *  Called by Power module when entering LPDS.
 */
static int UART_RegInt_preNotifyFxn(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg)
{
    /* Reconfigure the IO pins back to their GPIO configuration.
     * This ensures their states are retained while in standby.
     */
    UART2_Handle handle              = (UART2_Handle)clientArg;
    UART2LPF3_HWAttrs const *hwAttrs = handle->hwAttrs;

    if (eventType == PowerLPF3_ENTERING_STANDBY)
    {
        GPIO_PinConfig txConfig;

        if (hwAttrs->codingScheme == UART2LPF3_CODING_UART)
        {
            /* If UART-encoding is used then the pin must be set high during standby */
            txConfig = GPIO_CFG_OUTPUT_INTERNAL | GPIO_CFG_OUT_STR_MED | GPIO_CFG_OUT_HIGH;
        }
        else
        {
            /* If SIR-encoding is used then the pin must be set low during standby */
            txConfig = GPIO_CFG_OUTPUT_INTERNAL | GPIO_CFG_OUT_STR_MED | GPIO_CFG_OUT_LOW;
        }

        GPIO_setConfigAndMux(hwAttrs->txPin, txConfig, GPIO_MUX_GPIO);

        if (UART2LPF3_isFlowControlEnabled(hwAttrs))
        {
            /* Assert RTS while in standby to be safe */
            GPIO_setConfigAndMux(hwAttrs->rtsPin,
                                 GPIO_CFG_OUTPUT_INTERNAL | GPIO_CFG_OUT_STR_MED | GPIO_CFG_OUT_HIGH,
                                 GPIO_MUX_GPIO);
        }
    }

    return Power_NOTIFYDONE;
}

/*
 *  ======== UART_RegInt_postNotifyFxn ========
 *  Called by Power module when waking up from LPDS.
 */
static int UART_RegInt_postNotifyFxn(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg)
{
    /* Reconfigure the hardware if returning from sleep */
    if (eventType == PowerLPF3_AWAKE_STANDBY)
    {
        UART_RegIntLPF3_initHw((UART2_Handle)clientArg);
        /* Reconfigure the IOs from GPIO back to the UART peripheral *after* the peripheral has been reconfigured */
        UART_RegIntLPF3_initIO((UART2_Handle)clientArg);
    }

    return Power_NOTIFYDONE;
}
