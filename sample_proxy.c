#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

void handle_client(int client_socket) {
    int remote_socket;
    struct sockaddr_in remote_addr;
    struct hostent *he;
    char buffer[4096];
    int n;

    // 1. 클라이언트로부터 초기 요청 메시지를 읽음
    n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_socket);
        return;
    }
    buffer[n] = '\0'; // 문자열의 끝을 표시

    // 2. HTTP 요청 메시지에서 "Host:" 헤더 파싱
    char *host_header = strstr(buffer, "Host:");
    if (host_header == NULL) {
        fprintf(stderr, "Host header not found\n");
        close(client_socket);
        return;
    }
    host_header += 5;  // "Host:" 부분 건너뜀

    // 앞뒤 공백 제거
    while (*host_header == ' ' || *host_header == '\t')
        host_header++;

    char host[256];
    int i = 0;
    while (*host_header != '\r' && *host_header != '\n' && *host_header != '\0' && i < sizeof(host)-1) {
        host[i++] = *host_header++;
    }
    host[i] = '\0';

    printf("요청된 호스트: %s\n", host);

    // 3. 파싱한 호스트 정보를 사용해 원격 서버에 연결
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
    remote_addr.sin_port = htons(80);  // 기본 HTTP 포트 (필요에 따라 다른 포트로 조정 가능)
    memcpy(&remote_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(remote_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        perror("connect");
        close(remote_socket);
        close(client_socket);
        return;
    }

    // 4. 이미 읽은 초기 요청 메시지를 원격 서버로 전달
    if (write(remote_socket, buffer, n) < 0) {
        perror("write to remote");
        close(remote_socket);
        close(client_socket);
        return;
    }

    // 5. 이후 양방향 데이터 중계 (select() 이용)
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
            n = read(client_socket, buffer, sizeof(buffer));
            if (n <= 0) break;  // 연결 종료
            if (write(remote_socket, buffer, n) < 0) {
                perror("write to remote");
                break;
            }
        }

        // 원격 서버 -> 클라이언트
        if (FD_ISSET(remote_socket, &read_fds)) {
            n = read(remote_socket, buffer, sizeof(buffer));
            if (n <= 0) break;  // 연결 종료
            if (write(client_socket, buffer, n) < 0) {
                perror("write to client");
                break;
            }
        }
    }

    close(remote_socket);
    close(client_socket);
}

int main() {
    int listen_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    signal(SIGCHLD, SIG_IGN);  // 좀비 프로세스 방지

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_socket, 10) < 0) {
        perror("listen");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    printf("프록시 서버가 포트 8888에서 대기 중...\n");

    while (1) {
        client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        if (fork() == 0) {
            close(listen_socket);
            handle_client(client_socket);
            exit(0);
        }
        close(client_socket);
    }

    close(listen_socket);
    return 0;
}

