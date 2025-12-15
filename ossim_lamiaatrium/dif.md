# Tổng hợp các thay đổi HỆ THỐNG so với Source Code Gốc

Tài liệu này tổng hợp các **thay đổi hệ thống/infrastructure** đã được điều chỉnh sẵn - KHÔNG bao gồm các hàm TODO sinh viên cần implement.

---

## 1. Config Changes (`include/os-cfg.h`)

### 1.1 Bật `MM_FIXED_MEMSZ`
```c
// Gốc:
//#define MM_FIXED_MEMSZ

// Hiện tại:
#define MM_FIXED_MEMSZ
```

**Tại sao cần thay đổi này?**
- Khi `MM_FIXED_MEMSZ` được bật, hệ thống sử dụng kích thước bộ nhớ cố định thay vì dynamic.
- Điều này giúp **đơn giản hóa việc debug và testing** vì memory layout luôn consistent.
- Tránh các vấn đề về fragmentation và memory allocation phức tạp trong môi trường học tập.

### 1.2 Bật `MM64`
```c
// Gốc:
//#define MM64 1
#undef MM64

// Hiện tại:
#define MM64 1
//#undef MM64
```

**Tại sao cần thay đổi này?**
- Bật chế độ **5-level paging 64-bit** (PGD → P4D → PUD → PMD → PT).
- Cho phép sinh viên học và implement cơ chế paging hiện đại như trong Linux kernel.
- Hỗ trợ address space lớn hơn (48-bit virtual address).

---

## 2. Output Synchronization - Giải quyết Race Condition

### Vấn đề gốc:
Trong source code gốc, **output bị xáo trộn** (interleaved) do nhiều thread (timer, CPU, loader) cùng gọi `printf()` mà không có đồng bộ. Ví dụ output có thể bị:
```
Time slot   1
	CPU 0: DiTime slot   2spatched process  0
```
Thay vì:
```
Time slot   1
	CPU 0: Dispatched process  0
Time slot   2
```

### 2.1 Thêm global mutex (`include/timer.h` & `src/timer.c`)

**timer.h:**
```c
extern pthread_mutex_t output_lock;
```

**timer.c:**
```c
pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
```

**Tại sao cần?**
- Tạo **một mutex duy nhất** để tất cả các thread sử dụng khi in output.
- Đặt trong `timer.c` vì timer là "trung tâm điều phối" của hệ thống.
- Khai báo `extern` trong header để các file khác có thể sử dụng.

### 2.2 Thay đổi logic timer_routine (`src/timer.c`)

```c
// Gốc: In "Time slot" bên trong loop, SAU khi xử lý
while (!timer_stop) {
    printf("Time slot %3llu\n", current_time());
    // ... xử lý ...
    if (fsh == event) break;
}

// Hiện tại: In "Time slot" TRƯỚC loop và TRƯỚC khi signal devices
pthread_mutex_lock(&output_lock);
printf("Time slot %3llu\n", current_time());
fflush(stdout);
pthread_mutex_unlock(&output_lock);

while (!timer_stop) {
    // ... xử lý ...
    if (fsh == event) break;
    
    // In Time slot TRƯỚC khi signal devices tiếp theo
    pthread_mutex_lock(&output_lock);
    printf("Time slot %3llu\n", current_time());
    fflush(stdout);
    pthread_mutex_unlock(&output_lock);
    // ... signal devices ...
}
```

**Tại sao cần thay đổi này?**

1. **Đảm bảo thứ tự output đúng logic:**
   - "Time slot X" phải in **TRƯỚC** các action xảy ra trong time slot đó.
   - Gốc: Timer in sau khi devices đã chạy → output bị lệch.

2. **`fflush(stdout)` bắt buộc:**
   - stdout thường được buffer, nếu không flush thì output có thể bị delay.
   - Đảm bảo output xuất hiện ngay lập tức theo đúng thứ tự.

3. **Mutex lock/unlock:**
   - Ngăn các thread khác chen vào giữa khi đang in.
   - Đảm bảo mỗi dòng output được in hoàn chỉnh.

