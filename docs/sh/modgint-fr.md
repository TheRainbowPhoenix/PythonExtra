# `gint`: Module d'accès aux fonctionnalités de gint

PythonExtra est écrit à l'aide du [fxSDK](https://gitea.planet-casio.com/Lephenixnoir/fxsdk) et utilise [gint](https://gitea.planet-casio.com/Lephenixnoir/gint) pour exécuter l'add-in. Le module Python `gint` permet d'accéder aux fonctions internes de gint en Python pour le dessin, le clavier, etc. Comme gint possède beaucoup de fonctions utiles avec de bonnes performances, il est intéressant de s'en servir au lieu d'utiliser e.g. `casioplot` ou `turtle`.

Le module `gint` essaie de garder en Python la même API que dans la version originale de la bibliothèque en C, c'est pourquoi peu de fonctions utilisent des arguments nommés ou autres fonctions surchargées. Il y a quelques différences, documentées à la fin de cette page. En cas de doute, la documentation fournie par les [fichiers d'en-tête de gint](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint) (les `.h`) est tout à fait applicable pour comprendre les comportements que cette page n'explique pas.

Tous les noms de constantes, fonctions, etc. discutés dans cet article sont dans le module `gint`.

```py
import gint
# ou:
from gint import *
```

## Saisie au clavier

Les en-têtes de référence sont [`<gint/keyboard.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/keyboard.h) et [`<gint/keycodes.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/keycodes.h).

### Noms des touches

Le module fournit des constantes entières désignant toutes les touches du clavier. Les noms sont les suivants :

|            |            |            |            |            |             |
|------------|------------|------------|------------|------------|-------------|
|`KEY_F1`    |`KEY_F2`    |`KEY_F3`    |`KEY_F4`    |`KEY_F5`    |`KEY_F6`     |
|`KEY_SHIFT` |`KEY_OPTN`  |`KEY_VARS`  |`KEY_MENU`  |`KEY_LEFT`  |`KEY_UP`     |
|`KEY_ALPHA` |`KEY_SQUARE`|`KEY_POWER` |`KEY_EXIT`  |`KEY_DOWN`  |`KEY_RIGHT`  |
|`KEY_XOT`   |`KEY_LOG`   |`KEY_LN`    |`KEY_SIN`   |`KEY_COS`   |`KEY_TAN`    |
|`KEY_FRAC`  |`KEY_FD`    |`KEY_LEFTP` |`KEY_RIGHTP`|`KEY_COMMA` |`KEY_ARROW`  |
|`KEY_7`     |`KEY_8`     |`KEY_9`     |`KEY_DEL`   |`KEY_ACON`  |             |
|`KEY_4`     |`KEY_5`     |`KEY_6`     |`KEY_MUL`   |`KEY_DIV`   |             |
|`KEY_1`     |`KEY_2`     |`KEY_3`     |`KEY_ADD`   |`KEY_SUB`   |             |
|`KEY_0`     |`KEY_DOT`   |`KEY_EXP`   |`KEY_NEG`   |`KEY_EXE`   |             |

### Événements clavier

```
key_event:
  .time  -> int
  .mod   -> bool
  .shift -> bool
  .alpha -> bool
  .type  -> KEYEV_NONE | KEYEV_DOWN | KEYEV_UP | KEYEV_HOLD
  .key   -> KEY_*
```

gint communique les informations sur ce qu'il se passe au clavier via des _événements_. Les événements indiquent quand une touche (champ `.key`) a été pressée, maintenue, ou relâchée (champ `.type` égal à `KEYEV_DOWN`, `KEYEV_HOLD` et `KEYEV_UP` respectivement), quand (champ `.time`) et si des modifieurs (SHIFT ou ALPHA, champs `.shift` et `.alpha`) étaient actifs à ce moment-là.

