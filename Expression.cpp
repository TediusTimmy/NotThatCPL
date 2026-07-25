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
#include "Environment.h"
#include "Execution.h"
#include "Exceptions.h"

class Plus final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Plus(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override { return lhs->eval(env) + rhs->eval(env); }
 };

class Minus final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Minus(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override { return lhs->eval(env) - rhs->eval(env); }
 };

class Multiply final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Multiply(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override
    {
      int64_t LHS = lhs->eval(env);
      int64_t RHS = rhs->eval(env);
      // This isn't the best catch of numeric overflow, but it will suffice.
      if ((std::abs(LHS) >= 0x1000000) && (std::abs(RHS) >= 0x1000000))
       {
         // STATUS is set to 2 in Assignment, so that the debugger can't influence it.
         env.numericError = true;
       }
      return LHS * RHS;
    }
 };

class Divide final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Divide(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override
    {
      int64_t LHS = lhs->eval(env);
      int64_t RHS = rhs->eval(env);
      if (0 == RHS)
       {
         env.numericError = true;
         return 0;
       }
      return LHS / RHS;
    }
 };

class Mod final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Mod(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override
    {
      int64_t LHS = lhs->eval(env);
      int64_t RHS = rhs->eval(env);
      if (0 == RHS)
       {
         env.numericError = true;
         return LHS;
       }
      return LHS % RHS;
    }
 };

class Negate final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> arg;
public:
   Negate(std::unique_ptr<NumberExpr>&& arg) : arg(std::move(arg)) { }
   virtual int64_t eval(Environment& env) const override { return -arg->eval(env); }
 };

class Len final : public NumberExpr
 {
   size_t strVar;
public:
   Len(size_t strVar) : strVar(strVar) { }
   virtual int64_t eval(Environment& env) const override { return static_cast<int64_t>(env.strVars[strVar].getValue().length()); }
 };

class Abs final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> arg;
public:
   Abs(std::unique_ptr<NumberExpr>&& arg) : arg(std::move(arg)) { }
   virtual int64_t eval(Environment& env) const override { return std::abs(arg->eval(env)); }
 };

class Sgn final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> arg;
public:
   Sgn(std::unique_ptr<NumberExpr>&& arg) : arg(std::move(arg)) { }
   virtual int64_t eval(Environment& env) const override
    {
      int64_t ARG = arg->eval(env);
      return (ARG < 0) ? -1 : ((ARG > 0) ? 1 : 0);
    }
 };

class Round final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Round(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override
    {
      int64_t LHS = lhs->eval(env);
      int64_t RHS = rhs->eval(env);
      int64_t ten = 1;
      if (RHS > 0)
       {
         while (0 != RHS)
          {
            ten *= 10;
            RHS--;
          }
         int64_t quo = LHS / ten;
         int64_t rem = LHS % ten;
         if ((rem * 2) >= ten)
          {
            ++quo;
          }
         return quo;
       }
      return LHS;
    }
 };

class Min final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Min(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override { return std::min(lhs->eval(env), rhs->eval(env)); }
 };

