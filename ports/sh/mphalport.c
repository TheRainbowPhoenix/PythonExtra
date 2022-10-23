#include <unistd.h>
#include "py/mpconfig.h"
#include "py/mphal.h"
#include "../../shared/readline/readline.h"

#include "keymap.h"
#include <gint/display.h>
#include <gint/keyboard.h>

// Receive single character, blocking until one is available.
int mp_hal_stdin_rx_chr(void) {
    while(1) {
        key_event_t ev = getkey();
        int key = ev.key;

        /* (The following meanings are only for non-empty lines) */
        /* TODO: Check cons->cursor before triggering them */

        if(key == KEY_LEFT && ev.shift)
            return CHAR_CTRL_A; /* go-to-start-of-line */
        if(key == KEY_LEFT)
            return CHAR_CTRL_B; /* go-back-one-char */
        if(key == KEY_ACON)
            return CHAR_CTRL_C; /* cancel */
        if(key == KEY_DEL && !ev.shift)
            return 8; /* delete-at-cursor */
        if(key == KEY_RIGHT && ev.shift)
            return CHAR_CTRL_E; /* go-to-end-of-line */
        if(key == KEY_RIGHT)
            return CHAR_CTRL_F; /* go-forward-one-char */
        if(key == KEY_DEL && ev.shift)
            return CHAR_CTRL_K; /* kill from cursor to end-of-line */
        if(key == KEY_DOWN)
            return CHAR_CTRL_N; /* go to next line in history */
        if(key == KEY_UP)
            return CHAR_CTRL_P; /* go to previous line in history */
        if(key == KEY_EXE)
            return '\r';

        uint32_t code_point = keymap_translate(key, ev.shift, ev.alpha);
        if(code_point != 0)
            return code_point;
    }
}

// Send the string of given length.
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    int r = write(STDOUT_FILENO, str, len);
    (void)r;

    extern void pe_draw(void);
    pe_draw();
}
