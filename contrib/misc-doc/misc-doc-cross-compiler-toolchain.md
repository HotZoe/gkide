# [Cross Toolchain Types](http://crosstool-ng.github.io/docs/toolchain-types/)
There are four kinds of toolchains you could encounter.

First off, you must understand the following:

when it comes to compilers there are up to four machines involved:
- the machine configuring the toolchain components: the **config** machine
- the machine building the toolchain components: the **build** machine
- the machine running the toolchain: the **host** machine
- the machine the toolchain is generating code for: the **target** machine

We can most of the time assume that the **config** machine and the **build** machine are the same.
Most of the time, this will be true. The only time it isn’t is if you’re using distributed compilation
(such as distcc). Let’s forget this for the sake of simplicity.

So we’re left with three machines:
- **build**: the machine configuring & building the toolchain components
- **host**: the machine running the toolchain
- **target**: the machine the toolchain is generating code for

Any toolchain will involve those three machines. You can be as pretty sure of this as “2 and 2 are 4”.
Here is how they come into play:
- *build* `==` *host* `==` *target* (**native**)
  This is a plain native toolchain, targeting the exact same machine as the one it is built on, and
  running again on this exact same machine. You have to build such a toolchain when you want to use
  an updated component, such as a newer gcc for example.

- *build* `==` *host* `!=` *target* (**cross**)
  This is a classic cross-toolchain, which is expected to be run on the same machine it is compiled on,
  and generate code to run on a second machine, the target.

- *build* `!=` *host* `==` *target* (**cross-native**)
  Such a toolchain is also a native toolchain, as it targets the same machine as it runs on.
  But it is build on another machine. You want such a toolchain when porting to a new architecture,
  or if the build machine is much faster than the host machine.

- `build` != `host` != `target` (**canadian**)
   This one is called a **Canadian Cross** toolchain, and is tricky. The three machines in play are different.
   You might want such a toolchain if you have a fast build machine, but the users will use it on another machine,
   and will produce code to run on a third machine.

# More Ref Materials Links
- [MinGW-W64 Distributions](http://mingw-w64.org/doku.php/download)
- [Difference between Win-builds vs MinGW-builds](https://stackoverflow.com/questions/20495558/difference-between-win-builds-vs-mingw-builds/20502212)
- [How to Build a GCC Cross-Compiler](http://preshing.com/20141119/how-to-build-a-gcc-cross-compiler/)
- [How are msys, msys2, and msysgit related to each other?](https://stackoverflow.com/questions/25019057/how-are-msys-msys2-and-msysgit-related-to-each-other)
- [交叉编译器制作流程](http://blog.csdn.net/colin719/article/details/758000?locationNum=8)
- [关于Msys2和MinGW-w64](http://blog.csdn.net/yehuohan/article/details/52090282)
- [MinGW-64-bit(QtDoc)](http://wiki.qt.io/MinGW-64-bit)
- [关于MSYS2的使用](http://bbs.chinaunix.net/thread-4256341-1-1.html)
- [令人头疼的命名: MinGW, MinGW-W64, Win32, Win64](http://www.cnblogs.com/foohack/p/3877276.html)