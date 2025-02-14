#ifndef PROXY_H
#define PROXY_H

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

#define SERVER_PORT         8888       // 프록시 서버가 수신할 포트 (기본)
#define BUFFER_SIZE         4096       // 송수신 버퍼 크기
#define LISTEN_BACKLOG      10         // listen()에서 사용할 대기 큐 길이
#define DEFAULT_HTTP_PORT   80         // HTTP 기본 포트
#define DEFAULT_HTTPS_PORT  443        // HTTPS 기본 포트
#define DEFAULT_SERVER_PORT 8888       // 기본 프록시 서버 포트

void handle_client(int client_socket);

#endif // PROXY_H

