# SENTOSA

I solved this challenge after the competition ended, still enjoying the exploit process :D

```
Arch:     amd64-64-little
RELRO:    Full RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      PIE enabled
FORTIFY:  Enabled
```

## analysis

```
Welcome to Sentosa Development Center
Choose your action:
1. Start a project
2. View all projects
3. Edit a project
4. Cancel a project
5. Exit
```

* Start a project can create a project, which has information stored in heap with following structure, and the address of this chunk will be stored to a global array

```
+---------------+
|  name length  | 4
+---------------+
|               |
|  name         | name_len + 1
|               |
+---------------+
|  sanity check | 4
+---------------+
|  price        | 4
+---------------+
|  area         | 4
+---------------+
|  capacity     | 4
+---------------+
```

* View all projects will fetch each information chunk according to aforementioned global array and display their information
* Edit a project is not implemented
* Cancel a project will do sanity check and free the information chunk of a specific project. Then clear this address from the global array

In start project function, all attributes are directly written, while `name` is first read into stack then `strncpy` to heap. The read input function looks like this:

```c
void read_input(char *target, int len)
{
  int len2 = len - 1;
  char *target_iter;
  int64_t idx;
  char buf;
	
  if(len2)
  {
    target_iter = target;
    idx = 0;
    do
    {
      read(0, &buf, 1);
      if(buf == '\n')
      {
        target[idx] = 0;
        return;
      }
      idx++;
      *target_iter++ = buf;
    }
    while(idx != len2);
  }
  else
  {
    idx = 0;
  }
  target[idx] = 0;
  return;
}
```

Thus, we can input only length-1 bytes, and a null byte will be appended at the end.  
But wait, if `len` is 0, then we can input massive number of bytes, which indeed long enough to overwrite return address !  
There is canary on stack, however, so the only valuable thing I can overwrite is the address points to the information chunk. Arbitrary read is possible by overwriting this pointer to `target address - 4`, this fake pointer will be stored to global array, then we can use Visit to print the contents of target address.

<pre>
            0x00007f23e9682780      0x00000010e93b36e0
name buf -> 0x0000000000000000      0x0000000000000000
            0x0000000000000000      0x0000000000000000
            0x0000000000000000      0x0000000000000000
            0x0000000000000000      0x0000000000000000
            0x0000000000000000      0x0000000000000000
            0x0000000000000000  +-> <b>0x5611a9f51010</b>0000
            0x0000000000000000  |   0xce233afa9dff3a00
            0x00005611a9c4829a  |   0x00005611a9c483f8
            0x00007ffd1baf9ff4  |   0x00005611a9c47a30
            0x00007ffd1bafa110  |   0x00005611a9c48117 <- ret address of Start project
                                |
                        information pointer
</pre>

## exploit

First, since still no useful address available, I want to use null-byte overflow **(since the read input function will append null byte, we can't control the value of last byte)** to make the pointer points to somewhere a heap address locates, .  
To achieve this goal, I create proper number and size of project, then free two fast chunks with the same size, so that the fd field of target chunk will be filled, thus successfully get heap address.

> I make the fd field locates at price field of a project, so only 4 bytes of the heap address are leaked.  
> However, the lowest byte is fixed, and the highest byte, by observation, is mostly changed between 0x55 and 0x56, so we can still get correct address  most of the time.


With heap base address, I can arbitrarily read content on heap.  
My plan is to allocate small chunks then free one of them so that libc address will appear on heap. Unfortunately, the maximum chunk can be allocated is still fast chunk, so I had to forge these small chunks, which took quite lots of time since null byte can't be used under the usage of `strncpy`, and sanity check must be satisfied.  
After trial and error, I finally get libc address, then continue to leak stack address by `environ` symbol. And finally, leak stack canary by stack address with ease.
With stack canary, we can do ROP to open shell !

flag: `HITB{Thank_y0u_f0r_d3v3l0ping_SENTOSA}`
