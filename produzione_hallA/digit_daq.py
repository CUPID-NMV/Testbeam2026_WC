"""digit_daq.py -- la produzione Hall A (single_pan3) digitalizzata come una
presa dati del LNGS e scritta nel formato del DAQ (HDF5 v3 e il .log
accanto), per analizzarla con gli strumenti dei dati
(daq-standalone/tools/Distribuzioni_carica.ipynb, daqio).

La presa dati che si imita e' quella delle run LNGS__54-0V_0130-0157, dopo
l'inverter: 750 MS/s, 1012 campioni, PostTrigger 55%, self-trigger in OR sui
cinque canali con offset 450/600/600/80/80. Per ogni evento del MC:
  PMT (ch20, ch21): fotoni -> QE del PMT del prototipo, TTS e risoluzione del
    singolo PE come digit_mc.py -> forma del singolo PE misurata con
    l'inverter (libreria_digit_pmt_20261030.npz);
  SiPM (ch16, ch17, ch18 = pannello alto, mezzo, basso: l'abbinamento e'
    quello di SiPM_maker.py, da misurare): la produzione ha l'ottica dei
    pannelli spenta, quindi PE ~ Poisson(mappa(x, y) x energia) e tempi dalla
    mappa (mappa_luce.npz), poi crosstalk, saturazione dei pixel, guadagno,
    dark count e forma di 1 p.e. misurati al LNGS (libreria_sipm_0039.npz,
    la CatenaMisurata di SiPM_maker.py);
  rumore: tracce vere di solo piedistallo delle due librerie, spostate al
    livello di base delle run vere;
  trigger: un canale scatta se la sua ampiezza, misurata sulla finestra con
    rumore e buio come la misura chi analizza i dati (base mediana in 10-150
    ns), passa la soglia. Le soglie in conteggi sono il bordo basso della
    distribuzione delle ampiezze nelle run vere (vicino a soglie_pe_20261030.json,
    che misura dalla mediana di tutta la traccia). La finestra si mette come
    la mette il DAQ: l'attraversamento della soglia del canale che scatta per
    primo cade a un istante tirato dalla distribuzione misurata nelle run
    vere, a parte per SiPM e PMT;
  tempi: i gamma sono un flusso di 24 ore al rate della produzione, ogni
    gamma al suo istante (EventID / rate); i muoni sono quelli di muoni_2400h
    che cadono nelle stesse 24 ore. Trigger Time Tag a 17.07 ns su 31 bit.
Il ritardo dei SiPM rispetto ai PMT (cavi, catene) si misura: negli eventi
col trigger dei PMT e un segnale in un SiPM, la distanza fra i fronti nei
dati contro quella nel MC.

Non c'e': tempo morto del DAQ, pile-up di eventi diversi nella stessa
finestra, sottoelongazione dei SiPM dopo un impulso grande, rumore comune fra
i canali. Gli impulsi propri dei PMT ci sono solo quanto ne contengono le
tracce vere di rumore.

Uso (~/venvs/jupyter-global/bin/python, che ha uproot e h5py):
  python3 -I digit_daq.py prepara USCITA RUN.h5.gz [...] [--scala-luce S] [--libreria-pmt F.npz]
      dalle run vere: soglie, tempi del trigger, livelli di base, fronti per i ritardi;
      la libreria dei PMT scelta qui finisce in parametri.json e la usano
      pezzi, run e campione (di serie libreria_digit_pmt_20261030.npz; quella
      senza la diafonia ch16 -> ch21 e' libreria_digit_pmt_20261109.npz,
      vedi libreria_pmt_pulita.py). Con --efficienza-pmt F.json (2026-11-10) il
      trigger dei PMT non e' piu' un gradino: sopra la soglia un PMT scatta
      con probabilita' Phi((A - A50) / s), coi parametri misurati nello scan
      del self-trigger (efficienza_trigger_pmt_20261030.json). Il numero
      casuale viene da un generatore a parte, figlio di quello del pezzo: il
      resto della sequenza casuale non cambia
  python3 -I digit_daq.py pezzi USCITA [K ...] [--processi N]
      digitalizza i pezzi K della produzione gamma, "mu" per i muoni; senza K tutti
  python3 -I digit_daq.py ritardi USCITA
      ritardi dei SiPM dai pezzi gia' fatti; poi i pezzi si rifanno
  python3 -I digit_daq.py run USCITA [--eventi-per-run 100000] [--prefisso P]
      le run: PNNNN_750Ms_55PT.h5.gz e .log in USCITA/run/, P di serie
      MC_HallA__54-0V_ (in Distribuzioni_carica: CARTELLA = USCITA/run,
      PREFISSO = P, RUN = range(1, N + 1))
  python3 -I digit_daq.py campione USCITA CARTELLA --parametri DAQ --rate HZ [--luce fotoni]
      un campione a parte, per esempio quello di ottica_accesa.sh, coi
      parametri di DAQ (un'uscita di prepara): run sue in USCITA/run/, gli
      eventi uno dopo l'altro a HZ eventi generati al secondo
Con --scala-luce S i PE dei pannelli sono S volte quelli della mappa (di
serie 1: la mappa com'e'). Con --luce fotoni, per un campione con l'ottica
dei pannelli accesa, i PE dei SiPM sono i fotoni rivelati da Geant4 (PMT_ID
6-8, Detected = 1) coi loro tempi invece della mappa; il resto della catena
non cambia.
"""
import argparse
import glob
import gzip
import json
import math
import multiprocessing as mp
import os
import shutil
import sys
import time

import h5py
import numpy as np

QUI = os.path.dirname(os.path.abspath(__file__))
RADICE = os.path.dirname(QUI)
sys.path.insert(0, os.path.join(RADICE, "Analysis_script"))
import SiPM_maker as SM  # noqa: E402  la catena misurata dei SiPM

MIS = os.path.expanduser("~/daq-standalone/measurements")
LIB_PMT = os.path.join(MIS, "libreria_digit_pmt_20261030.npz")
LIB_SIPM = os.path.join(MIS, "libreria_sipm_0039.npz")
SOGLIE = os.path.join(MIS, "soglie_pe_20261030.json")
MAPPA = os.path.join(QUI, "mappa_luce.npz")
GAMMA = os.path.join(RADICE, "data", "produzione_hallA", "gamma_24h")
MUONI = os.path.join(RADICE, "data", "produzione_hallA", "muoni_2400h")

# --- la presa dati che si imita (log della 0130) ---
CANALI = [16, 17, 18, 20, 21]
SIPM, PMT = (0, 1, 2), (3, 4)                    # indici in CANALI
PANNELLO = ("top", "mid", "bot")                 # SIPM[k] legge PANNELLO[k] (SiPM_maker.CANALI)
OFFSET = [450, 600, 600, 80, 80]                 # SelfTriggerThresholdOffset
SOGLIA_ASSOLUTA = [1458, 1590, 1628, 1102, 1088] # SelfTriggerThreshold nell'header della 0130
NS = 1012                                        # RecordLength 1024 meno TailCut 12
DT = 1e3 / 750.0                                 # ns
FINESTRA_NS = NS * DT
TTT_NS, TTT_GIRO = 17.07, 2 ** 31
FONDO = 4095
LSB_MV = 1000.0 / 4096
PED_NS = (10.0, 150.0)                           # base per evento, come la misura dei dati
PED = (np.arange(NS) * DT >= PED_NS[0]) & (np.arange(NS) * DT < PED_NS[1])
QUIETO_PE = 20                                   # SiPM sotto 20 p.e.: nessun segnale (base, ritardi)
MARGINE = 25.0                                   # conteggi: il rumore puo' portare sopra soglia un segnale
                                                 # fin qui sotto (rumore ~3 conteggi, un dark count ~11)

