# utilisation des modules `kandinsky`, `ion` et `time` de l'implémentation Python sur `NumWorks`

PythonExtra offre la possibilité d'utiliser certains modules de la Numworks afin de rendre les scripts de cette machine compatible en l'état sur la fxCG50 (pas de support sur la fx9860G pour cause de mémoire insuffisante).

Il s'agit d'un Work in Progress (WIP) et le support est à ce stade partiel, néanmoins les modules `kandinsky`, `ion` et `time` de la NumWorks sont supportés.

## `kandinsky`

Le module `kandinsky` offre le support des primitives graphiques via les routines hautes performance de `gint`. Toutes les fonctions de ce module sont disponibles :

- `color(r,g,b)` : Génère la valeur de la couleur r,g,b. Vous pouvez aussi simplement utiliser un tuple pour définir une couleur : (r,g,b).

- `get_pixel(x,y)` : Renvoie la couleur du pixel aux coordonnées x,y sous forme de tuple (r,g,b).

- `set_pixel(x,y,color)` : Allume le pixel x,y de la couleur color.

- `draw_string(text,x,y,[color1],[color2])` : Affiche le texte text aux coordonnées x,y. Les arguments color1 (couleur du texte) et color2 (couleur de lʼarrière plan du texte) sont optionnels.

- `fill_rect(x,y,width,height,color)` : Remplit un rectangle de largeur width et de hauteur height avec la couleur color au point de coordonnées x et y.

Le module offre de plus un certain nombre de couleurs explicitement nommées et accessibles par une chaine de caractères. Les valeurs suivantes sont utilisables en lieu et place des paramètres de couleur des fonctions de `kandinsky` :
- "red", "r"
- "green", "g"
- "blue", "b"
- "black", "k"
- "white", "w"
- "yellow", "y"
- "pink"
- "magenta"
- "grey", "gray"
- "purple"
- "orange"
- "cyan"
- "brown"


## `ion`

Le module `ion` donne accès à la fonction `keydown(k)` qui renvoie True si la touche k placée en argument est appuyée et False sinon.

La "conversion" des touches entre la machine NumWorks et Casio fxCG50 se fait selon le mapping suivant :

| NumWorks | Casio fxCG50 |
|----------|--------------|
| KEY_LEFT     | KEY_LEFT |
| KEY_UP     | KEY_UP |
| KEY_DOWN     | KEY_DOWN |
| KEY_RIGHT     | KEY_RIGHT |
| KEY_OK      | KEY_F1 |
| KEY_BACK        | KEY_EXIT |
| KEY_HOME        | KEY_MENU |
| KEY_ONOFF       | KEY_ACON |
| KEY_SHIFT     | KEY_SHIFT |
| KEY_ALPHA     | KEY_ALPHA |
| KEY_XNT     | KEY_XOT |
| KEY_VAR     | KEY_VARS |
| KEY_TOOLBOX     | KEY_OPTN |
| KEY_BACKSPACE       | KEY_DEL |
| KEY_EXP     | KEY_EXP |
| KEY_LN     | KEY_LN |
| KEY_LOG     | KEY_LOG |
| KEY_IMAGINARY       | KEY_F2 |
| KEY_COMMA     | KEY_COMMA |
| KEY_POWER     | KEY_POWER |
| KEY_SINE        | KEY_SIN |
| KEY_COSINE      | KEY_COS |
| KEY_TANGENT     | KEY_TAN |
| KEY_PI      | KEY_F3 |
| KEY_SQRT        | KEY_F4 |
| KEY_SQUARE     | KEY_SQUARE |
| KEY_SEVEN       | KEY_7 |
| KEY_EIGHT       | KEY_8 |
| KEY_NINE        | KEY_9 |
| KEY_LEFTPARENTHESIS     | KEY_LEFTP |
| KEY_RIGHTPARENTHESIS        | KEY_RIGHTP |
| KEY_FOUR        | KEY_4 |
| KEY_FIVE        | KEY_5 |
| KEY_SIX     | KEY_6 |
| KEY_MULTIPLICATION      | KEY_MUL |
| KEY_DIVISION        | KEY_DIV |
| KEY_ONE     | KEY_1 |
| KEY_TWO     | KEY_2 |
| KEY_THREE       | KEY_3 |
| KEY_PLUS        | KEY_ADD |
| KEY_MINUS       | KEY_SUB |
| KEY_ZERO        | KEY_0 |
| KEY_DOT     | KEY_DOT |
| KEY_EE      | KEY_F5 |
| KEY_ANS     | KEY_NEG |
| KEY_EXE     | KEY_EXE |


## `time`

Le module `time` donne accès à deux fonctions :

- `monotonic()` : Renvoie la valeur de lʼhorloge au moment où la fonction est appelée.

- `sleep(t)` : Suspend lʼexécution pendant t secondes.


