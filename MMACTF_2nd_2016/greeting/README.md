greeting
==========
這題花了很多時間試各種解法，最後還是看了學長的提示才知道方向QQ 不過也是學會新東西了！

解題流程
----------
這題會先用 getnline() 接收 63bytes 的輸入，然後用帶有 format string 漏洞的 printf 連同一些歡迎訊息一起印出來。

首先，我們可以看到 getnline 裡面有呼叫 ```strlen```，並且參數是我們可控的 input buffer，若我們利用上面提到的漏洞將 strlen 的 GOT 寫成 ```system```，然後輸入 ```"/bin/sh"```，我們即可拿到 shell。

	80486bc:       8b 45 08                mov    eax,DWORD PTR [ebp+0x8]
	80486bf:       89 04 24                mov    DWORD PTR [esp],eax
	80486c2:       e8 f9 fd ff ff          call   80484c0 <strlen@plt>
	
但是問題在於 printf 是在 getnline 結束後才被呼叫的，利用 printf 改 GOT 已經太遲了。
此時可行的一個方法是我們將程式控回 call getnline 之前，這樣就可以再去 call strlen 一次了！
所以我們可以把 ```.fini_array``` 這個 section 裡面的某個 function pointer，像這裡我把 ```__do_global_dtors_aux```，給改成 ```main``` 的位址。

	[20] .fini_array       FINI_ARRAY      08049934 000934 000004 00  WA  0   0  4
	
這個 section 是當程式要結束前會來執行這裡面放的一些 function，來進行一些終止前的作業。

----------

所以我們要做的就是在一個 printf 的 call 裡面完成 strlen GOT 的覆蓋以及 .fini_array 裡面的值。
這裡要特別注意的是題目有列出這條限制：

	Note: To prevent from DoS attacks, output length is limited in 131072 characters.
	
一開始我沒有考慮到這個問題，任意擺放要蓋的順序，結果就是產生太多不必要的 overflow 輸出，比題目限制多噴了 2000 多個 characters QAQ 

後來 **@nae** 建議我將要蓋的數值由小排到大。假設 addr1 要蓋成 0x1234、addr2 要蓋成 0x1230，那擺到 buffer 上的順序就是 ```p32(addr2) + p32(addr1)```。這樣可以避免後面要蓋的值比前面小，導致必須 overflow 後重新算，造成的大量輸出。

都蓋成功後當程式等輸入時送進 "/bin/sh"，等一下 call strlen 的時候就可以成功得到 shell 了！

