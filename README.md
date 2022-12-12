# PythonExtra - A community MicroPython for CASIO calculators

*Original MicroPython README: [`README-MicroPython.md`](README-MicroPython.md).*

This is a MicroPython port for fx-CG 50, fx-9860G III and related CASIO calculators.

**Build instructions**

Requires the [fxSDK](/Lephenixnoir/fxsdk). Go to `ports/fxcg50` or `ports/fx9860g3` and run `make`. If it doesn't build, first try to use the `dev` branches for [fxSDK](/Lephenixnoir/fxSDK), [gint](/Lephenixnoir/gint), [fxlibc](/Lephenixnoir/fxlib) and [JustUI](/Lephenixnoir/JustUI). Rebuilds don't always work especially when checking out other commits (maybe my fault), so try to delete `build` if you think that's related.

Most of the code is in `ports/sh` and is shared between the platforms.

---

**TODO list**

Bugs to fix:
- Fix not world switching during filesystem accesses (very unstable)
- Fix current working directory not changing during a module import (for
  relative imports)
- Fix casioplot not stopping on the last `show_screen()`

Python features:
- Compare features with existing implementations and other brands
- Get filesystem access (open etc) with the POSIX interface
- Get a decent amount of RAM not just the fixed 32 kiB that's hardcoded so far

UI:
- Better input system in the shell (don't use VT-100 escapes, then move cursor
  and handle history)
- Add an option for fixed-width font which also sets $COLUMNS properly so that
  MicroPython paginates (requires better getenv/setenv support in fxlibc)
- Use [unicode-fonts](/Lephenixnoir/unicode-fonts) to provide Unicode support
  * Extend it with a fixed-width uf8x9 which maybe changes some glyphs (like
    `i` and `l`; the usual fixed-width unambiguous styles)
  * Try and provide a font smaller than 5x7 for more compact shell on mono
- A decent keymap that can input all relevant characters into the shell

Future wishes:
- Build for fx-9860G II (requires manual filesystem support)
- Lephe's secret ideas (complete previous list to unlock)

**Test cases**

* **Works on fx-CG**: Whether the program successfully runs on PythonExtra on
  models of the fx-CG family. This is typically tested on a Graph 90+E but
  should include the fx-CG 10/20/50.
* **Works on G-III**: Whether the program successfully runs on G-III models.
  This does **not** include older models like the fx-9860G II.
* **Raw speedup**: Speed difference from using PythonExtra instead of the
  official Python app, without changing the code.
* **Full speedup**: Speed difference from using PythonExtra-provided modules
  for rendering and input (usually `gint`), after changing the program's code.

| Program | Works on fx-CG | Works on G-III | Raw speedup | Full speedup |
| ------- | -------------- | -------------- | ----------- | ------------ |
| Chute tridimensionnelle | TODO | Yes | TODO | TODO |
| Bad Apple | TODO | TODO | TODO | TODO |
| Island of the Dead Kings | TODO | TODO | TODO | TODO |
| Synchro-Donjon | TODO | TODO | TODO | TODO |
| Flappy Bird | Yes | Yes | TODO | TODO |
