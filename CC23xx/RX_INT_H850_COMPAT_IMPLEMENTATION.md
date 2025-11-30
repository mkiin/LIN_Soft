# l_vog_lin_rx_int関数のH850互換実装

## 変更日
2025-12-01

## 変更の目的
`l_vog_lin_rx_int`関数を`l_vog_lin_check_header`を使用せず、H850アーキテクチャと同じ構造で直接処理する実装に変更しました。

## 変更内容

### 1. 関数シグネチャ
変更なし（既にH850互換の1バイト受信）
```c
void l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err)
```

### 2. 主な変更点

#### 2.1 l_vog_lin_check_header関数の呼び出しを削除
**変更前:**
```c
// RUN STANDBY状態
else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY )
{
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
    l_vog_lin_check_header(u1a_lin_data_array,u1a_lin_err);  // 削除
}

// RUN状態 - BREAK_UART_WAIT
case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
    l_vog_lin_check_header(u1a_lin_data_array,u1a_lin_err);  // 削除
    break;
```

**変更後:**
```c
// RUN STANDBY状態 - 直接処理
else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY )
{
    if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT )
    {
        if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON )
        {
            if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
            {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
                l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
                u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
                l_vog_lin_rx_dis();
            }
            else
            {
                l_vog_lin_rx_enb();
            }
        }
        else if( (u1a_lin_err & U1G_LIN_OVERRUN_ERR) != U1G_LIN_BYTE_CLR )
        {
            l_vog_lin_rx_enb();
        }
    }
}

// RUN状態 - BREAK_UART_WAIT - 直接処理
case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
    if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON )
    {
        if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
        {
            l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
            u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
            l_vog_lin_rx_dis();
        }
        else
        {
            l_vog_lin_rx_enb();
        }
    }
    else if( (u1a_lin_err & U1G_LIN_OVERRUN_ERR) != U1G_LIN_BYTE_CLR )
    {
        l_vog_lin_rx_enb();
    }
    break;
```

#### 2.2 追加されたステート処理

H850互換実装により、以下のステート処理が追加されました：

1. **U1G_LIN_SLSTS_BREAK_IRQ_WAIT** (Synch Break IRQ待ち)
   - 何もしない（外部IRQ割り込みで処理）

2. **U1G_LIN_SLSTS_SYNCHFIELD_WAIT** (Synch Field待ち)
   - UARTエラーチェック
   - 受信データが0x55であることを確認
   - エラー時はSynch Break待ちへ戻る
   - 正常時はIDENTFIELD_WAITへ移行

3. **U1G_LIN_SLSTS_IDENTFIELD_WAIT** (Ident Field待ち)
   - UARTエラーチェック
   - パリティチェック（受信IDと保護IDテーブルを比較）
   - SLEEPコマンドID処理
   - 通常フレームID処理（受信/送信の分岐）
   - NM情報の処理
   - レスポンススペース待ちタイマ設定

4. **U1G_LIN_SLSTS_RCVDATA_WAIT** (データ受信待ち)
   - 1バイトずつの受信処理（`u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] = u1a_lin_data;`）
   - チェックサムの累積計算
   - チェックサム検証
   - SLEEPコマンド判定（データ[0]==0x00）
   - 受信完了時のバッファ転送

### 3. H850との主な違い

CC23xxでは以下の点がH850と異なりますが、ロジックは同じです：

1. **ハードウェアレジスタ定義**
   - CC23xx: `UART_O_RSR_ECR` レジスタでエラー検出
   - H850: H83687固有のSFR

2. **タイマードライバ**
   - CC23xx: `LGPTimerLPF3` API使用
   - H850: H83687固有のタイマAPI

3. **UART制御**
   - CC23xx: `UART2LPF3` ドライバ使用
   - H850: H83687固有のUART制御

### 4. 削除された処理

以下の一時変数と配列変換処理を削除：
```c
// 削除された変数
l_u8 u1a_lin_data_array[1];  // l_vog_lin_check_header用の一時変数

// 削除された処理
u1a_lin_data_array[0] = u1a_lin_data;
```

### 5. 実装の利点

1. **処理フローの明確化**
   - ヘッダー処理が`l_vog_lin_rx_int`内で完結
   - ステートマシンの状態遷移が追いやすい

2. **H850との完全な互換性**
   - アーキテクチャが完全にH850と一致
   - 将来のメンテナンスが容易

3. **デバッグの容易性**
   - 1つの関数内でヘッダー処理全体を確認可能
   - 関数呼び出しのオーバーヘッド削減

## 修正ファイル

- **l_slin_core_CC2340R53.c**
  - l_vog_lin_rx_int関数（676行目～）を完全書き換え
  - 元：170行（676～845行）
  - 新：409行（H850互換の完全実装）
  - 差分：+239行（機能追加により行数増加）

## 動作確認項目

実装後、以下の項目を確認してください：

1. **Synch Break検出**
   - Framingエラー + 0x00データでBreak検出
   - タイマー起動確認

2. **Synch Field検出**
   - 0x55データの正常受信
   - エラー時のBreak待ち復帰

3. **ID Field処理**
   - パリティエラー検出
   - SLEEPコマンド判定
   - 送信/受信フレーム分岐

4. **データ受信**
   - 1バイトずつの受信
   - チェックサム計算
   - バッファ転送

5. **データ送信**
   - レスポンススペース待ち
   - NM情報の埋め込み
   - タイマー割り込みでの送信処理

## 関連ドキュメント

- [STATE_TRANSITION_ANALYSIS.md](STATE_TRANSITION_ANALYSIS.md) - 状態遷移解析
- [CHANGE_TO_SINGLE_BYTE_RX.md](CHANGE_TO_SINGLE_BYTE_RX.md) - 1バイト受信への変更
- [H850/slin_lib/l_slin_core_h83687.c](../H850/slin_lib/l_slin_core_h83687.c) - H850参照実装
