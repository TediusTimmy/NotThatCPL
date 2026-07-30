Notes
=====

In no particular order. The first note is that the program structure is pretty bad.
It was mostly developed in a top-down, bricolage manner.
Parts are there and then because that's when and where I needed them.


## RECORD

Only supported as
```
RECORD CMDREC(1)
   STRING CMD(1)
ENDREC
```
This automatically puts the console in raw mode for character-based IO.


## DIRECT

Only supported as
```
DIRECT
   SVC 7
CPL
```
Doesn't do anything, because I don't know what it does.


## FORMAT

Length specifiers are ignored.


## OPEN

Only supports one `access` per command invocation.
```
   OPEN access file
   OPEN access (file, ...)
```


## CURS

Ignores the file - always the one and only screen. Also, asserts that the count is 1.
Doesn't check that the string has length one, just uses it's first character.


## MIN and MAX

Only take two arguments.


## Assignment

Only handles one left-hand-side value.


## RETRIEVE

This unholy abomination against all that is good has only been
implemented for NUMBER and STRING, as I don't even know how the
self-modifying code versions would work.

## TABLE

The documentation isn't clear if table indexes are 1-based or 0-based.
I am guessing 1-based, because there is a statement about the meaning of TABLE(0).
I have implemented them as 1-based with TABLE(0) being the work area
(which the statement says IS NOT true).

