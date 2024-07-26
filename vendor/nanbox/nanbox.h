/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 Viktor Söderqvist
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * nanbox.h
 * --------
 *
 * This file provides a is a way to store various types of data in a 64-bit
 * slot, including a type tag, using NaN-boxing.  NaN-boxing is a way to store
 * various information in unused NaN-space in the IEEE754 representation.  For
 * 64-bit platforms, unused bits in pointers are also used to encode various
 * information.  The representation in inspired by that used by Webkit's
 * JavaScriptCore.
 *
 * Datatypes that can be stored:
 *
 *   * int (int32_t)
 *   * double
 *   * pointer
 *   * boolean (true and false)
 *   * null
 *   * undefined
 *   * empty
 *   * deleted
 *   * aux 'auxillary data' (5 types of 48-bit values)
 *
 * Any value with the top 13 bits set represents a quiet NaN.  The remaining
 * bits are called the 'payload'. NaNs produced by hardware and C-library
 * functions typically produce a payload of zero.  We assume that all quiet
 * NaNs with a non-zero payload can be used to encode whatever we want.
 */

#ifndef NANBOX_H
#define NANBOX_H

/*
 * Spall stuff
 */

#ifndef NANBOX_FN
#ifdef VM_USE_SPALL
#define NANBOX_FN static inline __attribute__((no_instrument_function))
#else
#define NANBOX_FN static inline
#endif
#endif

/*
 * Define this before including this file to get functions and type prefixed
 * with something other than "nanbox".
 */
#ifndef NANBOX_PREFIX
#define NANBOX_PREFIX nanbox
#endif

/* User-defined pointer type. Defaults to void*. Must be a pointer type. */
#ifndef NANBOX_POINTER_TYPE
#define NANBOX_POINTER_TYPE void*
#endif

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t, int32_t
#include <stdbool.h> // bool, true, false
#include <string.h>  // memset
#include <assert.h>

/*
 * Macros to expand the prefix.
 */
#undef NANBOX_XXNAME
#define NANBOX_XXNAME(prefix, name) prefix ## name
#undef NANBOX_XNAME
#define NANBOX_XNAME(prefix, name) NANBOX_XXNAME(prefix, name)
#undef NANBOX_NAME
#define NANBOX_NAME(name) NANBOX_XNAME(NANBOX_PREFIX, name)

/*
 * Detect OS and endianess.
 *
 * Most of this is inspired by WTF/wtf/Platform.h in Webkit's source code.
 */

/* Unix? */
#if defined(_AIX) \
    || defined(__APPLE__) /* Darwin */ \
    || defined(__FreeBSD__) || defined(__DragonFly__) \
    || defined(__FreeBSD_kernel__) \
    || defined(__GNU__) /* GNU/Hurd */ \
    || defined(__linux__) \
    || defined(__NetBSD__) \
    || defined(__OpenBSD__) \
    || defined(__QNXNTO__) \
    || defined(sun) || defined(__sun) /* Solaris */ \
    || defined(unix) || defined(__unix) || defined(__unix__)
#define NANBOX_UNIX 1
#endif

/* Windows? */
#if defined(WIN32) || defined(_WIN32)
#define NANBOX_WINDOWS 1
#endif

/* 64-bit mode? (Mostly equivallent to how WebKit does it) */
#if ((defined(__x86_64__) || defined(_M_X64)) \
     && (defined(NANBOX_UNIX) || defined(NANBOX_WINDOWS))) \
    || (defined(__ia64__) && defined(__LP64__)) /* Itanium in LP64 mode */ \
    || defined(__alpha__) /* DEC Alpha */ \
    || (defined(__sparc__) && defined(__arch64__) || defined (__sparcv9)) /* BE */ \
    || defined(__s390x__) /* S390 64-bit (BE) */ \
    || (defined(__ppc64__) || defined(__PPC64__)) \
    || defined(__aarch64__) /* ARM 64-bit */
#define NANBOX_64 1
#else
#define NANBOX_32 1
#endif

