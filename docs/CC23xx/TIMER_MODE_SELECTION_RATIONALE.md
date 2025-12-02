# LGPTimerのワンショットモード選択理由

## 質問
なぜタイマーを`LGPTimerLPF3_CTL_MODE_UP_ONCE`（ワンショットモード）で実行するのか？

## 回答

### 利用可能なタイマーモード

LGPTimerLPF3には以下の4つのモードがあります：

| モード | 定義 | 動作 |
|--------|------|------|
| `LGPTimerLPF3_CTL_MODE_DIS` | タイマー無効 | カウンタとイベントの更新が停止 |
| `LGPTimerLPF3_CTL_MODE_UP_ONCE` | **ワンショット** | 0からターゲット値までカウント後、**自動停止**してDISに移行 |
| `LGPTimerLPF3_CTL_MODE_UP_PER` | 周期アップカウント | 0からターゲット値まで**繰り返し**カウント |
| `LGPTimerLPF3_CTL_MODE_UPDWN_PER` | 周期アップダウンカウント | 0→ターゲット値→0を**繰り返し**カウント |

---

## ワンショットモードを選択した理由

### 1. LIN通信のタイマー使用パターンの特性

LIN通信では、**各イベントごとに異なるタイミングでタイマーを起動**する必要があります：

#### タイマー起動パターンの実例

```c
// パターン1: ヘッダータイムアウト検出 (Break受信後)
l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
// → 一度だけタイムアウト検出したい

// パターン2: レスポンススペース待ち (ID受信後、送信開始前)
l_vog_lin_bit_tm_set( U1G_LIN_RSSP );  // 1bit = 52.08us
// → 一度だけ待機したい

// パターン3: Inter-byte Space待ち (各バイト送信間隔)
l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );  // 11bit = 572.9us
// → 各バイト送信ごとに異なるタイミングで起動
```

