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
#ifndef NOTTHATCPL_EXCECUTION_H
#define NOTTHATCPL_EXCECUTION_H

#include <ios>
#include <vector>
#include <memory>
#include <map>

enum FORMAT
 {
   NUMBER,
   STRING,
   BLANK
 };

class Environment;

class StringExpr
 {
public:
   virtual std::string eval(const Environment&) = 0;
 };

class StringConst final : public StringExpr
 {
   std::string value;
public:
   explicit StringConst(const std::string& value) : value(value) { }
   virtual std::string eval(const Environment&) override { return value; }
 };

class StringCat final : public StringExpr
{
   std::unique_ptr<StringExpr> lhs, rhs;
public:
   StringCat(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual std::string eval(const Environment& env) override { return lhs->eval(env) + rhs->eval(env); }
};

class StringVar final : public StringExpr
 {
   size_t var;
public:
   explicit StringVar(size_t var) : var(var) { }
   virtual std::string eval(const Environment&) override;
 };

class NumberExpr
 {
public:
   virtual int64_t eval(const Environment&) = 0;
 };

class Plus final : public NumberExpr
{
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Plus(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(const Environment& env) override { return lhs->eval(env) + rhs->eval(env); }
};

class Minus final : public NumberExpr
{
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Minus(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(const Environment& env) override { return lhs->eval(env) - rhs->eval(env); }
};

class Multiply final : public NumberExpr
{
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Multiply(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(const Environment& env) override { return lhs->eval(env) * rhs->eval(env); }
};

class Divide final : public NumberExpr
{
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Divide(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(const Environment& env) override { return lhs->eval(env) / rhs->eval(env); } // TODO : division by 0
};

class Mod final : public NumberExpr
{
   std::unique_ptr<NumberExpr> lhs, rhs;
public:
   Mod(std::unique_ptr<NumberExpr>&& lhs, std::unique_ptr<NumberExpr>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual int64_t eval(const Environment& env) override { return lhs->eval(env) % rhs->eval(env); }
};

class NumberConst final : public NumberExpr
 {
   int64_t value;
public:
   explicit NumberConst(int64_t value) : value(value) { }
   virtual int64_t eval(const Environment&) override { return value; }
 };

class NumberVar final : public NumberExpr
 {
   size_t var;
public:
   explicit NumberVar(size_t var) : var(var) { }
   virtual int64_t eval(const Environment&) override;
 };

template <typename T>
class Predicate
 {
protected:
   std::unique_ptr<T> lhs, rhs;
public:
   Predicate(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) { }
   virtual bool eval(const Environment&) = 0;
 };

template <typename T>
class Equals : public Predicate<T>
 {
public:
   Equals(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : Predicate<T>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return this->lhs->eval(env) == this->rhs->eval(env); }
 };

template <typename T>
class NotEquals : public Predicate<T>
 {
public:
   NotEquals(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : Predicate<T>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return this->lhs->eval(env) != this->rhs->eval(env); }
 };

template <typename T>
class Less : public Predicate<T>
 {
public:
   Less(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : Predicate<T>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return this->lhs->eval(env) < this->rhs->eval(env); }
 };

template <typename T>
class Greater : public Predicate<T>
 {
public:
   Greater(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : Predicate<T>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return this->lhs->eval(env) > this->rhs->eval(env); }
 };

template <typename T>
class LessEqual : public Predicate<T>
 {
public:
   LessEqual(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : Predicate<T>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return this->lhs->eval(env) <= this->rhs->eval(env); }
 };

template <typename T>
class GreaterEqual : public Predicate<T>
 {
public:
   GreaterEqual(std::unique_ptr<T>&& lhs, std::unique_ptr<T>&& rhs) : Predicate<T>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return this->lhs->eval(env) >= this->rhs->eval(env); }
 };

std::string NormalizeStr(const std::string&);

class SortaEquals : public Predicate<StringExpr>
 {
public:
   SortaEquals(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : Predicate<StringExpr>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return NormalizeStr(this->lhs->eval(env)) == NormalizeStr(this->rhs->eval(env)); }
 };

class SortaNotEquals : public Predicate<StringExpr>
 {
public:
   SortaNotEquals(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : Predicate<StringExpr>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return NormalizeStr(this->lhs->eval(env)) != NormalizeStr(this->rhs->eval(env)); }
 };

class SortaLess : public Predicate<StringExpr>
 {
public:
   SortaLess(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : Predicate<StringExpr>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return NormalizeStr(this->lhs->eval(env)) < NormalizeStr(this->rhs->eval(env)); }
 };

class SortaGreater : public Predicate<StringExpr>
 {
public:
   SortaGreater(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : Predicate<StringExpr>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return NormalizeStr(this->lhs->eval(env)) > NormalizeStr(this->rhs->eval(env)); }
 };

class SortaLessEqual : public Predicate<StringExpr>
 {
public:
   SortaLessEqual(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : Predicate<StringExpr>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return NormalizeStr(this->lhs->eval(env)) <= NormalizeStr(this->rhs->eval(env)); }
 };

class SortaGreaterEqual : public Predicate<StringExpr>
 {
public:
   SortaGreaterEqual(std::unique_ptr<StringExpr>&& lhs, std::unique_ptr<StringExpr>&& rhs) : Predicate<StringExpr>(std::move(lhs), std::move(rhs)) { }
   virtual bool eval(const Environment& env) override { return NormalizeStr(this->lhs->eval(env)) >= NormalizeStr(this->rhs->eval(env)); }
 };


class ICode
 {
public:
   size_t lineNo;
   size_t lineStart;
   size_t lineEnd;
   ICode (size_t lineNo, size_t lineStart, size_t lineEnd) : lineNo(lineNo), lineStart(lineStart), lineEnd(lineEnd) { }
   virtual void execute(Environment&) = 0;
   virtual void fixJumps(const std::map<std::string, size_t>& /*labels*/, const std::map<std::string, size_t>& /*subs*/) { };
 };

class OpenImpl final : public ICode
 {
public:
   std::vector<size_t> fileNos;
   std::ios_base::openmode mode;
   OpenImpl (size_t lineNo, size_t lineStart, size_t lineEnd, const std::vector<size_t>& fileNos, std::ios_base::openmode mode) : ICode(lineNo, lineStart, lineEnd), fileNos(fileNos), mode(mode) { }
   virtual void execute(Environment&) override;
 };

class WritenImpl : public ICode
 {
public:
   size_t fileNo;
   std::vector<FORMAT> formats;
   std::unique_ptr<StringExpr> value;
   WritenImpl (size_t lineNo, size_t lineStart, size_t lineEnd, size_t fileNo, const std::vector<FORMAT>& formats, std::unique_ptr<StringExpr>& value) :
      ICode(lineNo, lineStart, lineEnd), fileNo(fileNo), formats(formats), value(value.release()) { }
   virtual void execute(Environment&) override;
 };

class WriteImpl final : public WritenImpl
 {
public:
   WriteImpl (size_t lineNo, size_t lineStart, size_t lineEnd, size_t fileNo, const std::vector<FORMAT>& formats, std::unique_ptr<StringExpr>& value) :
      WritenImpl(lineNo, lineStart, lineEnd, fileNo, formats, value) { }
   virtual void execute(Environment&) override;
 };

class ClsImpl final : public ICode
 {
public:
   size_t fileNo;
   ClsImpl (size_t lineNo, size_t lineStart, size_t lineEnd, size_t fileNo) :
      ICode(lineNo, lineStart, lineEnd), fileNo(fileNo) { }
   virtual void execute(Environment&) override;
 };

class ReadImpl : public ICode
 {
public:
   size_t fileNo;
   std::vector<FORMAT> formats;
   std::vector<size_t> vars;
   ReadImpl (size_t lineNo, size_t lineStart, size_t lineEnd, size_t fileNo, const std::vector<FORMAT>& formats, const std::vector<size_t>& vars) :
      ICode(lineNo, lineStart, lineEnd), fileNo(fileNo), formats(formats), vars(vars) { }
   virtual void execute(Environment&) override;
 };

class StopImpl final : public ICode
 {
public:
   int retCode;
   StopImpl (size_t lineNo, size_t lineStart, size_t lineEnd, int retCode) : ICode(lineNo, lineStart, lineEnd), retCode(retCode) { }
   virtual void execute(Environment&) override;
 };

class CloseImpl final : public ICode
 {
public:
   std::vector<size_t> fileNos;
   CloseImpl (size_t lineNo, size_t lineStart, size_t lineEnd, const std::vector<size_t>& fileNos) : ICode(lineNo, lineStart, lineEnd), fileNos(fileNos) { }
   virtual void execute(Environment&) override;
 };

class IfsImpl final : public ICode
 {
public:
   std::unique_ptr<Predicate<StringExpr> > condition;
   size_t orElse;
   IfsImpl (size_t lineNo, size_t lineStart, size_t lineEnd, std::unique_ptr<Predicate<StringExpr> >& condition, size_t orElse) :
      ICode(lineNo, lineStart, lineEnd), condition(condition.release()), orElse(orElse) { }
   virtual void execute(Environment&) override;
 };

class GotoImpl final : public ICode
 {
public:
   std::string label;
   size_t target;
   GotoImpl (size_t lineNo, size_t lineStart, size_t lineEnd, const std::string& label) : ICode(lineNo, lineStart, lineEnd), label(label), target(0U) { }
   virtual void execute(Environment&) override;
   virtual void fixJumps(const std::map<std::string, size_t>&, const std::map<std::string, size_t>&) override;
 };

#endif /* NOTTHATCPL_EXCECUTION_H */
