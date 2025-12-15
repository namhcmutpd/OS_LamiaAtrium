/*
 * Memory Management Statistics
 * For tracking and comparing different paging strategies
 */

#ifndef MM_STATS_H
#define MM_STATS_H

#include <stdint.h>
#include <pthread.h>

/* Statistics structure for memory management */
struct mm_stats {
    /* Memory access counters */
    uint64_t mem_read_count;       /* Number of memory read operations */
    uint64_t mem_write_count;      /* Number of memory write operations */
    
    /* Page table walk counters */
    uint64_t pgtbl_walk_count;     /* Number of page table walks */
    uint64_t pgtbl_levels_accessed; /* Total page table levels accessed */
    
    /* Page fault and swap counters */
    uint64_t page_fault_count;     /* Number of page faults */
    uint64_t swap_in_count;        /* Number of pages swapped in */
    uint64_t swap_out_count;       /* Number of pages swapped out */
    
    /* Page table storage size (bytes) */
    uint64_t pgtbl_storage_size;   /* Total bytes allocated for page tables */
    uint64_t pgtbl_tables_count;   /* Number of page tables allocated */
    
    /* Frame allocation */
    uint64_t frames_allocated;     /* Number of frames allocated */
    uint64_t frames_freed;         /* Number of frames freed */
};

/* Global statistics instance */
extern struct mm_stats global_mm_stats;
extern pthread_mutex_t stats_lock;

/* Statistics functions */
void mm_stats_init(void);
void mm_stats_print(void);
void mm_stats_reset(void);

/* Increment macros (thread-safe) */
#define STATS_INC_MEM_READ() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.mem_read_count++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_MEM_WRITE() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.mem_write_count++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_PGTBL_WALK(levels) do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.pgtbl_walk_count++; \
    global_mm_stats.pgtbl_levels_accessed += (levels); \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_PAGE_FAULT() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.page_fault_count++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_SWAP_IN() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.swap_in_count++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_SWAP_OUT() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.swap_out_count++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_ADD_PGTBL_SIZE(bytes) do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.pgtbl_storage_size += (bytes); \
    global_mm_stats.pgtbl_tables_count++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_FRAME_ALLOC() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.frames_allocated++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#define STATS_INC_FRAME_FREE() do { \
    pthread_mutex_lock(&stats_lock); \
    global_mm_stats.frames_freed++; \
    pthread_mutex_unlock(&stats_lock); \
} while(0)

#endif /* MM_STATS_H */
