# Test Plan — C TCP Client-Server with ASN.1 DER

## 1. 範圍 (Scope)

本測試計畫涵蓋以下三個層次：

| 層次 | 目標模組 | 測試方式 |
|------|----------|----------|
| Unit Test | `user_info.h`（encode / decode） | `test_user_info.c` 直接呼叫 |
| Integration Test | `server.c` + `client.c` 端對端傳輸 | Shell script 自動化 |
| Negative / Robustness Test | 非法輸入、邊界值、畸形封包 | Unit Test + Shell script |

**不在範圍內：** 多客戶端並發、TLS/加密、跨平台（非 Linux）。

---

## 2. 測試環境

- OS: Linux (WSL2 可接受)
- 編譯器: GCC，使用 `-Wall -Wextra`
- Port: 8080 (測試前確保未被佔用)
- 依賴: 無外部函式庫

編譯指令：
```sh
gcc -Wall -Wextra -o test_user_info test_user_info.c
gcc -o server server.c
gcc -o client client.c
```

---

## 3. 測試項目總覽

### 3.1 Unit Tests — ASN.1 DER Encode

| ID | 測試名稱 | 優先度 |
|----|----------|--------|
| U-ENC-01 | 正常輸入（一般 ASCII） | P0 |
| U-ENC-02 | 空字串 username 與 email | P1 |
| U-ENC-03 | 最大長度欄位（127 bytes） | P1 |
| U-ENC-04 | 多位元組 UTF-8 字元 | P1 |
| U-ENC-05 | 輸出緩衝區過小（buf_len 不足） | P0 |
| U-ENC-06 | 驗證 DER 位元組結構正確性 | P0 |

### 3.2 Unit Tests — ASN.1 DER Decode

| ID | 測試名稱 | 優先度 |
|----|----------|--------|
| U-DEC-01 | 解碼正確 DER 資料 | P0 |
| U-DEC-02 | Encode → Decode 往返一致性 | P0 |
| U-DEC-03 | buf_len = 0（空緩衝區） | P0 |
| U-DEC-04 | SEQUENCE tag 錯誤（非 0x30） | P0 |
| U-DEC-05 | UTF8String tag 錯誤（username） | P0 |
| U-DEC-06 | UTF8String tag 錯誤（email） | P0 |
| U-DEC-07 | seq_len 超過實際 buf_len | P0 |
| U-DEC-08 | username_len = MAX_FIELD_LEN（邊界溢出） | P0 |
| U-DEC-09 | 資料截斷（僅含 SEQUENCE header） | P1 |
| U-DEC-10 | 空字串解碼（encode 空字串後解碼） | P1 |

### 3.3 Integration Tests — 端對端

| ID | 測試名稱 | 優先度 |
|----|----------|--------|
| I-E2E-01 | 正常傳輸並正確顯示 | P0 |
| I-E2E-02 | 特殊字元（含空格、符號） | P1 |
| I-E2E-03 | 最大長度欄位端對端 | P1 |
| I-E2E-04 | UTF-8 中文字元端對端 | P1 |
| I-E2E-05 | Server 未啟動時 Client 的錯誤處理 | P1 |
| I-E2E-06 | Server 接收畸形 DER 封包 | P1 |

---

## 4. 詳細 Test Cases

### U-ENC-01：正常輸入

- **前置條件：** `UserInfo` 填入 `"alice"` / `"alice@example.com"`
- **執行：** 呼叫 `user_info_encode()`
- **預期結果：**
  - 回傳值 > 0
  - `buf[0] == 0x30`（SEQUENCE）
  - `buf[2] == 0x0C`（UTF8String）
  - 解碼後欄位與輸入相同

### U-ENC-02：空字串

- **前置條件：** username = `""`, email = `""`
- **執行：** 呼叫 `user_info_encode()`
- **預期結果：**
  - 回傳值 = 6（`0x30 0x04 0x0C 0x00 0x0C 0x00`）
  - 不崩潰

### U-ENC-03：最大長度欄位

- **前置條件：** username = 127 個 `'A'`，email = 127 個 `'B'`
- **執行：** 呼叫 `user_info_encode()`
- **預期結果：**
  - 回傳值 = 2 + (2+127) + (2+127) = 260
  - `buf[1] == 258`（SEQUENCE 長度，short-form 可表示）

### U-ENC-04：UTF-8 多位元組字元

- **前置條件：** username = `"使用者"`（9 bytes UTF-8），email = `"test@test.com"`
- **執行：** 呼叫 `user_info_encode()`
- **預期結果：** 回傳值正確；`buf[3] == 9`（username 長度為位元組數而非字元數）

### U-ENC-05：緩衝區過小

