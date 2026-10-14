#ifndef TSPR3_INTERNAL_TYPES_H
#define TSPR3_INTERNAL_TYPES_H
#include <stddef.h>
#include <stdint.h>
typedef struct { int32_t value; size_t count; } tspr3_frequency;
typedef enum { TSPR3_OK, TSPR3_INVALID, TSPR3_NO_MEMORY } tspr3_status;
#endif
