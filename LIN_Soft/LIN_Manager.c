/****************************************************************/
/**
 *  @file  LIN_Manager.c
 *  @if false
 *  @copyright  AISIN CORPORATION
 *  @endif
 */
/****************************************************************/
/*==============================================================*/
/*  Includes                                                    */
/*==============================================================*/
#include "l_std_def_d.h"
#include "Rte_DIGI_APP.h"
#include "l_slin_cmn.h"
#include "l_slin_def.h"
#include "l_slin_api.h"
#include "l_slin_tbl.h"
#include "l_slin_nmc.h"
#include "l_slin_drv_cc2340r53.h"

#include "LIN_Manager.h"
#include "ti_drivers_config.h"


/*==============================================================*/
/*  Types And Constants                                         */
/*==============================================================*/
#define U1L_LIN_MANAGER_FRAME_SIZE              ((u1)8U)    /*!< @brief ãƒ•ãƒ¬ãƒ¼ãƒ ã‚µã‚¤ã‚º */
#define U1L_LIN_MANAGER_DATABYTES_PER_FRAME     ((u1)7U)    /*!< @brief 1ãƒ•ãƒ¬ãƒ¼ãƒ ã‚ãŸã‚Šã®ãƒ‡ãƒ¼ã‚¿ãƒã‚¤ãƒˆæ•° */
#define U2L_LIN_MANAGER_NON_PAYLOAD_DATA_LENGTH ((u2)3U)    /*!< @brief ãƒšã‚¤ãƒ­ãƒ¼ãƒ‰ä»¥å¤–ã®ãƒ‡ãƒ¼ã‚¿é•· */

#define U1L_LIN_MANAGER_BITMASK_FRAME_COUNTINUOUS_INFO  ((u1)0x07U) /*!< @brief ãƒ“ãƒƒãƒˆãƒžã‚¹ã‚¯å€¤: ãƒ•ãƒ¬ãƒ¼ãƒ ç¶™ç¶šæƒ…å ± */

#define U1L_LIN_MANAGER_RECEIVING       ((u1)1U)    /*!< @brief å—ä¿¡ä¸­ */
#define U1L_LIN_MANAGER_NOT_RECEIVING   ((u1)0U)    /*!< @brief å—ä¿¡ä¸­ã§ã¯ãªã„ */

typedef enum {
    ENL_SINGLE = 0,             /*!< @brief å˜ç™º */
    ENL_SOF,                    /*!< @brief SoF */
    ENL_CONTINUATION,           /*!< @brief ç¶™ç¶š */
    ENL_EOF,                    /*!< @brief EoF */
} t_FrameContinuousInfoType;    /*!< @brief ãƒ•ãƒ¬ãƒ¼ãƒ ç¶™ç¶šæƒ…å ±åž‹ */

#define U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE   ((u1)10U)   /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ã‚µã‚¤ã‚º */


/*==============================================================*/
/*  Function Prototypes                                         */
/*==============================================================*/
/*!
 * @brief @b LINé€šä¿¡ç®¡ç†å—ä¿¡ãƒ¡ã‚¤ãƒ³å‡¦ç†
 * @param None
 * @return ãªã— @n
 * @details
 *      ãƒ¡ã‚¤ãƒ³å‘¨æœŸæ¯Žã«å®Ÿæ–½ã™ã‚‹LINå—ä¿¡å‡¦ç† @n
*/
static void f_LIN_Manager_MainFunctionRx( void );


/*!
 * @brief @b LINé€šä¿¡ç®¡ç†é€ä¿¡ãƒ¡ã‚¤ãƒ³å‡¦ç†
 * @param None
 * @return ãªã— @n
 * @details
 *      ãƒ¡ã‚¤ãƒ³å‘¨æœŸæ¯Žã«å®Ÿæ–½ã™ã‚‹LINé€ä¿¡å‡¦ç† @n
*/static void f_LIN_Manager_MainFunctionTx( void );


/*!
 * @brief @b å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚¯ãƒªã‚¢
 * @param None
 * @return ãªã— @n
 * @details
 *      å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚’ã‚¯ãƒªã‚¢ã™ã‚‹ @n
*/
static void f_LIN_Manager_ClearRxData( void );


