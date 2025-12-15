# PHÂN TÍCH CHI TIẾT TỪNG TIME SLOT - OS SIMULATION

---

## 1. Giải thích cấu trúc Input

### 1.1 File cấu hình Test (os_config)

**Format dòng 1:** `timeslot num_cpu num_process`
- `timeslot`: Đơn vị thời gian cho mỗi quantum
- `num_cpu`: Số lượng CPU
- `num_process`: Số lượng process

**Format dòng 2 (nếu có paging):** `RAM_SIZE SWAP_SIZE 0 0 0`

**Format các dòng tiếp theo:** `arrival_time process_name priority`

### 1.2 Mô tả các Process

**Process đơn giản (chỉ calc):**
- **p1s**: code_size=1, 10 lệnh calc
- **p2s**: code_size=20, 12 lệnh calc
- **p3s**: code_size=7, 11 lệnh calc
- **s0**: code_size=12, 15 lệnh calc
- **s1**: code_size=20, 7 lệnh calc
- **s2**: code_size=20, 12 lệnh calc
- **s3**: code_size=7, 11 lệnh calc

**Process có memory operations:**
- **p0s**: 14 lệnh (alloc, free, write, read)
- **m0s**: 6 lệnh (alloc, free, write)
- **m1s**: 6 lệnh (alloc, free)

---

## 2. TEST: sched

### Cấu hình
```
4 2 3        -> quantum=4, 2 CPUs, 3 processes
0 p1s 1      -> t=0: load p1s, priority=1 (thấp)
1 p2s 0      -> t=1: load p2s, priority=0 (cao)
2 p3s 0      -> t=2: load p3s, priority=0 (cao)
```

### Phân tích từng Time Slot

- **Time slot 0:**
  + Hệ điều hành khởi động loader routine (ld_routine)
  + Loader load process p1s từ file input/proc/p1s
  + Process được gán PID=1, Priority=1
  + P1 được đưa vào ready queue của priority level 1
  + Cả CPU 0 và CPU 1 đều idle

- **Time slot 1:**
  + CPU 1 dispatch process P1 (PID=1) từ ready queue priority 1
  + P1 bắt đầu thực thi lệnh calc đầu tiên
  + Loader load process p2s, gán PID=2, Priority=0
  + P2 được đưa vào ready queue priority 0 (ưu tiên cao hơn P1)
  + CPU 0 vẫn idle (chưa có process nào ở queue priority 0 sẵn sàng)

- **Time slot 2:**
  + CPU 0 dispatch process P2 (PID=2) từ ready queue priority 0
  + P2 bắt đầu thực thi
  + CPU 1 tiếp tục chạy P1
  + Loader load process p3s, gán PID=3, Priority=0
  + P3 được đưa vào ready queue priority 0

- **Time slot 3:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P1 (calc)
  + Không có sự kiện đặc biệt

- **Time slot 4:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P1 (calc)
  + P1 đã chạy được 4 slot (hết quantum)
  + Không có sự kiện đặc biệt (quantum check ở đầu slot tiếp theo)

- **Time slot 5:**
  + CPU 1: P1 hết quantum (đã chạy 4 slot), đưa P1 vào run queue priority 1
  + CPU 1: Scheduler tìm process trong queue priority 0, dispatch P3 (PID=3)
  + P3 bắt đầu thực thi trên CPU 1
  + CPU 0: P2 vẫn còn trong quantum, tiếp tục chạy
  + **Lưu ý**: P1 bị preempt vì có P3 (priority 0) trong queue

- **Time slot 6:**
  + CPU 0: P2 hết quantum (đã chạy 4 slot: 2,3,4,5), đưa P2 vào run queue priority 0
  + CPU 0: Re-dispatch P2 ngay (vì P2 vẫn có priority cao nhất trong queue)
  + CPU 1: Tiếp tục chạy P3 (calc)

- **Time slot 7:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P3 (calc)

- **Time slot 8:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P3 (calc)

- **Time slot 9:**
  + CPU 1: P3 hết quantum, đưa P3 vào run queue priority 0
  + CPU 1: Re-dispatch P3 (Round-robin trong queue priority 0)
  + CPU 0: Tiếp tục chạy P2

- **Time slot 10:**
  + CPU 0: P2 hết quantum, đưa P2 vào run queue priority 0
  + CPU 0: Re-dispatch P2
  + CPU 1: Tiếp tục chạy P3

- **Time slot 11:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P3 (calc)

- **Time slot 12:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P3 (calc)

- **Time slot 13:**
  + CPU 1: P3 hết quantum, đưa P3 vào run queue
  + CPU 1: Re-dispatch P3
  + CPU 0: Tiếp tục chạy P2

