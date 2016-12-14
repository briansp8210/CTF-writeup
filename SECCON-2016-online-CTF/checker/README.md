# checker

> category: Exploit
> points: 300

在解這題的時候走錯方向，一直想 overflow 在 bss 段的 name buffer 來看能不能印出 flag。後來看到 bruce 學長的思路，以及在賽後看了 [writeup](https://github.com/Inndy/ctf-writeup/tree/master/2016-seccon/checker)，才知道這題可以用 SSP (Stack Smash Protector) 來做。

## observe

這個程式一開始就會把 flag 給讀到位在 bss 的 buffer，所以如果能讀到那個位址，就可以得到 flag。另外在執行過程中也會有很多次輸入的機會。
第一次是讓使用者輸入名字，並存到 name buffer，由於這裡可以 overflow 到 flag，所以在這裡是了好久。
剩下的輸入都是直接寫到 main 的一個 local buffer，我們就是要利用這裡來 overflow。

## [SSP](http://j00ru.vexillium.org/blog/24_03_15/dragons_ctf.pdf)
當程式有開啟 stack canary 時，只要我們 overflow 時改到存在 stack 上的隨機數，就會出現像這樣的警告：

```
*** stack smashing detected ***: ./checker terminated
```

問題是，他是怎麼拿到我們執行檔的名稱呢？去看 `__stack_chk_fail` 的 source code 會發現他將警告訊息的前半部傳進 `__fortify_fail` 

```c
void
__attribute__ ((noreturn))
__stack_chk_fail (void)
{
  __fortify_fail ("stack smashing detected");
}
```

再去觀察 `__fortify_fail` 就會發現我們的執行檔名稱是從 `argv[0]` 取得的

```c
void
__attribute__ ((noreturn)) internal_function
__fortify_fail (const char *msg)
{
  /* The loop is added only to keep gcc happy.  */
  while (1)
    __libc_message (2, "*** %s ***: %s terminated\n",
                    msg, __libc_argv[0] ?: "<unknown>");
}
```

所以我們如果能控到位於 stack 的 `argv` 陣列，我們就可以將 `argv[0]` 改成我們要 leak 的位址，並且觸發 `__stack_chk_fail` 來印出該位址的內容，像這題我們就可以把 argv[0] 改成 flag buffer 的位址來印出 flag。

## exploit

首先要找到 `argv[0]` 和輸入 buffer 的 offset，這裡利用 qira 來找會比較方便。不過這個時候還沒辦法直接將 flag buffer 的位址給蓋上去，因為他的位址只有 3bytes，但是如果用 gdb 觀察會發現那裏原本用了 6bytes 來存位址，所以我們直接蓋的話會蓋不到高位導致失敗。所以這裡我們要先利用程式問我們知不知道 flag 的那個迴圈，以及他提供的 `getaline` 會在輸入結尾補 null byte 的特性來把高位清空。接著就可以將 flag buffer 的位址填上去來 leak flag。

```
*** stack smashing detected ***: SECCON{y0u_c4n'7_g37_4_5h3ll,H4h4h4} terminated
```


