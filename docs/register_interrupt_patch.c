/**
 * @file        register_interrupt_patch.c
 *
 * @brief       レジスタ割り込み実装パッチ
 *
 * このファイルには、l_slin_drv_cc2340r53.cに追加・置き換えるべき
 * 関数実装が含まれています。
 *
 */

/*==========================================================================*/
/* 1. 変数定義の置き換え */
/*==========================================================================*/

/* 以下の行を削除:
static UART2_Handle         xnl_lin_uart_handle;

代わりに以下を追加: */

static HwiP_Struct      xnl_lin_uart_hwi;               /**< @brief UART割り込みオブジェクト */
static l_u32            u4l_lin_uart_base_addr;         /**< @brief UARTベースアドレス */

/* レジスタ割り込み用の状態管理変数 */
static volatile l_u8    u1l_lin_rx_index = 0;          /**< @brief 受信バッファインデックス */
static volatile l_u8    u1l_lin_rx_expected_size = 0;  /**< @brief 期待する受信サイズ */
static volatile l_u8    u1l_lin_tx_index = 0;          /**< @brief 送信バッファインデックス */
static volatile l_u8    u1l_lin_tx_size = 0;           /**< @brief 送信データサイズ */
static volatile l_bool  b1l_lin_uart_initialized = FALSE; /**< @brief UART初期化済みフラグ */

/*==========================================================================*/
/* 2. UARTベースアドレス取得関数（新規追加） */
/*==========================================================================*/

/**
 * @brief   UARTベースアドレス取得
 *
 * @return  UARTベースアドレス
 */
l_u32 l_u4g_lin_uart_get_base_addr(void)
{
    return u4l_lin_uart_base_addr;
}

/*==========================================================================*/
/* 3. UART初期化関数（l_vog_lin_uart_init の置き換え） */
/*==========================================================================*/

/**
 * @brief   UARTレジスタの初期化 処理
 *
 *          UARTレジスタ直接アクセスによる初期化
 *
 */
