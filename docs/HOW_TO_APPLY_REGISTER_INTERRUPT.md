# レジスタ割り込み実装の適用手順

## 概要

このドキュメントでは、DMA割り込みから直接レジスタ割り込みへの移行手順を説明します。

## 作成したファイル

1. **REGISTER_INTERRUPT_IMPLEMENTATION.md** - 詳細な実装説明
2. **register_interrupt_patch.c** - 実装すべき関数のパッチファイル
3. **l_slin_drv_cc2340r53.c.backup** - 元ファイルのバックアップ

## 適用手順

### ステップ1: インクルードファイルの修正

**ファイル**: `LIN_Soft/l_slin_drv_cc2340r53.c`

以下の行を削除:
```c
#include <ti/drivers/UART2.h>
#include <ti/drivers/uart2/UART2LPF3.h>
```

以下を追加:
```c
#include <ti/drivers/dpl/HwiP.h>
#include <ti\devices\cc23x0r5\inc\hw_memmap.h>
#include <ti\devices\cc23x0r5\inc\hw_ints.h>
#include <ti/drivers/uart2/UART2Support.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/uart.h)
```

**ステータス**: ✅ 完了

### ステップ2: プロトタイプ宣言の修正

**ファイル**: `LIN_Soft/l_slin_drv_cc2340r53.c`

以下の行を削除:
```c
void  l_ifc_rx_ch1(UART2_Handle handle, void *u1a_lin_rx_data, size_t count, void *userArg, int_fast16_t u1a_lin_rx_status);
void  l_ifc_tx_ch1(UART2_Handle handle, void *u1a_lin_tx_data, size_t count, void *userArg, int_fast16_t u1a_lin_tx_status);
```

**ステータス**: ✅ 完了（プロトタイプ削除済み）

### ステップ3: 変数定義の修正

**ファイル**: `LIN_Soft/l_slin_drv_cc2340r53.c`

**行番号74付近**で以下を削除:
```c
static UART2_Handle         xnl_lin_uart_handle;        /**< @brief UART2ãƒãƒ³ãƒ‰ãƒ« */
```

以下を追加:
```c
static HwiP_Struct      xnl_lin_uart_hwi;               /**< @brief UART割り込みオブジェクト */
static l_u32            u4l_lin_uart_base_addr;         /**< @brief UARTベースアドレス */

/* レジスタ割り込み用の状態管理変数 */
static volatile l_u8    u1l_lin_rx_index = 0;          /**< @brief 受信バッファインデックス */
static volatile l_u8    u1l_lin_rx_expected_size = 0;  /**< @brief 期待する受信サイズ */
static volatile l_u8    u1l_lin_tx_index = 0;          /**< @brief 送信バッファインデックス */
static volatile l_u8    u1l_lin_tx_size = 0;           /**< @brief 送信データサイズ */
static volatile l_bool  b1l_lin_uart_initialized = FALSE; /**< @brief UART初期化済みフラグ */
```

**手順**:
```bash
# エディタで行74を検索して置き換え
vim +74 LIN_Soft/l_slin_drv_cc2340r53.c
```

または、sedコマンドで置換:
```bash
cd /home/yaruha/project-dev/else-dev/LIN_Workspace/LIN_Soft
sed -i '74s/.*/static HwiP_Struct      xnl_lin_uart_hwi;/' l_slin_drv_cc2340r53.c
```

### ステップ4: 関数実装の置き換え

`register_interrupt_patch.c`ファイルから以下の関数をコピーして、
`l_slin_drv_cc2340r53.c`の対応する関数と置き換えます:

#### 4-1. l_u4g_lin_uart_get_base_addr() を追加

**場所**: 関数定義の最初（l_vog_lin_uart_init()の前）

**内容**: `register_interrupt_patch.c`のセクション2をコピー

#### 4-2. l_vog_lin_uart_init() を置き換え

**元の関数**: 約30行（UART2_Params使用）
**新しい関数**: `register_interrupt_patch.c`のセクション3

**検索**: `void l_vog_lin_uart_init(void)`
**置換**: セクション3の全内容で置き換え

#### 4-3. l_vog_lin_uart_isr() を追加

**場所**: l_vog_lin_uart_init()の後

**内容**: `register_interrupt_patch.c`のセクション4をコピー

#### 4-4. l_vog_lin_rx_enb() を置き換え

**元の関数**: UART2_read()を使用
**新しい関数**: `register_interrupt_patch.c`のセクション5

**検索**: `void  l_vog_lin_rx_enb(`
**置換**: セクション5の全内容で置き換え

#### 4-5. l_vog_lin_rx_dis() を置き換え

**元の関数**: コメントのみ
**新しい関数**: `register_interrupt_patch.c`のセクション6

