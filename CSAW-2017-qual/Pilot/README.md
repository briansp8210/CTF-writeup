# Pilot

> Arch:     amd64-64-little  
> RELRO:    Partial RELRO  
> Stack:    No canary found  
> NX:       NX disabled  
> PIE:      No PIE (0x400000)  
> RWX:      Has RWX segments

## Exploit

The address printed after `[*]Location:` is address of input buffer. Besides, there is a stack overflow vulnerability.  
Thus we can write shellcode to buffer and do return to shellcode to get shell.

flag: `flag{1nput_c00rd1nat3s_Strap_y0urse1v3s_1n_b0ys}`
