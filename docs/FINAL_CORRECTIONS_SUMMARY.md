# レジスタ割り込み実装の最終修正まとめ

## 概要

TI公式ファイルの調査により、初期実装パッチに含まれていた **重大なエラー** を発見し、修正しました。
このドキュメントでは、修正された内容と正しい実装値を整理します。

---

## 🔴 重大な修正事項

### 1. UART0ベースアドレスの修正

**❌ 誤り (初期パッチ):**
```c
u4l_lin_uart_base_addr = 0x40008000U;  // 間違ったアドレス
```

**✅ 正解 (修正版):**
```c
#include <ti/devices/cc23x0r5/inc/hw_memmap.h>

u4l_lin_uart_base_addr = UART0_BASE;  // 0x40034000
```

**影響:** 
- 誤ったアドレスでは全く異なるペリフェラルにアクセスし、UART通信が完全に動作しません
- **必ず修正が必要です**

**根拠ファイル:** 
- `/TI/ti/devices/cc23x0r5/inc/hw_memmap.h`
  ```c
  #define UART0_BASE 0x40034000U
  ```

---

### 2. クロック周波数設定の修正

**❌ 誤り (初期パッチ):**
```c
ClockP_FreqHz freq;
ClockP_getCpuFreq(&freq);
freq.lo /= 2;  // CC27XXのみ必要。CC23X0では不要！
UARTConfigSetExpClk(u4l_lin_uart_base_addr, freq.lo, U4L_LIN_BAUDRATE, ...);
```

**✅ 正解 (修正版):**
```c
ClockP_FreqHz freq;
ClockP_getCpuFreq(&freq);
// freq.lo /= 2; は削除 (CC23X0では不要)
UARTConfigSetExpClk(u4l_lin_uart_base_addr, freq.lo, U4L_LIN_BAUDRATE, ...);
```

**影響:**
- 2で割ると、実際のボーレートが設定値の半分になります
- LIN通信のタイミングが合わず、通信エラーが多発します

**根拠:**
- CC27XX系列のみクロック分周が必要
- CC23X0 (CC2340R53) では分周不要
- TIのUART2LPF3.cの実装を参照

---

### 3. 割り込み番号の確認

**✅ 正しい値:**
```c
#include <ti/devices/cc23x0r5/inc/hw_ints.h>

HwiP_construct(&xnl_lin_uart_hwi, INT_UART0_COMB, l_vog_lin_uart_isr, &hwiParams);
```

**値:** `INT_UART0_COMB = 27`

**根拠ファイル:**
- `/TI/ti/devices/cc23x0r5/inc/hw_ints.h`
  ```c
  #define INT_UART0_COMB 27
  ```

---

### 4. 電源管理マクロの確認

**✅ 正しい値:**
```c
#include <ti/drivers/power/PowerCC23X0.h>

Power_setDependency(PowerLPF3_PERIPH_UART0);
```

**根拠ファイル:**
- `/TI/ti/drivers/power/PowerCC23X0.h`
  ```c
  #define PowerLPF3_PERIPH_UART0 (PowerLPF3_PERIPH_GROUP_LRFD2 | 0x10U)
  ```

**注意:**
- CC27XX用の`PowerCC27XX.h`を使用しないこと
- CC23X0専用の`PowerCC23X0.h`を使用すること

---

## 必須インクルードファイル

修正版の実装では、以下のインクルードが **必須** です:

```c
/* TI Device Specific Headers - CC23X0R5 */
#include <ti/devices/cc23x0r5/inc/hw_memmap.h>     // UART0_BASE定義
#include <ti/devices/cc23x0r5/inc/hw_ints.h>       // INT_UART0_COMB定義
#include <ti/devices/cc23x0r5/driverlib/uart.h>    // UART DriverLib API

/* TI Driver Headers */
#include <ti/drivers/power/PowerCC23X0.h>          // PowerLPF3_PERIPH_UART0
#include <ti/drivers/GPIO.h>                       // GPIO制御
#include <ti/drivers/dpl/ClockP.h>                 // CPU周波数取得
#include <ti/drivers/dpl/HwiP.h>                   // ハードウェア割り込み
```

---

## 修正版ファイルの使用方法

### ステップ1: 修正版パッチファイルの確認

以下のファイルを使用してください:

**✅ 使用するファイル:**
- `register_interrupt_patch_CORRECTED.c` ← **修正版 (これを使用)**

**❌ 使用しないファイル:**
- `register_interrupt_patch.c` ← 初期版 (エラーあり)

### ステップ2: 適用手順

`HOW_TO_APPLY_REGISTER_INTERRUPT.md` の手順に従いますが、
**必ず `register_interrupt_patch_CORRECTED.c` を使用してください。**

1. バックアップ作成 (既に完了)
   ```bash
   cp LIN_Soft/l_slin_drv_cc2340r53.c LIN_Soft/l_slin_drv_cc2340r53.c.backup
   ```

