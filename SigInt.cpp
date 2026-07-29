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
#include <signal.h>

#include "SigInt.h"

static volatile sig_atomic_t signalled = 0;

// The default implementation will be Linux signals / ANSI escapes.
#ifndef NOTTHATCPL_NATIVE_WINDOWS

void handler (int /*signal*/)
 {
   signalled = 1;
 }

void InstallSigintHandler(void)
 {
   struct sigaction sa;
   sa.sa_handler = handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sigaction(SIGINT, &sa, NULL);
 }

#else /* NOTTHATCPL_NATIVE_WINDOWS */

#include <windows.h>

BOOL WINAPI handler(DWORD signal)
 {
   if (signal == CTRL_C_EVENT)
    {
      signalled = 1;
      return TRUE;
    }
   return FALSE;
 }

void InstallSigintHandler(void)
 {
   SetConsoleCtrlHandler(handler, TRUE);
 }

#endif /* ! NOTTHATCPL_NATIVE_WINDOWS */

void CheckForSigInt(void)
 {
   if (signalled)
    {
      signalled = 0;
      throw SigInt();
    }
 }
