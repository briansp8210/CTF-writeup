# magicheap2

```
Arch:     amd64-64-little
RELRO:    Partial RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      No PIE (0x400000)
```

## Analysis

程式一開始會要我們輸入名字，並且動態分配一塊空間用來紀錄等一下創建的空間 (以下稱這個指標為 `pool_ptr`)。接下來就進入選單模式，功能大致如下：  

```
--------------------------------
       Magic Heap Creator
--------------------------------
 1. Create a Heap
 2. Edit a Heap
 3. Delete a Heap
 4. Exit
--------------------------------
Your choice :
```

* `Create`：分配一塊使用者指定大小的空間，並接受該大小的輸入至該空間
* `Edit`：接受使用者輸入新的內容至先前分配的空間，輸入長度是新指定的，並且沒有對應的檢查，因此這裡可以進行直接的 heap overflow
* `Delete`：釋放掉一塊先前分配的空間

所以重點會在如何利用這個 heap overflow 來做事，我是利用了 fastbin corruption。

## Exploit

首先，在輸入名字的時候我構造了一些內容進去 (下圖**粗體**部分)，而在 `name_buf` 後面的就是前面提到的 `pool_ptr`：

<pre>
                  free@got.plt           malloc@got.plt
                        ↓                       ↓
0x6020a0:       <b>0x0000000000602018</b>      <b>0x0000000000602050</b>
0x6020b0:    -→ <b>0x0000000000000021</b>      0x0000000001c83010
0x6020c0:    |  0x0000000000000000      0x0000000000000000
             |
      fake chunk size
</pre>

接著我利用 fastbin corruption，取得位於 `name_buf+8` 的 fake chunk，並在輸入內容的時候將 `name_ptr` 的值蓋成 `name`。如此一來我就能夠利用 `Edit` 功能來修改我一開始擺在 `name` 的指標指向的內容，從而做到 GOT hijacking。  
這裡我是先用 `Edit(0)` 來把 `free@got.plt` 蓋成 `puts@plt`，這樣我就可以用 `Delete(1)` 來 leak 出 `malloc` 的位址，並取得 libc base。最後我再次使用 `Edit(0)` 將 `free@got.plt` 蓋成 one gadget，然後使用 `Delete` 功能觸發 `free` 來開 shell。

flag: `FLAG{h34p_0verfl0w_is_e4ay_for_u}`
