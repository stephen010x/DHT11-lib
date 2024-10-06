#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#define PRINT_DEFINITIONS
#define DISABLE_PRINT
#include "print.h"

// max of 1024 characters in output string
// doesn't support floats for some reason
int8_t _print(const char * format, ...) {
  if (!Serial)
    return -2; // I don't remember why I chose '-2'
  char buffer[1024];
  va_list args;
  va_start(args, format);
  int retval = vsnprintf(buffer, 1024, format, args);
  if (retval > 0)
    Serial.print(buffer);
  return retval;
}

int8_t printbuff(uint8_t* buff, uint8_t len) {
  for (int i = 0; i < len; i++)
    _print("%02x ", buff[i]);
  return 0;
}
