//---
// gint:defs:call - Indirect calls
//
// The following types and functions can be used to create "indirect function
// calls": function calls that are executed several times or a long time after
// they are created. This is useful for callbacks, where you set up a function
// to be called when an event occurs. The function and its argument are kept in
// the call object until the event occurs, at which point the call is realized.
//---

#ifndef GINT_DEFS_CALL
#define GINT_DEFS_CALL

#ifdef __cplusplus
extern "C" {
#endif

/* gint_call_arg_t: All types of arguments allowed in an indirect call

   Because a function call cannot be easily pieced together, there are
   restrictions on what arguments can be passed. The following union lists all
   of the available types. Other types can be used if casted, mainly pointers;
   see the description of GINT_CALL() for details. */
typedef union {
	/* 32-bit integers */
	int i;
	unsigned int u;
	int32_t i32;
	uint32_t u32;
	/* 32-bit floating-point */
	float f;

	/* Pointers to most common types, in all possible cv-qualifications */
	#define POINTER(type, name) \
	type *name; \
	type const *name ## _c; \
	type volatile *name ## _v; \
	type volatile const *name ## _cv;

	POINTER(void, pv)
	POINTER(char, pc)
	POINTER(unsigned char, puc)
	POINTER(short, ps)
	POINTER(unsigned short, pus)
	POINTER(int, pi)
	POINTER(unsigned int, pui)
	POINTER(int8_t, pi8)
	POINTER(uint8_t, pu8)
	POINTER(int16_t, pi16)
	POINTER(uint16_t, pu16)
	POINTER(int32_t, pi32)
	POINTER(uint32_t, pu32)
	POINTER(int64_t, pi64)
	POINTER(uint64_t, pu64)
	POINTER(long long int, pll)
	POINTER(unsigned long long int, pull)
	POINTER(float, pf)
	POINTER(double, pd)
	#undef POINTER
} gint_call_arg_t;

/* gint_call_t: Indirect call with up to 4 register arguments */
typedef struct {
	void *function;
	gint_call_arg_t args[4];
} gint_call_t;

/* GINT_CALL(): Build an indirect call from function and arguments

   This macro builds an indirect call (of type gint_call_t). Indirect calls are
   used in various APIs (timers, RTC, DMA, USB...) to notify the program of
   events that are caused by the hardware instead of the program.

   The calls are often made asynchronously, which means that the function
   setting up the call finishes first, and then the call is made later while
   some other part of the program is running. This is tricky, because in order
   to perform the call:

   * The code and arguments must still exist, even though the function that
     provided them has returned long ago;
   * The call ABI is lost as soon as we store parameters instead of
     syntactically performing a call in the code.

   For the first issue, the caller has to make sure that every pointer that is
   passed to the call will still be valid when the call is made; in particular,
   pointers to variables on the stack can ony be used if the call is guaranteed
   to be made before the function ends (eg. if there is a synchronous wait in
   the function).

   For the second issue, gint's indirect call mechanism guarantees ABI
   compatibility by restricting the arguments that can be passed to the
   callback.

   * Only arguments that fit into registers can be passed. In practice, this
     mostly excludes 64-bit integers, double floating-point values, and custom
     structures. This way, there is a somewhat solid guarantee that the
     callback function will take arguments in r4...r7.
   * Only up to 4 arguments can be passed.
   * Only values of the types listed in gint_call_arg_t can be passed.

   If you need to work around one of these limitations, pass a pointer to a
   structure containing your arguments (if the call is invoked after the
   current function ends, make the structure static or global).

   If you need to pass a char or a short, cast to an int and have the function
   take an int. If you need to pass a pointer to a type not listed in
   gint_call_arg_t (such as a structure), cast it to (void *); the function can
   still take a pointer to the custom type as argument.

   If the conditions for the arguments to work are not met, the compiler will
   emit on of these two errors:

   * error: static assertion failed: "GINT_CALL: too many arguments (maximum
     4)" -> This is emitted if you have more than 4 arguments.
   * error: cast to union type from type not present in union
     -> This is emitted if you pass a parameter of an invalid type. */
#define GINT_CALL(func, ...) \
	((gint_call_t){ .function = (void *)func, .args = { \
		__VA_OPT__(GINT_CALL_ARGS1(__VA_ARGS__)) \
	}})
#define GINT_CALL_ARGS1(a1, ...) \
	GINT_CALL_ARG(a1), __VA_OPT__(GINT_CALL_ARGS2(__VA_ARGS__))
#define GINT_CALL_ARGS2(a2, ...) \
	GINT_CALL_ARG(a2), __VA_OPT__(GINT_CALL_ARGS3(__VA_ARGS__))
#define GINT_CALL_ARGS3(a3, ...) \
	GINT_CALL_ARG(a3), __VA_OPT__(GINT_CALL_ARGS4(__VA_ARGS__))
#define GINT_CALL_ARGS4(a4, ...) \
	({ __VA_OPT__(_Static_assert(0, \
		"GINT_CALL: too many arguments (maximum 4)");) \
	GINT_CALL_ARG(a4); })

#ifdef __cplusplus
 /* Kind of ugly but I don't have super cool templated logic right now.
    TODO: Allow any 4-byte type using C++ magic. */
 #define GINT_CALL_ARG(expr) ({ \
 	auto __arg = (expr); \
 	(*reinterpret_cast<gint_call_arg_t *>(&__arg)); \
 })