# --- PMT del prototipo: come digit_mc.py ---
QE_WL = [200, 250, 300, 350, 380, 400, 420, 450, 500, 550, 600, 650, 700]
QE = [0.18, 0.25, 0.30, 0.34, 0.35, 0.34, 0.32, 0.28, 0.18, 0.08, 0.03, 0.005, 0.0]
SPE_RES = 0.40
TTS_SIGMA = 0.3 / 2.355

COLONNE = (["EventID", "SourceType", "E_gen_MeV", "Edep_water_MeV", "TrackL_water_mm"]
           + ["Edep_fiber_%s_MeV" % s for s in "LCR"]
           + ["Edep_panel_%s_MeV" % p for p in PANNELLO]
           + ["Pos_panel_%s_%s_mm" % (p, a) for p in PANNELLO for a in "XY"])


# ======================================================================
#  grandezze di ogni evento: le stesse sui dati e sul MC
# ======================================================================

def caratteristiche(w, soglie):
    """w (n, 5, NS) in conteggi grezzi. Per canale: base (mediana in PED_NS),
    ped (media in PED_NS), ampiezza (massimo meno base, senza gli ultimi 20
    campioni), istante del massimo, attraversamento della soglia del trigger
    e passaggio al 50% del fronte, in ns dall'inizio della finestra."""
    n = len(w)
    t = np.arange(NS) * DT
    ped = (t >= PED_NS[0]) & (t < PED_NS[1])
    idx = np.arange(NS)
    r = {k: np.full((n, 5), np.nan, np.float32) for k in ("base", "ped", "amp", "tmax", "tx", "t50")}
    for j in range(5):
        y = w[:, j].astype(np.float32)
        b = np.median(y[:, ped], axis=1)
        x = y - b[:, None]
        x[:, -20:] = 0
        im = x.argmax(axis=1)
        am = x[np.arange(n), im]
        sopra = x >= soglie[j]
        i1 = np.where(sopra.any(axis=1), sopra.argmax(axis=1), 0)
        ok = i1 > 0
        y0, y1 = x[ok, i1[ok] - 1], x[ok, i1[ok]]
        r["tx"][ok, j] = (i1[ok] - 1 + (soglie[j] - y0) / np.maximum(y1 - y0, 1e-6)) * DT
        meta = 0.5 * am
        prima = np.where((x < meta[:, None]) & (idx[None, :] < im[:, None]), idx[None, :], -1).max(axis=1)
        ok = (prima >= 0) & (am > 30)
        i0 = prima[ok]
        x0, x1 = x[ok, i0], x[ok, i0 + 1]
        r["t50"][ok, j] = (i0 + (meta[ok] - x0) / np.maximum(x1 - x0, 1e-6)) * DT
        r["base"][:, j], r["ped"][:, j], r["amp"][:, j], r["tmax"][:, j] = b, y[:, ped].mean(axis=1), am, im * DT
    return r


def leggi_run(percorso, blocco=5000):
    """Blocchi (n, 5, NS) di una run vera (.h5 o .h5.gz) nei canali CANALI."""
    tmp = None
    if percorso.endswith(".gz"):
        tmp = os.path.join(os.path.abspath(PARAMETRI_DIR), ".run_%d.h5" % os.getpid())
        with gzip.open(percorso) as fi, open(tmp, "wb") as fo:
            shutil.copyfileobj(fi, fo, 16 << 20)
    try:
        with h5py.File(tmp or percorso, "r") as f:
            lista = [int(c) for c in f["/config"].attrs["ChannelList"]]
            ns = int(f["/config"].attrs["SamplesPerChannel"])
            assert ns == NS and abs(float(f["/config"].attrs["SamplingTime"]) * 1e9 - DT) < 1e-6
            ds = f["/events/waveforms"]
            for a in range(0, ds.shape[0], blocco):
                w = np.asarray(ds[a:a + blocco])
                yield np.stack([w[:, lista.index(c) * ns:(lista.index(c) + 1) * ns] for c in CANALI], axis=1)
    finally:
        if tmp and os.path.exists(tmp):
            os.unlink(tmp)


def durata_dal_log(percorso):
    import re
    try:
        m = re.search(r"Acquisition time: ([0-9.]+)", open(percorso.replace(".h5.gz", ".log").replace(".h5", ".log")).read())
        return float(m.group(1)) if m else None
    except OSError:
        return None


# ======================================================================
#  la catena: PMT, SiPM, mappa di luce
# ======================================================================

class Mappa:
    """PE per MeV in funzione di (x, y) e tempi d'arrivo in funzione di x,
    come SiPM_maker.eventi_mappa."""

    def __init__(self, percorso, fattore):
        m = np.load(percorso)
        self.xe, self.ye, self.te = m["x_edges"], m["y_edges"], m["t_edges"]
        pe = m["pe_per_mev"].copy()
        pe[~np.isfinite(pe)] = np.nanmean(pe)
        self.pe = pe * fattore
        tpdf = m["t_pdf_per_x"].copy()
        media = tpdf.sum(axis=0)
        tpdf[tpdf.sum(axis=1) == 0] = media / media.sum()
        self.tcdf = np.cumsum(tpdf, axis=1)
        self.tcdf /= self.tcdf[:, -1:]

    def attesi(self, x, y, e):
        ix = np.clip(np.digitize(x, self.xe) - 1, 0, len(self.xe) - 2)
        iy = np.clip(np.digitize(y, self.ye) - 1, 0, len(self.ye) - 2)
        return self.pe[ix, iy] * e, ix

    def tempi(self, ix, n, rng):
        b = np.minimum(np.searchsorted(self.tcdf[ix], rng.random(n)), len(self.te) - 2)
        return self.te[b] + rng.random(n) * (self.te[b + 1] - self.te[b])


