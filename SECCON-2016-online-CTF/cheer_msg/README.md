# cheer_msg

之前看到 [CTF-pwn-tips](https://github.com/Naetw/CTF-pwn-tips) 裡面提到 `alloca` 的利用方式，才知道這題要怎麼破。今天終於有空把他解出來了XD

## Analyze

程式一開始會請你輸入 message 的長度，接著利用 `alloca` 針對訊息長度來分配適當的空間。  
查 `alloca` 的 man page 可以發現他是直接在 caller 的 stack frame 上分一塊空間出來的。

>The alloca() function allocates size bytes of space in the stack frame of the caller.  This temporary space is automatically freed when the function that called alloca() returns to its caller.

接著如果去看程式的組語會發現他是用 `sub esp, eax` 來實作分配空間，其中 `eax` 和指定的 size 有關。因此如果我們指定 size 為負數，就有機會讓分配空間的位址比當前的 `esp` 還高，從而導致 overflow 等問題。
## Exploit

稍微測試一下後，我選了 -112 當作 size，這會讓分配到的位址，也就是 `esp` 比 `main` 的 `ebp` 還高。接著呼叫 `message` 會造成他的 stack frame 和 `main` 的 stack frame 部分重疊，在我們輸入 `name` 的時候因此可以蓋到 `main` 的 return address 來建 ROP chain。

```c
int __cdecl message(int buf, int len)
{
  char name;
  int v4;

  v4 = *MK_FP(__GS__, 20);
  printf("Message >> ");
  getnline((char *)buf, len);
  printf("\nOops! I forgot to ask your name...\nCan you tell me your name?\n\nName >> ");
  getnline(&name, 64);
  printf("\nThank you %s!\nMessage : %s\n", &name, buf);
  return *MK_FP(__GS__, 20) ^ v4;
}
```

於是就可以用 `printf` 來 leak libc address，然後 return 回 `main` 重複一次攻擊流程。這次就可以疊 `system("/bin/sh")` 來開 shell 了。
