# simplepdf

這題給了一個 pdf 檔，裡面夾帶了一個附件 `pdf10000.pdf`，把他打開後裡面又是一個附件 `pdf9999.pdf`，因此猜測應該是有很多層的 pdf 檔把最裡面的東西給包起來了，而且命名也有規則。
於是上網找了一下能夠提取 pdf 附件的指令，發現了 [pdfdetach](http://www.dsm.fordham.edu/cgi-bin/man-cgi.pl?topic=pdfdetach&ampsect=1)。主要用到的功能有：
* -list：列出該 pdf 夾帶的所有附件
* -save：儲存指定的附件

利用這些寫 script 讓他去一層層拆開該 pdf 檔，並假設至少會一直拆到 `pdf0.pdf`

```shell
#!/bin/bash

pdfdetach -save 1 simplepdf_f8004a3ad0acde31c40267b9856e63fc.pdf

for(( count=10000; count>=0; count=count-1 ))
do
    pdfdetach -save 1 "pdf${count}.pdf"
#   echo -e "pdf${count}.pdf has been processed !"
done
```

跑完後發現 `pdf0.pdf` 還解出了一個 `start.pdf`，這個 `start.pdf` 裡面又夾帶著同名的 `start.pdf`，打開來就會看到 flag 了。
另外應該要把 scipt 改成讓他邊拆邊把之前解出來的給刪掉比較好，因為全部打開後蠻佔空間的QQ

`flag{pdf_packing_is_fun}`
