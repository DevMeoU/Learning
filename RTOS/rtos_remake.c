#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h> // for sleep function

typedef struct {
    atomic_int lock;   // 0: unlocked, 1: locked
    TaskQueue wait_queue;
} Mutex;

void mutex_lock(Mutex *m) {
    while (atomic_test_and_set(&m->lock)) { // Atomic operation
        // Chuyển task hiện tại vào wait_queue
        current_task->state = BLOCKED;
        enqueue(m->wait_queue, current_task);
        yield(); // Chuyển quyền điều khiển cho task khác
    }
}

void mutex_unlock(Mutex *m) {
    atomic_clear(&m->lock); // Giải phóng lock
    if (!is_empty(m->wait_queue)) {
        Task *t = dequeue(m->wait_queue);
        t->state = READY;
        schedule(t); // Đánh thức task chờ
    }
}

typedef struct {
    int count;
    TaskQueue queue;
} Semaphore;

void sem_wait(Semaphore *s) {
    atomic_decrement(&s->count);
    if (s->count < 0) {
        current_task->state = BLOCKED;
        enqueue(s->queue, current_task);
        yield();
    }
}

void sem_signal(Semaphore *s) {
    atomic_increment(&s->count);
    if (s->count <= 0) {
        Task *t = dequeue(s->queue);
        t->state = READY;
        schedule(t);
    }
}

typedef struct {
    atomic_int lock;
} Spinlock;

void spinlock_lock(Spinlock *s) {
    while (atomic_test_and_set(&s->lock)) {
        // Busy-wait (không chuyển task)
        __asm__("nop"); // No operation
    }
}

void spinlock_unlock(Spinlock *s) {
    atomic_clear(&s->lock);
}

typedef struct {
    int *buffer;
    int capacity;
    int head, tail;
    Semaphore empty_slots;
    Semaphore full_slots;
    Mutex mutex;
} MessageQueue;

void send(MessageQueue *q, int msg) {
    sem_wait(&q->empty_slots); // Chờ slot trống
    mutex_lock(&q->mutex);
    q->buffer[q->tail] = msg;
    q->tail = (q->tail + 1) % q->capacity;
    mutex_unlock(&q->mutex);
    sem_signal(&q->full_slots); // Tăng số slot đầy
}

int receive(MessageQueue *q) {
    sem_wait(&q->full_slots); // Chờ slot đầy
    mutex_lock(&q->mutex);
    int msg = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    mutex_unlock(&q->mutex);
    sem_signal(&q->empty_slots); // Tăng slot trống
    return msg;
}

int atomic_test_and_set(int *lock) {
    int old;
    // Assembly instruction (vd: x86 XCHG)
    __asm__("xchg %0, %1" : "=r"(old) : "m"(*lock), "0"(1));
    return old;
}