---

## 3. CPU & Loader Output Sync (`src/os.c`)

### 3.1 Thêm output_lock cho tất cả printf trong cpu_routine

```c
// Mỗi printf đều được wrap:
pthread_mutex_lock(&output_lock);
printf("\tCPU %d: Processed %2d has finished\n", id, proc->pid);
fflush(stdout);
pthread_mutex_unlock(&output_lock);
```

**Áp dụng cho tất cả output:**
- "Processed X has finished"
- "Put process X to run queue"  
- "CPU X stopped"
- "Dispatched process X"
- "Loaded a process..."

**Tại sao cần?**
- **Consistency với timer:** Tất cả output đều cần mutex để không bị xáo trộn.
- **Atomic output:** Mỗi message được in hoàn chỉnh, không bị thread khác chen vào.

### 3.2 Thay đổi thứ tự load process trong ld_routine

```c
// Gốc: Load TRƯỚC, chờ SAU
struct pcb_t * proc = load(ld_processes.path[i]);  // Load ngay
...
while (current_time() < ld_processes.start_time[i]) {
    next_slot(timer_id);  // Chờ sau
}

// Hiện tại: Chờ TRƯỚC, load SAU
while (current_time() < ld_processes.start_time[i]) {
    next_slot(timer_id);  // Chờ đến đúng time slot
}
struct pcb_t * proc = load(ld_processes.path[i]);  // Rồi mới load
```

**Tại sao cần thay đổi này?**

1. **Đúng ngữ nghĩa của "start_time":**
   - Process chỉ nên được load **tại** thời điểm start_time, không phải trước đó.
   - Gốc: Load sớm có thể gây ra side effects không mong muốn.

2. **Output chính xác:**
   - Message "Loaded a process" phải xuất hiện **trong** time slot tương ứng.
   - Nếu load trước, message sẽ xuất hiện ở time slot sai.

3. **Resource management:**
   - Không allocate resources (memory cho PCB, page tables) sớm hơn cần thiết.

### 3.3 Kiểm tra mm trước khi khởi tạo

```c
// Gốc: Luôn malloc
krnl->mm = malloc(sizeof(struct mm_struct));
init_mm(krnl->mm, proc);

// Hiện tại: Chỉ malloc nếu chưa có
if (krnl->mm == NULL) {
    krnl->mm = malloc(sizeof(struct mm_struct));
    init_mm(krnl->mm, proc);
}
```

**Tại sao cần?**
- **Tránh memory leak:** Nếu mm đã được init, malloc lại sẽ leak memory cũ.
- **Shared mm structure:** Trong một số test case, nhiều process có thể share cùng mm.
- **Idempotent operation:** Code an toàn khi được gọi nhiều lần.

---

## 4. System Memory Safety (`src/sys_mem.c`)

### 4.1 Stack allocation thay vì heap

```c
// Gốc: malloc trên heap
struct pcb_t *caller = malloc(sizeof(struct pcb_t));
// BUG: Không có free() → memory leak mỗi lần syscall

// Hiện tại: Stack allocation
struct pcb_t caller_stack;
struct pcb_t *caller = &caller_stack;
memset(caller, 0, sizeof(struct pcb_t));
caller->krnl = krnl;
caller->pid = pid;
```

**Tại sao cần thay đổi này?**

1. **Memory leak:**
   - Gốc: Mỗi syscall malloc một PCB nhưng không free → leak tích lũy.
   - Stack: Tự động giải phóng khi function return.

2. **Performance:**
   - Stack allocation nhanh hơn malloc (chỉ cần adjust stack pointer).
   - Syscall được gọi rất thường xuyên → performance quan trọng.

3. **Thread safety:**
   - Stack là thread-local → không cần lo về race condition.
   - Heap allocation có thể bị race nếu nhiều thread cùng syscall.

### 4.2 Sử dụng krnl trực tiếp

```c
// Gốc:
MEMPHY_read(caller->krnl->mram, ...);

// Hiện tại:
MEMPHY_read(krnl->mram, ...);
```

