//---
// gint:usb-ff-bulk - A bulk-based transfer class using the fxlink protocol
//
// This interface (class code 0xff/0x77) implements a simple bidirectional
// communication channel running the fxlink protocol. It basically talks to
// fxlink's interactive mode on the host, and can broadly be used for 4 things:
//
// 1. Sending standard messages to fxlink (screenshots, logs...). Convenient
//    functions are provided in this header to minimize the overhead. fxlink
//    running in interactive mode (fxlink -t) will handle these messages in a
//    suitable way automatically (eg. save screenshots as PNG).
//
// 2. Sending custom messages to fxlink (bench results, memory dumps, crash
//    reports...). fxlink running in interactive mode will forward the data to
//    appropriate user-provided external programs to handle the messages.
//
// 3. Applications based around on data streams coming from the host (screen
//    projector, web browser...). This can be achieved by modifying fxlink or
//    using it as a library.
//
// 4. Remote control from the fxlink interactive mode (fxlink -t). This is
//    based on fxlink's 'command' and 'keyboard' protocols. This module will
//    automatically handle commands and respond to them as long as the message
//    handling function is called regularly.
//---

#ifndef GINT_USB_FF_BULK
#define GINT_USB_FF_BULK

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/usb.h>
#include <gint/config.h>
struct usb_fxlink_header;

/* This bulk transfer interface with class code 0xff/0x77 implements a simple
   bidirectional communication channel between the calculator and a host,
   running at high-speed USB 2.0 and using the fxlink protocol. You can use it
   to communicate with fxlink's interactive mode on the host. */
extern usb_interface_t const usb_ff_bulk;

//---
// Sending standard messages
//---

/* usb_fxlink_text(): Send raw text
   Send a string; fxlink will display it in the terminal. This can be used to
   back stdout/stderr. Sending lots of small messages can be slow; if that's a
   problem, fill in message manually. If size is 0, uses strlen(text). */
void usb_fxlink_text(char const *text, int size);

/* usb_fxlink_screenshot(): Take a screenshot

   This function sends a copy of the VRAM to fxlink. This is best used just
   before dupdate() since this ensures the image sent by USB is identical to
   the one displayed on screen.

   If `onscreen` is set and there are two VRAMs (on fx-CG or when using the
   gray engine on fx-9860G), sends a copy of the other VRAM. This is a bit more
   intuitive when taking a screenshot of the last shown image as a result of a
   key press. Note that this function never reads pixels directly from the
   display (it's usually slow and currently not even implemented). */
void usb_fxlink_screenshot(bool onscreen);

/* usb_fxlink_videocapture(): Send a frame for a video recording

   This function is essentially the same as usb_fxlink_screenshot(). It sends a
   capture of the VRAM to fxlink but uses the "video" type, which fxlink
   displays in real-time or saves as a video file. The meaning of the onscreen
   setting is identical to usb_fxlink_screenshot().

   This function can be called with onscreen=false as a dupdate() hook to
   automatically send new frames to fxlink. */
void usb_fxlink_videocapture(bool onscreen);

#if GINT_RENDER_MONO
/* Similar to usb_fxlink_screenshot(), but takes a gray screenshot if the gray
   engine is currently running. */
void usb_fxlink_screenshot_gray(bool onscreen);

/* Like usb_fxlink_videocapture(), but uses VRAM data from the gray engine. */
void usb_fxlink_videocapture_gray(bool onscreen);
#endif

//---
// Receiving messages
//---

/* usb_fxlink_handle_messages(): Process and return incoming messages

   This function processes incoming messages. It handles some types of messages
   internally (eg. fxlink commands) and returns the others to the caller by
   loading the provided header structure and returning true.

   When this function returns true, the caller should read the message contents
   on the interface's input pipe. Usually every read will return just a portion
   of the message (eg. 2048 bytes) so the contents should be read with a loop.

   As long as this function returns true it should be called againt to process
   any other messages. When there is no more messages to process, this function
   returns false. It is important to call this function regularly from the main
   thread when quick responses are expected. */
bool usb_fxlink_handle_messages(struct usb_fxlink_header *header);

/* usb_fxlink_drop_transaction(): Drop incoming data until end of transaction

   When a message arrives on the USB port incoming data *must* be processed in
   order for the pipe to accept any further data in the future. This function
   can be used in error situations to drop data until the end of the
   transaction (which is usually the end of the message, at least with the
   fxSDK's implementation of fxlink). */
void usb_fxlink_drop_transaction(void);

/* usb_fxlink_set_notifier(): Set up a notification for incoming messages

   Registers the provided function to be called when data arrives on the fxlink
   input pipe. This signals that usb_fxlink_handle_messages() should be called
   later from the main thread. Pass NULL to disable the notification. */
void usb_fxlink_set_notifier(void (*notifier_function)(void));

//---
// Direct bulk access
//
// The following functions can be used to access the bulk pipes directly. They
// provide the pipe numbers, which can be passed for instance to usb_write
// functions.
//---

/* usb_ff_bulk_output(): Pipe number for calculator -> host communication */
int usb_ff_bulk_output(void);

/* usb_ff_bulk_input(): Pipe number for host -> calculator communication */
int usb_ff_bulk_input(void);

//---
// Construction and analysis of fxlink protocol messages
//
// fxlink messages consist of a simple header followed by message contents
// (sometimes with a subheader). In addition to the functions above, it is
// possible to craft custom fxlink messages and send them manually.
//
// To send a message manually, simple write an fxlink header to the output
// pipe, followed by the contents. The message can be built from any number of
// writes to the pipe. After the last write, commit the pipe.
//---

/* usb_fxlink_header_t: Message header for fxlink

   Messages are categorized with an (application, type) pair; both are UTF-8
   strings of up to 16 bytes (NUL not required). The application name "fxlink"
   is reserved for built-in types of messages. Other names can be used freely,
   and fxlink's interactive mode on the host side will forward messages to
   external programs based on the application name.

   The size of the data to be transferred must be specified upfront, but this
   restriction might be lifted in the future. As with the rest of the USB
   protocol, all the integer are encoded as *little-endian*. */
typedef struct usb_fxlink_header
{
	/* Protocol version = 0x00000100 */
	uint32_t version;
	/* Size of the data to transfer (excluding this header) */
	uint32_t size;
	/* Size of individual transfers (related to the size of the FIFO) */
	uint32_t transfer_size;
	/* Application name and message type, UTF-8 (need not be zero-terminated) */
	char application[16];
	char type[16];

} usb_fxlink_header_t;

/* usb_fxlink_fill_header(): Fill an fxlink message header

   Fills an fxlink header's fields with the provided. You need to specify the
   exact amount of data that the fxlink message will contain. Returns false if
   parameters are invalid or don't fit the fields. */
bool usb_fxlink_fill_header(usb_fxlink_header_t *header,
	char const *application, char const *type, uint32_t data_size);

/* Subheader for the fxlink built-in "image" type */
typedef struct
{
   /* Image size; storage is row-major */
	uint32_t width;
	uint32_t height;
	/* Pixel format, see below */
	int pixel_format;

} usb_fxlink_image_t;

/* Pixel formats */
enum {
	/* Image is an array of *big-endian* uint16_t with RGB565 format */
	USB_FXLINK_IMAGE_RGB565 = 0,
	/* Image is an array of bits in mono format */
	USB_FXLINK_IMAGE_MONO,
	/* Image is two consecutive mono arrays, one for light, one for dark */
	USB_FXLINK_IMAGE_GRAY,
};

#ifdef __cplusplus
}
#endif

#endif /* GINT_USB_FF_BULK */
