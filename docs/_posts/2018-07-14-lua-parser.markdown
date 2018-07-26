---
layout: post
title:  "First language parser"
date:   2018-07-14
author: Rémi Bauzac

---

An interpreted language is divided in 2 parts:

* High level language: human readable (and understandable) source code
* Intermediate language: commonly, a binary representation of the high level language transformation. Most of the time, this language is hidden to the user. The intermediate language is interpreted by the virtual machine.

# Definitions

## High level language
The high level language is understandable by a human, so that he can easily interact with it.
As an example, the following Lua piece of code:

{% highlight lua %}
local a = 4
local b = 5
if a > b then print('a is greater than b !') end
{% endhighlight %}

## Intermediate language
The intermediate language is mainly a command list (called _operations_) which represents high level language as a binary structure (sometimes modified or optimized). Each operation contains an action (called _operation code_) and parameters. The intermediate language is commonly called _bytecode_.

You can represent the list of possible _operation codes_ like the following C enum:

{% highlight c %}
typedef enum {
  OP_RETURN,
  ...
} opCode;
{% endhighlight %}

# Lua
All the exercises are built around the [Lua language](http://www.lua.org), version 5.3.5.
Lua is a powerful, efficient, lightweight, embeddable scripting language. It supports procedural programming, object-oriented programming, functional programming, data-driven programming, and data description.

Lua was born in 1993. It combines simple procedural syntax with powerful data description constructs based on associative arrays and extensible semantics. Lua is dynamically typed, runs by interpreting bytecode with a register-based virtual machine, and has automatic memory management with incremental garbage collection, making it ideal for configuration, scripting, and rapid prototyping.

Lua owns a huge and very active community. All the documentation (for users as for developers) is available on [Lua website](http://www.lua.org).

Lua is:

* Fast: mainly adopted by video games teams for non-graphical logic;
* Portable: Lua is distributed in a small package and builds out-of-the-box in all platforms that have a standard C compiler;
* embeddable: Lua is a fast language engine with small footprint that you can embed easily into your application. Lua has a simple and well documented API that allows strong integration with code written in other languages;
* Powerful: A fundamental concept in the design of Lua is to provide meta-mechanisms for implementing features, instead of providing a host of features directly in the language
* Small: Adding Lua to an application does not bloat it. About 24K lines of code
* Free: MIT License


# Implementation
JiT an interpreted language involve three main components:

* The compiler, to transform the high level language to an intermediate language, with:
  * Lexical and syntax analysis
  * Intermediate language generation
  * Optimisations (optional)
* The interpreter, to create environment and execute commands
* And the JiT compiler, to create executable code.

This blog post is focused on the Lua compiler, with the smallest lua script available :

{% highlight lua %}
return 3
{% endhighlight %}

## syntax analysis and parsing
The API function to load lua code into the interpreter is 

{% highlight c %}
LUA_API int lua_load (lua_State *L, lua_Reader reader, void *data,
                      const char *chunkname, const char *mode);
{% endhighlight %}

After a maze of functions (as only Lua's developers know), the main parsing function (located in `lparse.c`) is

{% highlight c %}
LClosure *luaY_parser (lua_State *L, ZIO *z, Mbuffer *buff,
                       Dyndata *dyd, const char *name, int firstchar);
{% endhighlight %}

This function convert Lua code into a "Lua Closure". This Closure represents a function (in our example, the main one) accompanied by all the elements necessary for its execution :

* code (list of instructions)
* Link to global variables (upvalues)
* Constants (named k)
* debug informations
* number of input parameters
* embedded functions

The Lua parsing process is simple and not involve optimisations (type propagations, loops breakers, etc.).

This Lua Closure is then dump into a binary format, which is the  Lua **intermediate language**, commonly named _bytecode_.
The dump function is named `luaU_dump()` and located into `ldump.c` file.

The code of Lua is quitely easy to crawl and undestand, if you need more informations, feel free to have a look on it.

## Lua bytecode
Before jumping into JiT, the most important thing is to understand the bytecode :

* How an instruction is build;
* How constants and variables are managed;
* How functions are declared and called
* ...

The bytecode format of Lua code begin with an header. This header helps to recognize Lua format, and checks basic types size and endianness.  

* The Lua signature, to recognize Lua bytecode: `0x1b 0x4c 0x75 0x	61`
* The Lua version: `0x53` for 5.3
* Tha Lua format : `0x00`, the official one
* Verification data:
  * Data for file corruption checks: `0x19 0x93 0x0d 0x0a 0x1a 0x0a`
  * Data for size checks:
     * `sizeof(int)`;
     * `sizeof(size_t)`;
     * `sizeof(Instruction)`;
     * `sizeof(lua_Integer)`;
     * `sizeof(lua_Number)`
  * Data for endianness and float format: 
     *  Integer: `0x5678`
     *  Float: `370.5`

After the header, the Lua bytecode continue with the functions (beginning with the main one) :

* Debug informations
* arguments/stack handling
* Code
* Constants
* Upvalues
* Included functions

For our example, the generated bytecode is (please be careful with the endianness, all the intergers are in reverse order) :

```

0x1b 0x4c 0x75 0x61  -- signature
0x53 -- version 5.3
0x00 -- Lua official format
0x19 0x93 0x0d 0x0a 0x1a 0x0a -- file corruption check
0x04 -- sizeof integer
0x08 -- sizeof size_t (aka long)
0x04 -- sizeof Lua internal instruction
0x08 -- lua_Integer (On Linux/x86_64, lua_Integer is 64 bits long)
0x08 -- lua_Number (On Linux/x86_64, lua_Integer is 64 bits long)
0x78 0x56 0x00 0x00 0x00 0x00 0x00 0x00 -- Integer endianness check
0x00 0x00 0x00 0x00 0x00 0x28 0x77 0x40 -- Float checks
0x01 -- number of Upvalues (_ENV table)

-- here start the main function dump

0x10 0x40 0x6c 0x75 0x61 0x2d 0x70 0x61 0x72 0x73 0x65 0x72 0x2e 0x6c
0x75 0x61 -- String variable "@lua-parser.lua", for debug purpose
0x00 0x00 0x00 0x00 -- first defined line (4 bytes integer)
0x00 0x00 0x00 0x00 -- last defined line (4 bytes integer)
0x00 -- number of input params
0x01 -- is variable arguments ? 
0x02 -- Max stack size
0x03 0x00 0x00 0x00 -- number of Lua instructions

-- here start list of instructions (size of Instruction is 0x04 as describe  

0x01 0x00 0x00 0x00 -- first instruction (detailed later)
0x26 0x00 0x00 0x01 -- second instruction (detailed later)
0x26 0x00 0x80 0x00 -- third instruction (detailed later)

-- here start constants

0x01 0x00 0x00 0x00 -- number of constants (one in this case)
13 03 00 00 00 00 00 00 00 -- constant (Lua integer) = 3

-- here start the upvalues

0x01 0x00 0x00 0x00 -- number of upvalues (one in this case)
0x01 -- upvalue in stack ?
0x00 -- upvalue ID

-- here start the dump of another function inside the "main one"

0x00 0x00 0x00 0x00 -- there is no functions
 
-- here start debug informations (no need to describe here)

0x03 0x00 0x00 0x00 0x01 0x00 0x00 0x00 
0x01 0x00 0x00 0x00 0x01 0x00 0x00 0x00
0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x00
0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x05
0x5f 0x45 0x4e 0x56

```

## Lua instructions

Lua instructions are unsigned numbers (4 bytes in our case, Linux x86_64).
All instructions have an opcode in the first 6 bits, OP_LOADK, OP_RETURN for example (please have a look to lopcodes.h file).

Instructions can have the following fields, regarding to the needs in term of argument size:

* 'A' : 8 bits
* 'B' : 9 bits
* 'C' : 9 bits
* 'Ax' : 26 bits ('A', 'B', and 'C' together)
* 'Bx' : 18 bits ('B' and 'C' together)
* 'sBx' : signed Bx

```
 Instruction
--------------------------------------------------------------
|     6 bits    |    8 bits    |    9 bits    |    9 bits    |
--------------------------------------------------------------
|  Instruction  |       A      |       B      |      C       |
--------------------------------------------------------------
|  Instruction  |                 Ax                         |
--------------------------------------------------------------
|  Instruction  |       A      |              Bx             |
--------------------------------------------------------------
|  Instruction  |       A      |              sBx            |
--------------------------------------------------------------

```


A signed argument is represented in excess K; that is, the number value is the unsigned value minus K. K is exactly the maximum value for that argument (so that -max is represented by 0, and +max is represented by 2*max), which is half the maximum for the corresponding unsigned argument.

For our example, the 3 instructions are :

* 0x01 0x00 0x00 0x00 :
  * 0x01 is OP_LOADK instruction. This instruction get fields A (8 bits) and Bx (18 bits).
  * A is the relative pointer to the stack
  * Bx is the pointer to the constants stack. So the constant of ID 0 is loaded to the stack ID 0. The constant ID 0, as described in the Lua bytecode example, is the number 3.
* 0x26 0x00 0x00 0x01:
   * 0x26 is the OP_RETURN instruction. This instruction return from a function  with values from A ID of the stack to A+B-2 ID of the stack
* 0x26 0x00 0x80 0x00: return instruction, automatically added by Lua parser at the end of all binary


# Build

Before building, you need to install:

* Xcode command line extensions using `xcode-select --install` in command line terminal for MacOS

The code is [here](https://github.com/RemiBauzac/jit/tree/lua-parser). To build Lua, clone the project and run `make <target>` into src directory. The codes build easily on Linux or MacOS:

{% highlight shell %}
$ git clone git@github.com:RemiBauzac/jit.git
$ cd jit && git checkout first-language-parser
$ # For Linux
$ cd src && make linux
$ # For MacOSX
$ cd src && make macosx
$ ...
{% endhighlight %}

# Run
A Lua file is ready in the example directory : `lua-parser.lua`. You can use the luac (as Lua compiler) to compile the lua file to bytecode.

{% highlight shell %}
$ cd examples
$ ../src/src/luac -o lua-parser.bytecode lua-parser.lua 
$ hexdump -C lua-parser.bytecode
00000000  1b 4c 75 61 53 00 19 93  0d 0a 1a 0a 04 08 04 08  |.LuaS...........|
00000010  08 78 56 00 00 00 00 00  00 00 00 00 00 00 28 77  |.xV...........(w|
00000020  40 01 12 40 2e 2f 6c 75  61 2d 70 61 72 73 65 72  |@..@./lua-parser|
00000030  2e 6c 75 61 00 00 00 00  00 00 00 00 00 01 02 03  |.lua............|
00000040  00 00 00 01 00 00 00 26  00 00 01 26 00 80 00 01  |.......&...&....|
00000050  00 00 00 13 03 00 00 00  00 00 00 00 01 00 00 00  |................|
00000060  01 00 00 00 00 00 03 00  00 00 01 00 00 00 01 00  |................|
00000070  00 00 01 00 00 00 00 00  00 00 01 00 00 00 05 5f  |..............._|
00000080  45 4e 56                                          |ENV|
00000083
{% endhighlight %}

The luac binary can dump the code too, in a (quitely) human readable format.

{% highlight shell %}
$ cd examples
$ ../src/src/luac -o lua-parser.bytecode -l -l lua-parser.lua 
main <./lua-parser.lua:0,0> (3 instructions at 0x7fc6bfc03220)
0+ params, 2 slots, 1 upvalue, 0 locals, 1 constant, 0 functions
	1	[1]	LOADK    	0 -1	; 3
	2	[1]	RETURN   	0 2
	3	[1]	RETURN   	0 1
constants (1) for 0x7fc6bfc03220:
	1	3
locals (0) for 0x7fc6bfc03220:
upvalues (1) for 0x7fc6bfc03220:
	0	_ENV	1	0
{% endhighlight %}


# What's next ?
The next post will be about loading and interpreting this bytecode. See you soon.