(Le champ `.mod` n'est pas très intéressant, et la valeur `KEYEV_NONE` de `.type` est discutée dans `pollevent()`.)

Les fonctions `getkey()`, `getekey_opt()`, `pollevent()` et `waitevent()` renvoient toutes des événements.

### Saisie d'une touche avec attente

```py
getkey() -> key_event
```

La fonction `getkey()` met le programme en pause jusqu'à ce qu'une touche soit pressée ou répétée, et renvoie l'événement associé (qui est forcément de type `KEYEV_DOWN` ou `KEYEV_HOLD`). Par défaut, les seules touches qui sont répétées sont les touches directionnelles, une première fois après 400 ms, et ensuite toutes les 40 ms.

Pas mal de choses peuvent se produire pendant l'exécution de `getkey()`. L'utilisateur peut appuyer sur SHIFT ou ALPHA, ce qui affecte les champs `.shift` et `.alpha` de l'événement renvoyé. L'utilisateur peut également se rendre au menu principal avec MENU et éteindre la calculatrice avec SHIFT+AC/ON.

_Exemple._ Dans un menu de sélection de N éléments, on pourrait naviguer avec les touches haut et bas, sauter directement au début ou à la fin avec SHIFT haut et SHIFT bas, et valider avec EXE.

```py
ev = getkey()
if ev.key == KEY_EXE:
    pass       # Valider
elif ev.key == KEY_UP and ev.shift:
    pos = 0    # Revenir au début
elif ev.key == KEY_DOWN and ev.shift:
    pos = N-1  # Aller à la fin
elif ev.key == KEY_UP and pos > 0:
    pos -= 1   # Monter d'une position
elif ev.key == KEY_DOWN and pos < N-1:
    pos += 1   # Descendre d'une position
```

### Lecture des événements en temps réel

```py
pollevent() -> key_event
waitevent() -> key_event
clearevents() -> None
```

gint enregistre l'activité du clavier en tâche de fond pendant que le programme s'exécute. Les événements sont mis dans une file d'attente jusqu'à ce que le programme les lise. C'est comme ça par exemple que `getkey()` détermine quoi renvoyer.

Il est possible d'accéder aux événements directement à l'aide de la fonction `pollevent()`. `pollevent()` renvoie l'événement le plus ancien qui n'a pas encore été lu par le programme. Si le programme a lu tous les événements et qu'il n'y a plus rien en attente, `pollevent()` renvoie un "faux" évenement de type `KEYEV_NONE` pour indiquer qu'il n'y a plus rien à lire.

Comme `pollevent()` retourne instanténement, on peut s'en servir pour lire l'activité du clavier sans mettre le programme en pause.

_Exemple._ Une boucle de jeu pourrait, à chaque frame, lire tous les événements en attente pour déterminer quand le joueur appuie sur la touche SHIFT ("action" dans cet exemple) pour déclencher une action.

```py
# Dessiner le jeu...

while True:
    ev = pollevent()
    if ev.type == KEYEV_NONE:
        break  # Fin de la lecture des événements
    if ev.type == KEYEV_DOWN and ev.key == KEY_SHIFT:
        pass   # La touche SHIFT vient d'être pressée !
    # Ignore implicitement les autres touches

# Simuler le jeu...
```

La fonction `waitevent()` est similaire, mais si tous les événements ont été lus elle attend qu'un événement se produise avant de retourner. Elle est plus rarement utilisée parce qu'en général quand on veut attendre on utilise `getkey()`.

La fonction `clearevents()` lit et ignore tous les événements, i.e. elle "jette" toutes les informations sur ce qu'il s'est passé au clavier. Elle est utile pour connaître l'état instantané du clavier avec `keydown()` (voir ci-dessous). `clearevents()` est équivalente à la définition suivante :

```py
def clearevents():
    ev = pollevent()
    while ev.type != KEYEV_NONE:
        ev = pollevent()
```

### Lecture de l'état instantané du clavier

```py
keydown(key: int) -> bool
keyup(key: int) -> bool
keydown_all(*keys: [int]) -> bool
keydown_any(*keys: [int]) -> bool
```

Une fois les événements lus, on peut tester l'état individuellement si les touches sont pressées ou pas à l'aide de la fonction `keydown()`. `keydown(k)` renvoie `True` si la touche `k` est pressée, `False` sinon. Cette fonction ne marche **que si les événements ont été lus**, ce qu'on fait souvent soit avec `pollevent()` soit avec `clearevents()`.

_Exemple._ Une boucle de jeu pourrait tester si les touches gauche/droite sont pressées à chaque frame pour déplacer le joueur.

```py
while True:
    ev = pollevent()
    # ... comme dans l'exemple pollevent()

if keydown(KEY_LEFT):
    player_x -= 1
if keydown(KEY_RIGHT):
    player_x += 1
```

La fonction `keyup()` renvoie l'opposé de `keydown()`. La fonction `keydown_all()` prent une série de touches en paramètre et renvoie `True` si elles sout toutes pressées. `keydown_any()` est similaire et renvoie `True` si au moins une des touches listées est pressée.

### Lecture rapide des changements de position des touches

```py
cleareventflips() -> None
keypressed(key: int) -> bool
keyreleased(key: int) -> bool
```

`keydown()` indique uniquement l'état instantané des touches. Elle ne permet pas de déterminer à quel moment une touche passe de l'état relâché à l'état pressé ou l'inverse. Pour ça, il faut soit utiliser les événements (ce qui est un peu lourd), soit utiliser les fonctions ci-dessous.

Les fonctions `keypressed(k)` et `keyreleased(k)` indiquent si la touche a été pressée/relâchée depuis le dernier appel à `cleareventflips()`. Attention la notion de "pressée/relâchée" ici n'est pas le temps réel mais la lecture des événements.

_Exemple._ Une boucle de jeu peut tester à la fois l'état immédiat et les changements d'état des touches en utilisant les fonctions instantanée après `cleareventflips()` suivi de `clearevents()`.

```py
# Dessiner le jeu...

cleareventflips()
clearevents()

if keypressed(KEY_SHIFT):
    pass # Action !
if keydown(KEY_LEFT):
    player_x -= 1
if keydown(KEY_RIGHT):
    player_x += 1

# Simuler le jeu...
```

## Fonctions de dessin basiques

Les en-têtes de référence sont [`<gint/display.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/display.h), et pour certains détails techniques [`<gint/display-fx.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/display-fx.h) et [`<gint/display-cg.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/display-cg.h).

TODO

## Fonctions de dessin d'images

TODO

## Différences avec l'API C de gint

- `dsubimage()` n'a pas de paramètre `int flags`. Les flags en question ne ont que des optimisations mineures et pourraient disparaître dans une version future de gint.
- Les constructeurs d'image `image()` et `image_<format>()` n'existent pas dans l'API C.

TODO : Il y en a d'autres.
