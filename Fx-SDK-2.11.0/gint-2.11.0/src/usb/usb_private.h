//---
// gint:usb:usb-private - Private definitions for the USB driver
//---

#ifndef GINT_USB_USB_PRIVATE
#define GINT_USB_USB_PRIVATE

#include <gint/defs/attributes.h>
#include <gint/defs/timeout.h>
#include <gint/gint.h>
#include <gint/usb.h>

//---
// Configuration of the communication surface between module and host
//---

/* usb_configure_solve(): Find a configuration that can open these interfaces

   This function determines a way to share USB resources (endpoint numbers,
   pipes, and FIFO memory) between the provided interfaces, in order to open
   the connection with all of these interfaces enabled.

   This function must only be called when the USB connection is closed. It
   returns 0 on success and one of the USB_* error codes otherwise.

   @interfaces  NULL-terminated list of interfaces to open
   Returns an USB_* error code. */
int usb_configure_solve(usb_interface_t const **interfaces);

/* usb_configure_log(): Print configuration results with USB_LOG()
   This function can be called even if usb_configure_solve() fails. */
void usb_configure_log(void);

/* usb_configure(): Load the solved configuration to the USB module

   This function configures the USB module's pipes and FIFO memory to prepare
   handling requests to the interfaces activated in usb_configure_solve(). This
   configuration step is un-done by either another configuration through a
   successful usb_open(), or a context restore in the USB driver.

   This function loads all of the "static" data for the pipes, ie. PIPCFG,
   PIPEBUF, PIPEMAXP, and PIPERI, and doesn't change the "dynamic" data in
   PIPECTR. */
void usb_configure(void);

/* usb_configure_clear_pipes(): Clear configured pipes

   This function clears configured pipes' dynamic data in PIPECTR. It shoud be
   used when initializing the module but also when resetting the connection to
   the host (eg. after a world switch), since a renewed host will not expect
   any leftover data, non-zero sequence bits, etc. */
void usb_configure_clear_pipes(void);

/* endpoint_t: Driver information for each open endpoint in the device
   There is one such structure for all 16 configurable endpoints, for each
   direction (totaling 32). use_get_endpoint_*() is used to query the
   structure by endpoint address (including the IN/OUT bit). */
typedef struct {
	/* Which interface this endpoint belongs to */
	usb_interface_t const *intf;
	/* Associated endpoint descriptor. This contains the local address. */
	usb_dc_endpoint_t const *dc;
   /* Global address (endpoint number + direction) */
   uint8_t global_address;
	/* Associated pipe, must be a number from 1..9 */
	uint8_t pipe;
	/* Allocated pipe buffer area; this is valid for pipes 1..5. The
	   bufsize here is in range 1..32, as opposed to the field in PIPEBUF
	   which is in range 0..31. */
	uint8_t bufnmb;
	uint8_t bufsize;

} endpoint_t;

/* usb_configure_interfaces(): List configured interfaces */
usb_interface_t const * const *usb_configure_interfaces(void);

/* Returns the endpoint information structure for the endpoint identified by a
   global address. Returns NULL if the address is unused. */
endpoint_t *usb_get_endpoint_by_global_address(int global_address);

/* Returns the endpoint information structure for the endpoint identified by a
   local (interface) address, NULL if the address is unused. */
endpoint_t *usb_get_endpoint_by_local_address(usb_interface_t const *intf,
   int local_address);

/* Returns the endpoint information structure for the endpoint identified by a
   bound pipe. Returns NULL if the pipe is unused. */
endpoint_t *usb_get_endpoint_by_pipe(int pipe);

//---
// Pipe operations
//
// When writing to a pipe, the general workflow is as follows:
//
// 1. The user performs a write of a block of memory of any size. Because the
//    FIFO for the pipe only has a limited size, the driver splits the write
//    into "rounds" of the size of the FIFO.
//
//    The rounds are written to the FIFO. If the FIFO is full, the write
//    continues until the FIFO can be accessed again (often after the contents
//    of the FIFO have been transmitted, except in double-buffer mode).
//
//    If the last round is smaller than the size of the FIFO, the data is not
//    transmitted; this allows the user to perform another write immediately.
//
// 2. The user performs more writes, each of which are split into rounds, with
//    each round possibly triggering a transfer (if the FIFO is full). Each
//    write only finishes after all the data is written and the pipe is
//    available for more writing.
//
// 3. After the last write, the user *commits* the pipe, causing any data
//    remaining in the FIFO to be transferred even if the FIFO is not full. The
//    commit operation finishes when the pipe is writable again.
//---

/* usb_pipe_configure(): Configure a pipe when opening the connection */
void usb_pipe_configure(int address, endpoint_t const *ep);

/* usb_pipe_clear(): Clear all data in the pipe */
void usb_pipe_clear(int pipe);

/* usb_pipe_reset(): Clear all data in pipe and reset it to a blank state */
void usb_pipe_reset(int pipe);

