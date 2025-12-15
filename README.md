# OS_LamiaAtrium - Hệ Điều Hành Mô Phỏng

<p align="center">
  <strong>Bài Tập Lớn Môn Hệ Điều Hành (CO2018)</strong><br>
  <em>Trường Đại học Bách khoa - ĐHQG TP.HCM</em><br>
  <em>Học kỳ 1, Năm học 2025-2026</em>
</p>

---

## Cấu trúc thư mục

```
OS_LamiaAtrium/
├── README.md                    # Tài liệu hướng dẫn
├── ossim_lamiaatrium/
│   ├── lythuyet_bosung.tex      # Tài liệu lý thuyết bổ sung
│   ├── OS_LamiaAtrium_Report.pdf # Báo cáo chi tiết
│   ├── TongHop/                 # Tài liệu tổng hợp
│   │   ├── kienthuc.md          # Kiến thức paging
│   │   ├── phantich.md          # Phân tích test case
│   │   └── dif.md               # Danh sách thay đổi
│   └── ossim_lamiaatrium/       # Source code chính
│       ├── Makefile             # Build configuration
│       ├── os                   # Binary executable
│       ├── include/             # Header files
│       │   ├── mm.h             # Memory management
│       │   ├── sched.h          # Scheduler
│       │   ├── queue.h          # Queue data structure
│       │   └── ...
│       ├── src/                 # Source files
│       │   ├── mm-vm.c          # Virtual memory
│       │   ├── mm-memphy.c      # Physical memory
│       │   ├── sched.c          # Scheduler
│       │   ├── cpu.c            # CPU emulation
│       │   ├── os.c             # Main OS logic
│       │   └── ...
│       ├── input/               # Test input files
│       │   ├── proc/            # Process definitions
│       │   └── os_*             # Test configurations
│       └── output/              # Expected outputs
```

---

## Hướng dẫn biên dịch và chạy

### Yêu cầu hệ thống

- **Hệ điều hành:** Linux/Unix (hoặc WSL trên Windows)
- **Compiler:** GCC với hỗ trợ pthread
- **Make:** GNU Make

### Các bước thực hiện

```bash
# 1. Clone repository
git clone https://github.com/namhcmutpd/OS_LamiaAtrium.git
cd OS_LamiaAtrium/ossim_lamiaatrium/ossim_lamiaatrium

# 2. Biên dịch
make clean
make all

# 3. Chạy với test case
./os sched                    # Test scheduler
./os os_1_mlq_paging          # Test paging
./os os_syscall               # Test system call

# 4. So sánh với expected output
diff <(./os input/sched) output/sched.output
```

### Các tùy chọn biên dịch

Trong file `include/os-cfg.h`:

```c
#define MM_FIXED_MEMSZ    // Sử dụng memory size cố định
#define MM64 1            // Bật 5-level paging (64-bit)
#define MLQ_SCHED         // Sử dụng Multi-Level Queue
```

---

## Các test case

| Test Case | Mô tả | Thành phần kiểm tra |
|-----------|-------|---------------------|
| `sched` | 2 CPU, 3 processes, quantum=4 | MLQ Scheduling |
| `sched_0` | 1 CPU, 2 processes, priority preemption | Priority Scheduling |
| `sched_1` | Round-robin với nhiều process | Time quantum |
| `os_1_mlq_paging` | Paging với RAM 2MB | Page allocation |
| `os_1_mlq_paging_small_1K` | Paging với RAM 2KB | Swapping |
| `os_1_mlq_paging_small_4K` | Paging với RAM 4KB | Page replacement |
| `os_syscall` | System call testing | Syscall handler |

---

## Tài liệu tham khảo

1. **Silberschatz, A., Galvin, P. B., & Gagne, G.** (2018). *Operating System Concepts* (10th ed.). Wiley.

2. **Tanenbaum, A. S., & Bos, H.** (2014). *Modern Operating Systems* (4th ed.). Pearson.

3. **Love, R.** (2010). *Linux Kernel Development* (3rd ed.). Addison-Wesley.

4. **Bovet, D. P., & Cesati, M.** (2005). *Understanding the Linux Kernel* (3rd ed.). O'Reilly Media.

5. **Slide bài giảng môn CO2018** - Trường Đại học Bách khoa TP.HCM.


## Lời cảm ơn

Nhóm chúng em xin gửi lời cảm ơn chân thành đến Thầy Nguyễn Minh Tâm giảng viên môn Hệ Điều Hành đã tận tình hướng dẫn và cung cấp kiến thức nền tảng trong suốt quá trình học tập. Đồ án này là kết quả của quá trình nghiên cứu và thực hành, giúp chúng em hiểu sâu hơn về các cơ chế hoạt động của hệ điều hành hiện đại.

---

<p align="center">
  <em>© 2025 - Trường Đại học Bách khoa - ĐHQG TP.HCM</em><br>
  <em>Khoa Khoa học và Kỹ thuật Máy tính</em>
</p>
