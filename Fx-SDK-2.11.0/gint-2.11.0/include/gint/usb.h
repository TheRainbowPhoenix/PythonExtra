//---
// gint:usb - USB communication
//---

#ifndef GINT_USB
#define GINT_USB

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/defs/timeout.h>
#include <gint/gint.h>
#include <gint/config.h>
#include <stdarg.h>
#include <endian.h>

/* See "Interfaces to communicate with USB transfers" below for details */
typedef struct usb_interface usb_interface_t;
typedef struct usb_interface_endpoint usb_interface_endpoint_t;

//---
// General functions
//---

/* Error codes for USB functions */
enum {
	/* There are no interfaces */
	USB_OPEN_NO_INTERFACE = 1,
	/* There are more interfaces than supported (16) */
	USB_OPEN_TOO_MANY_INTERFACES = -2,
	/* There are not enough endpoint numbers for every interface, or there
	   are not enough pipes to set them up */
	USB_OPEN_TOO_MANY_ENDPOINTS = -3,
	/* There is not enough FIFO memory to use the requested buffer sizes */
	USB_OPEN_NOT_ENOUGH_MEMORY = -4,
	/* Information is missing, such as buffer size for some endpoints */
	USB_OPEN_MISSING_DATA = -5,
	/* Invalid parameters: bad endpoint numbers, bad buffer sizes... */
	USB_OPEN_INVALID_PARAMS = -6,
	/* USB interfaces are already opened */
	USB_OPEN_ALREADY_OPEN = -7,

	/* General timeout for a sync_timeout call */
	USB_TIMEOUT = -8,
	/* This pipe is busy with another call */
	USB_BUSY = -9,

	/* Both FIFO controllers are busy, none is available to transfer */
	USB_WRITE_NOFIFO = -10,
	/* This pipe has no ongoing transfer to commit */
	USB_COMMIT_INACTIVE = -11,
	/* This pipe is currently not receiving any data */
	USB_READ_IDLE = -12,
	/* No FIFO controller is available */
	USB_READ_NOFIFO = -13,

	/* (Internal codes) */
	USB_ERROR_ZERO_LENGTH = -100,
};

/* usb_open(): Open the USB link

   This function opens the USB link and notifies the host that the device is
   ready to connect. Usually the host immediately queries the device, and after
   some exchanges the device can be used. The USB link might not be ready when
   this function returns, use the callback or usb_open_wait() for that.

   The first parameters is a NULL-terminated array of interfaces to open. To
   see available interfaces, please see header files in <gint/usb-*.h>. Each
   interface can be used independently, however if there are not enough USB
   resources (buffer memory, pipes or endpoints) for all of them, usb_open()
   will return an error.

   The second parameter is a callback to be (asynchronously) invoked when the
   USB link is ready. Use GINT_CALL() to create one, or pass GINT_CALL_NULL for
   no callback. You can also use usb_open_wait() to synchronously wait for the
   link to be ready.

   @interfaces  NULL-terminate list of interfaces to open
   @callback    Optional function to be called when the USB link opens */
int usb_open(usb_interface_t const **interfaces, gint_call_t callback);

/* usb_open_wait(): Wait until the USB link is ready
   When called after usb_open(), this function waits until the communication is
   established. You should only call this if usb_open() returns 0. */
void usb_open_wait(void);

/* usb_is_open(): Check whether the USB link is active */
bool usb_is_open(void);

/* usb_is_open_interface(): Check whether a particular interface is open */
bool usb_is_open_interface(usb_interface_t const *interface);

/* usb_close(): Close the USB link

   This function closes the link opened by usb_open(), and notifies the host of
   the disconnection (if any was established). The USB link can be reopened
   later to perform more tasks.

   There are two reasons to close the USB link: to save battery power and to
   return to the calculator's main menu. You should thus close it if (1) the
   USB link might not be used for a while, or (2) you want to return to the
   main menu before using it again. */
void usb_close(void);

