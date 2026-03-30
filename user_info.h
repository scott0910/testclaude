/**
 * @file    user_info.h
 * @brief   ASN.1 DER encoding/decoding for UserInfo
 *
 * ASN.1 Schema (user_info.asn1):
 *   UserInfo ::= SEQUENCE {
 *       username   UTF8String,
 *       email      UTF8String,
 *       telephone  UTF8String
 *   }
 *
 * DER encoding layout:
 *   30 LL          -- SEQUENCE
 *     0C LL xx..   -- UTF8String (username)
 *     0C LL xx..   -- UTF8String (email)
 *     0C LL xx..   -- UTF8String (telephone)
 *
 * @author  Scott
 * @date    2026-03-30
 */

#ifndef USER_INFO_H
#define USER_INFO_H

#include <stdint.h>
#include <string.h>

#define MAX_FIELD_LEN  256
#define MAX_DER_LEN    600

/* ASN.1 DER tags */
#define ASN1_TAG_SEQUENCE   0x30
#define ASN1_TAG_UTF8STRING 0x0C

typedef struct {
    char username[MAX_FIELD_LEN];
    char email[MAX_FIELD_LEN];
    char telephone[MAX_FIELD_LEN];
} UserInfo;

/**
 * @brief  Encode a single ASN.1 UTF8String field into a buffer.
 * @return Number of bytes written.
 */
static inline int asn1_encode_utf8string(uint8_t *buf, const char *str) {
    int len = (int)strlen(str);
    buf[0] = ASN1_TAG_UTF8STRING;
    buf[1] = (uint8_t)len;          /* Short-form length (max 127 bytes) */
    memcpy(buf + 2, str, len);
    return 2 + len;
}

/**
 * @brief  Encode a UserInfo struct into DER format.
 * @return Total number of bytes written, or -1 on error.
 */
static inline int user_info_encode(const UserInfo *user, uint8_t *buf, int buf_len) {
    uint8_t tmp[MAX_DER_LEN];
    int offset = 0;

    /* Encode username, email, and telephone into temporary buffer */
    offset += asn1_encode_utf8string(tmp + offset, user->username);
    offset += asn1_encode_utf8string(tmp + offset, user->email);
    offset += asn1_encode_utf8string(tmp + offset, user->telephone);

    /* Wrap with SEQUENCE tag and length */
    if (2 + offset > buf_len) return -1;
    buf[0] = ASN1_TAG_SEQUENCE;
    buf[1] = (uint8_t)offset;       /* Short-form length */
    memcpy(buf + 2, tmp, offset);

    return 2 + offset;
}

/**
 * @brief  Decode a DER-encoded buffer into a UserInfo struct.
 * @return 0 on success, -1 on error.
 */
static inline int user_info_decode(const uint8_t *buf, int buf_len, UserInfo *user) {
    int offset = 0;

    /* Verify SEQUENCE tag */
    if (buf_len < 2 || buf[offset] != ASN1_TAG_SEQUENCE) return -1;
    offset++;
    int seq_len = buf[offset++];
    if (offset + seq_len > buf_len) return -1;

    /* Decode username */
    if (buf[offset] != ASN1_TAG_UTF8STRING) return -1;
    offset++;
    int username_len = buf[offset++];
    if (username_len >= MAX_FIELD_LEN) return -1;
    memcpy(user->username, buf + offset, username_len);
    user->username[username_len] = '\0';
    offset += username_len;

    /* Decode email */
    if (buf[offset] != ASN1_TAG_UTF8STRING) return -1;
    offset++;
    int email_len = buf[offset++];
    if (email_len >= MAX_FIELD_LEN) return -1;
    memcpy(user->email, buf + offset, email_len);
    user->email[email_len] = '\0';
    offset += email_len;

    /* Decode telephone */
    if (buf[offset] != ASN1_TAG_UTF8STRING) return -1;
    offset++;
    int telephone_len = buf[offset++];
    if (telephone_len >= MAX_FIELD_LEN) return -1;
    memcpy(user->telephone, buf + offset, telephone_len);
    user->telephone[telephone_len] = '\0';

    return 0;
}

#endif /* USER_INFO_H */
