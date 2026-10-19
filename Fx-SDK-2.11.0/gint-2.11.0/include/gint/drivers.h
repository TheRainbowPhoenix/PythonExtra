//---
//	gint:drivers - General tools for drivers
//---

#ifndef GINT_DRIVERS
#define GINT_DRIVERS

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

/* Device drivers and driver cycles

   A driver is any part of the program that manages some piece of hardware.
   Because gint coexists with the default operating system, special care has to
   be taken in manipulating the hardware to avoid compatibility problems, and
   this is implemented by the drivers.

   [Driver state vocabulary]

   There are several states of interest for a driver and its device:
   * The device is said to be *powered* if the clock is supplied to the module
     and the device can operate normally.
   * The driver is said to be *bound* to the device if is has exclusive access
     to the hardware and can operate on it.
   * The device is said to be *configured* if it is powered and the driver has
     initialized the hardware to start running its API.
   * The driver is said to be *active* if (1) it is bound, or (2) it is planned
     to be bound the next time gint takes over. A driver can be inactive if it
     delays its initialization until it is needed by the add-in. This is
     relevant for drivers which have an expensive start/stop sequence.
   * The device is said to be *shared* if it doesn't require its hardware state
     to be preserved when switching between gint and the OS. Usually this is
     specified by the add-in when the add-in knows that the driver will not be
     used over a certain period of time.

   For consistency in gint (related to world switches), only devices that are
   powered can be bound. It would be possible to have drivers power devices on
   and off while bound, but it complicates the design; therefore, if a device
   is shut down by the user, the driver will be unbound first.

   [Hardware sharing and driver state machine]

   gint's drivers are special because (1) gint is not the only kernel running
   on the machine, and (2) the other kernel doesn't know about it. To ensure
   stability driving the hardware ourselves, we must guarantee that OS code is
   oblivious to hardware changes, so we save and restore hardware state
   whenever gint takes control over or gives it back. This makes gint sort of
   a hypervisor while also being one of its guests. (Yatis once materialized
   this idea and used gint's world switch mechanic to build a true hypervisor.)

   If the built-in OS were aware of the hardware sharing, it would implement
   mechanisms to unbind its drivers from the hardware in order to allow gint to
   take over, and we would do so in return. However this process is not
   implemented in the OS, which means that we need to cover it ourselves. gint
   calls this operation a "foreign unbind" and it is mostly used in modules
   with asynchronous APIs where operations must finish before gint can take
   control.

   This means that drivers in gint have to handle three types of tasks:
   * "Foreign" tasks: unbinding the OS driver from the device.
   * "Hypervisor" tasks: handling the transition between OS and gint.
   * "Normal" tasks: driving the device for the gint add-in.

   Foreign tasks comprise everything that happens while the OS driver is bound,
   normal tasks is everything that happens while the gint driver is bound, and
   the "hypervisor" tasks are everything in-between. Driver functions for
   foreign tasks start with an "f" and functions for "hypervisor" tasks start
   with an "h".

   The state machine for driver binding and device power is as follows. Power
   management in gint occurs in the middle level; a foreign unbind does not
   change the device power, and the device power from the last foreign unbind
   is restored before running OS code.

            Device is owned and operated by the OS
                (power is either ON or OFF)
                           |    ^
                 funbind() |    | Running any OS code
                           v    |
                    Device is not bound
              Power <-- hpoweron() ---- Power
              is ON --- hpoweroff() --> is OFF
              |   ^
       bind() |   | unbind()
              v   |
            Device is powered and operated by gint

   For safety, the device is considered bound to the OS whenever OS code is
   run, even when it is not powered. The reason why gint unbinds its drivers
   before unpowering a device is to make sure that power management is kept in
   the "hypervisor" section of the code. The unbind() and funbind() functions
   should ensure that the device is idle with no interrupts pending, which
   allows proper shutdown and a clean hardware save state.

   [World switch]

   When handing back hardware control to the OS, gint restores devices to their
   state at the last control takeover (mostly). The drivers provide two
   "hypervisor" calls for this feature, hsave() and hrestore(). The combined
   saved states of all devices are called a *world*. The action of switching
   hardware states to isolate the execution of the two kernels is called a
   *world switch*.

   gint exposes a handful of OS features via world switches, such as
   return-to-main-menu (commonly used in getkey()) and BFile access to the
   filesystem. A stable world switch mitigates a lot of the drawbacks of using
   a custom kernel, up to (1) the inability to run gint code and OS code
   simultaneously (ie. timers during a BFile operation), and (2) the small
   runtime cost of a world switch. Note that (1) is more of a policy question,
   as it is always possible to access hardware while the OS runs (which mostly
   works but offers limited stability, whether gint is used or not).

   The world switch mechanism can be customized to a certain extent, allowing
   to not restore drivers during world transitions (such drivers are called
   "shared"). This is important in Yatis' tracer/debugger, which uses a
   gint-driven User Break Controller while in the OS world to control and
   monitor the execution of syscalls, performing world transitions to gint when
   breakpoints are hit to display and analyze code without affecting it. This
   can also be used to keep some profiling timers alive during OS operations.

   A switch from the OS world to the gint world will start like this;
      1. funbind() (with OS-handled interrupts still enabled)
      2. SR.IMASK=15
   Then, for most drivers:
      a3. hpoweron() if the device was not powered in the OS
      a4. hsave(<OS world buffer>)
      a5. hrestore(<gint world buffer>) if not running for the first time
      a6. bind()
      a7. configure() if running for the first time
      a8. SR.IMASK=0
   There is an exception if the driver is shared:
      b3. hpoweron() if the device was not powered in the OS
      b4. bind()
      b5. configure() if running for the first time
      b6. SR.IMASK=0

   A switch from the gint world to the OS world will execute this sequence:
      a1. unbind() (with gint-handled interrupts still enabled)
      a2. SR.IMASK=15
      a3. hsave(<gint world buffer>)
      a4. hrestore(<OS world buffer>)
      a5. hpoweroff() if the device was powered off at the last funbind()
      a6. SR.IMASK=0
   There is again an exception if the device is shared:
      b1. unbind() (with gint-handled interrupts still enabled)
      b2. SR.IMASK=15
      b3. hpoweroff() if the device was powered off at the last funbind()
      b4. SR.IMASK=0

   [Driver settings]

   Each driver has a *level* which indicates its relationship with other
   drivers. Specifically, a driver is only allowed to use functions from
   drivers of a lower level. gint makes sure that functions of the "hypervisor"
   and normal category are executed while all drivers of lower levels are
   bound (unless the user explicitly disables them).

   The driver can also initialize the following flags in the driver definition
   and customize them at runtime:

   * GINT_DRV_SHARED: Makes the driver shared, meaning that its hardware state
     is not saved and restored during world switches. Note that the CPU and
     INTC drivers are not shared so interrupts will not be available in a
     foreign world. */
