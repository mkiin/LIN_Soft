/*
 * UART_RegInt Configuration Template
 * 
 * This file provides a template for configuring the UART_RegInt driver.
 * Copy this content to your ti_drivers_config.c file and modify as needed.
 */

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)

#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC23X0.h>

#include "UART_RegInt.h"
#include "uart_regint/UART_RegIntLPF3.h"

/*
 *  ======== UART_RegInt Configuration ========
 */

/* Number of UART instances */
#define CONFIG_UART_REGINT_COUNT 1

/* UART instance objects */
UART_RegIntLPF3_Object uart_RegIntLPF3Objects[CONFIG_UART_REGINT_COUNT];

/* UART hardware attributes */
const UART_RegIntLPF3_HWAttrs uart_RegIntLPF3HWAttrs[CONFIG_UART_REGINT_COUNT] = {
    /* UART0 Configuration */
    {
        /* Base hardware attributes */
        .baseAddr     = UART0_BASE,               /* UART0 base address: 0x40034000 */
        .intNum       = INT_UART0_COMB,           /* UART0 combined interrupt: 27 */
        .intPriority  = 0x20,                     /* Interrupt priority (0x00-0xE0, 0xFF=default) */
        
        /* Ring buffer configuration (optional, set to NULL/0 if not used) */
        .rxBufPtr     = NULL,                     /* Pointer to RX ring buffer */
        .rxBufSize    = 0,                        /* RX ring buffer size */
        .txBufPtr     = NULL,                     /* Pointer to TX buffer */
        .txBufSize    = 0,                        /* TX buffer size */
        
        /* GPIO pin configuration */
        .flowControl  = UART_REGINT_FLOWCTRL_NONE, /* Flow control: NONE or HARDWARE */
        .rxPin        = CONFIG_GPIO_UART_0_RX,    /* RX GPIO pin index */
        .txPin        = CONFIG_GPIO_UART_0_TX,    /* TX GPIO pin index */
        .ctsPin       = GPIO_INVALID_INDEX,       /* CTS GPIO pin (if flow control enabled) */
        .rtsPin       = GPIO_INVALID_INDEX,       /* RTS GPIO pin (if flow control enabled) */
        
        /* Pin multiplexing (device-specific, see datasheet) */
        .txPinMux     = GPIO_MUX_PORTCFG_PFUNC3,  /* TX pin mux value */
        .rxPinMux     = GPIO_MUX_PORTCFG_PFUNC3,  /* RX pin mux value */
        .ctsPinMux    = GPIO_MUX_GPIO_INTERNAL,   /* CTS pin mux value */
        .rtsPinMux    = GPIO_MUX_GPIO_INTERNAL,   /* RTS pin mux value */
        
        /* Power management */
        .powerID      = PowerLPF3_PERIPH_UART0,   /* Power resource ID */
    },
};

/* UART configuration table */
const UART_RegInt_Config UART_RegInt_config[CONFIG_UART_REGINT_COUNT] = {
    {
        .object  = &uart_RegIntLPF3Objects[0],
        .hwAttrs = &uart_RegIntLPF3HWAttrs[0]
    },
};

/* UART instance count */
const uint_least8_t UART_RegInt_count = CONFIG_UART_REGINT_COUNT;

/*
 *  ======== Configuration Notes ========
 *
 *  1. Base Address and Interrupt Number:
 *     - UART0: baseAddr = UART0_BASE (0x40034000), intNum = INT_UART0_COMB (27)
 *     - These values are defined in hw_memmap.h and hw_ints.h
 *
 *  2. Interrupt Priority:
 *     - Valid range: 0x00 to 0xE0 (CC23X0 uses 3-bit priority)
 *     - Lower value = higher priority
 *     - 0x00 is not recommended (zero-latency interrupt, no critical sections)
 *     - 0xFF uses default priority (0x20)
 *
 *  3. GPIO Pin Configuration:
 *     - rxPin, txPin: Must match your board's GPIO configuration
 *     - These are indices from your GPIO configuration table
 *     - Example: CONFIG_GPIO_UART_0_RX, CONFIG_GPIO_UART_0_TX
 *
 *  4. Pin Multiplexing:
 *     - Depends on the specific device and pin assignment
 *     - Refer to the device datasheet for correct mux values
 *     - Common values:
 *       * GPIO_MUX_PORTCFG_PFUNC1 through PFUNC8 for peripheral functions
 *       * GPIO_MUX_GPIO_INTERNAL for unused pins
 *
 *  5. Flow Control:
 *     - UART_REGINT_FLOWCTRL_NONE: No flow control (most common)
 *     - UART_REGINT_FLOWCTRL_HARDWARE: Use CTS/RTS (requires ctsPin, rtsPin)
 *
 *  6. Ring Buffers (Optional):
 *     - Can be used for continuous receive buffering
 *     - If not needed, set rxBufPtr = NULL and rxBufSize = 0
 *     - Example with buffers:
 *       static uint8_t rxRingBuf[128];
 *       .rxBufPtr = rxRingBuf,
 *       .rxBufSize = 128,
 *
 *  7. Power Management:
 *     - powerID identifies the UART peripheral for clock control
 *     - UART0: PowerLPF3_PERIPH_UART0
 *     - The driver automatically manages power dependencies
 *
 *  8. Multiple UART Instances:
 *     - To add more UARTs, increase CONFIG_UART_REGINT_COUNT
 *     - Add corresponding entries to objects, hwAttrs, and config arrays
 *     - Example:
 *       #define CONFIG_UART_REGINT_COUNT 2
 *       UART_RegIntLPF3_Object uart_RegIntLPF3Objects[2];
 *       const UART_RegIntLPF3_HWAttrs uart_RegIntLPF3HWAttrs[2] = {
 *           { /* UART0 config */ },
 *           { /* UART1 config */ },
 *       };
 */

/*
 *  ======== Example GPIO Configuration ========
 *
 *  This is an example of how GPIO pins might be configured in your
 *  ti_drivers_config.c file. The actual configuration depends on your
 *  board design.
 *
 *  #define CONFIG_GPIO_UART_0_RX  0
 *  #define CONFIG_GPIO_UART_0_TX  1
 *
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *      // UART0 RX - DIO12
 *      GPIO_CFG_INPUT | GPIO_CFG_PULL_UP | GPIO_DIO12,
 *      // UART0 TX - DIO13
 *      GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW | GPIO_DIO13,
 *  };
 */

/*
 *  ======== LIN Communication Example Configuration ========
 *
 *  For LIN communication at 9600 bps with callback mode:
 *
 *  UART_RegInt_Params params;
 *  UART_RegInt_Params_init(&params);
 *  params.baudRate = 9600;
 *  params.dataLength = UART_REGINT_DataLen_8;
 *  params.stopBits = UART_REGINT_StopBits_1;
 *  params.parityType = UART_REGINT_Parity_NONE;
 *  params.readMode = UART_REGINT_MODE_CALLBACK;
 *  params.writeMode = UART_REGINT_MODE_CALLBACK;
 *  params.readCallback = lin_rx_callback;
 *  params.writeCallback = lin_tx_callback;
 *  params.eventCallback = lin_error_callback;
 *  params.eventMask = UART_REGINT_EVENT_OVERRUN | UART_REGINT_EVENT_FRAMING;
 *
 *  UART_RegInt_Handle handle = UART_RegInt_open(0, &params);
 */