//---
// Interfaces to communicate with USB transfers
//
// These interfaces define how the calculator behaves on the USB connection,
// and can include stuff like:
//
// -> Communicate with a custom protocol and a custom program on the PC
//    (like Protocol 7 does with FA-124, or fxlink)
// -> Exchange text as a Communications and CDC Control (class 0x03) device
//    (like an Internet router)
// -> Share a video stream as a video input (class 0x0e) device (like a webcam)
//
// Normal add-ins that just want to use the USB connection don't need to worry
// about programming the interfaces; they can simply use interfaces that are
// already implemented. Start with usb_open().
//---

/* usb_interface_t: A USB interface that can be enabled in usb_open()

   This driver provides a device that only has one configuration (due to how
   rare it is for devices to have several configurations). However, a number of
   interfaces can be activated independently. This structure describes an
   interface with regards to this driver.

   The driver chooses endpoint numbers and slices of the FIFO buffer for the
   interface to use, therefore the supplied descriptors cannot specify them.
   Instead, the supplied descriptors should use arbitrary endpoint numbers; the
   driver will use them to communicate with the interface, and transparently
   use concrete endpoint numbers internally. */
struct usb_interface {
	/* NULL-terminated array of descriptors for the interface */
	void const **dc;
	/* Array of endpoint parameters, see below */
	struct usb_interface_endpoint *params;

	/* Answer class-specific SETUP requests */
	/* TODO */

	/* Notification that an endpoint has data to be read. This function is
	   called frequently when data is being transmitted; the particular
	   timings depend on low-level details. The notification should be
	   passed down to cause the main thread to read later. Do not read in
	   the notification function! */
	void (*notify_read)(int endpoint);
};

/* usb_interface_endpoint_t: Parameters for an interface endpoint

   This structure mainly specifies the settings for the pipe associated to the
   endpoint. There 10 pipes, but not all can be used with any transfer type,
   and not all have access to the same amount of memory. */
struct usb_interface_endpoint {
	/* Endpoint number as specified in the interface's descriptors
	   (including the IN/OUT bit) */
	uint8_t endpoint;
	/* Requested buffer size, should be a multiple of 64 and not more than
	   2048. Valid only for bulk and isochronous endpoints. */
	uint16_t buffer_size;
};

/* usb_interface_pipe(): Get the pipe associated to an interface endpoint

   This function returns the pipe number that backs the specified endpoint
   number (using the local value of the interface, not the concrete one). This
   function is intended for interface implementations, not users. */
int usb_interface_pipe(usb_interface_t const *interface, int endpoint);

//---
// Pipe access API
//
// The following functions provide access to USB pipes. Normally the add-in
// will not know which pipe is allocated to each interface, so there is no way
// to reliably access a pipe directly. Instead you should use functions
// provided by the interfaces in <gint/usb-*.h>.
//
// The functions below are useful for interface implementations; an interface
// can get the pipe for an endpoint with usb_interface_pipe() and then use
// direct pipe access.
//---

/* usb_write_sync(): Synchronously write to a USB pipe

   This functions writes (size) bytes of (data) into the specified pipe. If the
   data fits into the pipe, this function returns right away, and the data is
   *not* transmitted. Otherwise, data is written until the pipe is full, at
   which point it is automatically transmitted. After the transfer, this
   function resumes writing, returning only once everything is written. Even
   then the last bytes will still not have been transmitted, to allow for other
   writes to follow. After the last write in a sequence, use usb_commit_sync()
   or usb_commit_async() to transmit the last bytes.

   If (use_dma=true), the write is performed with the DMA instead of the CPU.
   This requires at least 4-byte alignment on:
     1. The input data;
     2. The size of this write;
     3. The amount of data previously written to the pipe not yet committed.
   This is because using the DMA does not allow any insertion of CPU logic to
   handle unaligned stuff.

   This function will use a FIFO controller to access the pipe. The FIFO
   controller will be reserved for further writes until the contents of the
   pipe are commited with usb_commit_sync() or usb_commit_async(); when more
   than two pipes need to operate in parallel, keep the write sequences short
   and commit regularly to avoid holding the controllers.

   If the pipe is busy due to an ongoing asynchronous write or commit, or there
   is no FIFO controller available to perform the operation, this function
   waits for the ressources to become available then proceeds normally.

   @pipe       Pipe to write into
   @data       Source data
   @size       Size of source
   @dma        Whether to use the DMA to perform the write
   -> Returns an error code (0 on success). */