/* Big endian? (Mostly equivallent to how WebKit does it) */
#if defined(__MIPSEB__) /* MIPS 32-bit */ \
    || defined(__ppc__) || defined(__PPC__) /* CPU(PPC) - PowerPC 32-bit */ \
    || defined(__powerpc__) || defined(__powerpc) || defined(__POWERPC__) \
    || defined(_M_PPC) || defined(__PPC) \
    || defined(__ppc64__) || defined(__PPC64__) /* PowerPC 64-bit */ \
    || defined(__sparc)   /* Sparc 32bit */  \
    || defined(__sparc__) /* Sparc 64-bit */ \
    || defined(__s390x__) /* S390 64-bit */ \
    || defined(__s390__)  /* S390 32-bit */ \
    || defined(__ARMEB__) /* ARM big endian */ \
    || ((defined(__CC_ARM) || defined(__ARMCC__)) /* ARM RealView compiler */ \
        && defined(__BIG_ENDIAN))
#define NANBOX_BIG_ENDIAN 1
#endif

/*
 * In 32-bit mode, the double is unmasked. In 64-bit mode, the pointer is
 * unmasked.
 */
union NANBOX_NAME(_u) {
	uint64_t as_int64;
	#if defined(NANBOX_64)
	NANBOX_POINTER_TYPE pointer;
	#endif
	double as_double;
	#ifdef NANBOX_BIG_ENDIAN
	struct {
		uint32_t tag;
		uint32_t payload;
	} as_bits;
	#else
	struct {
		uint32_t payload;
		uint32_t tag;
	} as_bits;
	#endif
};

#undef NANBOX_T
#define NANBOX_T NANBOX_NAME(_t)
typedef union NANBOX_NAME(_u) NANBOX_T;

#if defined(NANBOX_64)

/*
 * 64-bit platforms
 *
 * This range of NaN space is represented by 64-bit numbers begining with
 * 13 bits of ones. That is, the first 16 bits are 0xFFF8 or higher.  In
 * practice, no higher value is used for NaNs.  We rely on the fact that no
 * valid double-precision numbers will be "higher" than this (compared as an
 * uint64).
 *
 * By adding 7 * 2^48 as a 64-bit integer addition, we shift the first 16 bits
 * in the doubles from the range 0000..FFF8 to the range 0007..FFFF.  Doubles
 * are decoded by reversing this operation, i.e. substracting the same number.
 *
 * The top 16-bits denote the type of the encoded nanbox_t:
 *
 *     Pointer {  0000:PPPP:PPPP:PPPP
 *             /  0001:xxxx:xxxx:xxxx
 *     Aux.   {           ...
 *             \  0005:xxxx:xxxx:xxxx
 *     Integer {  0006:0000:IIII:IIII
 *              / 0007:****:****:****
 *     Double  {          ...
 *              \ FFFF:****:****:****
 *
 * 32-bit signed integers are marked with the 16-bit tag 0x0006.
 *
 * The tags 0x0001..0x0005 can be used to store five additional types of
 * 48-bit auxillary data, each storing up to 48 bits of payload.
 *
 * The tag 0x0000 denotes a pointer, or another form of tagged immediate.
 * Boolean, 'null', 'undefined' and 'deleted' are represented by specific,
 * invalid pointer values:
 *
 *     False:     0x06
 *     True:      0x07
 *     Undefined: 0x0a
 *     Null:      0x02
 *     Empty:     0x00
 *     Deleted:   0x05
 *
 * All of these except Empty have bit 0 or bit 1 set.
 */

#define NANBOX_VALUE_EMPTY       0x0llu
#define NANBOX_VALUE_DELETED     0x5llu

// Booleans have bits 1 and 2 set. True also has bit 0 set.
#define NANBOX_VALUE_FALSE       0x06llu
#define NANBOX_VALUE_TRUE        0x07llu

// Null and undefined both have bit 1 set. Undefined also has bit 3 set.
#define NANBOX_VALUE_UNDEFINED   0x0Allu
#define NANBOX_VALUE_NULL        0x02llu

// This value is 7 * 2^48, used to encode doubles such that the encoded value
// will begin with a 16-bit pattern within the range 0x0007..0xFFFF.
#define NANBOX_DOUBLE_ENCODE_OFFSET 0x0007000000000000llu
// If the 16 first bits are 0x0002, this indicates an integer number.  Any
// larger value is a double, so we can use >= to check for either integer or
// double.
#define NANBOX_MIN_NUMBER           0x0006000000000000llu
#define NANBOX_HIGH16_TAG           0xffff000000000000llu

