# CLAUDE.md
UART0のレジスタガイド:
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/docs/driverlib/driverlib/register_descriptions/CPU_MMAP/UART0.html

各種ドライバー
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/drivers

UARTドライバー
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/drivers/UART2.c
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/drivers/uart2/UART2LPF3.c
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/drivers/uart2/UART2LPF3.h
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/drivers/uart2/UART2Support.h

HwP系
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/drivers/dpl/HwiP.h

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

HW固有定義
UART：/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/devices/cc23x0r5/inc/hw_uart.h
固有マクロ・型定義
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/devices/cc23x0r5/inc/hw_types.h

ドライバーライブラリ
UART : /home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/devices/cc23x0r5/driverlib/uart.c
/home/yaruha/project-dev/else-dev/LIN_Workspace/TI/ti/devices/cc23x0r5/driverlib/uart.c
## プロジェクト概要

このリポジトリは、Texas Instruments CC2340R53マイコン向けのLIN通信スタックの実装です。主に2つのコンポーネントで構成されています：

1. **LIN_Soft**: LIN通信プロトコルスタック（SLIN API）
2. **TI_UART_Driver**: Texas Instruments UART2ドライバ

## アーキテクチャ

### LIN通信スタックの階層構造

LIN通信スタックは以下の階層で実装されています（下層から上層へ）：

```
アプリケーション層
    ↓
LIN_Manager (LIN_Manager.c/h) - アプリケーション向けインターフェース
    ↓
SLIN API層 (l_slin_api.h) - LIN API関数群
    ↓
SLIN CORE層 (l_slin_core_cc2340r53.c/h) - LINプロトコル処理
    ↓
SLIN DRV層 (l_slin_drv_cc2340r53.c/h) - ハードウェア制御
    ↓
UART2ドライバ (TI_UART_Driver/) - TI提供のUARTドライバ
```

### LIN_Managerの役割

`LIN_Manager`は、LINフレームの分割・結合を管理するミドルウェア層です：

- **送信側**: 長いペイロード（最大307バイト）を7バイトずつのLINフレームに分割
  - 単発フレーム（SINGLE）: 1フレームで完結
  - 継続フレーム: SOF（Start of Frame）→ CONTINUATION → EOF（End of Frame）

- **受信側**: 分割されたLINフレームを結合して元のデータに復元

- **キュー管理**: 送信データを最大10フレームまでキューイング

### フレームフォーマット

LINフレームの構造：
- **D1（Data1）**: フレーム制御情報
  - bit 0-2: フレーム継続情報（SINGLE/SOF/CONTINUATION/EOF）
  - bit 3-4: 機能別識別情報
  - bit 5: Data.ind（データ指示）
  - bit 6-7: Sleep.ind/Wake up.ind
- **D2-D8**: ペイロードデータ（最大7バイト）

### SLIN APIの構成

#### 主要なAPI関数グループ

1. **初期化・接続制御**
   - `l_sys_init()`: システム初期化
   - `l_ifc_init_ch1()`: インターフェース初期化
   - `l_ifc_connect_ch1()`: LIN通信接続
   - `l_ifc_disconnect_ch1()`: LIN通信切断

2. **データ送受信**
   - `l_slot_rd_ch1()`: フレームデータ読み込み
   - `l_slot_wr_ch1()`: フレームデータ書き込み
   - `l_slot_enable_ch1()`: フレーム有効化
   - `l_slot_disable_ch1()`: フレーム無効化

3. **ネットワーク管理**
   - `l_nm_init_ch1()`: ネットワーク管理初期化
   - `l_nm_tick_ch1()`: NMティック処理（スリープ要求対応）

4. **状態管理**
   - `l_ifc_read_status_ch1()`: インターフェース状態読み取り
   - 同期フラグによるフレーム送受信完了の監視

### LINフレームID定義

主要なフレームID（`l_slin_api.h`に定義）：
- `0x20` (ID20H): 受信フレーム（アプリケーション←外部）
- `0x30` (ID30H): 送信フレーム（アプリケーション→外部）
- `0x33` (ID33H): BLEアンテナテスト要求
- `0x34` (ID34H): BLEアンテナテスト応答

### 割り込み処理

SLIN DRV層は以下の割り込みハンドラを提供：
- `l_vog_lin_rx_int()`: UART受信割り込み
- `l_vog_lin_tm_int()`: タイマ割り込み（タイムアウト検出）
- `l_vog_lin_irq_int()`: 外部割り込み（Wakeup検出用）

割り込み禁止/許可はマクロで実装：
- `l_u1g_lin_irq_dis()`: ネスト対応の割り込み禁止
- `l_vog_lin_irq_res()`: ネスト対応の割り込み許可

## コーディング規約

### 命名規則

変数・関数名のプレフィックス：
- **スコープ**: `g` (global), `l` (local), `a` (argument)
- **型**: `u1` (uint8_t), `u2` (uint16_t), `u4` (uint32_t), `b1` (bool), `f` (function), `vo` (void)
- **定数**: `U1G_`, `U2G_`, `U4G_` (グローバル定数), `U1L_`, `U2L_` (ローカル定数)

