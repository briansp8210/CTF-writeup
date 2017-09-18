# Auir

> Arch:     amd64-64-little  
> RELRO:    Partial RELRO  
> Stack:    No canary found  
> NX:       NX enabled  
> PIE:      No PIE (0x400000)

I was shocked by the size of this binary, it has lots of code in a single function, and many mysterious variables and operations.  
But its behavior is actually not so complicated after taking times to observe it :D

## Analysis

```
|-------------------------------|
|AUIR AUIR AUIR AUIR AUIR AUIR A|
|-------------------------------|
[1]MAKE ZEALOTS
[2]DESTROY ZEALOTS
[3]FIX ZEALOTS
[4]DISPLAY SKILLS
[5]GO HOME
|-------------------------------|
>>
```

* `MAKE ZEALOTS` will use `malloc` to allocate a chunk whose size is determined by us. Then we can send some input to store in the chunk.
* `DESTROY ZEALOTS` can be used to `free` specific chunks allocated by first option.
* `FIX ZEALOTS` will ask for a new size for an existing chunk, then we can send something up to `new size` bytes to it. This can be used to trigger heap overflow !
* `DISPLAY SKILLS` prints the content of a specific chunk

My plan is utilizing heap overflow to do fastbin corruption, and doing GOT hijacking to open shell.

## Exploit

First I need to know libc address. I make two small chunks and free the first one, this can make an address points to somewhere inside `main_arena`, which located at libc, appears at the `fd` field of the first chunk. Then I can print the contents of this chunk to get the address.  

With libc address, I can try to get a fake chunk near GOT.  
First of all, I make to fastbin chunk, then free the second one. After that, using `FIX ZEALOTS` function on first chunk to overwrite the `fd` field of second chunk to the target address.  
Finally, I make two chunks to get out target fake chunk, which has following layout (the fake chunk is marked as **bold**):

<pre>
0x604ff8:       <b>0x000000000000</b>0000
0x605000:       <b>0x0000000000604df8</b>
0x605008:       <b>0x00007f7284463168</b> -> padding higher 6 bytes
0x605010:       <b>0x00007f7284253870</b> -> I just overwrite this to original value
0x605018:       <b>0x00007f7283bff5a0</b> -> _ZNSolsEi@plt
0x605020:       <b>0x00007f7283ee1e70</b> -> setvbuf@plt
0x605028:       <b>0x00007f7283b8d7e0</b> -> _ZNSt8ios_base4InitC1Ev@plt
0x605030:       <b>0x00007f7283f69220</b> -> read@plt
0x605038:       <b>0x00007f7283ef6130</b> -> malloc@plt
0x605040:       <b>0x00007f7283e92740</b> -> __libc_start_main@plt
0x605048:       <b>0x00007f7283eac280</b> -> __cxa_atexit@plt
0x605050:       <b>0x0000000000400986</b> -> _ZNSt8ios_base4InitD1Ev@plt
0x605058:       0x00007f7283bf<b>f210</b> -> one_gadget

notice that from 0x60505a to 0x60505f belongs to prev_size field of next chunk, hence we can still overwrite it.
</pre>

I overwrite most of the GOT entry of functions to its PLT entry, so that it can still work properly when being called subsequently.  
The crucial point is overwriting GOT entry of `_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_` to one gadget. Since this function will be called in `main`, where stack frame is much more *clear* to satisfy the constraints of one gadget.

flag: `flag{W4rr10rs!_A1ur_4wa1ts_y0u!_M4rch_f0rth_and_t4k3_1t!}`

