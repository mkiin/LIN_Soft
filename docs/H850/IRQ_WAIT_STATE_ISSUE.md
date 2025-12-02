# H850 LINライブラリ UART方式でのBREAK_IRQ_WAIT状態の問題分析

## 重要な発見

**UART方式（IRQ不使用）に設定しても、コード内で`BREAK_IRQ_WAIT`状態に遷移する箇所があります。**
これは設計上の問題点であり、修正が必要です。

---

## 1. 問題箇所の特定

### 1-1. Synch Break受信時の状態遷移（RUN状態）

**ファイル**: `l_slin_core_h83687.c` 行756-787

```c
/*** RUN状態の場合 ***/
else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN ){
    switch( u1l_lin_slv_sts ){
    /*** Synch Break(UART)待ち状態 ***/
    case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
        /* Framingエラー発生 */
        if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON ) {
            /* 受信データが00hならば、SynchBreak受信 */
            if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
                /* ヘッダタイムアウトタイマセット */
                l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

                /* ★★★ 問題箇所 ★★★ */
                /* SynchBreak(IRQ待ち)状態に遷移 */
                u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;  // ← ここが問題！
                l_vog_lin_rx_dis();  // UART受信禁止
            }
        }
        break;

    /*** Synch Break(IRQ)待ち状態 ***/
    case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
        /* 何もしない */  // ← この状態でUART受信が来ても無視される
        break;
```

### 1-2. 同じ問題（RUN STANDBY状態）

**ファイル**: `l_slin_core_h83687.c` 行720-751

```c
/*** RUN STANDBY状態の場合 ***/
else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY ){
    /* Synch Break(UART)待ち状態 */
    if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT ){
        if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON ) {
            if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
                l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

                /* ★★★ 問題箇所 ★★★ */
                u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;  // ← ここも問題！
                l_vog_lin_rx_dis();  // UART受信禁止
            }
        }
    }
}
```

---

## 2. 問題の影響

### 2-1. UART方式での動作フロー（現状の問題あり）

```
[UART受信割り込み]
l_vog_lin_rx_int(0x00, FRAMING_ERROR)
    ↓
[Synch Break検出]
u1a_lin_data == 0x00 && Framing Error
    ↓
[★問題発生★]
u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT  // IRQ待ち状態に遷移
l_vog_lin_rx_dis()                               // UART受信禁止
    ↓
[次のSynch Field (0x55)が来ても...]
u1l_lin_slv_sts == BREAK_IRQ_WAIT
    ↓ UART受信割り込みは発生するが...
    ↓
case( U1G_LIN_SLSTS_BREAK_IRQ_WAIT ):
    /* 何もしない */  // ← データを無視！
    break;
    ↓
[★デッドロック状態★]
Synch Fieldを受信できず、ヘッダタイムアウトエラー発生
```

### 2-2. 期待される動作フロー（修正後）

```
[UART受信割り込み]
l_vog_lin_rx_int(0x00, FRAMING_ERROR)
    ↓
[Synch Break検出]
u1a_lin_data == 0x00 && Framing Error
    ↓
[修正後の動作]
#if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT  // Synch Field待ちへ直接遷移
    l_vog_lin_rx_enb()                                // UART受信継続
#elif U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT   // IRQ待ち状態
    l_vog_lin_rx_dis()                                // UART受信禁止
#endif
    ↓
[次のSynch Field (0x55)受信]
u1l_lin_slv_sts == SYNCHFIELD_WAIT
    ↓
case( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
    if( u1a_lin_data == 0x55 ) {
        u1l_lin_slv_sts = U1G_LIN_SLSTS_IDENTFIELD_WAIT
    }
    ↓
[正常にフレーム受信継続]
```

---

## 3. 根本原因

### 設計意図

このコードの設計意図は以下の通り：

1. **Synch Break受信後、UART受信を一時停止**
2. **IRQ割り込みでSynch Breakの終了エッジを検出**
3. **エッジ検出後、UART受信を再開してSynch Fieldを受信**

**問題点**: この設計は`U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE`の時のみ有効だが、
コード内に条件分岐（`#if`）がなく、**UART方式でも同じ動作をしてしまう**。

### なぜこの設計になっているか

LIN仕様では、Synch Breakの長さは13bit以上（約1.35ms @ 9600bps）あり、
この間にUART受信を有効にしていると、**複数のフレーミングエラー割り込みが発生**する可能性があります。