/*!
 * @brief @b é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚¯ãƒªã‚¢
 * @param None
 * @return ãªã— @n
 * @details
 *      é€ä¿¡ãƒ‡ãƒ¼ã‚¿(ã‚­ãƒ¥ãƒ¼)ã‚’ã‚¯ãƒªã‚¢ã™ã‚‹ @n
*/
static void f_LIN_Manager_ClearTxData( void );


/*!
 * @brief @b ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹ã‹ãƒã‚§ãƒƒã‚¯
 * @param None
 * @return TRUE=ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹, FALSE=ã‚­ãƒ¥ãƒ¼ãŒãªã„ @n
 * @details
 *      ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹ã‹ãªã„ã‹ã‚’ãƒã‚§ãƒƒã‚¯ã™ã‚‹ @n
*/
static u1 f_LIN_Manager_HasTxDataQueue( void );


/*!
 * @brief @b é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚¨ãƒ³ã‚­ãƒ¥ãƒ¼
 * @param u1 é€ä¿¡ãƒ‡ãƒ¼ã‚¿é•·
 * @param u1* é€ä¿¡ãƒ‡ãƒ¼ã‚¿
 * @return ãªã— @n
 * @details
 *      é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚’ã‚­ãƒ¥ãƒ¼ã«æ ¼ç´ã™ã‚‹ @n
*/
static void f_LIN_Manager_EnqueueTxData( u1 u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] );


/*!
 * @brief @b é€ä¿¡ãƒ‡ãƒ¼ã‚¿ãƒ‡ã‚­ãƒ¥ãƒ¼
 * @param u1* [out] é€ä¿¡ãƒ‡ãƒ¼ã‚¿é•·
 * @param u1* [out] é€ä¿¡ãƒ‡ãƒ¼ã‚¿
 * @return ãªã— @n
 * @details
 *      é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚’ã‚­ãƒ¥ãƒ¼ã‹ã‚‰å–ã‚Šå‡ºã™ @n
*/
static void f_LIN_Manager_DequeueTxData( u1* u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] );


/*!
 * @brief @b ãƒ‡ãƒ¼ã‚¿é•·è¨ˆç®—
 * @param u1* ãƒ‡ãƒ¼ã‚¿
 * @return ãƒ‡ãƒ¼ã‚¿é•· @n
 * @details
 *      ãƒ‡ãƒ¼ã‚¿ã‹ã‚‰ãƒ‡ãƒ¼ã‚¿é•·ã‚’ç®—å‡ºã™ã‚‹ @n
*/
static u2 f_LIN_Manager_CalcDataLength( u1* u1a_Data );


/*!
 * @brief @b D1ãƒ‡ãƒ¼ã‚¿ç”Ÿæˆ
 * @param t_FrameContinuousInfoType ãƒ•ãƒ¬ãƒ¼ãƒ ç¶™ç¶šæƒ…å ±
 * @return D1ãƒ‡ãƒ¼ã‚¿ @n
 * @details
 *      ãƒ•ãƒ¬ãƒ¼ãƒ ã®å…ˆé ­(Data1)ã«æ ¼ç´ã™ã‚‹ãƒ‡ãƒ¼ã‚¿ã‚’ç”Ÿæˆã™ã‚‹ @n
*/
static u1 f_LIN_Manager_GenD1( t_FrameContinuousInfoType ena_FrameContinuousInfo );


