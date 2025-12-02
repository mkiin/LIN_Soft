# ヘッダタイムアウト計算式の意味

## 質問
```c
l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
```
この計算式 `U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH` は何を意味するのか？

---

## 回答

### 定数定義

```c
#define U1G_LIN_HEADER_MAX_TIME  ((l_u8)49)   /* ヘッダータイムアウトMAX値 (bit長) */
#define U1G_LIN_BYTE_LENGTH      ((l_u8)10)   /* 1バイト長 (10bit) */
```

**計算結果:**
```
49 - 10 = 39 bit
```

---

## LINフレームのヘッダー構造

LINフレームのヘッダーは以下の3つのフィールドで構成されます：

```
┌─────────────┬──────────────┬──────────────┐
│ Break Field │ Synch Field  │ ID Field     │
├─────────────┼──────────────┼──────────────┤
│  13 bit以上 │   10 bit     │   10 bit     │
│  (最小値)   │  (0x55送信)  │  (ID送信)    │
└─────────────┴──────────────┴──────────────┘
     ↑
   Breakを検出した時点
```

### 各フィールドの詳細

#### 1. Break Field (ブレークフィールド)
- **最小長**: 13 bit
- **内容**: ドミナント状態（0）を最低13bit以上保持
- **検出方法**: Framing Error + データ0x00で検出

#### 2. Synch Field (シンクフィールド)
- **長さ**: 10 bit (1スタートビット + 8データビット + 1ストップビット)
- **データ**: 0x55 (01010101b)
- **目的**: ボーレート同期（オートボーレート対応時）

#### 3. ID Field (識別子フィールド)
- **長さ**: 10 bit (1スタートビット + 8データビット + 1ストップビット)
- **データ**: 6bit ID + 2bit パリティ
- **目的**: フレーム識別

---

## タイムアウト計算の意味

### 使用箇所

