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
#include "Compiler.h"
#include "Exceptions.h"
#include "SigInt.h"
#include "Screen.h"
#include "Execution.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <map>

void Compiler::readSource(const char* fileName, Environment& env)
 {
   std::ifstream file(fileName);
   std::string line;

   if (!file)
    {
      PutString("Could not open file ");
      PutString(fileName);
      PutString(" for reading");
      NewLine();
      throw StopCode(2);
    }

   while (std::getline(file, line))
    {
      env.source.push_back(line);
      CheckForSigInt();
    }

   // Remove a trailing blank line.
   if ((env.source.size() > 0U) && (0U == env.source[env.source.size() - 1U].length()))
    {
      env.source.resize(env.source.size() - 1U);
    }
 }

std::string Compiler::DeblankStr(const std::string& source)
 {
   std::string line = source;
   size_t i = 0;
   bool inQuotes = false;
   while (i < line.length())
    {
      if (!inQuotes)
       {
         while ((i < line.length()) && ((line[i] == ' ') || (line[i] == '\t')))
          {
            line.erase(i, 1U);
          }
         if (line[i] == ';')
          {
            line = line.substr(0U, i);
          }
         line[i] = std::toupper(static_cast<unsigned char>(line[i]));
       }
      // This should handle 'internal ''quotes'' correctly' by accident 
      if (line[i] == '\'')
       {
         inQuotes = !inQuotes;
       }
      ++i;
    }
   return line;
 }

// This language exists in that horrible time when spaces were a luxury.
void Compiler::deblank(Environment& env)
 {
   for (auto& line : env.source)
    {
      line = DeblankStr(line);
      CheckForSigInt();
    }
 }

void Compiler::printListing(Environment& env)
 {
   size_t lineNo = 1U;
   for (const auto& line : env.source)
    {
      std::stringstream str;
      str << std::setw(4) << lineNo;
      PutString(str.str().c_str());
      PutString(": ");
      PutString(line.c_str());
      NewLine();
      ++lineNo;
      CheckForSigInt();
    }
 }

class CompilerState final
 {
public:
   size_t lineNo;
   size_t charNo;
   bool ended;
   std::map<std::string, size_t> labels;
   std::map<std::string, size_t> subs;
   std::map<std::string, size_t> files;
   std::map<std::string, std::vector<FORMAT> > formats;
   CompilerState() : lineNo(0U), charNo(0U), ended(false) { }
 };

void addSystemVariables(Environment&);
void searchForSystem(CompilerState&, Environment&);
bool getNextLine(const CompilerState&, const Environment&, std::string&);
std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> getNextCommand(const std::string&, size_t);
void checkForLabel(const std::string&, CompilerState&, Environment&);

void Compiler::compileSource(Environment& env)
 {
   CompilerState state;
   addSystemVariables(env);
   searchForSystem(state, env);
   do
    {
      std::string line;
      if (getNextLine(state, env, line))
       {
         checkForLabel(line, state, env);
         std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(line, state.charNo);
         next.second(line, next.first, state, env);
       }
      else
       {
         PutString("No END command");
         NewLine();
         throw StopCode(3);
       }
      CheckForSigInt();
   } while (!state.ended);
   // Now fix up branches.
   for (auto& instr : env.icode)
    {
      instr->fixJumps(state.labels, state.subs);
    }
 }

[[noreturn]] void CompilerFailure(const CompilerState& state)
 {
   // Bad programmer! Bad!
   PutString("Bad command at line ");
   PutString(std::to_string(state.lineNo + 1U).c_str());
   NewLine();
   throw StopCode(3);
 }

