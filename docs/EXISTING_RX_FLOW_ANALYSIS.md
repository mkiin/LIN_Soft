# 既存実装の受信処理フロー分析

## 調査概要

既存の`l_slin_drv_cc2340r53.c`における受信割り込み有効化とコールバックの動作フローを調査しました。

## ユーザーの認識確認

**ユーザーの認識**: 「受信割り込み有効化後にread関数を呼び、その中でコールバックを呼び、また、そのコールバック内で受信割り込み有効化、read、の繰り返しで読み取っている」

**調査結果**: ✅ **認識は正しいです**

## 詳細な受信処理フロー

### 1. 受信開始のトリガー（`l_vog_lin_rx_enb`）

**ファイル**: `l_slin_drv_cc2340r53.c` 行254-269

```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
{
    l_s16 s2a_lin_ret;
    l_u1g_lin_irq_dis();                                // 割り込み禁止設定
    
    /* バッファデータを削除する処理 */
    if(u1a_lin_flush_rx == U1G_LIN_FLUSH_RX_USE)
    {
        UART2_flushRx(xnl_lin_uart_handle);             // ① FIFOフラッシュ
    }

    UART2_rxEnable(xnl_lin_uart_handle);                // ② UART受信割り込み許可
    s2a_lin_ret = UART2_read(xnl_lin_uart_handle,      // ③ UART受信開始
                             &u1l_lin_rx_buf,
                             u1a_lin_rx_data_size,
                             NULL);
    if(UART2_STATUS_SUCCESS != s2a_lin_ret)
    {
        u1g_lin_syserr = U1G_LIN_SYSERR_DRIVER;         // MCUステータス異常
    }
    l_vog_lin_irq_res();                                // 割り込み設定復元
}
```

**処理内容**:
1. **FIFOフラッシュ**: 必要に応じて受信FIFOをクリア
2. **受信有効化**: `UART2_rxEnable()` でUART受信割り込みを許可
3. **UART2_read()呼び出し**: DMAまたは割り込みによる受信を開始

### 2. UART2_read()の動作

`UART2_read()`は**非ブロッキング**モードで動作します（初期化時に`UART2_Mode_CALLBACK`を指定）:

```c
// l_vog_lin_uart_init()内 (行116-117)
xna_lin_uart_params.readMode        = UART2_Mode_CALLBACK;
xna_lin_uart_params.readCallback    = l_ifc_rx_ch1;  // コールバック登録
```

**動作**:
- `UART2_read()`は即座にリターン（ブロックしない）
- DMAまたはUART割り込みで指定バイト数を受信
- 受信完了時に**自動的に**コールバック`l_ifc_rx_ch1()`を呼び出す

### 3. 受信完了コールバック（`l_ifc_rx_ch1`）

**ファイル**: `l_slin_drv_cc2340r53.c` 行188-194

```c
void l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data, 
                  size_t count, void *userArg, int_fast16_t u1a_lin_rx_status)
{
    l_u8 u1a_lin_rx_set_err;

    /* 受信エラー情報の生成 */
    u1a_lin_rx_set_err = U1G_LIN_BYTE_CLR;  // 現在エラー検出未実装
    
    // ④ スレーブタスクに受信報告
    l_vog_lin_rx_int((l_u8 *)u1a_lin_rx_data, u1a_lin_rx_set_err);
}
```

**処理内容**:
- 受信データとエラーステータスを取得
- 上位層の`l_vog_lin_rx_int()`を呼び出す

### 4. 上位層の受信処理（`l_vog_lin_rx_int`）

**ファイル**: `l_slin_core_cc2340r53.c` 行671-850

この関数内で状態に応じて処理を行い、**次の受信を準備**します:

```c
void l_vog_lin_rx_int(l_u8 u1a_lin_data[], l_u8 u1a_lin_err)
{
    // ... 状態チェックと処理 ...
    
    switch(u1l_lin_slv_sts)
    {
    case U1G_LIN_SLSTS_BREAK_UART_WAIT:
        l_vog_lin_check_header(u1a_lin_data, u1a_lin_err);
        break;
        
    case U1G_LIN_SLSTS_RCVDATA_WAIT:
        // データ受信処理
        // ...
        
        // 処理完了後、次の受信を開始
        l_vol_lin_set_synchbreak();  // ⑤ 次のフレーム受信準備
        break;
        
    // ... その他の状態 ...
    }
}
```

### 5. 次の受信準備（`l_vol_lin_set_synchbreak`）

**ファイル**: `l_slin_core_cc2340r53.c` 行906-911

```c
static void l_vol_lin_set_synchbreak(void)
{
    // ⑥ 再び受信割り込み有効化とread呼び出し
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_USE, U1L_LIN_UART_HEADER_RXSIZE);
    l_vog_lin_int_enb();                                    // INT割り込みを許可
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT;        // Synch Break(UART)待ち状態に移行
}
```

**処理内容**:
- **再び`l_vog_lin_rx_enb()`を呼び出す**（フローの①に戻る）
- これにより、受信→コールバック→受信準備の**ループ**が形成される

## 受信処理の完全なフロー図