// There are 5 * 2^48 auxillary values can be stored in the 64-bit integer
// range NANBOX_MIN_AUX..NANBOX_MAX_AUX.
#define NANBOX_MIN_AUX_TAG          0x0001000000000000llu
#define NANBOX_MAX_AUX_TAG          0x0005ffff00000000llu

#define NANBOX_MIN_AUX1         0x0001000000000000llu
#define NANBOX_MIN_AUX2         0x0002000000000000llu
#define NANBOX_MIN_AUX3         0x0003000000000000llu
#define NANBOX_MIN_AUX4         0x0004000000000000llu
#define NANBOX_MIN_AUX5         0x0005000000000000llu

#define NANBOX_MIN_AUX              0x0001000000000000llu
#define NANBOX_MAX_AUX              0x0005ffffffffffffllu

// NANBOX_MASK_POINTER defines the allowed non-zero bits in a pointer.
#define NANBOX_MASK_POINTER         0x0000fffffffffffcllu

// The 'empty' value is guarranteed to consist of a repeated single byte,
// so that it should be easy to memset an array of nanboxes to 'empty' using
// NANBOX_EMPTY_BYTE as the value for every byte.
#define NANBOX_EMPTY_BYTE           0x0

// Define bool nanbox_is_xxx(NANBOX_T val) and NANBOX_T nanbox_xxx(void)
// with empty, deleted, true, false, undefined and null substituted for xxx.
#define NANBOX_IMMEDIATE_VALUE_FUNCTIONS(NAME, VALUE)                \
	NANBOX_FN NANBOX_T NANBOX_NAME(_##NAME)(void) {        \
		NANBOX_T val;                                        \
		val.as_int64 = VALUE;                                \
		return val;                                          \
	}                                                            \
	NANBOX_FN bool NANBOX_NAME(_is_##NAME)(NANBOX_T val) { \
		return val.as_int64 == VALUE;                        \
	}
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(empty, NANBOX_VALUE_EMPTY)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(deleted, NANBOX_VALUE_DELETED)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(false, NANBOX_VALUE_FALSE)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(true, NANBOX_VALUE_TRUE)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(undefined, NANBOX_VALUE_UNDEFINED)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(null, NANBOX_VALUE_NULL)

NANBOX_FN bool NANBOX_NAME(_is_undefined_or_null)(NANBOX_T val) {
	// Undefined and null are the same if we remove the 'undefined' bit.
	return (val.as_int64 & ~8) == NANBOX_VALUE_NULL;
}

NANBOX_FN bool NANBOX_NAME(_is_boolean)(NANBOX_T val) {
	// True and false are the same if we remove the 'true' bit.
	return (val.as_int64 & ~1) == NANBOX_VALUE_FALSE;
}
NANBOX_FN bool NANBOX_NAME(_to_boolean)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_boolean)(val));
	return val.as_int64 & 1;
}
NANBOX_FN NANBOX_T NANBOX_NAME(_from_boolean)(bool b) {
	NANBOX_T val;
	val.as_int64 = b ? NANBOX_VALUE_TRUE : NANBOX_VALUE_FALSE;
	return val;
}

/* true if val is a double or an int */
NANBOX_FN bool NANBOX_NAME(_is_number)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_NUMBER;
}

NANBOX_FN bool NANBOX_NAME(_is_int)(NANBOX_T val) {
	return (val.as_int64 & NANBOX_HIGH16_TAG) == NANBOX_MIN_NUMBER;
}
NANBOX_FN NANBOX_T NANBOX_NAME(_from_int)(int32_t i) {
	NANBOX_T val;
	val.as_int64 = NANBOX_MIN_NUMBER | (uint32_t)i;
	return val;
}
NANBOX_FN int32_t NANBOX_NAME(_to_int)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_int)(val));
	return (int32_t)val.as_int64;
}

NANBOX_FN bool NANBOX_NAME(_is_double)(NANBOX_T val) {
	return NANBOX_NAME(_is_number)(val) && !NANBOX_NAME(_is_int)(val);
}
NANBOX_FN NANBOX_T NANBOX_NAME(_from_double)(double d) {
	NANBOX_T val;
	val.as_double = d;
	val.as_int64 += NANBOX_DOUBLE_ENCODE_OFFSET;
	assert(NANBOX_NAME(_is_double)(val));
	return val;
}
NANBOX_FN double NANBOX_NAME(_to_double)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_double)(val));
	val.as_int64 -= NANBOX_DOUBLE_ENCODE_OFFSET;
	return val.as_double;
}

