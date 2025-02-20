#include "proxy.h"
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CHILD_PROCESSES 200

// 활성 자식 프로세스 수 (volatile sig_atomic_t를 사용하여 signal handler와 안전하게 공유)
volatile sig_atomic_t active_children = 0;

// SIGCHLD 핸들러: 종료된 자식 프로세스를 waitpid()로 재수집하고 active_children를 감소시킵니다.
void sigchld_handler(int signo) {
    (void) signo;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        active_children--;
    }
    errno = saved_errno;
}

int main(int argc, char *argv[]) {
    int listen_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int server_port = DEFAULT_SERVER_PORT;

    // 차단 도메인 목록 로드
    load_blocked_domains("blocked.txt");

    // 명령행 인자로 포트 번호 설정 (없으면 기본값 사용)
    if (argc > 1) {
        server_port = atoi(argv[1]);
        if (server_port <= 0) {
            fprintf(stderr, "유효하지 않은 포트 번호: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    // SIGCHLD 핸들러 설정 (좀비 프로세스 방지 및 active_children 업데이트)
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // 시스템 호출 자동 재시작
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // 서버 소켓 생성
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

    // 서버 주소 설정 및 바인딩
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
        // 최대 자식 프로세스 수를 초과하면 잠시 대기
        while (active_children >= MAX_CHILD_PROCESSES) {
            printf("최대 자식 프로세스 수(%d) 초과 - 대기 중...\n", MAX_CHILD_PROCESSES);
            sleep(1);
        }

        // 새로운 클라이언트 연결 수신
        client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        // fork()를 통해 자식 프로세스 생성
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_socket);
            continue;
        } else if (pid == 0) {
            // 자식 프로세스: 부모와는 다른 프로세스이므로 listen_socket 닫고 handle_client() 호출
            close(listen_socket);
            handle_client(client_socket);
            exit(0);
        } else {
            // 부모 프로세스: 자식 프로세스 수 증가 및 클라이언트 소켓 닫기
            active_children++;
            close(client_socket);
        }
    }

    close(listen_socket);
    return 0;
}

