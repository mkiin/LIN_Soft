# LIN_Soft UART移植計画書

## 目的

LIN_SoftのUART実装をTI UART2ドライバから新規作成のUART_RegIntドライバに移植する。

## 現状分析

### 影響を受けるファイル

1. **l_slin_drv_cc2340r53.c** - メイン変更対象
   - UART2ハンドル、パラメータ、API呼び出し
   - コールバック関数定義

2. **l_slin_api.h** - プロトタイプ宣言
   - UART2_Handle型を使用したコールバック宣言

### UART2からUART_RegIntへのAPI対応表

| UART2 API | UART_RegInt API | 備考 |
|-----------|-----------------|------|
| `UART2_Handle` | `UART_RegInt_Handle` | ハンドル型 |
| `UART2_Params` | `UART_RegInt_Params` | パラメータ構造体 |
| `UART2_Params_init()` | `UART_RegInt_Params_init()` | パラメータ初期化 |
| `UART2_open()` | `UART_RegInt_open()` | オープン |
| `UART2_close()` | `UART_RegInt_close()` | クローズ |
| `UART2_read()` | `UART_RegInt_read()` | 読み込み |
| `UART2_write()` | `UART_RegInt_write()` | 書き込み |
| `UART2_rxEnable()` | `UART_RegInt_rxEnable()` | RX有効化 |
| `UART2_flushRx()` | `UART_RegInt_flushRx()` | RXバッファフラッシュ |

### モード対応

| UART2定数 | UART_RegInt定数 | 値 |
|-----------|-----------------|-----|
| `UART2_Mode_CALLBACK` | `UART_REGINT_MODE_CALLBACK` | コールバックモード |
| `UART2_ReadReturnMode_FULL` | `UART_REGINT_RETURN_FULL` | 全データ受信 |
| `UART2_DataLen_8` | `UART_REGINT_LEN_8` | 8ビットデータ |
| `UART2_StopBits_1` | `UART_REGINT_STOP_ONE` | 1ストップビット |
| `UART2_Parity_NONE` | `UART_REGINT_PAR_NONE` | パリティなし |
| `UART2_STATUS_SUCCESS` | `UART_REGINT_STATUS_SUCCESS` | 成功 |

### コールバック関数シグネチャの変更

**UART2:**
```c
void callback(UART2_Handle handle, void *buf, size_t count, 
              void *userArg, int_fast16_t status);
```

**UART_RegInt:**
```c
void callback(UART_RegInt_Handle handle, void *buf, size_t count,
              void *userArg, int_fast16_t status);
```

## 移植手順

### フェーズ1: l_slin_api.hの更新

1. UART2インクルードをUART_RegIntに変更
2. コールバック関数プロトタイプを更新

### フェーズ2: l_slin_drv_cc2340r53.cの更新

1. インクルード変更
   - `<ti/drivers/UART2.h>` → `"../UART_DRV/UART_RegInt.h"`
   - `<ti/drivers/uart2/UART2LPF3.h>` 削除

2. 静的変数の型変更
   - `UART2_Handle` → `UART_RegInt_Handle`

3. `l_vog_lin_uart_init()`関数
   - `UART2_Params` → `UART_RegInt_Params`
   - `UART2_Params_init()` → `UART_RegInt_Params_init()`
   - `UART2_open()` → `UART_RegInt_open()`
   - パラメータ設定の更新

4. `l_vog_lin_rx_enb()`関数
   - `UART2_flushRx()` → `UART_RegInt_flushRx()`
   - `UART2_rxEnable()` → `UART_RegInt_rxEnable()`
   - `UART2_read()` → `UART_RegInt_read()`
   - ステータスチェック更新

5. `l_vog_lin_tx_char()`関数
   - `UART2_write()` → `UART_RegInt_write()`

6. `l_u1g_lin_read_back()`関数
   - レジスタアクセス方法の変更
   - UART_RegIntのハードウェア属性アクセス

7. `l_ifc_uart_close()`関数
   - `UART2_close()` → `UART_RegInt_close()`

8. コールバック関数
   - `l_ifc_rx_ch1()`: `UART2_Handle` → `UART_RegInt_Handle`
   - `l_ifc_tx_ch1()`: `UART2_Handle` → `UART_RegInt_Handle`

### フェーズ3: 動作検証

1. ビルド確認
2. リンクエラー確認
3. 型不整合の修正

## 互換性の考慮事項

### コールバックの互換性

UART_RegIntドライバはUART2と同じコールバックシグネチャを使用するため、
LIN_Softのコールバック処理は変更不要。

### エラーハンドリング

UART2とUART_RegIntは同じエラーコードを使用：
- `UART_REGINT_STATUS_SUCCESS` (0)
- `UART_REGINT_STATUS_EOVERRUN` (-8)
- その他のエラーコード

### DMA vs 割り込み

UART2はDMAを使用するが、UART_RegIntはレジスタ割り込みを使用。
LIN通信のタイミング要件を満たすことを確認する必要がある。

## リスクと対策

| リスク | 影響 | 対策 |
|--------|------|------|
| タイミング変更 | LIN通信エラー | 実機テストで検証 |
| レジスタアクセス変更 | リードバック失敗 | ハードウェア属性の正しいアクセス |
| コールバック遅延 | フレーム損失 | 割り込み優先度の確認 |

## 成功基準

1. ビルドエラーなし
2. LINフレーム送信成功
3. LINフレーム受信成功
4. リードバック検証動作
5. Sleep/Wakeup動作正常

## 次のステップ

1. l_slin_api.hの更新
2. l_slin_drv_cc2340r53.cの更新
3. ビルドとエラー修正
4. 動作確認テスト計画の作成