typedef struct {
	/* Driver name */
	char const *name;
	/* General constructor, is called before any interaction with the driver.
	   This can be used to adjust settings based on detected hardware. */
	void (*constructor)(void);

	// Foreign calls

	/* Foreign unbind: separate the hardware from the OS driver. If NULL, the
	   OS driver is always considered idle. */
	void (*funbind)(void);

	// "Hypervisor" calls

	/* Determine whether the device is powered. If NULL, the device is assumed
	   to be permanently powered. */
	bool (*hpowered)(void);
	/* Power on the device; this should allow register access to save the
	   peripheral state, with minimal state changes. Cannot be NULL if
	   hpowered() can return false. */
	void (*hpoweron)(void);
	/* Power off the device; cannot be NULL if hpowered() can return false. */
	void (*hpoweroff)(void);

	/* Save the hardware state; the (state) pointer points to a 4-aligned
	   region of (state_size) bytes. */
	void (*hsave)(void *state);
	/* Restore a hardware state previously saved by hsave(). */
	void (*hrestore)(void const *state);

	// Standard calls

	/* Bind the driver to acquire control of the device. May be NULL. */
	void (*bind)(void);
	/* Unbind the driver from the hardware. Usually sleeps until processes that
	   block world switches terminate, like bind(). May be NULL. */
	void (*unbind)(void);

	/* Initialize the hardware for the driver to work in gint. Usually installs
	   interrupt handlers and configures registers. May be NULL. */
	void (*configure)(void);

	/* Size of the peripheral's hardware state (assumed 4-aligned) */
	uint16_t state_size;

	/* Initial flags */
	uint8_t flags;

} gint_driver_t;

