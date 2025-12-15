/*
 * Memory Management Statistics Implementation
 * For tracking and comparing different paging strategies
 */

#include "mm_stats.h"
#include "mm64.h"
#include <stdio.h>
#include <string.h>

/* Global statistics instance */
struct mm_stats global_mm_stats;
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Initialize statistics to zero
 */
void mm_stats_init(void)
{
    pthread_mutex_lock(&stats_lock);
    memset(&global_mm_stats, 0, sizeof(struct mm_stats));
    pthread_mutex_unlock(&stats_lock);
}

/*
 * Reset statistics to zero
 */
void mm_stats_reset(void)
{
    mm_stats_init();
}

/*
 * Print statistics report
 */
void mm_stats_print(void)
{
    pthread_mutex_lock(&stats_lock);
    
    printf("\n");
    printf("================================================================================\n");
    printf("                    MEMORY MANAGEMENT STATISTICS REPORT                         \n");
    printf("================================================================================\n");
    
    printf("\n--- Memory Access Statistics ---\n");
    printf("  Total memory reads:          %llu\n", (unsigned long long)global_mm_stats.mem_read_count);
    printf("  Total memory writes:         %llu\n", (unsigned long long)global_mm_stats.mem_write_count);
    printf("  Total memory accesses:       %llu\n", 
           (unsigned long long)(global_mm_stats.mem_read_count + global_mm_stats.mem_write_count));
    
    printf("\n--- Page Table Walk Statistics ---\n");
    printf("  Total page table walks:      %llu\n", (unsigned long long)global_mm_stats.pgtbl_walk_count);
    printf("  Total PT levels accessed:    %llu\n", (unsigned long long)global_mm_stats.pgtbl_levels_accessed);
    if (global_mm_stats.pgtbl_walk_count > 0) {
        printf("  Avg levels per walk:         %.2f\n", 
               (double)global_mm_stats.pgtbl_levels_accessed / global_mm_stats.pgtbl_walk_count);
    }
    
    printf("\n--- Page Fault & Swap Statistics ---\n");
    printf("  Total page faults:           %llu\n", (unsigned long long)global_mm_stats.page_fault_count);
    printf("  Pages swapped in:            %llu\n", (unsigned long long)global_mm_stats.swap_in_count);
    printf("  Pages swapped out:           %llu\n", (unsigned long long)global_mm_stats.swap_out_count);
    
    printf("\n--- Page Table Storage Statistics ---\n");
    printf("  Page tables allocated:       %llu\n", (unsigned long long)global_mm_stats.pgtbl_tables_count);
    printf("  Total PT storage size:       %llu bytes", (unsigned long long)global_mm_stats.pgtbl_storage_size);
    if (global_mm_stats.pgtbl_storage_size >= 1024) {
        printf(" (%.2f KB)", (double)global_mm_stats.pgtbl_storage_size / 1024.0);
    }
    printf("\n");
    
    printf("\n--- Frame Allocation Statistics ---\n");
    printf("  Frames allocated:            %llu\n", (unsigned long long)global_mm_stats.frames_allocated);
    printf("  Frames freed:                %llu\n", (unsigned long long)global_mm_stats.frames_freed);
    printf("  Frames in use:               %llu\n", 
           (unsigned long long)(global_mm_stats.frames_allocated - global_mm_stats.frames_freed));
    
    /* Design comparison analysis */
    printf("\n--- Design Analysis ---\n");
#ifdef MM64
    printf("  Paging mode:                 5-Level (64-bit)\n");
    printf("  Page size:                   %d bytes\n", PAGING64_PAGESZ);
    printf("  Levels: PGD -> P4D -> PUD -> PMD -> PT\n");
    
    /* Calculate overhead */
    uint64_t total_accesses = global_mm_stats.mem_read_count + global_mm_stats.mem_write_count;
    if (total_accesses > 0) {
        double overhead_ratio = (double)global_mm_stats.pgtbl_levels_accessed / total_accesses;
        printf("  PT walk overhead ratio:      %.2f levels/access\n", overhead_ratio);
    }
    
    /* Storage efficiency */
    if (global_mm_stats.pgtbl_tables_count > 0) {
        double avg_table_size = (double)global_mm_stats.pgtbl_storage_size / global_mm_stats.pgtbl_tables_count;
        printf("  Avg table size:              %.0f bytes\n", avg_table_size);
    }
#else
    printf("  Paging mode:                 Single-Level (32-bit)\n");
    printf("  Page size:                   %d bytes\n", PAGING_PAGESZ);
#endif

    printf("\n================================================================================\n");
    
    pthread_mutex_unlock(&stats_lock);
}
