//---
// gint:serial - Serial communication
//---

// TODO: This is a template for a future implementation.

#ifndef GINT_SERIAL
#define GINT_SERIAL

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/defs/call.h>

typedef enum {
	SERIAL_PARITY_NONE,
	SERIAL_PARITY_EVEN,
	SERIAL_PARITY_ODD,

} GPACKEDENUM serial_parity_t;

typedef struct {
	/* Baud rate (valid options are 300, 600, 1200, 2400, 4800, 9600,
	   19200, 38400, 57600 and 115200) */
	int baudrate;
	/* Parity */
	serial_parity_t parity;
	/* Data width (valid options are 7 and 8) */
	uint8_t data_width;
	/* Stop bits (valid options are 1 and 2) */
	uint8_t stop_bits;

} serial_config_t;

// TODO: Error management...

int serial_open(serial_config_t const *config);

bool serial_is_open(void);

void serial_write_async(void const *data, size_t size, gint_call_t callback);

void serial_write_sync(void const *data, size_t size);

void serial_read_async(void const *data, size_t size, gint_call_t callback);

void serial_read_sync(void const *data, size_t size);

// Waits for communications to finish
void serial_wait(void);

// Calls serial_wait() automatically
void serial_close(void);

// TODO: Info on how much data is pending in each buffer?
// Ultimately we'll bind this to a file descriptor so we can't really know.

#ifdef __cplusplus
}
#endif

#endif /* GINT_SERIAL */
