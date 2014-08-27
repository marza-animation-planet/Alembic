#ifndef ABCPROC_GLOBALLOCK_H_
#define ABCPROC_GLOBALLOCK_H_

#include <ai.h>

class GlobalLock
{
public:
   
   inline ~GlobalLock()
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
   
   inline GlobalLock()
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
   
   static GlobalLock msInstance;
};

class ScopeLock
{
public:
   
   inline ScopeLock()
   {
      GlobalLock::Acquire();
   }
   
   inline ~ScopeLock()
   {
      GlobalLock::Release();
   }
};


#endif
