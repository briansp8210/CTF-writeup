# Election

```
Arch:     amd64-64-little
RELRO:    Full RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      No PIE (0x400000)
```

Too late to complete before ending of competition but at least seeing the flag :D

## Analysis

```
*** Election ***

1. stand
2. vote
3. result
0. eat chocolate
>>
```

The challenge is a voting system, we can add candidate, vote and display the result. The candidate information is maintain in a linked list with `list` as head. Following is the structure of its node:

```c
struct info {
    char *name;
    struct info *next;
    int32_t votes;
};
```

* `stand`: add a candidate info entry to the list.  
* `vote`: find a candidate from the list, increment his votes if exists.  
* `result`: display information of each candidate in the list.  

There is a `lv` flag to make sure we can't perform action with small `lv` after doing action with big `lv`.  
The vulnerability appears when we vote to candidates named `Oshima`. The system will ask us whether re-vote to correct name with input length 48 bytes, where trigger overflow:  

```c
printf("I'm not 'Oshima', but 'Ojima'!\nWould you modify the name and re-vote?\n>> ");
getnline(&input_buf, 48);
if ( !strcasecmp(&input_buf, "yes") )
    *(int32_t *)(ojima_info + 16) += votes_num;
```

<pre>
 input_buf -> <b>0x0000616d6968736f      0x00007ffe541629e0</b>
              <b>0x0000000000400800      0x00007f2041059388</b>
ojima_info -> 0x0000000001984090      0x00000000004009<u>01</u> <- votes_num, only 1 byte
              0x0000000000000000      0x09346bd50b645700
              0x00007ffe54162a00      0x0000000000400972
</pre>

Hence, we can perform arbitrarily write with ojima_info and votes_num in control.  
My plan is overwrite `__malloc_hook` to one gadget to open shell.

## Exploit

First, We need to leak libc address, this can be done by making `list` point to fake entry, whose `name` points to somewhere store libc address. However, we need to make sure that there is an entry in the list with `name` points to `Ojima` so that we can trigger the overflow again to perform subsequent exploit.  
Therefore, I utilize the arbitrarily write to construct a series of fake chunk to leak libc address and keep `Ojima` entry in list meanwhile.

<pre>
   list
    |
    |
    ↓      ----------------------------------------------
0x19d70f0: | 0xffffffffff600804      <b>0x0000000000602050</b> |---
0x19d7100: | 0x0000000000000000      0x0000000000020f01 |  |
           ----------------------------------------------  |
     ______________________________________________________|
    |
    ↓      ----------------------------------------------
 0x602050: | <u>0x0000000000401004</u>      <b>0x0000000000602070</b> |---
 0x602060: | 0x0000000000000000      0x0000000000000000 |  |
           ----------------------------------------------  |
     ______________________________________________________|
    |
    ↓      ----------------------------------------------
 0x602070: | <u>0x0000000000601f88</u>      <b>0x0000000000000000</b> |<-- end of list
 0x602080: | 0x0000000000000000      0x0000000000000000 |
           ----------------------------------------------
		   
# 0x401004 points to "ojima"
# 0x601f88 points to a libc address
</pre>

Finally, we can overwrite `__malloc_hook` to one gadget, and overwrite `lv` to 0 so that we can use `stand` function, which include a `malloc` function call to trigger `__malloc_hook` to open shell.

flag: `SECCON{I5_7h15_4_fr4ud_3l3c710n?}`
