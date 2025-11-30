# LIN送信処理とリードバック処理の問題分析

## 概要

CC23xxの移植作業において、**送信処理周りとリードバック周りがうまくできていない**という問題が発生しています。

本ドキュメントでは、H850とCC23xxの実装を詳細に比較し、問題点と修正方法を明確にします。

---

## 問題の本質

### 根本原因

**CC23xxは複数バイト一括送受信方式を採用しているが、H850は1バイト単位の送受信方式を採用している**

この設計思想の違いにより、以下の2つの機能が正しく動作していません：

1. **送信処理** (`l_vog_lin_tx_char`)
2. **リードバック処理** (`l_u1g_lin_read_back`)

---

## 1. 送信処理の問題

### H850の送信処理（1バイト単位）

**[H850/slin_lib/l_slin_drv_h83687.c](H850/slin_lib/l_slin_drv_h83687.c)** - 送信関数は存在しない（レジスタ直接操作）

H850では、送信は**レジスタへの直接書き込み**で実現されています。

**送信処理の流れ:**

```c
// H850のタイマ割り込み処理（l_slin_core_h83687.c:1162-1218）
case ( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    // データ送信2バイト目以降
    if( u1l_lin_rs_cnt > U1G_LIN_0 ){
        // 前回送信したバイトのリードバックチェック
        l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );

        if( l_u1a_lin_read_back != U1G_LIN_OK ){
            // BITエラー
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
            l_vol_lin_set_synchbreak();
        }
        else{
            // 最後まで送信完了
            if( u1l_lin_rs_cnt > u1l_lin_frm_sz ){
                l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );
                l_vol_lin_set_synchbreak();
            }
            // まだ全てのデータを送信完了していない
            else{
                // データの送信（チェックサムまで送信していない）
                if( u1l_lin_rs_cnt < u1l_lin_frm_sz ){
                    u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
                    u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );
                }
                // チェックサムの送信
                else{
                    u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = (l_u8)(~( (l_u8)u2l_lin_chksum ));
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_chksum
                     = (l_u16)u1l_lin_rs_tmp[ u1l_lin_frm_sz ];
                }

                l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  // ← 1バイト送信
                l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );
                u1l_lin_rs_cnt++;
            }
        }
    }
    // データ送信1バイト目
    else{
        u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
        u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );

        l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  // ← 1バイト送信
        l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );
        u1l_lin_rs_cnt++;
    }
    break;
```

**H850の送信関数は存在せず、レジスタに直接書き込む:**

実際には `l_vog_lin_tx_char()` という関数は定義されておらず、マクロまたはインライン関数でレジスタ直接操作を行っています。

```c
// H850ではレジスタ直接操作（推測）
#define l_vog_lin_tx_char(data)  U1G_LIN_REG_TDR.u1l_lin_byte = (data)
```

**H850の動作フロー:**

```
[タイマ割り込み1回目]
    ↓
u1l_lin_rs_cnt = 0
    ↓
1バイト目送信（l_vog_lin_tx_char(data[0])）
    ↓
タイマ再設定（1バイト長 + Inter-Byte Space）
    ↓
u1l_lin_rs_cnt = 1

[タイマ割り込み2回目]
    ↓
u1l_lin_rs_cnt = 1
    ↓
前回送信データ（data[0]）のリードバックチェック
    ↓
OK → 2バイト目送信（l_vog_lin_tx_char(data[1])）
    ↓
タイマ再設定
    ↓
u1l_lin_rs_cnt = 2

[繰り返し...]
```

### CC23xxの送信処理（複数バイト一括）