class Catena:
    """Tutto quello che serve per digitalizzare un evento, coi parametri di
    `prepara` (P) e un generatore (rng) che si cambia a ogni pezzo."""

    def __init__(self, P, tx):
        self.P = P
        self.soglia = np.array(P["soglia_conteggi"])
        self.rit = np.array(P["ritardo_ns"])
        self.tx_sipm, self.tx_pmt = tx["tx_sipm"], tx["tx_pmt"]
        L = np.load(P.get("libreria_pmt", LIB_PMT))     # parametri.json senza la chiave: la libreria di prima
        self.K = float(L["K_adcns_per_pe"])
        self.xp = L["forma_x_ns"]
        self.fp = [L["forma_ch20"] * self.K, L["forma_ch21"] * self.K]       # conteggi per PE di guadagno 1
        self.picco_pmt = [f.max() for f in self.fp]
        S = np.load(LIB_SIPM)
        self.cat = SM.CatenaMisurata(LIB_SIPM, P["sovratensione_V"], None)
        self.picco_sipm = [c["forma"].max() for c in self.cat.c]
        self.rumore = {}
        for j, c in enumerate(CANALI):
            r = (FONDO - L["rumore_ch%d" % c].astype(np.float32)) if j in PMT else S["rumore_ch%d" % c].astype(np.float32)
            self.rumore[j] = r + P["spostamento_base"][j]
        self.mappa = Mappa(MAPPA, P["scala_luce"] * self.cat.pde_rel)
        self.rng = None
        # efficienza del trigger dei PMT in ampiezza (parametri.json senza la chiave: il gradino di prima)
        eff = P.get("efficienza_pmt")
        self.eff = None if not eff else [(float(eff[str(c)]["A50"]), float(eff[str(c)]["s"])) for c in (20, 21)]
        self.rng_eff = None

    def usa(self, rng):
        self.rng = self.cat.rng = rng
        # generatore a parte per l'efficienza: spawn non tocca la sequenza di rng
        self.rng_eff = rng.spawn(1)[0] if self.eff else None

    def efficienza(self, jj, a):
        a50, s = self.eff[jj]
        return 0.5 * (1.0 + math.erf((a - a50) / (s * math.sqrt(2.0))))

    # --- forme ---
    def onda_pmt(self, jj, t, g, griglia):
        d = griglia[:, None] - t[None, :]
        return np.interp(d.ravel(), self.xp, self.fp[jj], left=0.0, right=0.0).reshape(d.shape) @ g

    def onda_sipm(self, k, t, a, griglia):
        return self.cat.forma_k(k, griglia[:, None] - t[None, :]) @ a if len(t) else np.zeros(len(griglia))

    def buio(self, k, lo, hi):
        c = self.cat.c[k]
        n = self.rng.poisson(c["dcr"] * (hi - lo) * 1e-9)
        return self.cat.pixel_k(self.rng.uniform(lo, hi, n), k)

    @staticmethod
    def griglia(fase, lo, hi):
        return fase + DT * np.arange(np.floor((lo - fase) / DT), np.ceil((hi - fase) / DT) + 1)

    @staticmethod
    def attraversa(v, g, s):
        i = int(np.argmax(v >= s))
        if i == 0:
            return g[0]
        return g[i - 1] + (s - v[i - 1]) / max(v[i] - v[i - 1], 1e-9) * DT

    # --- un evento ---
    def misura(self, w):
        """Ampiezza e attraversamento della soglia di ogni canale di una
        finestra, come in caratteristiche(): quello che vede chi analizza."""
        x = w - np.median(w[:, PED], axis=1, keepdims=True)
        x[:, -20:] = 0
        amp, tx = x.max(axis=1), np.full(5, np.inf)
        for j in np.where(amp >= self.soglia)[0]:
            i = int(np.argmax(x[j] >= self.soglia[j]))
            tx[j] = (i - 1 + (self.soglia[j] - x[j, i - 1]) / max(x[j, i] - x[j, i - 1], 1e-6)) * DT if i > 0 else 0.0
        return amp, tx

    def evento(self, pe_pmt, n_pan, ix_pan, t_pan=None):
        """pe_pmt: {0/1: (tempi ns, guadagni)} dei PMT; n_pan, ix_pan: PE e
        colonna della mappa per pannello; t_pan, se i PE vengono dai fotoni
        veri, i loro tempi per pannello. Ritorna None se il trigger non
        scatta, se no (forme d'onda (5, NS) int16, ampiezze del solo segnale
        (5), canale del trigger, istante dell'attraversamento nella finestra).

        Il trigger si decide come il comparatore: sul segnale CON il rumore.
        Prima il segnale da solo dice chi puo' passare (soglia meno MARGINE)
        e dove mettere la finestra; poi, sulla finestra con rumore e buio, un
        canale scatta se la sua ampiezza misurata passa la soglia, e la
        finestra si sposta finche' l'attraversamento del primo canale che
        scatta cade all'istante tirato dai dati."""
        rng = self.rng
        fase = rng.random() * DT
        livello = self.soglia - MARGINE
        amp, cross = np.zeros(5), np.full(5, np.inf)
        segnale, buio = {}, {}
        for jj, (t, g) in pe_pmt.items():
            j = PMT[jj]
            t = t + self.rit[j]
            segnale[j] = (t, g)
            gr = self.griglia(fase, t.min() - 6.0, min(t.max(), t.min() + 3000.0) + 20.0)
            v = self.onda_pmt(jj, t, g, gr)
            amp[j] = v.max()
            if amp[j] >= livello[j]:
                cross[j] = self.attraversa(v, gr, livello[j])
        for k in SIPM:
            if n_pan[k] == 0:
                continue
            t = (self.mappa.tempi(ix_pan[k], n_pan[k], rng) if t_pan is None else t_pan[k]) + self.rit[k]
            tp, ap = self.cat.pixel_k(t, k)
            segnale[k] = (tp, ap)
            # basta a passare la soglia? (limite largo: tutti i pixel insieme, guadagno +3 sigma)
            if len(ap) == 0 or ap.sum() * self.picco_sipm[k] * 1.05 < livello[k]:
                continue
            t0 = tp.min()
            lo, hi = t0 - self.cat.c[k]["pre_ns"] - 1500.0, t0 + 3500.0
            buio[k] = (*self.buio(k, lo, hi), lo, hi)
            gr = self.griglia(fase, t0 - 20.0, t0 + 500.0)
            v = self.onda_sipm(k, np.concatenate([tp, buio[k][0]]), np.concatenate([ap, buio[k][1]]), gr)
            amp[k] = v.max()
            if amp[k] >= livello[k]:
                cross[k] = self.attraversa(v, gr, livello[k])
        j0 = int(np.argmin(cross))
        if not np.isfinite(cross[j0]):
            return None
        # rumore e buio fissati una volta: la finestra si sposta, loro no
        rumore = np.stack([self.rumore[j][rng.integers(len(self.rumore[j]))] for j in range(5)])
        for k in SIPM:
            if k not in buio:
                lo, hi = cross[j0] - self.cat.c[k]["pre_ns"] - 1500.0, cross[j0] + 3500.0
                buio[k] = (*self.buio(k, lo, hi), lo, hi)
        tipo = j0 in SIPM
        tx = float(rng.choice(self.tx_sipm if tipo else self.tx_pmt))
        w0 = fase + DT * np.round((cross[j0] - tx - fase) / DT)
        u_eff = self.rng_eff.random(len(PMT)) if self.eff else None     # una volta per evento e per PMT
        for _ in range(4):
            gw = w0 + DT * np.arange(NS)
            w = rumore.copy()
            for j in segnale:
                if j in PMT:
                    w[j] += self.onda_pmt(PMT.index(j), *segnale[j], gw)
            for k in SIPM:
                tb, ab, lo, hi = buio[k]
                m = (tb >= w0 - self.cat.c[k]["pre_ns"]) & (tb < w0 + FINESTRA_NS)
                ts, as_ = segnale.get(k, (np.zeros(0), np.zeros(0)))
                w[k] += self.onda_sipm(k, np.concatenate([ts, tb[m]]), np.concatenate([as_, ab[m]]), gw)
            w = np.clip(np.round(w), 0, FONDO)
            a_mis, tx_mis = self.misura(w)
            if self.eff:                         # sopra la soglia il PMT scatta con probabilita' eps(A)
                for jj, j in enumerate(PMT):
                    if np.isfinite(tx_mis[j]) and u_eff[jj] >= self.efficienza(jj, a_mis[j]):
                        tx_mis[j] = np.inf
            j1 = int(np.argmin(tx_mis))
            if not np.isfinite(tx_mis[j1]):
                return None                      # col rumore non passa nessuno: niente trigger
            if (j1 in SIPM) != tipo:             # scatta per primo l'altro tipo di rivelatore
                tipo = j1 in SIPM
                tx = float(rng.choice(self.tx_sipm if tipo else self.tx_pmt))
            passo = np.round((tx_mis[j1] - tx) / DT)
            if passo == 0:
                break
            w0 += DT * passo
        return w.astype(np.int16), amp.astype(np.float32), j1, float(tx_mis[j1])


