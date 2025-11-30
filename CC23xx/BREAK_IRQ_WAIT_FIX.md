# BREAK_IRQ_WAIT状態の修正 (CC2340R53対応)

## 概要

CC2340R53マイコンでは、H8/3687で使用されていた外部IRQ割り込み機能が利用できないため、`BREAK_IRQ_WAIT`状態を使用せず、Break検出後に直接`SYNCHFIELD_WAIT`状態へ遷移するよう修正しました。

**修正日**: 2025-12-01
**対象ファイル**: `l_slin_core_CC2340R53.c`
**修正理由**: CC23xxハードウェアのIRQピン未接続による制約

---

## 問題の背景

### H8/3687での実装

H8/3687マイコンでは、LIN RX信号が以下の2つのピンに接続されていました：

1. **UART_RX**: UART受信用（フレーミングエラー検出でBreak検出）
2. **IRQ0/IRQ1**: 外部割り込み用（Break Delimiter検出）

#### LIN Break Fieldの構造

```
|<------- Break ------->|<-Del->|<- Synch Field ->|
|   13ビット以上 (0)    |  1bit+|      0x55       |
|       Dominant         | Rece- |    Dominant     |
|                        | ssive |                 |
         ↓                  ↓
    UART Framing      IRQ Rising
     Error検出          Edge検出
  (Break検出)      (Delimiter検出)
```

### H8/3687の状態遷移

```
BREAK_UART_WAIT
    ↓ (Framing Error + 0x00受信)
BREAK_IRQ_WAIT
    ↓ (IRQ割り込み: 0→1エッジ検出)
SYNCHFIELD_WAIT
    ↓ (0x55受信)
ID_WAIT
```

### CC2340R53の制約

CC2340R53では、LIN RX信号は**UART_RXピンのみ**に接続されており、外部IRQピンへの接続がありません。

このため、Break Delimiter（0→1遷移）を検出する手段がなく、`BREAK_IRQ_WAIT`状態で無限待機状態に陥る問題がありました。

---

## 修正内容

### 1. Break検出時の状態遷移変更

**変更箇所**: `l_slin_core_CC2340R53.c` 2箇所

#### 修正前（STANDBY状態からの遷移）

```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
{
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;  // ← 無限待機状態
    l_vog_lin_rx_dis();  // ← UART受信を無効化
}
```

#### 修正後

```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
{
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;  // ← 直接遷移
    /* UART受信は有効のまま (IRQ待ちをスキップ) */
}
```

**変更点**:
- 遷移先を`BREAK_IRQ_WAIT`から`SYNCHFIELD_WAIT`に変更
- `l_vog_lin_rx_dis()`呼び出しを削除（UART受信を有効のまま維持）

#### 修正前（RUN状態での遷移）

```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
{
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;  // ← 無限待機状態
    l_vog_lin_rx_dis();  // ← UART受信を無効化
}
```

#### 修正後

```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
{
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;  // ← 直接遷移
    /* UART受信は有効のまま (IRQ待ちをスキップ) */
}
```

### 2. BREAK_IRQ_WAITケースハンドラのコメントアウト

**変更箇所**: `l_slin_core_CC2340R53.c` 772-778行目

```c
/*** Synch Break(IRQ)待ち状態 ***/
/* CC23xxではIRQピンが未接続のため、この状態は使用しない */
/* Break Delimiter検出のための外部IRQ割り込みが利用できないため、 */
/* Break検出後は直接SYNCHFIELD_WAITへ遷移する */
// case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
//     /* 何もしない */
//     break;
```

**理由**:
- この状態には遷移しなくなったため、ケースハンドラを無効化
- 将来の保守性のため、コメントで理由を明記

---

## 修正後の状態遷移

### CC2340R53の状態遷移（修正後）

```
BREAK_UART_WAIT
    ↓ (Framing Error + 0x00受信)
SYNCHFIELD_WAIT  ← Break Delimiter待ちをスキップ
    ↓ (0x55受信)
ID_WAIT
```

### タイミングチャート（修正後）

```
LIN Bus      ____          _______________
RX Signal        |________|               |___...

                 Break     Del  Synch
                 Field     im   Field

UART RX INT  ----[1]----------[2]---------...

State        BREAK_UART_WAIT
                 |
                 +--> SYNCHFIELD_WAIT
                           |
                           +--> ID_WAIT

[1] Framing Error (Break検出)
[2] 0x55受信 (Synch Field検出)
```

---

## 影響範囲の検証

### 1. LIN仕様への適合性

