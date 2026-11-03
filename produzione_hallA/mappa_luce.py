"""Mappa di luce dei pannelli (produzione_hallA/FISICA.md, sezione 5).

costruisci: dagli elettroni da 1 MeV uniformi nello scintillatore del
  pannello alto, con l'ottica accesa (ottica_accesa.sh + mappa_elettroni.mac),
  i fotoelettroni per MeV in funzione del baricentro del deposito, a bin di
  50 x 50 mm, e la distribuzione dei tempi d'arrivo dei fotoelettroni in
  funzione di x (la distanza dal SiPM, che sta a -x).
valida: su gamma simulati con l'ottica accesa, i PE veri di ogni pannello
  contro quelli della mappa, Poisson attorno a mappa(x, y) x energia. I tre
  pannelli sono uguali e usano la stessa mappa.

Uso:
  python3 -I mappa_luce.py costruisci <cartella ottica_mappa> <mappa.npz>
  python3 -I mappa_luce.py valida <cartella ottica_valida> <mappa.npz>
Ognuno salva una figura accanto al file della mappa.
"""
import glob
import os
import sys

import numpy as np
import uproot
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

PAN = ("top", "mid", "bot")
XE = np.arange(-625, 626, 50.0)        # bordi in x, mm (SiPM a -x)
YE = np.arange(-275, 276, 50.0)
TE = np.arange(0, 300.5, 0.5)          # tempi, ns
NUOVA, VECCHIA, INK, INK2, GRID, SURF = "#2a78d6", "#52514e", "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"
plt.rcParams.update({
    "figure.facecolor": SURF, "axes.facecolor": SURF, "savefig.facecolor": SURF,
    "axes.edgecolor": INK2, "axes.labelcolor": INK, "xtick.color": INK2, "ytick.color": INK2,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.axisbelow": True,
    "axes.spines.top": False, "axes.spines.right": False, "font.size": 9,
    "xtick.top": False, "ytick.right": False, "axes.titlesize": 10, "axes.titleweight": "bold",
    "legend.frameon": False, "legend.fontsize": 8,
})


def leggi(cartella, con_fotoni):
    ev, ft = [], []
    for i, f in enumerate(sorted(glob.glob(f"{cartella}/parte_*.root"))):
        with uproot.open(f) as r:
            e = r["Eventi"].arrays(library="np")
            e["file"] = np.full(len(e["EventID"]), i)
            ev.append(e)
            if con_fotoni:
                p = r["Fotoni"].arrays(["EventID", "PMT_ID", "Detected", "Arrival_Time_ns"], library="np")
                p["file"] = np.full(len(p["EventID"]), i)
                ft.append(p)
    e = {k: np.concatenate([x[k] for x in ev]) for k in ev[0]}
    p = {k: np.concatenate([x[k] for x in ft]) for k in ft[0]} if con_fotoni else None
    return e, p


def costruisci(cartella, uscita):
    e, p = leggi(cartella, True)
    ed, pe = e["Edep_panel_top_MeV"], e["Hits_SiPM_top_Detected"]
    x, y, z = (e[f"Pos_panel_top_{a}_mm"] for a in "XYZ")
    m = ed > 0.05
    print(f"{len(ed)} elettroni, {m.sum()} con piu' di 50 keV nel pannello alto; energia media {ed[m].mean():.3f} MeV")
    sPE = np.histogram2d(x[m], y[m], [XE, YE], weights=pe[m])[0]
    sE = np.histogram2d(x[m], y[m], [XE, YE], weights=ed[m])[0]
    n = np.histogram2d(x[m], y[m], [XE, YE])[0]
    mappa = np.where(sE > 0, sPE / np.maximum(sE, 1e-12), np.nan)
    errore = np.where(sE > 0, np.sqrt(sPE) / np.maximum(sE, 1e-12), np.nan)
    print(f"mappa: {np.nanmean(mappa):.1f} PE/MeV in media, da {np.nanmin(mappa):.1f} a {np.nanmax(mappa):.1f}; "
          f"{int((n == 0).sum())} bin vuoti, {n[n > 0].min():.0f}-{n.max():.0f} elettroni per bin")
    # dipendenza dalla profondita': z del baricentro in 5 fette
    zb = np.linspace(-25, 25, 6)
    prof_z = [pe[m][(z[m] >= a) & (z[m] < b)].sum() / ed[m][(z[m] >= a) & (z[m] < b)].sum() for a, b in zip(zb[:-1], zb[1:])]
    print("PE/MeV per fette in z (dal fondo alla faccia coi groove):", " ".join(f"{v:.1f}" for v in prof_z))
    # tempi d'arrivo dei fotoelettroni (l'elettrone parte a t = 0), per bin in x
    k = (p["PMT_ID"] == 6) & (p["Detected"] == 1)
    chiave_ev = e["file"] * 10**7 + e["EventID"]
    xi = dict(zip(chiave_ev, np.digitize(x, XE) - 1))
    xf = np.array([xi.get(c, -1) for c in p["file"][k] * 10**7 + p["EventID"][k]])
    tt = p["Arrival_Time_ns"][k]
    t_pdf = np.array([np.histogram(tt[xf == i], TE)[0] for i in range(len(XE) - 1)], float)
    t_pdf /= np.maximum(t_pdf.sum(axis=1, keepdims=True), 1)
    np.savez(uscita, x_edges=XE, y_edges=YE, pe_per_mev=mappa, errore=errore, n=n, t_edges=TE, t_pdf_per_x=t_pdf,
             z_edges=zb, pe_per_mev_z=np.array(prof_z))
    print(f"salvata {uscita}")

    fig, ax = plt.subplots(1, 3, figsize=(18, 4.4), gridspec_kw={"width_ratios": [1.6, 1, 1]})
    im = ax[0].pcolormesh(XE, YE, mappa.T, cmap="Blues", shading="flat")
    fig.colorbar(im, ax=ax[0], label="fotoelettroni per MeV")
    ax[0].set(xlabel="x nello scintillatore (mm), SiPM a sinistra", ylabel="y (mm)", aspect="equal",
              title="Mappa di luce, pannello (uguale per i tre)")
    ax[0].grid(False)
    xc = 0.5 * (XE[1:] + XE[:-1])
    ax[1].errorbar(xc, np.nansum(sPE, axis=1) / np.nansum(sE, axis=1),
                   yerr=np.sqrt(np.nansum(sPE, axis=1)) / np.nansum(sE, axis=1), fmt="o", ms=4, color=NUOVA)
    ax[1].set(xlabel="x (mm), SiPM a sinistra", ylabel="fotoelettroni per MeV", ylim=(0, None),
              title="Luce lungo il pannello (media in y)")
    tc = 0.5 * (TE[1:] + TE[:-1])
    for i, col in ((1, NUOVA), (12, VECCHIA), (23, INK)):
        ax[2].plot(tc, t_pdf[i] / np.diff(TE), color=col, lw=1.3, label=f"x = {xc[i]:.0f} mm")
    ax[2].set(xlabel="tempo d'arrivo del fotoelettrone (ns)", ylabel="frazione per ns", xlim=(0, 120), yscale="log",
              ylim=(1e-4, 0.2), title="Tempi d'arrivo al SiPM")
    ax[2].legend(loc="upper right")
    fig.tight_layout()
    fig.savefig(uscita.replace(".npz", ".png"), dpi=130, bbox_inches="tight")


