//---
//	gint:keyboard - Keyboard input
//---

#ifndef GINT_KEYBOARD
#define GINT_KEYBOARD

#ifdef __cplusplus
extern "C" {
#endif

/* Keyboard, key events, keydown() and getkey()

   gint's keyboard driver regularly scans the keyboard matrix and produces *key
   events*. A key event basically says that a key has been pressed or released
   at some point in time. Key events are a faithful description of everything
   that happens on the keyboard.

   These key events are stored in the *event queue*, from where they can be
   retrieved:

   * pollevent() fetches the next keyboard event waiting in the queue. Events
     will always be returned in chronological order. If there is no keyboard
     event pending (ie. if nothing happened since last time events were read),
     this function returns a dummy event of type KEYEV_NONE.
   * waitevent() fetches the next keyboard event waiting in the queue. If the
     queue is empty, it waits until one becomes available or a timeout expires,
     whichever comes first. The timeout can be configured.

   When events have been read, other functions can be read to check whether a
   certain key (or set of keys) is pressed.

   * keydown() checks if the specified key is currently pressed. (*)
   * keydown_all() checks if all keys of a specified list are pressed. The list
     should end with value 0.
   * keydown_any() checks if any key of a specified list is pressed. The list
     should end with value 0.

   (*) The keydown() functions do not report the absolute current state of the
       key, but rather the state of the key according to the events that have
       been retrieved so far. If the queue contains {KEY_A, KEYEV_DOWN} and
       {KEY_A, KEYEV_UP}, the following sequence of events will happen:

         keydown(KEY_A)   -> 0
         pollevent()      -> {KEY_A, KEYEV_DOWN}
         keydown(KEY_A)   -> 1
         pollevent()      -> {KEY_A, KEYEV_UP}
         keydown(KEY_A)   -> 0

       Synchronizing keydown() and the events increases the accuracy of
       keyboard information for fast programs and its reliability for slow
       programs.

       When all events have been read from the queue, keydown() returns the
       absolute current key state, which is what will happen 90% of the time.

   Applications that are not interested in event contents but only in pressed
   keys can automatically read all events from the queue before they start
   using keydown():

   * clearevents() reads all pending events from the input queue.

   Games are often also interested in whether keys were pressed or released
   _since the last frame_. gint doesn't know what a frame is, but you can track
   key state flips by using cleareventflips() before reading events and
   keypressed() and keyreleased() afterwards.

   * keypressed() checks if the specified key is currently down and was up at
     the time of the last cleareventflips().
   * keyreleased() checks if the specified key is currently up and was down at
     the time of the last cleareventflips().

   A typical game loop might look like. cleareventflips() must be called
   _before_ clearevents().

     cleareventflips();
     clearevents(); // or pollevent(), waitevent()
     if(keydown(KEY_RIGHT)) move();
     if(keypressed(KEY_F6)) open_inventory();

   The previous functions are quite low-level. GUI programs that look like the
   system applications will prefer using a GetKey()-like functions that return
   a single key press at a time, heeds for releases, for SHIFT and ALPHA
   modifiers, handles repeats, backlight and return-to-menu.

   * getkey_opt() is gint's enhanced GetKey()-like function, with support for
     custom repetition and many fine-tunable options.
   * getkey() is a specific call to getkey_opt(), that imitates GetKey().

   These functions introduce a new type of key event called KEYEV_HOLD which
   represents a key repeat. */

#include <gint/defs/types.h>
#include <gint/keycodes.h>

/* key_event_t: Low-level or high-level keyboard event

   This structure represents an event that occurs on the keyboard. It is first
   produced by the keyboard scanner with limited information, then possibly
   enriched by getkey(). Events are produced each time the keyboard is scanned,
   which is 128 Hz by default. Hence, a key press and release occuring in less
   than 8 ms might not be detected.

   getkey() returns enriched events with [mod=1], in whic ase [shift] and
   [alpha] indicate whether the key has been modified. Only key press events
   returned by getkey() have [mod=1]. Note that you can't have, e.g.
   [key=KEY_SHIFT] and [mod=1] at the same time.

   The [time] attribute indicates when the event occurred. It is a snapshot of
   a time counter that increases at each keyboard scan and *wraps around every
   8 minutes* (at 128 Hz). I expect this attribute to be useful to analyze
   combo sequences in games. Make sure you are aware of the two nitpicks:
   * Don't keep the time values for too long because of the wrap-around effect.
   * 0xffff is "just before" 0x0000, not "long after". */
typedef struct
{
	uint time	:16;	/* Time of event, unique over short periods */

	uint		:2;	/* Reserved for future use */

	uint mod	:1;	/* Whether modifiers are used */
	uint shift	:1;	/* If mod=1, whether SHIFT was pressed */
	uint alpha	:1;	/* If mod=1, whether ALPHA was pressed */

	uint type	:3;	/* Type of key event */
	uint key	:8;	/* Hit key */

} GPACKED(4) key_event_t;

/* Keyboard event types, as in the [type] field of key_event_t */
enum
{
	KEYEV_NONE   = 0,	/* No event available (poll() only) */
	KEYEV_DOWN   = 1,	/* Key was pressed */
	KEYEV_UP     = 2,	/* Key was released */
	KEYEV_HOLD   = 3,	/* A key that was pressed has been held down */
	KEYEV_OSMENU = 4,	/* We went to the main menu and back */
};

/* Keyboard frequency analysis is a runtime setting since gint 2.4. This macro
   is preserved for compatibility until gint 3. */
#define KEYBOARD_SCAN_FREQUENCY keysc_scan_frequency()

//---
//  Scan frequency settings
//---

/* keysc_scan_frequency(): Get the current keyboard scan frequency in Hertz */
int keysc_scan_frequency(void);

/* keysc_scan_frequency_us(): Get keyboard scan delay in microseconds */
uint32_t keysc_scan_frequency_us(void);

/* keysc_set_scan_frequency(): Set the keyboard scan frequency in Hertz

   The new frequency must be at least 64 for the keyboard to work reliably, and
   at most 32768 for the underlying ETMU to support it. Out-of-range values are
   forced to the closest valid value.

   @freq  New scan frequency, in Hertz */
void keysc_set_scan_frequency(int freq);

//---
//	Event-level functions
//---

/* pollevent(): Poll the next keyboard event
   This function returns the next event from the event queue, chronologically.
   If no event is available, it returns a dummy event with type=KEYEV_NONE
   and time set to the current driver time. This function always returns events
   with mod=0. */
key_event_t pollevent(void);

/* waitevent(): Wait for the next keyboard event
   This function works as pollevent() but waits if no event is available. When
   timeout=NULL, it waits indefinitely. Otherwise, it waits until *timeout
   becomes non-zero. It is particularly suitable to set *timeout to 1 using a
   timer with [timer_timeout] as callback. See <gint/timer.h>. */
key_event_t waitevent(volatile int *timeout);

/* clearevents(): Read all events waiting in the queue */
void clearevents(void);

/* cleareventflips(): Set the time reference for keypressed()/keyreleased()
   The two functions keypressed() and keyreleased() will use the keyboard state
   at the time this function was called to determine whether any given key was
   just pressed or jut released. */
void cleareventflips(void);

//---
//	Key state functions
//---

/* keydown(): Current key state
   This function returns zero if the specified key is currently up (according
   to the last events that have been processed) and non-zero if it is down. */
int keydown(int key);

/* keydown_all(): Check a set of keys for simultaneous input
   Returns non-zero if all provided keys are down. The list should end with a 0
   as terminator. */
int keydown_all(int key1, ...);

/* keydown_any(): Check a set of keys for any input
   Returns nonzero if any one of the specified keys is currently pressed. The
   sequence should be terminated by a 0 integer. */
int keydown_any(int key1, ...);

/* keypressed(): Check if a key was just pressed
   This function returns non-zero if the specified key is currently down, *and*
   it was up at the time of the last call to cleareventflips(). */
int keypressed(int key);

/* keyreleased(): Check if a key was just released
   This function returns non-zero if the specified key is currently up, *and*
   it was down at the time of the last call to cleareventflips(). */
int keyreleased(int key);

//---
//	High-level functions
//---

/* getkey(): Wait for a key press

   This function mimics the behavior of the fxlib GetKey(). It returns a
   key_event_t object where [mod=1], and where [shift] and [alpha] indicate
   whether SHIFT or ALPHA was pressed before the key was hit. [event] is
   KEYEV_DOWN when a new key is pressed and KEYEV_HOLD in case of repeats.

   Similarities with GetKey() include:
   - Wait for a key to be pressed *after* the call (held keys don't count)
   - Supports SHIFT and ALPHA modifiers
   - Repeats arrows keys
   - Allows return to main menu if the MENU key is pressed
   - Controls backlight on models that have a back-lit screen

   getkey() is equivalent to getkey_opt(GETKEY_DEFAULT, NULL). */
key_event_t getkey(void);

/* The following are the option bits for getkey_opt(). */
enum {
	/* Enable modifiers keys */
	GETKEY_MOD_SHIFT      = 0x0001,
	GETKEY_MOD_ALPHA      = 0x0002,
	/* SHIFT + OPTN (requires GETKEY_MOD_SHIFT) or LIGHT toggles backlight */
	GETKEY_BACKLIGHT      = 0x0004,
	/* MENU triggers a task switch and displays the main menu */
	GETKEY_MENU           = 0x0008,
	/* Repeat arrow keys, or even all keys */
	GETKEY_REP_ARROWS     = 0x0010,
	GETKEY_REP_ALL        = 0x0020,
	/* Enable custom repeat profiles; see getkey_set_repeat_profile() */
	GETKEY_REP_PROFILE    = 0x0040,
	/* Enable application shortcuts; see getkey_set_feature_function() */
	GETKEY_FEATURES       = 0x0080,
	/* After coming back from the main menu, redraw the screen with dupdate */
	GETKEY_MENU_DUPDATE   = 0x0100,
	/* After coming back from the main menu, send a KEYEV_OSMENU event */
	GETKEY_MENU_EVENT     = 0x0200,
	/* Enable power off with SHIFT + AC/ON */
	GETKEY_POWEROFF       = 0x0400,

	/* No modifiers */
	GETKEY_NONE           = 0x0000,
	/* Default settings of getkey() */
	GETKEY_DEFAULT        = 0x05df,
};

/* getkey_feature_t: Custom feature function
   See getkey_set_feature_function() for details. */
typedef bool (*getkey_feature_t)(key_event_t event);

/* getkey_opt(): Enhanced getkey()

   This function enhances getkey() with more general features. An
   or-combination of option flags (see above) must be supplied as first
   argument; GETKEY_NONE stands for no option. getkey_opt() returns the same
   kind of events as getkey().

   getkey_opt() supports a generic timeout function in the form of a volatile
   pointer [timeout]. If it's NULL, getkey_opt() waits indefinitely. Otherwise,
   it waits until *timeout becomes non-zero. It's up to you to change the
   value whenever you want to interrupt the call; using a timer with
   [timer_timeout] as callback is suitable. See <gint/timer.h>.

   Event transforms in getkey_opt() (SHIFT, ALPHA and repetitions) are handled
   by changing the transform settings on the keyboard device. These settings
   are restored when getkey_opt() returns, so if they are originally disabled
   (which they are unless set manually) then the status of the SHIFT and ALPHA
   keys is lost between calls (this has an effect it getkey_opt() is
   interrupted by timeout). Therefore, in order to use modifiers across several
   calls to getkey_opt(), make sure to enable the transforms on the keyboard
   device; see <gint/drivers/keydev.h> for details.

   @options  An or-combination of values from the GETKEY_* enumeration
   @timeout  Optional pointer to a timeout value
   Returns a key event of type KEYEV_DOWN or KEYEV_HOLD with [mod=1]. */
key_event_t getkey_opt(int options, volatile int *timeout);

/* getkey_feature_function(): Get the current feature function */
getkey_feature_t getkey_feature_function(void);

/* getkey_set_feature_function(): Set the global feature function

   The feature function can be used to extend getkey() with application-wide
   shortcuts, in a way similar to the CATALOG, CAPTURE or OFF functions of the
   original getkey(). The feature function can be set globally at the
   application level, and thus does not require every call site that uses
   getkey() to support the shortcuts explicitly.

   The feature function receives events when they are generated; if can them
   process them, and return either true to accept the event (preventing
   getkey() from returning it) or false to refuse the event (in which case it
   is returned normally).

   The feature function is enabled by default and be disabled by removing the
   GETKEY_FEATURES flag in getkey_opt(). Setting function=NULL disables the
   functionality. */
void getkey_set_feature_function(getkey_feature_t function);

//---
//	Key code functions
//---

/* keycode_function(): Identify keys F1 .. F6
   This function returns number of each F-key (eg. it returns 2 for KEY_F2),
   and -1 for other keys. */
int keycode_function(int keycode);

/* keycode_digit(): Identify keys 0 .. 9
   This function returns the digit associated with digit keycodes (eg. it
   returns 7 for KEY_7) and -1 for other keys. */
int keycode_digit(int keycode);

#ifdef __cplusplus
}
#endif

//---
// Deprecated functions for repeat control
//
// The following types and functions have been deprecated. The handling of
// repeats is done inside the keyboard device (see <gint/drivers/keydev.h>).
// Until gint 2.9, the device had repeats disabled by default and getkey()
// enabled them. The problem is that repeats usually occur while getkey() is
// not running, so repeats wouldn't work as expected.
//
// Thus, getkey()-specific APIs related to repeat settings are now deprecated
// in favor of the ones provided by <gint/drivers/keydev.h>, which apply
// globally instead of just when getkey() is being run.
//---

typedef void *getkey_profile_t;

__attribute__((deprecated(
    "Use keydev_set_standard_repeats(), which is permanent, instead")))
void getkey_repeat(int first, int next);

__attribute__((deprecated(
    "Use keydev_transform(keydev_std()).repeater instead")))
getkey_profile_t getkey_repeat_profile(void);

__attribute__((deprecated(
    "Set the [repeater] attribute with keydev_set_transform() instead")))
void getkey_set_repeat_profile(getkey_profile_t profile);

#endif /* GINT_KEYBOARD */