**[CC23xx/l_slin_drv_cc2340r53.c:431-440](CC23xx/l_slin_drv_cc2340r53.c#L431-L440)**

```c
void  l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE,u1a_lin_data_size);
    /* 送信バッファレジスタに データを格納 */
#if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle,u1a_lin_data,u1a_lin_data_size,NULL);
#else
    UART_RegInt_write(xnl_lin_uart_handle,u1a_lin_data,u1a_lin_data_size);
#endif
}
```

**問題点:**

1. **配列を受け取っているが、H850は単一バイトを送信する設計**
   - H850: `l_vog_lin_tx_char(u1l_lin_rs_tmp[u1l_lin_rs_cnt])` ← 1バイト
   - CC23xx: `l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)` ← 配列

2. **CORE層は1バイトずつ送信する想定だが、DRV層は配列を受け取る**
   - CORE層からは1バイトしか渡されない
   - DRV層は配列として受け取り、サイズ情報も必要とする

3. **受信許可を送信時に実行している**
   - `l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE,u1a_lin_data_size);`
   - リードバック用の受信設定だが、送信サイズと同じサイズを受信設定している
   - これは1バイト送信では正しく動作しない

---

## 2. リードバック処理の問題

### H850のリードバック処理（1バイト単位）

**[H850/slin_lib/l_slin_drv_h83687.c:398-427](H850/slin_lib/l_slin_drv_h83687.c#L398-L427)**

```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    l_u8    u1a_lin_readback_data;
    l_u8    u1a_lin_result;

    /* 受信データの有無をチェック */
    if( U1G_LIN_FLG_SRF == U1G_LIN_BIT_SET ) {
        /* エラーが発生しているかをチェック */
        if(    (U1G_LIN_FLG_SOER == U1G_LIN_BIT_SET)
            || (U1G_LIN_FLG_SFER == U1G_LIN_BIT_SET)
            || (U1G_LIN_FLG_SPER == U1G_LIN_BIT_SET) ) {
            u1a_lin_result = U1G_LIN_NG;
        }
        else {
            u1a_lin_readback_data = U1G_LIN_REG_RDR.u1l_lin_byte; /* 受信データ取得 */
            /* 受信バッファの内容と引数を比較 */
            if( u1a_lin_readback_data != u1a_lin_data ){
                u1a_lin_result = U1G_LIN_NG;
            }
            else{
                u1a_lin_result = U1G_LIN_OK;
            }
        }
    }
    else {
        u1a_lin_result = U1G_LIN_NG;
    }

    return( u1a_lin_result );
}
```

**H850の動作:**

1. 受信フラグ（SRF）をチェック
2. エラーフラグ（SOER/SFER/SPER）をチェック
3. 受信レジスタ（RDR）から1バイト読み出し
4. 引数と比較
5. 一致すれば OK、不一致または エラーあれば NG

**重要なポイント:**

- **送信したバイトが自動的にUARTで受信される（Half-Duplex通信）**
- リードバックは受信レジスタから読み出す
- 1バイト送信 → 1バイト受信 の対応

### CC23xxのリードバック処理（複数バイト一括）

**[CC23xx/l_slin_drv_cc2340r53.c:468-498](CC23xx/l_slin_drv_cc2340r53.c#L468-L498)**

```c
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    l_u8    u1a_lin_index;
    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status  = U4L_LIN_HWREG(((( UART2_HWAttrs const * )( xnl_lin_uart_handle->hwAttrs ))->baseAddr +  UART_O_RSR_ECR ));
    u4a_lin_error_status  = U4G_DAT_ZERO; /*一旦エラー検出機能オフ*/

    /* エラーが発生しているかをチェック */
    if(    ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN ) == U4G_LIN_UART_ERR_OVERRUN )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY  ) == U4G_LIN_UART_ERR_PARITY  )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING ) == U4G_LIN_UART_ERR_FRAMING ) )
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

**問題点:**

1. **配列を受け取っているが、CORE層は1バイトしか渡さない**
   - H850 CORE層: `l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] )` ← 1バイト
   - CC23xx DRV層: `l_u8 u1a_lin_data[]` ← 配列を想定

2. **受信バッファ（`u1l_lin_rx_buf`）との比較が複数バイト前提**
   - `u1l_lin_rx_buf[u1a_lin_index]` を配列としてループで比較
   - 1バイトリードバックの場合、どのインデックスを見るべきか不明

3. **エラー検出機能が無効化されている**
   - `u4a_lin_error_status = U4G_DAT_ZERO; /*一旦エラー検出機能オフ*/`
   - エラー検出が常にOFFになっている

4. **受信完了の確認がない**
   - H850では受信フラグ（SRF）をチェックしている
   - CC23xxでは受信完了の確認がない

---

## 3. 1バイト受信移植時の修正方法

### 3-1. 送信処理の修正

#### 修正前（CC23xx複数バイト版）

**[CC23xx/l_slin_drv_cc2340r53.c:431-440](CC23xx/l_slin_drv_cc2340r53.c#L431-L440)**

```c
void  l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE,u1a_lin_data_size);
    /* 送信バッファレジスタに データを格納 */
