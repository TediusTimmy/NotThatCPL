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
#include <chrono>

void StrVar::setValue(size_t index, const std::string& val)
 {
   if (index < value.size())
    {
      value[index] = val;
      if (val.length() > size)
       {
         PutString("Warning assignment to variable ");
         PutString(name.c_str());
         PutString(" that exceeds allocated size.");
         NewLine();
       }
    }
   else
    {
      PutString("Warning assignment to variable ");
      PutString(name.c_str());
      PutString(" out of bounds.");
      NewLine();
    }
 }

int64_t IntVar::getValue(size_t index) const
 {
   if (index < value.size())
    {
      return value[index];
    }
   else
    {
      PutString("Warning read from variable ");
      PutString(name.c_str());
      PutString(" out of bounds.");
      NewLine();
      return 0;
    }
 }

void IntVar::setValue(size_t index, int64_t val)
 {
   if (index < value.size())
    {
      value[index] = val;
      if ((val > size) || (val < -(size + 1)))
       {
         PutString("Overflow in assignment to variable ");
         PutString(name.c_str());
         NewLine();
       }
    }
   else
    {
      PutString("Warning assignment to variable ");
      PutString(name.c_str());
      PutString(" out of bounds.");
      NewLine();
    }
 }

std::string StringVar::eval(const Environment& env) const
 {
   return env.strVars[var].getValue(0U);
 }

int64_t NumberVar::eval(Environment& env) const
 {
   return env.intVars[var].getValue(0U);
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
   size_t index = 0;
   for (const auto& val : value)
    {
      while (IO_BLANK == formats[index])
       {
         env.files[fileNo]->putStr(" ");
         ++index;
         if (index == formats.size())
          {
            index = 0U;
          }
       }
      env.files[fileNo]->putStr(val->toString(env));
      ++index;
      if (index == formats.size())
       {
         index = 0U;
       }
    }
 }

void WriteImpl::execute(Environment& env)
 {
   WritenImpl::execute(env);
   env.files[fileNo]->putLine();
 }

void ClsImpl::execute(Environment& env)
 {
   env.files[fileNo]->clearScreen();
 }

void ReadImpl::execute(Environment& env)
 {
   size_t index = 0;
   for (size_t varNo : vars)
    {
      while (IO_BLANK == formats[index])
       {
         ++index;
         if (index == formats.size())
          {
            index = 0U;
          }
       }
      std::string next;
      if (env.files[fileNo]->getStr(next))
       {
         if (IO_STRING == formats[index])
          {
            env.strVars[varNo].setValue(0U, next);
          }
         else // IO_NUMBER
          {
            env.intVars[varNo].setValue(0U, std::stoll(next));
          }
       }
      else
       {
         env.intVars[env.symbols.intVars["STATUS"]].setValue(0U, 1);
         return;
       }
      ++index;
      if (index == formats.size())
       {
         index = 0U;
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
   env.intVars[var].setValue(0U, env.intVars[var].getValue(0U) + val);
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
   env.strVars[var].setValue(0U, val->eval(env));
 }

void NumAssignImpl::execute(Environment& env)
 {
   // This dance keeps the debugger from setting STATUS.
   env.numericError = false;
   int64_t newVal = val->eval(env);
   if (true == env.numericError)
    {
      env.intVars[env.symbols.intVars["STATUS"]].setValue(0U, 2);
    }
   else
    {
      var->set(env, newVal);
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
   int64_t stat = env.intVars[env.symbols.intVars["STATUS"]].getValue(0U);
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

void GtimeIntImpl::execute(Environment& env)
 {
   int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 86400000;
   env.intVars[var].setValue(0U, time);
 }
