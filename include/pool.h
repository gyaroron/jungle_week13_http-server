#ifndef POOL_H
#define POOL_H

struct pool;

// 고정 수의 일꾼과 고정 크기 연결 대기열을 만듭니다.
struct pool *pool_create(int workers, int queue_cap, void (*handler)(int fd));

// 연결 번호를 대기열에 넣습니다. 성공하면 풀의 일꾼이 fd를 닫습니다.
// 대기열이 가득 차면 기다리지 않고 -1과 errno=EAGAIN을 돌려줍니다. 이때 fd는 호출자가 닫습니다.
int pool_submit(struct pool *p, int fd);

// 아직 일꾼이 꺼내지 않은 대기 연결을 모두 꺼내 reject(fd)에 넘깁니다. fd는 reject가 닫습니다.
// 종료할 때 부르면, 이미 처리 중인 요청만 끝내고 대기 연결은 기다리지 않고 돌려보냅니다.
// 꺼낸 연결 수를 돌려줍니다.
int pool_reject_queued(struct pool *p, void (*reject)(int fd));

// 새 제출을 막고 남은 연결을 처리한 뒤 모든 일꾼과 자원을 정리합니다.
void pool_shutdown(struct pool *p);

#endif
