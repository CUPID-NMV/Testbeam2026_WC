#!/usr/bin/env python3
"""
Confronto tra due parametrizzazioni del fondo gamma ambientale LNGS:
  (A) la nostra: spettro CONTINUO liscio (misura NaI Hall A, Bellini 2013,
      arXiv:1101.5298), usata in gun_gammas_background.mac
  (B) colleghi CUPID: lista di RIGHE gamma discrete (spettro di emissione "vero"),
      file histo_environmental_gamma_CUPID_python_extract_true_spectrum.txt

Entrambe normalizzate a integrale unitario -> confronto di FORMA (distribuzione
in energia dei gamma), + quantificazione delle differenze.
"""
import os
import numpy as np
import matplotlib.pyplot as plt

HERE = "/Users/benussi/Testbeam2026_WC_unified/Analysis_script_background"
MAC  = "/Users/benussi/Testbeam2026_WC_unified/gun_gammas_background.mac"
COLL = os.path.join(HERE, "histo_environmental_gamma_CUPID_python_extract_true_spectrum.txt")
OUT  = os.path.join(HERE, "output_final")
os.makedirs(OUT, exist_ok=True)

# ---- (B) colleghi: righe discrete (E keV, intensita') ----
Eb, Ib = [], []
with open(COLL) as f:
    for ln in f:
        p = ln.split()
        if len(p) >= 2:
            try:
                Eb.append(float(p[0])); Ib.append(float(p[1]))
            except ValueError:
                pass
Eb = np.array(Eb); Ib = np.array(Ib)            # keV, conteggi
Ib_n = Ib / Ib.sum()                             # frazione per riga

# ---- (A) nostra: punti continui dal .mac (E MeV -> keV, intensita' rel.) ----
Ea, Ia = [], []
with open(MAC) as f:
    for ln in f:
        if "/gps/hist/point" in ln:
            p = ln.split()
            try:
                Ea.append(float(p[1])*1000.0); Ia.append(float(p[2]))  # MeV->keV
            except (ValueError, IndexError):
                pass
Ea = np.array(Ea); Ia = np.array(Ia)             # keV, dN/dE (rel.)

# ---- binning comune per confronto quantitativo ----
BW = 50.0                                         # keV per bin
edges = np.arange(0, 3450+BW, BW)
cen = 0.5*(edges[:-1]+edges[1:])
# B: somma intensita' righe per bin
hb,_ = np.histogram(Eb, bins=edges, weights=Ib)
hb = hb/hb.sum()
# A: integra lo spettro continuo (interp) in ogni bin
def A_dNdE(E): return np.interp(E, Ea, Ia, left=0., right=0.)
ha = np.array([np.trapezoid(A_dNdE(np.linspace(e0,e1,20)), np.linspace(e0,e1,20))
               for e0,e1 in zip(edges[:-1],edges[1:])])
ha = ha/ha.sum()

# ---- quantificazione ----
def band_frac(cen, h, lo, hi): return h[(cen>=lo)&(cen<hi)].sum()
def mean_E(cen, h): return (cen*h).sum()/h.sum()
bands = [(0,500),(500,1500),(1500,2700),(2700,3450)]
print("="*66)
print("CONFRONTO PARAMETRIZZAZIONI FONDO GAMMA LNGS (forma normalizzata)")
print("="*66)
print(f"{'':24} | {'nostra (NaI cont.)':>18} | {'colleghi (righe)':>17}")
print("-"*66)
print(f"{'energia media <E>':24} | {mean_E(cen,ha):>15.0f} keV | {mean_E(cen,hb):>14.0f} keV")
for lo,hi in bands:
    print(f"{'frazione '+f'{lo}-{hi} keV':24} | {100*band_frac(cen,ha,lo,hi):>16.1f}% | {100*band_frac(cen,hb,lo,hi):>15.1f}%")
# righe forti nei colleghi
print("-"*66)
print("Righe gamma dominanti nel file colleghi (top 6 per intensita'):")
idx = np.argsort(-Ib)[:6]
for i in idx:
    print(f"   {Eb[i]:>8.1f} keV   ({100*Ib_n[i]:.1f}% del totale)")
# frazione ad alta energia (>2 MeV, rilevante per Cherenkov nel WC)
print("-"*66)
fa = band_frac(cen,ha,2000,3450); fb = band_frac(cen,hb,2000,3450)
print(f"Frazione gamma > 2 MeV:  nostra {100*fa:.2f}%   colleghi {100*fb:.2f}%   (rapporto {fb/fa:.1f}x)")

# ---- plot ----
fig,(ax_lin,ax_log)=plt.subplots(1,2,figsize=(15,6))
fig.suptitle("Ambient gamma background at LNGS — spectrum parametrisation comparison", fontsize=13)
for ax,logy in ((ax_lin,False),(ax_log,True)):
    # colleghi: righe (stem)
    ax.vlines(Eb, 0, Ib_n, color="#d62728", lw=1.0, alpha=0.8,
              label="CUPID colleagues (emission lines)")
    # nostra: curva continua, normalizzata alla stessa scala (per riga equivalente)
    Efine=np.linspace(0,2700,2000); Afine=A_dNdE(Efine)
    Afine=Afine/np.trapezoid(Afine,Efine)*BW    # densita' -> frazione per bin da 50 keV
    ax.plot(Efine, Afine, color="#1f77b4", lw=2.2, label="our param. (NaI continuum)")
    ax.set_xlabel("gamma energy (keV)"); ax.set_ylabel("normalised intensity")
    ax.set_xlim(0,2800); ax.grid(True,alpha=0.3,which="both"); ax.legend(fontsize=10)
    if logy: ax.set_yscale("log"); ax.set_ylim(1e-5,1); ax.set_title("log scale")
    else: ax.set_title("linear scale")
# marca righe note
for E,lab in [(1461,"⁴⁰K"),(1764,"²¹⁴Bi"),(2615,"²⁰⁸Tl")]:
    ax_log.axvline(E,color="gray",lw=0.7,ls=":"); ax_log.text(E,1.2e-5,lab,fontsize=8,rotation=90,color="gray")
plt.tight_layout()
p=os.path.join(OUT,"gamma_spectra_comparison.pdf")
fig.savefig(p,bbox_inches="tight"); print("\nsalvato:",p)
plt.show()