/*==============================================================*/
/*  Data Prototypes                                             */
/*==============================================================*/
static u1 u1l_LIN_Manager_RxData[U2G_LIN_MANAGER_MAX_PAYLOAD_SIZE];     /*!< @brief å—ä¿¡ãƒ‡ãƒ¼ã‚¿ */
static u2 u2l_LIN_Manager_RxDataLength;                                 /*!< @brief å—ä¿¡ãƒ‡ãƒ¼ã‚¿é•· */
static u1 u1l_LIN_Manager_ReceivingContinuousFrame;                     /*!< @brief ç¶™ç¶šãƒ•ãƒ¬ãƒ¼ãƒ å—ä¿¡ä¸­: 1=å—ä¿¡ä¸­, 0=å—ä¿¡ä¸­ã§ã¯ãªã„ */
static u2 u2l_LIN_Manager_TempRxDataLength;                             /*!< @brief ä»®å—ä¿¡ãƒ‡ãƒ¼ã‚¿é•· */
static u1 u1l_LIN_Manager_TempRxData[U1L_LIN_MANAGER_FRAME_SIZE];       /*!< @brief ä»®å—ä¿¡ãƒ‡ãƒ¼ã‚¿ */
static u1 u1l_LIN_Manager_TempRxDataStoreIndex;                         /*!< @brief ä»®å—ä¿¡ãƒ‡ãƒ¼ã‚¿æ ¼ç´ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

static u1 u1l_LIN_Manager_TxDataQueue[U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE][U1L_LIN_MANAGER_FRAME_SIZE];   /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ */
static u1 u1l_LIN_Manager_TxDataLengthQueue[U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE];                         /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿é•·ã‚­ãƒ¥ãƒ¼ */
static u1 u1l_LIN_Manager_TxDataQueueHead;                                                              /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼å…ˆé ­ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */
static u1 u1l_LIN_Manager_TxDataQueueTail;                                                              /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼æœ«å°¾ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */
static u1 u1l_LIN_Manager_TxData[U1L_LIN_MANAGER_FRAME_SIZE];                                           /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿ */
static u1 u1l_LIN_Manager_TxDataLength;                                                                 /*!< @brief é€ä¿¡ãƒ‡ãƒ¼ã‚¿é•· */

static u1 u1l_LIN_Manager_TxFrameComplete_ID30h;                        /*!< @brief ID30é€ä¿¡å®Œäº†ãƒ•ãƒ©ã‚°: TRUE=é€ä¿¡å®Œäº†, FALSE=é€ä¿¡æœªå®Œäº† */

static u1 u1l_LIN_Manager_slpreq;

/*==============================================================*/
/*  Functions                                                   */
/*==============================================================*/
void f_LIN_Manager_Initialize( void )
{
    u1 u1a_DataIndex;   /* ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

    (void)l_sys_init();                         /* ã‚·ã‚¹ãƒ†ãƒ åˆæœŸåŒ–API */

    l_ifc_init_ch1();                            /* ãƒãƒƒãƒ•ã‚¡ã€ãƒ‰ãƒ©ã‚¤ãƒã®åˆæœŸåŒ–API */
    l_nm_init_ch1();                             /* NMç”¨ åˆæœŸåŒ–API */

	b1g_LIN_TxFrameComplete_ID30h = U1G_DAT_BIT_ON; /* åˆå›žã‹ã‚‰é€ä¿¡ã§ãã‚‹ã‚ˆã†å®Œäº†ã«è¨­å®šã—ã¦ãŠã */

    (void)l_ifc_connect_ch1();                  /* LINé€šä¿¡æŽ¥ç¶šAPI */

    f_LIN_Manager_ClearRxData();
    f_LIN_Manager_ClearTxData();

    u1l_LIN_Manager_ReceivingContinuousFrame = U1L_LIN_MANAGER_NOT_RECEIVING;
    u2l_LIN_Manager_TempRxDataLength         = U1G_DAT_ZERO;
    u1l_LIN_Manager_TempRxDataStoreIndex     = U1G_DAT_ZERO;
    for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_DataIndex++ )
    {
        u1l_LIN_Manager_TempRxData[u1a_DataIndex] = U1G_DAT_CLR;
    }

    u1l_LIN_Manager_TxFrameComplete_ID30h = U1G_DAT_TRUE;

    u1l_LIN_Manager_slpreq = U1G_DAT_FALSE;

    GPIO_write(CONFIG_LIN_EN, U1G_DAT_ON);
}

void f_LIN_Manager_MainFunction( void )
{
    if( U1G_DAT_FALSE == u1l_LIN_Manager_slpreq )
    {
    	l_nm_tick_ch1( U1G_LIN_SLP_REQ_OFF );
    }
    else
    {
    	l_nm_tick_ch1( U1G_LIN_SLP_REQ_ON );
    }
    f_LIN_Manager_MainFunctionRx();
    f_LIN_Manager_MainFunctionTx();
}