void searchForSystem(CompilerState& state, Environment& env)
 {
   std::string line;
   bool found = false;
   do
    {
      if (getNextLine(state, env, line))
       {
         if (line.substr(0U, 6U) == "SYSTEM")
          {
            // Don't validate anything about the declaration, just look for it.
            // The compiler requires it.
            found = true;
          }
         else if ((line.substr(0U, 5U) == "TITLE") || (line.substr(0U, 5U) == "PRINT") ||
            (line.substr(0U, 4U) == "PAGE") || (line.substr(0U, 5U) == "EJECT") ||
            (line.substr(0U, 5U) == "SPACE") || line.empty())
          {
            // This is permissible, continue.
            // NOTE that COPY and DIRECT WILL cause an error.
          }
         else
          {
            CompilerFailure(state);
          }
       }
      else
       {
         PutString("No SYSTEM command");
         NewLine();
         throw StopCode(3);
       }
      state.lineNo++;
      CheckForSigInt();
   } while (!found);
 }

bool getNextLine(const CompilerState& state, const Environment& env, std::string& line)
 {
   bool result = state.lineNo < env.source.size();
   if (result)
    {
      line = env.source[state.lineNo];
    }
   return result;
 }

void UnimplementedCommand(const std::string& /*line*/, const std::string& cmd, CompilerState& /*state*/, Environment& /*env*/)
 {
   throw Unimplemented(cmd);
 }

void NextLine(CompilerState& state, const Environment& env)
 {
   if ((state.charNo < env.source[state.lineNo].length()) && (env.source[state.lineNo][state.charNo] == '/'))
    {
      state.charNo++;
    }
   else
    {
      state.lineNo++;
      state.charNo = 0U;
    }
 }

void IgnoredCommand(const std::string& /*line*/, const std::string& /*cmd*/, CompilerState& state, Environment& env)
 {
   NextLine(state, env);
   return;
 }

void EndCommand(const std::string& /*line*/, const std::string& /*cmd*/, CompilerState& state, Environment& env)
 {
   NextLine(state, env);
   state.ended = true;
   return;
 }

void FileCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void EntryCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void StringCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void IntegerCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void FormatCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void OpenCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void WriteCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void ReadCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void StopCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void CloseCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void IfsCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void GotoCommand(const std::string&, const std::string&, CompilerState&, Environment&);

// TODO : remove
void TODO(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   PutString("TODO ");
   PutString(cmd.c_str());
   PutString(" ");
   PutString(line.c_str());
   NewLine();
   NextLine(state, env);
   return;
 }

// TODO : remove
void TODO3(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   PutString("TODO ");
   PutString(cmd.c_str());
   PutString(" ");
   PutString(line.c_str());
   NewLine();
   NextLine(state, env);
   state.lineNo += 2;
   return;
 }

