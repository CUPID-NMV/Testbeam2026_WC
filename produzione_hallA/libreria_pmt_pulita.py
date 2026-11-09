"""libreria_pmt_pulita.py -- la libreria dei PMT di digit_daq.py senza la
diafonia del ch16 nel ch21 (2026-11-09).

daq-standalone/analisi/20261104_forma_1pe_pmt/libreria_pmt.py prende gli
eventi "vuoti" dei PMT (traccia media da togliere agli impulsi, tracce di
rumore) dallo scan dei SiPM. Nei 40000 eventi che legge c'e' solo lo scan del
ch16, e li' il ch21 porta la diafonia del ch16: +1.6 conteggi a 240-500 ns.
Tolta dagli impulsi del ch21, la diafonia diventa un'undershoot finta. Nel MC
gli impulsi del ch21 escono stretti, il suo rumore ha ~+0.07 PE nel cancello e
il PE e' sottostimato del 4%. Verifica:
data/produzione_hallA/daq_24h/confronto_20261109/baco_forma_ch21.ipynb.

Qui si usa la stessa funzione, libreria_pmt.costruisci, letta da
daq-standalone e non copiata:
  ch20  esattamente come prima (vuoti dallo scan del ch16, che nel ch20 non
        lascia niente);
  ch21  vuoti dallo scan del ch20: i SiPM sono parcheggiati e scatta l'altro
        PMT. costruisci prende i vuoti da "inizio dello scan dei SiPM" alla
        fine dell'array, quindi in coda ai primi eventi si mettono quelli
        dello scan del ch20, al posto dello scan del ch16.
Poi la griglia comune e il K medio come alla fine di costruisci.

    python3 -I libreria_pmt_pulita.py [SCAN.npz | SCAN.h5.gz] [--uscita F.npz]

SCAN.npz ha `d` (eventi x [ch20, ch21] x campioni, i primi 40000 della run
dello scan) e `dt_ns`, come la cache di baco_forma_ch21.ipynb. Senza argomenti
legge la run dello scan dalla copia locale.
"""
import argparse
import glob
import importlib.util
import json
import os
import sys

import numpy as np