void f_LIN_Manager_GetRxData( u2* u2a_DataLength, u1* u1a_Data )
{
    u2 u2a_DataIndex;   /* ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

    *u2a_DataLength = u2l_LIN_Manager_RxDataLength;
    if ( u2l_LIN_Manager_RxDataLength > U1G_DAT_ZERO )
    {
        for ( u2a_DataIndex = U2G_DAT_ZERO; u2a_DataIndex < u2l_LIN_Manager_RxDataLength; u2a_DataIndex++ )
        {
            u1a_Data[u2a_DataIndex] = u1l_LIN_Manager_RxData[u2a_DataIndex];
        }
        f_LIN_Manager_ClearRxData();
    }
}


void f_LIN_Manager_SetTxData( u2 u2a_DataLength, const u1* u1a_Data )
{
    u1 u1a_FullSizeFrameNum;            /* ãƒ•ãƒ«ã‚µã‚¤ã‚ºãƒ•ãƒ¬ãƒ¼ãƒ æ•° */
    u1 u1a_Surplus;                     /* ä½™ã‚Šãƒã‚¤ãƒˆæ•° */
    u1 u1a_FrameDivisionNum;            /* ãƒ•ãƒ¬ãƒ¼ãƒ åˆ†å‰²æ•° */
    u1 u1a_FrameIndex;                  /* ãƒ•ãƒ¬ãƒ¼ãƒ ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */
    u1 u1a_WriteFrameData[8];           /* æ›¸ãè¾¼ã¿ãƒ•ãƒ¬ãƒ¼ãƒ ãƒ‡ãƒ¼ã‚¿ */
    u1 u1a_WriteFramePos;               /* æ›¸ãè¾¼ã¿ãƒ‡ãƒ¼ã‚¿ä½ç½® */
    u1 u1a_WriteDataIndex;              /* æ›¸ãè¾¼ã¿ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

    u1a_FullSizeFrameNum = (u1)(u2a_DataLength / U1L_LIN_MANAGER_DATABYTES_PER_FRAME);
    u1a_Surplus          = (u1)(u2a_DataLength - ((u2)u1a_FullSizeFrameNum * (u2)U1L_LIN_MANAGER_DATABYTES_PER_FRAME));
    u1a_FrameDivisionNum = u1a_FullSizeFrameNum;
    if ( u1a_Surplus > (u1)0U )
    {
        u1a_FrameDivisionNum++;
    }

    /* å˜ç™ºãƒ•ãƒ¬ãƒ¼ãƒ ã®å ´åˆ */
    if ( u1a_FrameDivisionNum == (u1)1U )
    {
        u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_SINGLE );
        for ( u1a_WriteDataIndex = U1G_DAT_ZERO; u1a_WriteDataIndex < (u1)u2a_DataLength; u1a_WriteDataIndex++ )
        {
            u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = u1a_Data[u1a_WriteDataIndex];
        }
        for ( u1a_WriteDataIndex = (u1)u2a_DataLength; u1a_WriteDataIndex < U1L_LIN_MANAGER_DATABYTES_PER_FRAME; u1a_WriteDataIndex++ )
        {
            u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = (u1)0x00;
        }
        f_LIN_Manager_EnqueueTxData( U1L_LIN_MANAGER_FRAME_SIZE, &u1a_WriteFrameData[0] );
    }
    /* ç¶™ç¶šãƒ•ãƒ¬ãƒ¼ãƒ ã®å ´åˆ */
    else
    {
        u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_SOF );
        for ( u1a_WriteDataIndex = U1G_DAT_ZERO; u1a_WriteDataIndex < U1L_LIN_MANAGER_DATABYTES_PER_FRAME; u1a_WriteDataIndex++ )
        {
            u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = u1a_Data[u1a_WriteDataIndex];
        }
        f_LIN_Manager_EnqueueTxData( U1L_LIN_MANAGER_FRAME_SIZE, &u1a_WriteFrameData[0] );

        u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_CONTINUATION );
        /* ä½™ã‚Šãƒã‚¤ãƒˆãŒãªã„å ´åˆ */
        if ( u1a_FullSizeFrameNum == u1a_FrameDivisionNum )
        {
            for ( u1a_FrameIndex = (u1)1U; u1a_FrameIndex < u1a_FullSizeFrameNum; u1a_FrameIndex++ )
            {
                if ( u1a_FrameIndex == (u1a_FullSizeFrameNum - (u1)1U) )
                {
                    u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_EOF );
                }

                u1a_WriteFramePos = u1a_FrameIndex * U1L_LIN_MANAGER_DATABYTES_PER_FRAME;
                for ( u1a_WriteDataIndex = U1G_DAT_ZERO; u1a_WriteDataIndex < U1L_LIN_MANAGER_DATABYTES_PER_FRAME; u1a_WriteDataIndex++ )
                {
                    u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = u1a_Data[u1a_WriteFramePos + u1a_WriteDataIndex];
                }
                f_LIN_Manager_EnqueueTxData( U1L_LIN_MANAGER_FRAME_SIZE, &u1a_WriteFrameData[0] );
            }
        }
        /* ä½™ã‚Šãƒã‚¤ãƒˆãŒã‚ã‚‹å ´åˆ */
        else
        {
            for ( u1a_FrameIndex = (u1)1U; u1a_FrameIndex < u1a_FullSizeFrameNum; u1a_FrameIndex++ )
            {
                u1a_WriteFramePos = u1a_FrameIndex * U1L_LIN_MANAGER_DATABYTES_PER_FRAME;
                for ( u1a_WriteDataIndex = U1G_DAT_ZERO; u1a_WriteDataIndex < U1L_LIN_MANAGER_DATABYTES_PER_FRAME; u1a_WriteDataIndex++ )
                {
                    u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = u1a_Data[u1a_WriteFramePos + u1a_WriteDataIndex];
                }
                f_LIN_Manager_EnqueueTxData( U1L_LIN_MANAGER_FRAME_SIZE, &u1a_WriteFrameData[0] );
            }

            u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_EOF );
            u1a_WriteFramePos     = u1a_FullSizeFrameNum * U1L_LIN_MANAGER_DATABYTES_PER_FRAME;
            for ( u1a_WriteDataIndex = U1G_DAT_ZERO; u1a_WriteDataIndex < u1a_Surplus; u1a_WriteDataIndex++ )
            {
                u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = u1a_Data[u1a_WriteFramePos + u1a_WriteDataIndex];
            }
            for ( u1a_WriteDataIndex = u1a_Surplus; u1a_WriteDataIndex < U1L_LIN_MANAGER_DATABYTES_PER_FRAME; u1a_WriteDataIndex++ )
            {
                u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = (u1)0x00;
            }
            f_LIN_Manager_EnqueueTxData( U1L_LIN_MANAGER_FRAME_SIZE, &u1a_WriteFrameData[0] );
        }
    }
}