- **前置條件：** username = `"alice"`，email = `"alice@example.com"`，`buf_len = 5`
- **執行：** 呼叫 `user_info_encode()`
- **預期結果：** 回傳 `-1`，不寫入超出範圍的資料

### U-ENC-06：DER 位元組結構驗證

- **前置條件：** username = `"ab"`，email = `"c@d"`
- **執行：** 呼叫 `user_info_encode()`，手動檢查每個位元組
- **預期結果（hex）：**
  ```
  30 0B        -- SEQUENCE, len=11
    0C 02 61 62  -- UTF8String "ab"
    0C 03 63 40 64  -- UTF8String "c@d"
  ```

---

### U-DEC-01：解碼正確 DER 資料

- **前置條件：** 手動構造合法 DER bytes
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 0；欄位值正確

### U-DEC-02：Encode → Decode 往返一致性

- **前置條件：** 任意合法輸入
- **執行：** encode → decode，比較原始與解碼後的 UserInfo
- **預期結果：** `strcmp` 均為 0

### U-DEC-03：空緩衝區

- **前置條件：** `buf_len = 0`
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 `-1`

### U-DEC-04：SEQUENCE tag 錯誤

- **前置條件：** `buf[0] = 0x10`（非 `0x30`）
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 `-1`

### U-DEC-05：username UTF8String tag 錯誤

- **前置條件：** 合法 SEQUENCE header，但第 3 byte 為 `0x04`（OCTET STRING）
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 `-1`

### U-DEC-06：email UTF8String tag 錯誤

- **前置條件：** username 正確，但 email tag 為 `0x16`（IA5String）
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 `-1`

### U-DEC-07：seq_len 超過 buf_len

- **前置條件：** `buf[1] = 0xFF`，但實際 buf_len 僅有 10
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 `-1`

### U-DEC-08：username_len = MAX_FIELD_LEN（緩衝區溢出邊界）

- **前置條件：** 手動構造 `username_len = 256`（等於 `MAX_FIELD_LEN`）
- **執行：** 呼叫 `user_info_decode()`
- **預期結果：** 回傳 `-1`（`username_len >= MAX_FIELD_LEN` 的保護觸發）

### U-DEC-09：資料截斷

- **前置條件：** 僅提供 `{0x30, 0x10}`（宣稱 16 bytes，但後續無資料）
- **執行：** 呼叫 `user_info_decode(buf, 2, &user)`
- **預期結果：** 回傳 `-1`

### U-DEC-10：空字串往返

- **前置條件：** encode 空字串後再 decode
- **執行：** encode → decode
- **預期結果：** 兩個欄位均為空字串，回傳 0

---

### I-E2E-01：正常端對端傳輸

- **步驟：** 啟動 server → client 輸入一般 ASCII username / email → 連線傳送
- **預期結果：** Server 顯示正確的 username 與 email，兩端 exit code 均為 0

### I-E2E-02：特殊字元

- **前置條件：** username = `"john doe"`，email = `"john+tag@ex.com"`
- **預期結果：** Server 原樣輸出，不截斷或亂碼

### I-E2E-03：最大長度欄位

- **前置條件：** username / email 各填入 127 個字元
- **預期結果：** 傳輸成功，Server 正確顯示

### I-E2E-04：UTF-8 中文字元

- **前置條件：** username = `"測試用戶"`，email = `"test@example.com"`
- **預期結果：** Server 以 UTF-8 正確輸出中文，無亂碼

### I-E2E-05：Server 未啟動

- **步驟：** 確保 8080 未被監聽，直接執行 `./client`
- **預期結果：** Client 輸出 `connect: Connection refused`，exit code = 1

### I-E2E-06：畸形 DER 封包

- **步驟：** 使用 `nc` 或自訂腳本向 Server 傳送非合法 DER bytes（如 `\x00\x00\x00`）
- **預期結果：** Server 輸出 `ASN.1 decoding failed`，exit code = 1，不崩潰

---

## 5. 已知限制（Known Limitations）

| 項目 | 說明 |
|------|------|
| Long-form DER length | 欄位超過 127 bytes 時 length encoding 會錯誤（short-form 限制，未報錯） |
| 單一客戶端 | Server 僅接受一個連線後即關閉 |
| recv() 不保證完整性 | 單次 `recv()` 在高延遲/大封包下可能只收到部分資料（無 framing） |
| username_len short-form | encode 時僅取 `strlen()` 的低 8 bits 填入 length，若超過 255 會溢出 |

---

## 6. Pass / Fail 標準

- **Pass：** 所有 P0 測試通過；P1 測試通過率 ≥ 80%
- **Fail：** 任何 P0 測試失敗，或程式在任何測試中 crash（Segfault / heap corruption）
