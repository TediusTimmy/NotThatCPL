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
#ifndef NOTTHATCPL_EXCEPTIONS_H
#define NOTTHATCPL_EXCEPTIONS_H

#include <exception>

// Thrown by the compiler when an unimplemented part of CPL is encountered.
class Unimplemented final : public std::exception
 {
private:
   std::string message;
public:
   explicit Unimplemented(const std::string& message) : message(message) { }
   ~Unimplemented() throw() { }
   const char * what() const throw() { return message.c_str(); }
 };

// Thrown by any part of the code when a condition that is deemed impossible occurs.
// Like an assertion, but in production code. I've done quite a bit of software maintenance,
// and assertions are useless in the maintenance phase of a program's life.
class ProgrammerError final : public std::exception
 {
private:
   std::string message;
public:
   explicit ProgrammerError(const std::string& message) : message(message) { }
   ~ProgrammerError() throw() { }
   const char * what() const throw() { return message.c_str(); }
 };

// Thrown by the interpreter when STOP is encountered.
// Other cases:
//    1 : cepl file not provided
//    2 : cepl file not found / access denied
//    3 : compile failure
//    4 : logic error
class StopCode final : public std::exception
 {
private:
   int code;
public:
   explicit StopCode(int code) : code(code) { }
   int getCode() const { return code; }
 };

#endif /* NOTTHATCPL_EXCEPTIONS_H */
