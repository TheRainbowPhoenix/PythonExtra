#Flappy Bird Python
#par Xavier Andreani (tiplanet.org) et
#Sebastien Michelland (planet-casio.com)
#pour Graph 35+E II, fx-9750GIII, fx-9860GIII
#--------------------
from casioplot import *
from random import randint
from gint import *
from micropython import *
from gc import *
opt_level(3)
#CONFIG GRAPHIQ CASIO
L_FENETRE, H_FENETRE = 128, 64                   #dimensions de la zone graphique affichable
LNUMH_POLICE = {"small":(4,5), "medium":(4,5), "large": (6,7)}
POLICE = "small"                                 #taille de police (small, medium ou large)
LNUM_POLICE, H_POLICE = LNUMH_POLICE[POLICE][:2] #dimensions des caracteres de la police (largeur numeriques + hauteur)
#CONFIG GRAPHIQUE JEU
L_TUYAU, HMIN_TUYAU = 16, 9           #largeur + hauteur minimale des tuyaux
H_SOL = H_POLICE + 2                  #hauteur du sol
H_FOND = H_FENETRE - H_SOL            #hauteur du fond
D_FBIRD, L_FBIRD, H_FBIRD = 32, 10, 7 #position initiale + largeur + hauteur de Flappy Bird
INTH_TUYAUX, INTVMIN_TUYAUX = 64, 16  #espacement horizontal + vertical minimal entre les tuyaux
MAX_TUYAUX = 3                        #nombre de tuyaux maximal en RAM
#EXTENSION CASIOPLOT
#FONCTIONS GRAPHIQ JEU
COULEUR_BORD, COULEUR_FOND, COULEUR_SOL, COULEUR_TUYAU = C_BLACK, C_WHITE, C_WHITE, C_BLACK
PALETTE = (C_WHITE, C_BLACK)
BIRD_IMG = (
  "   ####   ",
  " ## #  #  ",
  "# # #   # ",
  "#  # #### ",
  " ### #   #",
  "  #   ### ",
  "   ###    ")
# traduction des caracteres en couleurs
BIRD = [[PALETTE[c == "#"] for c in line] for line in BIRD_IMG]
margin = 0 #Nombre de tuyaux supprimes
ntuyaux = 0 #Nombre de tuyaux
DEBUG = False
def y_altitude(alt):
  """retourne la position verticale d'affichage d'un objet situe a l'altitude alt"""
  return H_FOND - alt
def draw_tuyaux(x, hinf, hsup):
  """affiche une paire de tuyaux de hauteurs hinf (inferieur) et hsup (superieur) avec x comme limite gauche"""
  yinf = y_altitude(hinf)
  drect_border(x, hsup - 6, x + L_TUYAU, hsup,
  C_WHITE, 1, COULEUR_TUYAU); drect_border(x + 3, 0, x + L_TUYAU - 3, hsup - 6, C_WHITE, 1, COULEUR_TUYAU)
  drect_border(x, yinf, x + L_TUYAU, yinf + 6,
  C_WHITE, 1, COULEUR_TUYAU); drect_border(x + 3, yinf + 6, x + L_TUYAU - 3, yinf + hinf - 1, C_WHITE, 1, COULEUR_TUYAU)
def draw_fbird(x, y, x_ancien, y_ancien):
  """dessine Flappy Bird dans un rectangle de L_FBIRD*H_FBIRD pixels avec comme coin superieur gauche (x;y)"""
  y = round(y)
  for ligne in range(H_FBIRD):
    for col in range(L_FBIRD): dpixel(x + col, y + ligne, BIRD[ligne][col])
def draw_sol():
  """affiche le sol et dedans les informations"""
  dhline(H_FENETRE - H_POLICE - 2, COULEUR_BORD)
  info = vx_fenetre == 0 and "[AC]: Demarrer" or vx_fbird!=0 and "[AC]: Battre des ailes" or "[AC]: Quitter"
  draw_string(1, H_FENETRE - H_POLICE, info, (0, 0, 0), POLICE)
def draw_score():
  """affiche le score dans le coin inferieur droit"""
  ch_score = str(score)
  draw_string(L_FENETRE - LNUM_POLICE * len(ch_score), H_FENETRE - H_POLICE, ch_score, (0, 0, 0), POLICE)
def draw_jeu():
  """initialise l'affichage en dessinant l'ensemble des elements graphiques"""
  dclear(C_WHITE)
  draw_fbird(d_fbird - d_fenetre, y_altitude(alt_fbird), d_fbird_ancien - d_fenetre, y_altitude(alt_fbird_ancien))
  for k in range(0, len(d_tuyaux)):
    d_tuyau = d_tuyaux[k]
    if d_tuyau+L_TUYAU >= d_fenetre and d_tuyau < d_fenetre+L_FENETRE:
      draw_tuyaux(d_tuyau - d_fenetre, hinf_tuyaux[k], hsup_tuyaux[k])
  draw_sol(); draw_score()