/* usb_pipe_reset_fifos(): Unbind all FIFOs from pipes */
void usb_pipe_reset_fifos(void);

/* usb_pipe_write_bemp(): Callback for the BEMP interrupt on a pipe */
void usb_pipe_write_bemp(int pipe);

/* usb_pipe_read_brdy(): Callback for the BRDY interrupt on a read pipe */
void usb_pipe_read_brdy(int pipe);

/* usb_pipe_init_transfers(): Initialize transfer information */
void usb_pipe_init_transfers(void);

/* usb_wait_all_transfers(): Wait for all transfers to finish

   This function waits for all current operations on the pipes to finish their
   current read/write/fsync call. Once the waiting period is finished, the
   calls are guaranteed to be finished, but write transactions might not (as
   they require multiple calls finishing with a fsync(2)).

   If `await_long_writes` is set, this function also waits for all writes to be
   committed, which only makes sense if said writes are executed in a thread
   that is able to run while this is waiting. */
void usb_wait_all_transfers(bool await_long_writes);

/* usb_pipe_write4(): Copy arbitrary ranges of memory to a 4-byte USB FIFO

   This function copies arbitrarily-aligned data of any size into a 4-byte
   USB FIFO register. It rearranges data so as to perform only 4-byte aligned
   writes. If the data size isn't a multiple of 4 bytes, it stores the
   remainder into a short buffer (holding between 0 and 3 bytes), to be
   combined with fresh data on the next call. The remainder of the buffer can
   be discharged eventually with usb_pipe_flush4().

   @data         Data to write into the FIFO
   @size         Number of bytes to write
   @buffer       Address of short buffer
   @buffer_size  Address of short buffer's size tracker
   @FIFO         FIFO to output to */
void usb_pipe_write4(void const *data, int size, uint32_t volatile *buffer,
   uint8_t volatile *buffer_size, uint32_t volatile *FIFO);

/* usb_pipe_flush4(): Flush usb_pipe_write4()'s short buffer

   This function is used after a sequence of usb_pipe_write4() to flush the
   last few bytes remaining in the short buffer.

   @buffer       Contents of short buffer
   @buffer_size  Short buffer's size tracker
   @FIFO         FIFO to output to */
void usb_pipe_flush4(uint32_t buffer, int buffer_size,
   uint32_t volatile *FIFO);

/* usb_pipe_read4(): Copy arbitrary ranges of memory from a 4-byte USB FIFO

   This function performs arbitrarily-aligned reads of any size from a 4-byte
   USB FIFO register to regular memory. It performs only 4-byte reads on the
   FIFO (except when reading the last few bytes in the buffer) and copies read
   data to the supplied buffer. Any excess bytes read from the FIFO are stored
   in a short buffer to be used on the next call.

   @data         Buffer to read into
   @data_size    Read size (1 ≤ data_size ≤ fifo_size + *buffer_size)
   @fifo         USB FIFO register to read from
   @fifo_size    Amount of data left in the FIFO (excluding short buffer!)
   @buffer       Address of short buffer
   @buffer_size  Address of short buffer's size tracker */
void usb_pipe_read4(void *data, int size, uint32_t volatile *FIFO,
   int fifo_size, uint32_t volatile *buffer, uint8_t volatile *buffer_size);

//---
// Timeout waits
//---

/* usb_while(): A while loop with a timeout */
#define usb_while(COND) ({ \
   timeout_t __t = timeout_make_ms(100); \
   bool __b = false; \
   while((COND) && !(__b = timeout_elapsed(&__t))) {} \
	if(__b) USB_LOG("%s:%d: inf. while(" #COND ")\n", __FUNCTION__, __LINE__); \
})

//---
// SETUP requests
//---

/* Standard SETUP requests */
enum {
	GET_STATUS         =  0,
	CLEAR_FEATURE      =  1,
	SET_FEATURE        =  3,
	SET_ADDRESS        =  5,
	GET_DESCRIPTOR     =  6,
	SET_DESCRIPTOR     =  7,
	GET_CONFIGURATION  =  8,
	SET_CONFIGURATION  =  9,
	GET_INTERFACE      = 10,
	SET_INTERFACE      = 11,
	SYNCH_FRAME        = 12,
};

/* usb_req_setup(): Answer a SETUP request from the userspace handler

   THis function handles a SETUP request from the host, detected with the VALID
   bit in the INTSTS0 register. The inputs are the USBREQ, USBVAL, USBINDX and
   USBLENG registers, along with the DCP FIFO. */
void usb_req_setup(void);

//---
// Enumerated constants
//---

enum {
   PID_NAK = 0,
   PID_BUF = 1,
   PID_STALL2 = 2,
   PID_STALL3 = 3,
};
enum {
   TYPE_BULK = 1,
   TYPE_INTERRUPT = 2,
   TYPE_ISOCHRONOUS = 3,
};

#endif /* GINT_USB_USB_PRIVATE */
