#ifndef SCENEHELPER_LOCATORPROPERTY_H_
#define SCENEHELPER_LOCATORPROPERTY_H_

#include <Alembic/Util/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/Abc/All.h>

struct Locator
{
   double T[3];
   double S[3];
   
   inline Locator()
   {
      T[0] = T[1] = T[2] = 0.0f;
      S[0] = S[1] = S[2] = 1.0f;
   }
   
   inline Locator(const Locator &rhs)
   {
      operator=(rhs);
   }
   
   inline Locator& operator=(const Locator &rhs)
   {
      T[0] = rhs.T[0]; T[1] = rhs.T[1]; T[2] = rhs.T[2];
      S[0] = rhs.S[0]; S[1] = rhs.S[1]; S[2] = rhs.S[2];
      return *this;
   }
   
   inline operator double* ()
   {
      return &T[0];
   }
   
   inline operator const double* () const
   {
      return &T[0];
   }
   
   inline operator void* ()
   {
      return &T[0];
   }
   
   inline operator const void* () const
   {
      return &T[0];
   }
};

struct LocatorTraits
{
   typedef Locator value_type;
   
   static const Alembic::Util::PlainOldDataType pod_enum = Alembic::Util::kFloat64POD;
   
   static const int extent = 6;
   
   static const char* interpretation()
   {
      return "";
   }
   
   static const char* name()
   {
      return "LocatorTraits";
   }
   
   static Alembic::AbcCoreAbstract::DataType dataType()
   {
      return Alembic::AbcCoreAbstract::DataType(Alembic::Util::kFloat64POD, 6);
   }
   
   static value_type defaultValue()
   {
      value_type v;
      return v;
   }
};

typedef Alembic::Abc::ITypedScalarProperty<LocatorTraits> ILocatorProperty;

#endif
