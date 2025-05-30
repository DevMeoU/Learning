Dưới đây là giải thích nguyên lý hoạt động và cách **tạo ra các cơ chế đồng bộ hóa** như Mutex, Semaphore, Spinlock, Message Queue trong hệ thống đa nhiệm/RTOS:

---

### **1. Nguyên lý cơ bản**

Các cơ chế này đều dựa trên **atomic operations** (thao tác nguyên tử) và **scheduler** (trình lập lịch) để đảm bảo:

* **Tính nhất quán** : Tránh race condition khi truy cập tài nguyên dùng chung.
* **Hiệu suất** : Tối ưu thời gian chờ và chuyển đổi ngữ cảnh (context switching).

---

### **2. Mutex (Mutual Exclusion)**

#### **Nguyên lý hoạt động** :

* Mỗi Mutex có 1 biến trạng thái (locked/unlocked) và 1 hàng đợi (queue) các task đang chờ.
* Khi task A chiếm Mutex:
  * Nếu unlocked → locked, task A tiếp tục.
  * Nếu locked → task A được đưa vào hàng đợi và chuyển sang trạng thái blocked.

#### **Cách triển khai đơn giản** :

**c**

Copy

Download

```
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
```

---

### **3. Semaphore**

#### **Nguyên lý hoạt động** :

* Dùng biến đếm nguyên (`count`) để quản lý số lượng truy cập vào tài nguyên.
* **Hai thao tác chính** :
* **P() (Wait)** : Giảm `count`. Nếu `count < 0` → task chờ.
* **V() (Signal)** : Tăng `count`. Nếu `count ≤ 0` → đánh thức task chờ.

#### **Triển khai** :

**c**

Copy

Download

```
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
```

---

### **4. Spinlock**

#### **Nguyên lý** :

* Sử dụng **busy-wait** (vòng lặp liên tục) thay vì chuyển task sang trạng thái blocked.
* Phù hợp cho critical section ngắn, tránh chi phí context switching.

#### **Triển khai** :

**c**

Copy

Download

```
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
```

---

### **5. Message Queue**

#### **Nguyên lý** :

* Dùng buffer lưu trữ message và hai semaphore để quản lý slot trống/slot đầy.
* **Producer** : Gửi message vào queue nếu có slot trống.
* **Consumer** : Nhận message nếu queue không rỗng.

#### **Triển khai** :

**c**

Copy

Download

```
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
```

---

### **6. Nguyên lý Atomic Operations**

Các thao tác atomic (test-and-set, compare-and-swap) là nền tảng để triển khai các cơ chế đồng bộ:

* **Test-and-Set** (TAS): Đặt giá trị mới và trả về giá trị cũ (đảm bảo không bị ngắt giữa chừng).
  **c**

  Copy

  Download

  ```
  int atomic_test_and_set(int *lock) {
      int old;
      // Assembly instruction (vd: x86 XCHG)
      __asm__("xchg %0, %1" : "=r"(old) : "m"(*lock), "0"(1));
      return old;
  }
  ```

---

### **7. Sự khác biệt giữa Mutex và Semaphore**

| **Tiêu chí**         | **Mutex**                     | **Semaphore**                     |
| ---------------------------- | ----------------------------------- | --------------------------------------- |
| **Mục đích**        | Bảo vệ tài nguyên dùng chung   | Điều phối truy cập theo số lượng |
| **Sở hữu**           | Task chiếm lock phải giải phóng | Không có sở hữu                     |
| **Loại**              | Binary (0/1)                        | Binary hoặc đếm                      |
| **Priority Inversion** | Hỗ trợ priority inheritance       | Không hỗ trợ                         |

---

### **8. Ví dụ thực tế trong RTOS (FreeRTOS)**

* **Mutex** :
  **c**

  Copy

  Download

```
  SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
  xSemaphoreTake(mutex, portMAX_DELAY); // Lock
  // Critical section
  xSemaphoreGive(mutex); // Unlock
```

* **Semaphore** :
  **c**

  Copy

  Download

```
  SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0); // Max=5, initial=0
  xSemaphoreGive(sem); // Tăng count
  xSemaphoreTake(sem, portMAX_DELAY); // Giảm count
```

---

### **Kết luận**

Các cơ chế đồng bộ được xây dựng dựa trên:

* **Atomic operations** để đảm bảo tính nhất quán.
* **Hàng đợi task** và **scheduler** để quản lý trạng thái blocked/ready.
* **Phần cứng hỗ trợ** (vd: instructions TAS, CAS) và **hệ điều hành** (quản lý ngắt).
