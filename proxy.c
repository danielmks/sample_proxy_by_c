#include "proxy.h"

void handle_client(int client_socket) {
    int remote_socket;
    struct sockaddr_in remote_addr;
    struct hostent *he;
    char buffer[BUFFER_SIZE];
    int n;

    // 클라이언트로부터 초기 요청 메시지를 읽음
    n = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0) {
        close(client_socket);
        return;
    }
    buffer[n] = '\0';

    // HTTP CONNECT 방식(HTTPS)인지 확인
    if (strncmp(buffer, "CONNECT", 7) == 0) {
        // HTTPS 요청인 경우: "CONNECT host:port HTTP/1.1"
        char target[256];
        int port = DEFAULT_HTTPS_PORT;
        char *p = buffer + 8;  // "CONNECT " 이후부터 파싱 시작
        char *end = strstr(p, " ");
        if (end == NULL) {
            close(client_socket);
            return;
        }
        *end = '\0';  // host:port 분리

        // host와 port 파싱 (예: "www.example.com:443")
        char *colon = strchr(p, ':');
        if (colon) {
            *colon = '\0';
            strncpy(target, p, sizeof(target) - 1);
            target[sizeof(target) - 1] = '\0';
            port = atoi(colon + 1);
        } else {
            strncpy(target, p, sizeof(target) - 1);
            target[sizeof(target) - 1] = '\0';
        }
        printf("요청된 호스트 (HTTPS): %s, 포트: %d\n", target, port);

        // 원격 서버 연결
        he = gethostbyname(target);
        if (he == NULL) {
            perror("gethostbyname");
            close(client_socket);
            return;
        }

        remote_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (remote_socket < 0) {
            perror("socket");
            close(client_socket);
            return;
        }

        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port);
        memcpy(&remote_addr.sin_addr, he->h_addr_list[0], he->h_length);

        if (connect(remote_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
            perror("connect");
            close(remote_socket);
            close(client_socket);
            return;
        }

        // 연결 성공 시 클라이언트에 응답 전송
        const char *connection_established = "HTTP/1.1 200 Connection Established\r\n\r\n";
        if (write(client_socket, connection_established, strlen(connection_established)) < 0) {
            perror("write to client");
            close(remote_socket);
            close(client_socket);
            return;
        }
    } else {
        // 일반 HTTP 요청인 경우: Host 헤더 파싱
        char *host_header = strstr(buffer, "Host:");
        if (host_header == NULL) {
            fprintf(stderr, "Host header not found\n");
            close(client_socket);
            return;
        }
        host_header += 5;  // "Host:" 건너뜀

        // 앞뒤 공백 제거
        while (*host_header == ' ' || *host_header == '\t')
            host_header++;

        char host[256];
        size_t i = 0;
        while (*host_header != '\r' && *host_header != '\n' && *host_header != '\0' && i < sizeof(host) - 1) {
            host[i++] = *host_header++;
        }
        host[i] = '\0';

        printf("요청된 호스트 (HTTP): %s\n", host);

        int port = DEFAULT_HTTP_PORT;
        he = gethostbyname(host);
        if (he == NULL) {
            perror("gethostbyname");
            close(client_socket);
            return;
        }

        remote_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (remote_socket < 0) {
            perror("socket");
            close(client_socket);
            return;
        }

        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port);
        memcpy(&remote_addr.sin_addr, he->h_addr_list[0], he->h_length);

        if (connect(remote_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
            perror("connect");
            close(remote_socket);
            close(client_socket);
            return;
        }

        // HTTP 요청 메시지를 원격 서버로 전달
        if (write(remote_socket, buffer, n) < 0) {
            perror("write to remote");
            close(remote_socket);
            close(client_socket);
            return;
        }
    }

    // 양방향 데이터 중계 (select() 사용)
    fd_set read_fds;
    int maxfd = (client_socket > remote_socket ? client_socket : remote_socket) + 1;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);
        FD_SET(remote_socket, &read_fds);

        if (select(maxfd, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // 클라이언트 -> 원격 서버
        if (FD_ISSET(client_socket, &read_fds)) {
            n = read(client_socket, buffer, BUFFER_SIZE);
            if (n <= 0) break;
            if (write(remote_socket, buffer, n) < 0) {
                perror("write to remote");
                break;
            }
        }

        // 원격 서버 -> 클라이언트
        if (FD_ISSET(remote_socket, &read_fds)) {
            n = read(remote_socket, buffer, BUFFER_SIZE);
            if (n <= 0) break;
            if (write(client_socket, buffer, n) < 0) {
                perror("write to client");
                break;
            }
        }
    }

    close(remote_socket);
    close(client_socket);
}