# ======================================================================
#  un pezzo della produzione
# ======================================================================

def leggi_produzione(cartella):
    p = dict(r.strip().split(None, 1) for r in open(os.path.join(cartella, "produzione.txt")) if r.strip())
    return float(p["rate_Hz"]), int(p["pezzi"].split(" da ")[1].split()[0]), int(p["eventi_totali"])


def digitalizza(catena, file_root, t0_s, rate, durata_s, sorgente):
    """Gli eventi di un file che fanno scattare il trigger, con la verita' del MC.
    I PE dei pannelli vengono dalla mappa, o dai fotoni rivelati se
    parametri.json ha "luce": "fotoni" (campione con l'ottica accesa)."""
    import uproot
    rng = catena.rng
    fotoni = catena.P.get("luce", "mappa") == "fotoni"
    with uproot.open(file_root) as f:
        e = f["Eventi"].arrays(COLONNE, library="np")
        ph = f["Fotoni"].arrays(["EventID", "PMT_ID", "E_Hit_eV", "Arrival_Time_ns", "Detected"], library="np")
    sp = ph["PMT_ID"] >= 6                       # SiPM: ci sono solo con l'ottica dei pannelli accesa
    sip = {k: v[sp] for k, v in ph.items()}
    ph = {k: v[~sp] for k, v in ph.items()}
    o = np.argsort(e["EventID"], kind="stable")
    e = {k: v[o] for k, v in e.items()}
    eid = e["EventID"].astype(np.int64)
    t_ev = t0_s + (eid + rng.random(len(eid))) / rate
    tieni = t_ev < durata_s
    # PMT: fotoelettroni, ordinati per evento
    acc = rng.random(len(ph["EventID"])) < np.interp(1240.0 / ph["E_Hit_eV"], QE_WL, QE, left=0, right=0)
    pev = ph["EventID"][acc].astype(np.int64)
    pid = ph["PMT_ID"][acc].astype(int)
    pt = ph["Arrival_Time_ns"][acc] + rng.normal(0, TTS_SIGMA, acc.sum())
    pg = np.maximum(0.0, rng.normal(1.0, SPE_RES, acc.sum()))
    o = np.argsort(pev, kind="stable")
    pev, pid, pt, pg = pev[o], pid[o], pt[o], pg[o]
    s0, s1 = np.searchsorted(pev, eid), np.searchsorted(pev, eid, "right")
    n_pan = np.zeros((len(eid), 3), np.int64)
    ix_pan = np.zeros((len(eid), 3), np.int64)
    if fotoni:
        # SiPM: i fotoni rivelati da Geant4 (PDE gia' applicata), coi loro
        # tempi; scala_luce e pde_rel sotto 1 ne tolgono, aggiungerne no
        f_luce = catena.P["scala_luce"] * catena.cat.pde_rel
        if f_luce > 1:
            raise SystemExit("con la luce dai fotoni scala_luce x pde_rel non puo' superare 1 (%g)" % f_luce)
        m = sip["Detected"] == 1
        if f_luce < 1:
            m &= rng.random(len(m)) < f_luce
        sev, sk, st = sip["EventID"][m].astype(np.int64), sip["PMT_ID"][m] - 6, sip["Arrival_Time_ns"][m]
        o = np.argsort(sev, kind="stable")
        sev, sk, st = sev[o], sk[o], st[o]
        f0, f1 = np.searchsorted(sev, eid), np.searchsorted(sev, eid, "right")
        for k in SIPM:
            c = np.r_[0, np.cumsum(sk == k)]
            n_pan[:, k] = c[f1] - c[f0]
    else:
        # SiPM: PE dalla mappa
        for k, p in enumerate(PANNELLO):
            E = e["Edep_panel_%s_MeV" % p]
            m = E > 0
            mu, ix = catena.mappa.attesi(e["Pos_panel_%s_X_mm" % p][m], e["Pos_panel_%s_Y_mm" % p][m], E[m])
            n_pan[m, k] = rng.poisson(mu)
            ix_pan[m, k] = ix
    # candidati: un PE in un PMT, o abbastanza PE in un pannello. Il limite e'
    # largo -- tutti i pixel insieme, crosstalk piu' 3 radice(N) + 3 pixel,
    # guadagno +5% -- e sta ~300 conteggi sopra l'ampiezza possibile: copre
    # anche il MARGINE del rumore, che qui non serve sottrarre. (Sottrarlo
    # cambia i numeri casuali e non i risultati: i run del 2026-11-05 sono
    # fatti cosi'.)
    xt = np.array([c["xt"] for c in catena.cat.c])
    lim = (n_pan / (1 - xt) + 3 * np.sqrt(n_pan) + 3) * np.array(catena.picco_sipm) * 1.05
    cand = tieni & ((s1 > s0) | (lim >= catena.soglia[list(SIPM)]).any(axis=1))
    W, R = [], {k: [] for k in ("riga", "amp", "canale_trigger", "tx_ns")}
    for i in np.where(cand)[0]:
        sel = slice(s0[i], s1[i])
        pe_pmt = {jj: (pt[sel][pid[sel] == jj], pg[sel][pid[sel] == jj]) for jj in (0, 1)
                  if (pid[sel] == jj).any()}
        t_pan = [st[f0[i]:f1[i]][sk[f0[i]:f1[i]] == k] for k in SIPM] if fotoni else None
        out = catena.evento(pe_pmt, n_pan[i], ix_pan[i], t_pan)
        if out is None:
            continue
        W.append(out[0])
        for k, v in zip(("riga", "amp", "canale_trigger", "tx_ns"), (i,) + out[1:]):
            R[k].append(v)
    riga = np.array(R["riga"], int)
    npe = np.stack([np.r_[0, np.cumsum(pid == jj)][s1] - np.r_[0, np.cumsum(pid == jj)][s0] for jj in (0, 1)], axis=1)
    return {"wf": np.array(W, np.int16).reshape(-1, 5, NS), "t_s": t_ev[riga], "sorgente": np.full(len(riga), sorgente, np.int8),
            "EventID": eid[riga].astype(np.int32), "amp_segnale": np.array(R["amp"], np.float32).reshape(-1, 5),
            "canale_trigger": np.array(R["canale_trigger"], np.int8), "tx_ns": np.array(R["tx_ns"], np.float32),
            "npe_pmt": npe[riga].astype(np.int32), "pe_pannelli": n_pan[riga].astype(np.int32),
            **{k: e[k][riga].astype(np.float32) for k in COLONNE if k not in ("EventID",) and not k.startswith("Pos_")},
            "n_candidati": int(cand.sum()), "n_righe": int(len(eid))}


_CATENA = None


def _lavora(lavoro):
    """Un pezzo, in un processo a parte: (nome, file, t0, rate, durata, sorgente, seme, uscita)."""
    global _CATENA
    nome, file_root, t0, rate, durata, sorgente, seme, uscita = lavoro
    if _CATENA is None:
        P = json.load(open(os.path.join(PARAMETRI_DIR, "parametri.json")))
        _CATENA = Catena(P, np.load(os.path.join(PARAMETRI_DIR, "tempi_trigger.npz")))
    _CATENA.usa(np.random.default_rng(seme))
    c0 = time.time()
    r = digitalizza(_CATENA, file_root, t0, rate, durata, sorgente)
    tmp = uscita + ".tmp.npz"
    np.savez_compressed(tmp, **r)
    os.replace(tmp, uscita)
    return "%s: %d righe, %d candidati, %d col trigger, %.0f s" % (nome, r["n_righe"], r["n_candidati"], len(r["t_s"]), time.time() - c0)