#if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle,u1a_lin_data,u1a_lin_data_size,NULL);
#else
    UART_RegInt_write(xnl_lin_uart_handle,u1a_lin_data,u1a_lin_data_size);
#endif
}
```

#### 修正後（1バイト送信版）

**[CC23xx/l_slin_drv_cc2340r53.c:431-445](CC23xx/l_slin_drv_cc2340r53.c#L431-L445)** - 修正版

```c
void  l_vog_lin_tx_char(l_u8 u1a_lin_data)  // ← 引数を単一バイトに変更
{
    // リードバック用に1バイト受信設定
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE);  // ← サイズ指定削除

    /* 送信バッファに1バイトコピー */
    u1l_lin_tx_buf[0] = u1a_lin_data;

    /* 送信バッファレジスタに データを格納 */
#if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1, NULL);  // ← 1バイト送信
#else
    UART_RegInt_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1);  // ← 1バイト送信
#endif
}
```

**変更点:**

1. **関数シグネチャ変更**
   - `const l_u8 u1a_lin_data[], size_t u1a_lin_data_size` → `l_u8 u1a_lin_data`
   - H850と同じ単一バイト引数

2. **受信設定の修正**
   - `l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE)` ← サイズ指定なし
   - 1バイト受信設定に変更（`l_vog_lin_rx_enb`の実装も変更必要）

3. **送信処理の修正**
   - `u1l_lin_tx_buf[0]` に1バイトコピー
   - `UART2_write(..., 1)` で1バイト送信

#### l_vog_lin_rx_enb の修正

**修正前:**

```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
{
    // ...
    UART_RegInt_read(xnl_lin_uart_handle,
                     &u1l_lin_rx_buf,
                     u1a_lin_rx_data_size);  // ← サイズ可変
}
```

**修正後:**

```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx)  // ← サイズパラメータ削除
{
    // ...
    UART_RegInt_read(xnl_lin_uart_handle,
                     u1l_lin_rx_buf,        // ← バッファ先頭
                     1);                     // ← 常に1バイト受信
}
```

---

### 3-2. リードバック処理の修正

#### 修正前（CC23xx複数バイト版）

**[CC23xx/l_slin_drv_cc2340r53.c:468-498](CC23xx/l_slin_drv_cc2340r53.c#L468-L498)**

```c
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data[],l_u8 u1a_lin_data_size)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    l_u8    u1a_lin_index;
    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status  = U4L_LIN_HWREG(((( UART2_HWAttrs const * )( xnl_lin_uart_handle->hwAttrs ))->baseAddr +  UART_O_RSR_ECR ));
    u4a_lin_error_status  = U4G_DAT_ZERO; /*一旦エラー検出機能オフ*/

    /* エラーが発生しているかをチェック */
    if(    ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN ) == U4G_LIN_UART_ERR_OVERRUN )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY  ) == U4G_LIN_UART_ERR_PARITY  )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING ) == U4G_LIN_UART_ERR_FRAMING ) )
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

#### 修正後（1バイトリードバック版）

