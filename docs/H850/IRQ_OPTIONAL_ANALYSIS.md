# H850 LINライブラリ IRQ割り込み不要化の可能性分析

## 結論

**はい、IRQ割り込みを使わずにこのLINライブラリを実現することは可能です。**

このライブラリは設計段階から2つのWake-up検出方式に対応しており、コンパイル時の設定変更だけでIRQ割り込みなしでの動作が可能です。

---

## 1. Wake-up検出方式の選択肢

### 設定ファイル: `l_slin_def.h`

```c
#define  U1G_LIN_WAKEUP  (U1G_LIN_WP_INT_USE)  // 現在の設定
```

### 選択可能な方式

| 方式 | 定義値 | 説明 | IRQ割り込み |
|------|-------|------|-----------|
| **UART方式** | `U1G_LIN_WP_UART_USE` (0) | UARTでWake-up信号を検出 | **不要** |
| **INT方式** | `U1G_LIN_WP_INT_USE` (1) | 外部割り込みでWake-up信号を検出 | **必要** |

**現在の設定**: INT方式（IRQ使用）
**変更後**: UART方式（IRQ不要）に変更可能

---

## 2. IRQ割り込みの役割と代替方法

### IRQ割り込みが使用される場面

#### 使用場面1: スリープ状態からのWake-up検出

**INT方式の動作（現在）:**
```
[スリープ状態]
    ↓
LINバスに立ち下がりエッジ（Wake-up信号）
    ↓
IRQ0外部割り込み発生
    ↓
l_vog_lin_irq_int() 実行
    ↓
    ├─ u1l_lin_slv_sts = SYNCHFIELD_WAIT
    ├─ UART受信許可
    └─ IRQ割り込み禁止
    ↓
[Wake-up完了]
```

**UART方式の動作（代替）:**
```
[スリープ状態]
    ↓
LINバスにWake-up信号（0x80等のデータ）
    ↓
UART受信割り込み発生
    ↓
l_vog_lin_rx_int() 実行
    ↓
    ├─ Wake-upデータ検出
    ├─ u1l_lin_slv_sts = SYNCHFIELD_WAIT
    └─ 通常通信モードへ移行
    ↓
[Wake-up完了]
```

### 各方式の比較

| 項目 | UART方式 | INT方式 |
|------|---------|---------|
| **Wake-up検出速度** | やや遅い（バイト受信完了後） | 速い（エッジ検出） |
| **消費電力** | やや高い（UART常時動作） | 低い（IRQのみ待機） |
| **ハードウェア要件** | UART受信機能のみ | UART + 外部割り込みピン |
| **ノイズ耐性** | 高い（データ内容確認可能） | 低い（エッジのみ検出） |
| **実装の複雑さ** | シンプル | やや複雑 |
| **割り込みピン数** | 0本（UART Rxのみ） | 1本追加（IRQ0） |

---

## 3. コード変更箇所

### 3-1. 設定ファイル変更（必須）

**ファイル**: `H850/slin_lib/l_slin_def.h`

```c
// 変更前
#define  U1G_LIN_WAKEUP  (U1G_LIN_WP_INT_USE)

// 変更後
#define  U1G_LIN_WAKEUP  (U1G_LIN_WP_UART_USE)
```

この1行の変更だけで、IRQ割り込みを使わない動作に切り替わります。

### 3-2. アプリケーション層変更（必要に応じて）

**ファイル**: `H850/application/f_lin_main.c`

```c
// IRQ割り込みハンドラは不要になる
#if U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
#pragma interrupt ( f_vogi_inthdr_irq0 )
void f_vogi_inthdr_irq0(void)
{
    l_ifc_aux_ch1();
}
#endif
```

### 3-3. プロジェクト設定変更

- **割り込みベクタテーブル**: IRQ0割り込みの登録が不要
- **ピン設定**: IRQ0ピンの設定が不要（GPIO等に使用可能）
- **電源管理**: スリープ時のUART受信有効化が必要

---

## 4. コンパイル時の動作分岐

### l_slin_core_h83687.c の自動分岐

