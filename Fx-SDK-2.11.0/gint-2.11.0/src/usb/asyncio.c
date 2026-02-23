#include <gint/drivers/asyncio.h>
#include <string.h>
#include <assert.h>

void asyncio_op_clear(asyncio_op_t *op)
{
    memset((void *)op, 0, sizeof *op);
}

bool asyncio_op_busy(asyncio_op_t const *op)
{
    /* WAITING and READING states are busy (ie. read(2) call in progress) */
    if(op->type == ASYNCIO_READ)
        return op->size > 0;
    /* WRITING, FLYING-WRITE and PENDING states are busy */
    if(op->type == ASYNCIO_WRITE)
        return op->data_w != NULL;
    /* FLYING-COMMIT state is busy */
    if(op->type == ASYNCIO_SYNC)
        return true;

    return false;
}

void asyncio_op_start_write(asyncio_op_t *op, void const *data, size_t size,
    bool use_dma, gint_call_t const *callback)
{
    op->type = ASYNCIO_WRITE;
    op->dma = use_dma;
    op->data_w = data;
    op->size = size;
    op->callback = *callback;
}

void asyncio_op_finish_write(asyncio_op_t *op)
{
    gint_call(op->callback);

    /* Keep relevant states until the transaction finishes with an fsync(2) */
    op->dma = false;
    op->data_w = NULL;
    op->size = 0;
    op->callback = GINT_CALL_NULL;
    op->round_size = 0;
}

void asyncio_op_start_write_round(asyncio_op_t *op, size_t size)
{
    op->round_size = size;
}

void asyncio_op_finish_write_round(asyncio_op_t *op)
{
    op->buffer_used += op->round_size;
    op->data_w += op->round_size;
    op->size -= op->round_size;
    op->round_size = 0;
}

void asyncio_op_start_sync(asyncio_op_t *op, gint_call_t const *callback)
{
    if(op->type != ASYNCIO_WRITE)
        return;

    op->type = ASYNCIO_SYNC;
    op->callback = *callback;
}

void asyncio_op_finish_sync(asyncio_op_t *op)
{
    gint_call(op->callback);
    asyncio_op_clear(op);
}

void asyncio_op_start_read(asyncio_op_t *op, void *data, size_t size,
    bool use_dma, int *realized_size, bool autoclose,
    gint_call_t const *callback)
{
    assert(!asyncio_op_has_read_call(op) && size > 0 && !op->round_size);

    op->type = ASYNCIO_READ;
    op->dma = use_dma;
    op->autoclose_r = autoclose;
    op->data_r = data;
    op->size = size;
    op->realized_size_r = realized_size;
    op->callback = *callback;

    if(realized_size)
        *realized_size = 0;
}

bool asyncio_op_has_read_call(asyncio_op_t const *op)
{
    /* WAITING and READING states */
    return (op->type == ASYNCIO_READ) && (op->size > 0);
}

bool asyncio_op_has_read_hwseg(asyncio_op_t const *op)
{
    /* IDLE-READY and READING states */
    return (op->type == ASYNCIO_READ) && (op->buffer_used >= 0);
}

void asyncio_op_start_read_hwseg(asyncio_op_t *op, size_t size, bool cont)
{
    op->type = ASYNCIO_READ;
    op->buffer_used = size;
    op->cont_r = cont;
}

int asyncio_op_start_read_round(asyncio_op_t *op)
{
    op->round_size = (op->size < op->buffer_used ? op->size : op->buffer_used);
    return op->round_size;
}

int asyncio_op_finish_read_round(asyncio_op_t *op)
{
    int status = 0;

    if(op->realized_size_r)
        *op->realized_size_r += op->round_size;
    op->buffer_used -= op->round_size;
    if(op->data_r)
        op->data_r += op->round_size;
    op->size -= op->round_size;
    op->round_size = 0;

    bool read_fulfilled = (op->size == 0);
    bool hwseg_exhausted =
        (op->buffer_used == 0 && (op->autoclose_r || op->size > 0));
    bool transaction_exhausted = hwseg_exhausted && !op->cont_r;

    if(read_fulfilled || transaction_exhausted) {
        op->dma = false;
        op->autoclose_r = false;
        op->data_r = NULL;
        op->size = 0;
        op->callback = GINT_CALL_NULL;
        op->realized_size_r = NULL;
        status |= ASYNCIO_REQUEST_FINISHED;
    }
    if(hwseg_exhausted) {
        op->buffer_used = -1;
        status |= ASYNCIO_HWSEG_EXHAUSTED;

        if(status & ASYNCIO_REQUEST_FINISHED) {
            op->type = ASYNCIO_NONE;
            op->buffer_used = 0;
        }
    }
    if(transaction_exhausted) {
        status |= ASYNCIO_TRANSACTION_EXHAUSTED;
    }

    return status;
}

void asyncio_op_cancel_read(asyncio_op_t *op)
{
    op->dma = false;
    op->data_r = NULL;
    op->autoclose_r = false;
    op->size = 0;
    op->callback = GINT_CALL_NULL;
    op->realized_size_r = NULL;

    if(!asyncio_op_has_read_hwseg(op)) {
        op->type = ASYNCIO_NONE;
        op->buffer_used = 0;
    }
}
