#ifndef __GLTRACESIM_UTIL_MKDIR_HH__
#define __GLTRACESIM_UTIL_MKDIR_HH__

#include <sys/stat.h>
#include <cassert>
#include <cstdarg>
#include <sstream>
#include "debug.hh"

namespace gltracesim {

   /**
    *
    */
   inline void mkdir(const char *fmt, ...) {
       va_list args;
       va_start(args, fmt);


       std::stringstream d;
       while (*fmt != '\0') {
           if (strncmp(fmt, "/", 1) == 0) {
               d << *fmt;
               fmt += 1;
           } else if (strncmp(fmt, "%u64", 4) == 0) {
               int32_t i = va_arg(args, int32_t);
               d << i;
               fmt += 4;
               continue;
           } else if (strncmp(fmt, "%s", 2) == 0) {
               const char *c = va_arg(args, char*);
               d << c;
               fmt += 2;
               continue;
           } else if (strncmp(fmt, "%", 1) == 0) {
               DPRINTF(Error, "mkdir %s invalid format: %s", d.str().c_str(), fmt);
               assert(0);
           } else {
               d << *fmt;
               fmt += 1;
               continue;
           }

           //
           int ret = ::mkdir(
                d.str().c_str(),
                S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH
            );

           switch (ret) {
           case -1: continue;
           case 0: continue;
           case EEXIST: continue;
           default: {
               DPRINTF(Error, "mkdir %s failed[%i]: %s", d.str().c_str(), ret, strerror(ret));
               assert(0);
           }
           }
       }

       va_end(args);
   }

} // end namespace gltracesim

#endif // __GLTRACESIM_UTIL_MKDIR_HH__

