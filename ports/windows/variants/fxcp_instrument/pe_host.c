#include "pe_host.h"
#include "py/runtime.h"
#include "py/gc.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "pe_trace.h"
#include "py/profile.h"
#include "py/objfun.h"
#include <stdio.h>
static uint32_t local_header[PE_HEADER/4];
volatile uint32_t *pe_h = local_header;
uint16_t pe_vram[PE_PIXELS];
static void *mapping;
static HANDLE file_handle, map_handle, owner_handle;
static HANDLE run_event, ready_event;
static uint64_t virtual_us, epoch_ms, pixels;
static bool initialized;
static unsigned checks;
static void service(void);
void pe_wait_start(void) {
    pe_init();
    if(!mapping) {return;}
    volatile uint32_t *gate=(volatile uint32_t *)((char *)mapping+1236);
    while(*gate) {service();Sleep(5);}
}
unsigned pe_gc_blocks(uint32_t *out, unsigned capacity);
void pe_debug_service(void) {
    if(!mapping) {return;}
    volatile uint32_t *p=(volatile uint32_t *)((char *)mapping+1192);
    if(p[0]==p[1]) {return;}
    uint32_t request=p[0];
    p[2]=pe_gc_blocks((uint32_t *)((char *)mapping+900000),256);
    pe_heap_sample(); p[3]=1; MemoryBarrier(); p[1]=request;
    while(p[4]!=request) {
        if(pe_h[H_STOP]) {mp_raise_type(&mp_type_SystemExit);}
        if(owner_handle && WaitForSingleObject(owner_handle,0)==WAIT_OBJECT_0) {ExitProcess(125);}
        Sleep(5);
    }
    p[3]=0;
}
void *pe_mapping(void) {pe_init();return mapping;}
uint64_t pe_wall_us(void) {
    LARGE_INTEGER now; static LARGE_INTEGER frequency;
    QueryPerformanceCounter(&now); if(!frequency.QuadPart) {QueryPerformanceFrequency(&frequency);}
    return (uint64_t)(now.QuadPart / frequency.QuadPart)*1000000 +
        (uint64_t)(now.QuadPart % frequency.QuadPart)*1000000/frequency.QuadPart;
}
static mp_obj_t pe_time_monotonic(void) {
    pe_init();
    uint64_t us=pe_virtual()?pe_clock_us():pe_wall_us();
    return mp_obj_new_float((mp_float_t)us/1000000);
}
MP_DEFINE_CONST_FUN_OBJ_0(pe_time_monotonic_obj,pe_time_monotonic);
void pe_trace(unsigned kind, int value, int extra, const char *name) {
    /* Header 1024: producer, consumer, drops. 1024 x 256-byte records. */
    if(!mapping) {return;}
    if(mp_prof_is_executing) {
        if(kind<16) {((volatile uint32_t *)((char *)mapping+1120))[kind]++;}
        return;
    }
    if(kind>=6 && kind<=9 && value>=3) {
        const mp_code_state_t *frame=MP_STATE_THREAD(current_code_state);
        pe_trace(17,0,0,NULL);
        for(unsigned i=0;frame && i<16;i++,frame=frame->prev_state) {
            const mp_raw_code_t *rc=frame->fun_bc->rc;
            char label[236];
            snprintf(label,sizeof(label),"%s|%s",qstr_str(MP_CODE_QSTR_MAP(frame->fun_bc->context,0)),
                qstr_str(MP_CODE_QSTR_MAP(frame->fun_bc->context,rc->prelude.qstr_block_name_idx)));
            unsigned line=mp_prof_bytecode_lineno(rc,frame->ip-rc->prelude.opcodes);
            pe_trace(16,i,line,label);
        }
        if(kind!=9) {
            volatile uint32_t *slow=(volatile uint32_t *)((char *)mapping+1220);
            uint64_t delay=slow[0];
            if(slow[1] && (kind==7 || kind==8) && extra>0) {delay+=(uint64_t)extra*1000000/slow[1];}
            if(delay) {pe_delay_us(delay);}
        }
    }
    if(kind<16) {((volatile uint32_t *)((char *)mapping+1120))[kind]++;}
    bool storage=(kind>=6 && kind<=9) || kind==16 || kind==17;
    unsigned capacity=storage?512:1024;
    unsigned offset=storage?664000:400000;
    volatile uint32_t *control=(volatile uint32_t *)((char *)mapping+(storage?1240:1024));
    uint32_t write=control[0], read=control[1];
    static char discard[256];
    bool full=write-read>=capacity;
    char *record=full?discard:(char *)mapping+offset+(write%capacity)*256;
    uint64_t stamp=pe_wall_us();
    memcpy(record,&stamp,8); memcpy(record+8,&kind,4);
    memcpy(record+12,&value,4); memcpy(record+16,&extra,4);
    memset(record+20,0,236);
    if(name) {strncpy(record+20,name,235);}
    MemoryBarrier(); if(full) {control[2]++;} else {control[0]=write+1;}
}
void pe_heap_sample(void) {
    gc_info_t info; gc_info(&info);
    pe_h[H_SEQ]++; MemoryBarrier();
    pe_h[H_ALLOC]=info.used; pe_h[H_FREE]=info.free;
    pe_h[H_RESERVED]=info.max_free*MICROPY_BYTES_PER_GC_BLOCK;
    MemoryBarrier(); pe_h[H_SEQ]++;
}
void pe_trace_frame(unsigned kind, const void *state) {
    if(!mapping || mp_prof_is_executing) {return;}
    const mp_code_state_t *code=state;
    const mp_raw_code_t *rc=code->fun_bc->rc;
    char label[160];
    snprintf(label,sizeof(label),"%s|%s",qstr_str(MP_CODE_QSTR_MAP(code->fun_bc->context,0)),
        qstr_str(MP_CODE_QSTR_MAP(code->fun_bc->context,rc->prelude.qstr_block_name_idx)));
    pe_trace(kind,rc->line_of_definition,0,label);
}

