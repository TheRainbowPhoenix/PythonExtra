//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "py/mphal.h"
#include "console.h"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <unistd.h>

int mp_hal_stdin_rx_chr(void)
{
    while(1) {
        key_event_t ev = getkey();
        int code_point = console_key_event_to_char(ev);
        if(code_point != 0)
            return code_point;
    }
}

mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    int r = write(STDOUT_FILENO, str, len);
    return (r < 0 ? 0 : r);
}