static void f_LIN_Manager_MainFunctionRx( void )
{
    u1 u1a_isReceived = U1G_DAT_FALSE;              /* å—ä¿¡ã‚ã‚Š */
    u1 u1a_TempRxFrame[U1L_LIN_MANAGER_FRAME_SIZE]; /* ä»®å—ä¿¡ãƒ•ãƒ¬ãƒ¼ãƒ  */
    u1 u1a_TempRxDataIndex;                         /* ä»®å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */
    u2 u2a_RxDataIndex;                             /* å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */
    u1 u1a_FrameCountinuousInfo;                    /* ãƒ•ãƒ¬ãƒ¼ãƒ ç¶™ç¶šæƒ…å ± */

    l_u1g_lin_irq_dis();                                                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢ */
    if ( b1g_LIN_RxFrameComplete_ID20h == U1G_DAT_BIT_ON )
    {
        u1a_isReceived = U1G_DAT_TRUE;
        b1g_LIN_RxFrameComplete_ID20h = U1G_DAT_BIT_OFF;
        l_slot_rd_ch1( U1G_LIN_FRAME_ID20H, U1L_LIN_MANAGER_FRAME_SIZE, &u1a_TempRxFrame[0] );
    }
    l_vog_lin_irq_res();                                                        /* å‰²ã‚Šè¾¼ã¿è¨±å¯ */

    /* å—ä¿¡ã‚ã‚Šã®å ´åˆ */
    if ( u1a_isReceived == U1G_DAT_TRUE )
    {
        u1a_FrameCountinuousInfo = u1a_TempRxFrame[0] & U1L_LIN_MANAGER_BITMASK_FRAME_COUNTINUOUS_INFO;
        switch ( u1a_FrameCountinuousInfo )
        {
        case (u1)ENL_SINGLE:
            u1l_LIN_Manager_ReceivingContinuousFrame = U1L_LIN_MANAGER_NOT_RECEIVING;
            for ( u1a_TempRxDataIndex = (u1)1U; u1a_TempRxDataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_TempRxDataIndex++ )
            {
                u1l_LIN_Manager_RxData[(u2)u1a_TempRxDataIndex - (u2)1U] = u1a_TempRxFrame[u1a_TempRxDataIndex];
            }
            u2l_LIN_Manager_RxDataLength = f_LIN_Manager_CalcDataLength( &u1a_TempRxFrame[0] );
            break;
        case (u1)ENL_SOF:
            u1l_LIN_Manager_ReceivingContinuousFrame = U1L_LIN_MANAGER_RECEIVING;
            for ( u1a_TempRxDataIndex = (u1)1U; u1a_TempRxDataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_TempRxDataIndex++ )
            {
                u1l_LIN_Manager_TempRxData[(u2)u1a_TempRxDataIndex - (u2)1U] = u1a_TempRxFrame[u1a_TempRxDataIndex];
            }
            u2l_LIN_Manager_TempRxDataLength = f_LIN_Manager_CalcDataLength( &u1a_TempRxFrame[0] );
            u1l_LIN_Manager_TempRxDataStoreIndex = U1L_LIN_MANAGER_DATABYTES_PER_FRAME;
            break;
        case (u1)ENL_CONTINUATION:
            if ( u1l_LIN_Manager_ReceivingContinuousFrame == U1L_LIN_MANAGER_RECEIVING )
            {
                for ( u1a_TempRxDataIndex = (u1)1U; u1a_TempRxDataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_TempRxDataIndex++ )
                {
                    u1l_LIN_Manager_TempRxData[(u2)u1l_LIN_Manager_TempRxDataStoreIndex + (u2)u1a_TempRxDataIndex - (u2)1U] = u1a_TempRxFrame[u1a_TempRxDataIndex];
                }
                u1l_LIN_Manager_TempRxDataStoreIndex += U1L_LIN_MANAGER_DATABYTES_PER_FRAME;
            }
            break;
        case (u1)ENL_EOF:
            if ( u1l_LIN_Manager_ReceivingContinuousFrame == U1L_LIN_MANAGER_RECEIVING )
            {
                u1l_LIN_Manager_ReceivingContinuousFrame = U1L_LIN_MANAGER_NOT_RECEIVING;
                for ( u1a_TempRxDataIndex = (u1)1U; u1a_TempRxDataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_TempRxDataIndex++ )
                {
                    u1l_LIN_Manager_TempRxData[(u2)u1l_LIN_Manager_TempRxDataStoreIndex + (u2)u1a_TempRxDataIndex - (u2)1U] = u1a_TempRxFrame[u1a_TempRxDataIndex];
                }

                /* å—ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚’ç¢ºå®šã™ã‚‹ */
                u2l_LIN_Manager_RxDataLength = u2l_LIN_Manager_TempRxDataLength;
                for ( u2a_RxDataIndex = U1G_DAT_ZERO; u2a_RxDataIndex < u2l_LIN_Manager_RxDataLength; u2a_RxDataIndex++ )
                {
                    u1l_LIN_Manager_RxData[u2a_RxDataIndex] = u1l_LIN_Manager_TempRxData[u2a_RxDataIndex];
                }
            }
            break;
        default:
            break;
        }
    }
}