// This would be a lot easier if I disallowed line crunching
std::vector<std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> > buildTable(void)
 {
   std::vector<std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> > result;
   result.emplace_back(std::make_pair("TITLE", IgnoredCommand));
   result.emplace_back(std::make_pair("PRINT", IgnoredCommand));
   result.emplace_back(std::make_pair("PAGE", IgnoredCommand));
   result.emplace_back(std::make_pair("EJECT", IgnoredCommand));
   result.emplace_back(std::make_pair("SPACE", IgnoredCommand));
   result.emplace_back(std::make_pair("COPY", UnimplementedCommand));

   result.emplace_back(std::make_pair("DIRECT", TODO3));

   result.emplace_back(std::make_pair("FILE", FileCommand));

   result.emplace_back(std::make_pair("STRING", StringCommand));
   result.emplace_back(std::make_pair("INTEGER", IntegerCommand));
   result.emplace_back(std::make_pair("TABLE", TODO));
   result.emplace_back(std::make_pair("RECORD", TODO3));

   result.emplace_back(std::make_pair("SET", TODO));
   result.emplace_back(std::make_pair("DEFINE", TODO));

   result.emplace_back(std::make_pair("EXTERNAL", TODO));
   result.emplace_back(std::make_pair("FORMAT", FormatCommand));
   // This is why we have a table:
   result.emplace_back(std::make_pair("ENTRYPOINT", IgnoredCommand));
   result.emplace_back(std::make_pair("ENTRY", EntryCommand));

   result.emplace_back(std::make_pair("OPEN", OpenCommand));
   result.emplace_back(std::make_pair("CLOSE", CloseCommand));
   
   // This is why we have a table:
   result.emplace_back(std::make_pair("READB", TODO));
   result.emplace_back(std::make_pair("READ", ReadCommand));
   // This is why we have a table:
   result.emplace_back(std::make_pair("WRITEN", WriteCommand));
   result.emplace_back(std::make_pair("WRITE", WriteCommand));
   result.emplace_back(std::make_pair("WRITN", WriteCommand));
   result.emplace_back(std::make_pair("REWIND", TODO));

   // This is why we have a table:
   result.emplace_back(std::make_pair("IFSTRING", IfsCommand));
   result.emplace_back(std::make_pair("IFS", IfsCommand));
   result.emplace_back(std::make_pair("IF", TODO));
   result.emplace_back(std::make_pair("ELSE", TODO));

   result.emplace_back(std::make_pair("STOP", StopCommand));
   // This is why we have a table:
   result.emplace_back(std::make_pair("ENDLOOP", TODO));
   result.emplace_back(std::make_pair("ENDDO", TODO));
   result.emplace_back(std::make_pair("ENDFILE", TODO));
   result.emplace_back(std::make_pair("END", EndCommand));

   result.emplace_back(std::make_pair("SUBROUTINE", TODO));
   result.emplace_back(std::make_pair("CALL", TODO));
   result.emplace_back(std::make_pair("RETRIEVE", TODO));
   result.emplace_back(std::make_pair("GOTO(", TODO)); // Switch statement
   result.emplace_back(std::make_pair("GOTO", GotoCommand));

   // This is why we have a table:
   result.emplace_back(std::make_pair("LOOPWHILE", TODO));
   result.emplace_back(std::make_pair("LOOP", TODO)); // This is a for loop
   // This is why we have a table:
   result.emplace_back(std::make_pair("RETURNTO", TODO)); // I like to call this GO FUCK YOURSELF
   result.emplace_back(std::make_pair("RETURN", TODO));

   result.emplace_back(std::make_pair("GTIME(INTEGER,", TODO));
   result.emplace_back(std::make_pair("GTIME(STRING,", TODO));
   result.emplace_back(std::make_pair("CURSOR", TODO));
   result.emplace_back(std::make_pair("CURS", TODO));
   result.emplace_back(std::make_pair("CURP", TODO));

   // You would think that INCR would be sufficient, but we want to consume the whole word:
   result.emplace_back(std::make_pair("INCREMENT", TODO));
   result.emplace_back(std::make_pair("INCR", TODO));
   // You would think that DECR would be sufficient, but we want to consume the whole word:
   result.emplace_back(std::make_pair("DECREMENT", TODO));
   result.emplace_back(std::make_pair("DECR", TODO));

   result.emplace_back(std::make_pair("DO", UnimplementedCommand)); // This should never actually be used when returned.
   return result;
 }

std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> getNextCommand(const std::string& line, size_t start)
 {
   static std::vector<std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> > commandCompilers = buildTable();
   if (line.substr(start).empty())
    {
      return std::make_pair("", IgnoredCommand);
    }
   for (const auto& needle : commandCompilers)
    {
      if (line.substr(start, needle.first.length()) == needle.first)
       {
         return needle;
       }
    }
   return std::make_pair("AssignmentMaybe", TODO);
 }

bool isValidLabel(const std::string& line, size_t start, size_t end)
 {
   std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(line, start);
   if ((next.first == "AssignmentMaybe") || (next.first == "DO") || (next.first == "END")) // DONE is a valid label, and ENDTIME
    {
      if ((next.first == "DO") && (2U == end - start))
       {
         return false;
       }
      else if ((next.first == "END") && (3U == end - start))
       {
         return false;
       }

      bool good = std::isalpha(line[start]) || (line[start] == '?' ) || (line[start] == '@');
      for (size_t i = start + 1U; (i < end) && (true == good); ++i)
       {
         good = std::isalnum(line[i]) || (line[i] == '?' ) || (line[i] == '@');
       }
      return good;
    }
   return false;
 }