#AUTRES FONCTIONS JEU
def ajoute_tuyau():
  """ajoute une paire de tuyaux"""
  global d_tuyaux, hinf_tuyaux, hsup_tuyaux, margin, ntuyaux
  d_tuyaux.append(d_tuyaux[-1] + INTH_TUYAUX)
  hinf_tuyau = randint(HMIN_TUYAU, H_FOND - INTVMIN_TUYAUX - HMIN_TUYAU)
  hsup_tuyau = randint(HMIN_TUYAU, H_FOND - hinf_tuyau - INTVMIN_TUYAUX)
  hinf_tuyaux.append(hinf_tuyau); hsup_tuyaux.append(hsup_tuyau)
  ntuyaux += 1
  if ntuyaux>MAX_TUYAUX:
    del d_tuyaux[0]; del hinf_tuyaux[0]; del hsup_tuyaux[0]
    ntuyaux -= 1
    margin += 1

def simulation():
  """met a jour l'etat du jeu"""
  global d_fenetre, d_fbird, d_fbird_ancien, alt_fbird, alt_fbird_ancien, vx_fbird, vy_fbird, score, margin
  if d_fenetre + L_FENETRE >= d_tuyaux[-1] + INTH_TUYAUX: ajoute_tuyau()
  if d_tuyaux[score - margin] + L_TUYAU<d_fbird: score += 1 # le score correspond a l'index du tuyau a tester
  if vx_fbird > 0 and collision_fbird():
    vx_fbird = 0
    if vy_fbird>0: vy_fbird = 0
    draw_sol()
  if vx_fenetre != 0 and vx_fbird != 0:
    d_fenetre -= vx_fenetre
    d_fbird_ancien, alt_fbird_ancien = d_fbird, alt_fbird
    d_fbird += vx_fbird
    alt_fbird = max(H_FBIRD, alt_fbird + vy_fbird)
    vy_fbird += dy_gravite
def isect_rect(x1, y1, w1, h1, x2, y2, w2, h2):
  """teste si il y a une intersection entre les rectangles
  de largeur w1 hauteur h1 au coin superieur gauche (x1;y1)
  et largeur w2 hauteur h2 au coin superieur gauche (x2;y2)
  """
  return max(x1, x2) <= min(x1 + w1, x2 + w2) and max(y1, y2) <= min(y1 + h1, y2 + h2)
def collision_fbird():
  """teste si il y a collision entre Flappy Bird et les actuels tuyaux inferieur ou superieur"""
  d_tuyau, hinf_tuyau, hsup_tuyau = d_tuyaux[score - margin], hinf_tuyaux[score - margin], hsup_tuyaux[score - margin]
  y_fbird = y_altitude(alt_fbird)
  isect_inf = isect_rect(d_tuyau, y_altitude(hinf_tuyau), L_TUYAU, hinf_tuyau, d_fbird, y_fbird, L_FBIRD, H_FBIRD)
  isect_sup = isect_rect(d_tuyau, y_fbird, L_TUYAU, hsup_tuyau - y_fbird, d_fbird, y_fbird, L_FBIRD, H_FBIRD)
  return isect_inf or isect_sup
def action():
  """effectue l'action correspondant a la touche pressee"""
  try:
    global vx_fenetre, vy_fbird
    if vx_fenetre == 0:
      vx_fenetre = -1
      draw_sol()
    elif vx_fbird != 0: vy_fbird = 1.25
    else:
      print("score:\n", score)
      if DEBUG: print(mem_free())
      raise SystemExit
  except KeyboardInterrupt: action()
#ETAT INITIAL JEU
d_tuyaux = [L_FENETRE * 3 // 4]                               #emplacements horizontaux des tuyaux
hinf_tuyaux, hsup_tuyaux = [HMIN_TUYAU + 5], [HMIN_TUYAU + 5] #hauteurs des tuyaux
score = 0                                                     #nombre de tuyaux franchis
d_fbird, alt_fbird = D_FBIRD, H_FOND * 3 // 4                 #distance + altitude de Flappy Bird
d_fbird_ancien, alt_fbird_ancien = d_fbird, alt_fbird         # anciennes distance + altitude de Flappy Bird
vx_fbird, vy_fbird = 1, 0                                     # vitesses horizontale + verticale de Flappy Bird
dy_gravite = -0.1                                             # acceleration verticale de la pesanteur
vx_fenetre, d_fenetre = 0, 0                                  # vitesse scrolling horizontale + position de la fenetre
#LANCEMENT JEU
ajoute_tuyau()
def game():
  try:
    while True:
      try:
        draw_jeu(); dupdate()
        simulation()
      except KeyboardInterrupt: action()
  except KeyboardInterrupt:
    game()
if DEBUG: print(mem_free())
game()