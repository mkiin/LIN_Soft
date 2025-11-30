# IRQ割り込み発生のメカニズム（H850）

## 変更日
2025-12-01

## 質問
H850でIRQ割り込みを発生させるにはどうするのか？

---

## H850のIRQ割り込みメカニズム

### ハードウェア構成

```
LINバス
  ↓
LINトランシーバ
  ↓ RX信号
  ├→ UART_RX（シリアル通信ポート）
  └→ IRQ0/IRQ1（外部割り込みピン）← ★ Break Delimiter検出用
```

**重要:**
- LINトランシーバのRX出力は**2つのピンに接続**されている
- **UART_RX**: シリアルデータ受信用
- **IRQ0/IRQ1**: 外部割り込み用（エッジ検出）

---

## IRQ割り込みの初期化

### l_vog_lin_int_init()関数

**[H850/l_slin_drv_h83687.c:163-180](../H850/slin_lib/l_slin_drv_h83687.c#L163-L180)**
```c
void  l_vog_lin_int_init(void)
{
    l_u8    u1a_lin_flg;

    /*** INTの初期化 ***/
    u1a_lin_flg = l_u1g_lin_irq_dis();              /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_CLR;            /* 割り込み要求ディセーブル */
    U1G_LIN_FLG_INTEG = U1G_LIN_BIT_SET;            /* 立上りエッジ検出を選択 */
    U1G_LIN_FLG_PMIRQ = U1G_LIN_BIT_SET;            /* ポートモード:IRQ入力端子の選択 */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();
    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;            /* 割り込み要求フラグクリア */

    l_vog_lin_irq_res( u1a_lin_flg );               /* 割り込み設定を復元 */
}
```

### レジスタ設定の詳細

| レジスタ | ビット | 設定値 | 意味 |
|---------|-------|-------|------|
| **IEGR1** (Interrupt Edge Register) | INTEG | 1 | **立ち上がりエッジ検出** (0→1) |
| **IENR1** (Interrupt Enable Register) | INTIC | 0 | 割り込み無効（初期化時） |
| **PMR1** (Port Mode Register) | PMIRQ | 1 | **IRQ入力モード**（GPIOではなくIRQ機能） |
| **IRR1** (Interrupt Request Register) | INTIR | 0 | 割り込み要求フラグクリア |

**ポイント:**
- `INTEG = 1`: **立ち上がりエッジ（0→1遷移）を検出**
- `PMIRQ = 1`: ピンを**IRQ入力端子として使用**
- これにより、LINバスがドミナント（0）→レセッシブ（1）に変化すると、IRQ割り込みが発生

---

## IRQ割り込みの許可/禁止

### 割り込み許可: l_vog_lin_int_enb()

**[H850/l_slin_drv_h83687.c:283-306](../H850/slin_lib/l_slin_drv_h83687.c#L283-L306)**
```c
void  l_vog_lin_int_enb(void)
{
    l_u8    u1a_lin_flg;

    u1a_lin_flg = l_u1g_lin_irq_dis();          /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;        /* 割り込み要求フラグクリア */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();
    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_SET;        /* 割り込み要求イネーブル ← ★ここで許可 */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();

    l_vog_lin_irq_res( u1a_lin_flg );           /* 割り込み設定を復元 */
}
```

**処理:**
1. 割り込み要求フラグをクリア（`INTIR = 0`）
2. 割り込み許可（`INTIC = 1`）
3. これ以降、IRQピンで立ち上がりエッジが検出されると割り込みが発生

### 割り込み禁止: l_vog_lin_int_dis()

**[H850/l_slin_drv_h83687.c:309-332](../H850/slin_lib/l_slin_drv_h83687.c#L309-L332)**
```c
void  l_vog_lin_int_dis(void)
{
    l_u8    u1a_lin_flg;

    u1a_lin_flg = l_u1g_lin_irq_dis();          /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIC = U1G_LIN_BIT_CLR;        /* 割り込み要求ディセーブル ← ★ここで禁止 */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();
    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;        /* 割り込み要求フラグクリア */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();

    l_vog_lin_irq_res( u1a_lin_flg );           /* 割り込み設定を復元 */
}
```

**処理:**
1. 割り込み禁止（`INTIC = 0`）
2. 割り込み要求フラグをクリア（`INTIR = 0`）
3. これ以降、IRQピンでエッジが検出されても割り込みは発生しない

---

## IRQ割り込みハンドラ

### l_ifc_aux_ch1()関数（割り込みベクタから呼ばれる）

**[H850/l_slin_drv_h83687.c:515-533](../H850/slin_lib/l_slin_drv_h83687.c#L515-L533)**
```c
void  l_ifc_aux_ch1(void)
{
    l_u8    u1a_lin_flg;

    /*** 外部INT割り込みの報告 ***/
    u1a_lin_flg = l_u1g_lin_irq_dis();          /* 割り込み禁止設定 */

    U1G_LIN_FLG_INTIR = U1G_LIN_BIT_CLR;        /* 割り込み要求フラグクリア */
    l_vog_lin_nop();
    l_vog_lin_nop();
    l_vog_lin_nop();

    l_vog_lin_irq_res( u1a_lin_flg );           /* 割り込み設定を復元 */

    l_vog_lin_irq_int();                        /* スレーブタスクに外部INT割り込み報告 */
}
```

**処理フロー:**
```
IRQピンで立ち上がりエッジ検出
  ↓
ハードウェア割り込み発生
  ↓
割り込みベクタから l_ifc_aux_ch1() 呼び出し
  ↓
割り込み要求フラグクリア
  ↓
l_vog_lin_irq_int() 呼び出し（CORE層）
  ↓
BREAK_IRQ_WAIT → SYNCHFIELD_WAIT状態遷移
```

---

## Break Field受信時の動作シーケンス

### ステップ1: Break開始検出（UART割り込み）

**時刻t=0:**
```
LINバス: ドミナント（0）継続 → Framing Error
  ↓
UART受信割り込み発生
  ↓
l_ifc_rx_ch1() → l_vog_lin_rx_int()
  ↓
Break検出（0x00 + Framing Error）
  ↓
BREAK_IRQ_WAIT状態へ遷移
  ↓
l_vog_lin_rx_dis()  ← UART受信割り込み禁止
（まだ外部IRQは禁止のまま）
```

### ステップ2: Break Delimiter検出（IRQ割り込み）

**時刻t=1:**
```
LINバス: ドミナント（0）→ レセッシブ（1）← ★立ち上がりエッジ
  ↓
IRQピンで立ち上がりエッジ検出
  ↓
外部IRQ割り込み発生
  ↓
l_ifc_aux_ch1() → l_vog_lin_irq_int()
  ↓
SYNCHFIELD_WAIT状態へ遷移
  ↓
l_vog_lin_int_dis()  ← 外部IRQ割り込み禁止
l_vog_lin_rx_enb()   ← UART受信割り込み許可
```

### ステップ3: Synch Field受信（UART割り込み）

**時刻t=2:**
```
LINバス: 0x55データ受信
  ↓
UART受信割り込み発生
  ↓
l_ifc_rx_ch1() → l_vog_lin_rx_int()
  ↓
Synch Field (0x55) 確認
  ↓
IDENTFIELD_WAIT状態へ遷移
```

---

## タイミングチャート

```
時刻  LINバス    UART割り込み  IRQ割り込み   状態
----  ---------  -----------  -----------  --------------------
t=0   00000000   発生         禁止         BREAK_UART_WAIT
      (Break)    ↓
                 Break検出
                 UART禁止
                               有効化       BREAK_IRQ_WAIT
                               待機中...

t=1   00000000   禁止中       待機中       BREAK_IRQ_WAIT
      (Break継続)

t=2   0→1        禁止中       発生!!       BREAK_IRQ_WAIT
      (Delimiter)              ↓
                               Delimiter検出
                               IRQ禁止
                 有効化

t=3   11111111   待機中       禁止中       SYNCHFIELD_WAIT
      (Delimiter)

t=4   01010101   発生         禁止中       SYNCHFIELD_WAIT
      (0x55)     ↓
                 Synch検出

t=5   データ      発生         禁止中       IDENTFIELD_WAIT
      (Protected ↓
       ID)       ID検出
```

---

## H850でIRQ割り込みを発生させる条件

### 必須条件

1. **ハードウェア接続**
   - LINトランシーバのRX信号が**IRQ0またはIRQ1ピン**に接続されている
   - 同じRX信号が**UARTポート**にも接続されている

2. **レジスタ設定**
   - `INTEG = 1`: 立ち上がりエッジ検出
   - `PMIRQ = 1`: IRQ入力モード
   - `INTIC = 1`: 割り込み許可

3. **ソフトウェア制御**
   - `l_vog_lin_int_init()`: 初期化
   - `l_vog_lin_int_enb()`: 割り込み許可（BREAK_IRQ_WAIT状態移行前に呼ぶ必要なし、初期化で設定済み）
   - 実際には、Break検出後は**既に初期化済み**なので、IRQは有効

4. **物理的な信号変化**
   - LINバスが**ドミナント（0）からレセッシブ（1）へ遷移**
   - これがBreak Delimiterに相当

---

## CC23xxでIRQ割り込みを実装する場合の要件

### 必要なハードウェア変更

CC23xxで同じIRQ方式を実装するには：

1. **GPIO追加接続**
   - LINトランシーバのRX信号を、UART_RXに加えて**別のGPIOピンにも接続**
   - 例: DIO3やDIO4など

2. **GPIO設定**
   - GPIOを**入力モード**に設定
   - **立ち上がりエッジ割り込み**を有効化
   - CC23xxの場合: GPIO Driver APIを使用

### 必要なソフトウェア実装

**DRV層 (l_slin_drv_cc2340r53.c):**
```c
/* GPIO割り込みハンドラ */
void linGpioCallback(uint_least8_t index)
{
    /* Break Delimiter検出 */
    l_vog_lin_irq_int();  /* CORE層のIRQ処理を呼ぶ */
}

/* GPIO初期化 */
void l_vog_lin_int_init(void)
{
    GPIO_setConfig(CONFIG_GPIO_LIN_IRQ, GPIO_CFG_IN_PU | GPIO_CFG_INT_RISING);
    GPIO_setCallback(CONFIG_GPIO_LIN_IRQ, linGpioCallback);
    GPIO_enableInt(CONFIG_GPIO_LIN_IRQ);
}

/* 割り込み許可 */
void l_vog_lin_int_enb(void)
{
    GPIO_enableInt(CONFIG_GPIO_LIN_IRQ);
}

/* 割り込み禁止 */
void l_vog_lin_int_dis(void)
{
    GPIO_disableInt(CONFIG_GPIO_LIN_IRQ);
}
```

**CORE層 (l_slin_core_CC2340R53.c):**
```c
/* 外部IRQ割り込み処理（H850と同じ） */
void l_vog_lin_irq_int(void)
{
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN ){
        if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_IRQ_WAIT ){
            u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
            l_vog_lin_int_dis();   /* IRQ割り込み禁止 */
            l_vog_lin_rx_enb();    /* UART受信許可 */
        }
    }
}
```

---

## 簡易実装（IRQなし）の推奨理由

### ハードウェアIRQ実装の課題

1. **ハードウェア変更が必要**
   - 基板レイアウト変更
   - 追加配線
   - GPIOピンの確保

2. **実用上の必要性は低い**
   - LIN規格では、Synch Field (0x55) で同期を取る
   - Delimiter検出の精度向上効果は限定的
   - UART受信だけで十分機能する

### 簡易実装のメリット

1. **ハードウェア変更不要**
2. **実装が簡単**
3. **機能的に十分**

**推奨:** Break検出後、直接SYNCHFIELD_WAIT状態へ遷移する簡易実装

---

## まとめ

### H850でIRQ割り込みを発生させる方法

1. **ハードウェア**: LIN RX信号を**IRQピンに接続**
2. **レジスタ設定**: **立ち上がりエッジ検出**を設定（`INTEG=1`, `PMIRQ=1`）
3. **割り込み許可**: `l_vog_lin_int_enb()`で許可（初期化で既に設定）
4. **物理的変化**: LINバスが**0→1遷移**（Break Delimiter）
5. **割り込み発生**: ハードウェアがエッジ検出 → `l_ifc_aux_ch1()` → `l_vog_lin_irq_int()`

### CC23xxでの対応

- **ハードウェアIRQ未実装**のため、簡易実装を推奨
- Break検出後、**直接SYNCHFIELD_WAIT**へ遷移
- 機能的には問題なし

---

## 参照

- H850 IRQ初期化: [l_slin_drv_h83687.c:163-180](../H850/slin_lib/l_slin_drv_h83687.c#L163-L180)
- H850 IRQ許可: [l_slin_drv_h83687.c:283-306](../H850/slin_lib/l_slin_drv_h83687.c#L283-L306)
- H850 IRQハンドラ: [l_slin_drv_h83687.c:515-533](../H850/slin_lib/l_slin_drv_h83687.c#L515-L533)
- Break Delimiter説明: [IRQ_BREAK_DELIMITER_EXPLANATION.md](IRQ_BREAK_DELIMITER_EXPLANATION.md)

---

**Document Version:** 1.0
**Last Updated:** 2025-12-01
**Author:** H850 IRQ Mechanism Analysis
