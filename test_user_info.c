/**
 * @file    test_user_info.c
 * @brief   Unit tests for ASN.1 DER encode/decode in user_info.h
 *
 * Compile:
 *   gcc -Wall -Wextra -o test_user_info test_user_info.c
 *
 * Run:
 *   ./test_user_info
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "user_info.h"

/* ------------------------------------------------------------------ */
/*  Minimal test framework                                              */
/* ------------------------------------------------------------------ */

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT(cond, msg)                                              \
    do {                                                               \
        if (cond) {                                                    \
            printf("  [PASS] %s\n", msg);                             \
            g_pass++;                                                  \
        } else {                                                       \
            printf("  [FAIL] %s  (line %d)\n", msg, __LINE__);        \
            g_fail++;                                                  \
        }                                                              \
    } while (0)

#define RUN(name)                                                      \
    do {                                                               \
        printf("\n--- " #name " ---\n");                              \
        name();                                                        \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Helper                                                              */
/* ------------------------------------------------------------------ */

static UserInfo make_user(const char *username, const char *email) {
    UserInfo u;
    memset(&u, 0, sizeof(u));
    strncpy(u.username, username, MAX_FIELD_LEN - 1);
    strncpy(u.email,    email,    MAX_FIELD_LEN - 1);
    return u;
}

/* ------------------------------------------------------------------ */
/*  Encode Tests                                                        */
/* ------------------------------------------------------------------ */

/* U-ENC-01: Normal ASCII input */
static void test_enc_normal(void) {
    UserInfo u = make_user("alice", "alice@example.com");
    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&u, buf, sizeof(buf));

    ASSERT(len > 0,           "U-ENC-01: return value > 0");
    ASSERT(buf[0] == 0x30,    "U-ENC-01: buf[0] is SEQUENCE tag (0x30)");
    ASSERT(buf[2] == 0x0C,    "U-ENC-01: buf[2] is UTF8String tag (0x0C)");
}

/* U-ENC-02: Empty strings */
static void test_enc_empty(void) {
    UserInfo u = make_user("", "");
    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&u, buf, sizeof(buf));

    /* Expected: 30 06 0C 00 0C 00 0C 00 = 8 bytes */
    ASSERT(len == 8,        "U-ENC-02: empty fields produce 8 bytes");
    ASSERT(buf[3] == 0x00,  "U-ENC-02: username length byte is 0");
    ASSERT(buf[5] == 0x00,  "U-ENC-02: email length byte is 0");
    ASSERT(buf[7] == 0x00,  "U-ENC-02: telephone length byte is 0");
}

/* U-ENC-03: Maximum short-form field length (127 bytes each) */
static void test_enc_max_field(void) {
    UserInfo u;
    memset(&u, 0, sizeof(u));
    memset(u.username, 'A', 127);
    memset(u.email,    'B', 127);

    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&u, buf, sizeof(buf));
    int expected = 2 + (2 + 127) + (2 + 127) + (2 + 0); /* 262 */

    ASSERT(len == expected, "U-ENC-03: 127-byte fields produce correct total length");
    ASSERT(buf[3] == 127,  "U-ENC-03: username length byte is 127");
}

/* U-ENC-04: Multi-byte UTF-8 characters */
static void test_enc_utf8(void) {
    /* "使用者" = 9 bytes in UTF-8 */
    UserInfo u = make_user("\xe4\xbd\xbf\xe7\x94\xa8\xe8\x80\x85", "t@t.com");
    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&u, buf, sizeof(buf));

    ASSERT(len > 0,      "U-ENC-04: UTF-8 username encodes without error");
    ASSERT(buf[3] == 9,  "U-ENC-04: username length is byte count (9), not char count");
}

/* U-ENC-05: Output buffer too small */
static void test_enc_buf_too_small(void) {
    UserInfo u = make_user("alice", "alice@example.com");
    uint8_t buf[5];
    int len = user_info_encode(&u, buf, sizeof(buf));

    ASSERT(len == -1, "U-ENC-05: returns -1 when buffer is too small");
}