[l_slin_core_CC2340R53.c:704-710](l_slin_core_CC2340R53.c#L704-L710)
```c
/* 受信データが00hならば、SynchBreak受信 */
if( u1a_lin_data == U1G_LIN_SYNCH_BREAK_DATA )
{
    xng_lin_sts_buf.un_state.st_bit.u2g_lin_sts = U2G_LIN_STS_RUN;

    /* ヘッダタイムアウトタイマセット */
    l_vog_lin_bit_tm_set( U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH );
    //                    ↑                        ↑
    //                    49 bit                   10 bit
    //                    (ヘッダー全体の最大時間) (Break Fieldの長さ)
    //                    = 39 bit (残りヘッダー時間)

    /* SynchBreak(IRQ待ち)状態に移行 */
    u1l_lin_slv_sts = U1G_LIN_SLSTS_BREAK_IRQ_WAIT;
    l_vog_lin_rx_dis();
}
```

### 計算の理由

#### **Break検出時点で既に10bit消費している**

```
タイムライン:
                Break検出
                   ↓
├──────────────┼──────────┼──────────┤
│ Break Field  │ Synch    │ ID       │
│  10-13 bit   │ 10 bit   │ 10 bit   │
└──────────────┴──────────┴──────────┘
   ↑←既に消費→↑←残りヘッダー時間→→↑
   (10bit)      (Synch 10bit + ID 10bit = 20bit)
```

#### **LIN規格のヘッダー最大時間**

LIN規格では、ヘッダー全体の最大許容時間が定義されています：
- **ヘッダー最大時間**: 49 bit時間
  - Break Field: 最小13bit（通常10bit程度で検出）
  - Synch Field: 10 bit
  - ID Field: 10 bit
  - 合計: 約30-33 bit（理論値）
  - **マージン込みで49bit**

#### **なぜ10bitを引くのか**

Break受信割り込み時点で：
1. **Break Fieldの一部（約10bit分）を既に受信している**
2. **これから受信するのは**:
   - 残りのBreak Field（あれば）
   - Synch Field (10 bit)
   - ID Field (10 bit)

**よって:**
```
残りヘッダー時間 = ヘッダー全体の最大時間 - 既に消費した時間
                 = 49 bit - 10 bit
                 = 39 bit
```

---

## 具体的な動作シーケンス

### ケース1: 正常なヘッダー受信

```
時刻  イベント                          タイマー状態
----  --------------------------------  --------------------
t=0   Break検出 (0x00 + Framing Error)  タイマー起動: 39bit
      ↓ l_vog_lin_bit_tm_set(39)

t=1   Synch Field受信 (0x55)            タイマー継続中
      ↓ 状態: SYNCHFIELD_WAIT→IDENTFIELD_WAIT

t=2   ID Field受信 (例: 0x30)           タイマー停止
      ↓ l_vog_lin_frm_tm_stop()         l_vog_lin_frm_tm_stop()で停止

      レスポンス処理開始
```

**所要時間**: 約20-30 bit時間 → **39bit以内に完了** ✅

### ケース2: ヘッダータイムアウト

```
時刻  イベント                          タイマー状態
----  --------------------------------  --------------------
t=0   Break検出 (0x00 + Framing Error)  タイマー起動: 39bit
      ↓ l_vog_lin_bit_tm_set(39)

t=1   Synch Field受信 (0x55)            タイマー継続中

...   ★ ID Fieldが来ない ★             タイマー継続中
      (バスエラー、マスタ故障など)

t=39  タイマー割り込み発生！             タイムアウト検出
      ↓ l_vog_lin_tm_int()

      Physical Busエラー設定
      xng_lin_sts_buf.un_state.st_bit.u2g_lin_phy_bus_err = SET
```

**所要時間**: 39 bit時間経過 → **タイムアウトエラー** ❌

---

## なぜ49bitなのか？

### LIN仕様の背景

LIN規格では、ヘッダー送信の最大時間が規定されています：

| フィールド | 最小 | 標準 | 最大 |
|-----------|------|------|------|
| Break Field | 13 bit | 13-26 bit | 無制限 |
| Delimiter (Break後のレセッシブ) | 1 bit | 1 bit | - |
| Synch Field | 10 bit | 10 bit | 10 bit |
| ID Field | 10 bit | 10 bit | 10 bit |

**合計 (標準的なケース)**:
```
Break (13-26 bit) + Delimiter (1 bit) + Synch (10 bit) + ID (10 bit)
= 約34-47 bit
```

**マージン込み**: **49 bit**（安全側に設定）

### H850でも同じ定義

[H850/slin_lib/l_slin_core_h83687.h:47-48](../H850/slin_lib/l_slin_core_h83687.h#L47-L48)
```c
#define  U1G_LIN_HEADER_MAX_TIME  ((l_u8)49)   /* ヘッダータイムアウトMAX値 */
#define  U1G_LIN_BYTE_LENGTH      ((l_u8)10)   /* 1バイト長(10bit) */
```

**H850との互換性を維持** → 同じ定義を使用

---

## 19200bpsでの実際の時間

### ビット時間の計算

1 bit時間 = 1 / 19200 bps = **52.08 μs**

### ヘッダータイムアウト時間

```
残りヘッダー時間 = 39 bit × 52.08 μs/bit
                 = 2031.25 μs
                 ≈ 2.03 ms
```

### ヘッダー全体の最大時間

```
ヘッダー全体時間 = 49 bit × 52.08 μs/bit
                 = 2551.92 μs
                 ≈ 2.55 ms
```

---

## Physical Busエラー検出との関係

### Physical Busエラーの定義

LIN規格では、以下の場合にPhysical Busエラーとします：
- **連続してヘッダータイムアウトが発生**
- **一定時間（25000 bit時間）内に規定回数以上のタイムアウト**

### エラーカウント処理

[H850/slin_lib/l_slin_core_h83687.h:73](../H850/slin_lib/l_slin_core_h83687.h#L73)
```c
#define U2G_LIN_HERR_LIMIT  ((l_u16)510)  /* 25000bitタイム分のヘッダタイム回数(25000 / 49) */
```

**計算**:
```
ヘッダータイム回数 = 25000 bit / 49 bit
                  = 510.2...
                  ≈ 510 回
```

**意味**:
- 510回連続でヘッダータイムアウトが発生した場合
- → Physical Busエラーと判定
- → 約25000 bit時間 = 1.3秒 (@19200bps)

---

## まとめ

### 計算式の意味

```c
U1G_LIN_HEADER_MAX_TIME - U1G_LIN_BYTE_LENGTH
= 49 bit - 10 bit
= 39 bit
```

| 要素 | 意味 |
|-----|------|
| `U1G_LIN_HEADER_MAX_TIME` (49 bit) | LINヘッダー全体の最大許容時間 |
| `U1G_LIN_BYTE_LENGTH` (10 bit) | Break Field検出時に既に消費した時間 |
| **差分 (39 bit)** | **Break検出後、残りヘッダー受信に許容される最大時間** |

### 動作の流れ

```
1. Break検出 (約10bit消費)
   ↓
2. タイマー起動: 残り39bit時間でヘッダー受信完了を監視
   ↓
3-a. 正常: Synch Field (10bit) + ID Field (10bit) 受信
     → 約20bit消費 → 39bit以内 → タイマー停止 → 正常処理

3-b. 異常: ID Fieldが来ない
     → 39bit経過 → タイムアウト割り込み → エラー処理
```

### 19200bpsでの実時間

- **残りヘッダータイムアウト**: 39 bit = **約2.03 ms**
- **ヘッダー全体最大時間**: 49 bit = **約2.55 ms**

---

## 参照

- 実装箇所: [l_slin_core_CC2340R53.c:708](l_slin_core_CC2340R53.c#L708), [l_slin_core_CC2340R53.c:748](l_slin_core_CC2340R53.c#L748)
- H850互換定義: [H850/slin_lib/l_slin_core_h83687.h:47-48](../H850/slin_lib/l_slin_core_h83687.h#L47-L48)
- タイマー割り込みハンドラ: [l_slin_core_CC2340R53.c:1126-1254](l_slin_core_CC2340R53.c#L1126-L1254)
