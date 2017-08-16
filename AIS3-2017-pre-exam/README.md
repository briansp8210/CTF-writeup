# AIS3 2017 pre-exam writeup (pwn)

## pwn1

送一個位址給程式，接著會跳轉到該位址執行。`youcantseeme` 是個好選擇，不過他 address 的第一個 byte 是 0x0a，會中止 `scanf`，所以乾脆直接跳去 `0x8048613` 跑 `system("sh")`。

```
0804860a <youcantseeme>:
 804860a:       55                      push   ebp
 804860b:       89 e5                   mov    ebp,esp
 804860d:       83 ec 08                sub    esp,0x8
 8048610:       83 ec 0c                sub    esp,0xc
 8048613:       68 5c 87 04 08          push   0x804875c
 8048618:       e8 03 fe ff ff          call   8048420 <system@plt>
 804861d:       83 c4 10                add    esp,0x10
 8048620:       90                      nop
 8048621:       c9                      leave
 8048622:       c3                      ret
```

`ais3{4nn0y1n9_Wh1t3_SpAcE_CHAR4CTERS}`

## pwn2

剛看到題目嚇了一跳，因為之前沒接觸過 windows 的題型。  
首先在輸入 `ais3_user.name` 的時候可以直接 overflow 到 `ais3_user.pass`，接著在輸入密碼的時候照著填就可以成功 login 了。  
接著選擇進入 `readflag()`，可以發現他會把 flag xor `ais3_user.pass` 之後印出來。所以只要讓密碼等於 0 就可以了。

```c
void readflag() {
    char buf[100];
    FILE *fp;
    fp = fopen("./flag.txt", "rb");
    if (fp) {
        fread(buf, 40, 1, fp);
        fclose(fp);
        for (int i = 0; i < 40; i++) {
            buf[i] = buf[i] ^ ais3_user.pass;
        }
        printf("Magic : %s\n", buf);
        Sleep(2);
        exit(0);
    }
};
```

`ais3{Just_a_simpl3_overflow}`

## pwn3

寫 shellcode 來讀 flag，限用 `open`/`read`/`write`/`exit` 這幾種 system call，並且 shellcode 中不能出現 `"flag"`，另外單次 `read` 最多只能讀 42bytes。  
首先要解決 `"flag"` 的問題，我先寫一個 `read` 把 `"/home/pwn3/flag"` 讀到 stack 上，接著就可以做 `open -> read -> write` 印出 flag。  
這個時候發現 flag 怎麼印不完整，困惑之際 **@nae** 說要不要繼續讀，果真就出現了！總共讀了 3 次才讀完。

`ais3{r34d_0p3n_r34d_Writ3_c4ptur3_th3_fl4g_sh3llc0ding_1s_s0_fUn_y0ur_4r3_4_g0od_h4ck3r_h4h4}`

## pwn4

這題也是 windows 的題目，解起來很不習慣，卻也學到不少東西XD  
首先可以注意到以下兩個 function 有 fmt 和 bof 可以利用。

```c
void echo() {
    char buf[16];
    if (!count) {
        printf("What do you want to say : ");
        read(0, buf, 15);
        printf("You say : ");
        printf(buf);
        count++;
    }else {
        puts("Hello world !");
    }
}

int bof() {
    int size;
    char buf[20];
    puts("Do you know stack overflow ?");
    printf("Try your best : ");
    size = read(0,buf,100);
    puts("Boom !!!");
    return size;
}
```

我先利用 fmt，發現第一個印出來的東西是在 `.text` 的 address，可以用他算出 `.text` 的開頭。  
接著就可以利用 bof 來疊 ROP 了，目標是造出 `system("cmd.exe")`。  
觀察一下組語再查一下[文件](https://msdn.microsoft.com/en-us/library/ms235286.aspx)後確認 function call 的第一個參數是用 `rcx` 來傳遞的。然後程式本身自帶 `"cmd.exe"`，可以直接拿來用 *( 我一開始是自己擺在 stack 上，結果耗了很久，只能在 local 開 shell，遠端不知道為什麼會失敗 orz )*，至於 `system` 可以直接在 `.text` 中找到。  
最後利用 [Ropper](https://github.com/sashs/Ropper) 找出要用的 gadgets *( ROPgadget 解出來的 offset 不知道為什麼怪怪的 )*。

```
0x140001d45: pop rsi; ret;            # 把 system 的位址給 rsi
0x140001519: pop rdi; ret;            # 接著把參數給 rdi
0x140002648: mov rcx, rdi; call rsi;  # 把參數傳給 rcx 後呼叫 system
```

然後就可以順利開 shell 了！

```
Microsoft Windows [Version 10.0.10240]
(c) 2015 Microsoft Corporation. All rights reserved.

C:\Users\visitor\Desktop\pwn3>type flag.txt
type flag.txt
ais3{St4ck_0v3rfl0w_1s_v3ry_d4ng3rous}
```
