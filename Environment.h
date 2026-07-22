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
#ifndef NOTTHATCPL_ENVIRONMENT_H
#define NOTTHATCPL_ENVIRONMENT_H

#include "Execution.h"

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <map>
#include <ios>

class IntVar final
 {
private:
   int64_t value;
public:
   std::string name;
   int64_t size;
   int64_t getValue() const { return value; }
   void setValue(int64_t);
   IntVar(std::string name, int64_t size, int64_t value) : value(value), name(name), size(size) { }
 };

class StrVar final
 {
private:
   std::string value;
public:
   std::string name;
   size_t size;
   std::string getValue() const { return value; }
   void setValue(const std::string&);
   StrVar(std::string name, size_t size, std::string value) : value(value), name(name), size(size) { }
 };

class FileDecl
 {
public:
   virtual void open(std::ios_base::openmode) = 0;
   virtual void close(void) = 0;
   virtual void clearScreen(void) = 0;
   virtual void putStr(const std::string& val) = 0;
   virtual void putLine(const std::string& val) = 0; // putStr(); NewLine();
   virtual bool getStr(std::string&) = 0;
   virtual char getChar(void) = 0;
   virtual ~FileDecl() {};
 };

class DebugInfo final
 {
public:
   std::map<std::string, size_t> intVars;
   std::map<std::string, size_t> strVars;
 };

class Environment final
 {
public:
   std::vector<std::string> fileNames;
   std::vector<std::string> source;
   DebugInfo symbols;
   std::vector<IntVar> intVars;
   std::vector<StrVar> strVars;
   std::vector<std::unique_ptr<FileDecl> > files;
   std::vector<std::unique_ptr<ICode> > icode;
   size_t entry;
   size_t pc;

   Environment() : entry(static_cast<size_t>(-1)) { }
 };

#endif /* NOTTHATCPL_ENVIRONMENT_H */
