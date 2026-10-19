//---
// gint:usb:asyncio - Asynchronous I/O common definitions
//---

#ifndef GINT_USB_ASYNCIO
#define GINT_USB_ASYNCIO

#include <gint/defs/types.h>
#include <gint/defs/call.h>

/* Data tracking the progress of a multi-part multi-round async I/O operation.

   * Writes are multi-part because they are constructed over several calls to
     write(2) followed by a "commit" with fynsc(2). They are additionally
     multi-round because each call to write(2) requires mutiple rounds of
     hardware communication when the hardware buffer is smaller than the data.

   * Reads are multi-part because each transaction from the host requires
     several calls to read(2) ("user segments") if the user's buffer is shorter
     than the full transaction. In addition, reads are multi-round because a
     single read(2) to a user buffer takes multiple rounds of hardware
     communication ("hardware segments") whenever the user buffer is larger
     than the hardware buffer.

   The process of performing such an I/O operation, as tracked by this
   structure and use throughout gint, is as follows.

   ## Writes

                                   WRITING ---------------------.
                                     ^ |                        | HW buffer
                       Start writing | | Not full               | full: start
                                     | |                        | transmission
                     write(2)        | v                        v
      --> IDLE ------------------> PENDING <------------- FLYING-WRITE
           ^                         ^ |   DONE interrupt
           | DONE           write(2) | |
           | interrupt               | |
           |                         | | Data exhausted
           |     fsync(2): start     | v
      FLYING-SYNC <------------ IN-PROGRESS
                   transmission

   Initially the operation is in the IDLE state. When a write(2) is issued, it
   interacts with hardware then transitions to the IN-PROGRESS state, where it
   remains for any subsequent write(2). A fsync(2) will properly commit data to
   the hardware, finish the operation and return to the IDLE state.

   The FLYING-WRITE and FLYING-SYNC states refer to waiting periods, after
   issuing hardware commands, during which hardware communicates. Usually an
   interrupt signals when hardware is ready to resume work.

   Note that in a series of write(2), hardware is only instructed to send data
   once the hardware buffer is full. Therefore, a write(2) might transition
   directly from IDLE or IN-PROGRESS, to PENDING, to IN-PROGRESS, without
   actually communicating with the outside world.

   An asynchronous write(2) might return to the caller as soon as writing is
   finished even if the operation is left in the FLYING-WRITE state, and it may
   even return while the operation is in the WRITING state if the DMA is used.

   The invariants and meaning for each state are as follow:

   State(s)       Characterization             Description
   ============================================================================
   IDLE           type == ASYNCIO_NONE         No I/O operation
   ----------------------------------------------------------------------------
   PENDING        data_w && round_size == 0    Ready to write pending data
   ----------------------------------------------------------------------------
   WRITING,       round_size > 0               CPU/DMA write to HW in progress
   FLYING-WRITE                                HW transmission in progress
   ----------------------------------------------------------------------------
   IN-PROGRESS    !data_w && type == WRITE     Waiting for write(2) or fsync(2)
   ----------------------------------------------------------------------------
   FLYING-SYNC    type == ASYNCIO_SYNC         HW commit in progress
   ============================================================================

   The series of asyncio_op function calls for a write is as follows:

     transaction ::= write* fsync
     write ::= asyncio_op_start_write round+ asyncio_op_finish_write
     round ::= asyncio_op_start_write_round asyncio_op_finish_write_round
     fsync ::= asyncio_op_start_sync asyncio_op_finish_sync

   Each write(2) (with a single user-provided buffer) is split into multiple
   rounds that each fill the (small) hardware buffer. More writes can follow
   until an fynsc(2) commits the pipe.

   ## Reads

                     IN interrupt
     --> IDLE-EMPTY --------------> IDLE-READY
             |  \               read(2) | ^
     read(2) |   \ Transaction          | | User buffer
             |    \ exhausted*          | | filled
             |     '----<----------.    | |
             |                      \   | |
             |       IN interrupt    \  | |
             v   .--------->--------. \ v | .---.  Read from
          WAITING                    READING     v hardware
                 '---------<--------'       '---'
               HW buffer exhausted with
                 user buffer not full

   On this diagram, the right side indicates the presence of data to read from
   hardware while the bottom side indicates a read(2) request by the user.
   Notice the diagonal arrow back to IDLE-EMPTY insteaf of WAITING, which
   highlights that read(2) will always return at the end of a transaction even
   if the user-provided buffer is not full (to avoid waiting).

   A read(2) request (a "user segment") might consume several full hardware
   buffers ("hardware segments") if the user buffer is large, thus looping
   repeatedly between WAITING and READING. Conversely, each hardware segment
   might fulfill many read(2) requests if the user buffer is small, thus
   looping between IDLE-READY and READING.

   * Note that if the transaction finishes right as the user buffer fills up,
   we return to IDLE-READY and the next call to read(2) will successfully read
   0 bytes and transition back to IDLE-EMPTY. This allows the user to read the
   entire transaction by reading data until they get fewer bytes than requested
   without running the risk of blocking on the next transaction.

   The invariants and meaning for each state are as follow. The right side
   (presence of a hardware segment) is indicated by `buffer_used >= 0`, while
   the bottom side (presence of a read(2) request) is indicated by `size > 0`.
   As an exception, IDLE-EMPTY uses `buffer_used == 0` so that the default
   zero-initialization of transfer data is sufficient.

   State          Invariant characterization   Description
   ============================================================================
   IDLE-EMPTY     type == ASYNCIO_NONE         No I/O operation, the pipe is
                  && buffer_used == 0          idle with no request and no
                  && size == 0                 hardware segment (but we might
                  && round_size == 0           still be mid-transaction)
   ----------------------------------------------------------------------------
   IDLE-READY     type == ASYNCIO_READ         There is a hardware segment not
                  && buffer_used >= 0          marked as complete, but no
                  && size == 0                 read(2) request to consume it
                  && round_size == 0
   ----------------------------------------------------------------------------
   WAITING        type == ASYNCIO_READ         There is a read(2) request but
                  && buffer_used < 0           no hardware segment, and either
                  && size > 0                  the request is new or the
                  && round_size == 0           transaction isn't exhausted
   ----------------------------------------------------------------------------
   READING        type == ASYNCIO_READ         A read round in progress and
                  && buffer_used >= 0          either the read(2) request or
                  && size > 0                  hardware segment will be
                  && round_size > 0            exhausted when it ends
   ============================================================================

   The series of asyncio_op function calls for a read is a bit more complicated
   because transactions are divided into two non-comparable sequences of
   segments: one for packets received by the hardware buffer (on BRDY), one for
   the data being copied to user buffers (on read(2)).

   |<------ Transaction from the host (no size limit) ------>|

   v BRDY      v BRDY      v BRDY      v BRDY      v BRDY
   +-----------+-----------+-----------+-----------+---------+
   | HW buffer | HW buffer | HW buffer | HW buffer | (short) |  HW segments
   +-----------+------+----+-----------+-----------+---------+
   | R1        | R2   | R3 | R4        | R5        | R6      |  Read rounds
   +-----------+------+----+-----------+-----------+---------+
   | User buffer #1   |  User buffer #2            | (short) |  User segments
   +------------------+----------------------------+---------+
   ^ read(2)          ^ read(2)                    ^ read(2)

   Reads rounds are exactly the intersections between hardware segments and
   read(2) user segments.

   States can be checked and transitioned with the API functions below. */

