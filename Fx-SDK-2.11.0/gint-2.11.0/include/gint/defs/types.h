//---
//	gint:defs:types - Type definitions
//---

#ifndef GINT_DEFS_TYPES
#define GINT_DEFS_TYPES

#include <gint/defs/attributes.h>

/* For size_t, mainly */
#include <stddef.h>
/* For all fixed-width integer types */
#include <stdint.h>
/* For human-readable boolean types */
#include <stdbool.h>
/* Common system types: ssize_t, off_t, etc. */
#include <sys/types.h>
/* For va_list */
#include <stdarg.h>

/* Fixed-width types for bit fields are quite meaningless */
typedef unsigned int uint;

//---
//  Short types
//---

typedef unsigned int uint;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

//---
//	Structure elements
//----

/* Giving a type to padding bytes is misguiding, let's hide it in a macro */
#define pad_nam2(c)	_ ## c
#define pad_name(c)	pad_nam2(c)
#define pad(bytes)	uint8_t pad_name(__COUNTER__)[bytes]

/* byte_union() - union between an uint8_t 'byte' element and a bit field */
#define byte_union(name, fields)		\
	union {					\
		uint8_t byte;			\
		struct { fields } GPACKED(1);	\
	} GPACKED(1) name

/* word_union() - union between an uint16_t 'word' element and a bit field */
#define word_union(name, fields)		\
	union {					\
		uint16_t word;			\
		struct { fields } GPACKED(2);	\
	} GPACKED(2) name

/* lword_union() - union between an uint32_t 'lword' element and a bit field */
#define lword_union(name, fields)		\
	union {					\
		uint32_t lword;			\
		struct { fields } GPACKED(4);	\
	} GPACKED(4) name

#endif /* GINT_DEFS_TYPES */