bool extractLabel(const std::string& line, size_t start, char delim, bool orEOL, std::string& label)
 {
   size_t end = line.find(delim, start);
   if ((std::string::npos == end) && (true == orEOL))
    {
      end = line.length();
    }
   if (std::string::npos != end)
    {
      bool good = isValidLabel(line, start, end);
      if (true == good)
       {
         label = line.substr(start, end - start);
       }
      return good;
    }
   return false;
 }

void checkForLabel(const std::string& line, CompilerState& state, Environment& env)
 {
   // Labels may only occur at the beginning of a line.
   if (0U == state.charNo)
    {
      std::string label;
      if (extractLabel(line, 0U, ':', false, label))
       {
         state.charNo = label.length() + 1U;
         state.labels[label] = env.icode.size();
       }
    }
 }

void ConsumeStr(CompilerState& state, const std::string& cmd, bool separator = false)
 {
   state.charNo += cmd.length() + (separator ? 1U : 0U);
 }

#include "Files.cpp"

void FileCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   std::string label;
   ConsumeStr(state, cmd);
   if (extractLabel(line, state.charNo, ':', false, label))
    {
      ConsumeStr(state, label, true);
      std::string number;
      if (extractLabel(line, state.charNo, ',', false, number))
       {
         if ("SYSIPT" == number)
          {
            state.files[label] = env.files.size();
            env.files.emplace_back(std::make_unique<ScreenFile>());
          }
         else
          {
            // TODO : file numbers
            UnimplementedCommand(line, cmd, state, env);
          }
       }
      else
       {
         PutString("Bad file descriptor SYSXXX");
         NewLine();
         CompilerFailure(state);
       }
    }
   else
    {
      PutString("Bad file name");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void EntryCommand(const std::string& /*line*/, const std::string& /*cmd*/, CompilerState& state, Environment& env)
 {
   if (static_cast<size_t>(-1) == env.entry)
    {
      env.entry = env.icode.size();
    }
   else
    {
      PutString("Multiple ENTRY");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void addSystemVariables(Environment& env)
 {
   env.symbols.intVars["STATUS"] = env.intVars.size();
   env.intVars.emplace_back(IntVar("STATUS", 1ULL << 31L, 0U));
   env.symbols.intVars["@REM"] = env.intVars.size();
   env.intVars.emplace_back(IntVar("@REM", 1ULL << 31L, 0U));
   env.symbols.intVars["?@REM"] = env.intVars.size();
   env.intVars.emplace_back(IntVar("?@REM", 1ULL << 47L, 0U));

   env.symbols.strVars["EJECT"] = env.strVars.size();
   env.strVars.emplace_back(StrVar("EJECT", 0U, ""));
   env.symbols.strVars["VTAB"] = env.strVars.size();
   env.strVars.emplace_back(StrVar("VTAB", 0U, ""));
   env.symbols.strVars["BEEP"] = env.strVars.size();
   env.strVars.emplace_back(StrVar("BEEP", 0U, ""));
 }

bool IntegerLiteral(const std::string& line, size_t start, char delim, bool orEOL, std::string& number)
 {
   number = "";
   while (std::isdigit(line[start]))
    {
      number += line[start++];
    }
   return (number.length() != 0U) && ((line[start] == delim) || (orEOL && ('\0' == line[start])));
 }

void StringCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   std::string label;
   ConsumeStr(state, cmd);
   bool moreVars = false;
   do
    {
      if (extractLabel(line, state.charNo, '(', false, label))
       {
         ConsumeStr(state, label, true);
         std::string size;
         if (IntegerLiteral(line, state.charNo, ')', false, size))
          {
            ConsumeStr(state, size, true);
            env.symbols.strVars[label] = env.strVars.size();
            env.strVars.emplace_back(StrVar(label, std::stoull(size), "")); // Assume size_t == unsigned long long
          }
         else
          {
            PutString("Bad STRING length");
            NewLine();
            CompilerFailure(state);
          }
       }
      else
       {
         PutString("Bad STRING name");
         NewLine();
         CompilerFailure(state);
       }
      moreVars = ',' == line[state.charNo];
      if (moreVars)
       {
         ConsumeStr(state, ",");
       }
   } while (true == moreVars);
   NextLine(state, env);
 }

void IntegerCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   std::string label;
   ConsumeStr(state, cmd);
   bool moreVars = false;
   do
    {
      if (extractLabel(line, state.charNo, ',', true, label))
       {
         ConsumeStr(state, label, false);
         env.symbols.intVars[label] = env.intVars.size();
         env.intVars.emplace_back(IntVar(label, ('?' == label[0]) ? 1ULL << 47U : 1ULL << 31U, 0U));
       }
      else
       {
         PutString("Bad INTEGER name");
         NewLine();
         CompilerFailure(state);
       }
      moreVars = ',' == line[state.charNo];
      if (moreVars)
       {
         ConsumeStr(state, ",");
       }
   } while (true == moreVars);
   NextLine(state, env);
 }

void FormatCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   std::string label;
   ConsumeStr(state, cmd);
   if (extractLabel(line, state.charNo, ':', true, label))
    {
      ConsumeStr(state, label);
      std::vector<FORMAT>& format = state.formats[label];
      size_t next = state.charNo;
      do
       {
         switch (line[next + 1U])
          {
         case 'C':
            format.push_back(STRING);
            break;
         case 'N':
         case 'D':
            format.push_back(NUMBER);
            break;
         case 'X':
            format.push_back(BLANK);
            break;
          }
         next = line.find(',', next);
       } while (next != std::string::npos);
    }
   else
    {
      PutString("Bad FORMAT name");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void OpenCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd);
   std::ios_base::openmode mode;
   std::vector<size_t> fileNos;

   if ("IO" == line.substr(state.charNo, 2U))
    {
      mode = std::ios_base::in | std::ios_base::out;
      ConsumeStr(state, "IO");
    }
   else if ("OUTPUT" == line.substr(state.charNo, 6U))
    {
      mode = std::ios_base::out;
      ConsumeStr(state, "OUTPUT");
    }
   else if ("INPUT" == line.substr(state.charNo, 5U))
    {
      mode = std::ios_base::in;
      ConsumeStr(state, "INPUT");
    }
   else
    {
      PutString("Bad OPEN mode");
      NewLine();
      CompilerFailure(state);
    }

   if ('(' != line[state.charNo])
    {
      std::string fileName;
      if (extractLabel(line, state.charNo, ',', true, fileName) && // There shouldn't be a comma...
            (state.files.end() != state.files.find(fileName)))
       {
         ConsumeStr(state, fileName);
         fileNos.push_back(state.files[fileName]);
       }
      else
       {
         PutString("Bad file variable in OPEN");
         NewLine();
         CompilerFailure(state);
       }
    }
   else
    {
      // TODO : list of files
      UnimplementedCommand(line, cmd, state, env);
    }
   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<OpenImpl>(state.lineNo, lineStart, lineEnd, fileNos, mode));
   NextLine(state, env);
 }