enum { ASYNCIO_NONE, ASYNCIO_READ, ASYNCIO_WRITE, ASYNCIO_SYNC };

typedef volatile struct
{
    /* Type of I/O operation (NONE/WRITE/SYNC/READ) */
    uint8_t type;
    /* Whether the DMA should be used for hardware access */
    bool dma            :1;
    /* For reading pipes, whether the transaction is expected to continue with
       another hardware segment after the current one */
    bool cont_r         :1;
    /* For reading pipes, interrupt flag signaling an incoming hardware segment
       not yet added to the operation */
    bool interrupt_r    :1;
    /* For reading pipes, whether the current read call should close the
       current hardware segment if all the data is read even if the read call
       is not partial */
    bool autoclose_r    :1;
    /* Hardware resource being used for access (meaning depends on hardware).
       Usually, this is assigned for the duration of hardware transaction.
       This value is user-managed and not modified by asyncio_op functions. */
    uint8_t controller;

    /* Number of bytes in short buffer (0..3) */
    uint8_t shbuf_size;
    /* Short buffer */
    uint32_t shbuf;

    /* Size of data currently in the hardware buffer */
    int16_t buffer_used;
    /* Size of data being read/written in the current round (which may itself
       be asynchronous if it's using the DMA) */
    uint16_t round_size;

    union {
        /* Address of data to transfer, incremented gradually [write] */
        void const *data_w;
        /* Address of buffer to store data to, incremented gradually [read] */
        void *data_r;
    };
    /* Size of data left to transfer to satisfy the complete request */
    int size;
    /* For reading operations, pointer to total amount of transferred data */
    int *realized_size_r;
    /* Callback at the end of the current write, final commit, or read */
    gint_call_t callback;

} asyncio_op_t;

