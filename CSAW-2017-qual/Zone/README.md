# Zone

> Arch:     amd64-64-little  
> RELRO:    Partial RELRO  
> Stack:    Canary found  
> NX:       NX enabled  
> PIE:      No PIE (0x400000)

## Analysis

```
Environment setup: 0x7ffe5ca30940
1) Allocate block
2) Delete block
3) Write to last block
4) Print last block
5) Exit
```

This program implement a memory allocator. According to size user asked for, there are 0x40, 0x80, 0x100 and 0x200 blocks available.  
The available blocks are maintain in a pool, it records block address which will be allocated next for each size. This pool is located at stack, whose address is printed at beginning.  
For instance, next allocated 0x40 block will be `0x7fb09a2c8000` in this case.

```
0x7ffe5ca30940: 0x0000000000000040      0x00007fb09a2c8000
0x7ffe5ca30950: 0x00007fb09a2c8000      0x0000000000000080
0x7ffe5ca30960: 0x00007fb09a2c7000      0x00007fb09a2c7000
0x7ffe5ca30970: 0x0000000000000100      0x00007fb09a2c6000
0x7ffe5ca30980: 0x00007fb09a2c6000      0x0000000000000200
0x7ffe5ca30990: 0x00007fb09a2c5000      0x00007fb09a2c5000
```

Each block itself has two attributes at header, `size` and `fd` pointer.  
`size` indicates the user-available space of this block. `fd` used to record address of next allocated block with the same size after this current block is allocated.  
For instance, following is the chain structure of 0x40 blocks.

<pre>
                ----------------------------------------------
0x7fb09a2c8000: | 0x0000000000000040      <b>0x00007fb09a2c8050</b> |---
0x7fb09a2c8010: | 0x0000000000000000      0x0000000000000000 |  |
0x7fb09a2c8020: | 0x0000000000000000      0x0000000000000000 |  |
0x7fb09a2c8030: | 0x0000000000000000      0x0000000000000000 |  |
0x7fb09a2c8040: | 0x0000000000000000      0x0000000000000000 |  |
                ----------------------------------------------  |
         _______________________________________________________|
        |
        ↓       ----------------------------------------------
0x7fb09a2c8050: | 0x0000000000000040      <b>0x00007fb09a2c80a0</b> |---
0x7fb09a2c8060: | 0x0000000000000000      0x0000000000000000 |  |
0x7fb09a2c8070: | 0x0000000000000000      0x0000000000000000 |  |
0x7fb09a2c8080: | 0x0000000000000000      0x0000000000000000 |  |
0x7fb09a2c8090: | 0x0000000000000000      0x0000000000000000 |  |
                ----------------------------------------------  |
         _______________________________________________________|
        |
        ↓       ----------------------------------------------
0x7fb09a2c80a0: | 0x0000000000000040      <b>0x00007fb09a2c80f0</b> |---
0x7fb09a2c80b0: | 0x0000000000000000      0x0000000000000000 |  |
                              ...
</pre>

* `Allocate block` will check whether there is available block for required size. If there is, the allocator first replaces this block with the address at `fd` field of this block to the head of list. Afterwards, the `fd` field of this block will be cleared, and this block will be returned.  
If there are no available block for this size anymore, it will print `Nope sorry can't allocate that` warning message.

```c
victim = *(bin_ptr + 8);             // fetch block from head of list
if ( victim )                        // if block is available
{
  if ( *(victim + 8) )
    *(bin_ptr + 8) = *(victim + 8);  // head of list will be replaced with fd of currently allocated block
  else
    *(bin_ptr + 8) = 0;
  *(victim + 8) = 0;                 // clear fd filed of currently allocated block
  result = victim + 16;
}
else
{
  result = 0;
}
return result;
```

* `Delete block` will de-allocate a block and insert it to head of one of the lists according to its `size`. `fd` of this block will be filled with original head of this list.

```c
if ( victim )
{
  *(victim + 8) = *(bin_ptr + 8);  // fill fd of the de-allocated block with current head of list
  result = bin_ptr;
  *(bin_ptr + 8) = victim;         // this block will become head of list
}
return result;
```

Until now, we can notice that the implementation is quite like the fastbin mechanism in glibc, especially the LIFO property.

* `Write to last block` can write some data to most recently allocated block. The length limit of input is `size+1`, we can utilize this 1-byte overflow to overwrite `size` of following adjacent block.

```c
for ( i = 0; i <= size; ++i )  // 1-byte overflow
{
  if ( (unsigned int)read(0, &buf, 1uLL) == -1 )
    exit(-1);
  if ( buf == '\n' )
    break;
  *victim++ = buf;
}
```

* `Print last block` will print contents of most recently allocated block using `puts`

My plan is utilizing the trick similar to fastbin corruption to get a fake chunk at stack, and craft ROP chain to open shell. This will usually come with the challenge to find proper value on stack to serve as `size` of a fast chunk to circumvent the check.  
In this allocator, however, no such check exists ! Hence we get much more flexibility when picking target.

## Exploit

In this case, my target is get a fake chunk at `return address of main - 16`, since return address of `main` will be instruction in `__libc_start_main`, which is located at libc. By choosing this, I can first leak libc address, then immediately write ROP chain on it, Perfect !  
First of all, I allocate a 0x40 block, and overwrite `size` of following adjacent block to 0x80. Then I allocate 0x40 block again to get this size-modified block.
Next, I de-allocate the size-modified block, this will make it be inserted into 0x80 list. After that, I allocate 0x80 block to get this size-modified block again, this make me get a 0x80 block among 0x40 blocks, which means I can overwrite `fd` of following 0x40 block !

<pre>
the range we can write is marked as <b>bold</b>

0x7fb09a2c8000: 0x0000000000000040      0x0000000000000000
0x7fb09a2c8010: 0x4141414141414141      0x4141414141414141
0x7fb09a2c8020: 0x4141414141414141      0x4141414141414141
0x7fb09a2c8030: 0x4141414141414141      0x4141414141414141
0x7fb09a2c8040: 0x4141414141414141      0x4141414141414141
0x7fb09a2c8050: 0x0000000000000080      0x0000000000000000
0x7fb09a2c8060: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c8070: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c8080: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c8090: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c80a0: <b>0x0000000000000040      0x00007f30066980f0</b> <- I can overwrite this fd field
0x7fb09a2c80b0: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c80c0: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c80d0: <b>0x0000000000000000      0x0000000000000000</b>
0x7fb09a2c80e0: 0x00000000000000<b>00</b>      0x0000000000000000
</pre>

After overwriting `fd` of a 0x40 block, I allocate 0x40 block 2 times, the second allocation will be our target fake block. Finally, I can leak libc address and do ROP to open shell and cat the flag !

flag: `flag{d0n7_let_m3_g3t_1n_my_z0n3}`
