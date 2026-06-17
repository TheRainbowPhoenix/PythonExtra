//---
//	gint:keydev - Kernel's keyboard input devices
//---

#ifndef GINT_KEYDEV
#define GINT_KEYDEV

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/keyboard.h>
#include <stdbool.h>

/* Size of the buffer event queue */
#define KEYBOARD_QUEUE_SIZE 32

/* Event transforms

   Every keyboard input device has a built-in event stream editor that can
   transform the events in various ways to change the abstraction details, from
   the raw pollevent() with no transforms to the highest-level getkey().

   Because many transforms like <Instant Modifiers> need to observe the state
   of the keyboard at the time of the event to transform, this process needs to
   be performed right when events are requested and cannot be done later
   without keeping around a lot of information about past states.

   Just as switching from pollevent() to getkey() requires some caution,
   changing the transform settings while keys are pressed or modifiers are
   active minimally impacts the output of the editor. Here is a summary of the
   interactions:

   * <Delete Modifiers> and <Delete Releases> are only concerned about the
     current event and can be turned ON and OFF at any time without problems.

   * <Instant SHIFT> and <Instant ALPHA> only look at the state of the keyboard
     with keydev_keydown() and are not affected by other options.

   * <Delayed SHIFT> and <Delayed ALPHA> only trigger when a SHIFT or ALPHA
     press-release is performed when the keyboard is idle.
     -> If turned ON while a key is pressed, nothing happens until a further
        SHIFT or ALPHA key press while the keyboard is idle.
     -> If turned OFF while (1) a modifier key is being pressed, or (2) a
        modifier key has been pressed and released (charging up the delayed
        modifier), the charge is lost.
     Thus turning delayed modifiers ON->OFF->ON drops the current charge.

   * <Repeats> has the most interactions.
     -> Toggling <Instant SHIFT> or <Instant ALPHA> during a repeat takes
        effect immediately, so a streak can start with instant modifiers
        disabled, enable them mid-flight, and start producing repeating events
        with instant modifiers.
     -> Switching <Delayed SHIFT> or <Delayed ALPHA> during a modified streak
        drops the modifier immediately.
     -> Switching ON any modifier transform breaks a SHIFT or ALPHA streak.
     -> Switching OFF all modifier transforms allows a currently-pressed SHIFT
        or ALPHA key to start a repeat streak, which (due to the way candidates
        for repeats are considered) can only be used to turn a incomplete
        delayed modifier charge or an unused instant modifier into a streak.

   Generally speaking changing options while keys are pressed is safe, but it
   is better to do it while the device is idle for more consistency.

   pollevent() provides the raw events and bypasses the transforms. However,
   the information needed to resume transforming is still tracked when
   generating raw events: for instance, which key is a candidate for repetition
   is still tracked (by contrast, getkey() needs special code to catch up if
   pollevent() unqueued events that it needed). Delayed modifiers are not
   tracked because they can be charged only if the transform is enabled when
   the modifier key is pressed. */
enum {
	/* Delayed SHIFT: Pressing then immediately releasing SHIFT when the
	   keyboard is idle applies SHIFT to the next repeat streak. */
	KEYDEV_TR_DELAYED_SHIFT    = 0x01, /* = GETKEY_MOD_SHIFT */
	/* Delayed ALPHA: Idem with the ALPHA key */
	KEYDEV_TR_DELAYED_ALPHA    = 0x02, /* = GETKEY_MOD_ALPHA */
	/* Combination of the delayed modifiers */
	KEYDEV_TR_DELAYED_MODS     = KEYDEV_TR_DELAYED_SHIFT
	                           | KEYDEV_TR_DELAYED_ALPHA,

	/* Instant SHIFT: Each individual event of every repeat streak gets
	   SHIFT applied if SHIFT is pressed at the time of the repeat. */
	KEYDEV_TR_INSTANT_SHIFT    = 0x04,
	/* Instant ALPHA: Idem with the ALPHA key */
	KEYDEV_TR_INSTANT_ALPHA    = 0x08,
	/* Combination of the instance modifiers */
	KEYDEV_TR_INSTANT_MODS     = KEYDEV_TR_INSTANT_SHIFT
	                           | KEYDEV_TR_INSTANT_ALPHA,

	/* Combination of all modifiers */
	KEYDEV_TR_ALL_MODS         = KEYDEV_TR_DELAYED_MODS
	                           | KEYDEV_TR_INSTANT_MODS,