int usb_write_sync(int pipe, void const *data, int size, bool use_dma);

/* usb_write_sync_timeout(): Synchronously write, with a timeout */
int usb_write_sync_timeout(int pipe, void const *data, int size,
	bool use_dma, timeout_t const *timeout);

/* usb_write_async(): Asynchronously write to a USB pipe

   This function is similar to usb_write_sync(), but it only starts the writing
   and returns immediately without ever waiting. The writing then occurs in the
   background of the calling code, and the caller is notified through a
   callback when it completes. Use GINT_CALL() to create a callback or pass
   GINT_CALL_NULL.

   If the pipe is busy due to a previous asynchronous write, this function
   returns USB_WRITE_BUSY. If no FIFO controller is available for the transfer,
   it returns USB_WRITE_NOFIFO. When called with (use_dma=true), it returns as
   soon as the DMA starts, without even a guarantee that the first few bytes
   have been written.

   There is no guarantee that the write is complete until the callback is
   called, however calling again with data=NULL and size=0 can be used to
   determine whether the write has finished, since it will return 0 if the pipe
   is idle and USB_WRITE_BUSY otherwise.

   @pipe       Pipe to write into
   @data       Source data
   @size       Size of source
   @dma        Whether to use the DMA to perform the write
   @callback   Optional callback to invoke when the write completes
   -> Returns an error code (0 on success). */
int usb_write_async(int pipe, void const *data, int size, bool use_dma,
	gint_call_t callback);

/* usb_commit_sync(): Synchronously commit a write

   This function waits for any pending write on the pipe to finish, then
   transfers whatever data is left, and returns when the transfer completes. */
void usb_commit_sync(int pipe);

/* usb_commit_sync_timeout(): Synchronously commit a write, with timeout */
int usb_commit_sync_timeout(int pipe, timeout_t const *timeout);

/* usb_commit_async(): Asynchronously commit a write

   This function commits the specified pipe, causing the pipe to transfer
   written data in the pipe.

   If the pipe is currently busy due to an ongoing write or commit, it returns
   USB_COMMIT_BUSY. You should call usb_commit_async() when the pipe is ready,
   which is either when the previous synchronous call returns, or when the
   callback of the previous asynchronous call is invoked.

   This function returns immediately and invokes (callback) when the transfer
   of the remaining data completes. */
int usb_commit_async(int pipe, gint_call_t callback);

/* usb_read_sync(): Synchronously read from a USB pipe

   This function waits for data to become available on the specified `pipe`,
   and then reads up to `size` bytes into `data`. It is "synchronized" with USB
   transactions on the pipe in the following ways:

   1. If there is no active transaction, it waits for one to arrive.
   2. If the active transaction has no data left to read, it is skipped.
   3. If the transaction's data is completely read, the transaction is closed
      even if `data` is full. (See usb_read_async() for the alternative.)

   Basically usb_read_sync() returns the next `size` bytes of data that the
   calculator receives on the pipe, with a single exception: it doesn't read
   across transactions, so if the active transaction has 100 bytes left and you
   request 200, you will only get 100. usb_read_sync() only ever returns 0
   bytes if there is an empty transaction, which is rare.

   Because this is a blocking function, a call to usb_read_sync() will *FREEZE*
   if there is no data to read on the pipe and the host doesn't send any. In
   addition, it's not possible to detect how much data is left to read with
   usb_read_sync()'s interface. If you want to avoid or debug a freeze, use
   use usb_read_async() or set a timeout with usb_read_sync_timeout(). If you
   want to read a transaction until the end without knowing its size in
   advance, use usb_read_async().

   If `use_dma=true`, uses the DMA for transfers. This requires 4-byte
   alignment on the buffer, size, and number of bytes previously read in the
   current transaction.

   Returns the number of bytes read or a negative error code. */