# ======================================================================
#  comandi
# ======================================================================

PARAMETRI_DIR = "."


def cmd_prepara(a):
    """Dalle run vere: soglie, tempi del trigger, livelli di base, fronti."""
    # Soglie: la calibrazione del 30/10 (offset -> mV) misura l'ampiezza dalla
    # mediana di TUTTA la traccia, che per i SiPM sta sopra la base perche'
    # l'impulso ha una coda lunga. Qui la base e' prima dell'impulso: la soglia
    # si rilegge dai dati, nel bordo basso della distribuzione delle ampiezze
    # (quantile 0.5% sopra 0.97 volte la calibrazione). Sui SiPM il bordo e'
    # netto; sui PMT e' morbido (210-250 conteggi) e il quantile torna con la
    # calibrazione.
    s = json.load(open(SOGLIE))
    soglia_cal = [(s[str(c)]["mv_per_offset_bordo"] * o + s[str(c)]["intercetta_bordo_mv"]) / LSB_MV
                  for c, o in zip(CANALI, OFFSET)]
    S = np.load(LIB_SIPM)
    pe_sipm = [float(S["pe_conteggi_ch%d" % c]) for c in CANALI[:3]]
    quieto = [QUIETO_PE * p for p in pe_sipm] + [73.0, 73.0]        # PMT: mezzo PE
    amp = np.concatenate([caratteristiche(w, soglia_cal)["amp"] for run in a.dati for w in leggi_run(run)])
    soglia = [float(np.quantile(amp[(amp[:, j] >= 0.97 * soglia_cal[j]) & (amp[:, j] < 3 * soglia_cal[j]), j], 0.005))
              for j in range(5)]
    print("soglie (conteggi sopra la base): calibrazione %s, bordo nei dati %s" % (np.round(soglia_cal), np.round(soglia)))
    F, durata, n_ev = [], 0.0, 0
    for run in a.dati:
        parti = [caratteristiche(w, soglia) for w in leggi_run(run)]
        r = {k: np.concatenate([p[k] for p in parti]) for k in parti[0]}
        F.append(r)
        d = durata_dal_log(run)
        durata += d or np.nan
        n_ev += len(r["amp"])
        print("%s: %d eventi, %.0f s" % (os.path.basename(run), len(r["amp"]), d or np.nan), flush=True)
    r = {k: np.concatenate([x[k] for x in F]) for k in F[0]}
    sopra = r["amp"] >= np.array(soglia)
    solo_sipm = sopra[:, :3].any(axis=1) & ~sopra[:, 3:].any(axis=1)
    solo_pmt = sopra[:, 3:].any(axis=1) & ~sopra[:, :3].any(axis=1)
    tx = np.where(sopra, r["tx"], np.inf)
    tx_sipm, tx_pmt = tx[solo_sipm, :3].min(axis=1), tx[solo_pmt, 3:].min(axis=1)
    # livello di base: la media in PED_NS negli eventi senza segnale nel canale
    base_dati = [float(np.median(r["ped"][r["amp"][:, j] < quieto[j], j])) for j in range(5)]
    # stessa grandezza sulle tracce della libreria (SiPM: col buio sopra)
    lib_pmt = os.path.abspath(a.libreria_pmt)
    P0 = {"soglia_conteggi": soglia, "ritardo_ns": [0.0] * 5, "spostamento_base": [0.0] * 5,
          "sovratensione_V": a.sovratensione, "scala_luce": a.scala_luce, "libreria_pmt": lib_pmt}
    if a.efficienza_pmt:
        P0["efficienza_pmt"] = json.load(open(a.efficienza_pmt))
    cat = Catena(P0, {"tx_sipm": tx_sipm, "tx_pmt": tx_pmt})
    cat.usa(np.random.default_rng(12345))
    gw = np.arange(NS) * DT
    ped = (gw >= PED_NS[0]) & (gw < PED_NS[1])
    sposta = []
    for j in range(5):
        if j in PMT:
            base_lib = float(np.median(cat.rumore[j][:, ped].mean(axis=1)))
        else:
            v = []
            for i in range(2000):
                tb, ab = cat.buio(j, -cat.cat.c[j]["pre_ns"], FINESTRA_NS)
                v.append((cat.rumore[j][cat.rng.integers(len(cat.rumore[j]))] + cat.onda_sipm(j, tb, ab, gw))[ped].mean())
            base_lib = float(np.median(v))
        sposta.append(base_dati[j] - base_lib)
    # fronti per i ritardi: trigger dei PMT, segnale nel SiPM
    fronti = {}
    for k in SIPM:
        m = solo_pmt & (r["amp"][:, k] > quieto[k])
        jp = np.where(r["amp"][m, 3] >= r["amp"][m, 4], 3, 4)
        fronti["d50_dati_ch%d" % CANALI[k]] = (r["t50"][m, k] - r["t50"][m][np.arange(m.sum()), jp]).astype(np.float32)
    P = {"soglia_conteggi": soglia, "soglia_calibrazione": soglia_cal, "offset": OFFSET, "ritardo_ns": [0.0] * 5,
         "spostamento_base": sposta, "margine_conteggi": MARGINE,
         "base_dati": base_dati, "quieto_conteggi": quieto, "sovratensione_V": a.sovratensione, "pde_rel": cat.cat.pde_rel,
         "scala_luce": a.scala_luce, "dati": [os.path.basename(x) for x in a.dati], "dati_eventi": n_ev, "dati_durata_s": durata,
         "dati_rate_sopra_soglia_hz": (sopra.sum(axis=0) / durata).tolist(), "dati_rate_hz": n_ev / durata,
         "tx_sipm_mediana_ns": float(np.median(tx_sipm)), "tx_pmt_mediana_ns": float(np.median(tx_pmt)),
         "librerie": [lib_pmt, LIB_SIPM], "libreria_pmt": lib_pmt, "mappa": MAPPA, "soglie": SOGLIE}
    if a.efficienza_pmt:
        P["efficienza_pmt"] = P0["efficienza_pmt"]
        P["efficienza_pmt_file"] = os.path.abspath(a.efficienza_pmt)
    json.dump(P, open(os.path.join(a.uscita, "parametri.json"), "w"), indent=1)
    np.savez(os.path.join(a.uscita, "tempi_trigger.npz"), tx_sipm=tx_sipm.astype(np.float32),
             tx_pmt=tx_pmt.astype(np.float32), **fronti)
    print(json.dumps({k: P[k] for k in ("soglia_conteggi", "base_dati", "spostamento_base", "dati_rate_sopra_soglia_hz",
                                        "dati_rate_hz", "tx_sipm_mediana_ns", "tx_pmt_mediana_ns")}, indent=1))
    for k, v in fronti.items():
        print("%s: %d eventi, mediana %.1f ns" % (k, len(v), np.nanmedian(v)))