/* U-ENC-06: Verify exact DER byte layout */
static void test_enc_der_layout(void) {
    /* username="ab" (2 bytes), email="c@d" (3 bytes), telephone="" (0 bytes)
     * Expected: 30 0B  0C 02 61 62  0C 03 63 40 64  0C 00
     *           ^seq   ^utf8 a b    ^utf8 c @ d      ^utf8 (empty)
     */
    UserInfo u = make_user("ab", "c@d");
    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&u, buf, sizeof(buf));

    ASSERT(len == 13,          "U-ENC-06: total length is 13");
    ASSERT(buf[0]  == 0x30,    "U-ENC-06: SEQUENCE tag");
    ASSERT(buf[1]  == 0x0B,    "U-ENC-06: SEQUENCE length = 11");
    ASSERT(buf[2]  == 0x0C,    "U-ENC-06: username UTF8String tag");
    ASSERT(buf[3]  == 0x02,    "U-ENC-06: username length = 2");
    ASSERT(buf[4]  == 'a',     "U-ENC-06: username[0] = 'a'");
    ASSERT(buf[5]  == 'b',     "U-ENC-06: username[1] = 'b'");
    ASSERT(buf[6]  == 0x0C,    "U-ENC-06: email UTF8String tag");
    ASSERT(buf[7]  == 0x03,    "U-ENC-06: email length = 3");
    ASSERT(buf[8]  == 'c',     "U-ENC-06: email[0] = 'c'");
    ASSERT(buf[9]  == '@',     "U-ENC-06: email[1] = '@'");
    ASSERT(buf[10] == 'd',     "U-ENC-06: email[2] = 'd'");
    ASSERT(buf[11] == 0x0C,    "U-ENC-06: telephone UTF8String tag");
    ASSERT(buf[12] == 0x00,    "U-ENC-06: telephone length = 0");
}

/* ------------------------------------------------------------------ */
/*  Decode Tests                                                        */
/* ------------------------------------------------------------------ */

/* U-DEC-01: Decode manually-constructed valid DER */
static void test_dec_valid(void) {
    /* 30 0B  0C 02 61 62  0C 03 63 40 64  0C 00  => "ab" / "c@d" / "" */
    uint8_t der[] = {0x30, 0x0B, 0x0C, 0x02, 'a', 'b', 0x0C, 0x03, 'c', '@', 'd', 0x0C, 0x00};
    UserInfo u;
    int rc = user_info_decode(der, sizeof(der), &u);

    ASSERT(rc == 0,                            "U-DEC-01: returns 0 for valid DER");
    ASSERT(strcmp(u.username,  "ab")  == 0,    "U-DEC-01: username = \"ab\"");
    ASSERT(strcmp(u.email,     "c@d") == 0,    "U-DEC-01: email = \"c@d\"");
    ASSERT(strcmp(u.telephone, "")    == 0,    "U-DEC-01: telephone = \"\"");
}

/* U-DEC-02: Round-trip consistency (encode then decode) */
static void test_dec_roundtrip(void) {
    UserInfo orig = make_user("testuser", "test@example.com");
    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&orig, buf, sizeof(buf));

    UserInfo decoded;
    int rc = user_info_decode(buf, len, &decoded);

    ASSERT(rc == 0,                                        "U-DEC-02: decode succeeds");
    ASSERT(strcmp(orig.username, decoded.username) == 0,   "U-DEC-02: username matches");
    ASSERT(strcmp(orig.email,    decoded.email)    == 0,   "U-DEC-02: email matches");
}

/* U-DEC-03: Zero-length buffer */
static void test_dec_empty_buf(void) {
    uint8_t buf[1] = {0};
    UserInfo u;
    int rc = user_info_decode(buf, 0, &u);

    ASSERT(rc == -1, "U-DEC-03: returns -1 for buf_len=0");
}

/* U-DEC-04: Wrong SEQUENCE tag */
static void test_dec_bad_seq_tag(void) {
    uint8_t der[] = {0x10, 0x09, 0x0C, 0x02, 'a', 'b', 0x0C, 0x03, 'c', '@', 'd'};
    UserInfo u;
    int rc = user_info_decode(der, sizeof(der), &u);

    ASSERT(rc == -1, "U-DEC-04: returns -1 when SEQUENCE tag is wrong");
}

