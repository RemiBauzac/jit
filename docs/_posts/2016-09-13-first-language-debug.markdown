---
layout: post
title:  "First language debug"
date:   2016-09-18 23:00:20 +0200
author: Rémi Bauzac

---

In the [previous post](http://jit.bauzac.net/blog/first-language-jit) you built the JiT compiler, included in the virtual machine to execute the bytecode.
This post explain how to debug the generated binary, using gdb (Linux) or lldb (MacOS).
If gdb is not installed on your Linux, please install gdb package.

Before starting, you can read [this paper about gdb commands](https://people.eecs.berkeley.edu/~mavam/teaching/cs161-sp11/gdb-refcard.pdf) and [this one](http://lldb.llvm.org/lldb-gdb.html) for lldb.

# Running debugger

## gdb
To start debugger, use gdb as follow with the virtual machine executable as argument
{% highlight shell %}
$ gdb langvm
GNU gdb (Debian 7.7.1+dfsg-5) 7.7.1
Copyright (C) 2014 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
<http://www.gnu.org/software/gdb/documentation/>.
For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from langvm...done.
(gdb)
{% endhighlight %}

Set now arguments to langvm (-j and out.bc):
{% highlight shell %}
(gdb) set args -j out.bc
(gdb)
{% endhighlight %}

Set breakpoint to main function:
{% highlight shell %}
(gdb) break main
Breakpoint 1 at 0x400dc2: file vm.c, line 97.
(gdb)
{% endhighlight %}

And finally run the virtual machine:
{% highlight shell %}
gdb) run
Starting program: /home/rbauzac/Dev/jit/src/langvm -j out.bc

Breakpoint 1, main (argc=3, argv=0x7fffffffe6a8) at vm.c:97
97		int64_t ret = 0;
{% endhighlight %}

## lldb

To start debugger, use lldb as follow with the virtual machine executable as argument
{% highlight shell %}
$ lldb langvm
(lldb) target create "langvm"
Current executable set to 'langvm' (x86_64).
(lldb)
{% endhighlight %}

Set now arguments to langvm (-j and out.bc):
{% highlight shell %}
(lldb) settings set -- target.run-args  "-j" "out.bc"
(lldb)
{% endhighlight %}

Set breakpoint to main function:
{% highlight shell %}
(lldb) breakpoint set --name main
(lldb)
{% endhighlight %}

And finally run the virtual machine:
{% highlight shell %}
(lldb) run
* thread #1: tid = 0x71c1e8, 0x00000001000014e6 langvm`main(argc=3, argv=0x00007fff5fbffa08) + 22 at vm.c:97, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
    frame #0: 0x00000001000014e6 langvm`main(argc=3, argv=0x00007fff5fbffa08) + 22 at vm.c:97
   94  	int main(int argc, char **argv)
   95  	{
   96  		function *fmain;
-> 97  		int64_t ret = 0;
   98  		int jit = 0;
   99  		char *filename;
   100
{% endhighlight %}


# Get the binary address
For get the binary address, you have to set breakpoint just after this allocation in `main()` function :

{% highlight c %}
if (jit) {
    create_binary(fmain);
}
{% endhighlight %}

## gdb

Then you have to set breakpoint just after the `create_binary()` call:

{% highlight shell %}
(gdb) break 121
Breakpoint 2 at 0x400e79: file vm.c, line 121.
{% endhighlight %}

Next continue the execution:

{% highlight shell %}
(gdb) continue
Continuing.

Breakpoint 2, main (argc=3, argv=0x7fffffffe6a8) at vm.c:121
121		if (fmain->binary) {
{% endhighlight %}

And print the address of fmain->binary pointer:
{% highlight shell %}
(gdb) print fmain->binary
$1 = (uint8_t *) 0x7ffff7ff4000 "UH\211\345H\270!"
(gdb)
{% endhighlight %}

Now you know the binary's address to set breakpoint and start binary debug session.

## lldb

Then you have to set breakpoint just after the `create_binary()` call:

{% highlight shell %}
(lldb) breakpoint set --file vm.c --line 121 
Breakpoint 2: where = langvm`main + 232 at vm.c:121, address = 0x00000001000015b8
{% endhighlight %}

Next continue the execution:

{% highlight shell %}
(lldb) continue
...
{% endhighlight %}

And print the address of fmain->binary pointer:
{% highlight shell %}
(lldb) print fmain->binary
(uint8_t *) $0 = 0x0000000100036000 "UH\x89�H�#"
{% endhighlight %}

Now you know the binary's address to set breakpoint and start bianry debug session.

# Debug the binary

## gdb

To debug the binary, you have to set a breakpoint on its address, got from the fmain->binary print

{% highlight shell %}
(gdb) break *0x7ffff7ff4000
Breakpoint 2 at 0x7ffff7ff4000
{% endhighlight %}

Now, just continue the execution

{% highlight shell %}
(gdb) continue
Continuing.

Breakpoint 2, 0x00007ffff7ff4000 in ?? ()
(gdb)
{% endhighlight %}

To show the current instruction in assembly language, just use:

{% highlight shell %}
(gdb) display/i $pc
1: x/i $pc
=> 0x7ffff7ff4000:      push   %rbp
{% endhighlight %}

You can see that the current instruction is `push %rbp` which is the first instruction of your binary.
To jump to the next instruction, you can use the ni (as next instruction) command

{% highlight shell %}
(gdb) ni
0x00007ffff7ff4001 in ?? ()
1: x/i $pc
=> 0x7ffff7ff4001:      mov    %rsp,%rbp
(gdb)
{% endhighlight %}

Notice that the display command always add the same output after all next/ni command. You can remove this display with the `undisplay` command.
To see the registers content, you can use the `info register` command

{% highlight shell %}
(gdb) info register
rax            0x7ffff7ff4000	140737354088448
rbx            0x0	0
rcx            0x20	32
rdx            0x10	16
rsi            0x6022a0	6300320
rdi            0x7ffff7ff4000	140737354088448
rbp            0x7fffffffe5c0	0x7fffffffe5c0
rsp            0x7fffffffe570	0x7fffffffe570
r8             0xffffffff	4294967295
r9             0x0	0
r10            0x7fffffffe2f0	140737488347888
r11            0x7ffff7ac3020	140737348644896
r12            0x400850	4196432
r13            0x7fffffffe6a0	140737488348832
r14            0x0	0
r15            0x0	0
rip            0x7ffff7ff4001	0x7ffff7ff4001
eflags         0x206	[ PF IF ]
cs             0x33	51
ss             0x2b	43
ds             0x0	0
es             0x0	0
fs             0x0	0
gs             0x0	0
{% endhighlight %}

You can see the `rip` register, at the current (and right) address : 0x7ffff7ff4001
Then use ni to run to next instructions

{% highlight shell %}
(gdb) ni
0x00007ffff7ff4004 in ?? ()
1: x/i $pc
=> 0x7ffff7ff4004:	movabs $0x21,%rax
(gdb) ni
0x00007ffff7ff400e in ?? ()
1: x/i $pc
=> 0x7ffff7ff400e:	leaveq
{% endhighlight %}

Now you are on the leaveq instruction, you can verify the value of `rax` register before your binary function leave.

{% highlight shell %}
(gdb) info registers rax
rax            0x21	33
(gdb)
{% endhighlight %}

Now end the function

{% highlight shell %}
(gdb) ni
0x00007ffff7ff400f in ?? ()
1: x/i $pc
=> 0x7ffff7ff400f:	retq
(gdb) ni
0x0000000000400e98 in main (argc=3, argv=0x7fffffffe6a8) at vm.c:125
125			ret = execute();
1: x/i $pc
=> 0x400e98 <main+229>:	mov    %rax,-0x8(%rbp)
(gdb) ni
0x0000000000400e9c	125			ret = execute();
1: x/i $pc
=> 0x400e9c <main+233>:	jmp    0x400eae <main+251>
(gdb) print ret
$4 = 33
{% endhighlight %}

You can verify that you obtain the right return value, comming from `rax` register.

## lldb

To debug the binary, you have to set a breakpoint on its address, got from the fmain->binary print

{% highlight shell %}
(lldb) breakpoint set --address 0x0000000100036000
Breakpoint 2: address = 0x0000000100036000
{% endhighlight %}

Now, just continue the execution

{% highlight shell %}
(lldb) continue
Process 11376 resuming
Process 11376 stopped
* thread #1: tid = 0x8965e, 0x0000000100036000, queue = 'com.apple.main-thread', stop reason = breakpoint 2.1
    frame #0: 0x0000000100036000
->  0x100036000: pushq  %rbp
    0x100036001: movq   %rsp, %rbp
    0x100036004: movabsq $0x23, %rax
    0x10003600e: leave
Continuing.
{% endhighlight %}

You can see that the current instruction is `push %rbp` which is the first instruction of your binary.
To jump to the next instruction, you can use the ni (as next instruction) command

{% highlight shell %}
(lldb) ni
Process 11376 stopped
* thread #1: tid = 0x8965e, 0x0000000100036001, queue = 'com.apple.main-thread', stop reason = instruction step over
    frame #0: 0x0000000100036001
->  0x100036001: movq   %rsp, %rbp
    0x100036004: movabsq $0x23, %rax
    0x10003600e: leave
    0x10003600f: retq
{% endhighlight %}

To see the registers content, you can use the `register read` command

{% highlight shell %}
(lldb) register read
General Purpose Registers:
       rax = 0x0000000100036000
       rbx = 0x0000000000000000
       rcx = 0x0023b848e5894855
       rdx = 0x0000000000000008
       rdi = 0x0000000100036000
       rsi = 0x0000000100200150
       rbp = 0x00007fff5fbff9a0
       rsp = 0x00007fff5fbff940
        r8 = 0xc3c9000000000000
        r9 = 0x0000000000000000
       r10 = 0x00007fff95fb9001
       r11 = 0xffffffffffe35eb0
       r12 = 0x0000000000000000
       r13 = 0x0000000000000000
       r14 = 0x0000000000000000
       r15 = 0x0000000000000000
       rip = 0x0000000100036001
    rflags = 0x0000000000000206
        cs = 0x000000000000002b
        fs = 0x0000000000000000
        gs = 0x0000000000000000
{% endhighlight %}

Then use ni to run to next instructions

{% highlight shell %}
(lldb) ni
Process 11477 stopped
* thread #1: tid = 0x8afba, 0x00000001000015d6 langvm`main(argc=3, argv=0x00007fff5fbff9c8) + 262 at vm.c:125, queue = 'com.apple.main-thread', stop reason = instruction step over
    frame #0: 0x00000001000015d6 langvm`main(argc=3, argv=0x00007fff5fbff9c8) + 262 at vm.c:125
   122 			/* cast binary into function pointer */
   123 			uint64_t (*execute)(void) = (void *)fmain->binary;
   124 			/* and call this function */
-> 125 			ret = execute();
   126 		}
   127 		else {
   128 			/* start interpreter, get return value and print it */
{% endhighlight %}

lldb detect the end of the binary function, and jump over it.
you can check `rax` register value with `register read`

{% highlight shell %}
(lldb) register read rax
     rax = 0x0000000000000023
(lldb)
{% endhighlight %}

Now end the function

{% highlight shell %}
(lldb) ni
Process 11477 stopped
* thread #1: tid = 0x8afba, 0x00000001000015da langvm`main(argc=3, argv=0x00007fff5fbff9c8) + 266 at vm.c:126, queue = 'com.apple.main-thread', stop reason = instruction step over
    frame #0: 0x00000001000015da langvm`main(argc=3, argv=0x00007fff5fbff9c8) + 266 at vm.c:126
   123 			uint64_t (*execute)(void) = (void *)fmain->binary;
   124 			/* and call this function */
   125 			ret = execute();
-> 126 		}
   127 		else {
   128 			/* start interpreter, get return value and print it */
   129 			ret = interpret(fmain);
(lldb) print ret
(int64_t) $3 = 35
{% endhighlight %}

You can verify that you obtain the right return value, comming from `rax` register.

# What's next
With the next post, we will start to use variables, types and basic operations for your language.