//---
// Initialization and query functions
//---

/* asyncio_op_clear(): Initialize/clear the storage for an I/O operation */
void asyncio_op_clear(asyncio_op_t *op);

/* asyncio_op_busy(): Check whether the transfer is busy for syscalls

   This function checks whether the transfer is in a state where the CPU is
   busy wrt. starting a new syscall, ie. read(2), write(2) or fsync(2). Returns
   true if the CPU is busy and the call has to wait, false if the call can
   proceed immediately. */
bool asyncio_op_busy(asyncio_op_t const *op);

//---
// I/O functions
//---

/* Start/finish a write(2) call. */
void asyncio_op_start_write(asyncio_op_t *op,
    void const *data, size_t size, bool use_dma, gint_call_t const *callback);
void asyncio_op_finish_write(asyncio_op_t *op);

/* Start/finish a single-block write to hardware. */
void asyncio_op_start_write_round(asyncio_op_t *op, size_t size);
void asyncio_op_finish_write_round(asyncio_op_t *op);

/* Start an fsync(2) operation (after one or more writes) and finish it. */
void asyncio_op_start_sync(asyncio_op_t *op, gint_call_t const *callback);
void asyncio_op_finish_sync(asyncio_op_t *op);

/* Start a read(2) call. The call will finish automatically when the final
   round finishes. If `autoclose` is set, the current hardware segment will
   be marked as completed if the round reads it entirely, even if the request
   is fulfilled. */
void asyncio_op_start_read(asyncio_op_t *op, void *data, size_t size,
    bool use_dma, int *realized_size, bool autoclose,
    gint_call_t const *callback);

/* Start a hardware segment. `cont` should be true if there will be another
   segment in the same transaction. The segment will finish automatically when
   it is completely consumed by a read round. */
void asyncio_op_start_read_hwseg(asyncio_op_t *op, size_t size, bool cont);

bool asyncio_op_has_read_call(asyncio_op_t const *op);

bool asyncio_op_has_read_hwseg(asyncio_op_t const *op);

/* Start a single-block read from hardware. The requested size is automatically
   t->size, however the round may of course be smaller depending on how much
   data is available. Returns the round size. */
int asyncio_op_start_read_round(asyncio_op_t *op);

enum {
    ASYNCIO_HWSEG_EXHAUSTED = 0x01,
    ASYNCIO_REQUEST_FINISHED = 0x02,
    ASYNCIO_TRANSACTION_EXHAUSTED = 0x04,
};

/* Finish a single-block read from hardware. This function also finishes the
   current hardware segment and read call if appropriate, *except* that it
   doesn't invoke the read(2) callback. You should make a copy of it before
   calling and invoke it manually after. Returns a combination of the above
   flags indicating what finished along with the round. */
int asyncio_op_finish_read_round(asyncio_op_t *op);

/* Cancel a read call. This keeps the hardware segment part intact. */
void asyncio_op_cancel_read(asyncio_op_t *op);

#endif /* GINT_USB_ASYNCIO */
