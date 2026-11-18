"""Scrive gun_muons_hallA.mac: muoni cosmici in Hall A con il flusso di MACRO
(decisione dell'utente, 2026-11-03: e' quello delle simulazioni di CUPID).

- Angoli (dal 2026-11-18, decisione dell'utente: allineati ad AGATA, la
  simulazione ufficiale di CUPID). Prima erano letti a occhio dalla stessa
  figura di AGATA (fig. 4.1 della tesi Tozzi, "MACRO distribution implemented
  in AGATA") come una curva liscia in theta: in realta' la figura ha 5 bin
  uguali in cos(theta), da 0.5 a 1, e il MC aveva troppi muoni quasi verticali.
  Ora:
  * zenit: l'intensita' per area perpendicolare I(theta) ha, in ogni bin di
    0.1 in cos(theta), il peso di AGATA (figura digitalizzata, sotto). Dentro il
    bin la forma e' quella della mappa della montagna di MACRO, Ambrosio et al.,
    PRD 52 (1995) 3793: spessore di roccia della Tab. V (macro_prd1995_tabV_
    spessore_m.csv, azimut dal nord, celle vuote = regioni scartate da MACRO,
    riempite interpolando in azimut), intensita' verticale dell'eq. (3) e
    I = I_v / cos(theta). La mappa da sola coincide con AGATA entro il 5% fino
    a 46 gradi e sta sotto del 16% e del 26% nei due bin oltre (confronto in
    data/.../confronto_20261118/angoli_muoni_macro.ipynb, fuori da git).
    Fino a 60 gradi, come AGATA. Per un piano orizzontale dN/dtheta ~ I sin cos.
  * azimut: l'istogramma di AGATA (fig. 4.1, sinistra: 20 bin da 18 gradi),
    phi_GPS = Theta_AGATA + phi0. Lo zero rispetto al setup non e' noto.
  * theta e phi sono indipendenti, come nei due istogrammi di AGATA (la mappa
    di MACRO li lega: per ora non si usa).
- Energia: Mei & Hime (Phys. Rev. D 73, 053004, 2006), eq. 3, per LNGS
  (h = 3.1 km.w.e.), dN/dE ~ (E + 492.5 GeV)^-3.77, indipendente
  dall'angolo; come dN/dE su una griglia logaritmica fitta. MACRO (Astropart.
  Phys. 10 (1999) 11, eq. 2) da' quasi lo stesso spettro (mediana 142 GeV
  contro 140).
- Piano: orizzontale a z = +100 mm, sopra il serbatoio (+60 mm), largo
  quanto l'ingombro di WC e pannelli piu' un margine per lato, cosi' che ogni
  muone fino a TH_MAX che tocca il setup lo attraversi.
- Normalizzazione: flusso di MACRO per unita' di area perpendicolare,
  3.22 +- 0.08 x 10^-4 m^-2 s^-1 (Hall B; LVD 3.31 in Hall A, Borexino 3.43
  in Hall C); attraverso il piano orizzontale va moltiplicato per <cos theta>.

Uso: python3 muoni_macro.py [phi0_gradi]. phi0 ruota l'azimut: lo zero di
phi rispetto al setup non e' noto (serve l'orientazione del setup in Hall A),
di default 0.
"""
import math
import os
import sys

import numpy as np

QUI = os.path.dirname(os.path.abspath(__file__))
USCITA = os.path.normpath(os.path.join(QUI, "..", "gun_muons_hallA.mac"))
MAPPA = os.path.join(QUI, "macro_prd1995_tabV_spessore_m.csv")
PHI_MACRO = 3.22e-4        # m^-2 s^-1, per unita' di area perpendicolare
PHI0 = float(sys.argv[1]) if len(sys.argv) > 1 else 0.0

# AGATA, fig. 4.1 della tesi Tozzi, digitalizzata il 2026-11-18 (conteggi):
# zenit, 5 bin di cos(theta): 0.5-0.6, 0.6-0.7, 0.7-0.8, 0.8-0.9, 0.9-1.0
AGATA_COS_BORDI = np.array([0.5, 0.6, 0.7, 0.8, 0.9, 1.0])
AGATA_COS = np.array([1533, 2750, 4037, 4723, 4522.0])
# azimut ("Theta", piano orizzontale), 20 bin da 18 gradi da -180 a 180
AGATA_AZ = np.array([1110, 1345, 1027, 966, 756, 855, 1130, 1560, 1688, 1574,
                     1547, 1507, 1018, 909, 614, 456, 345, 386, 493, 966.0])
# MACRO, PRD 52 (1995) 3793: eq. (3) e roccia del Gran Sasso (Tab. IV)
A_EQ3, ALPHA, H0, RHO = 1.96e-6, 1.10, 972.0, 2.71     # cm^-2 s^-1 sr^-1, -, hg/cm^2, g/cm^3

# piano: ingombro di housing e serbatoio, x -1023..620, y -290..319, fondo a z -260.5 mm
Z_PIANO, Z_FONDO, TH_MAX = 100.0, -260.5, 60.0
MARGINE = (Z_PIANO - Z_FONDO) * math.tan(math.radians(TH_MAX))
X0, X1 = -1023.0 - MARGINE, 620.0 + MARGINE
Y0, Y1 = -290.0 - MARGINE, 319.0 + MARGINE
AREA = (X1 - X0) * (Y1 - Y0) / 1e6                # m^2