#### 4-1. Wake-up待機設定

```c
static void l_vol_lin_set_wakeup(void)
{
  /* WAKEUP検出方法がUARTの場合 */
  #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
    l_vog_lin_int_dis();          // INT割り込みを禁止
    l_vog_lin_rx_enb();           // UART受信割り込み許可

  /* WAKEUP検出方法がINTの場合 */
  #elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
    l_vog_lin_rx_dis();           // UART受信割り込み禁止
    l_vog_lin_int_enb_wakeup();   // INT割り込みを許可
  #endif
}
```

#### 4-2. スリープ移行処理

**`l_ifc_sleep_ch1()` の動作:**

**UART方式:**
```c
void l_ifc_sleep_ch1(void)
{
    // UART受信は有効のまま（Wake-up検出用）
    l_vog_lin_rx_enb();

    // タイマ停止
    l_vog_lin_frm_tm_stop();

    // スレーブタスク状態 = BREAK_UART_WAIT
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT;
}
```

**INT方式（現在）:**
```c
void l_ifc_sleep_ch1(void)
{
    // UART受信禁止
    l_vog_lin_rx_dis();

    // IRQ割り込み許可
    l_vog_lin_int_enb_wakeup();

    // タイマ停止
    l_vog_lin_frm_tm_stop();

    // スレーブタスク状態 = BREAK_IRQ_WAIT
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
}
```

---

## 5. スレーブタスク状態の変化

### UART方式での状態遷移

```
[通常動作]
    ↓
BREAK_UART_WAIT (0)  ← Synch Break待ち（UART受信）
    ↓
SYNCHFIELD_WAIT (2)  ← Synch Field待ち
    ↓
IDENTFIELD_WAIT (3)  ← ID待ち
    ↓
RCVDATA_WAIT (4) / SNDDATA_WAIT (5)
    ↓
BREAK_UART_WAIT (0)

[スリープ時]
BREAK_UART_WAIT (0)  ← UART受信待機
    ↓ (Wake-upデータ受信)
SYNCHFIELD_WAIT (2)
    ↓
[通常動作復帰]
```

### INT方式での状態遷移（現在）

```
[通常動作]
BREAK_UART_WAIT (0)
    ↓
... (同じ) ...
    ↓
BREAK_UART_WAIT (0)

[スリープ時]
BREAK_IRQ_WAIT (1)  ← IRQ割り込み待機
    ↓ (IRQ割り込み発生)
SYNCHFIELD_WAIT (2)
    ↓
[通常動作復帰]
```

**重要**: UART方式では `BREAK_IRQ_WAIT (1)` 状態を使用しない

---

## 6. メリット・デメリット比較

### IRQ割り込み使用（INT方式）のメリット

1. **高速Wake-up応答**
   - エッジ検出は即座に反応（数μs）
   - UART受信よりも約1ms程度高速

2. **低消費電力**
   - スリープ時にUART受信回路を停止可能
   - IRQ割り込みのみで待機

3. **ノイズによる誤Wake-upの可能性**
   - エッジ検出のみなので、短いノイズでも反応

### IRQ割り込み不使用（UART方式）のメリット

1. **ピン節約**
   - IRQ0ピンが不要（GPIO等に使用可能）
   - LIN Rxピン1本のみで完結

2. **ノイズ耐性向上**
   - Wake-upデータ内容を確認可能（0x80等）
   - 短いノイズでは誤Wake-upしない

3. **実装の簡素化**
   - 外部割り込み設定が不要
   - 状態遷移がシンプル

4. **移植性向上**
   - 外部割り込みピンがないマイコンでも動作可能
   - CC2340R53等のGPIO制約があるマイコンに有利

### IRQ割り込み不使用（UART方式）のデメリット

1. **やや高い消費電力**
   - スリープ時もUART受信回路が動作
   - 差は微小（数十〜数百μA程度）

2. **Wake-up応答の遅延**
   - UART受信完了まで待機（約1ms）
   - 実用上は問題ない遅延時間

3. **Wake-upデータパターン依存**
   - 0x80等の特定データパターンが必要
   - LIN仕様で規定されているため問題なし

