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
#include "Execution.h"
#include "Environment.h"
#include "Screen.h"
#include "Exceptions.h"

#include <algorithm>

void StrVar::setValue(const std::string& val)
 {
   value = val;
   if (value.length() > size)
    {
      PutString("Warning assignment to variable ");
      PutString(name.c_str());
      PutString(" that exceeds allocated size.");
    }
 }

std::string StringVar::eval(const Environment& env)
 {
   return env.strVars[var].getValue();
 }

int64_t NumberVar::eval(const Environment& env)
 {
   return env.intVars[var].getValue();
 }

std::string NormalizeStr(const std::string& source)
 {
   std::string result = source;
   result.erase(0U, result.find_first_not_of(" \t"));
   size_t end = result.find_last_not_of(" \t");
   if (std::string::npos != end)
    {
      result.erase(end + 1U);
   }
   else
    {
      result.clear();
    }
   std::transform(result.begin(), result.end(), result.begin(),
      [](unsigned char c) { return std::toupper(c); });
   return result;
 }

void OpenImpl::execute(Environment& env)
 {
   for (size_t fileNo : fileNos)
    {
      env.files[fileNo]->open(mode);
    }
 }

void WritenImpl::execute(Environment& env)
 {
   env.files[fileNo]->putStr(value->eval(env));
 }

void WriteImpl::execute(Environment& env)
 {
   env.files[fileNo]->putLine(value->eval(env));
 }

void ClsImpl::execute(Environment& env)
 {
   env.files[fileNo]->clearScreen();
 }

void ReadImpl::execute(Environment& env)
 {
   for (size_t varNo : vars)
    {
      std::string next;
      if (env.files[fileNo]->getStr(next))
       {
         // TODO numbers
         env.strVars[varNo].setValue(next);
       }
      else
       {
         // TODO : set STATUS
       }
    }
 }

void StopImpl::execute(Environment&)
 {
   throw StopCode(retCode);
 }

void CloseImpl::execute(Environment& env)
 {
   for (size_t fileNo : fileNos)
    {
      env.files[fileNo]->close();
    }
 }

void IfsImpl::execute(Environment& env)
 {
   bool cond = condition->eval(env);
   if (!cond)
    {
      env.pc = orElse - 1U; // Account for auto increment
    }
 }

void GotoImpl::execute(Environment& env)
 {
   env.pc = target - 1U; // Account for auto increment
 }

void GotoImpl::fixJumps(const std::map<std::string, size_t>& labels, const std::map<std::string, size_t>&)
 {
   if (labels.end() != labels.find(label))
    {
      target = labels.find(label)->second; // Do NOT account for auto increment
    }
   else
    {
      PutString("Branch to undefined label ");
      PutString(label.c_str());
      NewLine();
      throw StopCode(3);
    }
 }
