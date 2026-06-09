

# `machine`: Low-level Memory Access

For advanced debugging or hardware inspection, you can use the MicroPython machine module to read raw memory registers. 

```py
import casioplot
# or
from casioplot import *
```

**Contents**
- [Reading memory](#reading-memory)


## Reading memory

The module exposes three objects used for raw memory access.

Memory read are done with :
- `mem8` : Read/write 8 bits of memory.

- `mem16` : Read/write 16 bits of memory.

- `mem32` : Read/write 32 bits of memory.

Use subscript notation `[...]` to index these objects with the address of
interest. Note that the address is the byte address, regardless of the size of
memory being accessed.

For example, you can read the OS version :

```python
import machine
chars = [chr(machine.mem8[0x80020020 + i]) for i in range(15)]
os_ver = "".join(chars)
print(os_ver)
```

**Note**: the address may change depending on your OS version.
