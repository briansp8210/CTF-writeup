# overflow

```
Arch:     amd64-64-little
RELRO:    Full RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      No PIE (0x400000)
FORTIFY:  Enabled
```

## Analysis

這支程式提供了三種不同的 overflow，其中針對 `stack` 以及 `heap` 的部分分別會用 `alloca` 以及 `malloc` 來分配空間，至於 `bss` 就直接輸入到一個 global buffer。

```
************************
        Overflow
************************
 1. Stack Overflow
 2. Heap Overflow
 3. BSS Overflow
 4. Exit
************************
```

由於這三種 overflow 都是由 `gets` 造成的，所以可以攻擊的點很多。不過因為程式本身沒有什麼輸出可以 leak 重要的位址資訊，也沒辦法直接利用。  
後來想到如果可以繞過 stack canary，就可以疊 ROP 來做事，所以問題就剩如何在沒有 information leak 的狀況下做到這件事。  
這裡我用了一個之前沒試過的技巧 。首先可以注意到，canary 是從 `fs:0x28` 拿出來存放及做檢查的：

```
0x400b93 <main+8>       mov    rax,QWORD PTR fs:0x28  
0x400b9c <main+17>      mov    QWORD PTR [rbp-0x8],rax
...
0x400d0f <main+388>     mov    rax,QWORD PTR [rbp-0x8]
0x400d13 <main+392>     xor    rax,QWORD PTR fs:0x28  
```

在 `x86_64` 底下，`fs` 會指向在 `tls` 段中的 [`tcbhead_t`](https://code.woboq.org/userspace/glibc/sysdeps/x86_64/nptl/tls.h.html#42) 結構 (如下所示)，其中的 `stack_guard` 就是 canary。所以只要有辦法寫到這個位址，就可以成功繞過 canary 檢查。

```c
typedef struct
{
  void *tcb;        /* Pointer to the TCB.  Not necessarily the
               thread descriptor used by libpthread.  */
  dtv_t *dtv;
  void *self;       /* Pointer to the thread descriptor.  */
  int multiple_threads;
  int gscope_flag;
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
  unsigned long int vgetcpu_cache[2];
# ifndef __ASSUME_PRIVATE_FUTEX
  int private_futex;
# else
  int __glibc_reserved1;
# endif
  int __glibc_unused1;
  /* Reservation of some values for the TM ABI.  */
  void *__private_tm[4];
  /* GCC split stack support.  */
  void *__private_ss;
  long int __glibc_reserved2;
  /* Must be kept even if it is no longer used by glibc since programs,
     like AddressSanitizer, depend on the size of tcbhead_t.  */
  __128bits __glibc_unused2[8][4] __attribute__ ((aligned (32)));

  void *__padding[8];
} tcbhead_t;
```

接下來的問題就是如何覆寫到這個區段。這裡可以利用 glibc `malloc` 實作的一個[性質](https://code.woboq.org/userspace/glibc/malloc/malloc.c.html#2291)，當要求的空間大於某個門檻後，系統會改用 `mmap` 來配置空間，不是從 `heap` 拿，而這塊空間會接在 `tls` 的前面。因此只要有一定大小的 overflow 可以利用，就有機會蓋到上述結構中的內容。

```c
/*
   If have mmap, and the request size meets the mmap threshold, and
   the system supports mmap, and there are few enough currently
   allocated mmapped regions, try to directly map this request
   rather than expanding top.
 */

if (av == NULL
    || ((unsigned long) (nb) >= (unsigned long) (mp_.mmap_threshold)
    && (mp_.n_mmaps < mp_.n_mmaps_max)))
  {
    char *mm;           /* return value from mmap call*/

  try_mmap:
    /*
       Round up size to nearest page.  For mmapped chunks, the overhead
       is one SIZE_SZ unit larger than for normal chunks, because there
       is no following chunk whose prev_size field could be used.

       See the front_misalign handling below, for glibc there is no
       need for further alignments unless we have have high alignment.
     */
    if (MALLOC_ALIGNMENT == 2 * SIZE_SZ)
      size = ALIGN_UP (nb + SIZE_SZ, pagesize);
    else
      size = ALIGN_UP (nb + SIZE_SZ + MALLOC_ALIGN_MASK, pagesize);
    tried_mmap = true;

    /* Don't try if size wraps around 0 */
    if ((unsigned long) (size) > (unsigned long) (nb))
      {
        mm = (char *) (MMAP (0, size, PROT_READ | PROT_WRITE, 0));
        ...
```

## Exploit

首先我利用 `Stack Overflow` 把用於 information leak 以及 stack migration 的 ROP 寫上去。先做這步是因為等一下在蓋 `tls` 的內容時會壓壞一些東西，導致之後某些 function call 會壞掉。  
接著就用 `Heap overflow`，要求一塊很大的空間，並且利用上述的特性將 `tls` 中的 canary 蓋成剛才寫在 stack 上的 canary 的值。  
如此一來，`main` return 前檢查 canary 時就會順利通過，並接著跑 ROP 印出 libc 位址、將第二段 ROP 讀去 `.bss`，最後 stack migration 過去執行 `system("/bin/sh")` 開 shell。

flag: `FLAG{h3ap_and_st4ck_0v3rfl0w_1s_4asy_4_U}`
