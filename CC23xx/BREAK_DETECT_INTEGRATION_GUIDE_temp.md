# LIN 9.5bit Break検出モジュール 統合ガイド (プレビュー版)

## 概要

本ドキュメントは、CC2340R53向けLIN通信スタックに9.5ビットBreak検出機能を統合する手順を説明します。

**注意**: これはプレビュー版の実装です。本番環境への適用前に十分なテストを実施してください。

---

## ファイル構成

- `l_slin_break_detect_temp.h` - Break検出モジュールヘッダ
- `l_slin_break_detect_temp.c` - Break検出モジュール実装
- `BREAK_DETECT_INTEGRATION_GUIDE_temp.md` - 本ドキュメント

---

## 動作原理

### 1. Break検出シーケンス

```
LINバス信号:  ─┐                                        ┌──
(DIO23)        │◄────── Break (Low) ──────────────────►│
               └────────────────────────────────────────┘
               ↑                          ↑             ↑
               │                          │             │
          立下りエッジ                タイマー満了    立上りエッジ
          (Break開始)              (9.5bit経過)     (Delimiter)
               │                          │             │
               ▼                          ▼             ▼
           ┌────────┐              ┌────────┐     ┌──────────┐
状態:      │  WAIT  │──TIMING─────►│RUNNING │────►│DETECTED  │
           └────────┘              └────────┘     └──────────┘
                                      490μs              │
                                                         ▼
                                                  コールバック
                                                  UART受信開始
```

### 2. 主要な特徴

- **9.5ビット精度**: 490μs（9.5bit - 5μsマージン）で検出
- **ノイズ除外**: 9.5ビット未満のパルスは無視
- **エッジ切り替え**: 立下り→立上りを動的に切り替え
- **既存フロー統合**: 既存のUART受信フローに自然に統合

---

## 既存コードとの統合

### 統合ポイント1: 初期化処理

**既存**: `l_vog_lin_int_init()` (l_slin_drv_cc2340r53.c:222行目)

**統合方法**:

```c
void l_vog_lin_int_init(void)
{
    l_s16    s2a_lin_ret;

    l_u1g_lin_irq_dis();
    GPIO_init();

    /* ★★★ Break検出モジュール初期化を追加 ★★★ */
    l_vog_break_detect_init();

    /* 既存のGPIO設定は不要（Break検出モジュール内で実施） */

    l_vog_lin_irq_res();
}
```

### 統合ポイント2: Break待ち状態への遷移

**既存**: `l_vol_lin_set_synchbreak()` (l_slin_core_CC2340R53.c内)

**統合方法**:

```c
static void l_vol_lin_set_synchbreak(void)
{
    l_vog_lin_frm_tm_stop();                    /* タイマ停止 */

    /* ★★★ Break検出開始を追加 ★★★ */
    l_vog_break_detect_start();

    /* 旧GPIO設定は不要（Break検出モジュールが実施） */
    // l_vog_lin_int_enb();  ← この行を削除またはコメントアウト

    l_vog_lin_bus_tm_set();                     /* Physical Busエラー検出タイマ設定 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_UART_WAIT;  /* Break待ち状態 */
}
```

### 統合ポイント3: UART受信割り込み (既存処理の削除)

**既存**: `l_vog_lin_rx_int()` のBreak検出部分 (l_slin_core_CC2340R53.c:688-721行目)

**変更**:

```c
void l_vog_lin_rx_int(l_u8 u1a_lin_data, l_u8 u1a_lin_err)
{
    /*** RUN STANDBY状態の場合 ***/
    if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN_STANDBY )
    {
        /* ★★★ この部分のBreak検出処理を削除 ★★★ */
        /*
         * GPIO Break検出モジュールが処理するため、
         * UART Framing ErrorによるBreak検出は使用しない
         */

        /* Synch Break(UART)待ち状態 */
        // if( u1l_lin_slv_sts == U1G_LIN_SLSTS_BREAK_UART_WAIT )
        // {
        //     /* この部分を削除またはコメントアウト */
        // }
    }
    /*** RUN状態の場合 ***/
    else if( xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts == U2G_LIN_STS_RUN )
    {
        /* Synch Field待ち以降の処理はそのまま残す */
        switch( u1l_lin_slv_sts )
        {
        case( U1G_LIN_SLSTS_SYNCHFIELD_WAIT ):
            /* 既存処理をそのまま使用 */
            ...
```

