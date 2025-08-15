PythonExtra Internals
=====================

This chapter covers a tour of PythonExtra from the perspective of a developer, contributing
to PythonExtra. It acts as a comprehensive resource on the implementation details of PythonExtra
for both novice and expert contributors.

Development around PythonExtra usually involves modifying the core runtime, porting or
maintaining a new library. This guide describes at great depth, the implementation
details of PythonExtra including a getting started guide, compiler internals, porting
PythonExtra to a new platform and implementing a core PythonExtra library.

.. toctree::
   :maxdepth: 3

   gettingstarted.rst
   writingtests.rst
   compiler.rst
   memorymgt.rst
   library.rst
   optimizations.rst
   qstr.rst
   maps.rst
   publiccapi.rst
   extendingmicropython.rst
   porting.rst