void l_vog_lin_uart_init(void)
{
    HwiP_Params hwiParams;
    ClockP_FreqHz freq;
    
    if( FALSE == b1l_lin_uart_initialized )
    {
        l_u1g_lin_irq_dis();
        
        /* UARTベースアドレスを設定 */
        /* CONFIG_UART_INDEXに応じて適切なアドレスを設定 */
        u4l_lin_uart_base_addr = 0x40008000U;  /* UART0のベースアドレス */
        
        /* UARTクロック有効化 */
        Power_setDependency(PowerLPF3_PERIPH_UART0);
        
        /* UART無効化 */
        UARTDisable(u4l_lin_uart_base_addr);
        
        /* CPUクロック周波数取得 */
        ClockP_getCpuFreq(&freq);
        #if DeviceFamily_PARENT == DeviceFamily_PARENT_CC27XX
        /* CC27XXではUARTクロックはSVTCLKの半分 */
        freq.lo /= 2;
        #endif
        
        /* ボーレート、データフォーマット設定 */
        UARTConfigSetExpClk(u4l_lin_uart_base_addr,
                            freq.lo,
                            U4L_LIN_BAUDRATE,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
        
        /* FIFO有効化 */
        UARTEnableFifo(u4l_lin_uart_base_addr);
        
        /* FIFOレベル設定 */
        UARTSetFifoLevel(u4l_lin_uart_base_addr, UART_FIFO_TX2_8, UART_FIFO_RX6_8);
        
        /* すべての割り込みをクリア */
        UARTClearInt(u4l_lin_uart_base_addr,
                     UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | 
                     UART_INT_RT | UART_INT_TX | UART_INT_RX | UART_INT_CTS);
        
        /* 割り込みハンドラを登録 */
        HwiP_Params_init(&hwiParams);
        hwiParams.arg      = (uintptr_t)0;
        hwiParams.priority = 0x20;  /* 適切な優先度を設定 */
        HwiP_construct(&xnl_lin_uart_hwi, INT_UART0_COMB, l_vog_lin_uart_isr, &hwiParams);
        
        /* UART有効化 */
        HWREG(u4l_lin_uart_base_addr + UART_O_CTL) |= UART_CTL_UARTEN;
        
        b1l_lin_uart_initialized = TRUE;
        
        l_vog_lin_irq_res();
    }
}

/*==========================================================================*/
/* 4. UART割り込みハンドラ（新規追加） */
/*==========================================================================*/

/**
 * @brief   UART割り込みハンドラ
 *
 * @param   arg ユーザー引数（未使用）
 */
void l_vog_lin_uart_isr(uintptr_t arg)
{
    l_u32 u4a_status;
    l_u8 u1a_data;
    l_u8 u1a_err_status = U1G_LIN_BYTE_CLR;
    
    /* 割り込みステータス取得 */
    u4a_status = UARTIntStatus(u4l_lin_uart_base_addr, TRUE);
    
    /* 割り込みクリア */
    UARTClearInt(u4l_lin_uart_base_addr, u4a_status);
    
    /* 受信割り込み処理 */
    if (u4a_status & (UART_INT_RX | UART_INT_RT))
    {
        /* FIFOからデータを読み込み */
        while (UARTCharAvailable(u4l_lin_uart_base_addr) && (u1l_lin_rx_index < u1l_lin_rx_expected_size))
        {
            /* データレジスタから読み込み */
            u1a_data = (l_u8)HWREG(u4l_lin_uart_base_addr + UART_O_DR);
            u1l_lin_rx_buf[u1l_lin_rx_index] = u1a_data;
            u1l_lin_rx_index++;
        }
        
        /* 期待サイズ受信完了またはタイムアウト */
        if ((u1l_lin_rx_index >= u1l_lin_rx_expected_size) || (u4a_status & UART_INT_RT))
        {
            /* エラーチェック */
            l_u32 u4a_err = HWREG(u4l_lin_uart_base_addr + UART_O_RSR_ECR);
            if (u4a_err & UART_RSR_ECR_OE)
            {
                u1a_err_status = U1G_LIN_SOER_SET;  /* オーバーランエラー */
            }
            else if (u4a_err & UART_RSR_ECR_FE)
            {
                u1a_err_status = U1G_LIN_SFER_SET;  /* フレーミングエラー */
            }
            else if (u4a_err & UART_RSR_ECR_PE)
            {
                u1a_err_status = U1G_LIN_SPER_SET;  /* パリティエラー */
            }
            
            /* エラーステータスクリア */
            UARTClearRxError(u4l_lin_uart_base_addr);
            
            /* 受信割り込み無効化 */
            UARTDisableInt(u4l_lin_uart_base_addr, UART_INT_RX | UART_INT_RT);
            HWREG(u4l_lin_uart_base_addr + UART_O_CTL) &= ~UART_CTL_RXE;
            
            /* 上位層に通知 */
            l_vog_lin_rx_int(u1l_lin_rx_buf, u1a_err_status);
            
            /* 受信カウンタリセット */
            u1l_lin_rx_index = 0;
            u1l_lin_rx_expected_size = 0;
        }
    }
    
    /* 送信割り込み処理 */
    if (u4a_status & UART_INT_TX)
    {
        /* FIFOに空きがある限りデータを送信 */
        while (UARTSpaceAvailable(u4l_lin_uart_base_addr) && (u1l_lin_tx_index < u1l_lin_tx_size))
        {
            HWREG(u4l_lin_uart_base_addr + UART_O_DR) = u1l_lin_tx_buf[u1l_lin_tx_index];
            u1l_lin_tx_index++;
        }
        
        /* 送信完了 */
        if (u1l_lin_tx_index >= u1l_lin_tx_size)
        {
            /* 送信割り込み無効化 */
            UARTDisableInt(u4l_lin_uart_base_addr, UART_INT_TX);
            
            /* 送信カウンタリセット */
            u1l_lin_tx_index = 0;
            u1l_lin_tx_size = 0;
        }
    }
    
    /* エラー割り込み処理 */
    if (u4a_status & (UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE))
    {
        /* エラーステータスクリア */
        UARTClearRxError(u4l_lin_uart_base_addr);
    }
}

/*==========================================================================*/
/* 5. 受信開始関数（l_vog_lin_rx_enb の置き換え） */
/*==========================================================================*/

/**
 * @brief   UART受信割り込み許可 処理
 *
 *          UART受信割り込み許可
 *
 * @param   受信データサイズ
 */
void  l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx ,uint8_t u1a_lin_rx_data_size)
{
    l_u1g_lin_irq_dis();
    
    /* FIFOフラッシュ */
    if(u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE)
    {
        while (UARTCharAvailable(u4l_lin_uart_base_addr))
        {
            volatile l_u8 dummy = (l_u8)HWREG(u4l_lin_uart_base_addr + UART_O_DR);
            (void)dummy;
        }
    }
    
    /* 受信カウンタ初期化 */
    u1l_lin_rx_index = 0;
    u1l_lin_rx_expected_size = u1a_lin_rx_data_size;
    
    /* エラーステータスクリア */
    UARTClearRxError(u4l_lin_uart_base_addr);
    
    /* 受信割り込み有効化 */
    UARTEnableInt(u4l_lin_uart_base_addr, UART_INT_RX | UART_INT_RT);
    HWREG(u4l_lin_uart_base_addr + UART_O_CTL) |= UART_CTL_RXE;
    
    l_vog_lin_irq_res();
}

/*==========================================================================*/
/* 6. 受信無効化関数（l_vog_lin_rx_dis の置き換え） */
/*==========================================================================*/

/**
 * @brief   UART受信割り込み禁止 処理
 *
 *          UART受信割り込み禁止
 */
