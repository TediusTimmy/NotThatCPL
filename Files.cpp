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

// This file is INCLUDED!

class ScreenFile final : public FileDecl
 {
public:
   virtual void open(std::ios_base::openmode) override
    {
      // Can't open (or close) the screen.
    }

   virtual void close(void) override
    {
      // Can't (open or) close the screen.
    }

   virtual void clearScreen(void) override
    {
      ClearScreen();
    }

   virtual void putStr(const std::string& val) override
    {
      PutString(val.c_str());
    }

   virtual void putLine(const std::string& val) override
    {
      PutString(val.c_str());
      NewLine();
    }

   virtual bool getStr(std::string& result) override
    {
      return GetString(result);
    }

   virtual char getChar(void) override
    {
      return GetChar();
    }

   virtual void rewind(void) override
    {
      // Que?
    }

   virtual void flush(void) override
    {
      Flush();
    }
 };

class RealFile final : public FileDecl
 {
   std::fstream backing;
   std::string name;
public:
   explicit RealFile(const std::string& name) : name(name) { }

   virtual void open(std::ios_base::openmode mode) override
    {
      backing.open(name, mode);
    }

   virtual void close(void) override
    {
      backing.close();
    }

   virtual void clearScreen(void) override
    {
      // What does this even mean?
    }

   virtual void putStr(const std::string& val) override
    {
      backing << val;
    }

   virtual void putLine(const std::string& val) override
    {
      backing << val << std::endl;
    }

   virtual bool getStr(std::string& result) override
    {
      return !!std::getline(backing, result);
    }

   virtual char getChar(void) override
    {
      return backing.get();
    }

   virtual void rewind(void) override
    {
      backing.seekg(0U);
    }

   virtual void flush(void) override
    {
      backing.flush();
    }
 };

class VirtualFile final : public FileDecl
 {
   std::vector<std::string> backing;
   size_t line;
public:
   VirtualFile() : line(0U) { }

   virtual void open(std::ios_base::openmode) override
    {
      // Que?
    }

   virtual void close(void) override
    {
      backing.clear();
    }

   virtual void clearScreen(void) override
    {
      // What does this even mean?
    }

   virtual void putStr(const std::string& val) override
    {
      if (backing.empty())
       {
         backing.emplace_back("");
       }
      backing.back().append(val);
    }

   virtual void putLine(const std::string& val) override
    {
      putStr(val);
      backing.emplace_back("");
    }

   virtual bool getStr(std::string& result) override
    {
      bool retVal = false;
      if (line < backing.size())
       {
         retVal = true;
         result = backing[line++];
       }
      return retVal;
    }

   virtual char getChar(void) override
    {
      throw Unimplemented("virtual file byte read");
    }

   virtual void rewind(void) override
    {
      line = 0U;
    }

   virtual void flush(void) override
    {
      // Not meaningful
    }
 };
