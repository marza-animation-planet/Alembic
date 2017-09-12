#ifndef ABCPROC_GLOBALLOCK_H_
#define ABCPROC_GLOBALLOCK_H_

#include <ai.h>

class AbcProcGlobalLock
{
public:
   
   inline ~AbcProcGlobalLock()
   {
      AiCritSecClose(&mCS);
   }
   
   static inline void Acquire()
   {
      msInstance.acquire();
   }
   
   static inline void Release()
   {
      msInstance.release();
   }

private:
   
   inline AbcProcGlobalLock()
   {
      AiCritSecInit(&mCS);
   }
   
   inline void acquire()
   {
      AiCritSecEnter(&mCS);
   }
   
   inline void release()
   {
      AiCritSecLeave(&mCS);
   }
   
private:
   
   AtCritSec mCS;
   
   static AbcProcGlobalLock msInstance;
};

class AbcProcScopeLock
{
public:
   
   inline AbcProcScopeLock()
   {
      AbcProcGlobalLock::Acquire();
   }
   
   inline ~AbcProcScopeLock()
   {
      AbcProcGlobalLock::Release();
   }
};


#endif