#### コード実装箇所
[l_slin_core_CC2340R53.c:1221](l_slin_core_CC2340R53.c#L1221)
```c
case( U1G_LIN_SLSTS_SNDDATA_WAIT ):
    // 送信データのReadback確認
    l_u1a_lin_read_back = l_u1g_lin_rx_char();

    // Readback一致確認後
    if( l_u1a_lin_read_back == u1l_lin_rs_tmp[ u1l_lin_rs_cnt - U1G_LIN_1 ] )
    {
        // 全バイト送信完了チェック
        if( u1l_lin_rs_cnt > u1l_lin_frm_sz )
        {
            l_vol_lin_set_frm_complete( U1G_LIN_ERR_OFF );
            l_vol_lin_set_synchbreak();
        }
        // まだ送信するバイトがある
        else
        {
            l_vog_lin_tx_char( u1l_lin_rs_tmp[ u1l_lin_rs_cnt ] );

            // ★ここで毎回新しいタイマーを起動★
            l_vog_lin_bit_tm_set( U1G_LIN_BYTE_LENGTH + U1G_LIN_BTSP );

            u1l_lin_rs_cnt++;
        }
    }
```

**ポイント:** データ送信中、**バイトごとに新しいタイマーを起動**しています。

---

### 2. 各モードとの比較

#### ❌ `LGPTimerLPF3_CTL_MODE_UP_PER` (周期モード) が不適切な理由

**問題点:**
- **固定周期で繰り返し割り込みが発生**してしまう
- LIN通信では**イベントドリブンで異なる間隔**のタイマーが必要
- **送信完了後も割り込みが発生**し続け、不要な処理が実行される

**具体例:**
```c
// 仮に周期モードを使った場合
l_vog_lin_bit_tm_set(11);  // 11bit周期で設定

// 問題1: データ送信が8バイトで終わっても、タイマーは動き続ける
// 問題2: 次のフレームまで割り込みが発生し続ける
// 問題3: 無駄なCPU処理とエラー処理が必要になる
```

**対処が必要な処理:**
- フレーム完了後に明示的にタイマー停止が必須
- 不要な割り込み発生によるCPU負荷
- 状態管理が複雑化（タイマー割り込みを無視する処理が必要）

---

#### ❌ `LGPTimerLPF3_CTL_MODE_UPDWN_PER` (アップダウン周期) が不適切な理由

**問題点:**
- UP_PERと同じく**周期動作**のため同様の問題が発生
- さらに、カウント方向が変わるため**タイミング計算が複雑**になる
- LIN通信では**単調増加のカウント**で十分

---

### 3. ✅ ワンショットモードの利点

#### 利点1: **イベントドリブンに完全一致**

```c
void l_vog_lin_bit_tm_set(l_u8 u1a_lin_bit)
{
    // ターゲット値計算
    u4a_lin_timeout_us = ((l_u32)u1a_lin_bit * U4L_LIN_1BIT_TIMERVAL) / 1000UL;
    u4a_lin_counter_target = u4a_lin_timeout_us * 48UL - 1UL;

    // ターゲット設定
    LGPTimerLPF3_setInitialCounterTarget(xnl_lin_timer_handle, u4a_lin_counter_target, true);

    // ★ワンショットモードで起動★
    LGPTimerLPF3_start(xnl_lin_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}
```

**メリット:**
- 必要な時だけタイマー起動
- **設定値に到達したら自動停止** → 手動停止不要
- 次のイベントまで割り込み発生なし

#### 利点2: **明示的な停止処理が不要**

**ワンショットモードの場合:**
```c
// タイマー起動
l_vog_lin_bit_tm_set(11);

// → 11bit時間後に1回だけ割り込み発生
// → 自動的にDISABLEになる
// → 次のl_vog_lin_bit_tm_set()まで何もしない
```

**周期モードの場合（仮に使った場合）:**
```c
// タイマー起動
LGPTimerLPF3_start(handle, LGPTimerLPF3_CTL_MODE_UP_PER);

// → 11bit周期で割り込みが発生し続ける
// → フレーム完了時に明示的な停止が必須
l_vog_lin_frm_tm_stop();  // ← 停止し忘れると無限割り込み

// → 状態管理が複雑化
if (u1l_lin_slv_sts != U1G_LIN_SLSTS_SNDDATA_WAIT) {
    // タイマー割り込みを無視する処理が必要
    return;
}
```

#### 利点3: **異なる間隔のタイマーを簡単に実現**

LIN通信では以下のような**様々な間隔**が必要です：

| タイミング | ビット数 | 時間 (@19200bps) | 使用箇所 |
|-----------|---------|-----------------|---------|
| Response Space | 1 bit | 52.08 μs | ID受信後、送信開始前 |
| Inter-byte Space | 11 bit | 572.9 μs | 各バイト送信間隔 |
| Header Timeout | 48 bit | 2.5 ms | Break後のヘッダー受信タイムアウト |

**ワンショットモードなら:**
```c
// Response Space
l_vog_lin_bit_tm_set(1);   // 52.08μs後に割り込み → 自動停止

// Inter-byte Space
l_vog_lin_bit_tm_set(11);  // 572.9μs後に割り込み → 自動停止

// Header Timeout
l_vog_lin_bit_tm_set(48);  // 2.5ms後に割り込み → 自動停止
```

**周期モードだと:**
```c
// 各イベントで周期を変更する必要がある
LGPTimerLPF3_stop(handle);
LGPTimerLPF3_setInitialCounterTarget(handle, new_target, true);
LGPTimerLPF3_start(handle, LGPTimerLPF3_CTL_MODE_UP_PER);
// → さらにフレーム完了時に停止が必須
// → 実装が複雑化
```

---

### 4. H850互換性の観点

H850の実装でも同様の**ワンショットタイマー動作**を想定しています：

[H850/slin_lib/l_slin_core_h83687.c](../H850/slin_lib/l_slin_core_h83687.c)
```c
// H850でもタイマーは各イベントごとに起動
l_vog_lin_bit_tm_set( U1G_LIN_RSSP );  // Response Space

// タイマー割り込み後は自動停止（またはソフトウェア停止）
// → 次のイベントまでタイマー割り込みなし
```

---

## まとめ

### ワンショットモードを選択した理由

| 理由 | 説明 |
|-----|------|
| **1. イベントドリブン性** | LIN通信は各イベントで異なるタイミングが必要 → ワンショットが最適 |
| **2. 自動停止** | ターゲット到達後に自動停止 → 明示的な停止処理不要 |
| **3. 実装の簡潔性** | 周期モードのような不要な割り込み処理・停止処理が不要 |
| **4. CPU効率** | 必要な時だけ割り込み発生 → 無駄なCPU処理なし |
| **5. H850互換性** | H850の動作パターンと一致 |

### 周期モードが適切な場合（参考）

周期モードが適している例：
- **PWM信号生成** → 固定周期で出力を繰り返す
- **定期的なセンサーポーリング** → 一定間隔でADC読み取り
- **リアルタイムクロック** → 1ms/10ms周期のタイマー割り込み

**LIN通信は該当しない** → イベントドリブンでタイミングが不定期

---

## 実装箇所の参照

- タイマー設定: [l_slin_drv_cc2340r53.c:631-670](l_slin_drv_cc2340r53.c#L631-L670)
- タイマー割り込みハンドラ: [l_slin_core_CC2340R53.c:1126-1254](l_slin_core_CC2340R53.c#L1126-L1254)
- 送信時のタイマー使用: [l_slin_core_CC2340R53.c:1221](l_slin_core_CC2340R53.c#L1221)