/* U-DEC-05: Wrong UTF8String tag for username */
static void test_dec_bad_username_tag(void) {
    uint8_t der[] = {0x30, 0x09, 0x04, 0x02, 'a', 'b', 0x0C, 0x03, 'c', '@', 'd'};
    UserInfo u;
    int rc = user_info_decode(der, sizeof(der), &u);

    ASSERT(rc == -1, "U-DEC-05: returns -1 when username tag is wrong (0x04)");
}

/* U-DEC-06: Wrong UTF8String tag for email */
static void test_dec_bad_email_tag(void) {
    uint8_t der[] = {0x30, 0x09, 0x0C, 0x02, 'a', 'b', 0x16, 0x03, 'c', '@', 'd'};
    UserInfo u;
    int rc = user_info_decode(der, sizeof(der), &u);

    ASSERT(rc == -1, "U-DEC-06: returns -1 when email tag is wrong (0x16)");
}

/* U-DEC-07: seq_len exceeds buf_len */
static void test_dec_seq_len_overflow(void) {
    uint8_t der[] = {0x30, 0xFF, 0x0C, 0x02, 'a', 'b'};
    UserInfo u;
    int rc = user_info_decode(der, sizeof(der), &u);

    ASSERT(rc == -1, "U-DEC-07: returns -1 when seq_len > buf_len");
}

/* U-DEC-08: username_len = MAX_FIELD_LEN (boundary overflow guard) */
static void test_dec_username_len_boundary(void) {
    /* Construct a DER where username_len = 256 = MAX_FIELD_LEN */
    uint8_t der[600];
    memset(der, 'X', sizeof(der));
    der[0] = 0x30;
    der[1] = 0xFE;   /* seq_len = 254, fits in buf */
    der[2] = 0x0C;
    der[3] = 0xFF;   /* username_len = 255... but we test =256 via uint8 wrap */
    /* Note: uint8_t max is 255; to hit MAX_FIELD_LEN=256 we'd need long-form.
       Test that 255 (0xFF) is still rejected since 255 < 256 passes.
       Actually 255 < MAX_FIELD_LEN(256) so it passes the check.
       We test that decode doesn't overflow with a field length that would
       push offset past buf_len instead. */
    UserInfo u;
    int rc = user_info_decode(der, 6, &u);

    ASSERT(rc == -1, "U-DEC-08: returns -1 when username_len causes out-of-bounds read");
}

/* U-DEC-09: Truncated data (only SEQUENCE header, no fields) */
static void test_dec_truncated(void) {
    uint8_t der[] = {0x30, 0x10}; /* Declares 16 bytes but buf_len = 2 */
    UserInfo u;
    int rc = user_info_decode(der, sizeof(der), &u);

    ASSERT(rc == -1, "U-DEC-09: returns -1 for truncated buffer");
}

/* U-DEC-10: Empty string round-trip */
static void test_dec_empty_strings(void) {
    UserInfo orig = make_user("", "");
    uint8_t buf[MAX_DER_LEN];
    int len = user_info_encode(&orig, buf, sizeof(buf));

    UserInfo decoded;
    int rc = user_info_decode(buf, len, &decoded);

    ASSERT(rc == 0,                          "U-DEC-10: decode of empty strings succeeds");
    ASSERT(strlen(decoded.username) == 0,    "U-DEC-10: decoded username is empty");
    ASSERT(strlen(decoded.email)    == 0,    "U-DEC-10: decoded email is empty");
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("========================================\n");
    printf("  Unit Tests: user_info.h ASN.1 DER\n");
    printf("========================================\n");

    RUN(test_enc_normal);
    RUN(test_enc_empty);
    RUN(test_enc_max_field);
    RUN(test_enc_utf8);
    RUN(test_enc_buf_too_small);
    RUN(test_enc_der_layout);

    RUN(test_dec_valid);
    RUN(test_dec_roundtrip);
    RUN(test_dec_empty_buf);
    RUN(test_dec_bad_seq_tag);
    RUN(test_dec_bad_username_tag);
    RUN(test_dec_bad_email_tag);
    RUN(test_dec_seq_len_overflow);
    RUN(test_dec_username_len_boundary);
    RUN(test_dec_truncated);
    RUN(test_dec_empty_strings);

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", g_pass, g_fail);
    printf("========================================\n");

    return g_fail == 0 ? 0 : 1;
}
