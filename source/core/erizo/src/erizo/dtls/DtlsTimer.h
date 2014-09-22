#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef DTLS_Timer_h
#define DTLS_Timer_h

namespace dtls
{

class DtlsTimer
{
   public:
      DtlsTimer(unsigned int seq);
      virtual ~DtlsTimer();

      virtual void expired()=0;
      virtual void fire();
      unsigned int getSeq() { return mSeq; }
      //invalid could call though to context and call an external cancel
      void invalidate() { mValid = false; }
   private:
      unsigned int mSeq;
      bool mValid;
};


class DtlsTimerContext
{
   public:
      virtual ~DtlsTimerContext() {}
      virtual void addTimer(DtlsTimer* timer, unsigned int waitMs)=0;
      //could add cancel here
   protected:
      void fire(DtlsTimer *timer);
};

class TestTimerContext: public DtlsTimerContext {
  public:
     TestTimerContext(): DtlsTimerContext() {
      mTimer = 0;
     }
     void addTimer(DtlsTimer *timer, unsigned int seq);
     long long getRemainingTime();
     void updateTimer();

     DtlsTimer *mTimer;
     long long mExpiryTime;
};


}

#endif

/* ====================================================================

 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. None of the contributors names may be used to endorse or promote
    products derived from this software without specific prior written
    permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