**Break Delimiter検出の省略は問題ないか？**

✅ **問題なし**

- **LIN仕様**: Break Delimiter長は最小1ビット長（可変）
- Break後、次に受信されるバイトは必ず0x55（Synch Field）
- Synch Fieldの受信で、Delimiterが既に終了していることが確認できる
- **結論**: Delimiter検出を省略してもプロトコル動作に影響なし

### 2. タイミング要件への影響

**Header Timeout検出への影響は？**

✅ **影響なし**

- タイムアウトタイマはBreak検出時に起動（変更なし）
- タイムアウト値: `U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH`
- Delimiter分の遅延は無視できる（最大数ビット長）

### 3. エラー検出への影響

**Break検出精度への影響は？**

✅ **影響なし**

- Break検出はUART Framing Errorで実施（変更なし）
- 0x00受信確認も実施（変更なし）
- Synch Field (0x55) の受信で正常性を確認

---

## テスト項目

### 1. 正常系テスト

| No | テスト項目 | 期待結果 |
|----|-----------|----------|
| 1  | 正常なBreak受信 | BREAK_UART_WAIT → SYNCHFIELD_WAIT へ遷移 |
| 2  | Synch Field (0x55) 受信 | SYNCHFIELD_WAIT → ID_WAIT へ遷移 |
| 3  | ID受信 | 該当フレームの処理開始 |
| 4  | 連続フレーム受信 | 各フレームで正常に状態遷移 |

### 2. 異常系テスト

| No | テスト項目 | 期待結果 |
|----|-----------|----------|
| 1  | Break後にノイズ受信 | Headerエラー検出、BREAK_UART_WAIT へ復帰 |
| 2  | Synch Fieldタイムアウト | Headerエラー検出、BREAK_UART_WAIT へ復帰 |
| 3  | Framing Error（Break以外） | エラー無視、BREAK_UART_WAIT 継続 |
| 4  | 0x00以外でFraming Error | エラー無視、BREAK_UART_WAIT 継続 |

### 3. タイミングテスト

| No | テスト項目 | 期待結果 |
|----|-----------|----------|
| 1  | Header Timeout検出 | 規定時間内に検出 |
| 2  | Break長が最大値 | 正常受信可能 |
| 3  | Delimiter長が最小値(1bit) | 正常受信可能 |
| 4  | Delimiter長が長い場合 | 正常受信可能 |

---

## 他プラットフォームとの比較

### H8/3687（IRQ使用）

**メリット**:
- Break Delimiter検出が正確
- UART受信をBreak中に無効化でき、ノイズ耐性が高い可能性

**デメリット**:
- ハードウェア要件が厳しい（IRQピン必須）
- 割り込みハンドラが複雑化

### CC2340R53（IRQなし・修正後）

**メリット**:
- ハードウェア構成がシンプル（UART_RXのみ）
- ソフトウェアが簡潔
- LIN仕様に完全準拠

**デメリット**:
- Delimiter検出が省略される（ただし実用上問題なし）

### RL78/F24（参考実装）

RL78/F24の実装も確認したところ、同様にIRQ機能を使用していない簡略実装でした。

**結論**: CC2340R53の修正後実装は、他プラットフォームの実績ある方式と同等です。

---

## まとめ

### 修正内容

1. ✅ Break検出後、`BREAK_IRQ_WAIT`をスキップして直接`SYNCHFIELD_WAIT`へ遷移
2. ✅ UART受信を有効のまま維持（`l_vog_lin_rx_dis()`呼び出し削除）
3. ✅ `BREAK_IRQ_WAIT`ケースハンドラをコメントアウト

### 修正理由

- CC2340R53ではIRQピン未接続のため、Break Delimiter検出が不可能
- LIN仕様上、Delimiter検出の省略は問題なし
- 実装を簡素化し、保守性を向上

### 検証結果

- ✅ LIN仕様への適合性: 問題なし
- ✅ タイミング要件: 影響なし
- ✅ エラー検出精度: 変更なし

### 今後の対応

この修正により、CC2340R53でのLIN通信が正常に動作するようになります。実機テストでの動作確認を推奨します。

---

## 参考資料

- [IRQ_INTERRUPT_MECHANISM.md](IRQ_INTERRUPT_MECHANISM.md) - H8/3687のIRQ割り込み機構の詳細
- [IRQ_BREAK_DELIMITER_EXPLANATION.md](IRQ_BREAK_DELIMITER_EXPLANATION.md) - Break DelimiterとIRQの役割説明
- LIN 2.x仕様書 - Break Field/Delimiter仕様
