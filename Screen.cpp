/*
BSD 3-Clause License

Copyright (c) 2026, Thomas DiModica
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <iostream>
#include <limits>

#include "Screen.h"

void InitScreen(void)
 {
   // We are currently using ANSI escape sequences, so there is nothing to do.
 }

void DestroyScreen(void)
 {
   // Make sure the screen is cooked before leaving.
   Cook();
 }

// The default implementation will be Linux signals / ANSI escapes.
#ifndef NOTTHATCPL_NATIVE_WINDOWS

#include <termios.h>
#include <unistd.h>

void Raw(void)
 {
#ifndef NOTTHATCPL_NO_SCREEN_CHANGES
   struct termios tochange;
   tcgetattr(STDIN_FILENO, &tochange);
   tochange.c_lflag = ISIG | IEXTEN;
   tcsetattr(STDIN_FILENO, TCSANOW, &tochange);
   std::cout << "\33[?25l";
#endif
 }

void Cook(void)
 {
#ifndef NOTTHATCPL_NO_SCREEN_CHANGES
   struct termios tochange;
   tcgetattr(STDIN_FILENO, &tochange);
   tochange.c_lflag = TTYDEF_LFLAG;
   tcsetattr(STDIN_FILENO, TCSANOW, &tochange);
   std::cout << "\33[?25h";
#endif
 }

int GetChar(void)
 {
   return std::cin.get();
 }

#else /* NOTTHATCPL_NATIVE_WINDOWS */

// Currently, even Microsoft is encouraging the use of ANSI escape sequences.
#include <conio.h>

void Raw(void)
 {
#ifndef NOTTHATCPL_NO_SCREEN_CHANGES
   std::cout << "\33[?25l";
#endif
 }

void Cook(void)
 {
#ifndef NOTTHATCPL_NO_SCREEN_CHANGES
   std::cout << "\33[?25h";
#endif
 }

int GetChar(void)
 {
   return ::getch();
 }

#endif /* ! NOTTHATCPL_NATIVE_WINDOWS */

void Emphasis(void)
 {
   std::cout << "\33[1;34m";
 }

void Normal(void)
 {
   std::cout << "\33[22;39m";
 }

void GotoXY(int x, int y) // Zero-based
 {
#ifndef NOTTHATCPL_NO_SCREEN_CHANGES
   std::cout << "\33[" << (y + 1) << ";" << (x + 1) << "H";
#endif
 }

bool GetString(std::string& str)
 {
   return !!std::getline(std::cin, str);
 }

void PutString(const char* str)
 {
   std::cout << str;
 }

void NewLine(void)
 {
   std::cout << std::endl;
 }

void ClearScreen(void)
 {
#ifndef NOTTHATCPL_NO_SCREEN_CHANGES
   std::cout << "\33[2J";
#endif
 }

void ClearInputFlags(void)
 {
   std::cin.clear();
   std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
 }

void Backspace(void)
 {
   std::cout << '\b';
 }

void Flush(void)
 {
   std::cout.flush();
 }