def lavori(a):
    """I pezzi da fare: (nome, file, t0, rate, durata, sorgente, seme, uscita)."""
    rate, per, tot = leggi_produzione(GAMMA)
    durata = tot / rate
    fuori = os.path.join(a.uscita, "pezzi")
    os.makedirs(fuori, exist_ok=True)
    chi = a.pezzi or ["%d" % k for k in range(len(glob.glob(os.path.join(GAMMA, "pezzo_*.root"))))] + ["mu"]
    L = []
    for k in chi:
        if k == "mu":
            rmu, pmu, _ = leggi_produzione(MUONI)
            for i, fm in enumerate(sorted(glob.glob(os.path.join(MUONI, "pezzo_*.root")))):
                if i * pmu / rmu < durata:
                    L.append(("muoni_%04d" % i, fm, i * pmu / rmu, rmu, durata, 2, [2, i], os.path.join(fuori, "muoni_%04d.npz" % i)))
        else:
            k = int(k)
            L.append(("gamma_%04d" % k, os.path.join(GAMMA, "pezzo_%04d.root" % k), k * per / rate, rate, durata, 1, [1, k],
                      os.path.join(fuori, "gamma_%04d.npz" % k)))
    return L


def cmd_pezzi(a):
    esegui([x for x in lavori(a) if a.rifai or not os.path.exists(x[-1])], a)


def esegui(L, a):
    print("%d pezzi da fare, %d processi" % (len(L), a.processi), flush=True)
    if a.processi > 1:
        with mp.get_context("spawn").Pool(a.processi, initializer=_imposta, initargs=(a.uscita,)) as pool:
            for msg in pool.imap_unordered(_lavora, L):
                print(time.strftime("%H:%M:%S"), msg, flush=True)
    else:
        for x in L:
            print(time.strftime("%H:%M:%S"), _lavora(x), flush=True)


def _imposta(uscita):
    global PARAMETRI_DIR
    PARAMETRI_DIR = uscita


def cmd_ritardi(a):
    """Ritardo dei SiPM: distanza dei fronti negli eventi col trigger dei PMT
    e un segnale nel SiPM, dati contro MC; il ritardo si corregge di quanto
    il MC sta indietro."""
    P = json.load(open(os.path.join(a.uscita, "parametri.json")))
    T = np.load(os.path.join(a.uscita, "tempi_trigger.npz"))
    d50 = {k: [] for k in SIPM}
    for fn in sorted(glob.glob(os.path.join(a.uscita, "pezzi", "*.npz"))):
        z = np.load(fn)
        if len(z["t_s"]) == 0:
            continue
        r = caratteristiche(z["wf"], P["soglia_conteggi"])
        sopra = r["amp"] >= np.array(P["soglia_conteggi"])
        solo_pmt = sopra[:, 3:].any(axis=1) & ~sopra[:, :3].any(axis=1)
        for k in SIPM:
            m = solo_pmt & (r["amp"][:, k] > P["quieto_conteggi"][k])
            jp = np.where(r["amp"][m, 3] >= r["amp"][m, 4], 3, 4)
            d50[k].append(r["t50"][m, k] - r["t50"][m][np.arange(m.sum()), jp])
    for k in SIPM:
        mc = np.concatenate(d50[k]) if d50[k] else np.zeros(0)
        dati = T["d50_dati_ch%d" % CANALI[k]]
        corr = float(np.nanmedian(dati) - np.nanmedian(mc))
        print("ch%d: dati %d eventi, mediana %.1f ns; MC %d eventi, mediana %.1f ns -> ritardo %.1f + %.1f = %.1f ns" % (
            CANALI[k], len(dati), np.nanmedian(dati), len(mc), np.nanmedian(mc), P["ritardo_ns"][k], corr, P["ritardo_ns"][k] + corr))
        P["ritardo_ns"][k] += corr
    if a.scrivi:
        json.dump(P, open(os.path.join(a.uscita, "parametri.json"), "w"), indent=1)
        print("scritti in parametri.json: rifare i pezzi (pezzi --rifai)")


def scrivi_run(percorso, numero, w, ttt, sic, inizio, verita, info):
    S = h5py.string_dtype("ascii")
    with h5py.File(percorso, "w", libver="latest") as f:
        g = f.create_group("config")
        for k, v, d in (("Board", "MC", S), ("BoardSerial", 0, np.uint32), ("ChannelList", CANALI, np.uint32),
                        ("FormatVersion", 3, np.int32), ("Link", "MC: produzione_hallA/digit_daq.py", S),
                        ("OutputFormat", "HDF5", S), ("PostTriggerSize", 55, np.uint32), ("RecordLength", 1024, np.uint32),
                        ("RunNumber", numero, np.int32), ("SamplesPerChannel", NS, np.uint32), ("SamplingRate", "750MHz", S),
                        ("SamplingTime", DT * 1e-9, np.float64), ("SelfTriggerChannels", CANALI, np.uint32),
                        ("SelfTriggerMode", "paired", S), ("SelfTriggerThreshold", SOGLIA_ASSOLUTA, np.uint32),
                        ("StartTime", inizio, np.float64), ("TailCut", 12, np.uint32), ("TransparentMode", 0, np.int32),
                        ("TriggerMode", "Self", S), ("VMEBaseAddress", 0, np.uint32)):
            if d is S:
                g.attrs.create(k, v, dtype=S)
            else:
                g.attrs.create(k, np.asarray(v, d))
        ev = f.create_group("events")
        n = len(w)
        ev.create_dataset("event_counter", data=np.arange(n, dtype=np.uint32), chunks=(256,), maxshape=(None,))
        ev.create_dataset("trigger_time_tag", data=ttt.astype(np.uint32), chunks=(256,), maxshape=(None,))
        ev.create_dataset("start_index_cell", data=sic.astype(np.uint16), chunks=(256, 4), maxshape=(None, 4))
        ev.create_dataset("waveforms", data=w.reshape(n, 5 * NS), chunks=(8, 5 * NS), maxshape=(None, 5 * NS))
        mc = f.create_group("mc")          # la verita' del MC: gli strumenti dei dati non la guardano
        for k, v in verita.items():
            mc.create_dataset(k, data=v, compression="gzip")
        for k, v in info.items():
            mc.attrs[k] = v