---

## 7. 推奨設定

### 推奨: UART方式（IRQ不使用）

**理由:**

1. **CC2340R53への移植を考慮**
   - GPIO制約が厳しいマイコンでピン節約が重要
   - 外部割り込みピンの設定が複雑な場合がある

2. **実装の簡素化**
   - 1つの割り込みハンドラ（UART受信）だけで完結
   - デバッグが容易

3. **ノイズ耐性**
   - 車載環境ではノイズが多いため、データ内容確認が有利

4. **消費電力の差は微小**
   - Wake-up待機時間は短い（数秒〜数十秒）
   - トータルでの消費電力への影響は小さい

### INT方式を選択すべきケース

1. **極限まで低消費電力が必要**
   - バッテリー駆動で長期間スリープする場合

2. **高速Wake-up応答が必須**
   - 1ms以下のWake-up応答時間が要求される場合

3. **GPIOピンに余裕がある**
   - IRQ0ピンを割り当てても問題ない場合

---

## 8. CC2340R53への移植時の考慮事項

### CC2340R53の特徴

- **UART2モジュール**: TI独自のUART2ドライバ
- **DIO (Digital I/O)**: GPIO制約あり
- **RTC/Timer**: 高精度タイマ利用可能
- **IOC (I/O Controller)**: ピンマッピング柔軟

### 推奨アプローチ

**UART方式（IRQ不使用）を推奨**

**理由:**
1. CC2340R53のGPIO割り当てを節約
2. UART2モジュールで受信割り込み対応可能
3. DIOをWake-up検出に使う必要がない
4. 移植コード量が少ない

### 移植時の変更箇所

**変更が必要:**
- UART制御（SCI → UART2）
- タイマ制御（Timer Z0 → RTC or GPTimer）
- 割り込みハンドラ登録

**変更不要:**
- `l_slin_core_cc2340r53.c` の Wake-up検出ロジック
- `U1G_LIN_WAKEUP = U1G_LIN_WP_UART_USE` の設定
- 状態遷移ロジック

---

## 9. 動作検証ポイント

### UART方式への変更後の検証項目

1. **Wake-up信号検出**
   - マスタからのWake-up信号（0x80 @ 9600bps）を正しく検出できるか
   - ノイズによる誤Wake-upが発生しないか

2. **スリープ復帰時間**
   - Wake-up信号検出からフレーム受信開始までの時間
   - LIN仕様の要求時間内か（通常は問題なし）

3. **消費電力**
   - スリープ時の消費電流測定
   - INT方式との差を確認（実用上問題ないレベルか）

4. **ノイズ試験**
   - LINバスにノイズを印加してWake-upの安定性確認
   - UART方式の方がノイズ耐性が高いはず

5. **長時間動作試験**
   - スリープ/Wake-upを繰り返しても安定動作するか

---

## 10. まとめ

### IRQ割り込み不要化の可能性: **100%可能**

- **設定変更のみ**: `U1G_LIN_WAKEUP = U1G_LIN_WP_UART_USE`
- **コード変更**: 不要（条件コンパイルで自動対応）
- **機能**: 完全に維持される
- **性能**: 実用上問題なし（Wake-up応答が約1ms遅延）

### 推奨設定

```c
// H850/slin_lib/l_slin_def.h

// 現在（IRQ使用）
#define  U1G_LIN_WAKEUP  (U1G_LIN_WP_INT_USE)

// 推奨（IRQ不使用）
#define  U1G_LIN_WAKEUP  (U1G_LIN_WP_UART_USE)
```

### CC2340R53への移植時の利点

1. **GPIO節約**: IRQ0ピン不要
2. **実装簡素化**: 外部割り込み設定不要
3. **ノイズ耐性向上**: データ内容確認
4. **移植コード削減**: 割り込みハンドラ1つ削減

### 結論

**IRQ割り込みなしでのLIN通信は完全に可能であり、むしろCC2340R53等の最新マイコンへの移植には推奨される方式です。**

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: LIN IRQ Analysis Tool
