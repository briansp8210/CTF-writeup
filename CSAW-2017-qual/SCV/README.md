# SCV

> Arch:     amd64-64-little  
> RELRO:    Partial RELRO  
> Stack:    Canary found  
> NX:       NX enabled  
> PIE:      No PIE (0x400000)

## Analysis

```
-------------------------
[*]SCV GOOD TO GO,SIR....
-------------------------
1.FEED SCV....
2.REVIEW THE FOOD....
3.MINE MINERALS....
-------------------------
>>
```

* `FEED SCV` will ask us input something, which is long enough to overwrite return address
* `REVIEW THE FOOD` will print our input using `puts`
* `MINE MINERALS` to return from `main`

## Exploit

My plan is building ROP chain to leak libc address from GOT, then returning back to `main`, doing ROP again. While this time, we can build `system("/bin/sh")` to open shell.  
Before we can do this, stack canary must be handled. I first overwrote the last byte of canary, which is definitely a null byte, then used `REVIEW THE FOOD` to print the content along with canary.  
With canary, we can do aforementioned operation to open a shell.

flag: `flag{sCv_0n1y_C0st_50_M!n3ra1_tr3at_h!m_we11}`
