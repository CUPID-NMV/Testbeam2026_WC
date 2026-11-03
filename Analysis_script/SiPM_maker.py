"""SiPM_maker: digitalizzazione dei segnali dei SiPM dei pannelli dal MC, sul
modello di Signal_maker.ipynb (che fa i PMT del WC).

Il SiPM e' l'S13360-6075PE del datasheet Hamamatsu (S13360 series,
KAPD1052E07, luglio 2025), alla sovratensione scelta. La catena di lettura
(ingresso a 50 ohm, amplificatore, V1742) e' a parte, nei parametri
ELETTRONICA: il datasheet non la da'.

Ingressi (file ROOT di single_pan3):
  fotoni  ottica dei pannelli accesa: i fotoelettroni sono le righe di
          "Fotoni" con PMT_ID 6/7/8 e Detected = 1 (Geant4 applica gia' la
          PDE del datasheet a Vop), al loro tempo d'arrivo;
  mappa   ottica dei pannelli spenta (la produzione): per ogni pannello con
          energia, PE ~ Poisson(mappa(x, y) x energia) e tempi dalla
          distribuzione della mappa (produzione_hallA/mappa_luce.py);
  buio    nessun file: finestre con il solo dark count, per il rate dei
          trigger del rumore.
Per ogni fotoelettrone: crosstalk (catena con la probabilita' del datasheet),
saturazione dei 6400 pixel, guadagno con dispersione; dark count su tutta la
finestra e nel microsecondo prima (le loro code ci entrano); poi l'impulso
della catena, il rumore e il campionamento del V1742.

Uso, con uproot (~/venvs/jupyter-global/bin/python):
  python3 -I SiPM_maker.py fotoni <file o cartella> <uscita.npz>
  python3 -I SiPM_maker.py mappa  <file o cartella> <uscita.npz> --mappa mappa_luce.npz
  python3 -I SiPM_maker.py buio   <finestre> <uscita.npz>
Opzioni: --sovratensione V (default 3, cioe' Vop), --onde N (forme d'onda
salvate, default 2000; le grandezze riassuntive ci sono per tutti gli
eventi), --seme S.
Uscita: uscita.npz con le forme d'onda (ADC, [evento, canale, campione]) e,
per ogni evento, energia per pannello, fotoelettroni, ampiezza e carica per
canale; una figura uscita.png con alcune forme d'onda e gli spettri.
"""
import argparse
import glob
import json
import os

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# ============================================================
# SiPM: Hamamatsu S13360-6075PE, datasheet (tipici, Ta = 25 C)
# ============================================================
N_PIXEL = 6400             # pixel da 75 um su 6 x 6 mm, fill factor 82%
CT_PF = 1280.0             # capacita' terminale
SOVRA_VOP = 3.0            # Vop = VBR + 3 V (VBR = 53 +- 5 V: va preso dai dati del singolo SiPM)
G_VOP = 4.0e6              # guadagno a Vop
XT_VOP = 0.07              # probabilita' di crosstalk a Vop
DCR_VOP_HZ = 2.0e6         # dark count a 0.5 p.e., tipico (massimo 6.0e6)
# Con la sovratensione dV (grafici del datasheet per i pixel da 75 um):
#   guadagno proporzionale a dV (G = C dV / e), 4.0e6 a 3 V;
#   crosstalk lineare, 7% a 3 V e ~21% a 8 V;
#   PDE a 450 nm = 66% (1 - exp(-dV / 2.15 V)): 40% a 2 V, 50% a 3 V, 65% a 8 V;
#   dark count proporzionale a dV (il datasheet lo da' solo a Vop: approssimazione).
DISP_GUADAGNO = 0.10       # sigma/G del singolo pixel: non nel datasheet
AFTERPULSE = 0.0           # "basso" nel datasheet, senza un numero
TAU_AP_NS = 50.0


def guadagno(dv):  return G_VOP * dv / SOVRA_VOP
def crosstalk(dv): return max(0.0, XT_VOP + 0.028 * (dv - SOVRA_VOP))
def pde(dv):       return 0.66 * (1 - np.exp(-dv / 2.15))
def dcr(dv):       return DCR_VOP_HZ * dv / SOVRA_VOP