**Tại sao?**
- `krnl` đã được pass vào function, sử dụng trực tiếp thay vì đi qua `caller`.
- Tránh potential NULL dereference nếu caller->krnl không được set đúng.

---

## 5. Memory Library Thread Safety (`src/libmem.c`)

### 5.1 Include timer.h
```c
#include "timer.h"  // Để sử dụng output_lock
```

**Tại sao?** Cần access `output_lock` mutex đã định nghĩa trong timer module.

### 5.2 IODUMP với output_lock

```c
#ifdef IODUMP
pthread_mutex_lock(&output_lock);
printf("liballoc:%d\n", __LINE__);
// ... debug output ...
fflush(stdout);
pthread_mutex_unlock(&output_lock);
#endif
```

**Tại sao?**
- IODUMP là debug output, cũng cần được đồng bộ.
- Nếu không lock, debug output sẽ bị xáo trộn với output chính.

### 5.3 Validation checks

```c
if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
}
```

**Tại sao?**
- **Defensive programming:** Kiểm tra NULL trước khi dereference.
- **Graceful failure:** Return error code thay vì crash (segfault).
- **Debug friendly:** Dễ trace lỗi hơn khi có explicit check.

---

## 6. MM64 Infrastructure (`src/mm64.c`)

### 6.1 Page-aligned allocation

```c
static void* alloc_page_table(void) {
    void *ptr;
    if (posix_memalign(&ptr, PAGING64_PAGESZ, PAGING64_PAGESZ) != 0) {
        return NULL;
    }
    return ptr;
}
```

**Tại sao cần page alignment?**

1. **Hardware requirement:**
   - Page tables trong x86-64 **BẮT BUỘC** phải aligned 4KB (4096 bytes).
   - CPU sử dụng 12 bits thấp của địa chỉ page table cho flags.

2. **Bit manipulation:**
   - Nếu không aligned, các flags (Present, R/W, etc.) sẽ corrupt địa chỉ.
   - `malloc()` chỉ đảm bảo align 8/16 bytes, không đủ.

### 6.2 Present bit definition

```c
#define PT_PRESENT_BIT 0x1ULL
#define PT_ADDR_MASK   (~0xFFFULL)
```

**Tại sao cần?**

1. **64-bit address space:**
   - Gốc dùng `PAGING_PTE_PRESENT_MASK` (bit 31) → corrupt địa chỉ 64-bit.
   - Dùng bit 0 như x86-64 hardware thực sự làm.

2. **Address extraction:**
   - `PT_ADDR_MASK` clear 12 bits thấp để lấy địa chỉ page table tiếp theo.
   - Đúng với cách hardware xử lý.

---

## Tóm tắt: Tại sao các thay đổi này quan trọng?

| Thay đổi | Vấn đề gốc | Giải pháp |
|----------|-----------|-----------|
| **output_lock mutex** | Output bị xáo trộn giữa các thread | Mutex đồng bộ tất cả printf |
| **fflush(stdout)** | Output bị buffer, hiển thị sai thứ tự | Force flush ngay sau printf |
| **Time slot print order** | Time slot in sau actions | In trước khi signal devices |
| **Load order** | Process load trước start_time | Chờ đến đúng time slot rồi mới load |
| **Stack allocation** | Memory leak trong syscall | Dùng stack, tự động free |
| **NULL checks** | Segfault khi NULL pointer | Graceful error handling |
| **Page alignment** | Page table bị corrupt | posix_memalign 4KB |
| **PT_PRESENT_BIT** | 64-bit address bị corrupt | Dùng bit 0 thay vì bit 31 |

---

## Lưu ý khi làm bài

1. **Không sửa các thay đổi hệ thống này** - chúng là infrastructure cần thiết.
2. **Sử dụng output_lock** nếu thêm printf mới trong code của bạn.
3. **Luôn fflush(stdout)** sau printf để đảm bảo output đúng thứ tự.
4. **Kiểm tra NULL** trước khi dereference pointer.
