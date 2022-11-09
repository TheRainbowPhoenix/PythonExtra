# PythonExtra - A community MicroPython for CASIO calculators

*Original MicroPython README: [`README-MicroPython.md`](README-MicroPython.md).*

This is a MicroPython port for fx-CG 50, fx-9860G III and related CASIO calculators.

**Build instructions**

Requires the [fxSDK](/Lephenixnoir/fxsdk). Go to `ports/fxcg50` or `ports/fx9860g3` and run `make`. If it doesn't build, first try to use the `dev` branches for [fxSDK](/Lephenixnoir/fxSDK), [gint](/Lephenixnoir/gint), [fxlibc](/Lephenixnoir/fxlib) and [JustUI](/Lephenixnoir/JustUI).

Most of the code is in `ports/sh` and is shared between the platforms.

---

**TODO list**

Bugs to fix:
- Fix not world switching during filesystem accesses (very unstable)
- Fix the console not garbage collecting its lines (enable and test the feature)
- Fix not resetting the shell when importing a file from command-line

Python features:
- Compare features with existing implementations and other brands
- Get filesystem access (open etc) with the POSIX interface
- Get a decent amount of RAM not just the fixed 32 kiB that's hardcoded so far

UI:
- Better input system in the shell (don't use VT-100 escapes, then move cursor
  and handle history)
- Add an option for fixed-width font which also sets $COLUMNS properly so that
  MicroPython paginates (requires better getenv/setenv support in fxlib)
- Use [unicode-fonts](/Lephenixnoir/unicode-fonts) to provide Unicode support
  * Extend it with a fixed-width uf8x9 which maybe changes some glyphs (like
    `i` and `l`; the usual fixed-width unambiguous styles)
  * Try and provide a font smaller than 5x7 for more compact shell on mono
- A decent keymap that can input all relevant characters into the shell

Test cases:
- [ ] Chute tridimensionnelle
- [ ] Bad Apple
- [ ] Island of the Dead Kings
- [ ] Synchro-Donjon

Future wishes:
- Build for fx-9860G II (requires manual filesystem support)
- Lephe's secret ideas (complete previous list to unlock)
