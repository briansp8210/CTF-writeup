# writeme

```
Arch:     amd64-64-little
RELRO:    No RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      No PIE (0x400000)
```

程式會印出我們指定位址的值，並讓我們輸入 8bytes 的內容來把它蓋掉。  
這裡我是指定 `printf` 的 GOT，在得到 libc base 之後就用 one gadget 蓋掉它來開 shell。  

flag: `FLAG{y33SuTd5GsmOPwonYWqePbS3y3R9Tz33}`
