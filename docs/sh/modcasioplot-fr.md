# `casioplot` : Module officiel d'I/O de CASIO

`casioplot` est une bibliothèque de dessin fournie par CASIO pour supporter les modules officiels `turtle` et `matplotlib`. Le module a été [annoncé en Février 2020](https://www.planet-casio.com/Fr/forums/topic16154-1-modules-graphiques-python-en-avril-matplotlib-et-turtle.html) et [publié en Avril](https://www.planet-casio.com/Fr/forums/topic16243-1-rendu-graphique-en-python-partie-1-decouverte-de-matplotlib-et-turtle.html). Le module est disponible sur Graph 35+E II et Graph 90+E avec la même API. C'est le seul module personnalisé dans le port MicroPython de CASIO (à date de Mars 2024). PythonExtra fournit le module par compatibilité pour les scripts officiels.

```py
import casioplot
# or
from casioplot import *
```

**Sommaire**
- [Fonctions de dessin](#fonctions-de-dessin)
- [Considérations spéciale pour le rendu](#considerations-speciales-pour-le-rendu)
- [Différences avec le module `casioplot` officiel](#differences-avec-le-module-casioplot-officiel)

## Fonctions de dessin

```py
show_screen() -> None
clear_screen() -> None
set_pixel(x: int, y: int, ?color: (int, int, int)) -> None
get_pixel(x: int, y: int) -> (int, int, int)
draw_string(x: int, y: int, text: str, ?color: (int, int, int), ?size: str) -> None
```

La zone de dessin est de 128x64 pixels sur Graph 35+E II et 384x216 pixels sur la Graph 90+E. La Graph 35+E II ne supporte que deux couleurs (noir et blanc) tandis que la Graph 90+E peut afficher 65536 couleurs (RGB565 16 bits). Cependant, le format de couleurs est _quand même_ des triplets (r,g,b) avec 0 ≤ r,g,b ≤ 255. Ce choix maintient la compabilité entre les modèles et avec le PC, en échange d'un coût en performance. Les calculatrices approximent ces couleurs en les remplaçant par la couleur la plus proche qu'elles peuvent afficher.

Comme le module gint, `casioplot` dessine systématiquement dans un buffer interne qu'on appelle la VRAM, et n'affiche le résultat à l'écran que quand on lui demande explicitement, ou à la fin de l'exécution.

`show_screen()` affiche à l'écran les contenus de la VRAM après un dessin.

`clear_screen()` remplit la VRAM en blanc.

`set_pixel()` remplace la couleur du pixel à la position (x,y) par `color`, qui doit être un triplet (r,g,b) si fournie, et sera noir si absente. Construire des triplets est un peu lent donc il vaut mieux éviter d'écrire des appels comme `set_pixel(x, y, (0,0,0))` dans des boucles ; il est plus performant de stocker le triplet dans une variable et d'utiliser la variable.

`get_pixel()` renvoie la couleur du pixel à la position (x,y) de la VRAM sous la forme d'un triplet (r,g,b). Comme la VRAM stocke les couleurs au même format que l'écran, qui ne supporte généralement pas 16 millions de couleurs, appeler `get_pixel()` juste après `set_pixel()` renvoie une _approximation_ de la couleur originale. Comme cette fonction alloue un triplet, elle est atrocement lente à appeler en boucle, au point où elle peut créer du lag simplement par sa pression sur le GC, ce qui la rend parfois inutilisable.

`draw_string()` affiche du texte. (x,y) est la position du coin haut-gauche du rectangle dans lequel le texte est dessiné. La couleur est optionnelle ; si elle est spécifiée il faut que ce soit un triplet (r,g,b), sinon c'est noir. La taille peut être l'une des trois chaînes `"small"`, `"medium"` ou `"large"` ; la valeur par défaut est `"medium"`. Sur la Graph 35+E II, les polices `"small"` et `"medium"` sont identiques, tandis que sur la Graph 90+E les trois polices sont différentes. Tout `'\n'` dans la chaîne est remplacé par un espace avant d'afficher.

TODO: Exemple

## Considérations spéciale pour le rendu

_Mode texte._ `casioplot` passe automatiquement PythonExtra en mode graphique à l'import. Les programmes qui veulent faire du dessin graphique après un `print()` doivent appeler `show_screen()` _avant_ de commencer à dessiner le premier frame parce que `print()` repasse en mode texte.

_Performance._ Le fait que les fonctions de dessin écrivent silencieusement dans la VRAM est la principale raison pour laquelle `casioplot` est parfois beaucoup plus rapide que le dessin en Basic. PRGM réaffiche la VRAM à l'écran après chaque appel d'une fonction de dessin, ce qui est lent. Malheureusement, `casioplot` redevient plus lent que le Basic pour les dessins complexes pour lesquels il y a des fonctions spécialisées en Basic (comme `DrawStat`), puisque `casioplot` ne peut dessiner qu'un pixel à la fois. Dans ces situations, il vaut mieux utiliser les fonctions de dessin du [module gint](modgint-fr.md).

## Différences avec le module `casioplot` officiel

- L'implémentation PythonExtra du module n'affiche pas automatiquement l'écran à la fin du programme.
- Les polices pour `draw_string()` diffèrent des polices officielles sur Graph 35+E II.
- Les polices pour `draw_string()` ne supportent actuellement que les caractères ASCII.
- Les zone de dessin sur Graph 90+E est en haut à gauche de l'écran.
