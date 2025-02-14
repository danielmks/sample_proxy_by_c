# Sample Proxy Server By C

## 실행법
```
gcc sample_proxy.c
./a.out [포트번호]
```

프록시 서버가 포트 [포트번호]에서 대기 중... 
* 포트번호 설정하지 않을시 8888 포트로 실행(default)
출력후 서버 가동


## Sequence Diagram
![UML](images/sequence_diagram.png)


## Multi Process 구조
![Multi-Process](images/proxy_구조.png)