static void f_LIN_Manager_MainFunctionTx( void )
{
    u1 u1a_HasTxDataQueue;                      /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹ã‹ãƒã‚§ãƒƒã‚¯çµæžœ */

    /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹ä¸”ã¤ã€é€ä¿¡ã§ãã‚‹çŠ¶æ…‹ã§ã‚ã‚‹å ´åˆ */
    u1a_HasTxDataQueue = f_LIN_Manager_HasTxDataQueue();
    if ( ( u1a_HasTxDataQueue == U1G_DAT_TRUE )
      && ( u1l_LIN_Manager_TxFrameComplete_ID30h == U1G_DAT_TRUE ) )
    {
        f_LIN_Manager_DequeueTxData( &u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
        l_u1g_lin_irq_dis();                                                        /* å‰²ã‚Šè¾¼ã¿ç¦æ­¢ */
        u1l_LIN_Manager_TxFrameComplete_ID30h = U1G_DAT_FALSE;
        l_slot_wr_ch1( U1G_LIN_FRAME_ID30H, u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
        l_vog_lin_irq_res();                                                        /* å‰²ã‚Šè¾¼ã¿è¨±å¯ */
    }
}


static void f_LIN_Manager_ClearRxData( void )
{
    u2 u2a_DataIndex;

    u2l_LIN_Manager_RxDataLength = U2G_DAT_CLR;
    for ( u2a_DataIndex = U2G_DAT_ZERO; u2a_DataIndex < U2G_LIN_MANAGER_MAX_PAYLOAD_SIZE; u2a_DataIndex++ )
    {
        u1l_LIN_Manager_RxData[u2a_DataIndex] = U1G_DAT_CLR;
    }
}


static void f_LIN_Manager_ClearTxData( void )
{
    u1 u1a_QueueIndex;  /* ã‚­ãƒ¥ãƒ¼ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */
    u1 u1a_DataIndex;   /* ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

    u1l_LIN_Manager_TxDataQueueHead = U1G_DAT_CLR;
    u1l_LIN_Manager_TxDataQueueTail = U1G_DAT_CLR;
    for ( u1a_QueueIndex = U1G_DAT_ZERO; u1a_QueueIndex < U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE; u1a_QueueIndex++ )
    {
        u1l_LIN_Manager_TxDataLengthQueue[u1a_QueueIndex] = U1G_DAT_CLR;
        for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_DataIndex++ )
        {
            u1l_LIN_Manager_TxDataQueue[u1a_QueueIndex][u1a_DataIndex] = U1G_DAT_CLR;
        }
    }

    u1l_LIN_Manager_TxDataLength = U1G_DAT_CLR;
    for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < U1L_LIN_MANAGER_FRAME_SIZE; u1a_DataIndex++ )
    {
        u1l_LIN_Manager_TxData[u1a_DataIndex] = U1G_DAT_CLR;
    }
}


static u1 f_LIN_Manager_HasTxDataQueue( void )
{
    u1 u1a_HasQueue = U1G_DAT_FALSE;    /* ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹ã‹: TRUE=ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹, FALSE=ã‚­ãƒ¥ãƒ¼ãŒãªã„ */

    /* ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹å ´åˆ */
    if ( u1l_LIN_Manager_TxDataQueueHead != u1l_LIN_Manager_TxDataQueueTail )
    {
        u1a_HasQueue = U1G_DAT_TRUE;
    }

    return u1a_HasQueue;
}


static void f_LIN_Manager_EnqueueTxData( u1 u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] )
{
    u1 u1a_DataIndex;   /* ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

    /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼æœ«å°¾ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ã‚’é€²ã‚ã‚‹ */
    if ( (u1l_LIN_Manager_TxDataQueueTail + (u1)1U) < U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE )
    {
        u1l_LIN_Manager_TxDataQueueTail++;
    }
    else
    {
        u1l_LIN_Manager_TxDataQueueTail = U1G_DAT_ZERO;
    }

    /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼æœ«å°¾ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ãŒæŒ‡ã™å ´æ‰€ã«ãƒ‡ãƒ¼ã‚¿ã‚’æ ¼ç´ã™ã‚‹ */
    u1l_LIN_Manager_TxDataLengthQueue[u1l_LIN_Manager_TxDataQueueTail] = u1a_DataLength;
    for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < u1a_DataLength; u1a_DataIndex++ )
    {
        u1l_LIN_Manager_TxDataQueue[u1l_LIN_Manager_TxDataQueueTail][u1a_DataIndex] = u1a_Data[u1a_DataIndex];
    }
}