int usb_read_sync(int pipe, void *data, int size, bool use_dma);

/* usb_read_sync_timeout(): Synchronous read with a timeout */
int usb_read_sync_timeout(int pipe, void *data, int size, bool use_dma,
	timeout_t const *timeout);

/* Flags for usb_read_async(), see below. */
enum {
	/* Use the DMA for data transfer (requires full 4-byte alignment). */
	USB_READ_USE_DMA        = 0x01,
	/* Ignore reads of 0 bytes from exhausted transactions. */
	USB_READ_IGNORE_ZEROS   = 0x02,
	/* Close transactions when exhausted even by non-partial reads. */
	USB_READ_AUTOCLOSE      = 0x04,
	/* Wait for the read to finish before returning. */
	USB_READ_WAIT           = 0x08,
	/* Block for a new transaction if none is currenty active. */
	USB_READ_BLOCK          = 0x10,
};

/* usb_read_async(): Asynchronously read from a USB pipe

   This function is similar to usb_read_sync() but it is asynchronous, ie. it
   returns after starting the read and then runs in the background. Without
   options, usb_read_async() is guaranteed to return quickly without waiting
   for communications. The read itself also never blocks unless there is a
   disagreement with the host on the size of transactions.

   usb_read_async() is also slightly lower-level than usb_read_sync(). In
   particular, it only closes transactions when the data is fully read *and*
   the user buffer is not full. If the user requests exactly the number of
   bytes left in the transaction, the entire contents will be read but the
   transaction will be left in an "active with 0 bytes left" state. Only on the
   next read will the transaction be closed.

   This behavior is designed so that a read closes a transaction if and only if
   it returns fewer bytes than requested, which is useful for detecting the end
   of transfers when their size isn't known in advance.

   This function returns a preliminary error code. If it is zero, then the
   callback will be invoked later with *rc set to the final return value of the
   read, which is itself either an error (< 0) or the number of bytes read.

   Due to its low-level nature, raw usb_read_async() is a bit impractical to
   use and almost always requires repeated calls and callback waits. The
   following flags are provided to make it more convenient by incorporating
   features of usb_read_sync():

   * USB_READ_IGNORE_ZEROS causes usb_read_async() to ignore reads of 0 bytes
     from transactions that were exhausted but not closed.
   * USB_READ_AUTOCLOSE causes usb_read_async() to always close transactions
     when exhausted even if the read is not partial. This is useful when the
     size of the transaction is in fact known in advance.
   * USB_READ_WAIT causes usb_read_async() to wait for the end of the transfer
     before returning. Unless there is a driver bug or USB_READ_BLOCK is also
     set, this cannot cause the function to freeze since the read will always
     complete in finite time (returning USB_READ_IDLE if no transaction is
     active). When USB_READ_WAIT it set, `callback` and `rc` are ignored; the
     callback is not invoked, and the status of the entire read is returned.
   * USB_READ_BLOCK causes usb_read_async() to wait for a transaction if none
     is active. This can stall the transfer indefinitely if the host doesn't
     transmit any data, and freeze the call entirely if USB_READ_WAIT is set.
     This option really makes usb_read_async() a synchronous function.
   * A timeout is supported (pass NULL to ignore).

   With all options enabled, usb_read_async() becomes functionally identical to
   usb_read_sync_timeout(). */
int usb_read_async(int pipe, void *data, int size, int flags, int *rc,
	timeout_t const *timeout, gint_call_t callback);

/* usb_read_cancel(): Cancel an asynchronous read

   Once started, an async read will run in the background and keep writing to
   the provided buffer. This function cancels the operation so that no further
   writes to the buffer are made and the associated memory can be safely
   deallocated. */
void usb_read_cancel(int pipe);

//---
// USB debugging functions
//---

/* usb_set_log(): Set the logging function for the USB driver */
void usb_set_log(void (*logger)(char const *format, va_list args));
/* usb_get_log(): Get the logging function */
void (*usb_get_log(void))(char const *format, va_list args);
/* usb_log(): Send a message to the USB log */
void usb_log(char const *format, ...);

