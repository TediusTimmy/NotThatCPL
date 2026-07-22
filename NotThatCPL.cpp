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
#include "Screen.h"
#include "SigInt.h"
#include "Exceptions.h"
#include "Compiler.h"

#include <iomanip>
#include <sstream>

/*
   Centurion Programming Language (CPL, but Not THAT CPL) interpreter

   Stages:
      Read source file as lines.
      Deblank the lines.
      Compile to interpreter-code.
      Resolve all jumps.
      Run the interpreter-code.

   The interpreter-code is not bytecode.
   It is function objects which perform some command of CPL.
   They are in a big array to support GOTO, RETURN TO, and debugging.

   Debugging/compiling will work off of the deblanked lines, because I didn't
   want to write the code for handling "GO TO".
*/

int main (int argc, char ** argv)
 {
   int returnCode = 0;
   InitScreen();
   InstallSigintHandler();

   try
    {
      if (argc < 2)
       {
         PutString("usage: CPL source.cepl");
         NewLine();
         throw StopCode(1);
       }

      Environment env;

      // First thing: push other arguments.
      for (int i = 2; nullptr != argv[i]; ++i)
       {
         env.fileNames.push_back(argv[i]);
       }

      Compiler::readSource(argv[1], env);
      Compiler::deblank(env);

#ifdef NOTTHATCPL_WITH_DEBUGGING
      Compiler::printListing(env);
#endif /* NOTTHATCPL_WITH_DEBUGGING */

      Compiler::compileSource(env);

#ifdef NOTTHATCPL_WITH_DEBUGGING
      // Print compiled listing, to see if the compiler is buggy
      NewLine();
      for (const auto& icode : env.icode)
       {
         std::stringstream str;
         str << std::setw(4) << (icode->lineNo + 1U);
         PutString(str.str().c_str());
         PutString(": ");
         PutString(env.source[icode->lineNo].substr(0U, icode->lineStart).c_str());
         Emphasis();
         PutString(env.source[icode->lineNo].substr(icode->lineStart, icode->lineEnd).c_str());
         Normal();
         PutString(env.source[icode->lineNo].substr(icode->lineEnd).c_str());
         NewLine();
       }
#endif /* NOTTHATCPL_WITH_DEBUGGING */

      env.pc = env.entry;
      bool step = false;

#ifdef NOTTHATCPL_WITH_DEBUGGING
      bool stop = false;
      std::string last;
      do
       {

         if (env.icode.size() != env.pc)
          {
            std::stringstream str;
            str << std::setw(4) << (env.icode[env.pc]->lineNo + 1U);
            PutString(str.str().c_str());
            PutString(": ");
            PutString(env.source[env.icode[env.pc]->lineNo].substr(0U, env.icode[env.pc]->lineStart).c_str());
            Emphasis();
            PutString(env.source[env.icode[env.pc]->lineNo].substr(env.icode[env.pc]->lineStart, env.icode[env.pc]->lineEnd).c_str());
            Normal();
            PutString(env.source[env.icode[env.pc]->lineNo].substr(env.icode[env.pc]->lineEnd).c_str());
            NewLine();
          }
         else
          {
            PutString("At end of program.");
            NewLine();
          }

         bool understood = false;
         do
          {
            PutString("> ");
            std::string command;
            if (!GetString(command))
             {
               command = "QUIT";
             }
            command = Compiler::DeblankStr(command);
            if (command.empty())
             {
               command = last;
             }
            if ("RUN" == command)
             {
               if ((env.icode.size() == env.pc) && (env.pc != env.entry))
                {
                  PutString("Restarting from beginning.");
                  NewLine();
                  env.pc = env.entry;
                }
               else if (env.pc != env.entry)
                {
                  PutString("Continuing");
                  NewLine();
                }
               step = false;
               understood = true;
               last = command;
             }
            else if ("QUIT" == command)
             {
               env.pc = env.icode.size();
               stop = true;
               understood = true;
               last = command;
             }
            else if (("STEP" == command) || ("NEXT" == command))
             {
               step = true;
               understood = true;
               last = command;
             }
            else if ("HELP" == command)
             {
               PutString("RUN - start, continue, or restart");
               NewLine();
               PutString("STEP or NEXT - take one step");
               NewLine();
               PutString("QUIT - yep");
               NewLine();
               PutString("PRINTS - print a string expression");
               NewLine();
             }
            else if ("PRINTS" == command.substr(0U, 6U))
             {
               size_t loc = 6U;
               std::unique_ptr<StringExpr> expr = Compiler::StringExpression(command, loc, ',', true, env);
               if (nullptr != expr.get())
                {
                  PutString(expr->eval(env).c_str());
                  NewLine();
                }
               else
                {
                  PutString("Didn't understand that.");
                  NewLine();
                }
             }
          } while (false == understood);

         try
          {
#endif /* NOTTHATCPL_WITH_DEBUGGING */

            while (env.pc < env.icode.size())
             {
               env.icode[env.pc]->execute(env);
               ++env.pc;
               if (step) break;
               CheckForSigInt();
             }

#ifdef NOTTHATCPL_WITH_DEBUGGING             
          }
         catch (const SigInt&)
          {
            step = true;
            ClearInputFlags();
          }
       } while (!stop);
#endif /* NOTTHATCPL_WITH_DEBUGGING */
    }
   catch (const SigInt&)
    {
      PutString("^C");
      NewLine();
      returnCode = 130; // Just follow Bash
    }
   catch (const Unimplemented& e)
    {
      PutString("Unimplemented: ");
      PutString(e.what());
      NewLine();
      returnCode = 3;
    }
   catch (const ProgrammerError& e)
    {
      PutString("Bug: ");
      PutString(e.what());
      NewLine();
    }
   catch (const StopCode& e)
    {
      returnCode = e.getCode();
    }

   DestroyScreen();
   return returnCode;
 }