2. `register_interrupt_patch_CORRECTED.c` の内容をコピー

3. `l_slin_drv_cc2340r53.c` の対応する関数を置き換え

4. ヘッダーファイルの修正を適用 (既に完了)

5. ビルド・テスト

---

## 修正前後の比較表

| 項目 | 初期パッチ (誤り) | 修正版 (正解) | 影響 |
|------|------------------|--------------|------|
| **UARTベースアドレス** | `0x40008000` | `0x40034000` (UART0_BASE) | ❌ 致命的 |
| **クロック分周** | `freq.lo /= 2` あり | 分周なし | ❌ ボーレート半減 |
| **割り込み番号** | `INT_UART0_COMB` | `INT_UART0_COMB (=27)` | ✅ 正しい |
| **電源マクロ** | `PowerLPF3_PERIPH_UART0` | `PowerLPF3_PERIPH_UART0` | ✅ 正しい |
| **インクルード** | 不完全 | 完全な定義 | ⚠️ コンパイルエラー回避 |

---

## テスト時の確認ポイント

修正版を適用後、以下を確認してください:

### 1. コンパイル確認
```bash
# すべてのインクルードが解決されることを確認
# エラーなくビルドが通ること
```

### 2. ベースアドレスの確認
デバッガで `u4l_lin_uart_base_addr` の値を確認:
```
期待値: 0x40034000
```

### 3. ボーレート設定の確認
UARTConfigSetExpClkに渡される周波数が正しいこと:
```c
// 48MHz動作の場合
freq.lo = 48000000  // 分周なし
```

### 4. LIN通信動作確認
- 受信割り込みが正常に発生すること
- 送信が正常に完了すること
- ボーレート設定が正しいこと (オシロスコープで確認推奨)

---

## トラブルシューティング

### 問題: UART通信が全く動作しない

**確認事項:**
1. ✅ ベースアドレスが `0x40034000` であることを確認
2. ✅ `INT_UART0_COMB` が正しくインクルードされているか確認
3. ✅ 電源依存関係が設定されているか確認
4. ✅ GPIO設定 (CONFIG_LIN_EN) が正しいか確認

### 問題: ボーレートが合わない

**確認事項:**
1. ✅ クロック分周コード (`freq.lo /= 2`) が **削除されている** こと
2. ✅ CPU周波数が正しく取得されていること
3. ✅ UARTConfigSetExpClkの引数が正しいこと

### 問題: 割り込みが発生しない

**確認事項:**
1. ✅ HwiP_constructが成功していること
2. ✅ IMSC (Interrupt Mask Set/Clear) レジスタが正しく設定されていること
3. ✅ CTL (Control) レジスタのUARTEN, RXE, TXEが有効化されていること

---

## まとめ

### ✅ 完了した作業

1. ✅ 実装プランの作成 (`REGISTER_INTERRUPT_IMPLEMENTATION.md`)
2. ✅ 既存アーキテクチャの解析 (`EXISTING_RX_FLOW_ANALYSIS.md`)
3. ✅ TI公式ファイルの調査 (`TI_UART_FILES_SUMMARY.md`)
4. ✅ **修正版パッチファイルの作成** (`register_interrupt_patch_CORRECTED.c`)
5. ✅ 適用ガイドの作成 (`HOW_TO_APPLY_REGISTER_INTERRUPT.md`)
6. ✅ ヘッダーファイルの修正 (`l_slin_drv_cc2340r53.h`)

### 📝 次のステップ (ユーザー実施)

1. **修正版パッチの適用**
   - `register_interrupt_patch_CORRECTED.c` を使用
   - `HOW_TO_APPLY_REGISTER_INTERRUPT.md` の手順に従う

2. **ビルドテスト**
   - コンパイルエラーがないことを確認

3. **ハードウェアテスト**
   - 実機でLIN通信が動作することを確認
   - ボーレートが正しいことをオシロスコープで確認

4. **性能評価**
   - レイテンシ測定
   - エラーハンドリングのテスト

---

## 重要な注意事項

⚠️ **必ず修正版 (`register_interrupt_patch_CORRECTED.c`) を使用してください**

初期パッチには致命的なエラーが含まれており、そのまま使用すると:
- UART通信が全く動作しない
- ボーレートが半分になり通信エラーが多発する

修正版では、TI公式ファイルの調査に基づき、すべての値が正しく設定されています。

---

**作成日:** 2025-10-26  
**対象デバイス:** Texas Instruments CC2340R53 (CC23X0系列)  
**関連ドキュメント:**
- `REGISTER_INTERRUPT_IMPLEMENTATION.md` - 実装プラン
- `EXISTING_RX_FLOW_ANALYSIS.md` - 既存アーキテクチャ解析
- `TI_UART_FILES_SUMMARY.md` - TIファイル調査結果
- `HOW_TO_APPLY_REGISTER_INTERRUPT.md` - 適用手順
- `register_interrupt_patch_CORRECTED.c` - 修正版実装