例：
- `u1g_lin_byte`: グローバルuint8_t変数
- `u1a_lin_data`: 引数のuint8_t変数
- `f_LIN_Manager_Initialize`: 初期化関数
- `U1L_LIN_MANAGER_FRAME_SIZE`: ローカル定数（uint8_t）

### 型定義

基本型は`l_type.h`で定義：
- `l_u8` → `uint8_t`
- `l_u16` → `uint16_t`
- `l_u32` → `uint32_t`
- `l_bool` → `bool`

アプリケーション層では別の型定義を使用：
- `u1` → `uint8_t`
- `u2` → `uint16_t`
- `u4` → `uint32_t`

## 重要な実装ポイント

### 1. 送信完了フラグの管理

送信は`b1g_LIN_TxFrameComplete_ID30h`フラグで制御：
- フラグがTRUEの場合のみ新規送信可能
- `f_LIN_Manager_Callback_TxComplete()`でフラグを管理
- キューに未送信データがある場合は自動的に次のフレームを送信

### 2. 受信データの状態管理

受信は2つの状態で管理：
- `u1l_LIN_Manager_ReceivingContinuousFrame`: 継続フレーム受信中フラグ
- 一時バッファ（`u1l_LIN_Manager_TempRxData`）で継続フレームを結合
- EOF受信時に確定バッファ（`u1l_LIN_Manager_RxData`）に転送

### 3. エラーハンドリング

LINバスエラーの種類：
- フレーミングエラー: `U1G_LIN_FRAMING_ERR`
- オーバーランエラー: `U1G_LIN_OVERRUN_ERR`
- パリティエラー: パリティビットチェック失敗
- タイムアウトエラー: ヘッダー受信タイムアウト

エラーは`xng_lin_sts_buf`のステータスバッファに記録されます。

### 4. 低消費電力対応

- `GPIO_write(CONFIG_LIN_EN, ...)`: LINトランシーバのイネーブル制御
- `f_LIN_Manager_Driver_sleep()`: スリープモード移行時の処理
- スリープ要求は`l_nm_tick_ch1()`に`U1G_LIN_SLP_REQ_ON/OFF`を渡して制御

## ファイル構成の詳細

### LIN_Soft/

**アプリケーション層:**
- `LIN_Manager.c/h`: LINフレーム分割・結合管理

**SLIN API層:**
- `l_slin_api.h`: API関数プロトタイプとマクロ定義
- `l_slin_cmn.c/h`: 共通処理
- `l_slin_tbl.c/h`: テーブル管理（フレーム定義、スケジュール）
- `l_slin_nmc.c/h`: ネットワーク管理コンポーネント

**SLIN CORE層:**
- `l_slin_core_cc2340r53.c/h`: CC2340R53向けコアロジック

**SLIN DRV層:**
- `l_slin_drv_cc2340r53.c/h`: CC2340R53向けドライバ
- `l_slin_sfr_cc2340r53.h`: SFR（特殊機能レジスタ）定義

**型定義・制限値:**
- `l_type.h`: 基本型定義
- `l_stddef.h`: 標準定義
- `l_limit.h`: 制限値定義
- `l_slin_def.h`: SLIN定義

### TI_UART_Driver/

Texas Instruments提供のUART2ドライバ実装：
- `UART2.c/h`: UART2ドライバのメインAPI
- `UART2LPF3.c/h`: Low Power F3（CC2340R53）向け実装
- `UART2Support.h`: サポート関数
- `uart.c/h`: UARTドライバリブラリ
- `hw_uart.h`: ハードウェアレジスタ定義

## デバッグ時の注意点

1. **割り込みコンテキスト**: 受信完了コールバックは割り込みコンテキストで実行されるため、ブロッキング処理を行わないこと

2. **同期フラグの確認**:
   - `b1g_LIN_RxFrameComplete_ID20h`: ID20H受信完了
   - `b1g_LIN_TxFrameComplete_ID30h`: ID30H送信完了
   - フラグの読み書きは`l_u1g_lin_irq_dis()`/`l_vog_lin_irq_res()`で保護すること

3. **タイミング制約**: LIN通信はリアルタイム性が重要。`f_LIN_Manager_MainFunction()`は定期的に呼び出すこと

4. **バッファサイズ**:
   - 最大ペイロードサイズ: 307バイト（`U2G_LIN_MANAGER_MAX_PAYLOAD_SIZE`）
   - 送信キューサイズ: 10フレーム（`U1L_LIN_MANAGER_TXDATA_QUEUE_SIZE`）

## デバイス固有情報

- **ターゲットMCU**: Texas Instruments CC2340R53
- **LINボーレート設定**: `U1G_LIN_BAUDRATE`で選択可能（2400/9600/19200 bps）
- **UART設定**: 8データビット、1ストップビット、パリティ設定可能
- **DMA使用**: UART2ドライバはDMAを使用して効率的なデータ転送を実現

## 編集時の注意

一部のファイルには「編集禁止」の記載があります：
- `l_slin_api.h`
- `l_slin_core_cc2340r53.h`
- `l_slin_drv_cc2340r53.h`

これらはベンダー提供のコードであり、変更が必要な場合は上位層（LIN_Manager）で対応することを推奨します。