- **Time slot 14:**
  + CPU 0: **P2 HOÀN THÀNH** (đã thực thi đủ 12 lệnh calc)
  + CPU 0: Scheduler tìm process tiếp theo, dispatch P1 từ queue priority 1
  + CPU 1: Tiếp tục chạy P3

- **Time slot 15:**
  + CPU 0 tiếp tục chạy P1 (calc)
  + CPU 1 tiếp tục chạy P3 (calc)

- **Time slot 16:**
  + CPU 1: **P3 HOÀN THÀNH** (đã thực thi đủ 11 lệnh calc)
  + CPU 1: Không còn process nào trong ready queue -> **CPU 1 STOPPED**
  + CPU 0: Tiếp tục chạy P1

- **Time slot 17:**
  + CPU 0 tiếp tục chạy P1 (calc)
  + CPU 1 đã dừng

- **Time slot 18:**
  + CPU 0: P1 hết quantum, đưa P1 vào run queue
  + CPU 0: Re-dispatch P1 (không còn process nào khác)

- **Time slot 19:**
  + CPU 0 tiếp tục chạy P1 (calc)

- **Time slot 20:**
  + CPU 0: **P1 HOÀN THÀNH** (đã thực thi đủ 10 lệnh calc)
  + CPU 0: Không còn process nào -> **CPU 0 STOPPED**
  + **HỆ THỐNG KẾT THÚC**

### Tổng kết sched
- Tổng thời gian: 20 time slots
- P1 (priority 1): Load t=0, Finish t=20, Turnaround=20
- P2 (priority 0): Load t=1, Finish t=14, Turnaround=13
- P3 (priority 0): Load t=2, Finish t=16, Turnaround=14

---

## 3. TEST: sched_0

### Cấu hình
```
2 1 2        -> quantum=2, 1 CPU, 2 processes
0 s0 4       -> t=0: load s0, priority=4 (thấp)
4 s1 0       -> t=4: load s1, priority=0 (cao)
```

### Phân tích từng Time Slot

- **Time slot 0:**
  + Hệ điều hành khởi động loader routine
  + Loader load process s0 từ file input/proc/s0
  + Process được gán PID=1, Priority=4
  + P1 được đưa vào ready queue của priority level 4
  + CPU 0 idle

- **Time slot 1:**
  + CPU 0 dispatch process P1 (PID=1) từ ready queue
  + P1 bắt đầu thực thi lệnh calc đầu tiên

- **Time slot 2:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 2)
  + P1 đã chạy được 2 slot (hết quantum)

- **Time slot 3:**
  + CPU 0: P1 hết quantum, đưa P1 vào run queue priority 4
  + CPU 0: Re-dispatch P1 (không có process nào khác)
  + P1 tiếp tục thực thi (calc thứ 3)

- **Time slot 4:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 4)
  + Loader load process s1, gán PID=2, Priority=0
  + P2 được đưa vào ready queue priority 0 (CAO HƠN P1)
  + P1 đã chạy được 2 slot trong quantum hiện tại

- **Time slot 5:**
  + CPU 0: P1 hết quantum
  + CPU 0: **PREEMPTION!** Đưa P1 vào run queue priority 4
  + CPU 0: Scheduler kiểm tra queue priority 0, thấy P2
  + CPU 0: Dispatch P2 (priority 0 > priority 4)
  + P2 bắt đầu thực thi (calc thứ 1)
  + **P1 bị chiếm CPU vì P2 có priority cao hơn!**

- **Time slot 6:**
  + CPU 0 tiếp tục chạy P2 (calc thứ 2)

- **Time slot 7:**
  + CPU 0: P2 hết quantum, đưa P2 vào run queue priority 0
  + CPU 0: Re-dispatch P2 (priority 0 vẫn cao hơn P1 ở priority 4)
  + P2 tiếp tục thực thi (calc thứ 3)

- **Time slot 8:**
  + CPU 0 tiếp tục chạy P2 (calc thứ 4)

- **Time slot 9:**
  + CPU 0: P2 hết quantum, đưa P2 vào run queue
  + CPU 0: Re-dispatch P2
  + P2 tiếp tục (calc thứ 5)

- **Time slot 10:**
  + CPU 0 tiếp tục chạy P2 (calc thứ 6)

- **Time slot 11:**
  + CPU 0: P2 hết quantum, đưa P2 vào run queue
  + CPU 0: Re-dispatch P2
  + P2 tiếp tục (calc thứ 7 - lệnh cuối)

- **Time slot 12:**
  + CPU 0: **P2 HOÀN THÀNH** (đã thực thi đủ 7 lệnh calc)
  + CPU 0: Scheduler tìm process tiếp theo
  + CPU 0: Dispatch P1 từ queue priority 4
  + P1 tiếp tục từ chỗ bị preempt (calc thứ 5)

- **Time slot 13:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 6)

