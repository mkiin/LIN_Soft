# タイマー関数の干渉リスク分析

## 質問
3つのタイマー関数は干渉し合う危険性があるのか？

```c
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit);   // ヘッダー/送信間隔タイマー
void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit);   // 受信タイムアウトタイマー
void l_vog_lin_bus_tm_set(void);               // Physical Busエラータイマー
```

---

## 結論

### ⚠️ **干渉リスクあり**

3つの関数は**同一のタイマーハンドル `xnl_lin_timer_handle` を共有**しており、適切な状態管理がないと干渉します。

---

## 問題点

### 1. 同一ハードウェアタイマーの共有

**全ての関数で同じハンドルを使用:**

```c
// l_vog_lin_bit_tm_set()
LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, ...);
LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);

// l_vog_lin_rcv_tm_set()
LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, ...);
LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);

// l_vog_lin_bus_tm_set()
LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, ...);
LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
```

**問題:**
- 後から呼び出された関数が、前のタイマー設定を**上書き**してしまう
- 先に設定したタイムアウトが**キャンセル**される

### 2. 干渉する可能性のあるシーケンス

#### ケース1: 受信タイムアウト中にBus Timerが上書き ❌

```
タイムライン:
t=0    ID受信完了
       → l_vog_lin_rcv_tm_set(90)
       → タイマー起動: 6.56ms後にタイムアウト

t=0.1  データ受信中...

t=0.5  ★何らかの理由でl_vol_lin_set_synchbreak()が呼ばれる★
       → l_vog_lin_bus_tm_set()
       → タイマー上書き: 1.302秒後にタイムアウト

結果: ❌ 6.56msの受信タイムアウトが無効化
      → データ受信エラー検出できない
```

#### ケース2: 送信間隔タイマー中にBus Timerが上書き ❌

```
タイムライン:
t=0    データ送信完了
       → l_vog_lin_bit_tm_set(11)
       → タイマー起動: 572μs後に次バイト送信

t=0.1  ★エラーでl_vol_lin_set_synchbreak()が呼ばれる★
       → l_vog_lin_bus_tm_set()
       → タイマー上書き: 1.302秒後にタイムアウト

結果: ❌ 送信間隔タイマーが無効化
      → これは正常（エラー時は送信中断すべき）
      ✅ この場合は問題なし
```

---

## 現在の実装での干渉回避メカニズム

### ✅ 状態管理による排他制御

LIN通信の状態マシンにより、実際には干渉しない設計になっています：

#### 状態とタイマーの対応関係

| 状態 | 使用タイマー関数 | タイムアウト時間 | 目的 |
|------|----------------|----------------|------|
| **BREAK_UART_WAIT** | `l_vog_lin_bus_tm_set()` | 1.302秒 | Physical Busエラー検出 |
| **BREAK_IRQ_WAIT** | `l_vog_lin_bit_tm_set()` | 2.03ms | ヘッダー受信タイムアウト |
| **SYNCHFIELD_WAIT** | (継続中) | (同上) | ヘッダー受信タイムアウト |
| **IDENTFIELD_WAIT** | (継続中) | (同上) | ヘッダー受信タイムアウト |
| **RCVDATA_WAIT** | `l_vog_lin_rcv_tm_set()` | 6.56ms | データ受信タイムアウト |
| **SNDDATA_WAIT** | `l_vog_lin_bit_tm_set()` | 572μs | 送信間隔タイマー |

### 干渉しない理由

#### 理由1: **状態遷移が排他的**

```
Break待ち (bus_tm)
    ↓ Break検出
Break IRQ待ち (bit_tm: ヘッダー)
    ↓ Synch受信
Synch待ち (bit_tm継続)
    ↓ ID受信
ID待ち (bit_tm継続)
    ↓
    ├→ 受信フレーム → RCVDATA_WAIT (rcv_tm)
    └→ 送信フレーム → SNDDATA_WAIT (bit_tm: 送信間隔)
    ↓ 完了
Break待ち (bus_tm)
```

**ポイント:**
- `bus_tm` → `bit_tm` → `rcv_tm` or `bit_tm(送信)` → `bus_tm` の順
- **各状態で1種類のタイマーのみ使用**
- 次の状態に遷移する前に**必ずタイマーが完了または停止**

#### 理由2: **明示的なタイマー停止**

**送信フレーム時:**
```c
// [l_slin_core_CC2340R53.c:884]
l_vog_lin_frm_tm_stop();  // ヘッダタイムアウトタイマ停止
// ↓
l_vog_lin_bit_tm_set(U1G_LIN_RSSP);  // 送信用タイマー起動
```

