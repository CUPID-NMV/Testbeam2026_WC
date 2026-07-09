#!/usr/bin/env python3
"""
Distribuzione del numero di fotoni Cherenkov che arrivano al PMT per evento.
Sovrappone piu' geometrie sulla stessa configurazione di run.

Uso:
    python3 plot_photons_at_pmt.py [src] [Xtag] [Ytag] [Thtag]
default: src=e_tb  X0mm Y0mm Th0deg   (geometrie: single e single_wbc3)
"""
import sys, os
import numpy as np
import uproot
import matplotlib.pyplot as plt

DATA_DIR = "/Users/benussi/Testbeam2026_WC_unified/data"
OUT_DIR  = "/Users/benussi/Testbeam2026_WC_unified/Analysis_script_background/output_final"
os.makedirs(OUT_DIR, exist_ok=True)

SRC = sys.argv[1] if len(sys.argv) > 1 else "e_tb"
XT  = sys.argv[2] if len(sys.argv) > 2 else "X0mm"
YT  = sys.argv[3] if len(sys.argv) > 3 else "Y0mm"
TT  = sys.argv[4] if len(sys.argv) > 4 else "Th0deg"
CFG = f"{SRC}_{XT}_{YT}_{TT}"

# geometrie da sovrapporre: etichetta -> tag file, colore
GEOMS = {
    "single (orig.)": ("single",      "#7f7f7f"),
    "WBC2":           ("single_wbc2", "#d62728"),
    "WBC3":           ("single_wbc3", "#1f77b4"),
}

def photons_per_event(tag):
    fpath = os.path.join(DATA_DIR, f"sim_{tag}_{CFG}.root")
    with uproot.open(fpath) as f:
        n_gen = int(f["Eventi"].num_entries)
        eid = f["Fotoni"]["EventID"].array(library="np").astype(int)
    return np.bincount(eid, minlength=n_gen), n_gen

data = {}
for label, (tag, col) in GEOMS.items():
    n_tot, n_gen = photons_per_event(tag)
    data[label] = (n_tot, n_gen, col)
    n_hits = int((n_tot > 0).sum())
    print(f"{label:16} ({tag}): N_gen={n_gen}, con fotoni={n_hits} ({100*n_hits/n_gen:.1f}%), "
          f"⟨N_ph al PMT⟩={n_tot.mean():.1f} (tutti)  {n_tot[n_tot>0].mean():.1f} (solo hit)")

# range comune
allmax = max(np.percentile(nt, 99.5) for nt, _, _ in data.values())
bins = np.arange(0, int(allmax) + 3) - 0.5

geo_str = "  vs  ".join(GEOMS.keys())
fig, (ax_lin, ax_log) = plt.subplots(1, 2, figsize=(14, 5))
fig.suptitle(f"Photons reaching the PMT per event — {CFG}\n{geo_str}", fontsize=12)
for ax, logy in ((ax_lin, False), (ax_log, True)):
    for label, (n_tot, n_gen, col) in data.items():
        ax.hist(n_tot, bins=bins, density=True, histtype="step", lw=2.2, color=col,
                label=f"{label}  (⟨N⟩={n_tot.mean():.1f})")
    ax.set_xlabel("N photons at PMT per event")
    ax.set_ylabel("probability density")
    ax.grid(True, alpha=0.3, which="both")
    ax.legend(fontsize=10)
    if logy:
        ax.set_yscale("log"); ax.set_title("log scale")
    else:
        ax.set_title("linear scale")

plt.tight_layout()
geo_tags = "_".join(t for _, (t, _) in GEOMS.items())
out = os.path.join(OUT_DIR, f"photons_at_pmt_{CFG}_{geo_tags}.pdf")
fig.savefig(out, bbox_inches="tight")
print("salvato:", out)
plt.show()
