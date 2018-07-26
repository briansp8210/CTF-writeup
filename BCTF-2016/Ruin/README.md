# Ruin

```
Arch:     arm-32-little
RELRO:    No RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      No PIE (0x8000)
```

## Analysis

We can conduct multiple operation after entering the hardcoded password, like edit secret and name. There is an obvious heap overflow in the secret update function.

```c
void edit_secret()
{
    if ( !secret )
        secret = (char *)malloc(8);
    printf("please input your secret:");
    fgets(secret, 24, stdin);   /* heap overflow */
}
```

It worth noting that the only place we can call `free` directly is the `leave & return` function, which will exit the program after freeing 3 chunks.

## Exploit

First, notice that the `secret` pointer is placed just after `key`, hence we can leak `secret` by fill all 8-bytes of `key` to get heap address.  
After that, with the heap overflow and following boundary checking vulnerability, I chose to utilize *house of force* to exploit the challenge.

```c
printf("please input your name length:");
size = read_num();
/* Only checking upper bound, negative size is acceptable */
if ( size > 32 )
{
    puts("how could you get such a long name ?!");
    exit(1);
}
name = (char *)malloc(size);
printf("enter your name:");
fgets(name, size, stdin);
```

Since the challenge didn't come with the libc of target machine, instead of getting chunk among GOT, I got chunk among dynamic section (this binary is *NO RELRO*) so that I can overwrite `strtab`.  
I chose `free` as target, whose offset in `strtab` is 0x79.

```
Dynamic section at offset 0xe50 contains 26 entries:
  Tag        Type                         Name/Value
...
 0x6ffffef5 (GNU_HASH)                   0x8228
 0x00000005 (STRTAB)                     0x83b4
 0x00000006 (SYMTAB)                     0x8294
...
```

<pre>
0000000: 006c 6962 632e 736f 2e36 0065 7869 7400  .libc.so.6.exit.
0000010: 7374 726e 636d 7000 7075 7473 005f 5f73  strncmp.puts.__s
0000020: 7461 636b 5f63 686b 5f66 6169 6c00 6162  tack_chk_fail.ab
0000030: 6f72 7400 7374 6469 6e00 7072 696e 7466  ort.stdin.printf
0000040: 0066 6765 7473 0073 7464 6f75 7400 6d61  .fgets.stdout.ma
0000050: 6c6c 6f63 0066 7265 6164 0061 746f 6900  lloc.fread.atoi.
0000060: 7365 7462 7566 005f 5f6c 6962 635f 7374  setbuf.__libc_st
0000070: 6172 745f 6d61 696e 00<b>66 7265 65</b>00 6c64  art_main.<b>free</b>.ld
...
</pre>

After hijack the `strtab`, calling `leave & return` function to trigger `free`, which would be resolved to `system`. With `secret` filled with `"/bin/sh"` in advance, we can get a shell.
