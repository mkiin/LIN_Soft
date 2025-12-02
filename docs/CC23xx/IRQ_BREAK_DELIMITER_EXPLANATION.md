# IRQ（外部割り込み）とBREAK_IRQ_WAIT状態の説明

## 変更日
2025-12-01

## 質問
`U1G_LIN_SLSTS_BREAK_IRQ_WAIT`状態における「IRQ」とは何か？

---

## IRQとは

### IRQ = Interrupt Request（割り込み要求）

**H850での実装:**
- **外部割り込み（External Interrupt）** を指す
- LIN通信では、**Break Delimiter（ブレーク区切り）** の検出に使用
- GPIOピンの**立ち上がりエッジ検出**で実装

---

## LIN Break Fieldの構造

LIN仕様では、Break Fieldは以下の2つの部分で構成されます：

```
┌─────────────────┬──────────────┬──────────────┐
│ Break (Dominant)│ Delimiter    │ Synch Field  │
│  13 bit以上     │ 1 bit以上    │ 10 bit       │
│  (0x00状態)     │ (レセッシブ) │ (0x55)       │
└─────────────────┴──────────────┴──────────────┘
     ↑                ↑
   UART検出        IRQ検出
 (Framing Error)  (立ち上がりエッジ)
```

### 各部分の詳細

#### 1. Break (Dominant)
- **長さ**: 最低13ビット以上のドミナント状態（0）
- **検出方法**: UART Framing Error + データ0x00

#### 2. Delimiter (Break Delimiter)
- **長さ**: 最低1ビット以上のレセッシブ状態（1）
- **役割**: Break終了を示す
- **検出方法**: **外部割り込み（IRQ）** で立ち上がりエッジを検出

#### 3. Synch Field
- **データ**: 0x55 (01010101b)
- **検出方法**: UART受信データ

---

## H850でのIRQ使用理由

### 問題: UARTだけでは不十分

**UART Framing Errorだけでは:**
- Break開始は検出できる（ドミナント継続 → Framing Error）
- **しかし、Break終了（Delimiter）は検出できない**

**LIN仕様要件:**
- Break終了後、**正確なタイミング**でSynch Fieldを受信する必要がある
- Delimiter検出が重要

### 解決策: 外部割り込み（IRQ）

**H850の実装:**
```
LINバス信号
     ↓
GPIOピン（外部割り込み機能付き）
     ↓
立ち上がりエッジ検出
     ↓
IRQ割り込み発生
     ↓
l_vog_lin_irq_int() 呼び出し
```

**利点:**
- Delimiter（0→1遷移）を**ハードウェア**で正確に検出
- ソフトウェアポーリング不要
- タイミング精度向上

---

## BREAK_IRQ_WAIT状態の意味

### 状態遷移シーケンス

#### ステップ1: BREAK_UART_WAIT（Break開始検出）

