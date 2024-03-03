# Utilisation des modules `kandinsky`, `ion` et `time` de l'implémentation Python `Numworks`

PythonExtra offre la possibilité d'utiliser certains modules de la Numworks afin de rendre les scripts de cette machine compatible en l'état sur Casio fx-CG50 (pas de support sur la fx9860G pour cause de mémoire insuffisante et d'abence d'écran couleur).

Il s'agit d'un Work in Progress (WIP) et le support est sujet à tests approfondis à ce stade. Le port concerne les modules `kandinsky`, `ion` et `time` de la Numworks qui sont spécifiques à la machine et sont désormais supportés via cette implémentation. Les modules `math`, `cmath`, `random` étant identiques entre la version `Numworks` et les modules `builtins` de MicroPython, ils ne font donc pas partie de cette implémentation mais sont parfaitement utilisables sans modification dans les scripts.

Note : les modules `turtle` et `matplotlib.pyplot` ne sont pas repris dans cette implémentation. Il est possible d'utiliser les modules `turtle`, `matplotlib` et `casioplot` de Casio Education qui sont parfaitement fonctionnels et fournis en example dans `ports/sh/examples`.


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

Les fonctions suivantes sont des ajouts pour tirer partie de l'écran large de la fxCG et qui sont donc une extension du module `Kandinsky`. Elles ne sont donc par définition pas compatible avec le Python Numwork. Ces fonctions sont reconnaisables à leurs appellations qui commencent toutes par `CGEXT_` :

- `CGEXT_Enable_Wide_Screen()` : Active l'écran étendu de la fxCG, aucun paramètre n'est nécessaire. Les coordonnées x de l'écran physique peuvent être négatives pour empiéter sur la bande blanche de gauche et supérieures à 319 pixels pour empièter sur la bande blanche de droite;

- `CGEXT_Disable_Wide_Screen()` : Annule l'activation de l'écran étendu de la fxCG, aucun paramètre n'est nécessaire. Les coordonnées x de l'écran physique seront contraintes entre 0 et 320 pixels. Au-delà, le tracé ne sera pas effectué. 

- `CGEXT_Is_Wide_Screen_Enabled()` : Retourne `True` si l'écran étendu est actif et `False` dans le cas contraire.

- `CGEXT_Set_Margin_Color( color )` : Trace les marge de la fxCG50 (pourtours de l'écran `Numworks`) de la couleur passée en argument.

Note 1 : après avoir réalisé un tracé dans la zone étendue, il faut que celle-ci soit active pour permettre son effacement (typiquement via un appel à la fonction `fill_rect()` avec les paramètres adéquats).

Note 2 : En mode non étendu (par défaut à l'initialisation du module `Kandinsky`) les coordonnées de l'écran vont de (0,0) à (319,221) centré sur l'écran de la fxCG. En mode étendu, les coordonnées de l'écran vont de (-38,-1) à (358,223).


## `ion`

Le module `ion` donne accès à la fonction `keydown(k)` qui renvoie True si la touche k placée en argument est appuyée et False sinon.

La "conversion" des touches entre la machine Numworks et Casio fxCG50 se fait selon le mapping suivant :

| Numworks | Casio fxCG50 | Numworks Key # |
|----------|--------------|---------------------|
| KEY_LEFT     | KEY_LEFT | 0 |
| KEY_UP     | KEY_UP | 1 |
| KEY_DOWN     | KEY_DOWN | 2 |
| KEY_RIGHT     | KEY_RIGHT | 2 |
| KEY_OK      | KEY_F1 | 4 |
| KEY_BACK        | KEY_EXIT | 5 |
| KEY_HOME        | KEY_MENU | 6 |
| KEY_ONOFF       | KEY_ACON | 7 |
| ...           | ... | ... |
| KEY_SHIFT     | KEY_SHIFT | 12 |
| KEY_ALPHA     | KEY_ALPHA | 13 |
| KEY_XNT     | KEY_XOT | 14 |
| KEY_VAR     | KEY_VARS | 15 |
| KEY_TOOLBOX     | KEY_OPTN | 16 |
| KEY_BACKSPACE       | KEY_DEL | 17 |
| KEY_EXP     | KEY_EXP | 17 |
| KEY_LN     | KEY_LN | 19 |
| KEY_LOG     | KEY_LOG | 20 |
| KEY_IMAGINARY       | KEY_F2 | 21 |
| KEY_COMMA     | KEY_COMMA | 22 |
| KEY_POWER     | KEY_POWER | 23 |
| KEY_SINE        | KEY_SIN | 24 |
| KEY_COSINE      | KEY_COS | 25 |
| KEY_TANGENT     | KEY_TAN | 26 |
| KEY_PI      | KEY_F3 | 27 |
| KEY_SQRT        | KEY_F4 | 28 |
| KEY_SQUARE     | KEY_SQUARE | 29 |
| KEY_SEVEN       | KEY_7 | 30 |
| KEY_EIGHT       | KEY_8 | 31 |
| KEY_NINE        | KEY_9 | 32 |
| KEY_LEFTPARENTHESIS     | KEY_LEFTP | 33 |
| KEY_RIGHTPARENTHESIS        | KEY_RIGHTP | 34 |
| ...           | ... | ... |
| KEY_FOUR        | KEY_4 | 36  |
| KEY_FIVE        | KEY_5 | 37 |
| KEY_SIX     | KEY_6 | 38 |
| KEY_MULTIPLICATION      | KEY_MUL | 39 |
| KEY_DIVISION        | KEY_DIV | 40 |
| ...           | ... | ... |
| KEY_ONE     | KEY_1 | 42 |
| KEY_TWO     | KEY_2 | 43 |
| KEY_THREE       | KEY_3 | 44 |
| KEY_PLUS        | KEY_ADD | 45 |
| KEY_MINUS       | KEY_SUB | 46 |
| ...           | ... | ... |
| KEY_ZERO        | KEY_0 | 48 |
| KEY_DOT     | KEY_DOT | 49 |
| KEY_EE      | KEY_F5 | 50 |
| KEY_ANS     | KEY_NEG | 51 |
| KEY_EXE     | KEY_EXE | 52 |


## `time`

Le module `time` donne accès à deux fonctions :

- `monotonic()` : Renvoie la valeur de lʼhorloge au moment où la fonction est appelée.

- `sleep(t)` : Suspend lʼexécution pendant t secondes.