NANBOX_FN bool NANBOX_NAME(_is_pointer)(NANBOX_T val) {
    return !(val.as_int64 & ~NANBOX_MASK_POINTER) && val.as_int64;
}
NANBOX_FN NANBOX_POINTER_TYPE NANBOX_NAME(_to_pointer)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_pointer)(val));
	return val.pointer;
}
NANBOX_FN NANBOX_T NANBOX_NAME(_from_pointer)(NANBOX_POINTER_TYPE pointer) {
	NANBOX_T val;
	val.pointer = pointer;
	assert(NANBOX_NAME(_is_pointer)(val));
	return val;
}

NANBOX_FN bool NANBOX_NAME(_is_aux)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_AUX &&
	       val.as_int64 <= NANBOX_MAX_AUX;
}

NANBOX_FN NANBOX_POINTER_TYPE NANBOX_NAME(_to_aux)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_aux)(val));
    return (void *) ((size_t) val.pointer & NANBOX_MASK_POINTER);
}

NANBOX_FN bool NANBOX_NAME(_is_aux1)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_AUX1 &&
	       val.as_int64 < NANBOX_MIN_AUX2;
}
NANBOX_FN bool NANBOX_NAME(_is_aux2)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_AUX2 &&
	       val.as_int64 < NANBOX_MIN_AUX3;
}
NANBOX_FN bool NANBOX_NAME(_is_aux3)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_AUX3 &&
	       val.as_int64 < NANBOX_MIN_AUX4;
}
NANBOX_FN bool NANBOX_NAME(_is_aux4)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_AUX4 &&
	       val.as_int64 < NANBOX_MAX_AUX;
}
NANBOX_FN bool NANBOX_NAME(_is_aux5)(NANBOX_T val) {
	return val.as_int64 >= NANBOX_MIN_AUX5 &&
	       val.as_int64 <= NANBOX_MAX_AUX;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux1)(void *ptr) {
	NANBOX_T ret;
	ret.as_int64 = (uint64_t) ptr | NANBOX_MIN_AUX1;
	assert(NANBOX_NAME(_is_aux1)(ret));
	return ret;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux2)(void *ptr) {
	NANBOX_T ret;
	int printf(const char *, ...);
	ret.as_int64 = (uint64_t) ptr | NANBOX_MIN_AUX2;
	assert(NANBOX_NAME(_is_aux2)(ret));
	return ret;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux3)(void *ptr) {
	NANBOX_T ret;
	ret.as_int64 = (uint64_t) ptr | NANBOX_MIN_AUX3;
	assert(NANBOX_NAME(_is_aux3)(ret));
	return ret;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux4)(void *ptr) {
	NANBOX_T ret;
	ret.as_int64 = (uint64_t) ptr | NANBOX_MIN_AUX4;
	assert(NANBOX_NAME(_is_aux4)(ret));
	return ret;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux5)(void *ptr) {
	NANBOX_T ret;
	ret.as_int64 = (uint64_t) ptr | NANBOX_MIN_AUX5;
	assert(NANBOX_NAME(_is_aux5)(ret));
	return ret;
}

/* end if NANBOX_64 */
#elif defined(NANBOX_32)

/*
 * On 32-bit platforms we use the following NaN-boxing scheme:
 *
 * For values that do not contain a double value, the high 32 bits contain the
 * tag values listed below, which all correspond to NaN-space. When the tag is
 * 'pointer', 'integer' and 'boolean', their values (the 'payload') are store
 * in the lower 32 bits. In the case of all other tags the payload is 0.
 */
#define NANBOX_MAX_DOUBLE_TAG     0XFFF80000
#define NANBOX_INT_TAG            0XFFF80001
#define NANBOX_MIN_AUX_TAG        0XFFF90000
#define NANBOX_MAX_AUX_TAG        0XFFFDFFFF
#define NANBOX_MIN_AUX1_TAG       0XFFF90000
#define NANBOX_MIN_AUX2_TAG       0XFFFA0000
#define NANBOX_MIN_AUX3_TAG       0XFFFB0000
#define NANBOX_MIN_AUX4_TAG       0XFFFC0000
#define NANBOX_MIN_AUX5_TAG       0XFFFD0000
#define NANBOX_POINTER_TAG        0XFFFFFFFA
#define NANBOX_BOOLEAN_TAG        0XFFFFFFFB
#define NANBOX_UNDEFINED_TAG      0XFFFFFFFC
#define NANBOX_NULL_TAG           0XFFFFFFFD
#define NANBOX_DELETED_VALUE_TAG  0XFFFFFFFE
#define NANBOX_EMPTY_VALUE_TAG    0XFFFFFFFF

