# AIS3 2017 final writeup

## pwn1

這題和 pre-exam 的 [pwn3](https://github.com/briansp8210/CTF_writeup/tree/master/AIS3-2017-pre-exam#pwn3) 大同小異，所以我直接拿 exploit 來改。  
`ais3{hav3_Y0U_successfu11Y_sO1v3d_Y3T_another_0rw}`

## pwn3

 這個程式會讓使用者輸入兩條字串，並將他們做 xor 運算後印出結果，之後一直重複這樣的流程。  
 問題出在 `xorstr` 中的 buf 沒有初始化，有殘留 libc 的 address，可以想辦法 leak 出來。   
 這裡我是鎖定位於 `result+8` 的殘留值，所以我輸入長度為 8 的 key，這樣在輸出 result 的時候就能夠拿到 libc address 了。
 
 ```c
void xorstr(char *str){
    char result[128];
    char key[128];
    printf("What do you want to xor :");
    read_input(key,128);
    xorlen = strlen(key);
    for(count = 0 ; count < xorlen; count++){
        result[count] = str[count] ^ key[count];
    }
    printf("Result:%s",result);
}
```

`process` 和 `xorstr` stack frame 如下所示，剛剛我 leak 出來的 libc address 就是位於 `0x7ffce9c42668` 的 `0x7fca2d301290`。  
可以發現 `result` 雖然比 `key` 先宣告，但他的位址是比較高的，這也讓接下來的攻擊得以成立。

<pre>
                ---------------------------------------------- <- key
0x7ffce9c425e0: | 0x4242424242424242      0x0000000000000000 |
0x7ffce9c425f0: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42600: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42610: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42620: | 0x00007ffce9c426f0      0x00007ffce9c42630 |
0x7ffce9c42630: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42640: | 0x000000000000ff00      0x0000000000000000 |
0x7ffce9c42650: | 0x00007ffce9c426c0      0x0000000000000000 |
                ---------------------------------------------- <- result
0x7ffce9c42660: | 0x0303030303030303      0x00007fca2d301290 |
0x7ffce9c42670: | 0x0000000000000080      0x00007ffce9c426f0 |
0x7ffce9c42680: | 0x0000000000000000      0x00007fca2d7f5700 |
0x7ffce9c42690: | 0x000000000000000c      0x0000000000000000 |
0x7ffce9c426a0: | 0x0000000000000000      0x00007fca2d7fb168 |
0x7ffce9c426b0: | 0x0000000000000005      0x00000000004007ea |
0x7ffce9c426c0: | 0x0000008000000000      0x00007ffce9c426f0 |
0x7ffce9c426d0: | 0x0000000000000000      0x000000092d7fb168 |
                ----------------------------------------------
0x7ffce9c426e0:   0x00007ffce9c42770      <b>0x0000000000400999</b>
                ---------------------------------------------- <- str
0x7ffce9c426f0: | 0x4141414141414141      0x00007fca2d5eb900 |
0x7ffce9c42700: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42710: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42720: | 0x000000000000003c      0x00007ffce9c426c0 |
0x7ffce9c42730: | 0x0000000000000000      0x0000000000000000 |
0x7ffce9c42740: | 0x0000000000000000      0x00007fca00000000 |
0x7ffce9c42750: | 0x0000000000000000      0x00007fca2d7fb168 |
0x7ffce9c42760: | 0x0000000000000004      0x00000000004008a0 |
                ----------------------------------------------
0x7ffce9c42770:   0x00007ffce9c42780      0x00000000004009b4
0x7ffce9c42780:   0x00000000004009c0      0x00007fca2d22a830
</pre>

觀察 stack frame 可以注意到，由於 xor 的次數是從 `strlen(key)` 得來的，所以如果我輸入 128bytes 的 key，他會和上一次 `result` 的結果串起來，變成長度超過 128 的 key，這麼一來 `result` 也會被填超過 128bytes，因此覆蓋到上圖我用**粗體**標示的 return address。  
因為 xor 運算造成要覆蓋的值不是這麼好控制，所以我打算直接塞 one gadget 給他。  
以上圖的 stack frame 來看，我們要蓋得值是由 `0x4009b4` xor `*0x7ffce9c42668` 產生的。而 `*0x7ffce9c42668` 又是由 `*0x7ffce9c426f8` xor `*0x7ffce9c425e8` 產生的，所以經由如下賦值後：  

* `*0x7ffce9c426f8 = one_gadget ^ 0x4009b4 ^ 0xffffffffffffffff`
* `*0x7ffce9c425e8 = 0xffffffffffffffff`

return address 就會被蓋成：
`0x4009b4 ^ [(one_gadget ^ 0x4009b4 ^ 0xffffffffffffffff) ^ 0xffffffffffffffff]`  
也就是 one gadget 的位址，如此就能順利開 shell 了！

`ais3{xxXxXxxXx00oOo0OOorRrrRRrrr}`
