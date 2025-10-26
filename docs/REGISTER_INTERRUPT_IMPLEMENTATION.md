# レジスタ割り込み実装ドキュメント

## 実装状況

### 完了した作業
1. ✅ ヘッダファイルへの関数プロトタイプ追加 (`l_slin_drv_cc2340r53.h`)
   - `l_u4g_lin_uart_get_base_addr()` - UARTベースアドレス取得関数
   - `l_vog_lin_uart_isr()` - UART割り込みハンドラ

2. ✅ インクルードファイルの修正
   - DMA関連ヘッダ（UART2.h, UART2LPF3.h）を削除
   - レジスタアクセス用ヘッダを追加（hw_memmap.h, hw_ints.h, driverlib/uart.h）

### 次に実装すべき内容

#### 1. 状態管理変数の追加（`l_slin_drv_cc2340r53.c`）

以下の変数を追加：
```c
static HwiP_Struct      xnl_lin_uart_hwi;               /**< @brief UART割り込みオブジェクト */
static l_u32            u4l_lin_uart_base_addr;         /**< @brief UARTベースアドレス */

/* レジスタ割り込み用の状態管理変数 */
static volatile l_u8    u1l_lin_rx_index = 0;          /**< @brief 受信バッファインデックス */
static volatile l_u8    u1l_lin_rx_expected_size = 0;  /**< @brief 期待する受信サイズ */
static volatile l_u8    u1l_lin_tx_index = 0;          /**< @brief 送信バッファインデックス */
static volatile l_u8    u1l_lin_tx_size = 0;           /**< @brief 送信データサイズ */
static volatile l_bool  b1l_lin_uart_initialized = FALSE; /**< @brief UART初期化済みフラグ */
```

削除する変数：
```c
static UART2_Handle         xnl_lin_uart_handle;        /**< @brief UART2ãƒãƒ³ãƒ‰ãƒ« */
```

#### 2. UARTベースアドレス取得関数の実装

```c
/**
 * @brief   UARTベースアドレス取得
 *
 * @return  UARTベースアドレス
 */
l_u32 l_u4g_lin_uart_get_base_addr(void)
{
    return u4l_lin_uart_base_addr;
}
```

#### 3. UART初期化関数の書き換え（`l_vog_lin_uart_init()`）

現在の実装（DMA使用）：
```c
void l_vog_lin_uart_init(void)
{
    UART2_Params xna_lin_uart_params;
    
    if( NULL == xnl_lin_uart_handle )
    {
        l_u1g_lin_irq_dis();
        UART2_Params_init( &xna_lin_uart_params );
        xna_lin_uart_params.readMode        = UART2_Mode_CALLBACK;
        xna_lin_uart_params.writeMode       = UART2_Mode_CALLBACK;
        xna_lin_uart_params.readCallback    = l_ifc_rx_ch1;
        xna_lin_uart_params.writeCallback   = l_ifc_tx_ch1;
        // ... その他のパラメータ設定
        xnl_lin_uart_handle = UART2_open( CONFIG_UART_INDEX,&xna_lin_uart_params );
        if( NULL == xnl_lin_uart_handle )
        {
            u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;
        }
        l_vog_lin_irq_res();
    }
}
```

新しい実装（レジスタ直接）：
```c
void l_vog_lin_uart_init(void)
{
    HwiP_Params hwiParams;
    ClockP_FreqHz freq;
    
    if( FALSE == b1l_lin_uart_initialized )
    {
        l_u1g_lin_irq_dis();
        
        // UARTベースアドレスを取得（CONFIG_UART_INDEXから）
        // 通常UART0 = 0x40008000
        u4l_lin_uart_base_addr = 0x40008000U;  // または適切なマクロから取得
        
        // UARTクロック有効化（Powerドライバ使用）
        Power_setDependency(PowerLPF3_PERIPH_UART0);  // CONFIG_UART_INDEXに応じて変更
        
        // UART無効化
        UARTDisable(u4l_lin_uart_base_addr);
        
        // ボーレート設定
        ClockP_getCpuFreq(&freq);
        #if DeviceFamily_PARENT == DeviceFamily_PARENT_CC27XX
        freq.lo /= 2;  // CC27XXではUARTクロックはSVTCLKの半分
        #endif
        
        UARTConfigSetExpClk(u4l_lin_uart_base_addr,
                            freq.lo,
                            U4L_LIN_BAUDRATE,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
        
        // FIFO有効化
        UARTEnableFifo(u4l_lin_uart_base_addr);
        
        // FIFOレベル設定
        UARTSetFifoLevel(u4l_lin_uart_base_addr, UART_FIFO_TX2_8, UART_FIFO_RX6_8);
        
        // すべての割り込みをクリア
        UARTClearInt(u4l_lin_uart_base_addr,
                     UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | 
                     UART_INT_RT | UART_INT_TX | UART_INT_RX | UART_INT_CTS);
        
        // 割り込みハンドラを登録
        HwiP_Params_init(&hwiParams);
        hwiParams.arg      = (uintptr_t)0;  // 必要に応じて変更
        hwiParams.priority = 0x20;  // 適切な優先度を設定
        HwiP_construct(&xnl_lin_uart_hwi, INT_UART0_COMB, l_vog_lin_uart_isr, &hwiParams);
        
        // UART有効化
        HWREG(u4l_lin_uart_base_addr + UART_O_CTL) |= UART_CTL_UARTEN;
        
        b1l_lin_uart_initialized = TRUE;
        
        l_vog_lin_irq_res();
    }
}
```