	/* Repeats: Keys are repeated according to a repeat filter function */
	KEYDEV_TR_REPEATS          = 0x10,
	/* Delete Modifiers: Remove modifier keys from generated events, which
	   are generally useless when delayed/instant SHIFT/ALPHA are used */
	KEYDEV_TR_DELETE_MODIFIERS = 0x20,
	/* Delete Releases: Remove KEYEV_UP events */
	KEYDEV_TR_DELETE_RELEASES  = 0x40,
};

/* keydev_repeat_profile_t: Function deciding when and how to repeat keys. */
typedef int (*keydev_repeat_profile_t)(int key, int duration, int count);

/* keydev_transform_t: Full specification for transforms on the event stream */
typedef struct {
	/* List of enabled transforms. The order is cannot be changed because
	   any other order than the default produces useless results. This is
	   an OR-combination of KEYDEV_TR_* flags. */
	int enabled;

	/* Repeater function. This function is called whenever (key) is pressed
	   (KEYEV_DOWN) or repeated (KEYEV_HOLD), and should indicate how long
	   to wait until the next repeat. This setting is meaningful only if
	   <Repeats> is enabled.

	   -> (key) is the key currently being repeated.
	   -> (duration) is the time since it was first pressed (us).
	   -> (count) is the number of repeats produced so far.

	   The function should either return -1 to disable all further repeats,
	   or a positive number of microseconds until the next repeat. Note
	   that the precision of the delay is limited by the speed at which the
	   keyboard is scanned, which is nowhere near microsecond-level. By
	   default (128 Hz) the precision is about 7.8 ms. */
	keydev_repeat_profile_t repeater;

} GPACKEDENUM keydev_transform_t;

/* keydev_async_filter_t: Low-level asynchronous event filter. */
typedef bool (*keydev_async_filter_t)(key_event_t event);

/* keydev_t: Keyboard device

   This structure represents the state and settings of a keyboard device that
   provides input to the kernel and applications. The notion of keyboard device
   is useful for demo/replays that input events without the physical keyboard,
   and a couple of corner uses like control over USB.

   The keyboard device has built-in event transformations, which modify the
   stream of events by adding information, combining modifiers, and removing
   undesired events. Because the event transformation rely on the current state
   of the keyboard, they must be run by the driver whenever events are read, so
   they are tied to the device.

   The default keyboard functions pollevent(), waitevent(), getkey() and
   keydown() are shortcuts for keydev functions using the physical keyboard as
   their input. */
typedef struct {
	/* Current device time in scanning-ticks */
	uint time;
	/* Last time when repeats were considered */
	uint time_repeats;

	/* Next event in queue, position after last event in queue */
	int8_t queue_next;
	int8_t queue_end;
	/* Number of events lost because of missing queue space */
	uint16_t events_lost;

	/* Event transforms */
	keydev_transform_t tr;

	/* Asynchronous event filter. This is a low-level filter which is
	   called when events are generated to process and filter them. It
	   provides the unique ability to run keyboard-triggered code even if
	   the program's main thread is busy, for instance running some sort of
	   infinite loop. */
	keydev_async_filter_t async_filter;

	// <Delayed Modifiers>

	/* delayed_* is set when the delayed modifier is active (after a press/
	   release). pressed_* is set between the press and the release. */
	uint pressed_shift :1;
	uint pressed_alpha :1;
	uint delayed_shift :1;
	uint delayed_alpha :1;
	uint :4;

	// <Repeats>

	/* Candidate key for repeats (or 0 if no key is candidate yet) */
	int16_t rep_key;
	/* Number of repeats already sent */
	int16_t rep_count;
	/* Time since key was first pressed (us) */
	int rep_time;
	/* Delay until next repeat, set by the repeat planner (us) */
	int rep_delay;

	/* Latest state of keys we are aware of. At every processing step, the
	   difference between this and the fresh information is queued and this
	   is updated. state_now is identical to the real state obtained from
	   the device unless earlier events failed to be queued, in which case
	   a difference is maintained so they will be reconsidered later. */
	GALIGNED(4) uint8_t state_now[12];
	/* State of keys based on produced events. (state_queue + queue) is
	   always identical to (state_now). When the queue is empty both states
	   are the same. This is the user's view of the keyboard. */
	GALIGNED(4) uint8_t state_queue[12];
	/* Event queue (circular buffer) */
	key_event_t queue[KEYBOARD_QUEUE_SIZE];

	/* Parameters for the standard repeat function */
	int rep_standard_first, rep_standard_next;

   /* State of keys that have changes since the last flip monitoring reset. */
   GALIGNED(4) uint8_t state_flips[12];

} keydev_t;