static void close_mapping(void) {
    if (!initialized) {return;}
    pe_h[H_STATUS]=3;
    MemoryBarrier();
    if(mapping) {UnmapViewOfFile(mapping);}
    if(map_handle) {CloseHandle(map_handle);}
    if(file_handle) {CloseHandle(file_handle);}
    if(owner_handle) {CloseHandle(owner_handle);}
    if(run_event) {CloseHandle(run_event);}
    if(ready_event) {CloseHandle(ready_event);}
}
void pe_init(void) {
    if(initialized) {return;}
    initialized=true;
    epoch_ms=GetTickCount();
    wchar_t path[32768];
    if(GetEnvironmentVariableW(L"PE_MAP",path,32768)) {
        file_handle=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
        if(file_handle==INVALID_HANDLE_VALUE) {exit(120);}
        LARGE_INTEGER size;
        if(!GetFileSizeEx(file_handle,&size) || size.QuadPart!=PE_MAP_SIZE) {exit(121);}
        map_handle=CreateFileMappingW(file_handle,NULL,PAGE_READWRITE,0,PE_MAP_SIZE,NULL);
        if(!map_handle) {exit(122);}
        mapping=MapViewOfFile(map_handle,FILE_MAP_ALL_ACCESS,0,0,PE_MAP_SIZE);
        if(!mapping) {exit(123);}
        pe_h=(volatile uint32_t *)mapping;
        if(pe_h[H_MAGIC]!=0x50454931 || pe_h[H_VERSION]!=1) {exit(124);}
        if(pe_h[H_OWNER]) {owner_handle=OpenProcess(SYNCHRONIZE,FALSE,pe_h[H_OWNER]);}
        if(GetEnvironmentVariableW(L"PE_RUN_EVENT",path,32768)) {run_event=OpenEventW(SYNCHRONIZE|EVENT_MODIFY_STATE,FALSE,path);}
        if(GetEnvironmentVariableW(L"PE_READY_EVENT",path,32768)) {ready_event=OpenEventW(SYNCHRONIZE|EVENT_MODIFY_STATE,FALSE,path);}
    }
    pe_h[H_MAGIC]=0x50454931; pe_h[H_VERSION]=1;
    pe_h[H_W]=PE_W; pe_h[H_H]=PE_H; pe_h[H_FORMAT]=565;
    pe_h[H_PID]=GetCurrentProcessId(); pe_h[H_STATUS]=1;
    if(mapping) {uint64_t start=pe_wall_us();memcpy((char *)mapping+1040,&start,8);}
    for(unsigned i=0;i<PE_PIXELS;i++) {pe_vram[i]=0xffff;}
    atexit(close_mapping);
}
static void service(void) {
    if(pe_h[H_STOP]) {mp_raise_type(&mp_type_SystemExit);}
    if(owner_handle && WaitForSingleObject(owner_handle,0)==WAIT_OBJECT_0) {ExitProcess(125);}
    pe_debug_service();
}
bool pe_virtual(void) {pe_init(); return (pe_h[H_FLAGS]&1)!=0;}
uint64_t pe_clock_us(void) {pe_init(); return pe_virtual()?virtual_us:(uint64_t)(uint32_t)(GetTickCount()-(uint32_t)epoch_ms)*1000;}
void pe_delay_us(uint64_t us) {
    pe_init(); service();
    if(pe_virtual()) {virtual_us+=us;} else {Sleep((DWORD)((us+999)/1000));}
}
void pe_draw_count(unsigned count) {
    (void)count;pe_init(); pe_h[H_DRAWS]++;
}
void pe_pixel_count(unsigned count) {pixels+=count;}
static void publish_body(void) {
    if(mapping) {memcpy((uint8_t *)mapping+PE_HEADER,pe_vram,sizeof(pe_vram));}
    uint64_t us=pe_clock_us();
    pe_h[H_TIME_LO]=(uint32_t)us; pe_h[H_TIME_HI]=(uint32_t)(us>>32);
    gc_info_t info; gc_info(&info);
    pe_h[H_ALLOC]=info.used; pe_h[H_FREE]=info.free;
    pe_h[H_RESERVED]=info.max_free*MICROPY_BYTES_PER_GC_BLOCK;
    pe_h[H_PIXELS_LO]=(uint32_t)pixels; pe_h[H_PIXELS_HI]=(uint32_t)(pixels>>32);
}
void pe_publish(void) {
    pe_init(); pe_h[H_SEQ]++; MemoryBarrier();
    publish_body(); MemoryBarrier(); pe_h[H_SEQ]++;
}
void pe_present(void) {
    pe_trace(5,0,0,NULL);
    pe_init(); pe_h[H_SEQ]++; MemoryBarrier();
    pe_h[H_FRAME]++; publish_body(); MemoryBarrier(); pe_h[H_SEQ]++;
}
void pe_checkpoint(unsigned count,const int32_t *state) {
    pe_init();
    uint32_t token=pe_h[H_RELEASE];
    if(count>64) {mp_raise_ValueError(MP_ERROR_TEXT("at most 64 checkpoint integers"));}
    pe_h[H_SEQ]++; MemoryBarrier();
    if(mapping && state) {memcpy((uint8_t *)mapping+512,state,count*4);}
    pe_h[H_STATE_N]=count;
    pe_h[H_CHECKPOINT]++;
    publish_body(); MemoryBarrier(); pe_h[H_SEQ]++;
    if(!mapping) {return;}
    MemoryBarrier(); pe_h[H_STATUS]=2;
    if(ready_event) {SetEvent(ready_event);}
    while(pe_h[H_RELEASE]==token) {
        service();
        uint32_t request=pe_h[H_GC_REQUEST];
        if(request!=pe_h[H_GC_ACK]) {
            gc_collect(); pe_publish(); MemoryBarrier(); pe_h[H_GC_ACK]=request;
            if(ready_event) {SetEvent(ready_event);}
        }
        if(run_event) {WaitForSingleObject(run_event,100);} else {Sleep(1);}
    }
    pe_h[H_STATUS]=1;
}
void pe_vm_hook(void) {
    if((++checks&1023)!=0) {return;}
    pe_init(); pe_h[H_HEARTBEAT]++; service();
    static DWORD last_sample;
    DWORD now=GetTickCount();
    if((DWORD)(now-last_sample)>=250) {last_sample=now;pe_heap_sample();}
}
bool pe_poll(pe_event *event) {
    pe_init(); service(); pe_h[H_POLLS]++;
    uint32_t read=pe_h[H_READ], write=pe_h[H_WRITE];
    if(write-read>PE_CAPACITY) {mp_raise_msg(&mp_type_RuntimeError,MP_ERROR_TEXT("input ring corrupted"));}
    if(read==write || !mapping) {return false;}
    MemoryBarrier();
    pe_event *slot=(pe_event *)((uint8_t *)mapping+PE_EVENTS)+(read%PE_CAPACITY);
    uint64_t due=((uint64_t)slot->hi<<32)|slot->lo;
    if(due>pe_clock_us()) {return false;}
    *event=*slot;
    MemoryBarrier(); pe_h[H_READ]=read+1; pe_h[H_EVENT_TYPE]=event->type;
    return true;
}
