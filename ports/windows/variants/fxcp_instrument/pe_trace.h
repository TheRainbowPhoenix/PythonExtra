#ifndef PE_TRACE_H
#define PE_TRACE_H
#include <stdint.h>
/* Fixed-size, allocation-free SPSC records. UI work never runs in the VM. */
void pe_trace(unsigned kind, int value, int extra, const char *name);
uint64_t pe_wall_us(void);
void pe_trace_frame(unsigned kind, const void *state);
void pe_heap_sample(void);
void pe_debug_service(void);
void pe_wait_start(void);
#endif