def valuta(m, x, y):
    i = np.clip(np.digitize(x, m["x_edges"]) - 1, 0, len(m["x_edges"]) - 2)
    j = np.clip(np.digitize(y, m["y_edges"]) - 1, 0, len(m["y_edges"]) - 2)
    return m["pe_per_mev"][i, j]


def valida(cartella, mappa_npz):
    m = dict(np.load(mappa_npz))
    e, _ = leggi(cartella, False)
    rng = np.random.default_rng(1)
    vero, pred, edep = [], [], []
    for k in PAN:
        ed = e[f"Edep_panel_{k}_MeV"]
        s = ed > 0
        mu = valuta(m, e[f"Pos_panel_{k}_X_mm"][s], e[f"Pos_panel_{k}_Y_mm"][s]) * ed[s]
        vero.append(e[f"Hits_SiPM_{k}_Detected"][s]); pred.append(rng.poisson(mu)); edep.append(ed[s])
        print(f"pannello {k}: {s.sum()} depositi, PE veri {vero[-1].mean():.2f}, dalla mappa {pred[-1].mean():.2f} "
              f"(rapporto {pred[-1].sum() / max(vero[-1].sum(), 1):.3f})")
    vero, pred, edep = map(np.concatenate, (vero, pred, edep))
    print(f"tutti: PE veri {vero.sum()}, dalla mappa {pred.sum()}, rapporto {pred.sum() / vero.sum():.3f}")
    for soglia in (3, 7, 20, 90):
        print(f"  depositi con almeno {soglia:2d} PE: veri {np.mean(vero >= soglia):.4f}, mappa {np.mean(pred >= soglia):.4f}")
    fig, ax = plt.subplots(1, 2, figsize=(13, 4.4))
    b = np.arange(0, 121, 2)
    ax[0].stairs(np.histogram(vero, b)[0], b, color=VECCHIA, lw=1.6, ls="--", label="ottica completa")
    ax[0].stairs(np.histogram(pred, b)[0], b, color=NUOVA, lw=1.6, label="mappa di luce")
    ax[0].set(xlabel="fotoelettroni nel pannello", ylabel="depositi per 2 PE", yscale="log",
              title="Spettro dei PE dei gamma, tre pannelli")
    ax[0].legend(loc="upper right")
    be = np.linspace(0, 3, 31)
    c = 0.5 * (be[1:] + be[:-1])
    for dati, col, ls, nome in ((vero, VECCHIA, "--", "ottica completa"), (pred, NUOVA, "-", "mappa di luce")):
        med = [dati[(edep >= a) & (edep < bb)].mean() if np.any((edep >= a) & (edep < bb)) else np.nan
               for a, bb in zip(be[:-1], be[1:])]
        ax[1].plot(c, med, color=col, ls=ls, lw=1.6, label=nome)
    ax[1].set(xlabel="energia nel pannello (MeV)", ylabel="PE medi", title="PE medi in funzione dell'energia")
    ax[1].legend(loc="upper left")
    fig.tight_layout()
    fig.savefig(mappa_npz.replace(".npz", "_validazione.png"), dpi=130, bbox_inches="tight")


if __name__ == "__main__":
    modo, cartella, npz = sys.argv[1], sys.argv[2], sys.argv[3]
    costruisci(cartella, npz) if modo == "costruisci" else valida(cartella, npz)