static void f_LIN_Manager_DequeueTxData( u1* u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] )
{
    u1 u1a_DataIndex;   /* ãƒ‡ãƒ¼ã‚¿ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ */

    /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼å…ˆé ­ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ã‚’é€²ã‚ã‚‹ */
    if ( (u1l_LIN_Manager_TxDataQueueHead + (u1)1U) < U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE )
    {
        u1l_LIN_Manager_TxDataQueueHead++;
    }
    else
    {
        u1l_LIN_Manager_TxDataQueueHead = U1G_DAT_ZERO;
    }

    /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼å…ˆé ­ã‚¤ãƒ³ãƒ‡ãƒƒã‚¯ã‚¹ãŒæŒ‡ã™å ´æ‰€ã‹ã‚‰ãƒ‡ãƒ¼ã‚¿ã‚’å–ã‚Šå‡ºã™ */
    *u1a_DataLength = u1l_LIN_Manager_TxDataLengthQueue[u1l_LIN_Manager_TxDataQueueHead];
    for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < *u1a_DataLength; u1a_DataIndex++ )
    {
        u1a_Data[u1a_DataIndex] = u1l_LIN_Manager_TxDataQueue[u1l_LIN_Manager_TxDataQueueHead][u1a_DataIndex];
    }
}


static u2 f_LIN_Manager_CalcDataLength( u1* u1a_Data )
{
    u2 u2a_DataLength;  /* ãƒ‡ãƒ¼ã‚¿é•· */

    /* NFC_DATA_MESSAGE_è¦æ±‚/å¿œç­”ã®å ´åˆ */
    if ( u1a_Data[1] == (u1)0x00 )
    {
        u2a_DataLength  = (u2)u1a_Data[3] + U2L_LIN_MANAGER_NON_PAYLOAD_DATA_LENGTH;
        u2a_DataLength += ((u2)u1a_Data[2] << 8);
    }
    /* NFC_DATA_MESSAGE_è¦æ±‚/å¿œç­”ä»¥å¤–ã®å ´åˆ */
    else
    {
        u2a_DataLength = (u2)u1a_Data[3] + U2L_LIN_MANAGER_NON_PAYLOAD_DATA_LENGTH;
    }

    return u2a_DataLength;
}


