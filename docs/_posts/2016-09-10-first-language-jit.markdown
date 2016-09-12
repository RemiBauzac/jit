---
layout: post
title:  "First language JiT"
date:   2016-09-12 22:00:20 +0200
author: Rémi Bauzac

---

In the [previous post](http://jit.bauzac.net/blog/first-language-vm) you built the virtual machine to interpret the bytecode.
Hard work start now with the first JiT compiler for the virtual machine.

To create this JiT compiler, you need:

* Build executable binary from the bytecode
* Jump to this executable memory part
* Jump back to the virtual machine

To acheive all of these steps, you have to design the executable binary as a function (from processor point of view), call this function from virtual machine (in C code) and return from the the JiT function to the virtual machine.

# Build binary executable
As the executable will be run by a processor, it will be processor specific (x86, x86_64, armv7, armv9, etc.). This workshop is design for most common processor: x68_64.
Before starting, please takes 5 minutes to read [this article](https://en.wikipedia.org/wiki/X86) about x68_64 processors. I don't have ambition to describe all of x86 processor behaviors. The article does it better than I can, but you need some keys to continue:

* x86 instruction size is variable, from 1 to 15 bytes.
* Registers are like processor variables, each have a specific behavior. You will discover it along the posts.
* Registers have differents size in bits, from 512 to 8. The name of the register reveal it size, for example:
  * _rax_ is 64 bits integer register
  * _eax_ is the related 32 bits register
  * _ax_ is the related 16 bits register
  * _ah_ and _al_ (h as high and l as low) for 8 bits register
* Some registers behavior :
  * _rbp_ is the base frame pointer for the stack (accesses are relative, so we need a base pointer)
  * _rsp_ is the stack pointer (current position)
  * _rip_ is the instruction pointer (point to the current executed instruction, it's really helpful)
  * _rax_ is scratch register used as function returned value
 

## Build binary overview
To buibld binary, you have to :

* Allocate enought space on a temporary buffer

{% highlight c %}
  /**
   * Alloc temporary buffer.
   * Its goal is:
   *   - build the binary
   *   - know the binary size to allocate executable memory
   */
  prog = malloc(f->codesz*MAX_OPSZ);
  if (!prog) return r;
{% endhighlight %}

* Build binary from bytecode (intermediate language)

{% highlight c %}
/**
   * Start to write binary data into temporary buffer.
   * Begin with function prologue,
   * Then write data for each operations
   * End with function epilogue
   */
  orig = prog;
  /* Write function prologue to prog */
  JIT_PROLOGUE;
  for (pc = 0; pc < f->codesz; pc++) {
    operation op = f->code[pc];
    if (create_op[op.op]) prog = create_op[op.op](prog, &op);
  }
  JIT_EPILOGUE;
{% endhighlight %}

* Allocate executable memory with the size of generated binary (in the temporary buffer)

{% highlight c %}
  /* Get size and alloc memory */
  f->binarysz = prog - orig;
  if ((r = bin_alloc(f)) != 0) {
    f->binarysz = 0; f->binary = NULL;
    return r;
  }
{% endhighlight %}

* Copy binary to executable memory

{% highlight c %}
  /* copy data to executable memory */
  memcpy(f->binary, orig, f->binarysz);
{% endhighlight %}

Now, let's focus on tricky parts.


## Executable memory allocation (`bin_alloc` call in the example)

It's easy to allocate memory, using malloc function. It's more complexe to make memory executable because :

* Executable memory has to be aligned to OS memory page size
* Making memory executable is OS dependant for security reasons

For Linux, you have to 

* allocate memory, aligned to OS page size :

{% highlight c %}
  f->binary = memalign(PAGE_SIZE, f->binarysz);
  if (!f->binarysz) return 1;
{% endhighlight %}

* Make it executabe with memory protect

{% highlight c %}
  if (mprotect(f->binary, f->binarysz, PROT_EXEC|PROT_READ|PROT_WRITE) == -1) {
    bin_free(f);
    return 1;
  }
{% endhighlight %}


For MacOS, you have to :

* Compute allocation size:

{% highlight c %}
  int toalloc = f->binarysz;

  if (f->binarysz % pagesz) {
    toalloc = f->binarysz + (pagesz - f->binarysz%pagesz);
  }
{% endhighlight %}

* Create a memory mapping with mmap:

{% highlight c %}
  f->binary = mmap(0, toalloc, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
  if (f->binary == MAP_FAILED) {
    f->binary = NULL;
    return 1;
  }
{% endhighlight %}


## Build function binary
From the processor point of view, a function always starts with a prologue, and ends with an epilogue.

A prologue is used to:

* Save a copy of frame pointer in the stack : `push %rbp` in assembly language and `0x55 0x48` in binary
* set the new frame pointer at the current stack position : `mov %rsp, %rbp` in assembly language and `0x89 0xe5` in binary 

So, a function always starts with these 4 bytes : `0x55 0x48 0x89 0xe5`.

An epilogue is built with 2 instructions:

* `leaveq` instruction: is the inverse operation of prologue
* `retq`instruction : return to the caller

These 2 instructions are `0xc9 0xc3` in binary.

Between prologue and epilogue, you need to build binary from the bytecode. In this exemple, you have only one operation : `return`.

In the virtual machine for loop, the `return` operation set a return 64 bits value as a result  :
{% highlight c %}
/* walking through main function */
  for(pc = 0; pc < fmain->codesz; pc++) {
    op = fmain->code[pc];
    switch(op.op) {
      /* On return, get the return value from parameters and exit */
      case OP_RETURN:
        ret = op.param;
        break;
      /* Handle error case */
      case OP_NONE:
      default:
        fprintf(stderr, "Unavailable operand %u\n", op.op);
        exit(1);
    }
  }
{% endhighlight %}

On x86_64 processors, the result has to be set to _rax_ register to be handled by the caller. So you just need to set `op.param` to the _rax_ register :

{% highlight c %}
  /* mov op->param, %rax */
  APPEND2(0x48, 0xb8);
  APPEND(op->param, 8);
{% endhighlight %}

In the piece of code above,  APPEND2 and APPEND put binary data to the end of a buffer :

{% highlight c %}
static inline uint8_t *append(uint8_t *ptr, uint64_t bytes, unsigned int len)
{
  if (ptr) {
        if (len == 1) {
            *ptr = bytes;
    }
        else if (len == 2) {
            *(uint16_t *)ptr = bytes;
    }
        else if (len == 4) {
            *(uint32_t *)ptr = bytes;
        }
    else {
      *(uint64_t *)ptr = bytes;
    }
  }
  return ((uint8_t *)ptr) + len;
}

static inline uint8_t *appendoff(uint8_t *ptr, uint32_t offset) {
    *(uint32_t *)ptr = offset;
    return ptr + sizeof(uint32_t);
}

#define APPEND(bytes, len)        do { prog = append(prog, bytes, len); } while (0)
#define APPENDOFF(offset)         do { prog = appendoff(prog, offset); } while (0)

#define APPEND1(b1)               APPEND(b1, 1)
#define APPEND2(b1, b2)           APPEND((b1) + ((b2) << 8), 2)
#define APPEND3(b1, b2, b3)       APPEND2(b1,b2);APPEND1(b3)
#define APPEND4(b1, b2, b3, b4)   APPEND((b1) + ((b2) << 8) + ((b3) << 16) + ((b4) << 24), 4)

{% endhighlight %}


# Jump to the binary
Jump to the binary is really easy. The binary piece of code is built as a function. You just have to declare a function pointer in C :

{% highlight c %}
	/* cast binary into function pointer */
    uint64_t (*execute)(void) = (void *)fmain->binary;
{% endhighlight %}

Then call the function (execute in this example) : 
{% highlight c %}
	ret = execute();
{% endhighlight %}

# Jump back to the virtual machine
The Jump back is acheive by epilogue in the generated binary.


# include JiT compiler into the virtual machine
To easily test the JiT compiler and the virtal machine, you have to handle JiT case with a swtich during the vm call (-j for example) :

{% highlight shell %}
$ langvm
usage: ./langvm [-j] <file>
{% endhighlight %}

To include JiT compiler in virtual machine, you just have to call the binary creator (if JiT switch is on):

{% highlight c %}
  /* create executable */
  if (jit) {
    create_binary(fmain);
  }
{% endhighlight %}

And jump to the binary, failing back to interpreter on error
{% highlight c %}
  if (fmain->binary) {
    /* cast binary into function pointer */
    uint64_t (*execute)(void) = (void *)fmain->binary;
    /* and call this function */
    ret = execute();
  }
  else {
    /* start interpreter, get return value and print it */
    ret = interpret(fmain);
  }
{% endhighlight %}


# Build
The code of the first JiT compiler is [here](https://github.com/RemiBauzac/jit/tree/first-language-jit). The Makefile not allow cross-compilation. So you have to compile and run the source on the same machine (same architecture and same OS).

{% highlight shell %}
$ git clone git@github.com:RemiBauzac/jit.git
$ cd jit && git checkout first-language-jit
$ cd src && make ARCH=x86_64 OS=linux  # you can replace linux by macos if you compile and run on MacOS.
...
{% endhighlight %}

# Run
To run the virtual machine with JiT compiler, just call it with `-j` switch.

{% highlight shell %}
$ langvm -j out.bc
Return value is 35
$ langvm out.bc
Return value is 35
{% endhighlight %}

Your first JiT compiler is done !
 

# What's next
With the next post, you will learn how to debug JiT binary memory with gdb.