- **Time slot 14:**
  + CPU 0: P1 hết quantum, đưa P1 vào run queue
  + CPU 0: Re-dispatch P1
  + P1 tiếp tục (calc thứ 7)

- **Time slot 15:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 8)

- **Time slot 16:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc thứ 9)

- **Time slot 17:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 10)

- **Time slot 18:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc thứ 11)

- **Time slot 19:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 12)

- **Time slot 20:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc thứ 13)

- **Time slot 21:**
  + CPU 0 tiếp tục chạy P1 (calc thứ 14)

- **Time slot 22:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc thứ 15 - lệnh cuối)

- **Time slot 23:**
  + CPU 0: **P1 HOÀN THÀNH** (đã thực thi đủ 15 lệnh calc)
  + CPU 0: Không còn process nào -> **CPU 0 STOPPED**
  + **HỆ THỐNG KẾT THÚC**

### Tổng kết sched_0
- Tổng thời gian: 23 time slots
- P1 (priority 4): Load t=0, Finish t=23, Turnaround=23, Waiting=8 (bị P2 chiếm)
- P2 (priority 0): Load t=4, Finish t=12, Turnaround=8, Waiting=0
- **Hiện tượng**: Priority Preemption - P1 bị đẩy ra khi P2 (priority cao hơn) đến

---

## 4. TEST: sched_1

### Cấu hình
```
2 1 4        -> quantum=2, 1 CPU, 4 processes
0 s0 4       -> t=0: load s0, priority=4 (thấp nhất)
4 s1 0       -> t=4: load s1, priority=0
6 s2 0       -> t=6: load s2, priority=0
7 s3 0       -> t=7: load s3, priority=0
```

### Phân tích từng Time Slot

- **Time slot 0:**
  + Hệ điều hành khởi động loader routine
  + Loader load process s0, gán PID=1, Priority=4
  + P1 được đưa vào ready queue priority 4
  + CPU 0 idle