**エラー時:**
```c
// [l_slin_core_CC2340R53.c:1286]
l_vol_lin_set_synchbreak()
{
    l_vog_lin_bus_tm_set();  // 新しいタイマー起動
    // 前のタイマーは自動的に上書き（ワンショットなので問題なし）
}
```

#### 理由3: **l_vog_lin_bit_tm_set()内での停止処理**

```c
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    /* タイマー停止 */
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);  // ★明示的停止★
    }

    /* 新しい設定で起動 */
    LGPTimerLPF3_setInitialCounterTarget(...);
    LGPTimerLPF3_start(xnl_lin_timer_handle, ...);
}
```

**メリット:**
- 前のタイマーが確実に停止
- 新しい設定が確実に反映

---

## 潜在的な干渉シナリオ

### ⚠️ 危険なケース1: 割り込み中の呼び出し

```c
// タイマー割り込みハンドラ
void l_vog_lin_tm_int(void)
{
    // RUN状態 - RCVDATA_WAIT
    case( U1G_LIN_SLSTS_RCVDATA_WAIT ):
        // タイムアウト発生
        l_vol_lin_set_synchbreak();  // ← bus_tm起動

        // ★もし受信割り込みが同時に発生したら？★
        // → 受信割り込みで rcv_tm 再設定される可能性
        break;
}
```

**対策:**
- 割り込み禁止区間で状態遷移を完了
- CC23xxでは割り込みネスト管理あり（`l_u1g_lin_irq_dis()`）

### ⚠️ 危険なケース2: 状態不一致

```c
// 状態: RCVDATA_WAIT
// タイマー: rcv_tm 動作中

// ★バグで誤って bit_tm を呼び出した場合★
l_vog_lin_bit_tm_set(11);

// 結果: rcv_tm タイムアウトが無効化
```

**対策:**
- 状態管理の厳密な実装
- 各関数呼び出し箇所の検証

---

## 実際の呼び出しシーケンス検証

### シーケンス1: 正常な受信フレーム

```
1. Break待ち状態
   l_vog_lin_bus_tm_set()  // 1.302秒

2. Break検出
   l_vog_lin_bit_tm_set(39)  // 2.03ms (bus_tmを上書き ✅)

3. Synch受信 (bit_tm継続中)

4. ID受信 (bit_tm継続中)
   → 受信フレーム判定

5. 受信開始
   l_vog_lin_rcv_tm_set(90)  // 6.56ms (bit_tmを上書き ✅)

6. データ受信中 (rcv_tm継続中)

7. チェックサム受信完了
   l_vol_lin_set_synchbreak()
   l_vog_lin_bus_tm_set()  // 1.302秒 (rcv_tmを上書き ✅)
```

**結果:** ✅ **干渉なし** - 状態遷移に従って適切に上書き

### シーケンス2: 正常な送信フレーム

```
1. Break待ち状態
   l_vog_lin_bus_tm_set()  // 1.302秒

2. Break検出
   l_vog_lin_bit_tm_set(39)  // 2.03ms

3. ID受信
   → 送信フレーム判定
   l_vog_lin_frm_tm_stop()  // bit_tmを停止 ✅

4. Response Space待ち
   l_vog_lin_bit_tm_set(1)  // 52μs

5. タイマー割り込み → 1バイト送信
   l_vog_lin_bit_tm_set(11)  // 572μs (前のbit_tmを上書き ✅)

6. タイマー割り込み → 2バイト送信
   l_vog_lin_bit_tm_set(11)  // 572μs (前のbit_tmを上書き ✅)

   ... (繰り返し)

7. 送信完了
   l_vol_lin_set_synchbreak()
   l_vog_lin_bus_tm_set()  // 1.302秒
```

**結果:** ✅ **干渉なし** - 各バイト送信後に新しいタイマー設定

---

## H850との比較

### H850の実装

H850も**同一タイマーを共有**しています：

[H850/slin_lib/l_slin_drv_h83687.c](../H850/slin_lib/l_slin_drv_h83687.c)

```c
// 全て同じタイマーレジスタを使用
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = ...;  // ★同じレジスタ★
}

void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = ...;  // ★同じレジスタ★
}

void l_vog_lin_bus_tm_set(void)
{
    U2G_LIN_REG_TCNT.u2l_lin_word = U2G_LIN_WORD_CLR;
    U2G_LIN_REG_GRA.u2l_lin_word = ...;  // ★同じレジスタ★
}
```

**H850でも状態管理により干渉回避** → CC23xxと同じ設計 ✅