**INT方式の利点**:
- Synch Breakの立ち下がりエッジでUART受信開始
- 無駄なフレーミングエラー割り込みを回避

**UART方式の問題**:
- IRQ割り込みがないため、`BREAK_IRQ_WAIT`状態で停止してしまう

---

## 4. 修正方法

### 修正案1: 条件コンパイルで状態遷移を分岐（推奨）

**ファイル**: `l_slin_core_h83687.c` 行761-767, 行727-734

```c
/* 受信データが00hならば、SynchBreak受信 */
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    /* ヘッダタイムアウトタイマセット */
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

    /* Wake-up検出方式に応じて状態遷移を変更 */
    #if U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
        /* INT方式: IRQ待ち状態に遷移 */
        u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
        l_vog_lin_rx_dis();  // UART受信禁止（IRQ待機）
    #elif U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
        /* UART方式: 直接Synch Field待ちに遷移 */
        u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
        l_vog_lin_rx_enb();  // UART受信継続
    #endif
}
```

**メリット**:
- コンパイル時に不要なコードが削除される
- 実行時のオーバーヘッドなし
- 明確な動作分岐

**デメリット**:
- 2箇所の修正が必要（RUN状態 + RUN STANDBY状態）

### 修正案2: ランタイム判定で状態遷移を分岐

```c
/* 受信データが00hならば、SynchBreak受信 */
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    /* ヘッダタイムアウトタイマセット */
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

    /* Wake-up検出方式に応じて状態遷移を変更 */
    if( U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE ) {
        /* INT方式: IRQ待ち状態に遷移 */
        u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
        l_vog_lin_rx_dis();
    } else {
        /* UART方式: 直接Synch Field待ちに遷移 */
        u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
        l_vog_lin_rx_enb();
    }
}
```

**メリット**:
- 実行時に設定変更可能（あまり使わない）

**デメリット**:
- わずかな実行時オーバーヘッド
- コードサイズが大きくなる

---

## 5. UART方式でのSynch Break処理の代替案

### 代替案A: Synch Breakを無視（簡易版）

```c
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

    #if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
        /* Synch Breakを無視して、次のSynch Field待ち */
        u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
        /* エラーフラグはクリアされるので問題なし */
    #else
        u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
        l_vog_lin_rx_dis();
    #endif
}
```

**動作**:
- Synch Break (0x00 + Framing Error) を受信
- すぐにSynch Field待ち状態へ遷移
- 次の0x55を受信してID受信へ進む

**問題なし**: Synch Breakの長さは13bit以上あるが、最初の1バイト分を検出すれば十分

### 代替案B: 複数のFraming Errorを許容（堅牢版）

```c
#if U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
    /* UART方式: Synch Break中のフレーミングエラーを許容 */
    if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
        if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT ) {
            /* 最初のSynch Break検出 */
            l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME );
            u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
        }
        /* 2回目以降のSynch Break (0x00)は無視 */
    }
#else
    /* INT方式: 従来の動作 */
    ...
#endif
```

**動作**:
- Synch Break期間中に複数回0x00を受信しても問題なし
- 最初の1回だけ状態遷移、以降は無視

---

## 6. 修正の影響範囲

### 6-1. 修正が必要なファイル

| ファイル | 関数 | 行番号 | 修正内容 |
|---------|------|--------|---------|
| `l_slin_core_h83687.c` | `l_vog_lin_rx_int()` | 761-767 | Synch Break受信時の状態遷移（RUN状態） |
| `l_slin_core_h83687.c` | `l_vog_lin_rx_int()` | 727-734 | Synch Break受信時の状態遷移（RUN STANDBY状態） |

### 6-2. テストが必要な項目

1. **UART方式でのフレーム受信**
   - Synch Break → Synch Field → ID → Data → Checksum の正常受信
   - 複数フレームの連続受信

2. **Wake-up動作**
   - スリープ状態からのWake-up信号受信
   - Wake-up後の通常フレーム受信

3. **エラーハンドリング**
   - ヘッダタイムアウトエラー
   - チェックサムエラー

4. **INT方式での動作確認**
   - 修正後もINT方式が正常動作するか

---

## 7. 推奨する修正コード

### 修正箇所1: RUN状態でのSynch Break受信（行756-787）

