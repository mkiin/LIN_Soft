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
#define U1L_LIN_MANAGER_FRAME_SIZE              ((u1)8U)    /*!< @brief フレームサイズ */
#define U1L_LIN_MANAGER_DATABYTES_PER_FRAME     ((u1)7U)    /*!< @brief 1フレームあたりのデータバイト数 */
#define U2L_LIN_MANAGER_NON_PAYLOAD_DATA_LENGTH ((u2)3U)    /*!< @brief ペイロード以外のデータ長 */

#define U1L_LIN_MANAGER_BITMASK_FRAME_COUNTINUOUS_INFO  ((u1)0x07U) /*!< @brief ビットマスク値: フレーム継続情報 */

#define U1L_LIN_MANAGER_RECEIVING       ((u1)1U)    /*!< @brief 受信中 */
#define U1L_LIN_MANAGER_NOT_RECEIVING   ((u1)0U)    /*!< @brief 受信中ではない */

typedef enum {
    ENL_SINGLE = 0,             /*!< @brief 単発 */
    ENL_SOF,                    /*!< @brief SoF */
    ENL_CONTINUATION,           /*!< @brief 継続 */
    ENL_EOF,                    /*!< @brief EoF */
} t_FrameContinuousInfoType;    /*!< @brief フレーム継続情報型 */

#define U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE   ((u1)10U)   /*!< @brief 送信データキューサイズ */


/*==============================================================*/
/*  Function Prototypes                                         */
/*==============================================================*/
/*!
 * @brief @b LIN通信管理受信メイン処理
 * @param None
 * @return なし @n
 * @details
 *      メイン周期毎に実施するLIN受信処理 @n
*/
static void f_LIN_Manager_MainFunctionRx( void );


/*!
 * @brief @b LIN通信管理送信メイン処理
 * @param None
 * @return なし @n
 * @details
 *      メイン周期毎に実施するLIN送信処理 @n
*/static void f_LIN_Manager_MainFunctionTx( void );


/*!
 * @brief @b 受信データクリア
 * @param None
 * @return なし @n
 * @details
 *      受信データをクリアする @n
*/
static void f_LIN_Manager_ClearRxData( void );


/*!
 * @brief @b 送信データクリア
 * @param None
 * @return なし @n
 * @details
 *      送信データ(キュー)をクリアする @n
*/
static void f_LIN_Manager_ClearTxData( void );


/*!
 * @brief @b キューがあるかチェック
 * @param None
 * @return TRUE=キューがある, FALSE=キューがない @n
 * @details
 *      キューがあるかないかをチェックする @n
*/
static u1 f_LIN_Manager_HasTxDataQueue( void );


/*!
 * @brief @b 送信データエンキュー
 * @param u1 送信データ長
 * @param u1* 送信データ
 * @return なし @n
 * @details
 *      送信データをキューに格納する @n
*/
static void f_LIN_Manager_EnqueueTxData( u1 u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] );


/*!
 * @brief @b 送信データデキュー
 * @param u1* [out] 送信データ長
 * @param u1* [out] 送信データ
 * @return なし @n
 * @details
 *      送信データをキューから取り出す @n
*/
static void f_LIN_Manager_DequeueTxData( u1* u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] );


/*!
 * @brief @b データ長計算
 * @param u1* データ
 * @return データ長 @n
 * @details
 *      データからデータ長を算出する @n
*/
static u2 f_LIN_Manager_CalcDataLength( u1* u1a_Data );


/*!
 * @brief @b D1データ生成
 * @param t_FrameContinuousInfoType フレーム継続情報
 * @return D1データ @n
 * @details
 *      フレームの先頭(Data1)に格納するデータを生成する @n
*/
static u1 f_LIN_Manager_GenD1( t_FrameContinuousInfoType ena_FrameContinuousInfo );


/*==============================================================*/
/*  Data Prototypes                                             */
/*==============================================================*/
static u1 u1l_LIN_Manager_RxData[U2G_LIN_MANAGER_MAX_PAYLOAD_SIZE];     /*!< @brief 受信データ */
static u2 u2l_LIN_Manager_RxDataLength;                                 /*!< @brief 受信データ長 */
static u1 u1l_LIN_Manager_ReceivingContinuousFrame;                     /*!< @brief 継続フレーム受信中: 1=受信中, 0=受信中ではない */
static u2 u2l_LIN_Manager_TempRxDataLength;                             /*!< @brief 仮受信データ長 */
static u1 u1l_LIN_Manager_TempRxData[U1L_LIN_MANAGER_FRAME_SIZE];       /*!< @brief 仮受信データ */
static u1 u1l_LIN_Manager_TempRxDataStoreIndex;                         /*!< @brief 仮受信データ格納インデックス */