**[H850/l_slin_core_h83687.c:761-766](../H850/slin_lib/l_slin_core_h83687.c#L761-L766)**
```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    /* ヘッダタイムアウトタイマセット */
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    /* SynchBreak(IRQ待ち)状態に移行 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
    l_vog_lin_rx_dis();  // UART受信割り込み禁止
}
```

**処理:**
1. UART受信割り込みで0x00 + Framing Errorを検出
2. Break開始を確認
3. **UART受信割り込みを禁止** （l_vog_lin_rx_dis()）
4. **BREAK_IRQ_WAIT状態へ遷移**
5. **外部IRQ割り込みを待つ**

#### ステップ2: BREAK_IRQ_WAIT（Delimiter待ち）

**[H850/l_slin_core_h83687.c:1275-1278](../H850/slin_lib/l_slin_core_h83687.c#L1275-L1278)**
```c
else if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_IRQ_WAIT ){
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;  /* Synch Field待ち状態に移行 */
    l_vog_lin_int_dis();                                /* INT割り込みを禁止する */
    l_vog_lin_rx_enb();                                 /* 受信割り込み許可 */
}
```

**処理:**
1. 外部IRQ割り込み発生（Delimiter検出 = 0→1遷移）
2. `l_vog_lin_irq_int()`が呼ばれる
3. **SYNCHFIELD_WAIT状態へ遷移**
4. **外部IRQ割り込みを禁止**
5. **UART受信割り込みを許可** （l_vog_lin_rx_enb()）
6. Synch Field (0x55) の受信待ち

### タイムライン図

```
時刻  信号状態      イベント                        状態
----  ----------  ------------------------------  --------------------
t=0   ドミナント   Break開始                       BREAK_UART_WAIT
      0000000...   ↓ UART Framing Error
                   ↓ l_vog_lin_rx_int()

t=1   ドミナント   Break継続中                     BREAK_IRQ_WAIT
      0000000...   ↓ UART受信禁止
                   ↓ 外部IRQ割り込み待ち

t=2   レセッシブ   Delimiter（立ち上がりエッジ）    BREAK_IRQ_WAIT
      0→1         ↓ 外部IRQ発生
                   ↓ l_vog_lin_irq_int()

t=3   レセッシブ   Synch Field待ち                 SYNCHFIELD_WAIT
      1111...      ↓ UART受信許可
                   ↓ 外部IRQ禁止

t=4   データ       Synch Field受信                 SYNCHFIELD_WAIT
      01010101     ↓ UART受信割り込み
      (0x55)       ↓ l_vog_lin_rx_int()
```

---

## CC23xxでのIRQ実装の問題

### CC23xxの状況

**CC23xxのハードウェア:**
- LINトランシーバからのRX信号はUART0_RXピンに接続
- **Break Delimiter検出用の外部割り込みピンが未接続**
- GPIOでの立ち上がりエッジ検出は実装されていない

### 現状の実装（CC23xx）

**[CC23xx/l_slin_core_CC2340R53.c:772-775](l_slin_core_CC2340R53.c#L772-L775)**
```c
/*** Synch Break(IRQ)待ち状態 ***/
case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
    /* 何もしない */
    break;
```

**問題点:**
1. `U1G_LIN_SLSTS_BREAK_IRQ_WAIT`状態は定義されている
2. **しかし、外部IRQ割り込みハンドラ（l_vog_lin_irq_int）が実装されていない**
3. **BREAK_IRQ_WAIT → SYNCHFIELD_WAITへの遷移ができない**
4. **ヘッダータイムアウトまでこの状態で待ち続ける**

---

## 解決策の検討

### オプション1: 外部IRQ割り込みを実装（理想）

**必要な作業:**
1. LINトランシーバのRX信号を別のGPIOピンにも接続（ハードウェア変更）
2. GPIOで立ち上がりエッジ割り込みを設定
3. `l_vog_lin_irq_int()`関数を実装
4. Break Delimiter検出でSYNCHFIELD_WAITへ遷移

**利点:**
- H850完全互換
- 正確なDelimiter検出
- タイミング精度向上

**欠点:**
- ハードウェア変更が必要
- 追加のGPIOピンが必要

---

### オプション2: BREAK_IRQ_WAIT状態を省略（簡易）

**実装方針:**
- Break検出後、**即座にSYNCHFIELD_WAIT状態へ遷移**
- 外部IRQ割り込みを使用しない
- UART受信割り込みで次のデータ（Synch Field）を待つ

**修正内容:**

#### 修正1: Break検出時の処理（l_vog_lin_rx_int）

**現状（CC23xx）:**
```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;  // ← IRQ待ち
    l_vog_lin_rx_dis();
}
```

**修正案:**
```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;  // ← 直接Synch Field待ちへ
    // l_vog_lin_rx_dis(); を削除 → UART受信継続
    l_vog_lin_rx_enb();  // UART受信許可を明示的に呼ぶ
}
```

#### 修正2: BREAK_IRQ_WAIT状態のケースを削除

**現状:**
```c
case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
    /* 何もしない */
    break;
```

**修正案:**
```c
// このcase文を削除（到達しない）
```

**利点:**
- ハードウェア変更不要
- 実装が簡潔
- UARTだけで完結

**欠点:**
- Break Delimiter検出の精度が下がる
- H850と完全互換ではない
- Synch Fieldの受信タイミングが若干ずれる可能性

---

### オプション3: タイマーベースのDelimiter検出（中間案）

**実装方針:**
- Break検出後、**短時間のタイマー待機**（1-2ビット時間）
- タイマータイムアウトでSYNCHFIELD_WAIT状態へ遷移
- Delimiter相当の待機時間を確保

**修正内容:**
```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    /* Break Delimiter相当の短時間タイマー（2ビット程度） */
    l_vog_lin_bit_tm_set( 2 );  // 2ビット時間待機
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
    l_vog_lin_rx_dis();
}
```

**タイマー割り込みハンドラ（l_vog_lin_tm_int）:**
```c
case ( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
    /* Break Delimiter相当の待機完了 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
    l_vog_lin_rx_enb();  // UART受信許可
    /* タイマー再設定（残りヘッダータイムアウト） */
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH - 2 );
    break;
```

**利点:**
- ハードウェア変更不要
- Delimiter相当の待機時間を確保
- タイミング精度がオプション2より良い

**欠点:**
- タイマー処理が複雑化
- H850完全互換ではない

---

## 推奨実装

### 推奨: オプション2（BREAK_IRQ_WAIT状態省略）

**理由:**
1. CC23xxはハードウェアIRQ未実装
2. オプション1はハードウェア変更が必要（現実的でない）
3. オプション2が最もシンプルで実装が容易
4. LIN仕様上、Synch Fieldの検出で問題なし

**実装方針:**
- Break検出 → 即座にSYNCHFIELD_WAIT
- UART受信割り込みで次のデータ（0x55）を待つ
- BREAK_IRQ_WAIT状態を実質的に省略

---

## H850との互換性

| 項目 | H850 | CC23xx（推奨実装） | 互換性 |
|-----|------|-------------------|-------|
| **Break検出** | UART Framing Error | UART Framing Error | ✅ 同じ |
| **Delimiter検出** | 外部IRQ（エッジ検出） | **省略** | ⚠️ 異なる |
| **Synch Field検出** | UART受信データ | UART受信データ | ✅ 同じ |
| **状態遷移** | BREAK_UART_WAIT → BREAK_IRQ_WAIT → SYNCHFIELD_WAIT | BREAK_UART_WAIT → SYNCHFIELD_WAIT | ⚠️ 簡略化 |

**影響:**
- 機能的には問題なし（Synch Fieldで正常に同期）
- タイミング精度がわずかに低下（実用上は許容範囲）

---

## 実装例

### 修正箇所

**ファイル:** l_slin_core_CC2340R53.c

#### 修正1: Break検出処理（740-751行目, 698-711行目）

**現状:**
```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
    l_vog_lin_rx_dis();
}
```

**修正後:**
```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    /* CC23xxではIRQ未実装のため、直接SYNCHFIELD_WAITへ遷移 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
    l_vog_lin_rx_enb();  /* UART受信継続 */
}
```

#### 修正2: BREAK_IRQ_WAIT caseの削除（772-775行目）

**現状:**
```c
/*** Synch Break(IRQ)待ち状態 ***/
case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
    /* 何もしない */
    break;
```

**修正後:**
```c
// このcase文を削除（到達しない）
```

---

## まとめ

### IRQとは
- **Interrupt Request（割り込み要求）**
- H850では**外部割り込み（GPIO立ち上がりエッジ検出）**
- **Break Delimiter検出**に使用

### BREAK_IRQ_WAIT状態の役割
- Break終了（Delimiter）の**外部IRQ割り込み待ち**状態
- IRQ発生後、SYNCHFIELD_WAIT状態へ遷移

### CC23xxでの対応
- **外部IRQ未実装**のため、BREAK_IRQ_WAIT状態を省略
- Break検出後、**直接SYNCHFIELD_WAIT**へ遷移
- UART受信割り込みで次のSynch Field (0x55) を待つ

### 推奨実装
- **オプション2: BREAK_IRQ_WAIT状態省略**
- シンプルで実装が容易
- 機能的には問題なし

---

## 参照

- H850実装: [l_slin_core_h83687.c:1248-1289](../H850/slin_lib/l_slin_core_h83687.c#L1248-L1289)
- CC23xx現状: [l_slin_core_CC2340R53.c:772-775](l_slin_core_CC2340R53.c#L772-L775)
- LIN仕様書: Break Field定義

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Author:** LIN IRQ Analysis