std::unique_ptr<StringExpr> Compiler::StringExpression(const std::string& line, size_t& charNo, char delim, bool orEOL, Environment& env)
 {
   std::unique_ptr<StringExpr> value;
   if ('\'' == line[charNo])
    {
      std::string result;
      ++charNo;
      while ('\'' != line[charNo])
       {
         result += line[charNo++];
         if (('\'' == line[charNo]) && ('\'' == line[charNo + 1U]))
          {
            result += "'";
            charNo += 2;
          }
       }
      if ('\'' == line[charNo])
       {
         charNo++;
         value = std::make_unique<StringConst>(result);
       }
    }
   else
    {
      std::string var;
      if (extractLabel(line, charNo, delim, orEOL, var) && (env.symbols.strVars.end() != env.symbols.strVars.find(var)))
       {
         charNo += var.length();
         value = std::make_unique<StringVar>(env.symbols.strVars[var]);
       }
    }
   return value;
 }

void WriteCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true); // Don't check that it's a '('
   std::string fileVar, format;
   bool success = extractLabel(line, state.charNo, ',', false, fileVar) && (state.files.end() != state.files.find(fileVar));
   if (success) ConsumeStr(state, fileVar, true);
   success &= extractLabel(line, state.charNo, ')', false, format) && (state.formats.end() != state.formats.find(format));
   if (success) ConsumeStr(state, format, true);
   else
    {
      PutString("Error WRITE file or format");
      NewLine();
      CompilerFailure(state);
    }
   if (line.substr(state.charNo, 5U) != "EJECT")
    {
      std::unique_ptr<StringExpr> value = Compiler::StringExpression(line, state.charNo, ',', true, env);
      if (nullptr == value.get())
       {
         PutString("Bad WRITE value");
         NewLine();
         CompilerFailure(state);
       }
      size_t lineEnd = state.charNo;
      if ("WRITE" == cmd)
       {
         env.icode.emplace_back(std::make_unique<WriteImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar], state.formats[format], value));
       }
      else
       {
         env.icode.emplace_back(std::make_unique<WritenImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar], state.formats[format], value));
       }
    }
   else
    {
      ConsumeStr(state, "EJECT");
      size_t lineEnd = state.charNo;
      env.icode.emplace_back(std::make_unique<ClsImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar]));
    }
   NextLine(state, env);
 }

void ReadCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true); // Don't check that it's a '('
   std::string fileVar, format;
   bool success = extractLabel(line, state.charNo, ',', false, fileVar) && (state.files.end() != state.files.find(fileVar));
   if (success) ConsumeStr(state, fileVar, true);
   success &= extractLabel(line, state.charNo, ')', false, format) && (state.formats.end() != state.formats.find(format));
   if (success) ConsumeStr(state, format, true);
   else
    {
      PutString("Error READ file or format");
      NewLine();
      CompilerFailure(state);
    }
   std::vector<size_t> vars;
   bool done = false;
   do
    {
      std::string var;
      if (extractLabel(line, state.charNo, ',', true, var))
       {
         ConsumeStr(state, var);
         // TODO : use format to select variable type
         if (env.symbols.strVars.end() != env.symbols.strVars.find(var))
          {
            vars.push_back(env.symbols.strVars[var]);
          }
         else if (env.symbols.intVars.end() != env.symbols.intVars.find(var))
          {
            vars.push_back(env.symbols.intVars[var]);
          }
         else
          {
            PutString("Bad variable in READ");
            NewLine();
            CompilerFailure(state);
          }
       }
      else
       {
         PutString("READ variable?");
         NewLine();
         CompilerFailure(state);
       }
      done = ',' != line[state.charNo];
      if (!done)
       {
         ++state.charNo;
       }
    } while (!done);
   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<ReadImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar], state.formats[format], vars));
   NextLine(state, env);
 }

void StopCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd);
   std::string retCode;
   if (IntegerLiteral(line, state.charNo, '.', true, retCode))
    {
      ConsumeStr(state, retCode);
      size_t lineEnd = state.charNo;
      env.icode.emplace_back(std::make_unique<StopImpl>(state.lineNo, lineStart, lineEnd, std::stoi(retCode)));
    }
   else
    {
      PutString("STOP code?");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void CloseCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd);

   std::vector<size_t> fileNos;
   bool done = false;
   do
    {
      std::string fileName;
      if (extractLabel(line, state.charNo, ',', true, fileName) &&
            (state.files.end() != state.files.find(fileName)))
       {
         ConsumeStr(state, fileName);
         fileNos.push_back(state.files[fileName]);
       }
      else
       {
         PutString("Bad file variable in CLOSE");
         NewLine();
         CompilerFailure(state);
       }
      done = ',' != line[state.charNo];
      if (!done)
       {
         ++state.charNo;
       }
    } while (!done);
   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<CloseImpl>(state.lineNo, lineStart, lineEnd, fileNos));
   NextLine(state, env);
 }

void IfsCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t startLine = state.lineNo;
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true); // Don't check for '('

   std::unique_ptr<StringExpr> lhs = Compiler::StringExpression(line, state.charNo, '.', false, env);
   if (nullptr == lhs.get())
    {
      PutString("Bad IFS condition");
      NewLine();
      CompilerFailure(state);
    }

   // TODO .EQ. vs .HEQ.
   std::string predicate = line.substr(state.charNo, 4U);
   if ((0U != predicate.length()) && ('.' != predicate[predicate.length() - 1U]))
    {
      predicate = line.substr(state.charNo, 5U);
    }
   ConsumeStr(state, predicate);

   std::unique_ptr<StringExpr> rhs = Compiler::StringExpression(line, state.charNo, ')', false, env);
   if (nullptr == rhs.get())
    {
      PutString("Bad IFS condition");
      NewLine();
      CompilerFailure(state);
    }
   ++state.charNo; // Consume ')'

   std::unique_ptr<Predicate<StringExpr> > condition;
   if (".EQ." == predicate)
    {
      condition = std::make_unique<SortaEquals>(std::move(lhs), std::move(rhs));
    }
   else if (".NE." == predicate)
    {
      condition = std::make_unique<SortaNotEquals>(std::move(lhs), std::move(rhs));
    }
   else if (".LE." == predicate)
    {
      condition = std::make_unique<SortaLessEqual>(std::move(lhs), std::move(rhs));
    }
   else if (".GE." == predicate)
    {
      condition = std::make_unique<SortaGreaterEqual>(std::move(lhs), std::move(rhs));
    }
   else if (".GT." == predicate)
    {
      condition = std::make_unique<SortaGreater>(std::move(lhs), std::move(rhs));
    }
   else if (".LT." == predicate)
    {
      condition = std::make_unique<SortaLess>(std::move(lhs), std::move(rhs));
    }
   else if (".HEQ." == predicate)
    {
      condition = std::make_unique<Equals<StringExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".HNE." == predicate)
    {
      condition = std::make_unique<NotEquals<StringExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".HLE." == predicate)
    {
      condition = std::make_unique<LessEqual<StringExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".HGE." == predicate)
    {
      condition = std::make_unique<GreaterEqual<StringExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".HGT." == predicate)
    {
      condition = std::make_unique<Greater<StringExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".HLT." == predicate)
    {
      condition = std::make_unique<Less<StringExpr> >(std::move(lhs), std::move(rhs));
    }
   else
    {
      PutString("Bad relation in IFS");
      NewLine();
      CompilerFailure(state);
    }

   size_t instr = env.icode.size();
   env.icode.emplace_back(std::unique_ptr<ICode>());
   size_t lineEnd = state.charNo;

   std::string nextLine;
   if (getNextLine(state, env, nextLine))
    {
      std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(nextLine, state.charNo);
      next.second(nextLine, next.first, state, env);
    }
   size_t orElse = env.icode.size();

   env.icode[instr] = std::make_unique<IfsImpl>(startLine, lineStart, lineEnd, condition, orElse);
 }

void GotoCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd);

   std::string label;
   if (extractLabel(line, state.charNo, ',', true, label))
    {
      ConsumeStr(state, label);
    }
   else
    {
      PutString("Bad label");
      NewLine();
      CompilerFailure(state);
    }

   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<GotoImpl>(state.lineNo, lineStart, lineEnd, label));
   NextLine(state, env);
 }