```
┌─────────────────────────────────────────────────┐
│ 1. l_vog_lin_rx_enb()                           │
│    - UART2_rxEnable()                           │
│    - UART2_read() ← 非ブロッキング               │
└──────────────┬──────────────────────────────────┘
               │ 即座にリターン
               ↓
┌─────────────────────────────────────────────────┐
│ 【UART/DMA割り込み処理】                         │
│  - 指定バイト数を受信                            │
│  - 受信完了時にコールバック呼び出し               │
└──────────────┬──────────────────────────────────┘
               │ 自動的に呼び出される
               ↓
┌─────────────────────────────────────────────────┐
│ 2. l_ifc_rx_ch1() ← 受信完了コールバック         │
│    - 受信データを取得                            │
│    - エラーステータスを取得                       │
└──────────────┬──────────────────────────────────┘
               │
               ↓
┌─────────────────────────────────────────────────┐
│ 3. l_vog_lin_rx_int()                           │
│    - 状態に応じた処理                            │
│    - データ解析・チェックサム検証                 │
│    - フレームバッファへの格納                     │
└──────────────┬──────────────────────────────────┘
               │
               ↓
┌─────────────────────────────────────────────────┐
│ 4. l_vol_lin_set_synchbreak()                   │
│    - 次のフレーム受信準備                        │
│    - l_vog_lin_rx_enb() 呼び出し ← ループ！      │
└──────────────┬──────────────────────────────────┘
               │
               └─────┐
                     ↓
               ┌─────┘
               ↓
          (1に戻る)
```

## 重要なポイント

### ポイント1: コールバックチェーン

```
l_vog_lin_rx_enb()
  ↓ (UART2_read()を呼ぶ)
  ↓ (DMA/割り込み受信)
l_ifc_rx_ch1() ← コールバック1
  ↓
l_vog_lin_rx_int() ← コールバック2（上位層）
  ↓
l_vol_lin_set_synchbreak()
  ↓
l_vog_lin_rx_enb() ← 再度呼び出し（ループ）
```

### ポイント2: 非ブロッキング動作

- `UART2_read()`は**即座にリターン**
- 実際の受信は**バックグラウンド**で実行（DMA/割り込み）
- 受信完了は**コールバックで通知**

### ポイント3: 状態遷移

```c
// 状態管理変数: u1l_lin_slv_sts

U1G_LIN_SLSTS_BREAK_UART_WAIT  // Synch Break待ち
    ↓ (ヘッダー受信)
U1G_LIN_SLSTS_RCVDATA_WAIT     // データ受信待ち
    ↓ (データ受信完了)
U1G_LIN_SLSTS_BREAK_UART_WAIT  // 次のフレーム待ち (ループ)
```

### ポイント4: 自己再帰的な構造

このアーキテクチャは**自己再帰的**です:
- 受信完了コールバック内で次の受信を開始
- これにより**連続的な受信**が可能
- フレームごとに自動的に次の受信準備

## レジスタ割り込み実装への影響

### 既存実装の特徴
1. ✅ UART2ドライバがコールバックを管理
2. ✅ DMA/割り込みハンドラが自動的にコールバックを呼ぶ
3. ✅ 上位層は単に`l_vog_lin_rx_enb()`を呼ぶだけ

### レジスタ割り込み実装で考慮すべき点

#### 変更が必要な部分
1. **`l_vog_lin_rx_enb()`の実装**
   - `UART2_read()`の代わりにレジスタ直接設定
   - RX割り込み有効化

2. **割り込みハンドラ**
   - 受信完了時に`l_vog_lin_rx_int()`を呼ぶ
   - これにより既存の上位層コードは変更不要

#### 維持すべき動作
1. ✅ **コールバックチェーンを維持**
   - `l_vog_lin_rx_int()`は必ず呼ばれる必要がある
   
2. ✅ **非ブロッキング動作を維持**
   - `l_vog_lin_rx_enb()`は即座にリターン
   - 受信は割り込みで処理

3. ✅ **自己再帰的な構造を維持**
   - `l_vol_lin_set_synchbreak()`からの呼び出しに対応

## 推奨実装パターン

### レジスタ割り込み版の`l_vog_lin_rx_enb()`

```c
void l_vog_lin_rx_enb(uint8_t u1a_lin_flush_rx, uint8_t u1a_lin_rx_data_size)
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
    // ↑ ここで即座にリターン（非ブロッキング）
}
```

### レジスタ割り込みハンドラ内での呼び出し

```c
void l_vog_lin_uart_isr(uintptr_t arg)
{
    // ... 受信処理 ...
    
    if (受信完了)
    {
        /* 既存のコールバックチェーンを維持 */
        l_vog_lin_rx_int(u1l_lin_rx_buf, u1a_err_status);
        
        // ↑ この中で l_vol_lin_set_synchbreak() が呼ばれ
        // ↓ その中で l_vog_lin_rx_enb() が呼ばれる（ループ）
    }
}
```

## まとめ

### ユーザーの認識確認結果

✅ **完全に正しい認識です**

実装フロー:
1. `l_vog_lin_rx_enb()` で受信開始
2. `UART2_read()` を呼ぶ
3. DMA/割り込みで受信
4. 受信完了時にコールバック `l_ifc_rx_ch1()` 呼び出し
5. その中で `l_vog_lin_rx_int()` 呼び出し
6. `l_vog_lin_rx_int()` 内で `l_vol_lin_set_synchbreak()` 呼び出し
7. `l_vol_lin_set_synchbreak()` 内で再び `l_vog_lin_rx_enb()` 呼び出し
8. **ループして1に戻る**

### レジスタ割り込み実装での注意点

1. ✅ **コールバックチェーンを維持**する
   - 割り込みハンドラから`l_vog_lin_rx_int()`を呼ぶ
   
2. ✅ **非ブロッキング動作を維持**する
   - `l_vog_lin_rx_enb()`は即座にリターン
   
3. ✅ **自己再帰を許容**する
   - `l_vol_lin_set_synchbreak()` → `l_vog_lin_rx_enb()` のループ

4. ✅ **上位層コードは変更不要**
   - `l_vog_lin_rx_int()`のインターフェースを維持すれば、
     `l_slin_core_cc2340r53.c`は一切変更不要

この分析により、既存のアーキテクチャを理解した上で、レジスタ割り込み実装を
既存コードとシームレスに統合できることが確認できました。