# ============================================================
# ELETTRONICA: ingresso, amplificatore, V1742 (non nel datasheet)
# ============================================================
R_IN_OHM = 50.0
TAU_DISCESA_NS = R_IN_OHM * CT_PF * 1e-3   # 64 ns: la capacita' del SiPM sui 50 ohm d'ingresso
TAU_SALITA_NS = 3.0        # banda dell'amplificatore, ~100 MHz: non misurata
MV_PER_PE_VOP = 2.9        # ampiezza di 1 p.e. a Vop: fissa il guadagno dell'amplificatore. E'
                           # l'unico numero misurato al LNGS (run 0039 a 54 V, 2.75-3.02 mV):
                           # va sostituito col guadagno vero della scheda di amplificazione
POLARITA = +1              # impulsi positivi (DAQ del LNGS dal 2026-10-30)
DT_NS = 1 / 0.75           # V1742 a 750 MS/s
N_CAMPIONI = 1024          # 1.365 us
ADC_BIT, VPP_MV = 12, 1000.0
BASELINE_ADC = 420.0       # SiPM a DCOffset 0xB000 (daq-standalone, 2026-10-16)
RUMORE_ADC = 3.5           # bianco, come Signal_maker
T_EVENTO_NS = 300.0        # dove cade t = 0 dell'evento Geant4 nella finestra
PRE_BUIO_NS = 1000.0       # dark count anche prima della finestra
CANCELLO_NS = (T_EVENTO_NS - 50.0, T_EVENTO_NS + 700.0)   # per la carica
PANNELLI = ("top", "mid", "bot")
CANALI = {"top": 16, "mid": 17, "bot": 18}   # quale canale legge quale pannello: DA MISURARE

TEMPO = np.arange(N_CAMPIONI) * DT_NS
LSB_MV = VPP_MV / 2**ADC_BIT
E_CARICA = 1.602176634e-19


def forma(t):
    """Tensione su R_IN per un fotoelettrone di carica unitaria: due poli,
    salita (amplificatore) e discesa (C del SiPM su R_IN); integrale 1."""
    t = np.asarray(t, float)
    v = (np.exp(-t / TAU_DISCESA_NS) - np.exp(-t / TAU_SALITA_NS)) / (TAU_DISCESA_NS - TAU_SALITA_NS)
    return np.where(t > 0, v, 0.0)


PICCO_FORMA = forma(np.linspace(0, 200, 20001)).max()     # ns^-1
# guadagno dell'amplificatore che da' MV_PER_PE_VOP a Vop
AMP = MV_PER_PE_VOP * 1e-3 / (G_VOP * E_CARICA * 1e9 * R_IN_OHM * PICCO_FORMA)