class Max final : public NumberExpr
 {
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Max(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(Environment& env) const override { return std::max(lhs->eval(env), rhs->eval(env)); }
 };

enum TokenType
 {
   TOK_SYMBOL,
   TOK_NUMBER,
   TOK_IDENTIFIER
 };

static std::string nextTolkien;
static TokenType nextLewis;
void getNextTolkien(const std::string line, size_t& charNo); // Updates nextTolkien
std::unique_ptr<NumberExpr> primary(const std::string line, size_t& charNo, const Environment& env);
std::unique_ptr<NumberExpr> term(const std::string line, size_t& charNo, const Environment& env);
std::unique_ptr<NumberExpr> expression(const std::string line, size_t& charNo, const Environment& env);

std::unique_ptr<NumberExpr> RealExpression(const std::string line, size_t& charNo, const Environment& env)
 {
   getNextTolkien(line, charNo);
   std::unique_ptr<NumberExpr> result = expression(line, charNo, env);
   charNo -= nextTolkien.length();
   return result;
 }

#define CHECK(x) \
  if (nullptr == x.get()) \
   { \
     return result; \
   }

   // <term> { ( "+" | "-" ) <term> }
std::unique_ptr<NumberExpr> expression(const std::string line, size_t& charNo, const Environment& env)
 {
   std::unique_ptr<NumberExpr> result = term(line, charNo, env);

   CHECK(result)

   while (("+" == nextTolkien) || ("-" == nextTolkien))
    {
      std::string tolkien = nextTolkien;
      getNextTolkien(line, charNo);

      std::unique_ptr<NumberExpr> rhs = term(line, charNo, env);
      CHECK(rhs)
      if ("+" == tolkien)
       {
         std::unique_ptr<NumberExpr> next = std::make_unique<Plus>(std::move(result), std::move(rhs));
         result = std::move(next);
       }
      else if ("-" == tolkien)
       {
         std::unique_ptr<NumberExpr> next = std::make_unique<Minus>(std::move(result), std::move(rhs));
         result = std::move(next);
       }
    }

   return result;
 }

   // <primary> { ( "*" | "/" ) <primary> }
std::unique_ptr<NumberExpr> term(const std::string line, size_t& charNo, const Environment& env)
 {
   std::unique_ptr<NumberExpr> result = primary(line, charNo, env);

   CHECK(result)

   while (("*" == nextTolkien) || ("/" == nextTolkien))
    {
      std::string tolkien = nextTolkien;
      getNextTolkien(line, charNo);

      std::unique_ptr<NumberExpr> rhs = primary(line, charNo, env);
      CHECK(rhs)
      if ("*" == tolkien)
       {
         std::unique_ptr<NumberExpr> next = std::make_unique<Multiply>(std::move(result), std::move(rhs));
         result = std::move(next);
       }
      else if ("/" == tolkien)
       {
         std::unique_ptr<NumberExpr> next = std::make_unique<Divide>(std::move(result), std::move(rhs));
         result = std::move(next);
       }
    }

   return result;
 }

#define ASSERT_TOKEN(x) \
  if (x != nextTolkien) \
   { \
     return result; \
   } \
  getNextTolkien(line, charNo); \

   // "-" <primary> | <constant> | <variable> | "(" <expression> ")" | <function> "(" <arguments> ")"
std::unique_ptr<NumberExpr> primary(const std::string line, size_t& charNo, const Environment& env)
 {
   std::unique_ptr<NumberExpr> result;

   if (TOK_SYMBOL == nextLewis)
    {
      if ("-" == nextTolkien)
       {
         getNextTolkien(line, charNo);
         std::unique_ptr<NumberExpr> temp = primary(line, charNo, env);
         CHECK(temp)
         result = std::make_unique<Negate>(std::move(temp));
       }
      else if ("(" == nextTolkien)
       {
         getNextTolkien(line, charNo);
         std::unique_ptr<NumberExpr> temp = expression(line, charNo, env);
         CHECK(temp)
         ASSERT_TOKEN(")")
         result = std::move(temp);
       }
    }
   else if (TOK_IDENTIFIER == nextLewis)
    {
      if (("ABS" == nextTolkien) || ("SGN" == nextTolkien)) // 1 numeric argument
       {
         std::string tolkien = nextTolkien;
         getNextTolkien(line, charNo);
         ASSERT_TOKEN("(")
         std::unique_ptr<NumberExpr> temp = expression(line, charNo, env);
         CHECK(temp)
         ASSERT_TOKEN(")")
         if ("ABS" == tolkien)
          {
            result = std::make_unique<Abs>(std::move(temp));
          }
         else if ("SGN" == tolkien)
          {
            result = std::make_unique<Sgn>(std::move(temp));
          }
       }
      else if ("LEN" == nextTolkien) // 1 string argument
       {
         getNextTolkien(line, charNo);
         ASSERT_TOKEN("(")
         if ((TOK_IDENTIFIER != nextLewis) || (env.symbols.strVars.end() == env.symbols.strVars.find(nextTolkien)))
          {
            return result;
          }
         std::unique_ptr<NumberExpr> temp = std::make_unique<Len>(env.symbols.strVars.find(nextTolkien)->second);
         getNextTolkien(line, charNo);
         ASSERT_TOKEN(")")
         result = std::move(temp);
       }
      else if (("MIN" == nextTolkien) || ("MAX" == nextTolkien) || ("MOD" == nextTolkien) || ("ROUND" == nextTolkien)) // 2 arguments
       {
         std::string tolkien = nextTolkien;
         getNextTolkien(line, charNo);
         ASSERT_TOKEN("(")
         std::unique_ptr<NumberExpr> lhs = expression(line, charNo, env);
         CHECK(lhs)
         ASSERT_TOKEN(",")
         std::unique_ptr<NumberExpr> rhs = expression(line, charNo, env);
         CHECK(rhs)
         ASSERT_TOKEN(")")
         if ("MIN" == tolkien)
          {
            result = std::make_unique<Min>(std::move(lhs), std::move(rhs));
          }
         else if ("MAX" == tolkien)
          {
            result = std::make_unique<Max>(std::move(lhs), std::move(rhs));
          }
         else if ("MOD" == tolkien)
          {
            result = std::make_unique<Mod>(std::move(lhs), std::move(rhs));
          }
         else if ("ROUND" == tolkien)
          {
            result = std::make_unique<Round>(std::move(lhs), std::move(rhs));
          }
       }
      else if (env.symbols.intVars.end() != env.symbols.intVars.find(nextTolkien))
       {
         std::string tolkien = nextTolkien;
         getNextTolkien(line, charNo);
         if ("(" == nextTolkien)
          {
            throw Unimplemented("TABLE");
          }
         result = std::make_unique<NumberVar>(env.symbols.intVars.find(tolkien)->second);
       }
    }
   else // NUMBER
    {
      result = std::make_unique<NumberConst>(std::stoull(nextTolkien));
      getNextTolkien(line, charNo);
    }

   return result;
 }

void getNextTolkien(const std::string line, size_t& charNo)
 {
   if (std::isdigit(line[charNo]))
    {
      nextTolkien = "";
      while (std::isdigit(line[charNo]))
       {
         nextTolkien += line[charNo++];
       }
      nextLewis = TOK_NUMBER;
    }
   else if (std::isalpha(line[charNo]) || (line[charNo] == '?' ) || (line[charNo] == '@'))
    {
      nextTolkien = "";
      while (std::isalnum(line[charNo]) || (line[charNo] == '?' ) || (line[charNo] == '@'))
       {
         nextTolkien += line[charNo++];
       }
      nextLewis = TOK_IDENTIFIER;
    }
   else
    {
      nextTolkien = "";
      if ('\0' != line[charNo])
       {
         nextTolkien += line[charNo++];
       }
      nextLewis = TOK_SYMBOL;
    }
 }