#else
 /* GCC extension: Cast to union */
 #define GINT_CALL_ARG(expr) (gint_call_arg_t)(expr)
#endif

/* GINT_CALL_NULL: Empty function call */
#define GINT_CALL_NULL ((gint_call_t){ .function = NULL, .args = {} })

/* GINT_CALL_FLAG(): Build an indirect call to an odd address

   This is used as a workaround to have an extra flag in gint_call_t structures
   without altering their size and exploits the fact that SuperH code has to be
   aligned on even addresses.
   It is up to the caller to notice the presence of the flag and realign the
   address properly.

   This flag is currently only checked by gint_inth_callback to specify that the
   called function will take the interrupt context as its first argument. */
#define GINT_CALL_FLAG(func, ...) \
	GINT_CALL((void*)((uint32_t)(func) | 1), __VA_ARGS__)

/* gint_call(): Perform an indirect call */
static GINLINE int gint_call(gint_call_t cb)
{
#ifdef __cplusplus
	int (*f)(int r4, int r5, int r6, int r7) = (decltype(f))cb.function;
#else
	int (*f)(int r4, int r5, int r6, int r7) = cb.function;
#endif

        gint_call_arg_t *args = cb.args;
	return f ? f(args[0].i, args[1].i, args[2].i, args[3].i) : -1;
}

//---
// Predefined indirect calls
//
// * GINT_CALL_SET(pointer_to_int) will create an indirect call that sets
//   (*pointer_to_int) to 1 when invoked. Returns 0.
//
// * GINT_CALL_INC(pointer_to_int) will create an indirect call that increments
//   (*pointer_to_int) when invoked. Returns 0.
//
// * GINT_CALL_SET_STOP(pointer_to_int) is like GINT_CALL_SET() but it returns
//   1 (TIMER_STOP) so it can be used to set a pointer just once in a timer.
//
// * GINT_CALL_INC_STOP(pointer_to_int) similarly returns 1 (TIMER_STOP).
//---

/* GINT_CALL_SET(): Callback that sets an integer to 1
   This is defined as a function to make sure the pointer is to an int. */
static int GINT_CALL_SET_function(int volatile *pointer)
{
	(*pointer) = 1;
	return 0;
}
static GINLINE gint_call_t GINT_CALL_SET(int volatile *pointer)
{
	return GINT_CALL(GINT_CALL_SET_function, pointer);
}

/* GINT_CALL_INC(): Callback that increments an integer */
static int GINT_CALL_INC_function(int volatile *pointer)
{
	(*pointer)++;
	return 0;
}
static GINLINE gint_call_t GINT_CALL_INC(int volatile *pointer)
{
	return GINT_CALL(GINT_CALL_INC_function, pointer);
}

/* GINT_CALL_SET_STOP(): Same as GINT_CALL_SET(), but returns TIMER_STOP */
static int GINT_CALL_SET_STOP_function(int volatile *pointer)
{
	(*pointer) = 1;
	return 1;
}
static GINLINE gint_call_t GINT_CALL_SET_STOP(int volatile *pointer)
{
	return GINT_CALL(GINT_CALL_SET_STOP_function, pointer);
}

/* GINT_CALL_INC(): Callback that increments an integer */
static int GINT_CALL_INC_STOP_function(int volatile *pointer)
{
	(*pointer)++;
	return 1;
}
static GINLINE gint_call_t GINT_CALL_INC_STOP(int volatile *pointer)
{
	return GINT_CALL(GINT_CALL_INC_STOP_function, pointer);
}

#ifdef __cplusplus
}
#endif

#endif /* GINT_DEFS_CALL */
