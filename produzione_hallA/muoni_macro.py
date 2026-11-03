"""Scrive gun_muons_hallA.mac: muoni cosmici in Hall A con il flusso di MACRO
(decisione dell'utente, 2026-11-03: e' quello delle simulazioni di CUPID).

- Angoli: le distribuzioni zenitale e azimutale di MACRO, le stesse di
  gun_muons_lngs.mac, lette come intensita' per steradiante. Per un piano di
  generazione orizzontale serve la distribuzione dei muoni che lo
  attraversano, dN/dtheta ~ I(theta) sin(theta) cos(theta), che GPS vuole
  come contenuto di bin uguali e col primo peso a zero (il primo punto di un
  istogramma GPS diventa una delta sul bordo basso).
- Energia: Mei & Hime (Phys. Rev. D 73, 053004, 2006), eq. 3, per LNGS
  (h = 3.1 km.w.e.), dN/dE ~ (E + 492.5 GeV)^-3.77, indipendente
  dall'angolo; come dN/dE su una griglia logaritmica fitta.
- Piano: orizzontale a z = +100 mm, sopra il serbatoio (+60 mm), largo
  quanto l'ingombro di WC e pannelli piu' 773 mm per lato, cosi' che ogni
  muone fino a 65 gradi che tocca il setup lo attraversi.
- Normalizzazione: flusso di MACRO per unita' di area perpendicolare,
  3.22 +- 0.08 x 10^-4 m^-2 s^-1 (Hall B; LVD 3.31 in Hall A, Borexino 3.43
  in Hall C); attraverso il piano orizzontale va moltiplicato per <cos theta>.

Uso: python3 muoni_macro.py [phi0_gradi]. phi0 ruota l'azimut: lo zero di
phi di MACRO rispetto al setup non e' noto (serve l'orientazione del setup
in Hall A), di default 0.
"""
import math
import sys

import numpy as np

USCITA = "../gun_muons_hallA.mac"
PHI_MACRO = 3.22e-4        # m^-2 s^-1, per unita' di area perpendicolare
PHI0 = float(sys.argv[1]) if len(sys.argv) > 1 else 0.0

# MACRO, da gun_muons_lngs.mac: intensita' (unita' arbitrarie) ogni 5 gradi
TH_MACRO = np.arange(0, 66, 5.0)
I_MACRO = np.array([3500, 3300, 3000, 2800, 2500, 2200, 1800, 1500, 1300, 1100, 900, 700, 500, 150.0])
# azimut: densita' nei punti letti dalla figura (passo non uniforme)
PH_MACRO = np.array([0, 10, 30, 50, 70, 90, 110, 130, 150, 170, 210, 230, 250, 270, 290, 310, 330, 350, 360.0])
F_MACRO = np.array([875, 900, 1000, 1150, 1250, 1200, 1200, 400, 900, 1000, 1100, 800, 700, 700, 750, 750,
                    800, 850, 875.0])

# piano: ingombro di housing e serbatoio, x -1023..620, y -290..319, fondo a z -260.5 mm
Z_PIANO, Z_FONDO, TH_MAX = 100.0, -260.5, 65.0
MARGINE = (Z_PIANO - Z_FONDO) * math.tan(math.radians(TH_MAX))
X0, X1 = -1023.0 - MARGINE, 620.0 + MARGINE
Y0, Y1 = -290.0 - MARGINE, 319.0 + MARGINE
AREA = (X1 - X0) * (Y1 - Y0) / 1e6                # m^2


def integra(f, a, b, n=200):
    t = np.linspace(a, b, n + 1)
    v = f(t)
    return float(np.sum(0.5*(v[1:] + v[:-1]) * np.diff(t)))


def I(t):                                          # t in radianti
    return np.interp(np.degrees(t), TH_MACRO, I_MACRO)


# theta: bin di 1 grado, contenuto = integrale di I sin cos
bordi_th = np.radians(np.arange(0, TH_MAX + 0.5, 1.0))
w_th = [integra(lambda t: I(t)*np.sin(t)*np.cos(t), a, b) for a, b in zip(bordi_th[:-1], bordi_th[1:])]
cos_medio = integra(lambda t: I(t)*np.sin(t)*np.cos(t), 0, math.radians(TH_MAX), 2000) / \
            integra(lambda t: I(t)*np.sin(t), 0, math.radians(TH_MAX), 2000)
th_medio = integra(lambda t: t*I(t)*np.sin(t)*np.cos(t), 0, math.radians(TH_MAX), 2000) / \
           integra(lambda t: I(t)*np.sin(t)*np.cos(t), 0, math.radians(TH_MAX), 2000)

