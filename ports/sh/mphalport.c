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

        if(key == KEY_ACON)
            return CHAR_CTRL_C;
        if(key == KEY_EXE)
            return '\r';
        if(key == KEY_EXIT)
            return CHAR_CTRL_D;

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
