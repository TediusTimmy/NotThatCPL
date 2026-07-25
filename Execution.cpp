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

void IntVar::setValue(int64_t val)
 {
   value = val;
   if ((value > size) || (value < -(size + 1)))
    {
      PutString("Overflow in assignment to variable ");
      PutString(name.c_str());
    }
 }

std::string StringVar::eval(const Environment& env) const
 {
   return env.strVars[var].getValue();
 }

int64_t NumberVar::eval(Environment& env) const
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
   env.files[fileNo]->putStr(value->toString(env));
 }

void WriteImpl::execute(Environment& env)
 {
   env.files[fileNo]->putLine(value->toString(env));
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
         env.intVars[env.symbols.intVars["STATUS"]].setValue(1);
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

void IfImpl::execute(Environment& env)
 {
   bool cond = condition->eval(env);
   if (!cond)
    {
      env.pc = orElse - 1U; // Account for auto increment
    }
 }

void ElseImpl::execute(Environment& env)
 {
   env.pc = gotoDest - 1U; // Account for auto increment
 }

void IncrImpl::execute(Environment& env)
 {
   env.intVars[var].setValue(env.intVars[var].getValue() + val);
 }

void CursImpl::execute(Environment& env)
 {
   std::string temp = str->eval(env);
   temp.resize(1U);
   PutString(temp.c_str());
   Backspace();
 }

void StrAssignImpl::execute(Environment& env)
 {
   env.strVars[var].setValue(val->eval(env));
 }

void NumAssignImpl::execute(Environment& env)
 {
   // This dance keeps the debugger from setting STATUS.
   env.numericError = false;
   env.intVars[var].setValue(val->eval(env));
   if (true == env.numericError)
    {
      env.intVars[env.symbols.intVars["STATUS"]].setValue(2);
    }
 }

void EndFileImpl::execute(Environment& env)
 {
   for (size_t fileNo : fileNos)
    {
      env.files[fileNo]->flush();
    }
 }

void StatCallImpl::execute(Environment& env)
 {
   int64_t stat = env.intVars[env.symbols.intVars["STATUS"]].getValue();
   if (stat != 0)
    {
      PutString("*****   I/O ERROR   ADDRESS=");
      PutString(std::to_string(lineNo).c_str());
      PutString("   STATUS=");
      PutString(std::to_string(stat).c_str());
      PutString("   *****");
      throw StopCode(100);
    }
 }
