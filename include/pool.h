#ifndef POOL_H
#define POOL_H

struct pool;

// 고정 수의 일꾼과 고정 크기 연결 대기열을 만듭니다.
struct pool *pool_create(int workers, int queue_cap, void (*handler)(int fd));

// 연결 번호를 대기열에 넣습니다. 성공하면 풀의 일꾼이 fd를 닫습니다.
int pool_submit(struct pool *p, int fd);

// 새 제출을 막고 남은 연결을 처리한 뒤 모든 일꾼과 자원을 정리합니다.
void pool_shutdown(struct pool *p);

#endif
