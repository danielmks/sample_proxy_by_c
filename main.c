#include "proxy.h"

int main(int argc, char *argv[]) {
    int listen_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int server_port = DEFAULT_SERVER_PORT;

    // 차단할 도메인 목록을 blocked.txt 파일에서 로드
    load_blocked_domains("blocked.txt");

    // 명령행 인자로 포트 번호를 받으면 해당 값을 사용
    if (argc > 1) {
        server_port = atoi(argv[1]);
        if (server_port <= 0) {
            fprintf(stderr, "유효하지 않은 포트 번호: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGCHLD, SIG_IGN);

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
    server_addr.sin_port = htons(server_port);

    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_socket, LISTEN_BACKLOG) < 0) {
        perror("listen");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    printf("프록시 서버가 포트 %d에서 대기 중...\n", server_port);

    while (1) {
        client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        if (fork() == 0) {  // 자식 프로세스
            close(listen_socket);
            handle_client(client_socket);
            exit(0);
        }
        close(client_socket);
    }

    close(listen_socket);
    return 0;
}