TESTA_24H = """Summary : * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary : *  MC, NON E' UNA RUN VERA: produzione Hall A digitalizzata  *
Summary : * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary : MC: {programma}
Summary : MC: gamma {gamma} ({gen_gamma} generati in {durata_tot:.0f} s, {rate_gamma:.0f} Hz), muoni {muoni} ({rate_mu} Hz)
Summary : MC: questa run va da {t0:.3f} a {t1:.3f} s del flusso di {durata_tot:.0f} s; parametri in {parametri}
Summary : MC: scala della luce dei pannelli {scala_luce}, ritardi dei SiPM {ritardi} ns
Summary :
"""
TESTA_CAMPIONE = """Summary : * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary : *  MC, NON E' UNA RUN VERA: campione digitalizzato a parte  *
Summary : * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary : MC: {programma}
Summary : MC: campione {campione}: {generati} eventi generati a {rate} Hz, {durata_tot:.0f} s
Summary : MC: questa run va da {t0:.3f} a {t1:.3f} s del campione; parametri in {parametri}
Summary : MC: luce dei pannelli: {luce} (scala {scala_luce}), ritardi dei SiPM {ritardi} ns
Summary :
"""
LOG_DAQ = """Summary : → Active channels: 5
Summary :    ch16
Summary :    ch17
Summary :    ch18
Summary :    ch20
Summary :    ch21
Summary : OutputFormat read from config = 'HDF5'
Summary : → Self-trigger ENABLED (mode = paired, threshold = relative)
Summary :    trigger channels: ch16 ch17 ch18 ch20 ch21
Summary :    threshold driven by SelfTriggerThresholdOffset [counts]: ch16=450 ch17=600 ch18=600 ch20=80 ch21=80
Summary : → Sampling frequency set to 750MHz (1.333333 ns per sample)
Debug   :    ch16: DCOffset = 0xB000
Debug   :    ch17: DCOffset = 0xB000
Debug   :    ch18: DCOffset = 0xB000
Debug   :    ch20: DCOffset = 0xB000
Debug   :    ch21: DCOffset = 0xB000
Summary : → DRS4 corrections ENABLED (recorded data only; the self-trigger decision is unaffected).
Summary : → Self-trigger threshold(s) applied:
Summary :    ch16 → 1458 counts
Summary :    ch17 → 1590 counts
Summary :    ch18 → 1628 counts
Summary :    ch20 → 1102 counts
Summary :    ch21 → 1088 counts
Summary : → HDF5 output path selected: {nome}.h5

===== configurazione usata (MC: quella delle run LNGS__54-0V_0130-0157) =====
[digitizer]
ChannelList = [ 16, 17, 18, 20, 21 ]
DCOffset = 0xB000
DRS4Correction = true
NEvents = {per_run}
OutputFormat = 'HDF5'
PostTriggerSize = 55
PulsePolarity = 0
RecordLength = 1024
RunNumber = {numero}
SamplingRate = '750MHz'
SelfTrigger = true
SelfTriggerChannels = [ 16, 17, 18, 20, 21 ]
SelfTriggerMode = 'paired'
SelfTriggerThresholdMode = 'relative'
SelfTriggerThresholdOffset = [ 450, 600, 600, 80, 80 ]
TailCut = 12
TriggerPolarity = 0
===== fine configurazione =====

Summary : → Acquisition started (waiting for self triggers)
Summary : Acquisition complete.
Summary : → Total events recorded: {n}
Summary : → Acquisition time: {durata:.6f} s
Summary : → Trigger rate: {rate_khz:.6f} kHz
Summary : → Events written to file: {n}
"""
LOG = TESTA_24H + LOG_DAQ


def cmd_run(a):
    P = json.load(open(os.path.join(a.uscita, "parametri.json")))
    rate, per, tot = leggi_produzione(GAMMA)
    rmu = leggi_produzione(MUONI)[0]
    durata_tot = tot / rate
    files = sorted(glob.glob(os.path.join(a.uscita, "pezzi", "*.npz")))
    gamma = [f for f in files if os.path.basename(f).startswith("gamma_")]
    attesi = len(glob.glob(os.path.join(GAMMA, "pezzo_*.root")))
    if len(gamma) != attesi:
        sys.exit("pezzi gamma digitalizzati %d su %d: finire prima (comando pezzi)" % (len(gamma), attesi))
    # tempi e posizione di ogni evento, poi le forme d'onda run per run
    indice = []
    for i, fn in enumerate(files):
        with np.load(fn) as z:
            indice.append(np.stack([z["t_s"], np.full(len(z["t_s"]), i), np.arange(len(z["t_s"]))], axis=1))
    indice = np.concatenate(indice)
    indice = indice[np.argsort(indice[:, 0], kind="stable")]
    n_tot = len(indice)
    # Distribuzioni_carica tiene in cache le cariche col NOME della run: un MC
    # rifatto con altri parametri deve chiamarsi diversamente
    prefisso = a.prefisso or ("MC_HallA__54-0V_" if P["scala_luce"] == 1 else
                              "MC_HallA_luce%s__54-0V_" % ("%g" % P["scala_luce"]).replace(".", "p"))
    cartella = os.path.join(a.uscita, "run")
    os.makedirs(cartella, exist_ok=True)
    rng = np.random.default_rng(31)
    inizio_unix = float(a.inizio_unix or time.time())
    cache = {}
    t_prima = 0.0
    for numero, a0 in enumerate(range(0, n_tot, a.eventi_per_run), start=1):
        blocco = indice[a0:a0 + a.eventi_per_run]
        ultimo = a0 + a.eventi_per_run >= n_tot
        t_fine = durata_tot if ultimo else float(blocco[-1, 0])
        W = np.empty((len(blocco), 5, NS), np.int16)
        V = {}
        for i_file in np.unique(blocco[:, 1]).astype(int):
            if i_file not in cache:
                cache.clear()
                cache[i_file] = dict(np.load(files[i_file]))
            z = cache[i_file]
            m = blocco[:, 1] == i_file
            righe = blocco[m, 2].astype(int)
            W[m] = z["wf"][righe]
            for k, v in z.items():
                if k in ("wf", "n_candidati", "n_righe") or v.ndim == 0:
                    continue
                if k not in V:
                    V[k] = np.zeros((len(blocco),) + v.shape[1:], v.dtype)
                V[k][m] = v[righe]
        V["pezzo"] = np.array([int(os.path.basename(files[int(i)])[-8:-4]) for i in blocco[:, 1]], np.int16)
        ttt = np.round(blocco[:, 0] * 1e9 / TTT_NS).astype(np.int64) % TTT_GIRO
        sic2 = rng.integers(0, 1024, len(blocco))
        sic = np.stack([np.full(len(blocco), 0xFFFF), np.full(len(blocco), 0xFFFF), sic2,
                        (sic2 + rng.integers(-2, 4, len(blocco))) % 1024], axis=1)
        nome = "%s%04d_750Ms_55PT" % (prefisso, numero)
        h5 = os.path.join(cartella, nome + ".h5")
        info = {"programma": os.path.abspath(__file__), "gamma": GAMMA, "muoni": MUONI, "rate_gamma_hz": rate,
                "rate_muoni_hz": rmu, "t_inizio_s": t_prima, "t_fine_s": t_fine, "parametri": json.dumps(P)}
        scrivi_run(h5, numero, W, ttt, sic, inizio_unix + t_prima, V, info)
        with open(h5, "rb") as fi, gzip.open(h5 + ".gz.tmp", "wb", compresslevel=6) as fo:
            shutil.copyfileobj(fi, fo, 16 << 20)
        os.replace(h5 + ".gz.tmp", h5 + ".gz")
        os.unlink(h5)
        durata = t_fine - t_prima
        with open(os.path.join(cartella, nome + ".log"), "w") as f:
            f.write(LOG.format(programma=os.path.abspath(__file__), gamma=GAMMA, gen_gamma=tot, durata_tot=durata_tot,
                               rate_gamma=rate, muoni=MUONI, rate_mu=rmu, t0=t_prima, t1=t_fine,
                               parametri=os.path.join(os.path.abspath(a.uscita), "parametri.json"), scala_luce=P["scala_luce"],
                               ritardi=[round(x, 1) for x in P["ritardo_ns"][:3]], nome=nome, per_run=a.eventi_per_run,
                               numero=numero, n=len(blocco), durata=durata, rate_khz=len(blocco) / durata / 1e3))
        print("%s: %d eventi, %.1f s, %.2f Hz" % (nome, len(blocco), durata, len(blocco) / durata), flush=True)
        t_prima = t_fine


