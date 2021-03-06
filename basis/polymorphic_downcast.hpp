#pragma once

#include <base/logging.h>

namespace basis {

// This function is designed to model the behavior of polymorphic_downcast from
// http://www.boost.org/doc/libs/1_56_0/libs/conversion/cast.htm

// Downcasting means casting from a base class to a derived class.
//
// EXAMPLE:
//   ...
//   class Fruit { public: virtual ~Fruit(){}; ... };
//   class Banana : public Fruit { ... };
//   ...
//   void f( Fruit * fruit ) {
//   // ... logic which leads us to believe it is a Banana
//     Banana * banana = polymorphic_downcast<Banana*>(fruit);
//     ...
template <typename Derived, typename Base>
Derived polymorphic_downcast(Base base) {
#if DCHECK_IS_ON()
  DCHECK(dynamic_cast<Derived>(base) == base);
#endif // DCHECK_IS_ON
  return static_cast<Derived>(base);
}

}  // namespace basis