/* usb_set_trace(): Set the tracing function for the USB driver
   The function is called atomically, thus cannot be interrupted, therefore it
   is safe to call usb_trace() in interrupt handlers. */
void usb_set_trace(void (*tracer)(char const *message));
/* usb_get_trace(): Get the tracing function */
void (*usb_get_trace(void))(char const *message);
/* usb_trace(): Trace the current state of the driver */
void usb_trace(char const *message);

#ifdef GINT_USB_DEBUG
#define USB_LOG(...) usb_log(__VA_ARGS__)
#define USB_TRACE(...) usb_trace(__VA_ARGS__)
#else
#define USB_LOG(...) do {} while(0)
#define USB_TRACE(...) do {} while(0)
#endif

//---
// Standard descriptors
//---

/* Descriptor types */
enum {
	USB_DC_DEVICE                     = 1,
	USB_DC_CONFIGURATION              = 2,
	USB_DC_STRING                     = 3,
	USB_DC_INTERFACE                  = 4,
	USB_DC_ENDPOINT                   = 5,
	USB_DC_DEVICE_QUALIFIER           = 6,
	USB_DC_OTHER_SPEED_CONFIGURATION  = 7,
	USB_DC_INTERFACE_POWER            = 8,
};

/* Standard DEVICE descriptor */
typedef struct {
	uint8_t bLength; /* = 18 */
	uint8_t bDescriptorType; /* = USB_DC_DEVICE */
	uint16_t bcdUSB;

	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;

	uint16_t idVendor;
	uint16_t idProduct;

	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;

	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;

} GPACKED(2) usb_dc_device_t;

/* Standard CONFIGURATION descriptor */
typedef struct {
	uint8_t bLength; /* = 9 */
	uint8_t bDescriptorType; /* = USB_DC_CONFIG */
	uint16_t wTotalLength;

	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;

	uint8_t bmAttributes;
	uint8_t bMaxPower;

} GPACKED(1) usb_dc_configuration_t;

/* Standard INTERFACE descriptor */
typedef struct {
	uint8_t bLength; /* = 9 */
	uint8_t bDescriptorType; /* = USB_DC_INTERFACE */
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;

	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;

	uint8_t iInterface;

} GPACKED(1) usb_dc_interface_t;

/* Standard ENDPOINT descriptor */
typedef struct
{
	uint8_t bLength; /* = 7 */
	uint8_t bDescriptorType; /* = USB_DC_ENDPOINT */
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;

	uint16_t wMaxPacketSize;
	uint8_t bInterval;

} GPACKED(1) usb_dc_endpoint_t;

/* Standard STRING descriptor */
typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType; /* = USB_DC_STRING */
	uint16_t data[];

} GPACKED(2) usb_dc_string_t;

/* usb_dc_string(): Create a STRING descriptor and return its ID

   This function registers the provided string in an array of STRING
   descriptors used to answer GET_DESCRIPTOR requests, and returns the string's
   ID. USB 2.0 only has provision for 256 strings in the device, so this
   function will return 0 when out of space.

   The string should be encoded as UTF-16 (big-endian), which can be achieved
   with a "u" prefix on the string literal, for instance: u"Hello,World!". If
   (len) is specified, it should be the number of UTF-16 code points to count
   in the string. If it is 0, it defaults to the length of the string. */
uint16_t usb_dc_string(uint16_t const *literal, size_t len);

/* usb_dc_string_get(): Get the descriptor for a STRING id
   This is mostly used by the driver to answer GET_DESCRIPTOR requests. */
usb_dc_string_t *usb_dc_string_get(uint16_t id);

//---
// USB interrupts
//---

/* usb_interrupt_context: Context of the function interrupted by a USB interrupt
   The pointer is set back to NULL when the interrupt is finished being handled. */
extern gint_inth_callback_context_t* usb_interrupt_context;

#ifdef __cplusplus
}
#endif

#endif /* GINT_USB */
