# REBEL 15x2 source code for Linux
Ed Schroder's REBEL 15x2 source code that compiles on linux

# Prerequisites
1. A processor that supports the AVX2 instruction set

  `grep avx2 /proc/cpuinfo` should return a non-empty list

2. Following libraries installed on your system : libpthread, libm, libc
3. g++ compiler
4. bzip2 to decompress the weights of the neural nets

# How to build from source
1. clone this repository onto your local machine

2. open a terminal and cd to the 'src' directory of the cloned repository
   
   `bzip2 -d *.bz2` to decompress the weights of the neural nets
   
   `make`
   
   `./rebel15x2` will start REBEL 15x2 UCI interface in the terminal.
   
   You can now quit the engine and terminal and add the executable rebel15x2 as an engine to your favorite chess GUI.

# Credits & Copyrights
- Ed Schroder for REBEL 15x2

  Credits (quoting Ed Schroder's Rebel 15 homepage : http://rebel13.nl/windows/rebel-15.html):
    
  _"Fabien Letouzey, Thomas Gaksch and Jerry Donald Watson for Toga._
    
  _Chris Whittington for introducing me into NNUE, providing me with the necessary tools and writing the NNUE interference code._
   
  _Thorsten Czub for testing Rebel 15._

  _Rebel 15 is released under the GPLv3 licence."_

- Compatibility <intrin.h> header for GCC -- GCC equivalents of intrinsic Microsoft Visual C++ functions. Originally developed for the ReactOS (<http://www.reactos.org/>) and TinyKrnl (<http://www.tinykrnl.org/>) projects.

  Copyright (c) 2006 KJK::Hyperion <hackbunny@reactos.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   
