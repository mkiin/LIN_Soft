# LIN ヘッダー受信からレスポンス送信までの完全シーケンス

## 概要

本ドキュメントでは、LINスレーブノードがヘッダーを受信してからレスポンスを送信するまでの完全なシーケンスを詳細に解説します。特に、**BITエラー検出のためのリードバック処理**の仕組みと、その実装タイミングに焦点を当てます。

**実装**: CC2340R53 LINスタック
**作成日**: 2025-12-01

---

## 目次

1. [全体フロー概要](#全体フロー概要)
2. [詳細シーケンス](#詳細シーケンス)
3. [BITエラー検出とリードバック](#bitエラー検出とリードバック)
4. [タイミングチャート](#タイミングチャート)
5. [状態遷移図](#状態遷移図)
6. [重要な実装ポイント](#重要な実装ポイント)

---

## 全体フロー概要

### マスターとスレーブの通信フロー

```
[Master]                    [Slave = CC2340R53]
   |                              |
   |--- Break Field ------------->|
   |--- Delimiter --------------->|
   |--- Synch Field (0x55) ------>| ← UART RX割り込み
   |--- PID (Protected ID) ------>| ← UART RX割り込み
   |                              |
   |                              |[PID解析]
   |                              |[送信フレーム判定]
   |                              |[Response Space待ち]
   |                              |
   |<---- Data1 ------------------| ← 送信 + リードバック
   |                              |[BITエラーチェック]
   |                              |[Inter-byte Space待ち]
   |<---- Data2 ------------------| ← 送信 + リードバック
   |                              |[BITエラーチェック]
   |                              |...
   |<---- DataN ------------------| ← 送信 + リードバック
   |                              |[BITエラーチェック]
   |<---- Checksum ---------------| ← 送信 + リードバック
   |                              |[BITエラーチェック]
   |                              |[送信完了]
```

---

## 詳細シーケンス

### フェーズ1: ヘッダー受信（Break ～ PID）

#### 1-1. Break Field 受信

**実装箇所**: [l_slin_core_CC2340R53.c:704-712](l_slin_core_CC2340R53.c#L704-L712)

```c
/* Synch Break(UART)待ち状態 */
if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT )
{
    /* Framingエラー発生 */
    if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON )
    {
        /* 受信データが00hならば、SynchBreak受信 */
        if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
        {
            xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
            l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
            u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;  // ← 直接遷移
        }
    }
}
```

**処理内容**:
1. UART RX割り込み発生（Framing Error付き）
2. 受信データ = 0x00 を確認
3. Header Timeoutタイマー起動（40 bit時間）
4. 状態遷移: `SYNCHFIELD_WAIT`

#### 1-2. Synch Field 受信（0x55）

**実装箇所**: [l_slin_core_CC2340R53.c:780-810](l_slin_core_CC2340R53.c#L780-L810)

```c
/* Synch Field待ち状態 */
case( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
    /* 受信データが55hならば、Synchパターン受信成功 */
    if( u1a_lin_data == U1G_LIN_SYNCHFIELD_DATA )
    {
        u1l_lin_slv_sts = U1G_LIN_SLSTS_ID_WAIT;  /* ID受信待ち状態へ */
    }
    else
    {
        /* Synchパターン受信失敗 → Headerエラー */
        xng_lin_sts_buf.un_state.st_bit.u2g_lin_e_uart = U2G_LIN_BIT_SET;
        xng_lin_bus_sts.st_bit.u2g_lin_head_err = U2G_LIN_BIT_SET;
        l_vol_lin_set_synchbreak();
    }
    break;
```

**処理内容**:
1. UART RX割り込み発生
2. 受信データ = 0x55 を確認
3. 状態遷移: `ID_WAIT`

#### 1-3. PID (Protected ID) 受信

**実装箇所**: [l_slin_core_CC2340R53.c:813-936](l_slin_core_CC2340R53.c#L813-L936)

```c
/* ID受信待ち状態 */
case( U1G_LIN_SLSTS_ID_WAIT ):
    /* パリティチェック */
    if( u1l_lin_chk_id_parity( u1a_lin_data ) == U1G_LIN_OK )
    {
        /* IDとスロット番号の取得 */
        xnl_lin_id_sl = l_xng_lin_get_id_slot( u1a_lin_data );

        /* フレームが登録されているか確認 */
        if( xnl_lin_id_sl.u1g_lin_slot != U1G_LIN_NO_FRAME )
        {
            /* [送信フレーム時] */
            if( xng_lin_slot_tbl[ xnl_lin_id_sl.u1g_lin_slot ].u1g_lin_sndrcv == U1G_LIN_CMD_SND )
            {
                l_vog_lin_frm_tm_stop();                    // Header Timeout停止
                u2l_lin_chksum = U2G_LIN_WORD_CLR;         // チェックサム初期化
                u1l_lin_rs_cnt = U1G_LIN_BYTE_CLR;         // 送信カウンタ初期化

                l_vog_lin_rx_dis();                        // UART受信禁止

                /* 送信データをtmpバッファにコピー */
                u1l_lin_rs_tmp[ U1G_LIN_0 ] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[0];
                u1l_lin_rs_tmp[ U1G_LIN_1 ] = xng_lin_frm_buf[slot].xng_lin_data.u1g_lin_byte[1];
                // ... (以下、Data2～Data8も同様)

                l_vog_lin_bit_tm_set( U1G_LIN_RSSP );      // Response Space待ちタイマー (1 bit)

                u1l_lin_slv_sts = U1G_LIN_SLSTS_SNDDATA_WAIT;  // ← 送信待ち状態へ
            }
        }
    }
    break;
```

**処理内容**:
1. UART RX割り込み発生
2. PIDパリティチェック
3. ID→スロット番号変換
4. 送信フレーム判定
5. **Header Timeoutタイマー停止**
6. 送信データをtmpバッファにコピー
7. **Response Spaceタイマー起動（1 bit = 52.08 μs @ 19200bps）**
8. 状態遷移: `SNDDATA_WAIT`

---

### フェーズ2: Response Space待ち

**実装箇所**: タイマー割り込みハンドラ [l_slin_core_CC2340R53.c:1192-1244](l_slin_core_CC2340R53.c#L1192-L1244)

```
PID受信完了
    ↓
Response Spaceタイマー起動 (1 bit)
    ↓
[52.08 μs 経過]
    ↓
タイマー割り込み発生 (l_vog_lin_tm_int)
    ↓
SNDDATA_WAIT ケース処理
    ↓
[Data1の送信開始]
```

---

### フェーズ3: データバイト送信 + リードバック

#### 3-1. Data1送信（リードバックなし）

**実装箇所**: [l_slin_core_CC2340R53.c:1237-1244](l_slin_core_CC2340R53.c#L1237-L1244)

```c
/* データ送信待ち状態 */
case( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    /* データ送信1バイト目 (u1l_lin_rs_cnt == 0) */
    if( u1l_lin_rs_cnt == U1G_LIN_0 ){
        /* チェックサム演算 */
        u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
        u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) +
                         ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );

        l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  // ← Data1送信
        l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  // 11 bit タイマー
        u1l_lin_rs_cnt++;  // カウンタ = 1
    }
    break;
```

**重要ポイント**:
- **1バイト目（Data1）はリードバックチェックなし**
- 送信完了後、Inter-byte Space + 次バイト送信時間分のタイマー起動（11 bit）

#### l_vog_lin_tx_char() の動作

**実装箇所**: [l_slin_drv_cc2340r53.c:451-465](l_slin_drv_cc2340r53.c#L451-L465)

```c
void  l_vog_lin_tx_char(l_u8 u1a_lin_data)
{
    /* 1バイト受信許可（Half-Duplex用リードバック） */
    l_vog_lin_rx_enb(U1G_LIN_FLUSH_RX_NO_USE, U1G_LIN_1);  // ← ★ 受信を再有効化

    /* 送信バッファに1バイト格納 */
    u1l_lin_tx_buf[U1G_LIN_0] = u1a_lin_data;

    /* 1バイト送信 */
    UART_RegInt_write(xnl_lin_uart_handle, u1l_lin_tx_buf, U1G_LIN_1);
}
```

**処理内容**:
1. **UART受信を再有効化** ← Half-Duplex（送受信共通線）のため、自分の送信データを受信
2. 送信バッファにデータ格納
3. UART送信開始

**受信バッファ更新**:
- 送信したデータがUART経由でループバックされる
- UART RX割り込み発生 → `l_ifc_rx_ch1()` → `l_vog_lin_rx_int()`
- **`u1l_lin_rx_buf[0]`** に送信データが格納される

#### 3-2. Data2送信 + Data1のリードバック

**タイミング**:
```
[Data1送信]       [11 bit経過]         [タイマー割り込み]
    |                  |                        |
    +-- TX完了 --------+-- Inter-byte Space ---+
    +-- RX割り込み ----+                        |
       (リードバック)   |                        |
                        |                        |
                      [u1l_lin_rx_buf[0] = Data1]
                                                 |
                                           [Data2送信開始]
```

**実装箇所**: [l_slin_core_CC2340R53.c:1195-1234](l_slin_core_CC2340R53.c#L1195-L1234)

```c
/* データ送信待ち状態 */
case( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    /* 2バイト目以降 (u1l_lin_rs_cnt > 0) */
    if( u1l_lin_rs_cnt > U1G_LIN_0 ){
        /* ★ 前回送信データのリードバック確認 */
        l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );

        /* リードバック失敗(受信割り込みがかからない、もしくは受信データが異なる) */
        if( l_u1a_lin_read_back != U1G_LIN_OK ){
            /* BITエラー */
            xng_lin_frm_buf[ xnl_lin_id_sl.u1g_lin_slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
            l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );
            l_vol_lin_set_synchbreak();  // エラー時はBreak待ちへ戻る
        }
        /* リードバック成功 */
        else{
            /* 最後まで送信完了したか確認 */
            if( u1l_lin_rs_cnt > u1l_lin_frm_sz ){  // DL + 1 (Checksum送信後)
                l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );  // 送信成功
                l_vol_lin_set_synchbreak();
            }
            /* まだ送信データがある */
            else{
                /* チェックサムまで送信していない */
                if( u1l_lin_rs_cnt < u1l_lin_frm_sz ){
                    u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
                    u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) +
                                     ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );
                }
                /* チェックサム送信 */
                else{
                    u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = (l_u8)(~( (l_u8)u2l_lin_chksum ));
                    xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_chksum =
                        (l_u16)u1l_lin_rs_tmp[ u1l_lin_frm_sz ];
                }

                l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );  // ← 次バイト送信
                l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  // 11 bit
                u1l_lin_rs_cnt++;
            }
        }
    }
    break;
```

**処理フロー**:
1. **前回送信バイトのリードバック確認** (`u1l_lin_rs_cnt - 1` のデータ)
2. リードバック失敗 → BITエラー設定 → 中断
3. リードバック成功 → 次バイト送信準備
4. 全送信完了チェック（`u1l_lin_rs_cnt > DL`）
5. チェックサム計算
6. 次バイト送信
7. タイマー起動（11 bit）
8. カウンタインクリメント

#### l_u1g_lin_read_back() の動作

**実装箇所**: [l_slin_drv_cc2340r53.c:493-523](l_slin_drv_cc2340r53.c#L493-L523)

```c
l_u8  l_u1g_lin_read_back(l_u8 u1a_lin_data)
{
    l_u8    u1a_lin_result;
    l_u32   u4a_lin_error_status;

    /* エラーステータスをレジスタから取得 */
    u4a_lin_error_status = U4L_LIN_HWREG(
        ((( UART2_HWAttrs const * )( xnl_lin_uart_handle->hwAttrs ))->baseAddr +
         UART_O_RSR_ECR )
    );
    u4a_lin_error_status = U4G_DAT_ZERO;  /* 一旦エラー検出機能オフ */

    /* エラーが発生しているかをチェック */
    if(    ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_OVERRUN ) == U4G_LIN_UART_ERR_OVERRUN )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_PARITY  ) == U4G_LIN_UART_ERR_PARITY  )
        || ( ( u4a_lin_error_status & U4G_LIN_UART_ERR_FRAMING ) == U4G_LIN_UART_ERR_FRAMING ) )
    {
        u1a_lin_result = U1G_LIN_NG;  // UARTエラー発生
    }
    else
    {
        /* ★ 受信バッファの1バイト目と引数を比較 */
        if(u1l_lin_rx_buf[U1G_LIN_0] != u1a_lin_data)
        {
            u1a_lin_result = U1G_LIN_NG;  // データ不一致 → BITエラー
        }
        else
        {
            u1a_lin_result = U1G_LIN_OK;  // リードバック成功
        }
    }

    return( u1a_lin_result );
}
```

**チェック内容**:
1. **UARTエラーステータス確認**（Overrun/Parity/Framing）
2. **受信バッファと送信データの比較** (`u1l_lin_rx_buf[0]` vs `u1a_lin_data`)
3. いずれかが不一致 → `U1G_LIN_NG` → BITエラー

#### 3-3. Data3～DataN、Checksum送信

同様のプロセスを繰り返し：

```
[Data2送信] → [11 bit] → [タイマー] → [Data2リードバック] → [Data3送信]
[Data3送信] → [11 bit] → [タイマー] → [Data3リードバック] → [Data4送信]
...
[DataN送信] → [11 bit] → [タイマー] → [DataNリードバック] → [Checksum送信]
[Checksum送信] → [11 bit] → [タイマー] → [Checksumリードバック] → [送信完了]
```

**Checksum送信時の特別処理**:
- `u1l_lin_rs_cnt == u1l_lin_frm_sz` でチェックサム計算
- `u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = ~((l_u8)u2l_lin_chksum)`

**送信完了判定**:
- `u1l_lin_rs_cnt > u1l_lin_frm_sz` (DL + 1) でChecksum送信完了
- `l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF )` 呼び出し
- 状態遷移: `BREAK_UART_WAIT`

---

## BITエラー検出とリードバック

### リードバック方式の理由

LINはHalf-Duplex（単線）通信のため、送信と受信が同じ物理線上で行われます。CC2340R53では以下の方式でBITエラー検出を実現：

1. **送信時にUART受信を有効化** (`l_vog_lin_tx_char` 内)
2. **自分の送信データがループバック受信される**
3. **タイマー割り込みで受信バッファを確認** (`l_u1g_lin_read_back`)
4. **送信データと受信データを比較**

### リードバックタイミングの工夫

**なぜ1バイト目はリードバックしないのか？**

```c
if( u1l_lin_rs_cnt > U1G_LIN_0 ){  // 2バイト目以降のみリードバック
    l_u1a_lin_read_back = l_u1g_lin_read_back( u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] );
```

**理由**:
1. **タイミングの問題**: Data1送信直後にタイマー割り込みが発生するが、UART RX割り込みはまだ発生していない可能性がある
2. **バッファの安定性**: 1 bit（52.08 μs）のResponse Spaceは短く、受信バッファが更新されていない可能性
3. **設計の簡素化**: Data2送信時にData1のリードバックを行うことで、十分な時間的マージンを確保

**リードバックスケジュール**:

| タイマー割り込み | 送信バイト | リードバック対象 | 理由 |
|---------------|----------|---------------|------|
| 1回目 (Response Space後) | Data1 | なし | 初回送信 |
| 2回目 (11 bit後) | Data2 | Data1 | 1バイト前を確認 |
| 3回目 (11 bit後) | Data3 | Data2 | 1バイト前を確認 |
| ... | ... | ... | ... |
| N回目 (11 bit後) | Checksum | DataN | 1バイト前を確認 |
| N+1回目 (11 bit後) | なし（完了判定） | Checksum | 最後のバイト確認 |

### BITエラー検出条件

**エラーと判定される条件**:

1. **UARTエラー発生**:
   - Overrun Error
   - Parity Error
   - Framing Error

2. **データ不一致**:
   - 送信データ ≠ 受信バッファのデータ

**エラー発生時の動作**:
```c
if( l_u1a_lin_read_back != U1G_LIN_OK ){
    /* BITエラー */
    xng_lin_frm_buf[ slot ].un_state.st_bit.u2g_lin_e_bit = U2G_LIN_BIT_SET;
    l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON );  // エラーフラグ付き完了
    l_vol_lin_set_synchbreak();  // Break待ちへ戻る
}
```

---

## タイミングチャート

### 全体タイミング（DataLength = 3の場合）

```
時刻軸 (19200bps, 1bit = 52.08μs):
     0        500       1000      1500      2000      2500      3000      3500 (μs)
     |---------|---------|---------|---------|---------|---------|---------|

LIN Bus:
     [Break][Del][Synch][PID][RS][Data1][IBS][Data2][IBS][Data3][IBS][Chksum]
     ^^^^^^ ^^^ ^^^^^^^ ^^^ ^^ ^^^^^^ ^^^ ^^^^^^ ^^^ ^^^^^^ ^^^ ^^^^^^^^
     13bit  1b  10bit  10b 1b  10bit  1b  10bit  1b  10bit  1b   10bit

UART RX INT:
     ↓      ↓   ↓      ↓                    ↓         ↓         ↓
   Break  Del Synch  PID               Data1(LB)  Data2(LB)  Data3(LB)

UART TX:
                              ↑         ↑         ↑         ↑
                            Data1     Data2     Data3    Chksum

タイマーINT:
                              ↑         ↑         ↑         ↑         ↑
                             RS終了   IBS終了   IBS終了   IBS終了   IBS終了
                             (Data1   (Data2   (Data3   (Chksum  (完了判定)
                              送信)    送信)     送信)    送信)

状態遷移:
BREAK_WAIT → SYNCH_WAIT → ID_WAIT → SNDDATA_WAIT -----------------> BREAK_WAIT
                                     |                                ^
                                     +-- cnt=0 → cnt=1 → cnt=2 → cnt=3 → cnt=4

リードバック:
                                               Data1    Data2    Data3   Chksum
                                               確認     確認     確認     確認
```

**凡例**:
- `RS`: Response Space (1 bit)
- `IBS`: Inter-byte Space (1 bit)
- `LB`: Loopback (リードバック受信)
- `cnt`: `u1l_lin_rs_cnt` (送信カウンタ)

### 詳細タイミング（Data2送信時）

```
                    [Data1送信完了]
                         |
                         |<-------- 11 bit = 572.9μs -------->|
                         |                                    |
                         |  [Data1 TX完了]                    |
                         |       ↓                            |
LIN TX Line:     ________/[Data1]\___________________________/[Data2]\____
                                  ↑                           ↑
                                  └─ Stop Bit                 └─ Start Bit

                         |<-IBS->|                            |
                         |  1bit |                            |

UART RX INT:                  ↓
                         [Data1受信]
                         u1l_lin_rx_buf[0] = Data1

タイマーINT:                                                  ↓
                                                         [タイマー割り込み]
                                                         l_vog_lin_tm_int()
                                                              ↓
                                                         [リードバック]
                                                         l_u1g_lin_read_back(Data1)
                                                         比較: u1l_lin_rx_buf[0] == Data1?
                                                              ↓
                                                         [OK → Data2送信]
                                                         l_vog_lin_tx_char(Data2)
                                                              ↓
                                                         [次タイマー起動: 11bit]
```

---

## 状態遷移図

### 送信フレーム処理の状態遷移

```
                      [Master送信: Break]
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│ BREAK_UART_WAIT                                                 │
│  - Framing Error待ち                                            │
│  - データ=0x00確認                                              │
└─────────────────────────────────────────────────────────────────┘
                              ↓ Break検出
                              ↓ Header Timeout起動
┌─────────────────────────────────────────────────────────────────┐
│ SYNCHFIELD_WAIT                                                 │
│  - Synch Field (0x55) 待ち                                      │
└─────────────────────────────────────────────────────────────────┘
                              ↓ 0x55受信
┌─────────────────────────────────────────────────────────────────┐
│ ID_WAIT                                                         │
│  - PID受信待ち                                                  │
│  - パリティチェック                                             │
│  - ID→スロット変換                                              │
│  - 送信/受信判定                                                │
└─────────────────────────────────────────────────────────────────┘
                              ↓ 送信フレーム判定
                              ↓ Header Timeout停止
                              ↓ 送信データコピー
                              ↓ Response Spaceタイマー起動(1bit)
┌─────────────────────────────────────────────────────────────────┐
│ SNDDATA_WAIT                                                    │
│  [タイマー割り込み駆動]                                          │
│                                                                 │
│  u1l_lin_rs_cnt = 0:                                           │
│   - Data1送信                                                   │
│   - タイマー起動(11bit)                                         │
│   - cnt++                                                       │
│                                                                 │
│  u1l_lin_rs_cnt > 0:                                           │
│   - リードバック確認(cnt-1のデータ)                              │
│     NG → BITエラー → BREAK_UART_WAITへ                         │
│     OK:                                                         │
│       - cnt > DL → 送信完了 → BREAK_UART_WAITへ                │
│       - cnt <= DL:                                              │
│           - 次バイト送信                                        │
│           - タイマー起動(11bit)                                 │
│           - cnt++                                               │
└─────────────────────────────────────────────────────────────────┘
                              ↓ 送信完了 or エラー
┌─────────────────────────────────────────────────────────────────┐
│ BREAK_UART_WAIT                                                 │
│  - 次のフレーム待ち                                             │
└─────────────────────────────────────────────────────────────────┘
```

### カウンタとリードバックの対応表

| タイマー回数 | `u1l_lin_rs_cnt` | 送信バイト | リードバック対象 | リードバック結果 |
|-------------|-----------------|----------|---------------|---------------|
| 0 (Response Space) | 0 | - | - | - |
| 1 | 0 | Data1 | なし | - |
| 2 | 1 | Data2 | Data1 (`cnt-1=0`) | OK/NG判定 |
| 3 | 2 | Data3 | Data2 (`cnt-1=1`) | OK/NG判定 |
| ... | ... | ... | ... | ... |
| DL+1 | DL | Checksum | DataDL (`cnt-1=DL-1`) | OK/NG判定 |
| DL+2 | DL+1 | なし（完了） | Checksum (`cnt-1=DL`) | OK/NG判定 |

---

## 重要な実装ポイント

### 1. Half-Duplex対応

**問題**: 送信と受信が同じ物理線上

**解決策**:
- 送信時にUART受信を有効化 (`l_vog_lin_tx_char` で `l_vog_lin_rx_enb` 呼び出し)
- 自分の送信データをループバック受信
- リードバックで比較検証

### 2. タイミング制御

**Response Space**: 1 bit (52.08 μs @ 19200bps)
- PID受信完了からData1送信開始まで

**Inter-byte Space**: 1 bit (52.08 μs @ 19200bps)
- 各データバイト間

**バイト送信時間**: 10 bit (520.8 μs @ 19200bps)
- Start(1) + Data(8) + Stop(1)

**タイマー設定**:
```c
l_vog_lin_bit_tm_set( U1G_LIN_RSSP );                        // 1 bit
l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP ); // 11 bit
```

### 3. エラーハンドリング

**BITエラー検出時**:
1. `u2g_lin_e_bit` フラグ設定
2. `l_vol_lin_set_frm_complete( U1G_LIN_ERR_ON )`
3. `l_vol_lin_set_synchbreak()` で次フレーム待ちへ

**送信成功時**:
1. `l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF )`
2. `l_vol_lin_set_synchbreak()` で次フレーム待ちへ

### 4. チェックサム計算

**累積加算方式**:
```c
u2l_lin_chksum += (l_u16)u1l_lin_rs_tmp[ u1l_lin_rs_cnt ];
u2l_lin_chksum = ( u2l_lin_chksum & U2G_LIN_MASK_FF ) +  // 下位8bit
                 ( (u2l_lin_chksum >> U2G_LIN_8) & U2G_LIN_MASK_FF );  // キャリー加算
```

**最終計算**:
```c
u1l_lin_rs_tmp[ u1l_lin_frm_sz ] = (l_u8)(~( (l_u8)u2l_lin_chksum ));  // 1の補数
```

### 5. 受信バッファ管理

**送信中の受信バッファ**:
- `u1l_lin_rx_buf[0]`: 最新受信バイト（ループバックデータ）
- UART RX割り込みで自動更新
- リードバック時に参照

**バッファ初期化不要**:
- 各送信でループバック受信が上書き
- タイミングが保証されている

---

## まとめ

### ヘッダー受信からレスポンス送信までの流れ

1. **Break → Synch → PID受信** (Header受信フェーズ)
2. **PID解析 → 送信フレーム判定**
3. **Response Space待ち** (1 bit)
4. **Data1送信** (リードバックなし)
5. **Inter-byte Space + Data2送信時間待ち** (11 bit)
6. **Data1リードバック確認 + Data2送信**
7. **繰り返し**: Data3～Checksum送信
8. **Checksumリードバック確認 → 送信完了**

### BITエラー検出の仕組み

- **Half-Duplex特性を活用**: 送信データがループバック受信される
- **遅延確認方式**: N+1バイト目送信時にNバイト目をリードバック
- **UARTエラーも検出**: Overrun/Parity/Framing Error
- **データ比較**: 送信データ vs 受信バッファ

### キーポイント

✅ **1バイト目はリードバックしない** - タイミング的な理由
✅ **タイマー駆動で送信制御** - Response Space / Inter-byte Space
✅ **送信時に受信有効化** - Half-Duplex対応
✅ **エラー時は即座に中断** - BIT Error検出で送信停止

---

## 参考資料

### 関連ファイル

- [l_slin_core_CC2340R53.c](l_slin_core_CC2340R53.c) - コアロジック（状態遷移）
- [l_slin_drv_cc2340r53.c](l_slin_drv_cc2340r53.c) - ドライバ層（送受信、リードバック）
- [LIN_2X_ERROR_DETECTION_DESIGN.md](LIN_2X_ERROR_DETECTION_DESIGN.md) - エラー検出設計
- [INTERBYTE_SPACE_IMPLEMENTATION.md](INTERBYTE_SPACE_IMPLEMENTATION.md) - Inter-byte Space実装

### LIN仕様

- LIN 2.2a Specification
- LIN Physical Layer Specification

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Status:** ✅ 完成
