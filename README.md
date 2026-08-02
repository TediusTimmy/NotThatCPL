NotThatCPL
==========

This is an interpreter for the CPL programming language. No. NOT THAT CPL.
Not Cambridge Programming Language or Combined Programming Language (same language).
This is Centurion Programming Language, and is has just ELSE.

Centurion Programming language takes inspiration from FORTRAN.
The Centurion Computer Corporation was a small computer company from Texas that made
computers and accounting software for banks, hospitals, and other businesses.
All-in-one package: computer-software-support (for both).
And they wrote their own language to write all their software in.

Where did this come from? David Lovett of the YouTube channel Usagi Electric released
a video about how bad BASIC was for the Centurion. It compiles BASIC to a p-code,
but then the p-code is slow as sin. ( https://www.youtube.com/watch?v=bpdYBsgYF_c )
I thought to myself, I could write a BASIC interpreter, but my options for so doing
are Centurion Assembly (though porting 6502 BASIC would probably not be bad), and CPL.
However, both can only be run in an emulator, and only after a secret handshake with
Usagi because they are part of the Centurion Operating System, which has an active
rights holder. Apparently, they have two whole customers still.

So, I decided to do what Gates and Allen did to write Microsoft BASIC (8080 version):
write an interpreter for the target. CPL isn't that bad.  
I've written more cancerous languages myself.

After writing the CPL interpreter, I don't know if I would want to write a BASIC
interpreter in CPL: the string support in CPL is so lacking that it would be painful
to decode a string of BASIC in CPL.

Anyway, here is an interpreter for the CPL language, so that you can write programs
for Usagi Electric to show off. It tries to be accurate, but there are just things that
I don't know. What does SVC 7 do? Who knows whether any semiconductors exist?
But, so long as you don't try fancy things, you can write programs that can be run on
his old computer.

You should see the [notes](Notes.md) on how things were implemented.
Also, look at the sources [REAMDE](sources/README.md) for what is currently running.


Debugger
--------

The default build has a debugger. It can be removed to just run your programs.
The debugger will print out the deblanked source, then the compiled, deblanked source,
and then you can enter commands. Also, the current command is supposed to be
emphasized.

`RUN` - Run the program, or re-run it, or continue it.  
`STEP` or `NEXT` - Take a step of one command in the code.  
`QUIT` - Quit the debugger (and program).  
`PRINTS` - Print a string expression.  
`PRINTN` - Print a numeric expression.  
`HELP` - Get help.  
`STACK` - Print the call stack.  

It tries to do sane things when you press Ctrl-C to interrupt it, but,
interrupting it while waiting for input doesn't work right.