class Catena:
    def __init__(self, dv, rng):
        self.dv, self.rng = dv, rng
        self.g, self.xt, self.dcr = guadagno(dv), crosstalk(dv), dcr(dv)
        self.pde_rel = pde(dv) / pde(SOVRA_VOP)
        # mV per fotoelettrone, all'ampiezza della forma di area 1
        self.mv_area = AMP * self.g * E_CARICA * 1e9 * R_IN_OHM * 1e3   # mV ns
        self.mv_pe = self.mv_area * PICCO_FORMA

    def descrivi(self):
        return (f"S13360-6075PE a dV = {self.dv:.2f} V: guadagno {self.g:.2e}, crosstalk {100*self.xt:.1f}%, "
                f"DCR {self.dcr/1e6:.2f} MHz, PDE relativa a Vop {self.pde_rel:.3f}; catena: salita "
                f"{TAU_SALITA_NS:.0f} ns, discesa {TAU_DISCESA_NS:.0f} ns, amplificatore x{AMP:.2f}, "
                f"1 p.e. = {self.mv_pe:.2f} mV = {self.mv_pe/LSB_MV:.1f} conteggi")

    def pixel(self, tempi):
        """Dai fotoelettroni primari (tempi in ns dalla finestra) ai pixel accesi:
        crosstalk (stesso tempo), saturazione, afterpulse (in ritardo, ridotti)."""
        n = len(tempi)
        if n == 0:
            return np.zeros(0), np.zeros(0)
        extra = self.rng.geometric(1 - self.xt, n) - 1 if self.xt > 0 else np.zeros(n, int)
        t = np.repeat(tempi, 1 + extra)
        a = np.ones(len(t))
        if len(t) > 1:                                   # pixel gia' accesi nello stesso evento
            tieni = N_PIXEL * (1 - np.exp(-len(t) / N_PIXEL)) / len(t)
            m = self.rng.random(len(t)) < tieni
            t, a = t[m], a[m]
        if AFTERPULSE > 0 and len(t):
            m = self.rng.random(len(t)) < AFTERPULSE
            dt = self.rng.exponential(TAU_AP_NS, m.sum())
            t = np.concatenate([t, t[m] + dt])
            a = np.concatenate([a, 1 - np.exp(-dt / TAU_DISCESA_NS)])
        a = a * np.maximum(0.0, self.rng.normal(1.0, DISP_GUADAGNO, len(a)))
        return t, a

    def buio(self):
        n = self.rng.poisson(self.dcr * (N_CAMPIONI * DT_NS + PRE_BUIO_NS) * 1e-9)
        return self.rng.uniform(-PRE_BUIO_NS, N_CAMPIONI * DT_NS, n)

    def onda(self, tempi_segnale):
        """Forma d'onda in ADC per un canale; tempi dei fotoelettroni del
        segnale in ns dalla finestra. Ritorna l'onda e quanti pixel."""
        t_s, a_s = self.pixel(np.asarray(tempi_segnale, float))
        t_b, a_b = self.pixel(self.buio())
        t, a = np.concatenate([t_s, t_b]), np.concatenate([a_s, a_b])
        v = np.zeros(N_CAMPIONI)
        if len(t):
            v = self.mv_area * (forma(TEMPO[:, None] - t[None, :]) @ a)
        adc = BASELINE_ADC + POLARITA * v / LSB_MV + self.rng.normal(0, RUMORE_ADC, N_CAMPIONI)
        return np.clip(np.round(adc), 0, 2**ADC_BIT - 1).astype(np.uint16), len(t_s)


def leggi_file(percorso):
    if os.path.isdir(percorso):
        return sorted(glob.glob(os.path.join(percorso, "*.root")))
    return sorted(glob.glob(percorso))


def eventi_fotoni(files, catena):
    import uproot
    for i, f in enumerate(files):
        with uproot.open(f) as r:
            e = r["Eventi"].arrays(["EventID", "SourceType"] + [f"Edep_panel_{k}_MeV" for k in PANNELLI], library="np")
            p = r["Fotoni"].arrays(["EventID", "PMT_ID", "Detected", "Arrival_Time_ns"], library="np")
        m = (p["PMT_ID"] >= 6) & (p["Detected"] == 1)
        if catena.pde_rel < 1:                           # meno PDE che a Vop: si scartano PE
            m &= catena.rng.random(len(m)) < catena.pde_rel
        ev, sid, tt = p["EventID"][m], p["PMT_ID"][m] - 6, p["Arrival_Time_ns"][m]
        o = np.argsort(ev, kind="stable")
        ev, sid, tt = ev[o], sid[o], tt[o]
        inizi = np.searchsorted(ev, e["EventID"]); fini = np.searchsorted(ev, e["EventID"], "right")
        for j in range(len(e["EventID"])):
            if fini[j] == inizi[j]:
                continue
            s = slice(inizi[j], fini[j])
            tempi = [tt[s][sid[s] == k] for k in range(3)]
            yield i, e["EventID"][j], e["SourceType"][j], [e[f"Edep_panel_{k}_MeV"][j] for k in PANNELLI], tempi