---

## 設定ファイル (SysConfig)

### 必要なリソース

#### 1. LGPTimer (Break検出用)

```
Name: CONFIG_LGPTIMER_0
Mode: One-shot
Interrupt Priority: 2 (高優先度)
Target Interrupt: Enabled
```

#### 2. GPIO (既存のLIN RX設定を流用)

```
Name: U1G_LIN_INTPIN (既存定義を使用)
Mode: Input
Interrupt: Edge (動的切り替え)
Callback: l_vog_break_detect_gpio_isr
```

---

## テスト手順

### 1. 単体テスト

#### Test 1: 正常Break検出

```
入力: 13ビット(677μs)のLowパルス
期待結果:
  - Break検出
  - コールバック呼び出し
  - UART受信モードへ遷移
```

#### Test 2: 最小Break検出

```
入力: 9.5ビット(495μs)のLowパルス
期待結果:
  - Break検出
  - コールバック呼び出し
```

#### Test 3: 閾値境界(下限)

```
入力: 9.4ビット(490μs未満)のLowパルス
期待結果:
  - Break非検出
  - 待機状態継続
```

#### Test 4: ノイズ除外

```
入力: 1μsのLowパルス
期待結果:
  - Break非検出
  - 待機状態継続
```

### 2. 統合テスト

#### Test 5: LINフレーム受信

```
入力: Break→Sync(0x55)→PID→Data
期待結果:
  - 正常受信完了
  - データ正常取得
```

#### Test 6: 連続Break

```
入力: 2回連続のBreak信号
期待結果:
  - 両方とも正しく検出
  - エラーなく動作継続
```

---

## トラブルシューティング

### 問題1: Break検出しない

**原因候補**:
- GPIO設定が正しくない
- タイマーが初期化されていない
- U1G_LIN_INTPINの定義が間違っている

**確認方法**:
```c
// デバッグコードを追加
l_u8 state = l_u1g_break_detect_get_state();
// stateを確認: WAIT→TIMING→DETECTEDと遷移すること
```

### 問題2: 誤検出が多い

**原因候補**:
- ノイズの影響
- 閾値が短すぎる

**対策**:
```c
// l_slin_break_detect_temp.h で閾値を調整
#define U4G_LIN_BREAK_MARGIN_US     (10U)  // マージンを増やす
```

### 問題3: UART受信に切り替わらない

**原因候補**:
- コールバック内の処理が正しく動作していない
- UART受信が有効化されていない

**確認方法**:
```c
// l_vsl_break_detected_callback() にデバッグ出力を追加
```

---

## 性能情報

| 項目 | 値 |
|------|-----|
| Break検出精度 | ±1μs |
| CPU負荷 | 低 (割り込み駆動) |
| メモリ使用量 | 約20バイト (制御構造体) |
| タイマーリソース | LGPTimer 1個 |
| GPIO割り込み | 1個 |

---

## 既知の制限事項

1. **タイマーリソース**: CONFIG_LGPTIMER_0を専有使用
2. **GPIO共有**: U1G_LIN_INTPINはBreak検出とWakeup検出で共用
3. **リアルタイム性**: 割り込み優先度2以上を推奨

---

## 今後の改善案

1. **自動較正**: ボーレート自動検出による閾値動的調整
2. **統計情報**: Break検出回数、誤検出回数の記録
3. **省電力**: Standbyモード最適化

---

## 変更履歴

| 版数 | 日付 | 変更内容 |
|------|------|----------|
| 1.0 | 2025/XX/XX | プレビュー版作成 |

---

## 問い合わせ

本実装に関する質問や問題報告は、プロジェクト管理者まで。