void  l_vog_lin_rx_dis(void)
{
    /* レジスタ割り込み方式では明示的な無効化を実装 */
    l_u1g_lin_irq_dis();
    
    UARTDisableInt(u4l_lin_uart_base_addr, UART_INT_RX | UART_INT_RT);
    HWREG(u4l_lin_uart_base_addr + UART_O_CTL) &= ~UART_CTL_RXE;
    
    l_vog_lin_irq_res();
}

/*==========================================================================*/
/* 7. 送信関数（l_vog_lin_tx_char の置き換え） */
/*==========================================================================*/

/**
 * @brief   送信レジスタにデータをセットする 処理
 *
 *          送信レジスタにデータをセットする
 *
 * @param   送信データ
 * @return  なし
 */
void  l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_u8 u1a_index;
    
    l_u1g_lin_irq_dis();
    
    /* 送信バッファにコピー */
    for (u1a_index = 0; u1a_index < u1a_lin_data_size; u1a_index++)
    {
        u1l_lin_tx_buf[u1a_index] = u1a_lin_data[u1a_index];
    }
    
    u1l_lin_tx_size = (l_u8)u1a_lin_data_size;
    u1l_lin_tx_index = 0;
    
    /* 同時受信のための受信開始 */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE, (uint8_t)u1a_lin_data_size);
    
    /* TX有効化 */
    HWREG(u4l_lin_uart_base_addr + UART_O_CTL) |= UART_CTL_TXE;
    
    /* 最初のデータをFIFOに書き込んで送信トリガー */
    while (UARTSpaceAvailable(u4l_lin_uart_base_addr) && (u1l_lin_tx_index < u1l_lin_tx_size))
    {
        HWREG(u4l_lin_uart_base_addr + UART_O_DR) = u1l_lin_tx_buf[u1l_lin_tx_index];
        u1l_lin_tx_index++;
    }
    
    /* TX割り込み有効化（残りのデータ送信用） */
    if (u1l_lin_tx_index < u1l_lin_tx_size)
    {
        UARTEnableInt(u4l_lin_uart_base_addr, UART_INT_TX);
    }
    
    l_vog_lin_irq_res();
}

/*==========================================================================*/
/* 8. リードバック関数（l_u1g_lin_read_back の置き換え） */
/*==========================================================================*/

/**
 * @brief   リードバック 処理
 *
 *          リードバック 処理
 *
 * @param リードバック比較用データ
 * @return (0 / 1) : 読込み成功 / 読込み失敗
 */
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    l_u8    u1a_lin_index;
    
    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status  = HWREG(u4l_lin_uart_base_addr + UART_O_RSR_ECR);
    u4a_lin_error_status  = U4G_DAT_ZERO; /* 一旦エラー検出機能オフ */

    /* エラーが発生しているかをチェック */
    if(    ( ( u4a_lin_error_status & UART_RSR_ECR_OE ) != 0 )
        || ( ( u4a_lin_error_status & UART_RSR_ECR_PE ) != 0 )
        || ( ( u4a_lin_error_status & UART_RSR_ECR_FE ) != 0 ) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        u1a_lin_result = U1G_LIN_OK;
        /* 受信バッファの内容と引数を比較 */
        for (u1a_lin_index = U1G_DAT_ZERO; u1a_lin_index < u1a_lin_data_size; u1a_lin_index++)
        {
            if(u1l_lin_rx_buf[u1a_lin_index] != u1a_lin_data[u1a_lin_index])
            {
                u1a_lin_result = U1G_LIN_NG;
            }
        }
    }

    return( u1a_lin_result );
}

/*==========================================================================*/
/* 9. UARTクローズ関数（l_ifc_uart_close の置き換え） */
/*==========================================================================*/

/**
 * @brief   UARTクローズ処理
 *
 *          UART無効化とリソース解放
 */
void l_ifc_uart_close(void)
{
    if (b1l_lin_uart_initialized)
    {
        l_u1g_lin_irq_dis();
        
        /* すべての割り込みを無効化 */
        UARTDisableInt(u4l_lin_uart_base_addr,
                       UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | 
                       UART_INT_RT | UART_INT_TX | UART_INT_RX | UART_INT_CTS);
        
        /* UART無効化 */
        UARTDisable(u4l_lin_uart_base_addr);
        
        /* 割り込みハンドラ削除 */
        HwiP_destruct(&xnl_lin_uart_hwi);
        
        /* 電源依存解除 */
        Power_releaseDependency(PowerLPF3_PERIPH_UART0);
        
        b1l_lin_uart_initialized = FALSE;
        
        l_vog_lin_irq_res();
    }
}

/*==========================================================================*/
/* 10. 削除すべき関数（コメントアウトまたは削除） */
/*==========================================================================*/

/* 以下の関数は不要になるため削除:
 * - l_ifc_rx_ch1() - UART2受信コールバック
 * - l_ifc_tx_ch1() - UART2送信コールバック
 */

/***** End of File *****/
