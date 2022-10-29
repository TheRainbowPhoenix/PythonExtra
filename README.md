# PythonExtra - A community MicroPython for CASIO calculators

*Original MicroPython README: [`README-MicroPython.md`](README-MicroPython.md).*

This is a MicroPython port for fx-CG 50, fx-9860G III and related CASIO calculators.

---

**TODO list**

Bugs to fix:
- Don't refresh screen at every low-level console print
- Fix not world switching during filesystem accesses (very unstable)

Python features:
- Compare features with existing implementations and other brands
- Get filesystem access (open etc) with the POSIX interface
- Get a decent amount of RAM not just the fixed 32 kiB that's hardcoded so far

UI:
- Use [JustUI](/Lephenixnoir/JustUI) to get a file browser (already available)
- Add an option for fixed-width font which also sets $COLUMNS properly so that
  MicroPython paginates (requires better getenv/setenv support in fxlib)
- Use [unicode-fonts](/Lephenixnoir/unicode-fonts) to provide Unicode support
  * Extend it with a fixed-width uf8x9 which maybe changes some glyphs (like
    `i` and `l`; the usual fixed-width unambiguous styles)
  * Try and provide a font smaller than 5x7 for more compact shell on mono


Future wishes:
- Build for fx-9860G II (requires manual filesystem support)
- Lephe's secret ideas (complete previous list to unlock)
