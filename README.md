# Cgoes Cminus Compiler (CCC)
A compiler for Robert Heckendorn's C- (cminus) language. 

## A note on plagarism
If you are currently in the class: *DO NOT USE THIS CODE IN YOUR PROJECT*. 
MIT License does *not* enable its use in a school assignment. 
Plagiarizing will get you suspended or expelled, I've seen it happen before.
You're welcome to read it and fill gaps in understanding in Flex/Bison, such as function calls and syntax, *not* the algorithms and logical structure of the program. 


# Using the CCC
It is recommended you run this on a distribution of Linux.

## Usage
```
    ./c- <inputfile> [flags]
    tm <objectfile>.tm
```

## Flags
```
-d      Enable yydebug for flex/bison debugging
-t      Testing flag, for general debugging
-p      Print the syntax tree generated by bison
-P      Print the annotated syntax tree (annotated with types, etc)
-o [ <filename> | - ]  Generate TM instructions. 
(Default: "fred.tm". <filename>: outputs to the filename. '-': stdout)
```

## Environment requirements
    Bison 3.0 or higher (Hard requirement, can confirm 2.5 freaks out)
    Flex 2.5.39 or higher (I think flex 2.5.37 will work as well)
    gcc 4.9 or higher (Hard requirement, C++11)
    Make 4.0 (Most any version will work fine)

## Quickstart guide
    git clone <repo>
    cd <repo>/src
    make tm # Makes the Tiny Machine emulator
    make    # Makes CCC
    ./c- 6tests/scope35.c- -P -o scope35.tm
    tm scope35.c-
    g

# License
    Code and documentation copyright 2015-2018 Christopher Goes.
    All code and documentation released under the MIT License.
