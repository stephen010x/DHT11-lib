#ifndef PRINT_H
#define PRINT_H

#ifndef DISABLE_PRINT
#define ENABLE_PRINT
#endif

#ifdef ENABLE_PRINT
#include <stdarg.h>
int8_t _print(const char* format, ...);
int8_t printbuff(uint8_t* buff, uint8_t len);
#else 
#ifndef PRINT_DEFINITIONS
#define _print(...) ((uint8_t)0)
#define printbuff(...) ((uint8_t)0)
#endif
#endif

// Creates syntax error for those who (such as myself) 
// from habitually using printf instead of print
#define printf(...) UNDEFINED_PRINTF

#endif
