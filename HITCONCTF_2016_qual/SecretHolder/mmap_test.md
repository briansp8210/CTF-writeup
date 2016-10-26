# mmap and malloc test

```c
#include <stdlib.h>

int main(void)
{
    int *p, *q, *r, *s, *t, *u;

    p = malloc(0x32000);
    free(p);
    q = malloc(0x32000);
    free(q);
    r = malloc(0x40000);
    free(r);
    s = malloc(0x40000);
    free(s);
    t = malloc(0x64000);
    free(t);
    u = malloc(0x64000);
    free(u);
	
    return 0;
}
```

## 利用 ltrace 測試結果
```
__libc_start_main(0x400536, 1, 0x7ffca9664a78, 0x4005f0 <unfinished ...>
malloc(204800)                                          = 0x7fa82df73010
free(0x7fa82df73010)                                    = <void>
malloc(204800)                                          = 0x69f010
free(0x69f010)                                          = <void>
malloc(262144)                                          = 0x69f010
free(0x69f010)                                          = <void>
malloc(262144)                                          = 0x69f010
free(0x69f010)                                          = <void>
malloc(409600)                                          = 0x7fa82df41010
free(0x7fa82df41010)                                    = <void>
malloc(409600)                                          = 0x69f010
free(0x69f010)                                          = <void>
+++ exited (status 0) +++
```

可以注意到第一次分配大空間並 free 掉之後，接下來再要相同或稍大的空間時，都會拿到 heap 中的空間。不過若是再要更大一點的空間，又會由 `mmap` 來分配。不過 free 掉後再要一次又會拿到 heap 裡的空間。
和 malloc.c 裡面的說明相對照，應該是有一個門檻，只要要求的大小過了該門檻就會先用 `mmap` 來分配，並在該空間被 free 掉後提高門檻。
malloc.c 中的註解：

```
The threshold goes up in value when the application frees memory that was
allocated with the mmap allocator. The idea is that once the application
starts freeing memory of a certain size, it's highly probable that this is
a size the application uses for transient allocations. This estimator
is there to satisfy the new third requirement.
```

大概是這樣，如果有錯誤的話還請務必指出 XD
