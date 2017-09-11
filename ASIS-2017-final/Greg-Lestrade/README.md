# Greg Lestrade

```
Arch:     amd64-64-little
RELRO:    Partial RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      No PIE (0x400000)
```

## Analysis

First we are asked to input credential, which is hard coded as `7h15_15_v3ry_53cr37_1_7h1nk`. There is actually a stack overflow vulnerability here, however, stack canary prevents us from exploiting this.  
After logging in, there is a function as following form within an infinite loop. It requires us inputting a string containing only lower case letters. It will print the string in a vulnerable way if the input passed the check. Format string vulnerability can't be directly used since the necessary `%` will definitely be captured.

```c
read(0, cmd, 1023uLL);
len = strlen(cmd) + 1;
for ( i = 0; i < len; ++i )
{
  if ( cmd[i] <= 96 || cmd[i] > 122 )
  {
    puts("[*] for secure commands, only lower cases are expected. Sorry admin");
    result = 0LL;
    return result;
  }
}
printf(cmd);
```

Hence, the challenge will be finding a way to escape the check.

## Exploit

The variable used to store length of command is only 1-byte long, thus we can make the length of command to be `(multiple of 0x100) - 1`, the minus 1 is to cancel the effect of line 8 of above code. This way, the initial check `i < len` of the for loop will fail, and we can now utilize FMT vulnerability to do GOT hijacking.  
Here we have two ways to get the flag:

* Directly hijack `puts` or other functions to the hidden function at `0x400876` to print flag.
* Hijack `strlen` to `system` and send `"/bin/sh"` as command to open a shell !

flag: `ASIS{_ASIS_N3W_pwn_1S_goblin_pwn4b13!}`
