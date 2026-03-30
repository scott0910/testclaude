/**
 * @file    client.c
 * @brief   TCP client that sends ASN.1 DER-encoded user information to the server
 * @author  Scott
 * @date    2026-03-30
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "user_info.h"

#define SERVER_IP "127.0.0.1"
#define PORT      8080

int main(void) {
    int sock_fd;
    struct sockaddr_in server_addr;
    UserInfo user;
    uint8_t der_buf[MAX_DER_LEN];
    int der_len;

    /* Create TCP socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock_fd);
        return 1;
    }

    /* Connect to server */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, PORT);

    /* Prompt user to enter username and email */
    memset(&user, 0, sizeof(user));
    printf("Enter username : ");
    fgets(user.username, sizeof(user.username), stdin);
    user.username[strcspn(user.username, "\n")] = '\0'; /* Remove trailing newline */

    printf("Enter email    : ");
    fgets(user.email, sizeof(user.email), stdin);
    user.email[strcspn(user.email, "\n")] = '\0'; /* Remove trailing newline */

    printf("Enter telephone: ");
    fgets(user.telephone, sizeof(user.telephone), stdin);
    user.telephone[strcspn(user.telephone, "\n")] = '\0'; /* Remove trailing newline */

    /* Encode UserInfo into ASN.1 DER format */
    der_len = user_info_encode(&user, der_buf, sizeof(der_buf));
    if (der_len < 0) {
        fprintf(stderr, "ASN.1 encoding failed\n");
        close(sock_fd);
        return 1;
    }

    printf("Encoded %d bytes in ASN.1 DER format\n", der_len);

    /* Send DER-encoded data to server */
    if (send(sock_fd, der_buf, der_len, 0) < 0) {
        perror("send");
        close(sock_fd);
        return 1;
    }

    printf("User information sent successfully.\n");

    close(sock_fd);

    return 0;
}