**検索**: `void  l_vog_lin_rx_dis(void)`
**置換**: セクション6の全内容で置き換え

#### 4-6. l_vog_lin_tx_char() を置き換え

**元の関数**: UART2_write()を使用
**新しい関数**: `register_interrupt_patch.c`のセクション7

**検索**: `void  l_vog_lin_tx_char(`
**置換**: セクション7の全内容で置き換え

#### 4-7. l_u1g_lin_read_back() を置き換え

**元の関数**: UART2ハンドルからレジスタアクセス
**新しい関数**: `register_interrupt_patch.c`のセクション8

**検索**: `l_u8  l_u1g_lin_read_back(`
**置換**: セクション8の全内容で置き換え

#### 4-8. l_ifc_uart_close() を置き換え

**元の関数**: UART2_close()を使用
**新しい関数**: `register_interrupt_patch.c`のセクション9

**検索**: `void l_ifc_uart_close(void)`
**置換**: セクション9の全内容で置き換え

#### 4-9. 不要な関数を削除

以下の関数を削除またはコメントアウト:
- `l_ifc_rx_ch1()` - UART2受信コールバック
- `l_ifc_tx_ch1()` - UART2送信コールバック

**検索**: `void  l_ifc_rx_ch1(`と`void  l_ifc_tx_ch1(`
**操作**: 関数全体をコメントアウトまたは削除

### ステップ5: ビルドとテスト

```bash
# プロジェクトをビルド
cd /home/yaruha/project-dev/else-dev/LIN_Workspace
make clean
make

# ビルドエラーがある場合は修正

# 書き込み・実行
make flash
```

### ステップ6: 動作確認

1. **基本送受信テスト**
   - LINヘッダー送受信
   - LINレスポンス送受信

2. **リードバックテスト**
   - 送信データと受信データの一致確認

3. **エラーハンドリングテスト**
   - フレーミングエラー検出
   - オーバーランエラー検出

## 重要な注意事項

### 1. UARTベースアドレスの設定

`l_vog_lin_uart_init()`内で、CONFIG_UART_INDEXに応じて正しいベースアドレスを設定してください:

```c
/* UART0の場合 */
u4l_lin_uart_base_addr = 0x40008000U;

/* UART1の場合（存在する場合） */
// u4l_lin_uart_base_addr = 0x40009000U;
```

また、対応する電源依存とinterrupt番号も変更:
```c
Power_setDependency(PowerLPF3_PERIPH_UART0);  // または UART1
HwiP_construct(&xnl_lin_uart_hwi, INT_UART0_COMB, ...);  // または INT_UART1_COMB
```

### 2. 割り込み優先度の調整

LIN通信のタイミング要件に応じて、割り込み優先度を調整してください:

```c
hwiParams.priority = 0x20;  /* 高優先度が必要な場合は値を下げる（0が最高優先度） */
```

### 3. エラー検出の有効化

本番環境では、リードバック関数内のエラー検出を有効化してください:

```c
/* 以下の行を削除またはコメントアウト */
// u4a_lin_error_status  = U4G_DAT_ZERO; /* 一旦エラー検出機能オフ */
```

## トラブルシューティング

### ビルドエラー

**エラー**: `UART2_Handle` が未定義
**解決**: ステップ1のインクルード修正を確認

**エラー**: `HwiP_Struct` が未定義
**解決**: `#include <ti/drivers/dpl/HwiP.h>` を追加

**エラー**: `UARTConfigSetExpClk` が未定義
**解決**: driverlibのuart.hインクルードを確認

### 実行時エラー

**問題**: UART初期化失敗
**確認項目**:
- UARTベースアドレスが正しいか
- CONFIG_UART_INDEXに対応する電源依存が設定されているか

**問題**: データ受信できない
**確認項目**:
- 受信割り込みが有効化されているか (`UARTEnableInt`)
- RXE（受信有効化）ビットが設定されているか

**問題**: データ送信できない
**確認項目**:
- TXE（送信有効化）ビットが設定されているか
- FIFOに空きがあるか

## ロールバック手順

問題が発生した場合、バックアップから復元できます:

```bash
cd /home/yaruha/project-dev/else-dev/LIN_Workspace/LIN_Soft
cp l_slin_drv_cc2340r53.c.backup l_slin_drv_cc2340r53.c
```

ヘッダファイルも元に戻す必要がある場合:
```bash
git checkout l_slin_drv_cc2340r53.h
```

## まとめ

この実装により、以下が実現されます:

1. ✅ DMA不要（リソース節約）
2. ✅ シンプルな実装（デバッグ容易）
3. ✅ LIN通信に適した低レイテンシ
4. ✅ エラー検出機能の強化

実装が完了したら、実際のLIN通信環境でテストしてください。
