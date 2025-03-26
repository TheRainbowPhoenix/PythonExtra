# PythonExtra - A community MicroPython for CASIO calculators

*Original MicroPython README: [`README-MicroPython.md`](README-MicroPython.md).*

This is a MicroPython port for fx-CG 50, fx-9860G III and related CASIO calculators.

**Build instructions**

Requires the [fxSDK](/Lephenixnoir/fxsdk). Go to `ports/fxcg50` or `ports/fx9860g3` and run `make`. If it doesn't build, first try to use the `dev` branches for [fxSDK](/Lephenixnoir/fxSDK), [gint](/Lephenixnoir/gint), [fxlibc](/Lephenixnoir/fxlib) and [JustUI](/Lephenixnoir/JustUI). Rebuilds don't always work especially when checking out other commits (maybe my fault), so try to delete `build` if you think that's related.

Most of the code is in `ports/sh` and is shared between the platforms.

---

**Test suite**

Located in [`ports/sh/examples`](ports/sh/examples).

* **fx-CG**: Whether the program successfully runs on PythonExtra on
  models of the fx-CG family. This is typically tested on a Graph 90+E but
  should include the fx-CG 10/20/50.
* **G-III**: Whether the program successfully runs on G-III models.
  This does **not** include older models like the fx-9860G II.
* **Raw speedup**: Speedup from using PythonExtra instead of the official
  Python app, without changing the code (as a ratio of execution times).
* **Full speedup**: Seedup from using PythonExtra-provided modules for
  rendering and input (usually `gint`), after changing the program's code.

| Program | fx-CG | G-III | Raw speedup | Full speedup |
| ------- | ----- | ----- | ----------- | ------------ |
| Chute tridimensionnelle | Yes | Yes | x1.36 | x2.26 ⁽¹⁾ |
| Bad Apple | Meh⁽²⁾ | - | TODO | TODO |
| Island of the Dead Kings | TODO | TODO | TODO | TODO |
| Synchro-Donjon (AI mode) | Yes | TODO | x1.82 | - |
| Flappy Bird | Yes | Yes | x1.04 | x2.35 |

⁽¹⁾ After upgrading to full-screen 396x224 instead of the original 120x80. (!)

⁽²⁾ Bad Apple requires unloading modules to not run out of memory, and I
haven't been able to consistently do that. See the `unload-modules` branch.

---

**Basic benchmarks**

PythonExtra is slightly faster than the official Python app, probably due to
optimization during compilation of the VM. The tests below show the effect on
basic Python operations.

| Test | Program | Official Python (fx-CG 50) | PythonExtra (fx-CG 50) |
| ---- | ------- | -------------------------- | ---------------------- |
| VM speed | `pe_loop.py` (1 million `pass`) | ~12 seconds | 8.9 seconds |
| Shell output | `pe_print.py` (print 100000 integers) | ~22 seconds | 11.3 seconds |
| Large integers | `pe_fact.py` (compute 250!, 500 times) | ~15 seconds | 8.6 seconds |

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
- Add an option for fixed-width font which also sets $COLUMNS properly so that
  MicroPython paginates (requires better getenv/setenv support in fxlibc)
- Use [unicode-fonts](/Lephenixnoir/unicode-fonts) to provide Unicode support
  * Extend it with a fixed-width uf8x9 which maybe changes some glyphs (like
    `i` and `l`; the usual fixed-width unambiguous styles)
  * Try and provide a font smaller than 5x7 for more compact shell on mono
- A decent keymap that can input all relevant characters into the shell
- Features that would match MicroPython's readline:
   * Multi-line input (figure out how to store it); also, auto-indent
   * History (use `MP_STATE_PORT(readline_hist)` with `readline_push_history()`
     for a start); also, zsh-style search
   * Autocompletion (use `mp_repl_autocomplete()` which should hook just fine)

Future wishes:
- Build for fx-9860G II (requires manual filesystem support)
- Lephe's secret ideas (complete previous list to unlock)
