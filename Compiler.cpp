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
         while ((i < (line.length() - 1U)) && ((line[i] == ' ') || (line[i] == '\t')))
          {
            line.erase(i, 1U);
          }
         if (line[i] == ';')
          {
            line.resize(i);
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

void Compiler::printListing(const Environment& env)
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
   std::map<std::string, std::string> record;
   CompilerState() : lineNo(0U), charNo(0U), ended(false) { }
 };

void addSystemVariables(Environment&);
void searchForSystem(CompilerState&, const Environment&);
bool getNextLine(const CompilerState&, const Environment&, std::string&);
std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> getNextCommand(const std::string&, size_t);
void checkForLabel(const std::string&, CompilerState&, const Environment&);

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

void searchForSystem(CompilerState& state, const Environment& env)
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
   if ((state.charNo < env.source[state.lineNo].length()) && (env.source[state.lineNo][state.charNo] == '\\'))
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
void SetCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void IfCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void IncrCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void LoopWhileCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void CursCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void AssignmentCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void DefineCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void EndFileCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void CallCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void TableCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void IntTimeCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void DirectCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void RecordCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void GotoXYCommand(const std::string&, const std::string&, CompilerState&, Environment&);
void ReadBCommand(const std::string&, const std::string&, CompilerState&, Environment&);

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

   result.emplace_back(std::make_pair("DIRECT", DirectCommand));

   result.emplace_back(std::make_pair("FILE", FileCommand));

   result.emplace_back(std::make_pair("STRING", StringCommand));
   result.emplace_back(std::make_pair("INTEGER", IntegerCommand));
   result.emplace_back(std::make_pair("TABLE", TableCommand));
   result.emplace_back(std::make_pair("RECORD", RecordCommand));

   result.emplace_back(std::make_pair("SET", SetCommand));
   result.emplace_back(std::make_pair("DEFINE", DefineCommand));

   result.emplace_back(std::make_pair("EXTERNAL", IgnoredCommand));
   result.emplace_back(std::make_pair("FORMAT", FormatCommand));
   // This is why we have a table:
   result.emplace_back(std::make_pair("ENTRYPOINT", IgnoredCommand));
   result.emplace_back(std::make_pair("ENTRY", EntryCommand));

   result.emplace_back(std::make_pair("OPEN", OpenCommand));
   result.emplace_back(std::make_pair("CLOSE", CloseCommand));
   
   // This is why we have a table:
   result.emplace_back(std::make_pair("READB", ReadBCommand));
   result.emplace_back(std::make_pair("READ", ReadCommand));
   // This is why we have a table:
   result.emplace_back(std::make_pair("WRITEN", WriteCommand));
   result.emplace_back(std::make_pair("WRITE", WriteCommand));
   result.emplace_back(std::make_pair("WRITN", WriteCommand));
   result.emplace_back(std::make_pair("REWIND", TODO));

   // This is why we have a table:
   result.emplace_back(std::make_pair("IFSTRING", IfsCommand));
   result.emplace_back(std::make_pair("IFS", IfsCommand));
   result.emplace_back(std::make_pair("IF", IfCommand));
   result.emplace_back(std::make_pair("ELSE", UnimplementedCommand));

   result.emplace_back(std::make_pair("STOP", StopCommand));
   // This is why we have a table:
   result.emplace_back(std::make_pair("ENDLOOP", UnimplementedCommand));
   result.emplace_back(std::make_pair("ENDDO", UnimplementedCommand));
   result.emplace_back(std::make_pair("ENDFILE", EndFileCommand));
   result.emplace_back(std::make_pair("END", EndCommand));

   result.emplace_back(std::make_pair("SUBROUTINE", TODO));
   result.emplace_back(std::make_pair("CALL", CallCommand));
   result.emplace_back(std::make_pair("RETRIEVE", TODO));
   result.emplace_back(std::make_pair("GOTO(", TODO)); // Switch statement
   result.emplace_back(std::make_pair("GOTO", GotoCommand));

   // This is why we have a table:
   result.emplace_back(std::make_pair("LOOPWHILE", LoopWhileCommand));
   result.emplace_back(std::make_pair("LOOP", TODO)); // This is a for loop
   // This is why we have a table:
   result.emplace_back(std::make_pair("RETURNTO", TODO)); // I like to call this GO FUCK YOURSELF
   result.emplace_back(std::make_pair("RETURN", TODO));

   result.emplace_back(std::make_pair("GTIME(INTEGER,", IntTimeCommand));
   result.emplace_back(std::make_pair("GTIME(STRING,", TODO));
   result.emplace_back(std::make_pair("CURSOR", GotoXYCommand));
   result.emplace_back(std::make_pair("CURS", CursCommand));
   result.emplace_back(std::make_pair("CURP", TODO));

   // You would think that INCR would be sufficient, but we want to consume the whole word:
   result.emplace_back(std::make_pair("INCREMENT", IncrCommand));
   result.emplace_back(std::make_pair("INCR", IncrCommand));
   // You would think that DECR would be sufficient, but we want to consume the whole word:
   result.emplace_back(std::make_pair("DECREMENT", IncrCommand));
   result.emplace_back(std::make_pair("DECR", IncrCommand));

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
   return std::make_pair("AssignmentMaybe", AssignmentCommand);
 }