enum {
	/* Driver is clean (needs to be configured before running) */
	GINT_DRV_CLEAN = 0x01,
	/* Device was powered during the last foreign unbind */
	GINT_DRV_FOREIGN_POWERED = 0x02,

	/* Driver does not require hardware state saves during world switches */
	GINT_DRV_SHARED = 0x10,

	/* Flags that can be set in the (flags) attribute of the driver struct */
	GINT_DRV_INIT_ = 0x10,
};

/* gint_world_t: World state capture

   The world state is a copy of the (almost) complete hardware state, which can
   be used to switch between several kernels running in parallel on the same
   machine. gint runs in a different world than the OS, allowing it to control
   peripheral modules in ways incompatible with the OS without compromising the
   stability of either program.

   The world state is a sequence of 4-aligned buffers each holding a copy of a
   module's state, as saved (and restored) by a driver. It is prefixed with an
   array of pointers, one for each driver, specifying the driver's spot within
   the sequence.

   The layout is as follows:
   * An array of (void *), with one entry per driver, in priority order. Each
     pointer is to a buffer in the sequence.
   * A sequence of buffers of size (state_size), rounded up to a multiple of 4
     bytes, for each driver in priority order.

   The world is returned as a (void *) array but allocated in one block. */
typedef void **gint_world_t;

/* GINT_DECLARE_DRIVER(): Declare a driver to the kernel

   Use this macro to declare a driver by passing it the name of a gint_driver_t
   structure. This macro moves the structure to the .gint.drivers.* sections,
   which are automatically traversed at startup.

   The level argument represents the priority level: lower numbers mean that
   drivers will be loaded sooner. This numbering allows a primitive form of
   dependency for drivers. You need to specify a level which is strictly
   higher than the level of all the drivers you depend on.

   The level number *MUST HAVE EXACTLY 2 DIGITS*, as it is used as a string in
   the section name and the linker then sorts by name. If your driver has a
   level lower than 10, you must add a leading 0. */
#define GINT_DECLARE_DRIVER(level, name) \
	GSECTION(".gint.drivers." #level) GVISIBLE extern gint_driver_t name;

//---
// Internal driver control
//
// The following data is exposed for introspection and debugging purposes; it
// is not part of the gint API. There is *no stability guarantee* that the
// following types and functions will remain unchanged in future minor and
// patch versions.
//---

/* Drivers in order of increasing priority level, provided by linker script */
extern gint_driver_t gint_drivers[];
/* End of array; see also gint_driver_count() */
extern gint_driver_t gint_drivers_end[];
/* Current flags for all drivers */
extern uint8_t *gint_driver_flags;

/* Number of drivers in the (gint_drivers) array */
#define gint_driver_count() \
	((gint_driver_t *)&gint_drivers_end - (gint_driver_t *)&gint_drivers)

/* Allocate a new world buffer (single block), returns NULL on error */
gint_world_t gint_world_alloc(void);

/* Free a world buffer */
void gint_world_free(gint_world_t world);

/* The world buffers of gint and the OS */
extern gint_world_t gint_world_addin, gint_world_os;

/* Switch from the OS world to a gint-managed world */
void gint_world_switch_in(gint_world_t world_os, gint_world_t world_addin);

/* Switch from a gint-managed world to the OS world */
void gint_world_switch_out(gint_world_t world_addin, gint_world_t world_os);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS */
