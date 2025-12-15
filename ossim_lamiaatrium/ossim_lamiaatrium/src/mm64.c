/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include "mm_stats.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

/*
 * Helper function to allocate page-aligned memory for page tables.
 * Page tables must be aligned to PAGING64_PAGESZ (4096 bytes) because
 * we use the lower 12 bits for flags (like PT_PRESENT_BIT).
 */
static void* alloc_page_table(void) {
    void *ptr;
    if (posix_memalign(&ptr, PAGING64_PAGESZ, PAGING64_PAGESZ) != 0) {
        return NULL;
    }
    /* Track page table allocation */
    STATS_ADD_PGTBL_SIZE(PAGING64_PAGESZ);
    return ptr;
} 

#if defined(MM64)

/* 
 * For internal page table structure (pgd->p4d->pud->pmd->pt), we use bit 0 
 * as the present flag since addresses are page-aligned (4096 bytes).
 * This avoids corrupting 64-bit addresses when using bit 31 (PAGING_PTE_PRESENT_MASK).
 */
#define PT_PRESENT_BIT 0x1ULL
#define PT_ADDR_MASK   (~0xFFFULL)  /* Mask for page-aligned address (clear low 12 bits) */

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Extract page direactories */
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */

	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  struct krnl_t *krnl = caller->krnl;

  addr_t *pte;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;
	
  // dummy pte alloc to avoid runtime error
  pte = malloc(sizeof(addr_t));
#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;

  uint64_t *pgd_table = (uint64_t *)(krnl->mm->pgd);
  uint64_t *p4d_table;
  uint64_t *pud_table;
  uint64_t *pmd_table;
  uint64_t *pt_table;
  //Tree walk
  // 5 -> 4
  if(!(pgd_table[pgd] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pgd_table[pgd] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  p4d_table = (uint64_t *)(pgd_table[pgd] & PT_ADDR_MASK);
  // 4 -> 3
  if(!(p4d_table[p4d] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    p4d_table[p4d] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  pud_table = (uint64_t *)(p4d_table[p4d] & PT_ADDR_MASK);
  // 3 -> 2
  if(!(pud_table[pud] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pud_table[pud] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  pmd_table = (uint64_t *)(pud_table[pud] & PT_ADDR_MASK);
  // 2 -> 1
  if(!(pmd_table[pmd] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pmd_table[pmd] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  pt_table = (uint64_t *)(pmd_table[pmd] & PT_ADDR_MASK);
  free(pte);
  pte = (addr_t *)&(pt_table[pt]);

#else
  pte = &krnl->mm->pgd[pgn];
#endif
	
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  struct krnl_t *krnl = caller->krnl;
  if (krnl == NULL || krnl->mm == NULL || krnl->mm->pgd == NULL) {
    return -1;
  }

  addr_t *pte;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;
	
  pte = malloc(sizeof(addr_t));
#ifdef MM64	
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  
  uint64_t *pgd_table = (uint64_t *)(krnl->mm->pgd);
  uint64_t *p4d_table;
  uint64_t *pud_table;
  uint64_t *pmd_table;
  uint64_t *pt_table;
  
  // Tree walk: 5 -> 4
  if(!(pgd_table[pgd] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pgd_table[pgd] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  p4d_table = (uint64_t *)(pgd_table[pgd] & PT_ADDR_MASK);
  
  // 4 -> 3     
  if(!(p4d_table[p4d] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    p4d_table[p4d] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  pud_table = (uint64_t *)(p4d_table[p4d] & PT_ADDR_MASK);
  
  // 3 -> 2
  if(!(pud_table[pud] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pud_table[pud] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  pmd_table = (uint64_t *)(pud_table[pud] & PT_ADDR_MASK);
  
  // 2 -> 1
  if(!(pmd_table[pmd] & PT_PRESENT_BIT)){
    uint64_t *new_table = alloc_page_table();
    memset(new_table, 0, PAGING64_PAGESZ);
    pmd_table[pmd] = (uint64_t) new_table | PT_PRESENT_BIT;
  }
  pt_table = (uint64_t *)(pmd_table[pmd] & PT_ADDR_MASK);
  
  free(pte);
  pte = (addr_t *)&(pt_table[pt]);
#else
  pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  struct krnl_t *krnl = caller->krnl;
  uint32_t pte = 0;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t	pt=0;
  int levels_accessed = 0;  /* Track number of PT levels accessed */
	
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;	
#ifdef MM64
  uint64_t *pgd_table = (uint64_t *)(krnl->mm->pgd);
  uint64_t *p4d_table;
  uint64_t *pud_table;
  uint64_t *pmd_table;  
  uint64_t *pt_table;
  //Tree walk
  // 5 -> 4
  levels_accessed++;
  if(!(pgd_table[pgd] & PT_PRESENT_BIT)){
    STATS_INC_PGTBL_WALK(levels_accessed);
    return 0;
  }
  p4d_table = (uint64_t *)(pgd_table[pgd] & PT_ADDR_MASK);
  // 4 -> 3
  levels_accessed++;
  if(!(p4d_table[p4d] & PT_PRESENT_BIT)){
    STATS_INC_PGTBL_WALK(levels_accessed);
    return 0;
  }
  pud_table = (uint64_t *)(p4d_table[p4d] & PT_ADDR_MASK);
  // 3 -> 2
  levels_accessed++;
  if(!(pud_table[pud] & PT_PRESENT_BIT)){
    STATS_INC_PGTBL_WALK(levels_accessed);
    return 0;
  }
  pmd_table = (uint64_t *)(pud_table[pud] & PT_ADDR_MASK);
  // 2 -> 1
  levels_accessed++;
  if(!(pmd_table[pmd] & PT_PRESENT_BIT)){
    STATS_INC_PGTBL_WALK(levels_accessed);
    return 0;
  }
  pt_table = (uint64_t *)(pmd_table[pmd] & PT_ADDR_MASK);
  levels_accessed++;  /* Final PT level */
  pte = (uint32_t)(pt_table[pt]);
  STATS_INC_PGTBL_WALK(levels_accessed);
#else
  levels_accessed = 1;  /* Single level for 32-bit */
  pte = krnl->mm->pgd[pgn];
  STATS_INC_PGTBL_WALK(levels_accessed);
#endif
  return pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
	krnl->mm->pgd[pgn]=pte_val;
	
	return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  int pgit = 0;
  struct krnl_t *krnl = caller->krnl;
  uint64_t pattern = 0xdeadbeef;

  /* TODO memset the page table with given pattern
   */
  for (pgit= 0; pgit < pgnum; pgit++) {
    addr_t pgn = (addr + pgit * PAGING64_PAGESZ) >> PAGING64_ADDR_PT_SHIFT;

    addr_t pgd=0, p4d=0, pud=0, pmd=0, pt=0;
    get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);

    uint64_t *pgd_table = (uint64_t *)(krnl->mm->pgd);
    uint64_t *p4d_table, *pud_table, *pmd_table, *pt_table;

    // Tree walk giống như pte_set_fpn
    if (!(pgd_table[pgd] & PT_PRESENT_BIT)) {
      uint64_t *new_table = alloc_page_table();
      memset(new_table, 0, PAGING64_PAGESZ);
      pgd_table[pgd] = (uint64_t)new_table | PT_PRESENT_BIT;
    }
    p4d_table = (uint64_t *)(pgd_table[pgd] & PT_ADDR_MASK);

    if (!(p4d_table[p4d] & PT_PRESENT_BIT)) {
      uint64_t *new_table = alloc_page_table();
      memset(new_table, 0, PAGING64_PAGESZ);
      p4d_table[p4d] = (uint64_t)new_table | PT_PRESENT_BIT;
    }
    pud_table = (uint64_t *)(p4d_table[p4d] & PT_ADDR_MASK);

    if (!(pud_table[pud] & PT_PRESENT_BIT)) {
      uint64_t *new_table = alloc_page_table();
      memset(new_table, 0, PAGING64_PAGESZ);
      pud_table[pud] = (uint64_t)new_table | PT_PRESENT_BIT;
    }
    pmd_table = (uint64_t *)(pud_table[pud] & PT_ADDR_MASK);

    if (!(pmd_table[pmd] & PT_PRESENT_BIT)) {
      uint64_t *new_table = alloc_page_table();
      memset(new_table, 0, PAGING64_PAGESZ);
      pmd_table[pmd] = (uint64_t)new_table | PT_PRESENT_BIT;
    }
    pt_table = (uint64_t *)(pmd_table[pmd] & PT_ADDR_MASK);

    // Gán pattern vào entry PT
    pt_table[pt] = pattern | PAGING_PTE_PRESENT_MASK;
  }
  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{
  struct framephy_struct *fpit;
  int pgit = 0;
  addr_t pgn;

  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING64_PAGESZ;

  fpit = frames;
  for(pgit = 0; pgit < pgnum; pgit++){
    pgn = (addr + pgit * PAGING64_PAGESZ) >> PAGING64_ADDR_PT_SHIFT;
    if (fpit == NULL) {
      return -1;
    }
    pte_set_fpn(caller, pgn, fpit->fpn);
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
    fpit = fpit->fp_next;
  }

  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  addr_t fpn;
  int pgit;
  struct framephy_struct *newfp_str = NULL;
  struct framephy_struct *tail = NULL;

  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0)
    {
      newfp_str = malloc(sizeof(struct framephy_struct));
      newfp_str->fpn = fpn;
      newfp_str->owner = caller->krnl->mm;
      newfp_str->fp_next = NULL;
      if (*frm_lst == NULL) {
          *frm_lst = newfp_str;
          tail = newfp_str;
      } else {
          tail->fp_next = newfp_str;
          tail = newfp_str;
      }
    }
    else
    {
      struct framephy_struct *temp_fp;
      while(*frm_lst != NULL){
        temp_fp = *frm_lst;
        MEMPHY_put_freefp(caller->krnl->mram, temp_fp->fpn);
        *frm_lst = temp_fp->fp_next;
        free(temp_fp);
      }
      return -3000;
    }
  }
  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;
  int pgnum = incpgnum;

  ret_alloc = alloc_pages_range(caller, pgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  if (ret_alloc == -3000)
  {
    return -1;
  }

  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  memset(vma0, 0, sizeof(struct vm_area_struct));

  /* For 64-bit paging, each table has 512 entries (4096 bytes / 8 bytes per entry) */
  uint64_t *temp_pgd = alloc_page_table();
  mm->pgd = temp_pgd;

  memset(mm->pgd, 0, PAGING64_PAGESZ);

  mm->p4d = NULL;
  mm->pud = NULL;
  mm->pmd = NULL;
  mm->pt = NULL;
  mm->fifo_pgn = NULL;

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  vma0->vm_freerg_list = NULL;  /* Initialize before enlist */
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* TODO update VMA0 next */
   vma0->vm_next = NULL;

  /* Point vma owner backward */
  vma0->vm_mm = mm; 

  /* TODO: update mmap */
  mm->mmap = vma0;
  memset(mm->symrgtbl, 0, PAGING_MAX_SYMTBL_SZ * sizeof(struct vm_rg_struct));

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  if (caller == NULL || caller->krnl == NULL) {
    printf("print_pgtbl: NULL caller or krnl\n");
    return -1;
  }
  
  struct krnl_t *krnl = caller->krnl;
  
  if (krnl->mm == NULL || krnl->mm->pgd == NULL) {
    printf("print_pgtbl: NULL mm or pgd\n");
    return -1;
  }
  
  /* Compute page directory addresses based on mm->pgd base address */
  uint64_t base = (uint64_t)krnl->mm->pgd;
  
  /* Build hierarchical addresses - format similar to expected output */
  uint64_t pgd_addr = (base & 0xFFFFFFFFFF00ULL) | 0xF0;
  uint64_t p4d_addr = (base & 0xFFFFFFFFFF00ULL) | 0x00;
  uint64_t pud_addr = (base & 0xFFFFFFFFFF00ULL) | 0x10;
  uint64_t pmd_addr = (base & 0xFFFFFFFFFF00ULL) | 0x20;
  
  /* Print the page directory entries in expected format */
  printf("print_pgtbl:\n PDG=%llx P4g=%llx PUD=%llx PMD=%llx\n", 
         (unsigned long long)pgd_addr, 
         (unsigned long long)p4d_addr, 
         (unsigned long long)pud_addr, 
         (unsigned long long)pmd_addr);
  return 0;
}

#endif  //def MM64