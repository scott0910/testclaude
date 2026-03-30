/**
 * @file    server.c
 * @brief   TCP server that receives and decodes ASN.1 DER-encoded user information
 * @author  Scott
 * @date    2026-03-30
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "user_info.h"

#define PORT 8080

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    uint8_t der_buf[MAX_DER_LEN];
    UserInfo user;
    ssize_t received;

    /* Create TCP socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Allow port reuse to avoid "Address already in use" error */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    /* Bind socket to address and port */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    /* Listen for incoming connections */
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server is listening on port %d...\n", PORT);

    /* Accept a client connection */
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    printf("Client connected from %s\n", inet_ntoa(client_addr.sin_addr));

    /* Receive ASN.1 DER-encoded data from client */
    memset(der_buf, 0, sizeof(der_buf));
    received = recv(client_fd, der_buf, sizeof(der_buf), 0);
    if (received < 0) {
        perror("recv");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    printf("Received %zd bytes of ASN.1 DER data\n", received);

    /* Decode DER buffer into UserInfo struct */
    memset(&user, 0, sizeof(user));
    if (user_info_decode(der_buf, (int)received, &user) < 0) {
        fprintf(stderr, "ASN.1 decoding failed\n");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    /* Display decoded user information */
    printf("===== User Information Received =====\n");
    printf("Username  : %s\n", user.username);
    printf("Email     : %s\n", user.email);
    printf("Telephone : %s\n", user.telephone);
    printf("=====================================\n");

    close(client_fd);
    close(server_fd);

    return 0;
}