def cmd_campione(a):
    """Un campione a parte (per esempio ottica_accesa.sh, con l'ottica dei
    pannelli accesa) coi parametri di un'uscita di prepara, in run sue: un
    pezzo per file .root, gli eventi uno dopo l'altro al rate dato."""
    import uproot
    P = json.load(open(os.path.join(a.parametri, "parametri.json")))
    P["luce"], P["campione"] = a.luce, os.path.abspath(a.cartella)
    qui = os.path.join(a.uscita, "parametri.json")
    if os.path.exists(qui) and json.load(open(qui)) != P:
        a.rifai = True                           # parametri cambiati: i pezzi gia' fatti non valgono
    json.dump(P, open(qui, "w"), indent=1)
    shutil.copy(os.path.join(a.parametri, "tempi_trigger.npz"), a.uscita)
    files = sorted(glob.glob(os.path.join(a.cartella, "*.root")))
    gen = [int(uproot.open(f)["Run"].arrays(library="np")["N_generated"].sum()) for f in files]
    t0 = np.r_[0, np.cumsum(gen)] / a.rate
    durata = float(t0[-1])
    st = uproot.open(files[0])["Eventi"].arrays(["SourceType"], library="np")["SourceType"].astype(int)
    sorgente = int(np.bincount(st).argmax()) if len(st) else -1
    print("%s: %d file, %d eventi generati, %.0f s a %g Hz; luce dei pannelli: %s" % (
        a.cartella, len(files), sum(gen), durata, a.rate, a.luce), flush=True)
    fuori = os.path.join(a.uscita, "pezzi")
    os.makedirs(fuori, exist_ok=True)
    L = [("parte_%04d" % i, f, t0[i], a.rate, durata, sorgente, [3, i], os.path.join(fuori, "parte_%04d.npz" % i))
         for i, f in enumerate(files)]
    esegui([x for x in L if a.rifai or not os.path.exists(x[-1])], a)
    # le run: il campione e' piccolo, sta tutto in memoria
    Z = [dict(np.load(x[-1])) for x in L]
    t = np.concatenate([z["t_s"] for z in Z])
    if len(t) == 0:
        sys.exit("nessun evento fa scattare il trigger")
    o = np.argsort(t, kind="stable")
    t, W = t[o], np.concatenate([z["wf"] for z in Z])[o]
    V = {k: np.concatenate([z[k] for z in Z])[o] for k in Z[0] if k != "wf" and Z[0][k].ndim > 0}
    V["pezzo"] = np.concatenate([np.full(len(z["t_s"]), i, np.int16) for i, z in enumerate(Z)])[o]
    prefisso = a.prefisso or "MC_%s_%s__54-0V_" % (os.path.basename(os.path.normpath(a.cartella)), a.luce)
    cartella = os.path.join(a.uscita, "run")
    os.makedirs(cartella, exist_ok=True)
    rng = np.random.default_rng(31)
    inizio_unix = float(a.inizio_unix or time.time())
    t_prima = 0.0
    for numero, a0 in enumerate(range(0, len(t), a.eventi_per_run), start=1):
        s = slice(a0, a0 + a.eventi_per_run)
        n = len(t[s])
        t_fine = durata if a0 + a.eventi_per_run >= len(t) else float(t[s][-1])
        ttt = np.round(t[s] * 1e9 / TTT_NS).astype(np.int64) % TTT_GIRO
        sic2 = rng.integers(0, 1024, n)
        sic = np.stack([np.full(n, 0xFFFF), np.full(n, 0xFFFF), sic2, (sic2 + rng.integers(-2, 4, n)) % 1024], axis=1)
        nome = "%s%04d_750Ms_55PT" % (prefisso, numero)
        h5 = os.path.join(cartella, nome + ".h5")
        info = {"programma": os.path.abspath(__file__), "campione": P["campione"], "luce": a.luce, "rate_hz": a.rate,
                "t_inizio_s": t_prima, "t_fine_s": t_fine, "parametri": json.dumps(P)}
        scrivi_run(h5, numero, W[s], ttt, sic, inizio_unix + t_prima, {k: v[s] for k, v in V.items()}, info)
        with open(h5, "rb") as fi, gzip.open(h5 + ".gz.tmp", "wb", compresslevel=6) as fo:
            shutil.copyfileobj(fi, fo, 16 << 20)
        os.replace(h5 + ".gz.tmp", h5 + ".gz")
        os.unlink(h5)
        d = t_fine - t_prima
        with open(os.path.join(cartella, nome + ".log"), "w") as f:
            f.write((TESTA_CAMPIONE + LOG_DAQ).format(
                programma=os.path.abspath(__file__), campione=P["campione"], generati=sum(gen), rate=a.rate,
                durata_tot=durata, t0=t_prima, t1=t_fine, parametri=os.path.abspath(qui), luce=a.luce,
                scala_luce=P["scala_luce"], ritardi=[round(x, 1) for x in P["ritardo_ns"][:3]], nome=nome,
                per_run=a.eventi_per_run, numero=numero, n=n, durata=d, rate_khz=n / d / 1e3))
        print("%s: %d eventi, %.0f s, %.3g Hz" % (nome, n, d, n / d), flush=True)
        t_prima = t_fine


def main():
    global PARAMETRI_DIR
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("prepara")
    p.add_argument("uscita")
    p.add_argument("dati", nargs="+")
    p.add_argument("--scala-luce", type=float, default=1.0)
    p.add_argument("--sovratensione", type=float, default=SM.SOVRA_VOP)
    p.add_argument("--libreria-pmt", default=LIB_PMT,
                   help="forma del singolo PE, K e rumore dei PMT (di serie %s)" % os.path.basename(LIB_PMT))
    p.add_argument("--efficienza-pmt", default=None,
                   help="JSON con A50 e s del trigger dei PMT (efficienza_trigger_pmt_20261030.json); di serie il gradino")
    p = sub.add_parser("pezzi")
    p.add_argument("uscita")
    p.add_argument("pezzi", nargs="*")
    p.add_argument("--processi", type=int, default=1)
    p.add_argument("--rifai", action="store_true", help="rifa' anche i pezzi gia' fatti")
    p = sub.add_parser("ritardi")
    p.add_argument("uscita")
    p.add_argument("--scrivi", action="store_true", help="aggiorna parametri.json")
    p = sub.add_parser("run")
    p.add_argument("uscita")
    p.add_argument("--eventi-per-run", type=int, default=100000)
    p.add_argument("--prefisso", default=None, help="di serie MC_HallA__54-0V_ (MC_HallA_luceS__54-0V_ con --scala-luce S)")
    p.add_argument("--inizio-unix", type=float, default=None)
    p = sub.add_parser("campione")
    p.add_argument("uscita")
    p.add_argument("cartella", help="i .root del campione, per esempio ../data/produzione_hallA/ottica_muoni")
    p.add_argument("--parametri", required=True, help="un'uscita di prepara: parametri.json e tempi_trigger.npz")
    p.add_argument("--rate", type=float, required=True, help="eventi generati al secondo (muoni di Hall A: 0.001789)")
    p.add_argument("--luce", choices=("mappa", "fotoni"), default="mappa",
                   help="PE dei SiPM dalla mappa o dai fotoni rivelati (campione con l'ottica dei pannelli accesa)")
    p.add_argument("--processi", type=int, default=1)
    p.add_argument("--rifai", action="store_true", help="rifa' anche i pezzi gia' fatti")
    p.add_argument("--eventi-per-run", type=int, default=100000)
    p.add_argument("--prefisso", default=None, help="di serie MC_<cartella>_<luce>__54-0V_")
    p.add_argument("--inizio-unix", type=float, default=None)
    a = ap.parse_args()
    os.makedirs(a.uscita, exist_ok=True)
    PARAMETRI_DIR = a.uscita
    {"prepara": cmd_prepara, "pezzi": cmd_pezzi, "ritardi": cmd_ritardi, "run": cmd_run,
     "campione": cmd_campione}[a.cmd](a)


if __name__ == "__main__":
    main()