def eventi_mappa(files, catena, mappa):
    import uproot
    xe, ye, te = mappa["x_edges"], mappa["y_edges"], mappa["t_edges"]
    pe_mev = mappa["pe_per_mev"].copy()
    pe_mev[~np.isfinite(pe_mev)] = np.nanmean(pe_mev)    # bin senza elettroni: la media
    tpdf = mappa["t_pdf_per_x"].copy()
    media = tpdf.sum(axis=0); media /= media.sum()
    tpdf[tpdf.sum(axis=1) == 0] = media
    tcdf = np.cumsum(tpdf, axis=1); tcdf /= tcdf[:, -1:]
    rng = catena.rng
    colonne = ["EventID", "SourceType"] + [f"Edep_panel_{k}_MeV" for k in PANNELLI] + \
              [f"Pos_panel_{k}_{a}_mm" for k in PANNELLI for a in "XY"]
    for i, f in enumerate(files):
        with uproot.open(f) as r:
            e = r["Eventi"].arrays(colonne, library="np")
        for j in range(len(e["EventID"])):
            ed = [e[f"Edep_panel_{k}_MeV"][j] for k in PANNELLI]
            if max(ed) <= 0:
                continue
            tempi = []
            for k, en in zip(PANNELLI, ed):
                if en <= 0:
                    tempi.append(np.zeros(0)); continue
                ix = int(np.clip(np.digitize(e[f"Pos_panel_{k}_X_mm"][j], xe) - 1, 0, len(xe) - 2))
                iy = int(np.clip(np.digitize(e[f"Pos_panel_{k}_Y_mm"][j], ye) - 1, 0, len(ye) - 2))
                n = rng.poisson(pe_mev[ix, iy] * en * catena.pde_rel)
                b = np.searchsorted(tcdf[ix], rng.random(n))
                tempi.append(te[b] + rng.random(n) * (te[b + 1] - te[b]))
            yield i, e["EventID"][j], e["SourceType"][j], ed, tempi


def eventi_buio(n):
    for j in range(n):
        yield -1, j, -1, [0.0, 0.0, 0.0], [np.zeros(0)] * 3


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("modo", choices=("fotoni", "mappa", "buio"))
    ap.add_argument("ingresso")
    ap.add_argument("uscita")
    ap.add_argument("--mappa")
    ap.add_argument("--sovratensione", type=float, default=SOVRA_VOP)
    ap.add_argument("--onde", type=int, default=2000)
    ap.add_argument("--seme", type=int, default=1)
    a = ap.parse_args()
    rng = np.random.default_rng(a.seme)
    cat = Catena(a.sovratensione, rng)
    print(cat.descrivi())

    if a.modo == "buio":
        sorgente, files = eventi_buio(int(a.ingresso)), []
    else:
        files = leggi_file(a.ingresso)
        print(f"{len(files)} file da {a.ingresso}")
        if a.modo == "fotoni":
            sorgente = eventi_fotoni(files, cat)
        else:
            sorgente = eventi_mappa(files, cat, dict(np.load(a.mappa)))

    c0, c1 = (np.searchsorted(TEMPO, c) for c in CANCELLO_NS)
    onde, righe = [], []
    for i_file, ev_id, src, edep, tempi in sorgente:
        w = np.zeros((3, N_CAMPIONI), np.uint16)
        npe = np.zeros(3, int)
        for k in range(3):
            w[k], npe[k] = cat.onda(np.asarray(tempi[k]) + T_EVENTO_NS)
        s = POLARITA * (w.astype(float) - BASELINE_ADC)
        righe.append((i_file, ev_id, src, *edep, *[len(x) for x in tempi], *npe,
                      *s.max(axis=1), *(s[:, c0:c1].sum(axis=1) * DT_NS)))
        if len(onde) < a.onde:
            onde.append(w)
    # colonne: indice del file, EventID, SourceType, energia (3), PE del
    # segnale (3), pixel accesi dal segnale dopo crosstalk e saturazione (3),
    # ampiezza massima (3, conteggi sopra la baseline), carica nel cancello
    # (3, ADC ns)
    r = np.array(righe, float).reshape(-1, 18)
    nomi = ["indice_file", "EventID", "SourceType", "Edep_top_MeV", "Edep_mid_MeV", "Edep_bot_MeV",
            "pe_top", "pe_mid", "pe_bot", "pixel_top", "pixel_mid", "pixel_bot",
            "ampiezza_top", "ampiezza_mid", "ampiezza_bot", "carica_top", "carica_mid", "carica_bot"]
    param = {"modo": a.modo, "sovratensione_V": a.sovratensione, "guadagno": cat.g, "crosstalk": cat.xt,
             "dcr_Hz": cat.dcr, "pde_rel": cat.pde_rel, "mV_per_pe": cat.mv_pe, "amplificatore": AMP,
             "tau_salita_ns": TAU_SALITA_NS, "tau_discesa_ns": TAU_DISCESA_NS, "baseline_adc": BASELINE_ADC,
             "rumore_adc": RUMORE_ADC, "dt_ns": DT_NS, "t_evento_ns": T_EVENTO_NS, "cancello_ns": CANCELLO_NS,
             "canali": CANALI, "file": files}
    np.savez_compressed(a.uscita, onde=np.array(onde, np.uint16).reshape(-1, 3, N_CAMPIONI),
                        parametri=json.dumps(param), **{n: r[:, i] for i, n in enumerate(nomi)})
    print(f"{len(r)} eventi, {len(onde)} forme d'onda -> {a.uscita}")
    if len(r):
        for k, nome in enumerate(PANNELLI):
            pe = r[:, 6 + k]
            print(f"  {nome} (ch{CANALI[nome]}): PE del segnale media {pe.mean():.1f}, ampiezza mediana "
                  f"{np.median(r[:, 12 + k]):.0f} conteggi = {np.median(r[:, 12 + k]) * LSB_MV / cat.mv_pe:.1f} p.e. d'ampiezza")
        if a.modo == "buio":
            tw = N_CAMPIONI * DT_NS * 1e-9
            for soglia_pe in (1.5, 2.5, 3.5, 5.5, 7.5):
                p = np.mean(r[:, 12:15] * LSB_MV / cat.mv_pe > soglia_pe, axis=0)
                print(f"  massimo della finestra sopra {soglia_pe} p.e.: {p.round(4)}  (~{(p / tw).round(0)} Hz)")
        figura(a.uscita, np.array(onde), r, cat)