// The 'empty' value is guarranteed to consist of a repeated single byte,
// so that it should be easy to memset an array of nanboxes to 'empty' using
// NANBOX_EMPTY_BYTE as the value for every byte.
#define NANBOX_EMPTY_BYTE 0xff

/* The minimum uint64_t value for the auxillary range */
#define NANBOX_MIN_AUX            0xfff9000000000000llu
#define NANBOX_MAX_AUX            0xfffdffffffffffffllu

// Define nanbox_xxx and nanbox_is_xxx for deleted, undefined and null.
#define NANBOX_IMMEDIATE_VALUE_FUNCTIONS(NAME, TAG)                   \
	NANBOX_FN NANBOX_T NANBOX_NAME(_##NAME)(void) {       \
		NANBOX_T val;                                         \
		val.as_bits.tag = TAG;                                \
		val.as_bits.payload = 0;                              \
		return val;                                           \
	}                                                             \
	NANBOX_FN bool NANBOX_NAME(_is_##NAME)(NANBOX_T val) {  \
		return val.as_bits.tag == TAG;                        \
	}
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(deleted, NANBOX_DELETED_VALUE_TAG)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(undefined, NANBOX_UNDEFINED_TAG)
NANBOX_IMMEDIATE_VALUE_FUNCTIONS(null, NANBOX_NULL_TAG)

// The undefined and null tags differ only in one bit
NANBOX_FN bool NANBOX_NAME(_is_undefined_or_null)(NANBOX_T val) {
	return (val.as_bits.tag & ~1) == NANBOX_UNDEFINED_TAG;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_empty)(void) {
	NANBOX_T val;
	val.as_int64 = 0xffffffffffffffffllu;
	return val;
}
NANBOX_FN bool NANBOX_NAME(_is_empty)(NANBOX_T val) {
	return val.as_bits.tag == 0xffffffff;
}

/* Returns true if the value is auxillary space */
NANBOX_FN bool NANBOX_NAME(_is_aux)(NANBOX_T val) {
	return val.as_bits.tag >= NANBOX_MIN_AUX_TAG &&
	       val.as_bits.tag < NANBOX_POINTER_TAG;
}

NANBOX_FN void *NANBOX_NAME(_to_aux)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_aux)(val));
	return (void *) (size_t) val.as_bits.payload;
}

NANBOX_FN bool NANBOX_NAME(_is_aux1)(NANBOX_T val) {
	return val.as_bits.tag >= NANBOX_MIN_AUX1_TAG &&
	       val.as_bits.tag < NANBOX_MIN_AUX2_TAG;
}

NANBOX_FN bool NANBOX_NAME(_is_aux2)(NANBOX_T val) {
	return val.as_bits.tag >= NANBOX_MIN_AUX2_TAG &&
	       val.as_bits.tag < NANBOX_MIN_AUX3_TAG;
}

NANBOX_FN bool NANBOX_NAME(_is_aux3)(NANBOX_T val) {
	return val.as_bits.tag >= NANBOX_MIN_AUX3_TAG &&
	       val.as_bits.tag < NANBOX_MIN_AUX4_TAG;
}

NANBOX_FN bool NANBOX_NAME(_is_aux4)(NANBOX_T val) {
	return val.as_bits.tag >= NANBOX_MIN_AUX4_TAG &&
	       val.as_bits.tag < NANBOX_MIN_AUX5_TAG;
}

NANBOX_FN bool NANBOX_NAME(_is_aux5)(NANBOX_T val) {
	return val.as_bits.tag >= NANBOX_MIN_AUX5_TAG &&
	       val.as_bits.tag < NANBOX_POINTER_TAG;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux1)(void *ptr) {
	NANBOX_T ret;
	ret.as_bits.payload = (size_t) ptr;
	ret.as_bits.tag = NANBOX_MIN_AUX1_TAG;
	assert(NANBOX_NAME(_is_aux1(ret)));
	return ret;
}


NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux2)(void *ptr) {
	NANBOX_T ret;
	ret.as_bits.payload = (size_t) ptr;
	ret.as_bits.tag = NANBOX_MIN_AUX2_TAG;
	assert(NANBOX_NAME(_is_aux2(ret)));
	return ret;
}


NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux3)(void *ptr) {
	NANBOX_T ret;
	ret.as_bits.payload = (size_t) ptr;
	ret.as_bits.tag = NANBOX_MIN_AUX3_TAG;
	assert(NANBOX_NAME(_is_aux3(ret)));
	return ret;
}


NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux4)(void *ptr) {
	NANBOX_T ret;
	ret.as_bits.payload = (size_t) ptr;
	ret.as_bits.tag = NANBOX_MIN_AUX4_TAG;
	assert(NANBOX_NAME(_is_aux4(ret)));
	return ret;
}


NANBOX_FN NANBOX_T NANBOX_NAME(_from_aux5)(void *ptr) {
	NANBOX_T ret;
	ret.as_bits.payload = (size_t) ptr;
	ret.as_bits.tag = NANBOX_MIN_AUX5_TAG;
	assert(NANBOX_NAME(_is_aux5(ret)));
	return ret;
}

// Define nanbox_is_yyy, nanbox_to_yyy and nanbox_from_yyy for
// boolean, int, pointer and aux1-aux5
#define NANBOX_TAGGED_VALUE_FUNCTIONS(NAME, TYPE, TAG) \
	NANBOX_FN bool NANBOX_NAME(_is_##NAME)(NANBOX_T val) { \
		return val.as_bits.tag == TAG; \
	} \
	NANBOX_FN TYPE NANBOX_NAME(_to_##NAME)(NANBOX_T val) { \
		assert(val.as_bits.tag == TAG); \
		return (TYPE)val.as_bits.payload; \
	} \
	NANBOX_FN NANBOX_T NANBOX_NAME(_from_##NAME)(TYPE a) { \
		NANBOX_T val; \
		val.as_bits.tag = TAG; \
		val.as_bits.payload = (int32_t)a; \
		return val; \
	}

NANBOX_TAGGED_VALUE_FUNCTIONS(boolean, bool, NANBOX_BOOLEAN_TAG)
NANBOX_TAGGED_VALUE_FUNCTIONS(int, int32_t, NANBOX_INT_TAG)
NANBOX_TAGGED_VALUE_FUNCTIONS(pointer, NANBOX_POINTER_TYPE, NANBOX_POINTER_TAG)

NANBOX_FN NANBOX_T NANBOX_NAME(_true)(void) {
	return NANBOX_NAME(_from_boolean)(true);
}
NANBOX_FN NANBOX_T NANBOX_NAME(_false)(void) {
	return NANBOX_NAME(_from_boolean)(false);
}
NANBOX_FN bool NANBOX_NAME(_is_true)(NANBOX_T val) {
	return val.as_bits.tag == NANBOX_BOOLEAN_TAG && val.as_bits.payload;
}
NANBOX_FN bool NANBOX_NAME(_is_false)(NANBOX_T val) {
	return val.as_bits.tag == NANBOX_BOOLEAN_TAG && !val.as_bits.payload;
}

NANBOX_FN bool NANBOX_NAME(_is_double)(NANBOX_T val) {
	return val.as_bits.tag < NANBOX_INT_TAG;
}
// is number = is double or is int
NANBOX_FN bool NANBOX_NAME(_is_number)(NANBOX_T val) {
	return val.as_bits.tag <= NANBOX_INT_TAG;
}

NANBOX_FN NANBOX_T NANBOX_NAME(_from_double)(double d) {
	NANBOX_T val;
	val.as_double = d;
	assert(NANBOX_NAME(_is_double)(val) &&
	       val.as_bits.tag <= NANBOX_MAX_DOUBLE_TAG);
	return val;
}
NANBOX_FN double NANBOX_NAME(_to_double)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_double)(val));
	return val.as_double;
}

#endif /* elif NANBOX_32 */

/*
 * Representation-independent functions
 */

NANBOX_FN double NANBOX_NAME(_to_number)(NANBOX_T val) {
	assert(NANBOX_NAME(_is_number)(val));
	return NANBOX_NAME(_is_int)(val) ? NANBOX_NAME(_to_int)(val)
	                                 : NANBOX_NAME(_to_double)(val);
}

#endif /* NANBOX_H */