---

## 改善案（必要に応じて）

現在の実装で問題ありませんが、さらに安全にする場合：

### 案1: rcv_tm と bus_tm にも明示的停止を追加

**現在:**
```c
void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    // 停止処理なし
    LGPTimerLPF3_setInitialCounterTarget(...);
    LGPTimerLPF3_start(...);
}
```

**改善後:**
```c
void l_vog_lin_rcv_tm_set(l_u8 u1a_lin_bit)
{
    /* タイマー停止 */
    if (xnl_lin_timer_handle != NULL)
    {
        LGPTimerLPF3_stop(xnl_lin_timer_handle);  // ★追加★
    }

    LGPTimerLPF3_setInitialCounterTarget(...);
    LGPTimerLPF3_start(...);
}
```

**メリット:**
- 明示的な停止で意図が明確
- デバッグ時の問題切り分けが容易

**デメリット:**
- コード量増加（わずか）
- H850との完全互換性が若干低下

### 案2: タイマー割り込みハンドラで状態チェック強化

**現在:**
```c
void l_vog_lin_tm_int(void)
{
    switch( u1l_lin_slv_sts )
    {
    case( U1G_LIN_SLSTS_RCVDATA_WAIT ):
        // 受信タイムアウト処理
        break;
    }
}
```

**改善後:**
```c
void l_vog_lin_tm_int(void)
{
    switch( u1l_lin_slv_sts )
    {
    case( U1G_LIN_SLSTS_RCVDATA_WAIT ):
        // ★状態と期待タイマーの整合性チェック★
        if (/* rcv_tm が設定されているはず */)
        {
            // 受信タイムアウト処理
        }
        else
        {
            // 異常: 想定外のタイマー割り込み
            u1g_lin_syserr = U1G_LIN_SYSERR_TIMER;
        }
        break;
    }
}
```

---

## まとめ

### 干渉リスクの評価

| 項目 | 評価 | 理由 |
|-----|------|------|
| **ハードウェアリソース** | ⚠️ リスクあり | 同一タイマーハンドル共有 |
| **状態管理** | ✅ 適切 | 排他的な状態遷移 |
| **実装** | ✅ 安全 | H850互換の設計 |
| **総合評価** | ✅ **問題なし** | **状態管理により干渉回避** |

### 結論

**現在の実装で干渉リスクは実質的にありません。**

**理由:**
1. ✅ **状態マシンによる排他制御** - 各状態で1種類のタイマーのみ使用
2. ✅ **明示的な停止処理** - `l_vog_lin_bit_tm_set()`で停止
3. ✅ **ワンショットモード** - タイマー完了後に自動停止
4. ✅ **H850互換設計** - 実績のあるアーキテクチャ

### ただし注意すべき点

⚠️ **将来の変更時に注意:**
- 状態遷移ロジックの変更時
- 新しいタイマー用途の追加時
- 割り込みハンドラの変更時

⚠️ **デバッグ時:**
- 状態とタイマー設定の整合性確認
- タイマー割り込み発生タイミングの検証

---

## 検証項目

実装後、以下を確認してください：

1. ✅ **各状態でのタイマー動作**
   - BREAK_UART_WAIT: bus_tm のみ
   - BREAK_IRQ_WAIT ~ IDENTFIELD_WAIT: bit_tm のみ
   - RCVDATA_WAIT: rcv_tm のみ
   - SNDDATA_WAIT: bit_tm のみ

2. ✅ **状態遷移時のタイマー切り替え**
   - Break検出: bus_tm → bit_tm
   - 受信開始: bit_tm → rcv_tm
   - 受信完了: rcv_tm → bus_tm
   - 送信開始: bit_tm(ヘッダー) → bit_tm(送信間隔)

3. ✅ **異常系での動作**
   - タイムアウト発生時の状態遷移
   - エラー時のタイマー停止

---

## 参照

- タイマー関数実装: [l_slin_drv_cc2340r53.c:631-777](l_slin_drv_cc2340r53.c#L631-L777)
- 状態遷移処理: [l_slin_core_CC2340R53.c](l_slin_core_CC2340R53.c)
- H850参照実装: [H850/slin_lib/l_slin_drv_h83687.c:436-547](../H850/slin_lib/l_slin_drv_h83687.c#L436-L547)
- 関連ドキュメント:
  - [TIMER_FUNCTIONS_IMPLEMENTATION.md](TIMER_FUNCTIONS_IMPLEMENTATION.md)
  - [STATE_TRANSITION_ANALYSIS.md](STATE_TRANSITION_ANALYSIS.md)
