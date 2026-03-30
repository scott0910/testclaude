# 測試報告 — C TCP Client-Server with ASN.1 DER

| 項目 | 內容 |
|------|------|
| 報告日期 | 2026-03-30 |
| 專案 | C TCP Client-Server with ASN.1 DER |
| 作者 | Scott |
| 測試環境 | Linux 5.15.167.4-microsoft-standard-WSL2 |
| 測試依據 | TEST_PLAN.md |

---

## 總結

| 測試類型 | 案例數 | 通過 | 失敗 | 通過率 |
|----------|--------|------|------|--------|
| Unit Test | 43 | 43 | 0 | 100% |
| Integration Test | 13 | 13 | 0 | 100% |
| **合計** | **56** | **56** | **0** | **100%** |

> **結論：所有測試通過，無任何失敗項目。**

---

## Unit Tests — `test_user_info.c`

### ASN.1 DER Encode

| Test ID | 測試項目 | 斷言數 | 結果 |
|---------|----------|--------|------|
| U-ENC-01 | 正常 ASCII 輸入 | 3 | PASS |
| U-ENC-02 | 空字串 username / email / telephone | 4 | PASS |
| U-ENC-03 | 最大長度欄位（username/email 各 127 bytes） | 2 | PASS |
| U-ENC-04 | 多位元組 UTF-8 字元 | 2 | PASS |
| U-ENC-05 | 輸出緩衝區過小 | 1 | PASS |
| U-ENC-06 | DER 位元組結構逐一驗證（含 telephone） | 13 | PASS |

### ASN.1 DER Decode

| Test ID | 測試項目 | 斷言數 | 結果 |
|---------|----------|--------|------|
| U-DEC-01 | 解碼手動構造的合法 DER（含 telephone） | 4 | PASS |
| U-DEC-02 | Encode → Decode 往返一致性 | 3 | PASS |
| U-DEC-03 | buf_len = 0（空緩衝區） | 1 | PASS |
| U-DEC-04 | SEQUENCE tag 錯誤（0x10 而非 0x30） | 1 | PASS |
| U-DEC-05 | username UTF8String tag 錯誤（0x04） | 1 | PASS |
| U-DEC-06 | email UTF8String tag 錯誤（0x16） | 1 | PASS |
| U-DEC-07 | seq_len 超過實際 buf_len | 1 | PASS |
| U-DEC-08 | username_len 造成越界讀取 | 1 | PASS |
| U-DEC-09 | 資料截斷（僅含 SEQUENCE header） | 1 | PASS |
| U-DEC-10 | 空字串往返解碼 | 3 | PASS |

### 原始輸出