# phi: bin di 5 gradi, contenuto = integrale della densita' interpolata
bordi_ph = np.radians(np.arange(0, 360.1, 5.0))
w_ph = [integra(lambda p: np.interp(np.degrees(p), PH_MACRO, F_MACRO), a, b) for a, b in zip(bordi_ph[:-1], bordi_ph[1:])]

# energia: Mei & Hime su 0.1 GeV - 100 TeV, 20 punti per decade
a_mh = 693.0 * (1 - math.exp(-0.4 * 3.1))
E = np.logspace(-1, 5, 121)                        # GeV
dnde = (E + a_mh) ** -3.77
e_media = a_mh / (3.77 - 2)

rate = PHI_MACRO * cos_medio * AREA                # Hz
r = []
r.append("# =======================================================")
r.append("# SORGENTE: muoni cosmici in Hall A, flusso di MACRO (per la produzione")
r.append("# di 24 ore con WC e pannelli, produzione_hallA/FISICA.md).")
r.append("# Generata da produzione_hallA/muoni_macro.py: non modificarla a mano.")
r.append("#")
r.append("# Angoli: distribuzioni di MACRO (le stesse di gun_muons_lngs.mac) lette")
r.append("# come intensita' I(theta) per steradiante; per il piano orizzontale")
r.append("# dN/dtheta ~ I sin(theta) cos(theta), bin di 1 grado fino a 65, phi a bin")
r.append("# di 5 gradi, primo peso zero.")
r.append(f"#   theta medio {math.degrees(th_medio):.1f} gradi, <cos theta> = {cos_medio:.3f}")
r.append(f"#   rotazione dell'azimut phi0 = {PHI0:.1f} gradi (orientazione del setup in Hall A: da misurare)")
r.append("# Energia: Mei & Hime 2006, h = 3.1 km.w.e., dN/dE ~ (E + 492.5 GeV)^-3.77,")
r.append(f"#   da 0.1 GeV a 100 TeV, media {e_media:.0f} GeV")
r.append(f"# Piano: z = {Z_PIANO:.0f} mm, x {X0:.1f}..{X1:.1f}, y {Y0:.1f}..{Y1:.1f} mm, {AREA:.3f} m^2")
r.append(f"# Rate: {PHI_MACRO:.3g} m^-2 s^-1 (MACRO) x <cos theta> x area = {rate:.4g} Hz")
r.append(f"#   -> {rate*86400:.1f} muoni in 24 ore")
r.append("# Solo mu-: per i segnali la carica non conta.")
r.append("# Da includere DOPO /run/initialize.")
r.append("# =======================================================")
r.append("/gps/particle mu-")
r.append("/gps/pos/type Plane")
r.append("/gps/pos/shape Rectangle")
r.append(f"/gps/pos/centre {0.5*(X0+X1):.2f} {0.5*(Y0+Y1):.2f} {Z_PIANO:.1f} mm")
r.append(f"/gps/pos/halfx {0.5*(X1-X0):.2f} mm")
r.append(f"/gps/pos/halfy {0.5*(Y1-Y0):.2f} mm")
r.append("/gps/ang/type user")
if PHI0 != 0.0:
    c, s = math.cos(math.radians(PHI0)), math.sin(math.radians(PHI0))
    r.append(f"/gps/ang/rot1 {c:.6f} {s:.6f} 0")
    r.append(f"/gps/ang/rot2 {-s:.6f} {c:.6f} 0")
r.append("/gps/hist/type theta")
r.append("/gps/hist/point 0 0")
r += [f"/gps/hist/point {b:.6f} {w:.6g}" for b, w in zip(bordi_th[1:], w_th)]
r.append("/gps/hist/type phi")
r.append("/gps/hist/point 0 0")
r += [f"/gps/hist/point {b:.6f} {w:.6g}" for b, w in zip(bordi_ph[1:], w_ph)]
r.append("/gps/ene/type Arb")
r.append("/gps/hist/type arb")
r += [f"/gps/hist/point {e*1000:.6g} {v:.6g}" for e, v in zip(E, dnde / dnde[0])]
r.append("/gps/hist/inter Lin")
open(USCITA, "w").write("\n".join(r) + "\n")
print(f"{USCITA}: theta medio {math.degrees(th_medio):.1f} gradi, <cos> {cos_medio:.3f}, piano {AREA:.3f} m^2, "
      f"rate {rate:.4g} Hz, {rate*86400:.1f} muoni in 24 ore, energia media {e_media:.0f} GeV")