#### 4. UART割り込みハンドラの実装

```c
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
```

#### 5. 受信開始関数の書き換え（`l_vog_lin_rx_enb()`）

```c
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
```

#### 6. 送信関数の書き換え（`l_vog_lin_tx_char()`）

```c
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
```

#### 7. リードバック関数の書き換え（`l_u1g_lin_read_back()`）

```c
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
```

#### 8. 削除すべき関数

以下の関数は不要になるため削除またはコメントアウト：
- `l_ifc_rx_ch1()` - UART2コールバック（受信）
- `l_ifc_tx_ch1()` - UART2コールバック（送信）
- `l_ifc_uart_close()` - UART2クローズ

代わりに以下を実装：
```c
void l_ifc_uart_close(void)
{
    if (b1l_lin_uart_initialized)
    {
        /* UART無効化 */
        UARTDisable(u4l_lin_uart_base_addr);
        
        /* 割り込み削除 */
        HwiP_destruct(&xnl_lin_uart_hwi);
        
        /* 電源依存解除 */
        Power_releaseDependency(PowerLPF3_PERIPH_UART0);
        
        b1l_lin_uart_initialized = FALSE;
    }
}
```

## 実装時の注意事項

### 1. UARTベースアドレスの決定

`CONFIG_UART_INDEX`に応じてベースアドレスを決定する必要があります。
TI Driversの設定ファイル（`ti_drivers_config.c`）から取得するか、以下のように決定：

```c
// CC2340R53のUARTベースアドレス
#define UART0_BASE  0x40008000U
#define UART1_BASE  0x40009000U  // もし存在すれば

// CONFIG_UART_INDEXから決定
if (CONFIG_UART_INDEX == 0) {
    u4l_lin_uart_base_addr = UART0_BASE;
}
```

### 2. 割り込み番号の決定

HwiP_construct()で使用する割り込み番号も同様に決定：
- UART0: `INT_UART0_COMB`
- UART1: `INT_UART1_COMB`（もし存在すれば）

### 3. FIFOサイズの制約

CC2340R53のUART FIFOは8バイトです。LINフレームは最大9バイトなので、
複数回に分けて送受信する必要があります（割り込みハンドラで対応済み）。

### 4. エラー処理の強化

本番環境では、`u4a_lin_error_status = U4G_DAT_ZERO;` の行を削除し、
実際のエラー検出を有効化すべきです。

### 5. タイミング調整

LIN通信のタイミング要件を満たすため、割り込み優先度を適切に設定してください。

## テスト計画

1. **基本送受信テスト**
   - 1バイト送受信
   - 9バイト送受信（LIN最大フレームサイズ）

2. **リードバックテスト**
   - 送信データと受信データの一致確認

3. **エラーハンドリングテスト**
   - オーバーランエラーの検出
   - フレーミングエラーの検出

4. **連続送受信テスト**
   - 複数フレームの連続送受信

5. **実LIN通信テスト**
   - LINヘッダー送受信
   - LINレスポンス送受信
   - LINフレーム全体の通信

## まとめ

この実装により、DMAを使用せずにUART割り込みのみでLIN通信を実現できます。
実装は比較的シンプルで、デバッグも容易です。LIN通信は低速（最大19200bps）
なので、CPU負荷の増加は許容範囲内です。
