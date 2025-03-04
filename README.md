# Multi-process HTTP/HTTPS Proxy Server

이 프로젝트는 멀티 프로세스 구조를 사용하여 HTTP와 HTTPS(Connect 방식) 요청을 모두 처리할 수 있는 프록시 서버입니다.

## 특징

- **HTTP/HTTPS 지원**: 
  - HTTP 요청은 원격 서버로 직접 전달
  - HTTPS 요청은 CONNECT 방식을 통해 터널링 지원
- **멀티 프로세스 구조**: 
  - `fork()`를 이용해 각 클라이언트 연결을 독립적인 프로세스로 처리
- **매개변수 기반 포트 설정**:
  - 실행 시 포트 번호를 인자로 받아 설정 (기본 포트: 8888)
- **도메인 차단 기능**:
  - `blocked.txt` 파일에 차단할 도메인을 추가하면, 클라이언트가 해당 도메인에 접근 시 403 Forbidden과 함께 차단 페이지를 표시

## 파일 구성

- `main.c`: 프로그램의 진입점 및 서버 초기화 코드
- `proxy.c`, `proxy.h`: 프록시 요청 처리 관련 함수 정의
- `blocked.txt`: 차단할 도메인 목록 파일
- `Makefile`: 빌드 스크립트
- `README.md`: 프로젝트 설명
- `images`: README용 이미지 폴더
- `403message.html` : 차단시 페이지 출력

## 빌드 및 실행

1. **빌드** 
   ```bash
   make
   ```
   
2. **실행**
* 기본 포트(8888)로 실행:
   ```bash
   ./proxy
   ```
* 또는 포트를 지정하여 실행:
   ```bash
   ./proxy 8080
   ```

## 주의점
* 프록시 접속이 되지 않을 경우, 방화벽 설정이나 포트 충돌을 확인
* HTTPS 터널링의 경우, 클라이언트에서는 SSL 핸드셰이크가 발생하므로
프록시 서버가 403 응답을 전송하면 브라우저에서 별도의 오류 메시지가 표시될 수 있다

## 다이어그램

### 시퀀스 다이어그램

![Sequence Diagram](images/sequence_diagram_v3.png)
