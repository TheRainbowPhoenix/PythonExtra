#ifndef PE_HOST_H
#define PE_HOST_H
#include <stdint.h>
#include <stdbool.h>
#define PE_W 320
#define PE_H 528
#define PE_PIXELS (PE_W*PE_H)
#define PE_HEADER 4096
#define PE_EVENTS (PE_HEADER+PE_PIXELS*2)
#define PE_CAPACITY 2048
#define PE_MAP_SIZE 1048576
enum { H_MAGIC, H_VERSION, H_W, H_H, H_FORMAT, H_PID, H_STATUS, H_SEQ,
 H_FRAME, H_CHECKPOINT, H_TIME_LO, H_TIME_HI, H_WRITE, H_READ, H_OVERFLOW,
 H_RELEASE, H_STOP, H_FLAGS, H_FRAME_US, H_GC_REQUEST, H_GC_ACK,
 H_ALLOC, H_FREE, H_DRAWS, H_PIXELS_LO, H_PIXELS_HI, H_POLLS, H_EVENT_TYPE,
 H_RESERVED, H_STATE_N, H_HEARTBEAT, H_EXIT, H_OWNER, H_BLIT_WARNINGS };
typedef struct {uint32_t lo,hi,type,key; int32_t x,y;} pe_event;
extern volatile uint32_t *pe_h;
extern uint16_t pe_vram[PE_PIXELS];
void pe_init(void);
void *pe_mapping(void);
bool pe_virtual(void);
uint64_t pe_clock_us(void);
void pe_delay_us(uint64_t us);
void pe_publish(void);
void pe_present(void);
void pe_checkpoint(unsigned count, const int32_t *state);
void pe_vm_hook(void);
bool pe_poll(pe_event *event);
void pe_draw_count(unsigned pixels);
void pe_pixel_count(unsigned pixels);
#endif