```
========================================
  Unit Tests: user_info.h ASN.1 DER
========================================

--- test_enc_normal ---
  [PASS] U-ENC-01: return value > 0
  [PASS] U-ENC-01: buf[0] is SEQUENCE tag (0x30)
  [PASS] U-ENC-01: buf[2] is UTF8String tag (0x0C)

--- test_enc_empty ---
  [PASS] U-ENC-02: empty fields produce 8 bytes
  [PASS] U-ENC-02: username length byte is 0
  [PASS] U-ENC-02: email length byte is 0
  [PASS] U-ENC-02: telephone length byte is 0

--- test_enc_max_field ---
  [PASS] U-ENC-03: 127-byte fields produce correct total length
  [PASS] U-ENC-03: username length byte is 127

--- test_enc_utf8 ---
  [PASS] U-ENC-04: UTF-8 username encodes without error
  [PASS] U-ENC-04: username length is byte count (9), not char count

--- test_enc_buf_too_small ---
  [PASS] U-ENC-05: returns -1 when buffer is too small

--- test_enc_der_layout ---
  [PASS] U-ENC-06: total length is 13
  [PASS] U-ENC-06: SEQUENCE tag
  [PASS] U-ENC-06: SEQUENCE length = 11
  [PASS] U-ENC-06: username UTF8String tag
  [PASS] U-ENC-06: username length = 2
  [PASS] U-ENC-06: username[0] = 'a'
  [PASS] U-ENC-06: username[1] = 'b'
  [PASS] U-ENC-06: email UTF8String tag
  [PASS] U-ENC-06: email length = 3
  [PASS] U-ENC-06: email[0] = 'c'
  [PASS] U-ENC-06: email[1] = '@'
  [PASS] U-ENC-06: email[2] = 'd'
  [PASS] U-ENC-06: telephone UTF8String tag
  [PASS] U-ENC-06: telephone length = 0

--- test_dec_valid ---
  [PASS] U-DEC-01: returns 0 for valid DER
  [PASS] U-DEC-01: username = "ab"
  [PASS] U-DEC-01: email = "c@d"
  [PASS] U-DEC-01: telephone = ""

--- test_dec_roundtrip ---
  [PASS] U-DEC-02: decode succeeds
  [PASS] U-DEC-02: username matches
  [PASS] U-DEC-02: email matches

--- test_dec_empty_buf ---
  [PASS] U-DEC-03: returns -1 for buf_len=0

--- test_dec_bad_seq_tag ---
  [PASS] U-DEC-04: returns -1 when SEQUENCE tag is wrong

--- test_dec_bad_username_tag ---
  [PASS] U-DEC-05: returns -1 when username tag is wrong (0x04)

--- test_dec_bad_email_tag ---
  [PASS] U-DEC-06: returns -1 when email tag is wrong (0x16)

--- test_dec_seq_len_overflow ---
  [PASS] U-DEC-07: returns -1 when seq_len > buf_len

--- test_dec_username_len_boundary ---
  [PASS] U-DEC-08: returns -1 when username_len causes out-of-bounds read

--- test_dec_truncated ---
  [PASS] U-DEC-09: returns -1 for truncated buffer

--- test_dec_empty_strings ---
  [PASS] U-DEC-10: decode of empty strings succeeds
  [PASS] U-DEC-10: decoded username is empty
  [PASS] U-DEC-10: decoded email is empty

========================================
  Results: 43 passed, 0 failed
========================================
```

---

## Integration Tests — `test_integration.sh`

| Test ID | 測試項目 | 斷言數 | 結果 |
|---------|----------|--------|------|
| I-E2E-01 | 正常 ASCII 輸入端對端傳輸（含 telephone） | 4 | PASS |
| I-E2E-02 | 特殊字元（空格、`+`、`-` 符號） | 3 | PASS |
| I-E2E-03 | 最大長度欄位（username/email 各 127 bytes） | 2 | PASS |
| I-E2E-04 | UTF-8 中文字元端對端 | 1 | PASS |
| I-E2E-05 | Server 未啟動時的錯誤處理 | 2 | PASS |
| I-E2E-06 | 畸形 DER 封包（`\x00\x00\x00`） | 1 | PASS |

### 原始輸出

```
--- I-E2E-01: Normal ASCII input ---
  [PASS] I-E2E-01: Server output contains correct username
  [PASS] I-E2E-01: Server output contains correct email
  [PASS] I-E2E-01: Server output contains correct telephone
  [PASS] I-E2E-01: Client exits successfully (exit code 0)

--- I-E2E-02: Special characters (spaces, symbols) ---
  [PASS] I-E2E-02: Username with space decoded correctly
  [PASS] I-E2E-02: Email with '+' decoded correctly
  [PASS] I-E2E-02: Telephone with '+' and '-' decoded correctly

--- I-E2E-03: Maximum field length (127 bytes) ---
  [PASS] I-E2E-03: Client reports 282 encoded bytes
  [PASS] I-E2E-03: Server receives and decodes max-length fields

--- I-E2E-04: UTF-8 Chinese characters ---
  [PASS] I-E2E-04: Server output contains UTF-8 Chinese username

--- I-E2E-05: Server not running (connection refused) ---
  [PASS] I-E2E-05: Client exits with non-zero code when server is down
  [PASS] I-E2E-05: Client prints connection error message

--- I-E2E-06: Malformed DER packet ---
  [PASS] I-E2E-06: Server reports ASN.1 decoding failure

========================================
  Integration Results: 13 passed, 0 failed
========================================
```

---

## 已知限制（不影響本次測試結果）

| # | 項目 | 說明 |
|---|------|------|
| 1 | Short-form DER 長度限制 | 每欄位最多 127 bytes；超過需改用 long-form encoding |
| 2 | Server 單次連線 | 每次啟動僅接受一個 client 連線 |
| 3 | telephone 欄位無格式驗證 | 接受任意字串，未驗證電話號碼格式 |