bool isValidLineLabel(const std::string& line, size_t start, size_t end)
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

bool extractLineLabel(const std::string& line, size_t start, char delim, bool orEOL, std::string& label)
 {
   size_t end = line.find(delim, start);
   if ((std::string::npos == end) && (true == orEOL))
    {
      end = line.length();
    }
   if (std::string::npos != end)
    {
      bool good = isValidLineLabel(line, start, end);
      if (true == good)
       {
         label = line.substr(start, end - start);
       }
      return good;
    }
   return false;
 }

bool isValidLabel(const std::string& line, size_t start, size_t end)
 {
   bool good = std::isalpha(line[start]) || (line[start] == '?' ) || (line[start] == '@');
   for (size_t i = start + 1U; (i < end) && (true == good); ++i)
    {
      good = std::isalnum(line[i]) || (line[i] == '?' ) || (line[i] == '@');
    }
   return good;
 }

bool extractLabel(const std::string& line, size_t start, char delim, bool orEOL, std::string& label)
 {
   size_t end = line.find(delim, start);
   if ((std::string::npos == end) && (true == orEOL))
    {
      end = line.find('\\', start);
      if (std::string::npos == end)
       {
         end = line.length();
       }
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

void checkForLabel(const std::string& line, CompilerState& state, const Environment& env)
 {
   // Labels may only occur at the beginning of a line.
   if (0U == state.charNo)
    {
      std::string label;
      if (extractLineLabel(line, 0U, ':', false, label))
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
         else if ("SYS" == number.substr(0U, 3U))
          {
            size_t fileNo = std::stoull(number.substr(3U, 3U));
            state.files[label] = env.files.size();
            if (fileNo < env.fileNames.size())
             {
               env.files.emplace_back(std::make_unique<RealFile>(env.fileNames[fileNo]));
             }
            else
             {
               env.files.emplace_back(std::make_unique<VirtualFile>());
             }
          }
         else
          {
            PutString("Unknown file descriptor ");
            PutString(number.c_str());
            NewLine();
            CompilerFailure(state);
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
   env.intVars.emplace_back(IntVar("STATUS", 0U, 1ULL << 31L, 0U));
   env.symbols.intVars["@REM"] = env.intVars.size();
   env.intVars.emplace_back(IntVar("@REM", 0U, 1ULL << 31L, 0U));
   env.symbols.intVars["?@REM"] = env.intVars.size();
   env.intVars.emplace_back(IntVar("?@REM", 0U, 1ULL << 47L, 0U));

   env.symbols.strVars["EJECT"] = env.strVars.size();
   env.strVars.emplace_back(StrVar("EJECT", 0U, 0U, ""));
   env.symbols.strVars["VTAB"] = env.strVars.size();
   env.strVars.emplace_back(StrVar("VTAB", 0U, 0U, ""));
   env.symbols.strVars["BEEP"] = env.strVars.size();
   env.strVars.emplace_back(StrVar("BEEP", 0U, 0U, ""));
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
            env.strVars.emplace_back(StrVar(label, 0U, std::stoull(size), "")); // Assume size_t == unsigned long long
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
         ConsumeStr(state, label);
         env.symbols.intVars[label] = env.intVars.size();
         env.intVars.emplace_back(IntVar(label, 0U, ('?' == label[0]) ? 1ULL << 47U : 1ULL << 31U, 0U));
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
         ++next;
         switch (line[next])
          {
         case 'C':
            format.push_back(IO_STRING);
            break;
         case 'N':
         case 'D':
            format.push_back(IO_NUMBER);
            break;
         case 'X':
            format.push_back(IO_BLANK);
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
      if (extractLabel(line, state.charNo, '\0', true, fileName) &&
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
      ++state.charNo;
      std::string fileName;
      bool done = false;
      do
       {
         if ((extractLabel(line, state.charNo, ',', true, fileName) || extractLabel(line, state.charNo, ')', true, fileName)) &&
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
         if (',' != line[state.charNo])
          {
            done = true;
          }
         ++state.charNo;
       }
      while (!done);
    }
   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<OpenImpl>(state.lineNo, lineStart, lineEnd, fileNos, mode));
   NextLine(state, env);
 }

bool StringLiteral(const std::string& line, size_t& charNo, std::string& result)
 {
   bool retVal = false;
   if ('\'' == line[charNo])
    {
      ++charNo;
      while (('\'' != line[charNo]) && ('\0' != line[charNo]))
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
         retVal = true;
       }
    }
   return retVal;
 }

std::unique_ptr<StringExpr> Compiler::StringExpression(const std::string& line, size_t& charNo, char delim, bool orEOL, const Environment& env)
 {
   std::unique_ptr<StringExpr> value;
   if ('\'' == line[charNo])
    {
      std::string result;
      if (StringLiteral(line, charNo, result))
       {
         value = std::make_unique<StringConst>(result);
       }
    }
   else
    {
      std::string var;
      if (extractLabel(line, charNo, delim, orEOL, var) && (env.symbols.strVars.end() != env.symbols.strVars.find(var)))
       {
         charNo += var.length();
         value = std::make_unique<StringVar>(env.symbols.strVars.find(var)->second);
       }
    }
   return value;
 }

std::unique_ptr<NumberExpr> Compiler::NumberExpression(const std::string& line, size_t& charNo, char delim, bool orEOL, const Environment& env)
 {
   std::unique_ptr<NumberExpr> value = RealExpression(line, charNo, env);
   if ((line[charNo] != delim) && !(orEOL && ('\0' == line[charNo])))
    {
      return std::unique_ptr<NumberExpr>();
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
      size_t index = 0;
      const std::vector<FORMAT>& formatVec = state.formats[format];
      std::vector<std::unique_ptr<WritableExpr> > vals;
      bool done = false;
      do
       {
         if (IO_STRING == formatVec[index])
          {
            std::unique_ptr<StringExpr> strVal = Compiler::StringExpression(line, state.charNo, ',', true, env);
            if (nullptr != strVal.get())
             {
               vals.emplace_back(std::move(strVal));
             }
            else
             {
               PutString("Bad WRITE STRING value");
               NewLine();
               CompilerFailure(state);
             }
          }
         else if (IO_NUMBER == formatVec[index])
          {
            std::unique_ptr<NumberExpr> numVal = Compiler::NumberExpression(line, state.charNo, ',', true, env);
            if (nullptr != numVal.get())
             {
               vals.emplace_back(std::move(numVal));
             }
            else
             {
               PutString("Bad WRITE INTEGER value");
               NewLine();
               CompilerFailure(state);
             }
          }
         if (',' != line[state.charNo])
          {
            done = true;
          }
         else
          {
            ++state.charNo;
          }
         ++index;
         if (index == formatVec.size())
          {
            index = 0U;
          }
       } while (!done);
      size_t lineEnd = state.charNo;
      if ("WRITE" == cmd)
       {
         env.icode.emplace_back(std::make_unique<WriteImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar], formatVec, std::move(vals)));
       }
      else
       {
         env.icode.emplace_back(std::make_unique<WritenImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar], formatVec, std::move(vals)));
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
   size_t index = 0;
   const std::vector<FORMAT>& formatVec = state.formats[format];
   std::vector<size_t> vars;
   bool done = false;
   do
    {
      std::string var;
      if (extractLabel(line, state.charNo, ',', true, var))
       {
         ConsumeStr(state, var);
         while (formatVec[index] == IO_BLANK)
          {
            ++index;
            if (index == formatVec.size())
             {
               index = 0U;
             }
          }
         if ((IO_STRING == formatVec[index]) && (env.symbols.strVars.end() != env.symbols.strVars.find(var)))
          {
            vars.push_back(env.symbols.strVars[var]);
          }
         else if ((IO_NUMBER == formatVec[index]) && (env.symbols.intVars.end() != env.symbols.intVars.find(var)))
          {
            vars.push_back(env.symbols.intVars[var]);
          }
         else
          {
            PutString("Bad variable in READ");
            NewLine();
            CompilerFailure(state);
          }
         ++index;
         if (index == formatVec.size())
          {
            index = 0U;
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

void DoCommand(CompilerState& state, Environment& env)
 {
   std::string nextLine;
   if (getNextLine(state, env, nextLine))
    {
      std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(nextLine, state.charNo);
      if ("DO" != next.first)
       {
         next.second(nextLine, next.first, state, env);
       }
      else
       {
         NextLine(state, env);
         bool moreCommands = true;
         while (true == moreCommands)
          {
            if (getNextLine(state, env, nextLine))
             {
               next = getNextCommand(nextLine, state.charNo);
               if ("ENDDO" != next.first)
                {
                  next.second(nextLine, next.first, state, env);
                }
               else
                {
                  ConsumeStr(state, next.first);
                  moreCommands = false;
                }
             }
            else
             {
               moreCommands = false;
             }
          }
         NextLine(state, env); // Consume ENDDO
       }
    }
 }

void IfsCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t startLine = state.lineNo;
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true); // Don't check for '('

   std::unique_ptr<StringExpr> lhs = Compiler::StringExpression(line, state.charNo, '.', false, env);
   if (nullptr == lhs.get())
    {
      PutString("Bad IFS condition lhs");
      NewLine();
      CompilerFailure(state);
    }

   std::string predicate = line.substr(state.charNo, 4U);
   if ((0U != predicate.length()) && ('.' != predicate[predicate.length() - 1U]))
    {
      predicate = line.substr(state.charNo, 5U);
    }
   ConsumeStr(state, predicate);

   std::unique_ptr<StringExpr> rhs = Compiler::StringExpression(line, state.charNo, ')', false, env);
   if (nullptr == rhs.get())
    {
      PutString("Bad IFS condition rhs");
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
   DoCommand(state, env);
   size_t orElse = env.icode.size();
   env.icode[instr] = std::make_unique<IfsImpl>(startLine, lineStart, lineEnd, std::move(condition), orElse);

   std::string nextLine;
   if (getNextLine(state, env, nextLine))
    {
      std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(nextLine, state.charNo);
      if ("ELSE" == next.first)
       {
         static_cast<IfImpl*>(env.icode[instr].get())->orElse++; // Jump PAST the ELSE branch
         startLine = state.lineNo;
         lineStart = state.charNo;
         ConsumeStr(state, next.first); // Consume ELSE

         // Unconditional branch to end of ELSE commands
         instr = env.icode.size();
         env.icode.emplace_back(std::unique_ptr<ICode>());
         lineEnd = state.charNo;

         DoCommand(state, env);

         // Unconditional branch to end of ELSE commands
         env.icode[instr] = std::make_unique<ElseImpl>(startLine, lineStart, lineEnd, env.icode.size());
       }
      // Else do nothing : ignore the next command
    }
 }

void GotoCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd);

   std::string label;
   if (extractLineLabel(line, state.charNo, ',', true, label))
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

void SetCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   std::string label;
   ConsumeStr(state, cmd);
   bool moreVars = false;
   do
    {
      if (extractLabel(line, state.charNo, ':', false, label))
       {
         ConsumeStr(state, label, true);
         std::string initial;
         if (IntegerLiteral(line, state.charNo, ',', true, initial))
          {
            ConsumeStr(state, initial);
            env.symbols.intVars[label] = env.intVars.size(); // Assume size_t == unsigned long long
            env.intVars.emplace_back(IntVar(label, 0U, ('?' == label[0]) ? 1ULL << 47U : 1ULL << 31U, std::stoull(initial)));
          }
         else
          {
            PutString("Bad SET value");
            NewLine();
            CompilerFailure(state);
          }
       }
      else
       {
         PutString("Bad SET name");
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

void IfCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t startLine = state.lineNo;
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true); // Don't check for '('

   std::unique_ptr<NumberExpr> lhs = Compiler::NumberExpression(line, state.charNo, '.', false, env);
   if (nullptr == lhs.get())
    {
      PutString("Bad IF condition lhs");
      NewLine();
      CompilerFailure(state);
    }

   std::string predicate = line.substr(state.charNo, 4U);
   ConsumeStr(state, predicate);

   std::unique_ptr<NumberExpr> rhs = Compiler::NumberExpression(line, state.charNo, ')', false, env);
   if (nullptr == rhs.get())
    {
      PutString("Bad IF condition rhs");
      NewLine();
      CompilerFailure(state);
    }
   ++state.charNo; // Consume ')'

   std::unique_ptr<Predicate<NumberExpr> > condition;
   if (".EQ." == predicate)
    {
      condition = std::make_unique<Equals<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".NE." == predicate)
    {
      condition = std::make_unique<NotEquals<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".LE." == predicate)
    {
      condition = std::make_unique<LessEqual<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".GE." == predicate)
    {
      condition = std::make_unique<GreaterEqual<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".GT." == predicate)
    {
      condition = std::make_unique<Greater<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".LT." == predicate)
    {
      condition = std::make_unique<Less<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else
    {
      PutString("Bad relation in IF");
      NewLine();
      CompilerFailure(state);
    }

   size_t instr = env.icode.size();
   env.icode.emplace_back(std::unique_ptr<ICode>());
   size_t lineEnd = state.charNo;
   DoCommand(state, env);
   size_t orElse = env.icode.size();
   env.icode[instr] = std::make_unique<IfImpl>(startLine, lineStart, lineEnd, std::move(condition), orElse);

   std::string nextLine;
   if (getNextLine(state, env, nextLine))
    {
      std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(nextLine, state.charNo);
      if ("ELSE" == next.first)
       {
         static_cast<IfImpl*>(env.icode[instr].get())->orElse++; // Jump PAST the ELSE branch
         startLine = state.lineNo;
         lineStart = state.charNo;
         ConsumeStr(state, next.first); // Consume ELSE

         // Unconditional branch to end of ELSE commands
         instr = env.icode.size();
         env.icode.emplace_back(std::unique_ptr<ICode>());
         lineEnd = state.charNo;

         DoCommand(state, env);

         // Unconditional branch to end of ELSE commands
         env.icode[instr] = std::make_unique<ElseImpl>(startLine, lineStart, lineEnd, env.icode.size());
       }
      // Else do nothing : ignore the next command
    }
 }

void IncrCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   std::string label;
   ConsumeStr(state, cmd);
   if (extractLabel(line, state.charNo, ',', true, label) && (env.symbols.intVars.end() != env.symbols.intVars.find(label)))
    {
      ConsumeStr(state, label);
      int64_t incr = 1;
      if (',' == line[state.charNo])
       {
         ++state.charNo;
         std::string val;
         if (IntegerLiteral(line, state.charNo, ';', true, val))
          {
            ConsumeStr(state, val);
            incr = std::stoull(val);
          }
         else
          {
            PutString("Bad INCR value");
            NewLine();
            CompilerFailure(state);
          }
       }
      size_t lineEnd = state.charNo;
      if ('D' == cmd[0])
       {
         // Then this was actually DECR!
         incr = -incr;
       }
      env.icode.emplace_back(std::make_unique<IncrImpl>(state.lineNo, lineStart, lineEnd, env.symbols.intVars[label], incr));
    }
   else
    {
      PutString("Bad INTEGER name");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void LoopWhileCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t startLine = state.lineNo;
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true); // Don't check for '('

   std::unique_ptr<NumberExpr> lhs = Compiler::NumberExpression(line, state.charNo, '.', false, env);
   if (nullptr == lhs.get())
    {
      PutString("Bad LOOP WHILE condition lhs");
      NewLine();
      CompilerFailure(state);
    }

   std::string predicate = line.substr(state.charNo, 4U);
   ConsumeStr(state, predicate);

   std::unique_ptr<NumberExpr> rhs = Compiler::NumberExpression(line, state.charNo, ')', false, env);
   if (nullptr == rhs.get())
    {
      PutString("Bad LOOP WHILE condition rhs");
      NewLine();
      CompilerFailure(state);
    }
   ++state.charNo; // Consume ')'

   std::unique_ptr<Predicate<NumberExpr> > condition;
   if (".EQ." == predicate)
    {
      condition = std::make_unique<Equals<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".NE." == predicate)
    {
      condition = std::make_unique<NotEquals<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".LE." == predicate)
    {
      condition = std::make_unique<LessEqual<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".GE." == predicate)
    {
      condition = std::make_unique<GreaterEqual<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".GT." == predicate)
    {
      condition = std::make_unique<Greater<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else if (".LT." == predicate)
    {
      condition = std::make_unique<Less<NumberExpr> >(std::move(lhs), std::move(rhs));
    }
   else
    {
      PutString("Bad relation in LOOP WHILE");
      NewLine();
      CompilerFailure(state);
    }

   size_t instr = env.icode.size();
   env.icode.emplace_back(std::unique_ptr<ICode>());
   size_t lineEnd = state.charNo;

   std::string nextLine;
   bool moreCommands = true;
   while (true == moreCommands)
    {
      if (getNextLine(state, env, nextLine))
       {
         std::pair<std::string, void (*)(const std::string&, const std::string&, CompilerState&, Environment&)> next = getNextCommand(nextLine, state.charNo);
         if ("ENDLOOP" != next.first)
          {
            next.second(nextLine, next.first, state, env);
          }
         else
          {
            env.icode.emplace_back(std::make_unique<ElseImpl>(state.lineNo, state.charNo, state.charNo + next.first.length(), instr));
            ConsumeStr(state, next.first);
            moreCommands = false;
          }
       }
      else
       {
         moreCommands = false;
       }
    }
   NextLine(state, env); // Consume ENDLOOP

   size_t orElse = env.icode.size();
   env.icode[instr] = std::make_unique<IfImpl>(startLine, lineStart, lineEnd, std::move(condition), orElse);
 }

void CursCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   std::string label;
   ConsumeStr(state, cmd, true);
   if (extractLabel(line, state.charNo, ',', false, label))
    {
      ConsumeStr(state, label, true);
    }
   else
    {
      PutString("Bad CURS");
      NewLine();
      CompilerFailure(state);
    }
   if (IntegerLiteral(line, state.charNo, ',', false, label) && ("1" == label))
    {
      ConsumeStr(state, label, true);
    }
   else
    {
      PutString("Bad CURS length");
      NewLine();
      CompilerFailure(state);
    }

   std::unique_ptr<StringExpr> str = Compiler::StringExpression(line, state.charNo, ')', false, env);
   if (nullptr == str.get())
    {
      PutString("Bad CURS string");
      NewLine();
      CompilerFailure(state);
    }

   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<CursImpl>(state.lineNo, lineStart, lineEnd, std::move(str)));

   NextLine(state, env);
 }

void StringAssignment(const std::string& line, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   std::string label;
   ++state.charNo; // Assume '
   size_t varNo;
   if (extractLabel(line, state.charNo, '\'', false, label) && (env.symbols.strVars.end() != env.symbols.strVars.find(label)))
    {
      ConsumeStr(state, label, true);
      varNo = env.symbols.strVars[label];
    }
   else
    {
      PutString("Bad string assignment target");
      NewLine();
      CompilerFailure(state);
    }
   if ('=' != line[state.charNo])
    {
      PutString("I thought this was assignment, but I may have been mistaken");
      NewLine();
      CompilerFailure(state);
    }
   ++state.charNo;
   std::unique_ptr<StringExpr> result;
   if ('\'' == line[state.charNo])
    {
      std::string temp;
      ++state.charNo;
      while (('\'' != line[state.charNo]) && ('\0' != line[state.charNo]))
       {
         temp += line[state.charNo++];
       }
      if ('\'' == line[state.charNo])
       {
         temp = Compiler::DeblankStr(temp);
         size_t charNo = 0U;
         bool done = false;
         do
          {
            std::string var;
            if (extractLabel(temp, charNo, '+', true, var) && (env.symbols.strVars.end() != env.symbols.strVars.find(var)))
             {
               charNo += var.length();
               std::unique_ptr<StringExpr> value = std::make_unique<StringVar>(env.symbols.strVars[var]);
               if (nullptr == result.get())
                {
                  result = std::move(value);
                }
               else
                {
                  std::unique_ptr<StringExpr> plus = std::make_unique<StringCat>(std::move(result), std::move(value));
                  result = std::move(plus);
                }
             }
            else
             {
               PutString("Bad string assignment value");
               NewLine();
               CompilerFailure(state);
             }
            if ('+' == temp[charNo])
             {
               ++charNo;
             }
            else
             {
               done = true;
             }
          }
         while (!done);
         ++state.charNo;
       }
      else
       {
         PutString("No terminator");
         NewLine();
         CompilerFailure(state);
       }
    }
   else if ('"' == line[state.charNo])
    {
      std::string temp;
      ++state.charNo;
      while (('"' != line[state.charNo]) && ('\0' != line[state.charNo]))
       {
         temp += line[state.charNo++];
       }
      if ('"' == line[state.charNo])
       {
         temp = Compiler::DeblankStr(temp);
         size_t charNo = 0U;
         bool done = false;
         do
          {
            std::unique_ptr<StringExpr> value = Compiler::StringExpression(temp, charNo, '+', true, env);
            if (nullptr != value)
             {
               if (nullptr == result.get())
                {
                  result = std::move(value);
                }
               else
                {
                  std::unique_ptr<StringExpr> plus = std::make_unique<StringCat>(std::move(result), std::move(value));
                  result = std::move(plus);
                }
             }
            else
             {
               PutString("Bad string assignment value");
               NewLine();
               CompilerFailure(state);
             }
            if ('+' == temp[charNo])
             {
               ++charNo;
             }
            else
             {
               done = true;
             }
          }
         while (!done);
         ++state.charNo;
       }
      else
       {
         PutString("No terminator");
         NewLine();
         CompilerFailure(state);
       }
    }
   else
    {
      PutString("Bad string assignment value");
      NewLine();
      CompilerFailure(state);
    }
   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<StrAssignImpl>(state.lineNo, lineStart, lineEnd, varNo, std::move(result)));
 }

void NumberAssignment(const std::string& line, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   std::unique_ptr<NumberSetter> setter = RealNumberAssignment(line, state.charNo, env);
   if (nullptr == setter.get())
    {
      PutString("Bad assignment target");
      NewLine();
      CompilerFailure(state);
    }
   if ('=' != line[state.charNo])
    {
      PutString("I thought this was assignment, but I may have been mistaken");
      NewLine();
      CompilerFailure(state);
    }
   ++state.charNo;
   std::unique_ptr<NumberExpr> value = Compiler::NumberExpression(line, state.charNo, '\\', true, env);
   if (nullptr != value)
    {
      size_t lineEnd = state.charNo;
      env.icode.emplace_back(std::make_unique<NumAssignImpl>(state.lineNo, lineStart, lineEnd, std::move(setter), std::move(value)));
    }
   else
    {
      PutString("Bad assignment value");
      NewLine();
      CompilerFailure(state);
    }
 }

void AssignmentCommand(const std::string& line, const std::string&, CompilerState& state, Environment& env)
 {
   if ('\'' == line[state.charNo])
    {
      StringAssignment(line, state, env);
    }
   else
    {
      NumberAssignment(line, state, env);
    }
   NextLine(state, env);
 }

void DefineCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   std::string label;
   ConsumeStr(state, cmd);
   bool moreVars = false;
   do
    {
      if (extractLabel(line, state.charNo, ':', false, label))
       {
         ConsumeStr(state, label, true);
         std::string value;
         if (StringLiteral(line, state.charNo, value))
          {
            ConsumeStr(state, value);
            env.symbols.strVars[label] = env.strVars.size();
            env.strVars.emplace_back(StrVar(label, 0U, value.length(), value));
          }
         else
          {
            PutString("Bad DEFINE value");
            NewLine();
            CompilerFailure(state);
          }
       }
      else
       {
         PutString("Bad DEFINE name");
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

void EndFileCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
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
         PutString("Bad file variable in ENDFILE");
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
   env.icode.emplace_back(std::make_unique<EndFileImpl>(state.lineNo, lineStart, lineEnd, fileNos));
   NextLine(state, env);
 }

void CallCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd);
   std::string function;
   if (extractLabel(line, state.charNo, '(', true, function))
    {
      if ("STAT" == function)
       {
         ConsumeStr(state, function);
         size_t lineEnd = state.charNo;
         env.icode.emplace_back(std::make_unique<StatCallImpl>(state.lineNo, lineStart, lineEnd));
       }
      else if ("IOERR" == function)
       {
         ConsumeStr(state, function);
         size_t lineEnd = state.charNo;
         env.icode.emplace_back(std::make_unique<StatCallImpl>(state.lineNo, lineStart, lineEnd));
       }
      else
       {
         //size_t lineEnd = state.charNo;
         PutString("TODO : Generic call");
         NewLine();
       }
    }
   else
    {
      PutString("Bad CALL");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void TableCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
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
            env.symbols.intVars[label] = env.intVars.size();
            env.intVars.emplace_back(IntVar(label, std::stoull(size), ('?' == label[0]) ? 1ULL << 47U : 1ULL << 31U, 0U));
          }
         else
          {
            PutString("Bad TABLE length");
            NewLine();
            CompilerFailure(state);
          }
       }
      else
       {
         PutString("Bad TABLE name");
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

void IntTimeCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   std::string label;
   ConsumeStr(state, cmd);
   if (extractLabel(line, state.charNo, ')', true, label) && (env.symbols.intVars.end() != env.symbols.intVars.find(label)))
    {
      ConsumeStr(state, label, true);
      size_t lineEnd = state.charNo;
      env.icode.emplace_back(std::make_unique<GtimeIntImpl>(state.lineNo, lineStart, lineEnd, env.symbols.intVars[label]));
    }
   else
    {
      PutString("Bad INTEGER name in GTIME");
      NewLine();
      CompilerFailure(state);
    }
   NextLine(state, env);
 }

void DirectCommand(const std::string& line, const std::string&, CompilerState& state, Environment& env)
 {
   if (((state.lineNo + 2U) < env.source.size()) &&
      (line == "DIRECT") && (env.source[state.lineNo + 1U] == "SVC7") && (env.source[state.lineNo + 2U] == "CPL"))
    {
      // I don't know what this does, but I will treat it as a NOP.
      NextLine(state, env);
      state.lineNo += 2;
    }
   else
    {
      throw Unimplemented(line); // DIRECT
    }
 }

void RecordCommand(const std::string& line, const std::string&, CompilerState& state, Environment& env)
 {
   if (((state.lineNo + 2U) < env.source.size()) &&
      (line.substr(0U, 6U) == "RECORD") && (env.source[state.lineNo + 1U].substr(0U, 6U) == "STRING") && (env.source[state.lineNo + 2U] == "ENDREC"))
    {
      std::string recName = line.substr(6U, line.find('(') - 6U);
      std::string dest = env.source[state.lineNo + 1U].substr(6U, env.source[state.lineNo + 1U].find('(') - 6U);
      state.record[recName] = dest;
      NextLine(state, env);
      StringCommand(env.source[state.lineNo], "STRING", state, env);
      NextLine(state, env);
      Raw();
    }
   else
    {
      throw Unimplemented(line); // RECORD
    }
 }

void GotoXYCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   std::string label;
   ConsumeStr(state, cmd, true);
   if (extractLabel(line, state.charNo, ',', false, label))
    {
      ConsumeStr(state, label, true);
    }
   else
    {
      PutString("Bad CURSOR");
      NewLine();
      CompilerFailure(state);
    }

   std::unique_ptr<NumberExpr> y = Compiler::NumberExpression(line, state.charNo, ',', false, env);
   if (nullptr == y.get())
    {
      PutString("Bad CURSOR line");
      NewLine();
      CompilerFailure(state);
    }
   ConsumeStr(state, ",");

   std::unique_ptr<NumberExpr> x = Compiler::NumberExpression(line, state.charNo, ')', false, env);
   if (nullptr == x.get())
    {
      PutString("Bad CURSOR column");
      NewLine();
      CompilerFailure(state);
    }
   ConsumeStr(state, ")");

   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<GotoXYImpl>(state.lineNo, lineStart, lineEnd, std::move(x), std::move(y)));

   NextLine(state, env);
 }

void ReadBCommand(const std::string& line, const std::string& cmd, CompilerState& state, Environment& env)
 {
   size_t lineStart = state.charNo;
   ConsumeStr(state, cmd, true);
   std::string fileVar, label;
   if (extractLabel(line, state.charNo, ',', false, fileVar) && (state.files.end() != state.files.find(fileVar)))
    {
      ConsumeStr(state, fileVar, true);
    }
   else
    {
      PutString("Bad READB");
      NewLine();
      CompilerFailure(state);
    }

   if (extractLabel(line, state.charNo, ')', false, label) && (state.record.end() != state.record.find(label)))
    {
      ConsumeStr(state, label, true);
    }
   else
    {
      PutString("Bad CURSOR");
      NewLine();
      CompilerFailure(state);
    }

   size_t lineEnd = state.charNo;
   env.icode.emplace_back(std::make_unique<ReadBImpl>(state.lineNo, lineStart, lineEnd, state.files[fileVar], env.symbols.strVars[state.record[label]]));

   NextLine(state, env);
 }