```c
/*** Synch Break(UART)待ち状態 ***/
case( U1G_LIN_SLSTS_BREAK_UART_WAIT ):
    /* Framingエラー発生 */
    if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON ) {
        /* 受信データが00hならば、SynchBreak受信 */
        if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
            /* ヘッダタイムアウトタイマセット */
            l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

            /* ========== 修正箇所 START ========== */
            #if U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
                /* INT方式: Synch BreakのエッジをIRQで検出 */
                u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
                l_vog_lin_rx_dis();  // UART受信禁止（IRQ待機）
            #elif U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
                /* UART方式: 直接Synch Field待ちへ遷移 */
                u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
                /* UART受信は継続（l_vog_lin_rx_enb()は既に実行済み） */
            #endif
            /* ========== 修正箇所 END ========== */
        }
        /* 受信データが00h以外ならば */
        else {
            /* エラーフラグのリセット */
            l_vog_lin_rx_enb();  /* Synch Break待ち */
        }
    }
    /* Over Runエラーの発生 */
    else if( (u1a_lin_err & U1G_LIN_OVERRUN_ERR) != U1G_LIN_BYTE_CLR ) {
        /* エラーフラグのリセット */
        l_vog_lin_rx_enb();  /* Synch Break待ち */
    }
    /* エラー未発生 */
    else {
        /* エラー発生していない場合は何もしない */
    }
    break;
```

### 修正箇所2: RUN STANDBY状態でのSynch Break受信（行720-751）

```c
/*** RUN STANDBY状態の場合 ***/
else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY ){
    /* Synch Break(UART)待ち状態 */
    if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT ){
        /* Framingエラー発生 */
        if( (u1a_lin_err & U1G_LIN_FRAMING_ERR_ON) == U1G_LIN_FRAMING_ERR_ON ) {
            /* 受信データが00hならば、SynchBreak受信 */
            if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA ) {
                xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;
                /* ヘッダタイムアウトタイマセット */
                l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );

                /* ========== 修正箇所 START ========== */
                #if U1G_LIN_WAKEUP == U1G_LIN_WP_INT_USE
                    /* INT方式: Synch BreakのエッジをIRQで検出 */
                    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
                    l_vog_lin_rx_dis();  // UART受信禁止（IRQ待機）
                #elif U1G_LIN_WAKEUP == U1G_LIN_WP_UART_USE
                    /* UART方式: 直接Synch Field待ちへ遷移 */
                    u1l_lin_slv_sts = U1G_LIN_SLSTS_SYNCHFIELD_WAIT;
                    /* UART受信は継続 */
                #endif
                /* ========== 修正箇所 END ========== */
            }
            /* 受信データが00h以外ならば */
            else {
                /* エラーフラグのリセット */
                l_vog_lin_rx_enb();  /* Synch Break待ち */
            }
        }
        /* Over Runエラーの発生 */
        else if( (u1a_lin_err & U1G_LIN_OVERRUN_ERR) != U1G_LIN_BYTE_CLR ) {
            /* エラーフラグのリセット */
            l_vog_lin_rx_enb();  /* Synch Break待ち */
        }
        /* エラー未発生 */
        else {
            /* エラー発生していない場合は何もしない */
        }
    }
}
```

---

## 8. まとめ

### 現状の問題

**`U1G_LIN_WAKEUP = U1G_LIN_WP_UART_USE`に設定しても、UART方式では正常動作しません。**

**理由**:
- Synch Break受信後、`BREAK_IRQ_WAIT`状態に遷移してしまう
- この状態ではUART受信データを無視するため、Synch Fieldを受信できない
- ヘッダタイムアウトエラーが発生する

### 必要な修正

**2箇所のコード修正が必須**:
1. RUN状態でのSynch Break受信処理（行761-767）
2. RUN STANDBY状態でのSynch Break受信処理（行727-734）

**修正内容**:
- `#if U1G_LIN_WAKEUP`で条件分岐を追加
- UART方式では`SYNCHFIELD_WAIT`に直接遷移
- INT方式では従来通り`BREAK_IRQ_WAIT`に遷移

### 修正後の動作

**UART方式**:
```
BREAK_UART_WAIT → SYNCHFIELD_WAIT → IDENTFIELD_WAIT → ...
```

**INT方式**:
```
BREAK_UART_WAIT → BREAK_IRQ_WAIT → (IRQ) → SYNCHFIELD_WAIT → ...
```

### 重要性

**この修正なしでは、UART方式でのLIN通信は動作しません。**
IRQ割り込みを使わない実装を行う場合、この修正は**必須**です。

---

**Document Version**: 1.0
**Last Updated**: 2025-01-22
**Author**: LIN IRQ State Analysis Tool