static u1 f_LIN_Manager_GenD1( t_FrameContinuousInfoType ena_FrameContinuousInfo )
{
    u1 u1a_Data1;   /* D1ãƒ‡ãƒ¼ã‚¿ */

    u1a_Data1  = (u1)0x20U;                 /* Sleep.ind=0, Wake up.ind=0, Data.ind=1, æ©Ÿèƒ½åˆ¥è­˜åˆ¥æƒ…å ±=0(NFCæ©Ÿèƒ½) */
    u1a_Data1 |= ena_FrameContinuousInfo;

    return u1a_Data1;
}


void f_LIN_Manager_Callback_TxComplete( u1 u1a_frame, u1 u1a_DataLength, u1* u1a_Data )
{
    u1 u1a_HasTxDataQueue;                      /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹ã‹ãƒã‚§ãƒƒã‚¯çµæžœ */

    if ( u1a_frame == U1G_LIN_FRAME_ID30H )
    {
        if ( ( u1a_Data[0] == u1l_LIN_Manager_TxData[0] )
          && ( u1a_Data[1] == u1l_LIN_Manager_TxData[1] )
          && ( u1a_Data[2] == u1l_LIN_Manager_TxData[2] )
          && ( u1a_Data[3] == u1l_LIN_Manager_TxData[3] )
          && ( u1a_Data[4] == u1l_LIN_Manager_TxData[4] )
          && ( u1a_Data[5] == u1l_LIN_Manager_TxData[5] )
          && ( u1a_Data[6] == u1l_LIN_Manager_TxData[6] )
          && ( u1a_Data[7] == u1l_LIN_Manager_TxData[7] ) )
        {
            /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ãŒã‚ã‚‹å ´åˆ */
            u1a_HasTxDataQueue = f_LIN_Manager_HasTxDataQueue();
            if ( u1a_HasTxDataQueue == U1G_DAT_TRUE )
            {
                f_LIN_Manager_DequeueTxData( &u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
                l_slot_wr_ch1( U1G_LIN_FRAME_ID30H, u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
            }
            /* é€ä¿¡ãƒ‡ãƒ¼ã‚¿ã‚­ãƒ¥ãƒ¼ãŒãªã„å ´åˆ */
            else
            {
                u1l_LIN_Manager_TxFrameComplete_ID30h = U1G_DAT_TRUE;
            }
        }
        else
        {
            /* ç•°å¸¸ */
        }
    }
}

void f_LIN_Manager_set_slpreq( u1 u1a_LIN_Manager_slpreq )
{
    u1l_LIN_Manager_slpreq = u1a_LIN_Manager_slpreq;
}


void f_LIN_Manager_Driver_sleep( void )
{
    GPIO_write(CONFIG_LIN_EN, U1G_DAT_OFF);
}
/**** End of File *********************************************/
