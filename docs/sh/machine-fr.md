Voici la traduction en français de ce texte :

# `machine` : Accès à la mémoire de bas niveau

Pour le débogage avancé ou l'inspection du matériel, vous pouvez utiliser le module MicroPython `machine` pour lire les registres de mémoire bruts.

```py
import casioplot
# ou
from casioplot import *
```

**Sommaire**
- [Lecture de la mémoire](#lecture-de-la-mémoire)

## Lecture de la mémoire

Le module expose trois objets utilisés pour l'accès à la mémoire brute.

Les lectures en mémoire s'effectuent avec :
- `mem8` : Lecture/écriture de 8 bits de mémoire.
- `mem16` : Lecture/écriture de 16 bits de mémoire.
- `mem32` : Lecture/écriture de 32 bits de mémoire.

Utilisez la notation par indice `[...]` pour indexer ces objets avec l'adresse souhaitée. Notez que l'adresse est une adresse en octets, quelle que soit la taille de la mémoire à laquelle on accède.

Par exemple, vous pouvez lire la version du système d'exploitation (OS) :

```python
import machine
chars = [chr(machine.mem8[0x80020020 + i]) for i in range(15)]
os_ver = "".join(chars)
print(os_ver)
```

**Remarque** : l'adresse peut changer en fonction de votre version du système d'exploitation.