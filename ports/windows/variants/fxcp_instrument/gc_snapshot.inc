/* Included inside gc.c: inspect allocation metadata, never arbitrary object pointers. */
#ifdef PE_HOST_INSTRUMENT
#include "py/objarray.h"
unsigned pe_gc_blocks(uint32_t *out, unsigned capacity) {
    unsigned count=0;
    GC_ENTER();
    for(mp_state_mem_area_t *area=&MP_STATE_MEM(area);area;area=NEXT_AREA(area)) {
        size_t limit=(area->gc_pool_end-area->gc_pool_start)/BYTES_PER_BLOCK;
        for(size_t block=0;block<limit;block++) {
            unsigned kind=ATB_GET_KIND(area,block);
            if(kind!=AT_HEAD && kind!=AT_MARK) {continue;}
            size_t n=1;
            while(block+n<limit && ATB_GET_KIND(area,block+n)==AT_TAIL) {n++;}
            unsigned slot=count;
            if(count==capacity) {
                slot=0;
                for(unsigned i=1;i<count;i++) {if(out[i*2+1]<out[slot*2+1]) {slot=i;}}
            }
            if(count<capacity || n*BYTES_PER_BLOCK>out[slot*2+1]) {
                out[slot*2]=(uint32_t)(uintptr_t)(area->gc_pool_start+block*BYTES_PER_BLOCK);
                out[slot*2+1]=n*BYTES_PER_BLOCK;
                const void *ptr=(void *)(uintptr_t)out[slot*2];
                uintptr_t tag=*(const uintptr_t *)ptr;
                unsigned type=0;
                if(tag==(uintptr_t)&mp_type_bytearray) {type=1;}
                else if(tag==(uintptr_t)&mp_type_bytes) {type=2;}
                else if(tag==(uintptr_t)&mp_type_str) {type=3;}
                else if(tag==(uintptr_t)&mp_type_list) {type=4;}
                else if(tag==(uintptr_t)&mp_type_dict) {type=5;}
                else if(tag==(uintptr_t)&mp_type_tuple) {type=6;}
                else if(tag==(uintptr_t)&mp_type_memoryview) {type=7;}
                else if(tag==(uintptr_t)&mp_type_array) {type=8;}
                out[512+slot]=type;
                memset((char *)out+4096+slot*32,0,32);
                memcpy((char *)out+4096+slot*32,ptr,n*BYTES_PER_BLOCK<32?n*BYTES_PER_BLOCK:32);
                if(count<capacity) {count++;}
            }
            block+=n-1;
        }
    }
    GC_EXIT();
    return count;
}
#endif
