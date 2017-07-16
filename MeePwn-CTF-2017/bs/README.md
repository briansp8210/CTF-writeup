# [MeePwn CTF 1st 2017] bs

> Category: pwnable  
> Author: briansp8210 @ BambooFox

```
RELRO:    Partial RELRO
Stack:    No canary found
NX:       NX enabled
PIE:      No PIE (0x8048000)
```

程式主要做的是接受使用者輸入一組數字，隨後進行 binary search。  
首先會需要登入，如果輸入的密碼正確就可以以 root 登入，否則就必須指定一個非 0 的 uid。  
接著 `check_root()` 會檢查是否為 root，這裡他只比對 uid 的低 2 bytes，因此若是用像 `0xffff0000` 這樣的 uid 是可以成功繞過檢查的。

```c
if ( !(_WORD)uid )
    is_root = 1;
```

接下來就輸入要 sort 多少數字，並逐一輸入。這裡只有 root 身分才能夠 sort 超過 31 個數字。  
在輸入完後可以看剛剛給了什麼數字，這裡沒有檢查 index 是否為正，因此可以讀到 `.got.plt` 上的值，[查詢](https://libc.blukat.me/?q=printf%3Af30%2Cread%3A2a0&l=libc6_2.24-3ubuntu2.2_i386)之後得知是 `libc6_2.24-3ubuntu2.2_i386`。  
在 `bubble_sort` 之後就可以指定要 search 哪一個值並開始搜。過程中用來存左右界和中間點 index 的變數都是 `char` 型別，所以像 sort number 較多，而一開始 `array[mid_idx] < target_val` 的情況下，中間點 index 就會 overflow 成負的。

```c
while ( l_bnd <= r_bnd )
{
  printf("MID[%hhi] is 0x%x\n", mid_idx, array[mid_idx]);
  if ( array[mid_idx] >= target_val )
  {
    if ( array[mid_idx] == target_val )
    {
      printf("%d found at %hhi !!!\n", target_val, mid_idx);
      edit(mid_idx);
      break;
    }
    r_bnd = mid_idx - 1;
    mid_idx = l_bnd + mid_idx - 1;
    mid_idx /= 2;
  }
  else
  {
    l_bnd = mid_idx + 1;
    mid_idx += 1 + r_bnd;
    mid_idx /= 2;
  }
}
```

我這邊是搜 `0x6fffffff`，這是 `array[-41]` 的原始值，因此 binary search 跑到 -41 的地方會成功找到目標，並呼叫 `edit(-41)`。`edit()` 會從傳入的 index 開始，針對各個 `array` 的元素詢問是否要做修改。由於這裡傳入的是 -41，我們可以改的範圍從 `array[-41]` 到 `array[126]`，涵蓋 `.got.plt`。  
首先，我把 `open@got.plt` 改去跑 `xor eax, eax ; ret` ，這會讓 `login()` 改從 `stdin` 讀密碼。接著再把 `memcpy@got.plt` 蓋成 `system`，以及 `__isoc99_scanf@got.plt` 蓋成 `main`。如此一來就可以 return 回 `main`，並在 `login()` 中比對密碼的地方呼叫 `system("/bin/sh")`。

```c
fd = open("/dev/urandom", 0);
read(fd, &correct_password, 16u);
puts("Enter your password:");
read(0, &password, 16u);
if ( !memcmp(&correct_password, &password, 16u) )
{
  puts("Welcome back master !");
  uid = 0;
}
```

flag: `MeePwnCTF{C_1n73g3r_0v3v3rFl0w}`