static u1 u1l_LIN_Manager_TxDataQueue[U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE][U1L_LIN_MANAGER_FRAME_SIZE];   /*!< @brief 送信データキュー */
static u1 u1l_LIN_Manager_TxDataLengthQueue[U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE];                         /*!< @brief 送信データ長キュー */
static u1 u1l_LIN_Manager_TxDataQueueHead;                                                              /*!< @brief 送信データキュー先頭インデックス */
static u1 u1l_LIN_Manager_TxDataQueueTail;                                                              /*!< @brief 送信データキュー末尾インデックス */
static u1 u1l_LIN_Manager_TxData[U1L_LIN_MANAGER_FRAME_SIZE];                                           /*!< @brief 送信データ */
static u1 u1l_LIN_Manager_TxDataLength;                                                                 /*!< @brief 送信データ長 */

static u1 u1l_LIN_Manager_TxFrameComplete_ID30h;                        /*!< @brief ID30送信完了フラグ: TRUE=送信完了, FALSE=送信未完了 */

static u1 u1l_LIN_Manager_slpreq;

/*==============================================================*/
/*  Functions                                                   */
/*==============================================================*/
void f_LIN_Manager_Initialize( void )
{
    u1 u1a_DataIndex;   /* データインデックス */

    (void)l_sys_init();                         /* システム初期化API */
	
    l_ifc_init_ch1();                            /* バッファ、ドライバの初期化API */
    l_nm_init_ch1();                             /* NM用 初期化API */
	
	b1g_LIN_TxFrameComplete_ID30h = U1G_DAT_BIT_ON; /* 初回から送信できるよう完了に設定しておく */
    
    (void)l_ifc_connect_ch1();                  /* LIN通信接続API */

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
    u2 u2a_DataIndex;   /* データインデックス */

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
    u1 u1a_FullSizeFrameNum;            /* フルサイズフレーム数 */
    u1 u1a_Surplus;                     /* 余りバイト数 */
    u1 u1a_FrameDivisionNum;            /* フレーム分割数 */
    u1 u1a_FrameIndex;                  /* フレームインデックス */
    u1 u1a_WriteFrameData[8];           /* 書き込みフレームデータ */
    u1 u1a_WriteFramePos;               /* 書き込みデータ位置 */
    u1 u1a_WriteDataIndex;              /* 書き込みデータインデックス */

    u1a_FullSizeFrameNum = (u1)(u2a_DataLength / U1L_LIN_MANAGER_DATABYTES_PER_FRAME);
    u1a_Surplus          = (u1)(u2a_DataLength - ((u2)u1a_FullSizeFrameNum * (u2)U1L_LIN_MANAGER_DATABYTES_PER_FRAME));
    u1a_FrameDivisionNum = u1a_FullSizeFrameNum;
    if ( u1a_Surplus > (u1)0U )
    {
        u1a_FrameDivisionNum++;
    }

    /* 単発フレームの場合 */
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
    /* 継続フレームの場合 */
    else
    {
        u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_SOF );
        for ( u1a_WriteDataIndex = U1G_DAT_ZERO; u1a_WriteDataIndex < U1L_LIN_MANAGER_DATABYTES_PER_FRAME; u1a_WriteDataIndex++ )
        {
            u1a_WriteFrameData[u1a_WriteDataIndex + (u1)1U] = u1a_Data[u1a_WriteDataIndex];
        }
        f_LIN_Manager_EnqueueTxData( U1L_LIN_MANAGER_FRAME_SIZE, &u1a_WriteFrameData[0] );

        u1a_WriteFrameData[0] = f_LIN_Manager_GenD1( ENL_CONTINUATION );
        /* 余りバイトがない場合 */
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
        /* 余りバイトがある場合 */
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
    u1 u1a_isReceived = U1G_DAT_FALSE;              /* 受信あり */
    u1 u1a_TempRxFrame[U1L_LIN_MANAGER_FRAME_SIZE]; /* 仮受信フレーム */
    u1 u1a_TempRxDataIndex;                         /* 仮受信データインデックス */
    u2 u2a_RxDataIndex;                             /* 受信データインデックス */
    u1 u1a_FrameCountinuousInfo;                    /* フレーム継続情報 */

    l_u1g_lin_irq_dis();                                                        /* 割り込み禁止 */
    if ( b1g_LIN_RxFrameComplete_ID20h == U1G_DAT_BIT_ON )
    {
        u1a_isReceived = U1G_DAT_TRUE;
        b1g_LIN_RxFrameComplete_ID20h = U1G_DAT_BIT_OFF;
        l_slot_rd_ch1( U1G_LIN_FRAME_ID20H, U1L_LIN_MANAGER_FRAME_SIZE, &u1a_TempRxFrame[0] );
    }
    l_vog_lin_irq_res();                                                        /* 割り込み許可 */

    /* 受信ありの場合 */
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

                /* 受信データを確定する */
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
    u1 u1a_HasTxDataQueue;                      /* 送信データキューがあるかチェック結果 */

    /* 送信データキューがある且つ、送信できる状態である場合 */
    u1a_HasTxDataQueue = f_LIN_Manager_HasTxDataQueue();
    if ( ( u1a_HasTxDataQueue == U1G_DAT_TRUE )
      && ( u1l_LIN_Manager_TxFrameComplete_ID30h == U1G_DAT_TRUE ) )
    {
        f_LIN_Manager_DequeueTxData( &u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
        l_u1g_lin_irq_dis();                                                        /* 割り込み禁止 */
        u1l_LIN_Manager_TxFrameComplete_ID30h = U1G_DAT_FALSE;
        l_slot_wr_ch1( U1G_LIN_FRAME_ID30H, u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
        l_vog_lin_irq_res();                                                        /* 割り込み許可 */
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
    u1 u1a_QueueIndex;  /* キューインデックス */
    u1 u1a_DataIndex;   /* データインデックス */

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
    u1 u1a_HasQueue = U1G_DAT_FALSE;    /* キューがあるか: TRUE=キューがある, FALSE=キューがない */

    /* キューがある場合 */
    if ( u1l_LIN_Manager_TxDataQueueHead != u1l_LIN_Manager_TxDataQueueTail )
    {
        u1a_HasQueue = U1G_DAT_TRUE;
    }

    return u1a_HasQueue;
}


static void f_LIN_Manager_EnqueueTxData( u1 u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] )
{
    u1 u1a_DataIndex;   /* データインデックス */

    /* 送信データキュー末尾インデックスを進める */
    if ( (u1l_LIN_Manager_TxDataQueueTail + (u1)1U) < U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE )
    {
        u1l_LIN_Manager_TxDataQueueTail++;
    }
    else
    {
        u1l_LIN_Manager_TxDataQueueTail = U1G_DAT_ZERO;
    }

    /* 送信データキュー末尾インデックスが指す場所にデータを格納する */
    u1l_LIN_Manager_TxDataLengthQueue[u1l_LIN_Manager_TxDataQueueTail] = u1a_DataLength;
    for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < u1a_DataLength; u1a_DataIndex++ )
    {
        u1l_LIN_Manager_TxDataQueue[u1l_LIN_Manager_TxDataQueueTail][u1a_DataIndex] = u1a_Data[u1a_DataIndex];
    }
}



static void f_LIN_Manager_DequeueTxData( u1* u1a_DataLength, u1 u1a_Data[U1L_LIN_MANAGER_FRAME_SIZE] )
{
    u1 u1a_DataIndex;   /* データインデックス */

    /* 送信データキュー先頭インデックスを進める */
    if ( (u1l_LIN_Manager_TxDataQueueHead + (u1)1U) < U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE )
    {
        u1l_LIN_Manager_TxDataQueueHead++;
    }
    else
    {
        u1l_LIN_Manager_TxDataQueueHead = U1G_DAT_ZERO;
    }

    /* 送信データキュー先頭インデックスが指す場所からデータを取り出す */
    *u1a_DataLength = u1l_LIN_Manager_TxDataLengthQueue[u1l_LIN_Manager_TxDataQueueHead];
    for ( u1a_DataIndex = U1G_DAT_ZERO; u1a_DataIndex < *u1a_DataLength; u1a_DataIndex++ )
    {
        u1a_Data[u1a_DataIndex] = u1l_LIN_Manager_TxDataQueue[u1l_LIN_Manager_TxDataQueueHead][u1a_DataIndex];
    }
}


static u2 f_LIN_Manager_CalcDataLength( u1* u1a_Data )
{
    u2 u2a_DataLength;  /* データ長 */

    /* NFC_DATA_MESSAGE_要求/応答の場合 */
    if ( u1a_Data[1] == (u1)0x00 )
    {
        u2a_DataLength  = (u2)u1a_Data[3] + U2L_LIN_MANAGER_NON_PAYLOAD_DATA_LENGTH;
        u2a_DataLength += ((u2)u1a_Data[2] << 8);
    }
    /* NFC_DATA_MESSAGE_要求/応答以外の場合 */
    else
    {
        u2a_DataLength = (u2)u1a_Data[3] + U2L_LIN_MANAGER_NON_PAYLOAD_DATA_LENGTH;
    }

    return u2a_DataLength;
}


static u1 f_LIN_Manager_GenD1( t_FrameContinuousInfoType ena_FrameContinuousInfo )
{
    u1 u1a_Data1;   /* D1データ */

    u1a_Data1  = (u1)0x20U;                 /* Sleep.ind=0, Wake up.ind=0, Data.ind=1, 機能別識別情報=0(NFC機能) */
    u1a_Data1 |= ena_FrameContinuousInfo;

    return u1a_Data1;
}


void f_LIN_Manager_Callback_TxComplete( u1 u1a_frame, u1 u1a_DataLength, u1* u1a_Data )
{
    u1 u1a_HasTxDataQueue;                      /* 送信データキューがあるかチェック結果 */

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
            /* 送信データキューがある場合 */
            u1a_HasTxDataQueue = f_LIN_Manager_HasTxDataQueue();
            if ( u1a_HasTxDataQueue == U1G_DAT_TRUE )
            {
                f_LIN_Manager_DequeueTxData( &u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
                l_slot_wr_ch1( U1G_LIN_FRAME_ID30H, u1l_LIN_Manager_TxDataLength, &u1l_LIN_Manager_TxData[0] );
            }
            /* 送信データキューがない場合 */
            else
            {
                u1l_LIN_Manager_TxFrameComplete_ID30h = U1G_DAT_TRUE;
            }
        }
        else
        {
            /* 異常 */
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