# 🖥️ HỆ THỐNG PAGING - KIẾN THỨC TỔNG HỢP

## Mục lục
1. [Tổng quan lý thuyết](#1-tổng-quan-lý-thuyết)
2. [Cấu hình trong code](#2-cấu-hình-trong-code)
3. [Cấu trúc Page Table Entry (PTE)](#3-cấu-trúc-page-table-entry-pte)
4. [Hệ thống Page Table 5 cấp (MM64)](#4-hệ-thống-page-table-5-cấp-mm64)
5. [Cấu trúc dữ liệu chính](#5-cấu-trúc-dữ-liệu-chính)
6. [Quá trình Allocation](#6-quá-trình-allocation-liballoc)
7. [Quá trình Read/Write](#7-quá-trình-readwrite)
8. [Swapping](#8-swapping)
9. [Ví dụ thực tế](#9-ví-dụ-thực-tế)
10. [Tóm tắt](#10-tóm-tắt)

---

## 1. Tổng quan lý thuyết

### 1.1 Paging là gì?
Paging là kỹ thuật quản lý bộ nhớ cho phép:
- **Virtual Address Space**: Mỗi process có không gian địa chỉ ảo riêng
- **Physical Memory**: RAM vật lý được chia thành các **frames**
- **Page Table**: Bảng ánh xạ từ địa chỉ ảo → địa chỉ vật lý

```
┌─────────────────────────────────────────────────────────────────┐
│                    VIRTUAL ADDRESS SPACE                        │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐     │
│  │  Page 0  │   │  Page 1  │   │  Page 2  │   │  Page 3  │     │
│  └────┬─────┘   └────┬─────┘   └────┬─────┘   └────┬─────┘     │
│       │              │              │              │            │
│       │         PAGE TABLE          │              │            │
│       │    ┌─────────────────┐      │              │            │
│       └───►│ PGN 0 → FPN 5   │      │              │            │
│            │ PGN 1 → FPN 2   │◄─────┘              │            │
│            │ PGN 2 → SWAP    │                     │            │
│            │ PGN 3 → FPN 7   │◄────────────────────┘            │
│            └─────────────────┘                                  │
│                    │                                            │
│                    ▼                                            │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              PHYSICAL MEMORY (RAM)                        │   │
│  │  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐       │   │
│  │  │Frame 0│ │Frame 1│ │Frame 2│ │Frame 3│ │Frame 4│ ...   │   │
│  │  └───────┘ └───────┘ └───────┘ └───────┘ └───────┘       │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 Cấu trúc địa chỉ ảo

```
┌─────────────────────────────────────────────────────────────┐
│              VIRTUAL ADDRESS (22-bit trong code này)        │
├────────────────────────────────┬────────────────────────────┤
│       Page Number (PGN)        │        Offset              │
│         14 bits                │        8 bits              │
│       (bits 8-21)              │      (bits 0-7)            │
└────────────────────────────────┴────────────────────────────┘
                │                             │
                │                             │
                ▼                             │
         Page Table                           │
              │                               │
              ▼                               │
      Frame Number (FPN)                      │
              │                               │
              ▼                               ▼
┌─────────────────────────────────────────────────────────────┐
│              PHYSICAL ADDRESS                               │
├────────────────────────────────┬────────────────────────────┤
│       Frame Number (FPN)       │        Offset              │
└────────────────────────────────┴────────────────────────────┘
```

---

## 2. Cấu hình trong code

### 2.1 Các hằng số quan trọng (`include/mm.h`)

```c
#define PAGING_CPU_BUS_WIDTH 22   // Độ rộng bus = 22 bits
#define PAGING_PAGESZ  256        // Page/Frame size = 256 bytes
#define PAGING_MEMRAMSZ BIT(21)   // RAM mặc định = 2MB
#define PAGING_MEMSWPSZ BIT(29)   // Swap space = 512MB
```

### 2.2 Bảng tính toán

| Thông số | Giá trị | Công thức |
|----------|---------|-----------|
| **Page Size** | 256 bytes | `PAGING_PAGESZ` |
| **Offset bits** | 8 bits | log₂(256) = 8 |
| **Address Space** | 4MB | 2²² bytes |
| **Max Pages** | 16,384 | 2²² / 256 = 2¹⁴ |
| **RAM (default)** | 2MB | 2²¹ bytes |
| **RAM Frames** | 8,192 | 2²¹ / 256 |

### 2.3 Cấu hình test files

```
# Format: num_cpus num_loaders num_processes
# RAM_size SWAP_size 0 0 0
# start_time process_file priority

Ví dụ (os_1_mlq_paging_small_1K):
2 4 8
2048 16777216 0 0 0    ← RAM=2KB, SWAP=16MB
1 p0s  130
2 s3  39
...
```

---

## 3. Cấu trúc Page Table Entry (PTE)

### 3.1 Format của PTE (32-bit)

```
┌──────────────────────────────────────────────────────────────────┐
│                        PTE (32 bits)                             │
├───┬───┬───┬───┬─────────────────┬─────────────────────────────────┤
│31 │30 │29 │28 │    27-15        │           14-0                  │
│ P │ S │ R │ D │   USRNUM        │     FPN / SWPOFF                │
├───┼───┼───┼───┼─────────────────┼─────────────────────────────────┤
│ 1 │ 0 │ - │ - │       -         │  Frame Page Number (FPN)        │  ← Page trong RAM
├───┼───┼───┼───┼─────────────────┼─────────────────────────────────┤
│ 1 │ 1 │ - │ - │       -         │  SWPTYP(4-0) | SWPOFF(25-5)     │  ← Page trong SWAP
└───┴───┴───┴───┴─────────────────┴─────────────────────────────────┘

P = Present (1 = valid entry)
S = Swapped (1 = page đang ở swap space)
R = Reserved
D = Dirty (1 = page đã bị modify)
```

### 3.2 Code định nghĩa (`include/mm.h`)

```c
// Bit masks
#define PAGING_PTE_PRESENT_MASK BIT(31)   // Bit 31: Present
#define PAGING_PTE_SWAPPED_MASK BIT(30)   // Bit 30: Swapped
#define PAGING_PTE_DIRTY_MASK BIT(28)     // Bit 28: Dirty

// FPN field: bits 0-12
#define PAGING_PTE_FPN_LOBIT 0
#define PAGING_PTE_FPN_HIBIT 12

// Swap offset field: bits 5-25
#define PAGING_PTE_SWPOFF_LOBIT 5
#define PAGING_PTE_SWPOFF_HIBIT 25
```

### 3.3 Các macro thao tác PTE

```c
// Set bit
#define SETBIT(v,mask) (v=v|mask)
#define CLRBIT(v,mask) (v=v&~mask)

// Get/Set value
#define SETVAL(v,value,mask,offst) (v=(v&~mask)|((value<<offst)&mask))
#define GETVAL(v,mask,offst) ((v&mask)>>offst)

// Extract từ PTE
#define PAGING_PTE_FPN(pte)   GETVAL(pte,PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT)
#define PAGING_PTE_SWP(pte)   GETVAL(pte,PAGING_PTE_SWPOFF_MASK,PAGING_SWPFPN_OFFSET)

// Extract từ Address
#define PAGING_OFFST(x)  GETVAL(x,PAGING_OFFST_MASK,PAGING_ADDR_OFFST_LOBIT)
#define PAGING_PGN(x)    GETVAL(x,PAGING_PGN_MASK,PAGING_ADDR_PGN_LOBIT)
```

---

## 4. Hệ thống Page Table 5 cấp (MM64)

Khi `#define MM64`, hệ thống sử dụng **5-level page table** giống Linux kernel:

```
┌────────────────────────────────────────────────────────────────────────────┐
│                        VIRTUAL ADDRESS (64-bit mode)                       │
├──────────┬──────────┬──────────┬──────────┬──────────┬─────────────────────┤
│   PGD    │   P4D    │   PUD    │   PMD    │   PT     │      OFFSET         │
│  index   │  index   │  index   │  index   │  index   │    (12 bits)        │
└────┬─────┴────┬─────┴────┬─────┴────┬─────┴────┬─────┴─────────────────────┘
     │          │          │          │          │
     ▼          │          │          │          │
┌─────────┐     │          │          │          │
│   PGD   │     │          │          │          │
│  Table  │─────┘          │          │          │
└────┬────┘                │          │          │
     ▼                     │          │          │
┌─────────┐                │          │          │
│   P4D   │◄───────────────┘          │          │
│  Table  │───────────────────────────┘          │
└────┬────┘                                      │
     ▼                                           │
┌─────────┐                                      │
│   PUD   │◄─────────────────────────────────────┘
│  Table  │
└────┬────┘
     ▼
┌─────────┐
│   PMD   │
│  Table  │
└────┬────┘
     ▼
┌─────────┐
│   PT    │  ← Page Table cuối cùng chứa PTE → FPN
│  Table  │
└─────────┘
```

### 4.1 Định nghĩa trong `include/mm64.h`

```c
#define PAGING64_PAGESZ  4096    // Page table size = 4KB

// Address bit positions
#define PAGING64_ADDR_PGD_LOBIT 48
#define PAGING64_ADDR_P4D_LOBIT 39
#define PAGING64_ADDR_PUD_LOBIT 30
#define PAGING64_ADDR_PMD_LOBIT 21
#define PAGING64_ADDR_PT_LOBIT  12
```

### 4.2 Code page walk (`src/mm64.c`)

```c
// Tree walk từ PGD → PT
uint64_t *pgd_table = (uint64_t *)(krnl->mm->pgd);

// Level 5 → 4 (PGD → P4D)
if(!(pgd_table[pgd] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pgd_table[pgd] = (uint64_t) new_table | PT_PRESENT_BIT;
}
p4d_table = (uint64_t *)(pgd_table[pgd] & PT_ADDR_MASK);

// Tương tự cho các level khác...

// Cuối cùng: pt_table[pt] chứa PTE
pte = (addr_t *)&(pt_table[pt]);
```

---

## 5. Cấu trúc dữ liệu chính

### 5.1 Memory Management Structure (`include/os-mm.h`)

```c
struct mm_struct {
    // Page table hierarchy (64-bit mode)
    uint64_t *pgd;   // Page Global Directory
    uint64_t *p4d;   // Page 4-level Directory  
    uint64_t *pud;   // Page Upper Directory
    uint64_t *pmd;   // Page Middle Directory
    uint64_t *pt;    // Page Table

    // Virtual Memory Areas
    struct vm_area_struct *mmap;
    
    // Symbol table cho các region đã allocate
    struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ];
    
    // FIFO list để chọn victim page khi swap
    struct pgn_t *fifo_pgn;
};
```

### 5.2 Virtual Memory Area

```c
struct vm_area_struct {
    unsigned long vm_id;      // VMA ID
    addr_t vm_start;          // Địa chỉ bắt đầu
    addr_t vm_end;            // Địa chỉ kết thúc
    addr_t sbrk;              // Program break (heap pointer)
    
    struct mm_struct *vm_mm;  // Parent mm_struct
    struct vm_rg_struct *vm_freerg_list;  // Free regions
    struct vm_area_struct *vm_next;       // Next VMA
};
```

```
┌─────────────────────────────────────────────────────────────────┐
│                    VIRTUAL ADDRESS SPACE                        │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                      VMA 0 (Heap)                         │  │
│  │  vm_start ────────────────────────────────────► vm_end    │  │
│  │      │                                              │     │  │
│  │      │    ┌──────────┐  ┌──────────┐               │     │  │
│  │      │    │ Region 0 │  │ Region 1 │    ...        │     │  │
│  │      │    │ (alloc)  │  │ (free)   │               │     │  │
│  │      │    └──────────┘  └──────────┘               │     │  │
│  │      │                        ▲                    │     │  │
│  │      │                      sbrk                   │     │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                      VMA 1 (Stack)                        │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.3 Physical Memory Structure

```c
struct memphy_struct {
    BYTE *storage;    // Mảng byte lưu trữ dữ liệu
    int maxsz;        // Kích thước tối đa
    
    int rdmflg;       // Random access flag
    int cursor;       // Sequential access cursor
    
    // Quản lý frames
    struct framephy_struct *free_fp_list;  // Danh sách frame trống
    struct framephy_struct *used_fp_list;  // Danh sách frame đang dùng
};

struct framephy_struct { 
    addr_t fpn;                    // Frame Page Number
    struct framephy_struct *fp_next;
    struct mm_struct* owner;       // Process sở hữu frame này
};
```

### 5.4 Region và Page structures

```c
// Virtual memory region
struct vm_rg_struct {
    addr_t rg_start;
    addr_t rg_end;
    struct vm_rg_struct *rg_next;
};

// Page number node (for FIFO)
struct pgn_t {
    addr_t pgn;
    struct pgn_t *pg_next; 
};
```

---

## 6. Quá trình Allocation (liballoc)

### 6.1 Flow diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     ALLOCATION FLOW                             │
│                                                                 │
│  1. liballoc(caller, size, reg_id)                             │
│         │                                                       │
│         ▼                                                       │
│  2. __alloc(caller, vmaid=0, rgid, size, &alloc_addr)          │
│         │                                                       │
│         ▼                                                       │
│  3. get_free_vmrg_area() ─── Tìm free region đủ lớn            │
│         │                                                       │
│         ├──► Found: Cắt region, trả về địa chỉ                 │
│         │                                                       │
│         └──► Not Found:                                         │
│                   │                                             │
│                   ▼                                             │
│  4. inc_vma_limit() ─── Tăng VMA limit                         │
│         │                                                       │
│         ▼                                                       │
│  5. vm_map_ram() ─── Map pages vào RAM                         │
│         │                                                       │
│         ▼                                                       │
│  6. alloc_pages_range() ─── Allocate physical frames           │
│         │                                                       │
│         ├──► MEMPHY_get_freefp() ─── Lấy frame từ free list    │
│         │                                                       │
│         └──► Nếu hết RAM:                                       │
│                   │                                             │
│                   ▼                                             │
│              find_victim_page() ─── FIFO chọn victim           │
│                   │                                             │
│                   ▼                                             │
│              __swap_cp_page() ─── Swap victim ra disk          │
│                                                                 │
│  7. vmap_page_range() ─── Cập nhật page table (PTE)            │
│         │                                                       │
│         ▼                                                       │
│  8. pte_set_fpn(pgn, fpn) ─── Set PTE entry                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 Code `__alloc()` (`src/mm64.c`)

```c
int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
    struct vm_rg_struct rgnode;

    // 1. Tìm free region
    if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0) {
        // Found! Sử dụng region này
        caller->krnl->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
        caller->krnl->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
        *alloc_addr = rgnode.rg_start;
        return 0;
    }

    // 2. Không tìm thấy → Tăng VMA limit
    if (inc_vma_limit(caller, vmaid, size) < 0) {
        return -1;  // Không thể tăng limit
    }

    // 3. Thử lại tìm free region
    if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0) {
        caller->krnl->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
        caller->krnl->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
        *alloc_addr = rgnode.rg_start;
        return 0;
    }

    return -1;
}
```

### 6.3 Code `inc_vma_limit()` (`src/mm-vm.c`)

```c
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

    // Align size to page boundary
    addr_t inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
    int incnumpage = inc_amt / PAGING_PAGESZ;

    // Get area at current break point
    struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);

    // Save old end
    addr_t old_end = cur_vma->vm_end;

    // Increase limits
    cur_vma->vm_end += inc_amt;
    cur_vma->sbrk += inc_sz;

    // Map to RAM
    if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                   old_end, incnumpage, newrg) < 0) {
        // Rollback on failure
        cur_vma->vm_end = old_end;
        cur_vma->sbrk -= inc_sz;
        return -1;
    }
    return 0;
}
```

---

## 7. Quá trình Read/Write

### 7.1 Read Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                       READ FLOW                                 │
│                                                                 │
│  1. libread(caller, reg_id, offset, &data)                     │
│         │                                                       │
│         ▼                                                       │
│  2. __read(caller, vmaid, rgid, offset, &data)                 │
│         │                                                       │
│         ▼                                                       │
│  3. Tính virtual address:                                       │
│         addr = symrgtbl[rgid].rg_start + offset                │
│         │                                                       │
│         ▼                                                       │
│  4. pg_getval(mm, addr, &data, caller)                         │
│         │                                                       │
│         ▼                                                       │
│  5. Tách địa chỉ:                                               │
│         pgn = PAGING_PGN(addr)      // Page Number             │
│         off = PAGING_OFFST(addr)    // Offset trong page       │
│         │                                                       │
│         ▼                                                       │
│  6. pte_get_entry(caller, pgn) ─── Lấy PTE                     │
│         │                                                       │
│         ├──► PTE.PRESENT && !PTE.SWAPPED:                      │
│         │         fpn = PAGING_PTE_FPN(pte)                    │
│         │         phyaddr = fpn * PAGESZ + offset              │
│         │         MEMPHY_read(mram, phyaddr, &data)            │
│         │                                                       │
│         └──► PTE.PRESENT && PTE.SWAPPED:                       │
│                   │                                             │
│                   ▼                                             │
│              PAGE FAULT! → pg_getpage() để swap in             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 7.2 Code `pg_getval()` (`src/mm64.c`)

```c
int pg_getval(struct mm_struct *mm, addr_t addr, BYTE *data, struct pcb_t *caller)
{
    // Tách địa chỉ
    addr_t pgn = PAGING_PGN(addr);     // Page number
    int off = PAGING_OFFST(addr);      // Offset trong page

    // Lấy PTE
    uint32_t pte = pte_get_entry(caller, pgn);
    
    if (!(pte & PAGING_PTE_PRESENT_MASK)) {
        return -1;  // Page không valid
    }

    // Kiểm tra page có trong RAM không
    if (pte & PAGING_PTE_SWAPPED_MASK) {
        // Page đang ở swap → cần swap in (xử lý page fault)
        pg_getpage(mm, pgn, &fpn, caller);
    }

    // Đọc từ RAM
    addr_t fpn = PAGING_PTE_FPN(pte_get_entry(caller, pgn));
    addr_t phyaddr = fpn * PAGING_PAGESZ + off;
    MEMPHY_read(caller->krnl->mram, phyaddr, data);
    
    return 0;
}
```

### 7.3 Address Translation Example

```
Virtual Address = 0x12345 (hex) = 74565 (dec)

Page Size = 256 bytes = 2^8
Offset bits = 8

┌─────────────────────────────────────────┐
│  Virtual Address: 0x12345               │
│  Binary: 0001 0010 0011 0100 0101       │
├─────────────────────┬───────────────────┤
│  Page Number (PGN)  │   Offset          │
│  0x123 = 291        │   0x45 = 69       │
└─────────────────────┴───────────────────┘
         │
         │  Page Table Lookup
         │  PTE[291] → FPN = 7
         ▼
┌─────────────────────┬───────────────────┐
│  Frame Number (FPN) │   Offset          │
│       7             │   0x45 = 69       │
└─────────────────────┴───────────────────┘
         │
         │  Physical Address = FPN * PAGESZ + Offset
         │                   = 7 * 256 + 69
         │                   = 1792 + 69 = 1861
         ▼
┌─────────────────────────────────────────┐
│  Physical Address: 1861                 │
└─────────────────────────────────────────┘
```

---

## 8. Swapping

### 8.1 Khi nào cần swap?

```
┌─────────────────────────────────────────────────────────────────┐
│                      SWAP SCENARIOS                             │
│                                                                 │
│  Scenario 1: Allocation khi hết RAM                            │
│  ─────────────────────────────────────                         │
│  alloc_pages_range() → MEMPHY_get_freefp() fails               │
│         │                                                       │
│         ▼                                                       │
│  find_victim_page() ─── FIFO: lấy page đầu tiên trong list     │
│         │                                                       │
│         ▼                                                       │
│  __swap_cp_page(RAM[victim], SWAP[free])                       │
│         │                                                       │
│         ▼                                                       │
│  Cập nhật victim PTE: SWAPPED=1, SWPOFF=swap_offset            │
│         │                                                       │
│         ▼                                                       │
│  Dùng frame của victim cho allocation mới                      │
│                                                                 │
│  ─────────────────────────────────────────────────────────────  │
│                                                                 │
│  Scenario 2: Access page đang ở swap (Page Fault)              │
│  ───────────────────────────────────────────────               │
│  pg_getval() hoặc pg_setval() → PTE.SWAPPED=1                  │
│         │                                                       │
│         ▼                                                       │
│  find_victim_page() ─── Tìm page để swap out                   │
│         │                                                       │
│         ▼                                                       │
│  __swap_cp_page(RAM[victim], SWAP[free])   ─── Victim → Swap   │
│  __swap_cp_page(SWAP[target], RAM[victim]) ─── Target → RAM    │
│         │                                                       │
│         ▼                                                       │
│  Cập nhật cả 2 PTEs                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 8.2 FIFO Page Replacement Algorithm

```c
// Tìm victim page (FIFO - First In First Out)
int find_victim_page(struct mm_struct *mm, addr_t *pgn)
{
    // FIFO: Lấy page đầu tiên trong list (oldest)
    struct pgn_t *pg = mm->fifo_pgn;
    
    if (pg == NULL) return -1;
    
    *pgn = pg->pgn;
    
    // Remove từ đầu list
    mm->fifo_pgn = pg->pg_next;
    free(pg);
    
    return 0;
}

// Khi allocate page mới, thêm vào cuối FIFO list
int enlist_pgn_node(struct pgn_t **pgnlist, addr_t pgn)
{
    struct pgn_t *newnode = malloc(sizeof(struct pgn_t));
    newnode->pgn = pgn;
    newnode->pg_next = NULL;
    
    // Thêm vào cuối list (newest)
    if (*pgnlist == NULL) {
        *pgnlist = newnode;
    } else {
        struct pgn_t *tail = *pgnlist;
        while (tail->pg_next != NULL) {
            tail = tail->pg_next;
        }
        tail->pg_next = newnode;
    }
    return 0;
}
```

### 8.3 Swap copy function

```c
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
    // Copy từng byte của page
    for (int i = 0; i < PAGING_PAGESZ; i++) {
        BYTE data;
        addr_t srcaddr = srcfpn * PAGING_PAGESZ + i;
        addr_t dstaddr = dstfpn * PAGING_PAGESZ + i;
        
        MEMPHY_read(mpsrc, srcaddr, &data);
        MEMPHY_write(mpdst, dstaddr, data);
    }
    return 0;
}
```

---

## 9. Ví dụ thực tế

### 9.1 Test `os_1_mlq_paging_small_1K`

**Cấu hình:**
```
RAM = 2048 bytes = 2KB
Swap = 16MB
Frame size = 256 bytes
Số frames trong RAM = 2048 / 256 = 8 frames
```

**Quá trình thực thi:**

```
┌─────────────────────────────────────────────────────────────────┐
│                    EXECUTION TIMELINE                           │
│                                                                 │
│  Process 1 (p0s): alloc 300 bytes                              │
│  ─────────────────────────────────────                         │
│  • Cần: ceil(300/256) = 2 pages                                │
│  • RAM frames used: 2                                          │
│  • RAM free: 6 frames                                          │
│                                                                 │
│  Process 2 (s3): alloc 400 bytes                               │
│  ─────────────────────────────────────                         │
│  • Cần: ceil(400/256) = 2 pages                                │
│  • RAM frames used: 4                                          │
│  • RAM free: 4 frames                                          │
│                                                                 │
│  Process 3 (m1s): alloc 500 bytes                              │
│  ─────────────────────────────────────                         │
│  • Cần: ceil(500/256) = 2 pages                                │
│  • RAM frames used: 6                                          │
│  • RAM free: 2 frames                                          │
│                                                                 │
│  Process 4 (s2): alloc 600 bytes                               │
│  ─────────────────────────────────────                         │
│  • Cần: ceil(600/256) = 3 pages                                │
│  • RAM chỉ còn 2 frames! → SWAP                                │
│  • Victim page (FIFO): Page đầu tiên của Process 1             │
│  • Swap out 1 page → có 3 frames → allocate thành công         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 9.2 Process file format

```
# File: input/proc/p0s
20 1              ← 20 instructions, priority 1
alloc 300 0       ← allocate 300 bytes vào region 0
write 100 0 0     ← write value 100 vào region 0, offset 0
read 0 0          ← read từ region 0, offset 0
free 0            ← free region 0
```

---

## 10. Tóm tắt

### 10.1 Các file quan trọng

| File | Chức năng |
|------|-----------|
| `include/mm.h` | Định nghĩa constants, macros cho paging |
| `include/mm64.h` | Định nghĩa cho 5-level page table |
| `include/os-mm.h` | Định nghĩa các structures (mm_struct, vma, etc.) |
| `src/mm64.c` | Page table operations, PTE management, swap |
| `src/mm-vm.c` | Virtual memory area management |
| `src/mm-memphy.c` | Physical frame management |
| `src/libmem.c` | User API: liballoc, libfree, libread, libwrite |

### 10.2 Flow tổng quan

```
User Code (process instructions)
       │
       ▼
   libmem.c (liballoc, libfree, libread, libwrite)
       │
       ▼
   mm-vm.c (__alloc, __free) + mm64.c (__read, __write)
       │
       ▼
   mm64.c (page table operations, PTE, swap handling)
       │
       ▼
   mm-memphy.c (physical frame management)
       │
       ▼
   RAM (memphy_struct) / Swap Space (memphy_struct)
```

### 10.3 Key concepts

1. **Virtual Address** = Page Number + Offset
2. **Page Table** maps Page Number → Frame Number
3. **PTE** contains: Present, Swapped, Dirty bits + FPN/SwapOffset
4. **FIFO** algorithm for page replacement
5. **5-level page table** (MM64): PGD → P4D → PUD → PMD → PT → PTE

### 10.4 Quan hệ giữa các cấu trúc

```
┌──────────────────────────────────────────────────────────────────┐
│                         KERNEL (krnl_t)                          │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ mm_struct *mm      ─────────────────────────────┐          │  │
│  │ memphy_struct *mram ────────────┐               │          │  │
│  │ memphy_struct **mswp ───────┐   │               │          │  │
│  └─────────────────────────────│───│───────────────│──────────┘  │
│                                │   │               │             │
│                                ▼   ▼               ▼             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐   │
│  │  SWAP SPACE     │  │      RAM        │  │   mm_struct     │   │
│  │  (memphy)       │  │   (memphy)      │  │                 │   │
│  │                 │  │                 │  │  pgd ──► 5-level│   │
│  │  free_fp_list   │  │  free_fp_list   │  │  mmap ──► VMAs  │   │
│  │  storage[]      │  │  storage[]      │  │  symrgtbl[]     │   │
│  │                 │  │                 │  │  fifo_pgn       │   │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

---

## Phụ lục: Debugging Tips

### A. Kiểm tra PTE

```c
uint32_t pte = pte_get_entry(caller, pgn);
printf("PTE for page %d: 0x%08x\n", pgn, pte);
printf("  Present: %d\n", (pte & PAGING_PTE_PRESENT_MASK) ? 1 : 0);
printf("  Swapped: %d\n", (pte & PAGING_PTE_SWAPPED_MASK) ? 1 : 0);
printf("  FPN: %d\n", PAGING_PTE_FPN(pte));
```

### B. Kiểm tra free frames

```c
struct framephy_struct *fp = mram->free_fp_list;
int count = 0;
while (fp != NULL) {
    count++;
    fp = fp->fp_next;
}
printf("Free frames in RAM: %d\n", count);
```

### C. Kiểm tra FIFO list

```c
struct pgn_t *pg = mm->fifo_pgn;
printf("FIFO page list: ");
while (pg != NULL) {
    printf("%d -> ", pg->pgn);
    pg = pg->pg_next;
}
printf("NULL\n");
```

---

# 📚 PHẦN 2: KIẾN THỨC NÂNG CAO

---

## 11. Tại sao cần Paging?

### 11.1 Vấn đề của Contiguous Memory Allocation

Trước khi có paging, bộ nhớ được cấp phát **liên tục (contiguous)**:

```
┌─────────────────────────────────────────────────────────────────┐
│              CONTIGUOUS ALLOCATION (Không dùng Paging)          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  RAM:  [  Process A  ][  Free  ][  Process B  ][  Free  ][P_C]  │
│        └─────────────┘└────────┘└─────────────┘└────────┘└───┘  │
│             500KB        200KB       300KB        150KB   100KB │
│                                                                 │
│  Vấn đề: Process D cần 300KB nhưng không có vùng liên tục!     │
│          Tổng free = 350KB > 300KB, nhưng bị PHÂN MẢNH!        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Các vấn đề:**
1. **External Fragmentation**: Bộ nhớ bị phân mảnh, không thể sử dụng
2. **Compaction tốn kém**: Di chuyển processes để gom vùng trống rất chậm
3. **Process phải load toàn bộ**: Cần đủ RAM cho cả process

### 11.2 Paging giải quyết như thế nào?

```
┌─────────────────────────────────────────────────────────────────┐
│                    PAGING SOLUTION                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Virtual Memory (Process D - 300KB = 3 pages):                  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐                           │
│  │ Page 0  │ │ Page 1  │ │ Page 2  │                           │
│  └────┬────┘ └────┬────┘ └────┬────┘                           │
│       │           │           │                                 │
│       │      Page Table       │                                 │
│       │    ┌───────────┐      │                                 │
│       └───►│ P0 → F3   │      │                                 │
│            │ P1 → F7   │◄─────┘                                 │
│            │ P2 → F1   │◄────────                               │
│            └───────────┘                                        │
│                 │                                               │
│                 ▼                                               │
│  Physical RAM (không cần liên tục!):                            │
│  [P_A][P_D.2][P_B][P_D.0][P_C][P_B][P_B][P_D.1][P_A]...        │
│   F0    F1    F2    F3    F4   F5   F6    F7    F8              │
│                                                                 │
│  ✓ Process D được cấp 3 frames KHÔNG liên tục                  │
│  ✓ Không có external fragmentation                              │
│  ✓ Không cần compaction                                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 11.3 Lợi ích của Paging

| Lợi ích | Mô tả |
|---------|-------|
| **Không fragmentation** | Frames có kích thước cố định, dễ quản lý |
| **Virtual Memory** | Process có thể lớn hơn RAM vật lý |
| **Isolation** | Mỗi process có không gian địa chỉ riêng |
| **Sharing** | Nhiều process có thể share cùng frame (shared libraries) |
| **Protection** | PTE có permission bits (read/write/execute) |
| **Demand Paging** | Chỉ load page khi cần, tiết kiệm RAM |

### 11.4 Khi nào cần sử dụng Paging?

```
┌─────────────────────────────────────────────────────────────────┐
│                 KHI NÀO CẦN PAGING?                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ✓ Chạy nhiều processes cùng lúc (multi-tasking)               │
│  ✓ Process cần nhiều bộ nhớ hơn RAM vật lý                     │
│  ✓ Cần isolation giữa các processes                            │
│  ✓ Cần share code/data giữa processes                          │
│  ✓ Hệ thống cần bảo mật (memory protection)                    │
│                                                                 │
│  ✗ KHÔNG CẦN khi:                                               │
│    - Embedded systems đơn giản                                  │
│    - Real-time systems cần deterministic timing                 │
│    - Chỉ chạy 1 process                                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 12. Tại sao cần 5-Level Page Table?

### 12.1 Vấn đề với Single-Level Page Table

Giả sử hệ thống 64-bit với page size 4KB:

```
Address Space = 2^64 bytes
Page Size = 4KB = 2^12 bytes
Số pages = 2^64 / 2^12 = 2^52 pages

Mỗi PTE = 8 bytes (64-bit)
Kích thước Page Table = 2^52 × 8 = 2^55 bytes = 32 PETABYTES!

→ KHÔNG THỂ LƯU TRỮ!
```

### 12.2 Giải pháp: Multi-Level Page Table

**Ý tưởng**: Không cần lưu toàn bộ page table, chỉ lưu phần đang dùng.

```
┌─────────────────────────────────────────────────────────────────┐
│              SINGLE-LEVEL vs MULTI-LEVEL                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  SINGLE-LEVEL (Flat):                                           │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ PTE0 │ PTE1 │ PTE2 │ ... │ PTE_N │  (N = 2^52 entries)  │    │
│  └─────────────────────────────────────────────────────────┘    │
│  → Phải allocate toàn bộ array dù chỉ dùng vài entries         │
│  → Lãng phí bộ nhớ khổng lồ!                                   │
│                                                                 │
│  MULTI-LEVEL (Tree):                                            │
│           ┌─────┐                                               │
│           │ PGD │ (512 entries, 4KB)                           │
│           └──┬──┘                                               │
│              │ (chỉ allocate nếu cần)                          │
│        ┌─────┼─────┐                                            │
│        ▼     ▼     ▼                                            │
│     ┌─────┐ NULL ┌─────┐                                       │
│     │ P4D │      │ P4D │ (mỗi cái 4KB)                         │
│     └──┬──┘      └──┬──┘                                       │
│        │            │                                           │
│        ▼            ▼                                           │
│     ┌─────┐      ┌─────┐                                       │
│     │ PUD │      │ PUD │                                       │
│     └──┬──┘      └──┬──┘                                       │
│        │            │                                           │
│       ...          ...                                          │
│                                                                 │
│  → Chỉ allocate các branches đang sử dụng                      │
│  → Tiết kiệm bộ nhớ đáng kể!                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 12.3 Chi tiết 5-Level Page Table (Linux x86-64)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    64-BIT VIRTUAL ADDRESS                                   │
├─────────┬─────────┬─────────┬─────────┬─────────┬───────────────────────────┤
│  63-57  │  56-48  │  47-39  │  38-30  │  29-21  │  20-12  │     11-0       │
│ (unused)│   PGD   │   P4D   │   PUD   │   PMD   │   PT    │    OFFSET      │
│  7 bits │  9 bits │  9 bits │  9 bits │  9 bits │  9 bits │    12 bits     │
└─────────┴────┬────┴────┬────┴────┬────┴────┬────┴────┬────┴───────┬────────┘
               │         │         │         │         │            │
               │         │         │         │         │            │
               ▼         │         │         │         │            │
          ┌─────────┐    │         │         │         │            │
          │   PGD   │    │         │         │         │            │
          │  Table  │    │         │         │         │            │
          │ [0-511] │    │         │         │         │            │
          └────┬────┘    │         │         │         │            │
               │         │         │         │         │            │
               ▼         ▼         │         │         │            │
          ┌─────────┐              │         │         │            │
          │   P4D   │◄─────────────┘         │         │            │
          │  Table  │                        │         │            │
          │ [0-511] │                        │         │            │
          └────┬────┘                        │         │            │
               │                             │         │            │
               ▼                             ▼         │            │
          ┌─────────┐                                  │            │
          │   PUD   │◄─────────────────────────────────┘            │
          │  Table  │                                               │
          │ [0-511] │                                               │
          └────┬────┘                                               │
               │                                                    │
               ▼                                                    ▼
          ┌─────────┐                                               
          │   PMD   │◄──────────────────────────────────────────────┘
          │  Table  │
          │ [0-511] │
          └────┬────┘
               │
               ▼
          ┌─────────┐
          │   PT    │  ← Page Table cuối cùng
          │  Table  │     chứa PTE với Frame Number
          │ [0-511] │
          └────┬────┘
               │
               ▼
          ┌─────────┐
          │  FRAME  │  + OFFSET → Physical Address
          │  (RAM)  │
          └─────────┘
```

### 12.4 Tại sao 5 levels?

| Level | Bits | Entries | Addressable per level |
|-------|------|---------|----------------------|
| PGD | 9 | 512 | 256 TB |
| P4D | 9 | 512 | 512 GB |
| PUD | 9 | 512 | 1 GB |
| PMD | 9 | 512 | 2 MB |
| PT | 9 | 512 | 4 KB (page) |
| Offset | 12 | - | 1 byte |

**Tổng**: 9+9+9+9+9+12 = **57 bits** addressable (128 PB virtual address space)

**Lịch sử tiến hóa:**
- **2-level**: 32-bit systems (4GB address space)
- **3-level**: PAE mode (64GB physical)
- **4-level**: x86-64 ban đầu (256TB virtual)
- **5-level**: Intel Ice Lake+ (128PB virtual) - cho big data, databases

### 12.5 Trong code này (MM64)

```c
// include/mm64.h
#define PAGING64_PAGESZ  4096    // 4KB per page/table

// Mỗi level có 512 entries (9 bits index)
// Entry size = 8 bytes (64-bit pointer + flags)
// Table size = 512 × 8 = 4096 bytes = 1 page
```

---

## 13. Tree Walk - Cách hoạt động chi tiết

### 13.1 Tree Walk là gì?

**Tree Walk** (hay **Page Table Walk**) là quá trình duyệt qua các cấp của page table để chuyển đổi địa chỉ ảo → địa chỉ vật lý.

```
┌─────────────────────────────────────────────────────────────────┐
│                    TREE WALK PROCESS                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Input: Virtual Address = 0x00007F4A_12345678                  │
│                                                                 │
│  Step 1: Extract indices from VA                                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ PGD_idx = (VA >> 48) & 0x1FF = 0x000 (entry 0)          │   │
│  │ P4D_idx = (VA >> 39) & 0x1FF = 0x0FE (entry 254)        │   │
│  │ PUD_idx = (VA >> 30) & 0x1FF = 0x128 (entry 296)        │   │
│  │ PMD_idx = (VA >> 21) & 0x1FF = 0x091 (entry 145)        │   │
│  │ PT_idx  = (VA >> 12) & 0x1FF = 0x145 (entry 325)        │   │
│  │ Offset  = VA & 0xFFF = 0x678                            │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  Step 2: Walk the tree                                          │
│                                                                 │
│  CR3 (PGD base) ──► PGD[0] ──► P4D_base                        │
│                                    │                            │
│                     P4D_base ──► P4D[254] ──► PUD_base          │
│                                                  │               │
│                                   PUD_base ──► PUD[296] ──► PMD │
│                                                            │    │
│                                             PMD_base ──► PMD[145] ──► PT │
│                                                                      │   │
│                                                       PT_base ──► PT[325] │
│                                                                      │    │
│                                                              Frame Number │
│                                                                      │    │
│  Step 3: Calculate Physical Address                                  │    │
│  Physical Address = (Frame Number × Page Size) + Offset              │    │
│                   = (FPN × 4096) + 0x678                            │    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 13.2 Code Tree Walk trong project

```c
// Từ src/mm64.c - pte_set_fpn()

int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
    struct krnl_t *krnl = caller->krnl;
    
    // Bước 1: Tách page number thành các indices
    addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
    get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
    
    // Bước 2: Tree walk với lazy allocation
    uint64_t *pgd_table = (uint64_t *)(krnl->mm->pgd);
    uint64_t *p4d_table, *pud_table, *pmd_table, *pt_table;
    
    // Level 5 → 4 (PGD → P4D)
    if (!(pgd_table[pgd_idx] & PT_PRESENT_BIT)) {
        // Table chưa tồn tại → Allocate mới!
        uint64_t *new_table = alloc_page_table();
        memset(new_table, 0, PAGING64_PAGESZ);
        pgd_table[pgd_idx] = (uint64_t)new_table | PT_PRESENT_BIT;
    }
    p4d_table = (uint64_t *)(pgd_table[pgd_idx] & PT_ADDR_MASK);
    
    // Level 4 → 3 (P4D → PUD) - tương tự
    if (!(p4d_table[p4d_idx] & PT_PRESENT_BIT)) {
        uint64_t *new_table = alloc_page_table();
        memset(new_table, 0, PAGING64_PAGESZ);
        p4d_table[p4d_idx] = (uint64_t)new_table | PT_PRESENT_BIT;
    }
    pud_table = (uint64_t *)(p4d_table[p4d_idx] & PT_ADDR_MASK);
    
    // ... tiếp tục cho PUD → PMD → PT
    
    // Bước 3: Set PTE tại vị trí cuối cùng
    pt_table[pt_idx] = fpn | PAGING_PTE_PRESENT_MASK;
}
```

### 13.3 Ý nghĩa của Tree Walk

```
┌─────────────────────────────────────────────────────────────────┐
│                 Ý NGHĨA CỦA TREE WALK                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. TIẾT KIỆM BỘ NHỚ (Lazy Allocation)                         │
│  ────────────────────────────────────                          │
│  • Chỉ allocate table khi CẦN (on-demand)                      │
│  • Process dùng ít memory → ít tables được tạo                 │
│  • Ví dụ: Process chỉ dùng 4KB → chỉ cần 5 tables (20KB)      │
│           thay vì 32 PB cho full page table!                   │
│                                                                 │
│  2. SPARSE ADDRESS SPACE                                        │
│  ───────────────────────                                       │
│  • Process có thể có "holes" trong address space               │
│  • Stack ở high address, heap ở low address                    │
│  • Vùng giữa không cần page tables                             │
│                                                                 │
│  3. SHARING GIỮA PROCESSES                                      │
│  ─────────────────────────                                     │
│  • Nhiều processes có thể share subtree                        │
│  • Ví dụ: Shared libraries → share PUD/PMD/PT                  │
│  • Copy-on-Write: fork() chỉ copy PGD, share phần còn lại     │
│                                                                 │
│  4. PROTECTION                                                  │
│  ──────────                                                    │
│  • Mỗi entry có permission bits                                │
│  • Kernel pages: User không thể access                         │
│  • Read-only pages: Không thể write                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 13.4 TLB - Translation Lookaside Buffer

Tree walk tốn **5 memory accesses** cho mỗi địa chỉ → Rất chậm!

**Giải pháp**: TLB cache

```
┌─────────────────────────────────────────────────────────────────┐
│                    TLB - FAST PATH                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Virtual Address ──► TLB Lookup                                 │
│                          │                                      │
│                    ┌─────┴─────┐                                │
│                    │           │                                │
│                    ▼           ▼                                │
│               TLB HIT      TLB MISS                             │
│                 │              │                                │
│                 │              ▼                                │
│                 │         Tree Walk (slow)                      │
│                 │              │                                │
│                 │              ▼                                │
│                 │         Update TLB                            │
│                 │              │                                │
│                 └──────┬───────┘                                │
│                        │                                        │
│                        ▼                                        │
│                Physical Address                                 │
│                                                                 │
│  TLB Entry: { Virtual Page Number → Physical Frame Number }    │
│                                                                 │
│  Hit rate: ~99% → Hầu hết translations là O(1)!                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 14. Virtual Memory Area (VMA) - Chi tiết

### 14.1 VMA là gì?

VMA (Virtual Memory Area) đại diện cho một **vùng liên tục** trong không gian địa chỉ ảo của process.

```
┌─────────────────────────────────────────────────────────────────┐
│                PROCESS VIRTUAL ADDRESS SPACE                    │
├─────────────────────────────────────────────────────────────────┤
│  High Address                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                     KERNEL SPACE                        │    │
│  │                   (not accessible)                      │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                        STACK                            │    │
│  │                      VMA (id=1)                         │    │
│  │              vm_start ──────► vm_end                    │    │
│  │                 ↓ grows downward                        │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                                                         │    │
│  │                    (unmapped gap)                       │    │
│  │                                                         │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                   MEMORY MAPPING                        │    │
│  │                 (shared libraries)                      │    │
│  │                      VMA (id=2)                         │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                                                         │    │
│  │                    (unmapped gap)                       │    │
│  │                                                         │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                        HEAP                             │    │
│  │                      VMA (id=0)                         │    │
│  │              vm_start ──────► sbrk ──────► vm_end       │    │
│  │                         ↑ grows upward                  │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                        BSS                              │    │
│  │                (uninitialized data)                     │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                        DATA                             │    │
│  │                 (initialized data)                      │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │                        TEXT                             │    │
│  │                    (code segment)                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│  Low Address (0x0)                                              │
└─────────────────────────────────────────────────────────────────┘
```

### 14.2 Cấu trúc VMA trong code

```c
struct vm_area_struct {
    unsigned long vm_id;      // ID của VMA (0 = heap, 1 = stack, etc.)
    addr_t vm_start;          // Địa chỉ bắt đầu của VMA
    addr_t vm_end;            // Địa chỉ kết thúc (exclusive)
    addr_t sbrk;              // Program break - điểm cuối của heap đang dùng
    
    struct mm_struct *vm_mm;  // Parent memory descriptor
    struct vm_rg_struct *vm_freerg_list;  // Danh sách vùng free trong VMA
    struct vm_area_struct *vm_next;       // Link đến VMA tiếp theo
};
```

### 14.3 VMA Operations

```
┌─────────────────────────────────────────────────────────────────┐
│                    VMA OPERATIONS                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. get_vma_by_num(mm, vmaid)                                  │
│     └── Tìm VMA theo ID trong linked list                      │
│                                                                 │
│  2. inc_vma_limit(caller, vmaid, inc_sz)                       │
│     └── Mở rộng VMA khi cần thêm memory                        │
│     └── Gọi vm_map_ram() để map pages mới                      │
│                                                                 │
│  3. get_vm_area_node_at_brk(caller, vmaid, size, alignedsz)    │
│     └── Lấy vùng tại vị trí sbrk hiện tại                      │
│     └── Dùng để allocate memory mới                            │
│                                                                 │
│  4. validate_overlap_vm_area(caller, vmaid, start, end)        │
│     └── Kiểm tra vùng mới có overlap với VMA khác không        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 14.4 VMA và Region (vm_rg_struct)

```
┌─────────────────────────────────────────────────────────────────┐
│                    VMA vs REGION                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  VMA (vm_area_struct):                                          │
│  ├── Đại diện cho một vùng VIRTUAL ADDRESS SPACE               │
│  ├── Ví dụ: Heap từ 0x1000 đến 0x5000                          │
│  └── Chứa nhiều regions                                         │
│                                                                 │
│  Region (vm_rg_struct):                                         │
│  ├── Đại diện cho một ALLOCATION cụ thể trong VMA              │
│  ├── Ví dụ: alloc(300) → region từ 0x1000 đến 0x112C          │
│  └── Có thể USED hoặc FREE                                      │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                       VMA (Heap)                        │    │
│  │  vm_start                                      vm_end   │    │
│  │     │                                             │     │    │
│  │     ▼                                             ▼     │    │
│  │  ┌──────────┬──────────┬──────────┬──────────┬───────┐  │    │
│  │  │ Region 0 │ Region 1 │   FREE   │ Region 2 │ FREE  │  │    │
│  │  │  (used)  │  (used)  │ (free_rg)│  (used)  │       │  │    │
│  │  └──────────┴──────────┴──────────┴──────────┴───────┘  │    │
│  │                                       ▲                 │    │
│  │                                      sbrk               │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
│  symrgtbl[0] → Region 0 (rg_start=0x1000, rg_end=0x112C)      │
│  symrgtbl[1] → Region 1 (rg_start=0x1130, rg_end=0x1194)      │
│  symrgtbl[2] → Region 2 (rg_start=0x1300, rg_end=0x1364)      │
│                                                                 │
│  vm_freerg_list → FREE region (0x1194 → 0x1300)               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 15. Memory Mapping (vm_map_ram)

### 15.1 vm_map_ram là gì?

`vm_map_ram` là hàm quan trọng nhất - nó **map virtual pages vào physical frames**.

```c
addr_t vm_map_ram(struct pcb_t *caller, 
                  addr_t astart,      // Virtual address start
                  addr_t aend,        // Virtual address end
                  addr_t mapstart,    // Mapping start address
                  int incpgnum,       // Number of pages to map
                  struct vm_rg_struct *ret_rg)  // Output region
```

### 15.2 Quá trình vm_map_ram

```
┌─────────────────────────────────────────────────────────────────┐
│                    vm_map_ram FLOW                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Input: Map 3 pages từ virtual address 0x1000                  │
│                                                                 │
│  Step 1: alloc_pages_range(caller, 3, &frm_lst)                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Tìm 3 free frames từ RAM                                │   │
│  │  Nếu không đủ → Swap out victim pages                    │   │
│  │  Kết quả: frm_lst = [FPN 5] → [FPN 12] → [FPN 3]        │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           │                                     │
│                           ▼                                     │
│  Step 2: vmap_page_range(caller, 0x1000, 3, frm_lst, ret_rg)   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Với mỗi page:                                           │   │
│  │    - Tính page number: pgn = addr / PAGE_SIZE           │   │
│  │    - Set PTE: pte_set_fpn(caller, pgn, fpn)             │   │
│  │    - Thêm vào FIFO list: enlist_pgn_node(&fifo_pgn, pgn)│   │
│  │                                                          │   │
│  │  Page 0 (VA 0x1000): PTE[4] = FPN 5 | PRESENT           │   │
│  │  Page 1 (VA 0x1100): PTE[5] = FPN 12 | PRESENT          │   │
│  │  Page 2 (VA 0x1200): PTE[6] = FPN 3 | PRESENT           │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           │                                     │
│                           ▼                                     │
│  Result:                                                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Virtual Space          Page Table         Physical RAM  │   │
│  │  ┌──────────┐          ┌─────────┐        ┌───────────┐  │   │
│  │  │Page 4    │────────► │PTE[4]=5 │──────► │ Frame 5   │  │   │
│  │  │VA 0x1000 │          │         │        │           │  │   │
│  │  ├──────────┤          ├─────────┤        ├───────────┤  │   │
│  │  │Page 5    │────────► │PTE[5]=12│──────► │ Frame 12  │  │   │
│  │  │VA 0x1100 │          │         │        │           │  │   │
│  │  ├──────────┤          ├─────────┤        ├───────────┤  │   │
│  │  │Page 6    │────────► │PTE[6]=3 │──────► │ Frame 3   │  │   │
│  │  │VA 0x1200 │          │         │        │           │  │   │
│  │  └──────────┘          └─────────┘        └───────────┘  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 16. mm_struct - Memory Descriptor

### 16.1 mm_struct là gì?

`mm_struct` là **bộ mô tả bộ nhớ** của một process - chứa tất cả thông tin về memory management.

```c
struct mm_struct {
    // Page Table Hierarchy
    uint64_t *pgd;   // Root của page table tree
    uint64_t *p4d;   // (Cache cho performance)
    uint64_t *pud;
    uint64_t *pmd;
    uint64_t *pt;

    // Virtual Memory Areas
    struct vm_area_struct *mmap;  // Linked list các VMAs
    
    // Symbol Table - lưu các regions đã allocate
    struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ];
    
    // Page Replacement - FIFO list
    struct pgn_t *fifo_pgn;  // Danh sách pages theo thứ tự allocation
};
```

### 16.2 Tổng quan mm_struct

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          mm_struct OVERVIEW                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         mm_struct                                   │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │                                                                     │    │
│  │  pgd ──────────────────────────────────┐                           │    │
│  │                                         │                           │    │
│  │  mmap ─────────┐                        │                           │    │
│  │                │                        │                           │    │
│  │  symrgtbl[] ───┼────┐                   │                           │    │
│  │                │    │                   │                           │    │
│  │  fifo_pgn ─────┼────┼────┐              │                           │    │
│  │                │    │    │              │                           │    │
│  └────────────────┼────┼────┼──────────────┼───────────────────────────┘    │
│                   │    │    │              │                                │
│                   ▼    │    │              ▼                                │
│  ┌─────────────────┐   │    │    ┌─────────────────────────────────────┐    │
│  │ VMA Linked List │   │    │    │        PAGE TABLE TREE              │    │
│  ├─────────────────┤   │    │    ├─────────────────────────────────────┤    │
│  │ VMA 0 (Heap)    │   │    │    │           PGD                       │    │
│  │ vm_start=0x1000 │   │    │    │          /   \                      │    │
│  │ vm_end=0x5000   │   │    │    │        P4D   P4D                    │    │
│  │ sbrk=0x3000     │   │    │    │        /       \                    │    │
│  ├─────────────────┤   │    │    │      PUD       PUD                  │    │
│  │ VMA 1 (Stack)   │   │    │    │      /           \                  │    │
│  │ vm_start=0xF000 │   │    │    │    PMD           PMD                │    │
│  │ vm_end=0xFFFF   │   │    │    │    /               \                │    │
│  └─────────────────┘   │    │    │   PT               PT               │    │
│                        │    │    │   │                 │               │    │
│                        │    │    │  PTE              PTE              │    │
│                        │    │    └─────────────────────────────────────┘    │
│                        │    │                                               │
│                        ▼    │                                               │
│  ┌─────────────────────┐    │                                               │
│  │   SYMBOL TABLE      │    │                                               │
│  ├─────────────────────┤    │                                               │
│  │ [0]: 0x1000-0x112C  │    │   (Region từ alloc 300 0)                    │
│  │ [1]: 0x1130-0x1194  │    │   (Region từ alloc 100 1)                    │
│  │ [2]: (empty)        │    │                                               │
│  │ ...                 │    │                                               │
│  │ [29]: (empty)       │    │                                               │
│  └─────────────────────┘    │                                               │
│                             │                                               │
│                             ▼                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    FIFO PAGE LIST                                   │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │  fifo_pgn → [pgn=4] → [pgn=5] → [pgn=6] → [pgn=7] → NULL          │    │
│  │              ↑                                        ↑             │    │
│  │           oldest                                   newest           │    │
│  │         (victim)                                                    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 17. Kernel Structure (krnl_t)

### 17.1 krnl_t là gì?

`krnl_t` đại diện cho **kernel context** - chứa tất cả resources được chia sẻ giữa các processes.

```c
struct krnl_t {
    struct mm_struct *mm;           // Shared memory descriptor
    struct memphy_struct *mram;     // Physical RAM
    struct memphy_struct **mswp;    // Array of swap devices
    struct memphy_struct *active_mswp;  // Current swap device
};
```

### 17.2 Quan hệ giữa các structures

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMPLETE SYSTEM OVERVIEW                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         KERNEL (krnl_t)                             │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │                                                               │  │    │
│  │  │  mm ────────────────────┐                                     │  │    │
│  │  │                         │                                     │  │    │
│  │  │  mram ─────────┐        │                                     │  │    │
│  │  │                │        │                                     │  │    │
│  │  │  mswp[] ───┐   │        │                                     │  │    │
│  │  │            │   │        │                                     │  │    │
│  │  └────────────┼───┼────────┼─────────────────────────────────────┘  │    │
│  └───────────────┼───┼────────┼─────────────────────────────────────────┘    │
│                  │   │        │                                             │
│                  │   │        ▼                                             │
│                  │   │   ┌─────────────────────────────────────────────┐    │
│                  │   │   │              mm_struct                      │    │
│                  │   │   │  ┌───────────────────────────────────────┐  │    │
│                  │   │   │  │ pgd → Page Table Tree                 │  │    │
│                  │   │   │  │ mmap → VMA List                       │  │    │
│                  │   │   │  │ symrgtbl[] → Allocated regions        │  │    │
│                  │   │   │  │ fifo_pgn → Page replacement queue     │  │    │
│                  │   │   │  └───────────────────────────────────────┘  │    │
│                  │   │   └─────────────────────────────────────────────┘    │
│                  │   │                                                      │
│                  │   ▼                                                      │
│                  │  ┌─────────────────────────────────────────────┐         │
│                  │  │           PHYSICAL RAM (mram)               │         │
│                  │  │  ┌───────────────────────────────────────┐  │         │
│                  │  │  │ storage[]: Raw byte array             │  │         │
│                  │  │  │ maxsz: Total RAM size                 │  │         │
│                  │  │  │ free_fp_list → Available frames       │  │         │
│                  │  │  │ used_fp_list → Allocated frames       │  │         │
│                  │  │  └───────────────────────────────────────┘  │         │
│                  │  └─────────────────────────────────────────────┘         │
│                  │                                                          │
│                  ▼                                                          │
│  ┌─────────────────────────────────────────────┐                            │
│  │         SWAP DEVICES (mswp[])               │                            │
│  │  ┌───────────────────────────────────────┐  │                            │
│  │  │ mswp[0]: Swap device 0                │  │                            │
│  │  │   storage[]: Swap file content        │  │                            │
│  │  │   free_fp_list → Free swap slots      │  │                            │
│  │  ├───────────────────────────────────────┤  │                            │
│  │  │ mswp[1]: Swap device 1                │  │                            │
│  │  │   ...                                 │  │                            │
│  │  └───────────────────────────────────────┘  │                            │
│  └─────────────────────────────────────────────┘                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 18. Process và Memory - Liên kết

### 18.1 PCB (Process Control Block)

```c
struct pcb_t {
    uint32_t pid;              // Process ID
    uint32_t priority;         // Scheduling priority
    struct code_seg_t *code;   // Code segment (instructions)
    addr_t regs[10];           // Registers
    uint32_t pc;               // Program Counter
    
    struct krnl_t *krnl;       // Pointer to kernel context
    // ...
};
```

### 18.2 Quan hệ Process ↔ Memory

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    PROCESS - MEMORY RELATIONSHIP                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐         ┌─────────────────┐                           │
│  │   Process 1     │         │   Process 2     │                           │
│  │  (pcb_t)        │         │  (pcb_t)        │                           │
│  │  pid = 1        │         │  pid = 2        │                           │
│  │  krnl ──────────┼────┐    │  krnl ──────────┼────┐                      │
│  └─────────────────┘    │    └─────────────────┘    │                      │
│                         │                           │                      │
│                         │                           │                      │
│                         ▼                           ▼                      │
│                    ┌─────────────────────────────────────┐                 │
│                    │        SHARED KERNEL (krnl_t)       │                 │
│                    │  ┌─────────────────────────────────┐│                 │
│                    │  │ mm → Shared mm_struct           ││                 │
│                    │  │      (tất cả processes dùng     ││                 │
│                    │  │       chung page table!)        ││                 │
│                    │  │                                 ││                 │
│                    │  │ mram → Shared RAM               ││                 │
│                    │  │ mswp → Shared Swap              ││                 │
│                    │  └─────────────────────────────────┘│                 │
│                    └─────────────────────────────────────┘                 │
│                                                                             │
│  NOTE: Trong simulation này, tất cả processes SHARE cùng mm_struct!        │
│        Trong OS thực, mỗi process có mm_struct RIÊNG.                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 19. Instruction Flow - Từ Process đến Memory

### 19.1 Complete Flow của `alloc`

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMPLETE ALLOC FLOW                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Process executes: alloc 300 0                                              │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │ 1. CPU (cpu.c - run())                                              │    │
│  │    ├── Fetch instruction: ins = code->text[pc]                      │    │
│  │    ├── pc++                                                         │    │
│  │    └── Switch(ALLOC) → liballoc(proc, 300, 0)                      │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                           │                                                 │
│                           ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │ 2. Library (libmem.c - liballoc())                                  │    │
│  │    ├── Lock mutex (thread safety)                                   │    │
│  │    ├── __alloc(caller, vmaid=0, rgid=0, size=300, &alloc_addr)     │    │
│  │    ├── Print "liballoc:XXX"                                         │    │
│  │    ├── print_pgtbl() if PAGETBL_DUMP                               │    │
│  │    └── Unlock mutex                                                 │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                           │                                                 │
│                           ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │ 3. VM Layer (mm64.c - __alloc())                                    │    │
│  │    ├── get_free_vmrg_area() - tìm free region                      │    │
│  │    │   └── Không tìm thấy → inc_vma_limit()                        │    │
│  │    ├── inc_vma_limit(caller, vmaid, 300)                           │    │
│  │    │   ├── Align size: 300 → 512 (2 pages × 256)                   │    │
│  │    │   ├── cur_vma->vm_end += 512                                   │    │
│  │    │   ├── cur_vma->sbrk += 300                                     │    │
│  │    │   └── vm_map_ram() - map 2 pages                              │    │
│  │    └── Lưu vào symrgtbl[0]                                         │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                           │                                                 │
│                           ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │ 4. Page Mapping (mm64.c - vm_map_ram())                             │    │
│  │    ├── alloc_pages_range(caller, 2, &frm_lst)                      │    │
│  │    │   ├── MEMPHY_get_freefp(mram) × 2                             │    │
│  │    │   └── Nếu hết RAM → find_victim_page() + swap                 │    │
│  │    └── vmap_page_range(caller, addr, 2, frm_lst)                   │    │
│  │        ├── pte_set_fpn(caller, pgn0, fpn0)                         │    │
│  │        ├── pte_set_fpn(caller, pgn1, fpn1)                         │    │
│  │        ├── enlist_pgn_node(&fifo_pgn, pgn0)                        │    │
│  │        └── enlist_pgn_node(&fifo_pgn, pgn1)                        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                           │                                                 │
│                           ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │ 5. Page Table Update (mm64.c - pte_set_fpn())                       │    │
│  │    ├── get_pd_from_pagenum(pgn) → extract indices                  │    │
│  │    ├── TREE WALK with lazy allocation:                             │    │
│  │    │   ├── PGD[idx] not present? → alloc_page_table()             │    │
│  │    │   ├── P4D[idx] not present? → alloc_page_table()             │    │
│  │    │   ├── PUD[idx] not present? → alloc_page_table()             │    │
│  │    │   ├── PMD[idx] not present? → alloc_page_table()             │    │
│  │    │   └── PT[idx] = fpn | PRESENT_BIT                            │    │
│  │    └── Done!                                                        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  Result: Virtual addresses 0x1000-0x11FF now mapped to physical frames     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 20. Tổng kết - Cheat Sheet

### 20.1 Files và Chức năng

| File | Chức năng chính |
|------|-----------------|
| `os-cfg.h` | Cấu hình: MM_PAGING, MM64, IODUMP, PAGETBL_DUMP |
| `mm.h` | Constants: page size, PTE format, macros |
| `mm64.h` | 5-level page table definitions |
| `os-mm.h` | Data structures: mm_struct, vma, memphy |
| `mm64.c` | Page table operations, __alloc, __read, __write |
| `mm-vm.c` | VMA management, inc_vma_limit |
| `mm-memphy.c` | Physical frame management |
| `libmem.c` | User API: liballoc, libfree, libread, libwrite |
| `cpu.c` | Instruction execution: run(), calc(), etc. |

### 20.2 Key Functions

```
liballoc() → __alloc() → inc_vma_limit() → vm_map_ram() → alloc_pages_range() → pte_set_fpn()
                                                              ↓
                                                    MEMPHY_get_freefp()
                                                              ↓
                                                    (if no free) find_victim_page() → swap

libread() → __read() → pg_getval() → pte_get_entry() → MEMPHY_read()
                              ↓
                    (if swapped) pg_getpage() → swap in

libwrite() → __write() → pg_setval() → pte_get_entry() → MEMPHY_write()
                               ↓
                     (if swapped) pg_getpage() → swap in
```

### 20.3 Memory Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│                    MEMORY HIERARCHY                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ VIRTUAL (Process View)                                  │    │
│  │   alloc/free/read/write operate on VIRTUAL addresses   │    │
│  └────────────────────────────┬────────────────────────────┘    │
│                               │                                 │
│                          Page Table                             │
│                          Translation                            │
│                               │                                 │
│  ┌────────────────────────────▼────────────────────────────┐    │
│  │ PHYSICAL (Hardware View)                                │    │
│  │   Actual data stored in RAM frames                      │    │
│  └────────────────────────────┬────────────────────────────┘    │
│                               │                                 │
│                            Swap                                 │
│                          (if needed)                            │
│                               │                                 │
│  ┌────────────────────────────▼────────────────────────────┐    │
│  │ SWAP (Disk)                                             │    │
│  │   Pages that don't fit in RAM                           │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```