- **Time slot 1:**
  + CPU 0 dispatch P1 từ ready queue
  + P1 bắt đầu thực thi (calc #1)

- **Time slot 2:**
  + CPU 0 tiếp tục chạy P1 (calc #2)

- **Time slot 3:**
  + CPU 0: P1 hết quantum, put to queue, re-dispatch P1
  + P1 tiếp tục (calc #3)

- **Time slot 4:**
  + CPU 0 tiếp tục chạy P1 (calc #4)
  + Loader load process s1, gán PID=2, Priority=0
  + P2 vào ready queue priority 0

- **Time slot 5:**
  + CPU 0: P1 hết quantum
  + CPU 0: **PREEMPTION!** Put P1 to queue priority 4
  + CPU 0: Dispatch P2 (priority 0 > priority 4)
  + P2 bắt đầu thực thi (calc #1)

- **Time slot 6:**
  + CPU 0 tiếp tục chạy P2 (calc #2)
  + Loader load process s2, gán PID=3, Priority=0
  + P3 vào ready queue priority 0

- **Time slot 7:**
  + CPU 0: P2 hết quantum, put P2 to queue priority 0
  + CPU 0: Dispatch P3 (Round-robin trong priority 0: P3 đứng sau P2 trong queue)
  + P3 bắt đầu thực thi (calc #1)
  + Loader load process s3, gán PID=4, Priority=0
  + P4 vào ready queue priority 0
  + **Queue priority 0**: [P2, P4]

- **Time slot 8:**
  + CPU 0 tiếp tục chạy P3 (calc #2)

- **Time slot 9:**
  + CPU 0: P3 hết quantum, put P3 to queue
  + CPU 0: Dispatch P2 (Round-robin: P2 đứng đầu queue)
  + P2 tiếp tục (calc #3)
  + **Queue priority 0**: [P4, P3]

- **Time slot 10:**
  + CPU 0 tiếp tục chạy P2 (calc #4)

- **Time slot 11:**
  + CPU 0: P2 hết quantum, put P2 to queue
  + CPU 0: Dispatch P4 (Round-robin)
  + P4 bắt đầu thực thi (calc #1)
  + **Queue priority 0**: [P3, P2]

- **Time slot 12:**
  + CPU 0 tiếp tục chạy P4 (calc #2)

- **Time slot 13:**
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P3
  + P3 tiếp tục (calc #3)
  + **Queue priority 0**: [P2, P4]

- **Time slot 14:**
  + CPU 0 tiếp tục chạy P3 (calc #4)

- **Time slot 15:**
  + CPU 0: P3 hết quantum, put P3 to queue
  + CPU 0: Dispatch P2
  + P2 tiếp tục (calc #5)
  + **Queue priority 0**: [P4, P3]

- **Time slot 16:**
  + CPU 0 tiếp tục chạy P2 (calc #6)

- **Time slot 17:**
  + CPU 0: P2 hết quantum, put P2 to queue
  + CPU 0: Dispatch P4
  + P4 tiếp tục (calc #3)

- **Time slot 18:**
  + CPU 0 tiếp tục chạy P4 (calc #4)

- **Time slot 19:**
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P3
  + P3 tiếp tục (calc #5)

- **Time slot 20:**
  + CPU 0 tiếp tục chạy P3 (calc #6)

- **Time slot 21:**
  + CPU 0: P3 hết quantum, put P3 to queue
  + CPU 0: Dispatch P2
  + P2 tiếp tục (calc #7 - lệnh cuối)

- **Time slot 22:**
  + CPU 0: **P2 HOÀN THÀNH** (7 lệnh calc)
  + CPU 0: Dispatch P4
  + P4 tiếp tục (calc #5)

- **Time slot 23:**
  + CPU 0 tiếp tục chạy P4 (calc #6)

- **Time slot 24:**
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P3
  + P3 tiếp tục (calc #7)

- **Time slot 25:**
  + CPU 0 tiếp tục chạy P3 (calc #8)

- **Time slot 26:**
  + CPU 0: P3 hết quantum, put P3 to queue
  + CPU 0: Dispatch P4
  + P4 tiếp tục (calc #7)

- **Time slot 27:**
  + CPU 0 tiếp tục chạy P4 (calc #8)

- **Time slot 28:**
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P3
  + P3 tiếp tục (calc #9)

- **Time slot 29:**
  + CPU 0 tiếp tục chạy P3 (calc #10)

- **Time slot 30:**
  + CPU 0: P3 hết quantum, put P3 to queue
  + CPU 0: Dispatch P4
  + P4 tiếp tục (calc #9)

- **Time slot 31:**
  + CPU 0 tiếp tục chạy P4 (calc #10)

- **Time slot 32:**
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P3
  + P3 tiếp tục (calc #11)

- **Time slot 33:**
  + CPU 0 tiếp tục chạy P3 (calc #12 - lệnh cuối)

- **Time slot 34:**
  + CPU 0: **P3 HOÀN THÀNH** (12 lệnh calc)
  + CPU 0: Dispatch P4
  + P4 tiếp tục (calc #11 - lệnh cuối)

- **Time slot 35:**
  + CPU 0: **P4 HOÀN THÀNH** (11 lệnh calc)
  + CPU 0: Queue priority 0 trống, chuyển sang priority 4
  + CPU 0: Dispatch P1
  + P1 tiếp tục từ chỗ bị preempt (calc #5)

- **Time slot 36:**
  + CPU 0 tiếp tục chạy P1 (calc #6)

- **Time slot 37:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc #7)

- **Time slot 38:**
  + CPU 0 tiếp tục chạy P1 (calc #8)

- **Time slot 39:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc #9)

- **Time slot 40:**
  + CPU 0 tiếp tục chạy P1 (calc #10)

- **Time slot 41:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc #11)

- **Time slot 42:**
  + CPU 0 tiếp tục chạy P1 (calc #12)

- **Time slot 43:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc #13)

- **Time slot 44:**
  + CPU 0 tiếp tục chạy P1 (calc #14)

- **Time slot 45:**
  + CPU 0: P1 hết quantum, re-dispatch P1
  + P1 tiếp tục (calc #15 - lệnh cuối)

- **Time slot 46:**
  + CPU 0: **P1 HOÀN THÀNH** (15 lệnh calc)
  + CPU 0: Không còn process nào -> **CPU 0 STOPPED**
  + **HỆ THỐNG KẾT THÚC**

### Tổng kết sched_1
- Tổng thời gian: 46 time slots
- P1 (priority 4): Load t=0, Finish t=46, Turnaround=46, **STARVATION** - chờ 31 slots
- P2 (priority 0): Load t=4, Finish t=22, Turnaround=18
- P3 (priority 0): Load t=6, Finish t=34, Turnaround=28
- P4 (priority 0): Load t=7, Finish t=35, Turnaround=28
- **Hiện tượng**: Starvation - P1 phải đợi TẤT CẢ process priority 0 hoàn thành

---

## 5. TEST: os_0_mlq_paging

### Cấu hình
```
6 2 4                    -> quantum=6, 2 CPUs, 4 processes
1048576 16777216 0 0 0   -> RAM=1MB, SWAP=16MB (có paging)
0 p0s 0                  -> t=0: p0s, priority=0 (có memory operations)
2 p1s 15                 -> t=2: p1s, priority=15 (calc)
4 p1s 0                  -> t=4: p1s, priority=0 (calc)
6 p1s 0                  -> t=6: p1s, priority=0 (calc)
```

### Phân tích từng Time Slot

- **Time slot 0:**
  + Hệ điều hành khởi động loader routine
  + Loader load process p0s, gán PID=1, Priority=0
  + P1 được đưa vào ready queue priority 0
  + CPU 0 và CPU 1 idle

- **Time slot 1:**
  + CPU 1 dispatch P1 từ ready queue
  + P1 bắt đầu thực thi (lệnh calc đầu tiên)
  + CPU 0 idle

- **Time slot 2:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **alloc 300 0** (cấp phát 300 bytes vào region 0)
  + Hệ thống gọi liballoc, cấp phát bộ nhớ ảo
  + Page table được cập nhật (print_pgtbl)
  + Loader load process p1s, gán PID=2, Priority=15
  + P2 vào ready queue priority 15

- **Time slot 3:**
  + CPU 0 dispatch P2 từ ready queue priority 15
  + P2 bắt đầu thực thi (calc)
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **alloc 300 4** (cấp phát 300 bytes vào region 4)
  + liballoc được gọi, page table cập nhật

- **Time slot 4:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **free 0** (giải phóng region 0)
  + libfree được gọi, page table cập nhật
  + Loader load process p1s, gán PID=3, Priority=0
  + P3 vào ready queue priority 0 (CAO HƠN P2)
  + CPU 0 tiếp tục chạy P2

- **Time slot 5:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **alloc 100 1** (cấp phát 100 bytes vào region 1)
  + liballoc được gọi
  + CPU 0 tiếp tục chạy P2 (calc)

- **Time slot 6:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **write 100 1 20** (ghi giá trị 100 vào region 1 offset 20)
  + libwrite được gọi, page table cập nhật
  + Loader load process p1s, gán PID=4, Priority=0
  + P4 vào ready queue priority 0
  + CPU 0 tiếp tục chạy P2

- **Time slot 7:**
  + CPU 1: P1 hết quantum (đã chạy 6 slot), put P1 to queue priority 0
  + CPU 1: Dispatch P3 từ queue priority 0
  + P3 bắt đầu thực thi (calc #1)
  + CPU 0 tiếp tục chạy P2
  + **Queue priority 0**: [P4, P1]

- **Time slot 8:**
  + CPU 1 tiếp tục chạy P3 (calc #2)
  + CPU 0 tiếp tục chạy P2 (calc)

- **Time slot 9:**
  + CPU 0: P2 hết quantum, put P2 to queue priority 15
  + CPU 0: Scheduler kiểm tra - có P4 ở priority 0, dispatch P4
  + P4 bắt đầu thực thi (calc #1)
  + CPU 1 tiếp tục chạy P3

- **Time slot 10:**
  + CPU 0 tiếp tục chạy P4 (calc #2)
  + CPU 1 tiếp tục chạy P3 (calc #3)

- **Time slot 11:**
  + CPU 0 tiếp tục chạy P4 (calc #3)
  + CPU 1 tiếp tục chạy P3 (calc #4)

- **Time slot 12:**
  + CPU 0 tiếp tục chạy P4 (calc #4)
  + CPU 1 tiếp tục chạy P3 (calc #5)

- **Time slot 13:**
  + CPU 1: P3 hết quantum, put P3 to queue priority 0
  + CPU 1: Dispatch P1 (Round-robin trong queue priority 0)
  + P1 tiếp tục từ chỗ dừng
  + P1 thực thi lệnh: **read 1 20 20** (đọc từ region 1, offset 20, size 20)
  + libread được gọi
  + CPU 0 tiếp tục chạy P4

- **Time slot 14:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **write 102 2 20** (ghi 102 vào region 2 offset 20)
  + libwrite được gọi, page table cập nhật
  + CPU 0 tiếp tục chạy P4 (calc #5)

- **Time slot 15:**
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P3
  + P3 tiếp tục (calc #6)
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **read 2 20 20**
  + libread được gọi

- **Time slot 16:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **write 103 3 20**
  + libwrite được gọi
  + CPU 0 tiếp tục chạy P3 (calc #7)

- **Time slot 17:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **read 3 20 20**
  + libread được gọi
  + CPU 0 tiếp tục chạy P3 (calc #8)

- **Time slot 18:**
  + CPU 1 tiếp tục chạy P1
  + P1 thực thi lệnh: **calc** (lệnh #12)
  + CPU 0 tiếp tục chạy P3 (calc #9)

- **Time slot 19:**
  + CPU 1: P1 hết quantum, put P1 to queue
  + CPU 1: Dispatch P4
  + P4 tiếp tục (calc #6)
  + CPU 0: **P3 HOÀN THÀNH** (10 lệnh calc)
  + CPU 0: Dispatch P1
  + P1 tiếp tục, thực thi lệnh: **free 4** (giải phóng region 4)
  + libfree được gọi

- **Time slot 20:**
  + CPU 0 tiếp tục chạy P1 (calc #13)
  + CPU 1 tiếp tục chạy P4 (calc #7)

- **Time slot 21:**
  + CPU 0: **P1 HOÀN THÀNH** (14 lệnh)
  + CPU 0: Dispatch P2 từ queue priority 15
  + P2 tiếp tục (calc)
  + CPU 1 tiếp tục chạy P4

- **Time slot 22:**
  + CPU 0 tiếp tục chạy P2 (calc)
  + CPU 1 tiếp tục chạy P4 (calc #8)

- **Time slot 23:**
  + CPU 1: **P4 HOÀN THÀNH** (10 lệnh calc)
  + CPU 1: Không còn process ở priority 0 -> kiểm tra priority khác
  + Không còn process cho CPU 1 -> **CPU 1 STOPPED**
  + CPU 0 tiếp tục chạy P2

- **Time slot 24:**
  + CPU 0 tiếp tục chạy P2 (calc)

- **Time slot 25:**
  + CPU 0: **P2 HOÀN THÀNH** (10 lệnh calc)
  + CPU 0: Không còn process nào -> **CPU 0 STOPPED**
  + **HỆ THỐNG KẾT THÚC**

### Tổng kết os_0_mlq_paging
- Tổng thời gian: 25 time slots
- P1 (p0s, priority 0): Load t=0, Finish t=21, có memory operations
- P2 (p1s, priority 15): Load t=2, Finish t=25
- P3 (p1s, priority 0): Load t=4, Finish t=19
- P4 (p1s, priority 0): Load t=6, Finish t=23
- **Đặc điểm**: Paging được sử dụng, page table được in ra khi có memory operations

---

## 6. TEST: os_1_mlq_paging

### Cấu hình
```
2 4 8                    -> quantum=2, 4 CPUs, 8 processes
1048576 16777216 0 0 0   -> RAM=1MB, SWAP=16MB
1 p0s  130               -> t=1: p0s, priority=130 (thấp nhất)
2 s3   39                -> t=2: s3, priority=39
4 m1s  15                -> t=4: m1s, priority=15
6 s2   120               -> t=6: s2, priority=120
7 m0s  120               -> t=7: m0s, priority=120
9 p1s  15                -> t=9: p1s, priority=15
11 s0  38                -> t=11: s0, priority=38
16 s1  0                 -> t=16: s1, priority=0 (cao nhất)
```

### Phân tích từng Time Slot

- **Time slot 0:**
  + Hệ điều hành khởi động loader routine
  + Không có process nào được load ở t=0
  + Tất cả 4 CPUs idle

- **Time slot 1:**
  + Loader load process p0s, gán PID=1, Priority=130
  + P1 vào ready queue priority 130
  + Tất cả CPUs idle (chờ dispatch ở slot tiếp)

- **Time slot 2:**
  + CPU 3 dispatch P1 (p0s)
  + P1 bắt đầu thực thi (calc)
  + Loader load process s3, gán PID=2, Priority=39
  + P2 vào ready queue priority 39 (CAO HƠN P1)
  + CPU 0, 1, 2 idle

- **Time slot 3:**
  + CPU 2 dispatch P2 (s3)
  + P2 bắt đầu thực thi (calc #1)
  + CPU 3 tiếp tục chạy P1
  + P1 thực thi lệnh: **alloc 300 0**
  + liballoc được gọi
  + CPU 0, 1 idle

- **Time slot 4:**
  + CPU 3: P1 hết quantum, put P1 to queue, re-dispatch P1
  + P1 thực thi lệnh: **alloc 300 4**
  + liballoc được gọi
  + Loader load process m1s, gán PID=3, Priority=15
  + P3 vào ready queue priority 15 (CAO NHẤT hiện tại)
  + CPU 2 tiếp tục chạy P2

- **Time slot 5:**
  + CPU 2: P2 hết quantum, put P2 to queue priority 39
  + CPU 2: Dispatch P3 (priority 15 > priority 39)
  + P3 bắt đầu thực thi, lệnh: **alloc 300 0**
  + libfree được gọi (??? - có thể là free implicit)
  + CPU 1 dispatch P2 (s3) từ queue
  + P2 tiếp tục (calc #2)
  + liballoc cho P1
  + CPU 3: P1 thực thi lệnh: **free 0**
  + libfree được gọi

- **Time slot 6:**
  + CPU 3: P1 hết quantum, re-dispatch P1
  + P1 thực thi lệnh: **alloc 100 1**
  + liballoc được gọi (2 lần cho các regions)
  + Loader load process s2, gán PID=4, Priority=120
  + P4 vào ready queue priority 120
  + CPU 2 tiếp tục chạy P3
  + CPU 1 tiếp tục chạy P2

- **Time slot 7:**
  + CPU 2: P3 hết quantum, put P3 to queue, re-dispatch P3
  + P3 thực thi: **alloc 100 1**, rồi **free 0**
  + libfree, libwrite được gọi
  + CPU 1: P2 hết quantum, put P2 to queue, re-dispatch P2
  + CPU 0 dispatch P4 (s2)
  + P4 bắt đầu thực thi (calc #1)
  + Loader load process m0s, gán PID=5, Priority=120
  + P5 vào ready queue priority 120
  + CPU 3: P1 thực thi lệnh: **write 100 1 20**

- **Time slot 8:**
  + CPU 3: P1 hết quantum, put P1 to queue priority 130
  + CPU 3: Dispatch P5 (priority 120 < priority 130)
  + P5 bắt đầu thực thi, lệnh: **alloc 300 0**
  + liballoc được gọi (2 lần)
  + CPU 2 tiếp tục chạy P3
  + CPU 1 tiếp tục chạy P2
  + CPU 0 tiếp tục chạy P4

- **Time slot 9:**
  + CPU 2: P3 hết quantum, put P3 to queue, re-dispatch P3
  + P3 thực thi: **alloc 100 2**, rồi **free 2**
  + libfree được gọi
  + CPU 1: P2 hết quantum, re-dispatch P2
  + CPU 0: P4 hết quantum, re-dispatch P4
  + Loader load process p1s, gán PID=6, Priority=15
  + P6 vào ready queue priority 15
  + liballoc cho P6
  + CPU 3 tiếp tục chạy P5

- **Time slot 10:**
  + CPU 3: P5 hết quantum, put P5 to queue
  + CPU 3: Dispatch P6 (priority 15 < priority 120)
  + P6 bắt đầu thực thi (calc #1)
  + libfree được gọi (P3 free 2)
  + CPU 2: **P3 HOÀN THÀNH** (6 lệnh của m1s)
  + CPU 1, 0 tiếp tục

- **Time slot 11:**
  + CPU 2: Dispatch P5 từ queue
  + P5 thực thi: **alloc 100 1**, rồi **free 0**
  + libfree được gọi (2 lần)
  + CPU 1: P2 hết quantum, put P2 to queue, re-dispatch P2
  + CPU 0: P4 hết quantum, put P4 to queue, re-dispatch P4
  + Loader load process s0, gán PID=7, Priority=38
  + P7 vào ready queue priority 38
  + CPU 3 tiếp tục chạy P6

- **Time slot 12:**
  + CPU 3: P6 hết quantum, put P6 to queue, re-dispatch P6
  + P6 tiếp tục (calc #2)
  + liballoc cho P1 (từ p0s)
  + CPU 2 tiếp tục chạy P5
  + CPU 1 tiếp tục chạy P2
  + CPU 0 tiếp tục chạy P4

- **Time slot 13:**
  + CPU 1: P2 hết quantum, put P2 to queue
  + CPU 1: Dispatch P7 (priority 38 < priority 39)
  + P7 bắt đầu thực thi (calc #1)
  + CPU 2: P5 hết quantum, put P5 to queue
  + CPU 2: Dispatch P2 (Round-robin)
  + CPU 0: P4 hết quantum, put P4 to queue
  + CPU 0: Dispatch P5
  + P5 thực thi: **write 102 1 20**
  + libwrite được gọi
  + CPU 3 tiếp tục chạy P6

- **Time slot 14:**
  + CPU 3: P6 hết quantum, put P6 to queue, re-dispatch P6
  + CPU 2: **P2 HOÀN THÀNH** (11 lệnh calc của s3)
  + CPU 2: Dispatch P4 từ queue
  + P4 tiếp tục (calc)
  + libwrite cho P5
  + CPU 1 tiếp tục chạy P7
  + CPU 0 tiếp tục chạy P5

- **Time slot 15:**
  + CPU 1: P7 hết quantum, put P7 to queue, re-dispatch P7
  + CPU 0: **P5 HOÀN THÀNH** (6 lệnh của m0s)
  + CPU 0: Dispatch P1 (từ queue priority 130)
  + P1 tiếp tục, lệnh: **read 1 20 20**
  + libread được gọi
  + CPU 3 tiếp tục chạy P6
  + CPU 2 tiếp tục chạy P4

- **Time slot 16:**
  + CPU 3: P6 hết quantum, put P6 to queue, re-dispatch P6
  + CPU 2: P4 hết quantum, put P4 to queue, re-dispatch P4
  + Loader load process s1, gán PID=8, Priority=0
  + P8 vào ready queue priority 0 (CAO NHẤT!)
  + libwrite cho P1
  + CPU 1 tiếp tục chạy P7
  + CPU 0 tiếp tục chạy P1

- **Time slot 17:**
  + CPU 1: P7 hết quantum, put P7 to queue
  + CPU 1: Dispatch P8 (priority 0 CAO NHẤT)
  + P8 bắt đầu thực thi (calc #1)
  + CPU 0: P1 hết quantum, put P1 to queue
  + CPU 0: Dispatch P7 (next highest priority available)
  + P7 tiếp tục (calc)
  + CPU 3, 2 tiếp tục

- **Time slot 18:**
  + CPU 3: P6 hết quantum, put P6 to queue, re-dispatch P6
  + CPU 2: P4 hết quantum, put P4 to queue, re-dispatch P4
  + CPU 1 tiếp tục chạy P8
  + CPU 0 tiếp tục chạy P7

- **Time slot 19:**
  + CPU 1: P8 hết quantum, put P8 to queue, re-dispatch P8
  + CPU 0: P7 hết quantum, put P7 to queue, re-dispatch P7
  + CPU 3 tiếp tục chạy P6
  + CPU 2 tiếp tục chạy P4

- **Time slot 20:**
  + CPU 3: **P6 HOÀN THÀNH** (10 lệnh calc của p1s)
  + CPU 3: Dispatch P1 từ queue
  + P1 tiếp tục, lệnh: **read 2 20 20**
  + libread được gọi
  + CPU 2: **P4 HOÀN THÀNH** (12 lệnh calc của s2)
  + CPU 2: Không còn process phù hợp -> **CPU 2 STOPPED**
  + CPU 1 tiếp tục chạy P8
  + CPU 0 tiếp tục chạy P7

- **Time slot 21:**
  + CPU 1: P8 hết quantum, put P8 to queue, re-dispatch P8
  + CPU 0: P7 hết quantum, put P7 to queue, re-dispatch P7
  + CPU 3 tiếp tục chạy P1
  + P1 thực thi: **write 103 3 20**
  + libwrite được gọi

- **Time slot 22:**
  + CPU 3: P1 hết quantum, put P1 to queue, re-dispatch P1
  + P1 thực thi: **read 3 20 20**
  + libread được gọi
  + CPU 1 tiếp tục chạy P8
  + CPU 0 tiếp tục chạy P7

- **Time slot 23:**
  + CPU 1: P8 hết quantum, put P8 to queue, re-dispatch P8
  + CPU 0: P7 hết quantum, put P7 to queue, re-dispatch P7
  + CPU 3 tiếp tục chạy P1

- **Time slot 24:**
  + CPU 1: **P8 HOÀN THÀNH** (7 lệnh calc của s1)
  + CPU 1: Không còn process priority 0 -> **CPU 1 STOPPED**
  + CPU 3: P1 hết quantum, put P1 to queue, re-dispatch P1
  + P1 thực thi: **calc**, rồi **free 4**
  + libfree được gọi
  + CPU 0 tiếp tục chạy P7

- **Time slot 25:**
  + CPU 0: P7 hết quantum, put P7 to queue, re-dispatch P7
  + CPU 3 tiếp tục chạy P1

- **Time slot 26:**
  + CPU 3: **P1 HOÀN THÀNH** (14 lệnh của p0s)
  + CPU 3: Không còn process -> **CPU 3 STOPPED**
  + CPU 0 tiếp tục chạy P7

- **Time slot 27:**
  + CPU 0: P7 hết quantum, put P7 to queue, re-dispatch P7
  + P7 tiếp tục

- **Time slot 28:**
  + CPU 0: **P7 HOÀN THÀNH** (15 lệnh calc của s0)
  + CPU 0: Không còn process nào -> **CPU 0 STOPPED**
  + **HỆ THỐNG KẾT THÚC**

### Tổng kết os_1_mlq_paging
- Tổng thời gian: 28 time slots
- 4 CPUs chạy song song với 8 processes
- Priority levels sử dụng: 0, 15, 38, 39, 120, 130
- **P8 (priority 0)**: Được ưu tiên cao nhất khi load ở t=16
- **P1 (priority 130)**: Bị preempt nhiều lần, chạy cuối cùng
- **Memory operations**: alloc, free, read, write được thực hiện với paging

### Thứ tự hoàn thành:
1. P3 (m1s) - slot 11
2. P2 (s3) - slot 14
3. P5 (m0s) - slot 15
4. P6 (p1s) - slot 20
5. P4 (s2) - slot 20
6. P8 (s1) - slot 24
7. P1 (p0s) - slot 26
8. P7 (s0) - slot 28

---

## Tổng kết chung

### Các khái niệm quan trọng được minh họa:

1. **Multi-Level Queue (MLQ)**: Process với priority số nhỏ hơn được ưu tiên chạy trước

2. **Round-Robin**: Các process cùng priority level được luân phiên theo quantum

3. **Preemption**: Process đang chạy bị đẩy ra khi process priority cao hơn đến

4. **Starvation**: Process priority thấp có thể phải đợi rất lâu (P1 trong sched_1 đợi 31 slots)

5. **Paging**: Memory operations (alloc, free, read, write) sử dụng virtual memory với page table

6. **Multi-CPU**: Nhiều CPUs có thể chạy song song, mỗi CPU dispatch độc lập từ ready queue