QUI = os.path.dirname(os.path.abspath(__file__))
DAQ = os.path.expanduser("~/daq-standalone")
COSTRUTTORE = os.path.join(DAQ, "analisi", "20261104_forma_1pe_pmt", "libreria_pmt.py")
MIS = os.path.join(DAQ, "measurements")
VECCHIA = os.path.join(MIS, "libreria_digit_pmt_20261030.npz")
SCAN_RUN = os.path.expanduser("~/nmv-copia/WC/LNGS/LNGS_scan_54-0V_0000_750Ms_55PT.h5.gz")
USCITA = os.path.join(QUI, "libreria_digit_pmt_20261109.npz")
CHIAVI_CANALE = ("forma", "t50", "sogliaA", "K", "n_impulsi", "n_rumore", "sigma_rumore", "sigma_q",
                 "frazione_nel_cancello", "rumore")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("scan", nargs="?", default=SCAN_RUN)
    ap.add_argument("--uscita", default=USCITA)
    a = ap.parse_args()

    spec = importlib.util.spec_from_file_location("libreria_pmt", COSTRUTTORE)
    lp = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lp)
    S = {c: json.load(open(glob.glob(os.path.join(MIS, "noise_scan_20261030_*_ch%d.json" % c))[0])) for c in (16, 20, 21)}
    if a.scan.endswith(".npz"):
        z = np.load(a.scan)
        d, dt = z["d"], float(z["dt_ns"])
    else:
        sys.path.insert(0, os.path.join(DAQ, "tools"))
        import padme
        h, _, d, _ = padme.leggi(a.scan, lp.CANALI, lp.N_EVENTI, serve_sic=False)
        dt = float(h["SamplingTime"]) * 1e9
    if len(d) != lp.N_EVENTI:
        sys.exit("servono i primi %d eventi dello scan, ce ne sono %d" % (lp.N_EVENTI, len(d)))

    # 1) come libreria_pmt.py: deve ridare la libreria vecchia
    scans = {20: S[20], 21: S[21], "sipm": [S[16]]}
    std, ris_std = lp.costruisci(d, dt, scans)
    V = np.load(VECCHIA)
    diff = max(np.abs(std["forma_ch%d" % c] - V["forma_ch%d" % c]).max() for c in lp.CANALI)
    if diff > 0:
        sys.exit("costruisci non ridà la libreria vecchia (differenza %.2e): fermo" % diff)
    print("costruisci ridà la libreria vecchia: ok")

    # 2) i vuoti dallo scan del ch20 al posto di quelli dello scan del ch16
    inizio = min(p["eventi_da"] for p in S[16]["punti"])
    ev20 = np.concatenate([np.arange(p["eventi_da"], p["eventi_a"]) for p in S[20]["punti"]])
    if ev20.max() >= inizio:
        sys.exit("lo scan del ch20 si sovrappone a quello dei SiPM: fermo")
    # in coda: per il ch21 gli eventi dello scan del ch20; per il ch20 (che qui
    # non si usa, ma costruisci lo calcola) altrettanti eventi dello scan del ch16
    coda = np.stack([d[inizio:inizio + len(ev20), 0], d[ev20, 1]], axis=1)
    d2 = np.concatenate([d[:inizio], coda])
    pul, ris_pul = lp.costruisci(d2, dt, scans)

    # 3) ch20 dalla prima, ch21 dalla seconda; griglia comune e K medio come in costruisci
    ris = {20: ris_std[20], 21: ris_pul[21]}
    out = dict(std)
    for k in CHIAVI_CANALE:
        out["%s_ch21" % k] = pul["%s_ch21" % k]
    x0 = max(ris[c]["xf"][0] for c in lp.CANALI)
    x1 = min(ris[c]["xf"][-1] for c in lp.CANALI)
    xf = np.arange(x0, x1, lp.FINE)
    K = float(np.mean([ris[c]["K"] for c in lp.CANALI]))
    for c in lp.CANALI:
        out["forma_ch%d" % c] = np.interp(xf, ris[c]["xf"], ris[c]["forma"]) * ris[c]["K"] / K
    out["forma_x_ns"] = xf
    out["K_adcns_per_pe"] = K
    out["run"] = os.path.basename(a.scan)
    out["nota"] = ("ch20 come libreria_digit_pmt_20261030; ch21 coi vuoti dallo scan del ch20 invece che del ch16 "
                   "(diafonia ch16 -> ch21), da produzione_hallA/libreria_pmt_pulita.py")

    for c in lp.CANALI:
        # carica per PE degli impulsi in uscita da digit_daq (forma * K), nella finestra
        # -10..+30 ns dal picco dove si misura la gobba del PE
        f_new, f_old = out["forma_ch%d" % c] * K, V["forma_ch%d" % c] * float(V["K_adcns_per_pe"])
        q = []
        for f, x in ((f_old, V["forma_x_ns"]), (f_new, xf)):
            xp = x[np.argmax(f)]
            q.append(f[(x >= xp - 10) & (x <= xp + 30)].sum() * lp.FINE)
        g = np.flatnonzero(xf >= xf[np.argmax(f_new)])
        print("ch%d: K %.0f -> %.0f ADC*ns per PE; PE in -10..+30 ns dal picco %.0f -> %.0f ADC*ns; "
              "a +40 ns dal picco %.3f -> %.3f del picco; rumore: %d tracce" % (
                  c, float(V["K_ch%d" % c]), float(out["K_ch%d" % c]), q[0], q[1],
                  np.interp(V["forma_x_ns"][np.argmax(f_old)] + 40, V["forma_x_ns"], f_old) / f_old.max(),
                  np.interp(xf[g[0]] + 40, xf, f_new) / f_new.max(), len(out["rumore_ch%d" % c])))
    print("K medio %.0f -> %.0f" % (float(V["K_adcns_per_pe"]), K))
    np.savez_compressed(a.uscita, **out)
    print("scritto", a.uscita)


if __name__ == "__main__":
    main()