**[CC23xx/l_slin_drv_cc2340r53.c:468-508](CC23xx/l_slin_drv_cc2340r53.c#L468-L508)** - 修正版

```c
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)  // ← 引数を単一バイトに変更
{
    l_u8    u1a_lin_readback_data;
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    int_fast16_t rxCount;

    /* 受信完了を待つ（タイムアウト付き） */
    // UART2のRxCountを確認（受信済みバイト数）
    rxCount = UART2_getRxCount(xnl_lin_uart_handle);

    if (rxCount <= 0) {
        /* 受信データなし */
        return U1G_LIN_NG;
    }

    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status = U4L_LIN_HWREG(
        ((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR
    );

    /* エラーが発生しているかをチェック */
    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        /* 受信バッファから1バイト取得 */
        u1a_lin_readback_data = u1l_lin_rx_buf[0];

        /* 受信バッファの内容と引数を比較 */
        if (u1a_lin_readback_data != u1a_lin_data)
        {
            u1a_lin_result = U1G_LIN_NG;
        }
        else
        {
            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return u1a_lin_result;
}
```

**変更点:**

1. **関数シグネチャ変更**
   - `l_u8 u1a_lin_data[], l_u8 u1a_lin_data_size` → `l_u8 u1a_lin_data`
   - H850と同じ単一バイト引数

2. **受信完了チェック追加**
   - `UART2_getRxCount()` で受信済みバイト数を確認
   - 受信データがない場合は NG を返す

3. **エラー検出機能の有効化**
   - `u4a_lin_error_status = U4G_DAT_ZERO;` の行を削除
   - エラーステータスを正しくチェック

4. **1バイト比較**
   - ループ削除
   - `u1l_lin_rx_buf[0]` の1バイトのみ比較

---

## 4. CORE層の送信処理

CORE層の送信処理は、H850とCC23xxで同じロジックを使用できます。

### タイマ割り込み処理での送信ステート

**[H850/slin_lib/l_slin_core_h83687.c:1162-1218](H850/slin_lib/l_slin_core_h83687.c#L1162-L1218)**

```c
case ( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    /* データ送信2バイト目以降 */
    if( u1l_lin_rs_cnt > U1G_LIN_0 ){
        // 前回送信バイトのリードバック
        l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );

        if( l_u1a_lin_read_back != U1G_LIN_OK ){
            // BITエラー
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
            l_vol_lin_set_synchbreak();
        }
        else{
            if( u1l_lin_rs_cnt > u1l_lin_frm_sz ){
                // 送信完了
                l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );
                l_vol_lin_set_synchbreak();
            }
            else{
                // チェックサム累積
                if( u1l_lin_rs_cnt < u1l_lin_frm_sz ){
                    u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
                    u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );
                }
                else{
                    // チェックサム計算
                    u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = (l_u8)(~( (l_u8)u2l_lin_chksum ));
                    xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_chksum
                     = (l_u16)u1l_lin_rs_tmp[ u1l_lin_frm_sz ];
                }

                l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  // ← 1バイト送信
                l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );
                u1l_lin_rs_cnt++;
            }
        }
    }
    /* データ送信1バイト目 */
    else{
        u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
        u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) + ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );

        l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  // ← 1バイト送信
        l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );
        u1l_lin_rs_cnt++;
    }
    break;
```

**このコードはCC23xxでもそのまま使用可能です。**

ただし、DRV層の `l_vog_lin_tx_char()` と `l_u1g_lin_read_back()` を1バイト対応に修正する必要があります。

---

## 5. 送信・リードバック処理のシーケンス図

### H850の動作シーケンス

```
[タイマ割り込み1] u1l_lin_rs_cnt = 0
    ↓
┌─────────────────────────────────────┐
│ 1バイト目送信                        │
│ l_vog_lin_tx_char(data[0])          │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ UART送信                             │
│ TDR = data[0]                        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Half-Duplex受信                      │
│ RDR = data[0] (自動受信)             │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ タイマ再設定                         │
│ l_vog_lin_bit_tm_set(BYTE_LENGTH)   │
└─────────────────────────────────────┘
    ↓
u1l_lin_rs_cnt = 1

[タイマ割り込み2] u1l_lin_rs_cnt = 1
    ↓
┌─────────────────────────────────────┐
│ 前回送信バイトのリードバック         │
│ l_u1g_lin_read_back(data[0])        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 受信フラグチェック (SRF)             │
│ if (SRF == 1) {                      │
│   read_data = RDR;                   │
│   if (read_data == data[0])          │
│     return OK;                       │
│ }                                    │
└─────────────────────────────────────┘
    ↓ OK
┌─────────────────────────────────────┐
│ 2バイト目送信                        │
│ l_vog_lin_tx_char(data[1])          │
└─────────────────────────────────────┘
    ↓
[繰り返し...]
```

### CC23xx（修正後）の動作シーケンス

```
[タイマ割り込み1] u1l_lin_rs_cnt = 0
    ↓
┌─────────────────────────────────────┐
│ 1バイト目送信                        │
│ l_vog_lin_tx_char(data[0])          │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 受信設定（リードバック用）           │
│ l_vog_lin_rx_enb(FLUSH_NO_USE)      │
│ UART_RegInt_read(handle, buf, 1)    │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ UART送信                             │
│ u1l_lin_tx_buf[0] = data[0]         │
│ UART_RegInt_write(handle, buf, 1)   │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Half-Duplex受信（自動）              │
│ u1l_lin_rx_buf[0] = data[0]         │
│ 受信完了コールバック実行             │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ タイマ再設定                         │
│ l_vog_lin_bit_tm_set(BYTE_LENGTH)   │
└─────────────────────────────────────┘
    ↓
u1l_lin_rs_cnt = 1

[タイマ割り込み2] u1l_lin_rs_cnt = 1
    ↓
┌─────────────────────────────────────┐
│ 前回送信バイトのリードバック         │
│ l_u1g_lin_read_back(data[0])        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 受信完了チェック                     │
│ rxCount = UART2_getRxCount(handle)  │
│ if (rxCount <= 0) return NG;        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ エラーチェック                       │
│ error = UART_RSR_ECR;                │
│ if (error != 0) return NG;          │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ データ比較                           │
│ read_data = u1l_lin_rx_buf[0];      │
│ if (read_data == data[0])            │
│   return OK;                         │
└─────────────────────────────────────┘
    ↓ OK
┌─────────────────────────────────────┐
│ 2バイト目送信                        │
│ l_vog_lin_tx_char(data[1])          │
└─────────────────────────────────────┘
    ↓
[繰り返し...]
```

---

## 6. 修正コード一覧

### 6-1. DRV層ヘッダファイル修正

**[CC23xx/l_slin_drv_cc2340r53.h](CC23xx/l_slin_drv_cc2340r53.h)** - 関数プロトタイプ変更

```c
// 修正前
void   l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size);
void   l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size);
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data[], l_u8 u1a_lin_data_size);

// 修正後
void   l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx);  // ← サイズ削除
void   l_vog_lin_tx_char(l_u8 u1a_lin_data);         // ← 単一バイト
l_u8   l_u1g_lin_read_back(l_u8 u1a_lin_data);       // ← 単一バイト
```

### 6-2. DRV層実装ファイル修正

**[CC23xx/l_slin_drv_cc2340r53.c](CC23xx/l_slin_drv_cc2340r53.c)** - 関数実装変更

#### `l_vog_lin_rx_enb` の修正

```c
// 修正前
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
{
    // ...
    UART_RegInt_read(xnl_lin_uart_handle, &u1l_lin_rx_buf, u1a_lin_rx_data_size);
}

// 修正後
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx)
{
    l_vog_lin_irq_dis();

    if (u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE) {
        UART_RegInt_readCancel(xnl_lin_uart_handle);
    }

    // 常に1バイト受信設定
    UART_RegInt_read(xnl_lin_uart_handle, u1l_lin_rx_buf, 1);

    l_vog_lin_irq_res();
}
```

#### `l_vog_lin_tx_char` の修正

```c
// 修正前
void l_vog_lin_tx_char(const l_u8 u1a_lin_data[], size_t u1a_lin_data_size)
{
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE, u1a_lin_data_size);
    #if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle, u1a_lin_data, u1a_lin_data_size, NULL);
    #else
    UART_RegInt_write(xnl_lin_uart_handle, u1a_lin_data, u1a_lin_data_size);
    #endif
}

// 修正後
void l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    // リードバック用に1バイト受信設定
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE);

    /* 送信バッファに1バイトコピー */
    u1l_lin_tx_buf[0] = u1a_lin_data;

    /* 1バイト送信 */
    #if ORIGINAL_UART_DRIVER
    UART2_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1, NULL);
    #else
    UART_RegInt_write(xnl_lin_uart_handle, u1l_lin_tx_buf, 1);
    #endif
}
```

#### `l_u1g_lin_read_back` の修正

```c
// 修正前
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data[], l_u8 u1a_lin_data_size)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    l_u8    u1a_lin_index;

    u4a_lin_error_status = U4L_LIN_HWREG(((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR);
    u4a_lin_error_status = U4G_DAT_ZERO; /*一旦エラー検出機能オフ*/

    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        u1a_lin_result = U1G_LIN_OK;
        for (u1a_lin_index = U1G_DAT_ZERO; u1a_lin_index < u1a_lin_data_size; u1a_lin_index++)
        {
            if (u1l_lin_rx_buf[u1a_lin_index] != u1a_lin_data[u1a_lin_index])
            {
                u1a_lin_result = U1G_LIN_NG;
            }
        }
    }

    return u1a_lin_result;
}

// 修正後
l_u8 l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    l_u8    u1a_lin_readback_data;
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;
    int_fast16_t rxCount;

    /* 受信完了を待つ（タイムアウト付き） */
    rxCount = UART2_getRxCount(xnl_lin_uart_handle);

    if (rxCount <= 0) {
        /* 受信データなし */
        return U1G_LIN_NG;
    }

    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status = U4L_LIN_HWREG(
        ((UART2_HWAttrs const *)(xnl_lin_uart_handle->hwAttrs))->baseAddr + UART_O_RSR_ECR
    );

    /* エラーが発生しているかをチェック */
    if (   ((u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN) == U4G_LIN_UART_ERR_OVERRUN)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY)  == U4G_LIN_UART_ERR_PARITY)
        || ((u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING) == U4G_LIN_UART_ERR_FRAMING) )
    {
        u1a_lin_result = U1G_LIN_NG;
    }
    else
    {
        /* 受信バッファから1バイト取得 */
        u1a_lin_readback_data = u1l_lin_rx_buf[0];

        /* 受信バッファの内容と引数を比較 */
        if (u1a_lin_readback_data != u1a_lin_data)
        {
            u1a_lin_result = U1G_LIN_NG;
        }
        else
        {
            u1a_lin_result = U1G_LIN_OK;
        }
    }

    return u1a_lin_result;
}
```

---

## 7. CORE層の変更箇所

CORE層も複数バイト受信から1バイト受信に変更する必要があります。
詳細は [CHANGE_TO_SINGLE_BYTE_RX.md](CC23xx/CHANGE_TO_SINGLE_BYTE_RX.md) を参照してください。

**主な変更点:**

1. **`l_vog_lin_rx_int()` の関数シグネチャ変更**
   - `void l_vog_lin_rx_int(l_u8 u1a_lin_data[], l_u8 u1a_lin_err)` → `void l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err)`

2. **ヘッダ受信処理の分割**
   - 3バイト一括受信 → 1バイトずつ受信
   - `SYNCHFIELD_WAIT` 状態と `IDENTFIELD_WAIT` 状態の追加

3. **データ受信処理の変更**
   - データ+チェックサム一括受信 → 1バイトずつ受信
   - チェックサム計算を一括 → 累積計算

---

## 8. まとめ

### 問題点の再確認

| 項目 | H850 | CC23xx（現状） | 問題 |
|------|------|----------------|------|
| **送信関数** | `l_vog_lin_tx_char(l_u8 data)` | `l_vog_lin_tx_char(const l_u8 data[], size_t size)` | **引数不一致** |
| **リードバック関数** | `l_u1g_lin_read_back(l_u8 data)` | `l_u1g_lin_read_back(l_u8 data[], l_u8 size)` | **引数不一致** |
| **受信設定** | レジスタ直接操作 | `l_vog_lin_rx_enb(flush, size)` | **サイズ指定が不要** |
| **送信処理** | 1バイト送信 | 複数バイト送信 | **1バイトずつ送信できない** |
| **リードバック** | 1バイト比較 | 複数バイト比較 | **1バイト比較ができない** |
| **エラー検出** | 有効 | 無効（`= U4G_DAT_ZERO`） | **エラー検出されない** |

### 修正が必要な関数

| 関数名 | ファイル | 修正内容 |
|--------|---------|---------|
| `l_vog_lin_tx_char` | l_slin_drv_cc2340r53.c | 配列→単一バイト、1バイト送信に変更 |
| `l_u1g_lin_read_back` | l_slin_drv_cc2340r53.c | 配列→単一バイト、1バイト比較に変更、受信完了チェック追加、エラー検出有効化 |
| `l_vog_lin_rx_enb` | l_slin_drv_cc2340r53.c | サイズ指定削除、常に1バイト受信 |
| `l_vog_lin_rx_int` | l_slin_core_CC2340R53.c | 配列→単一バイト、ヘッダ・データ処理分割 |

### 作業優先度

| 優先度 | 項目 | 理由 |
|--------|------|------|
| **最高** | `l_vog_lin_tx_char` 修正 | 送信処理の基本、これがないと送信できない |
| **最高** | `l_u1g_lin_read_back` 修正 | リードバック失敗で送信エラー扱いになる |
| **高** | `l_vog_lin_rx_enb` 修正 | 受信設定の基本、送信・受信両方で使用 |
| **高** | `l_vog_lin_rx_int` 修正（CORE層） | 受信処理全体、ヘッダ・データ受信に影響 |

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: LIN TX/Readback Issue Analysis Tool