/* keydev_std(): Standard keyboard input device
   Returns the keyboard device structure for the calculator's keyboard. */
keydev_t *keydev_std(void);

//---
//	Functions for keyboard device drivers
//---

/* keydev_init(): Initialize a keyboard device to its idle state */
void keydev_init(keydev_t *d);

/* keydev_process_state(): Process the new keyboard states for events
   This function compares the provided scanned state with the stored state and
   generates events to catch up. This should be used by scan-based keyboard
   devices after a scan. */
void keydev_process_state(keydev_t *d, uint8_t state[12]);

/* keydev_process_key(): Process a new key state for events
   This function compares the provided key state with the stored state and
   generates events to catch up. This should be used by event-based keyboard
   devices (such as demo replays) to feed in new events. */
void keydev_process_key(keydev_t *d, int keycode, bool state);

/* keydev_repeat_event(): Generate a repeat event if applicable
   At the end of every scan tick, this source will generate a repeat event if
   the repeat transform is enabled and the conditions for a repeat are
   satisfied. */
key_event_t keydev_repeat_event(keydev_t *d);

/* keydev_tick(): Prepare the next tick
   This function maintains time trackers in the device and should be called in
   each frame after the scanning is finished and the keydev_process_*()
   functions have been called. The timer elapsed since the last tick should
   be specified as well. */
void keydev_tick(keydev_t *d, uint us);

//---
//	Low-level API to read events from the device
//---

/* keydev_unqueue_event(): Retrieve the next keyboard event in queue

   This source provides the queued KEYEV_UP, KEYEV_DOWN and KEYEV_HOLD events
   generated from the regular scans in chronological order. It does not apply
   transforms; to do this, use keydev_read(). */
key_event_t keydev_unqueue_event(keydev_t *d);

/* keydev_idle(): Check if all keys are released
   A list of keys to ignore can be specified as variable arguments. The list
   must be terminated by a 0 keycode. */
bool keydev_idle(keydev_t *d, ...);

/* keydev_async_filter(): Obtain current async filter */
keydev_async_filter_t keydev_async_filter(keydev_t const *d);

/* keydev_set_async_filter(): Set low-level async filter */
void keydev_set_async_filter(keydev_t *d, keydev_async_filter_t filter);

//---
//	High-level API to read from the device
//---

/* keydev_keydown(): Check if a key is down according to generated events */
bool keydev_keydown(keydev_t *d, int key);

/* keydev_keypressed(): Check if a key was pressed
   This compares to the state at the time of the last keydev_clear_flips(). */
bool keydev_keypressed(keydev_t *d, int key);

/* keydev_keyreleased(): Check if a key was released
   This compares to the state at the time of the last keydev_clear_flips(). */
bool keydev_keyreleased(keydev_t *d, int key);

/* keydev_clear_flips(): Reset flip info used by keypressed()/keyreleased() */
void keydev_clear_flips(keydev_t *d);

/* keydev_transform(): Obtain current transform parameters */
keydev_transform_t keydev_transform(keydev_t *d);

/* keydev_set_transform(): Set transform parameters */
void keydev_set_transform(keydev_t *d, keydev_transform_t tr);

/* keydev_set_standard_repeats(): Enable a simple repeater

   This function changes the [repeater] member of the devices's transform. It
   loads the default repeat profile which applies one delay (in us) before the
   first repeat, and then a second, usually shorter delay, between subsequent
   repeats.

   The unit of the argument is in microseconds, but the granularity of the
   delay is dependent on the keyboard scan frequency. In the default setting
   (128 Hz scans), the possible repeat delays are approximately 8 ms, 16 ms,
   23 ms, 31 ms. The system default is (500 ms, 125 ms). With the 128 Hz
   setting, this default is reached exactly without approximation. gint's
   default is (400 ms, 40 ms) for more reactivity.

   Note: Due to a current API limitation, every input device uses the delays
   for the physical keyboard.

   @first_us  Delay between key press and first repeat (microseconds)
   @next_us   Delay between subsequent repeats (microseconds) */
void keydev_set_standard_repeats(keydev_t *d, int first_us, int next_us);

/* keydev_read(): Retrieve the next transformed event
   If there is no event, returns an event with type KEYEV_NONE, unless
   [wait=true], in which case waits for an event to occur or *timeout to
   become non-zero (if timeout is not NULL), whichever comes first. */
key_event_t keydev_read(keydev_t *d, bool wait, volatile int *timeout);

#ifdef __cplusplus
}
#endif

#endif /* GINT_KEYDEV */