def figura(uscita, onde, r, cat):
    INK, INK2, GRID, SURF = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"
    COL = ("#2a78d6", "#eb6834", "#1baf7a")
    plt.rcParams.update({"figure.facecolor": SURF, "axes.facecolor": SURF, "savefig.facecolor": SURF,
                         "axes.edgecolor": INK2, "axes.grid": True, "grid.color": GRID, "axes.axisbelow": True,
                         "axes.spines.top": False, "axes.spines.right": False, "font.size": 9,
                         "xtick.top": False, "ytick.right": False, "xtick.direction": "out",
                         "ytick.direction": "out", "xtick.minor.visible": False, "ytick.minor.visible": False,
                         "legend.frameon": False, "legend.fontsize": 8, "axes.titlesize": 10,
                         "axes.titleweight": "bold"})
    fig, ax = plt.subplots(1, 3, figsize=(18, 4.4))
    if len(onde):
        idx = np.argsort(r[:len(onde), 6:9].sum(axis=1))[::-1][:1]
        for k in range(3):
            ax[0].plot(TEMPO, (onde[idx[0], k].astype(float) - BASELINE_ADC) * LSB_MV, color=COL[k], lw=1.0,
                       label=f"{PANNELLI[k]} (ch{CANALI[PANNELLI[k]]}), {int(r[idx[0], 6 + k])} PE")
    ax[0].set(xlabel="tempo nella finestra (ns)", ylabel="segnale (mV)", title="L'evento con piu' PE")
    ax[0].legend(loc="upper right")
    amp = r[:, 12:15] * LSB_MV / cat.mv_pe
    b = np.linspace(0, max(5, np.percentile(amp, 99.5)), 120)
    for k in range(3):
        ax[1].stairs(np.histogram(amp[:, k], b)[0], b, color=COL[k], lw=1.4, label=PANNELLI[k])
    ax[1].set(xlabel="ampiezza massima (p.e. d'ampiezza)", ylabel="eventi", yscale="log",
              title="Spettro d'ampiezza")
    ax[1].legend(loc="upper right")
    q1 = cat.mv_area / LSB_MV          # ADC ns per pixel
    for k in range(3):
        ax[2].plot(r[:, 9 + k], r[:, 15 + k] / q1, ".", ms=2, color=COL[k], alpha=0.5, label=PANNELLI[k])
    ax[2].set(xlabel="pixel accesi dal segnale", ylabel="carica nel cancello (p.e.)",
              title="Carica contro pixel del segnale")
    ax[2].legend(loc="upper left")
    fig.tight_layout()
    fig.savefig(uscita.replace(".npz", ".png"), dpi=120, bbox_inches="tight")


if __name__ == "__main__":
    main()