# --- zenit: forma della mappa di MACRO, pesi di AGATA ---------------------
def intensita_mappa(th_gradi):
    """I(theta) media in azimut (unita' arbitrarie) dalla Tab. V di MACRO"""
    M = np.genfromtxt(MAPPA, delimiter=",")             # righe: azimut 0..355; colonne: azimut, zenit 5..60
    azi, T = M[:, 0], M[:, 1:].T                        # T[zenit, azimut], metri
    zen = np.arange(5.0, 61.0, 5.0)
    L = np.log(T)
    for i in range(len(zen)):                           # celle vuote: interpolazione in azimut (periodica)
        ok = np.isfinite(L[i])
        L[i, ~ok] = np.interp(azi[~ok], azi[ok], L[i, ok], period=360)
    zen = np.concatenate([[0.0], zen])                  # zenit 0: la media dell'anello a 5 gradi
    L = np.vstack([np.full(len(azi), L[0].mean()), L])
    ph = np.arange(0.5, 360, 1.0)
    Lp = np.vstack([np.interp(ph, azi, L[i], period=360) for i in range(len(zen))])
    Lt = np.vstack([np.interp(th_gradi, zen, Lp[:, j]) for j in range(len(ph))]).T
    h = np.exp(Lt) * RHO                                # hg/cm^2
    Iv = A_EQ3 * (H0 / h) ** ALPHA * np.exp(-h / H0)
    return (Iv / np.cos(np.radians(th_gradi))[:, None]).mean(1)


th_f = np.arange(0.05, TH_MAX, 0.1)                     # gradi
I_m = intensita_mappa(th_f)
c_f = np.cos(np.radians(th_f))
dcos = np.sin(np.radians(th_f)) * np.radians(0.1)
k_bin = np.searchsorted(AGATA_COS_BORDI, c_f, side="right") - 1
peso_mappa = np.array([(I_m * dcos)[k_bin == k].sum() for k in range(5)])
scala = (AGATA_COS / AGATA_COS.sum()) / (peso_mappa / peso_mappa.sum())
I_a = I_m * scala[k_bin]                                # I(theta) allineata ad AGATA

# theta: bin di 1 grado, contenuto = integrale di I sin cos (piano orizzontale)
bordi_th = np.arange(0, TH_MAX + 0.5, 1.0)
w_th = [float((I_a * np.sin(np.radians(th_f)) * c_f)[(th_f >= a) & (th_f < b)].sum())
        for a, b in zip(bordi_th[:-1], bordi_th[1:])]
bordi_th = np.radians(bordi_th)
cos_medio = (I_a * c_f * dcos).sum() / (I_a * dcos).sum()
th_medio = math.radians((th_f * I_a * c_f * dcos).sum() / (I_a * c_f * dcos).sum())

# phi: bin di 5 gradi, contenuto = integrale dell'istogramma di AGATA (phi_GPS = Theta_AGATA, modulo 360)
bordi_ph = np.radians(np.arange(0, 360.1, 5.0))
dens = lambda p: AGATA_AZ[((np.mod(p + 180.0, 360.0)) // 18.0).astype(int)]
w_ph = [float(dens(np.arange(a + 0.05, b, 0.1)).mean() * (b - a))
        for a, b in zip(np.degrees(bordi_ph[:-1]), np.degrees(bordi_ph[1:]))]

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
r.append("# Angoli (dal 2026-11-18): allineati ad AGATA (fig. 4.1 della tesi Tozzi):")
r.append("# I(theta) per area perpendicolare coi pesi di AGATA in 5 bin di cos(theta)")
r.append("# e, dentro i bin, la forma della mappa di MACRO (PRD 52 (1995) 3793, Tab. V,")
r.append("# eq. 3, sec theta); per il piano dN/dtheta ~ I sin(theta) cos(theta), bin di")
r.append(f"# 1 grado fino a {TH_MAX:.0f}. Azimut: l'istogramma di AGATA (20 bin da 18 gradi), bin")
r.append("# di 5 gradi. Primo peso zero in theta e in phi.")
r.append(f"#   theta medio {math.degrees(th_medio):.1f} gradi, <cos theta> = {cos_medio:.3f}")
r.append(f"#   rotazione dell'azimut phi0 = {PHI0:.1f} gradi (orientazione del setup in Hall A: da misurare)")
r.append("# Energia: Mei & Hime 2006, h = 3.1 km.w.e., dN/dE ~ (E + 492.5 GeV)^-3.77,")
r.append(f"#   da 0.1 GeV a 100 TeV, media {e_media:.0f} GeV")
r.append(f"# Piano: z = {Z_PIANO:.0f} mm, x {X0:.1f}..{X1:.1f}, y {Y0:.1f}..{Y1:.1f} mm, {AREA:.3f} m^2")
r.append(f"# Rate: {PHI_MACRO:.3g} m^-2 s^-1 (MACRO) x <cos theta> x area = {rate:.4g} Hz")
r.append(f"#   -> {rate*86400:.1f} muoni generati sul piano in 24 ore (il piano e' piu' largo del setup)")
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
      f"rate {rate:.4g} Hz, {rate*86400:.1f} muoni sul piano in 24 ore, energia media {e_media:.0f} GeV")
