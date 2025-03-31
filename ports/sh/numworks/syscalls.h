//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.numworks.syscalls: fx-CG syscalls used for the Numworks module

#ifndef __PYTHONEXTRA_NUMWORKS_SYSCALLS_H
#define __PYTHONEXTRA_NUMWORKS_SYSCALLS_H

/* Returns the battery type. 1: Alkaline batteries, 2: Ni-MH. */
int CASIOWIN_GetBatteryType(void);

/* Parameter should always be 1. Returns battery voltage * 100. */
int CASIOWIN_GetMainBatteryVoltage(int one);

#endif /* __PYTHONEXTRA_NUMWORKS_SYSCALLS_H */
