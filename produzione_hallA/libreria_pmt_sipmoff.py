"""libreria_pmt_sipmoff.py -- la libreria dei PMT di digit_daq.py dalla run coi
SiPM spenti del 2026-11-10 (LNGS__54-0V_PMTOnly_offset-10-10_HVoff_0000):
self-trigger solo sui due PMT, offset 10 per tutti e due, SiPM senza tensione,
stessa configurazione delle run (750 MS/s, DCOffset 0xB000, inverter).

Perche': la forma del singolo PE del ch21 della libreria del 9/11 (dallo scan
del 30/10, coi vuoti dello scan del ch20) ha ancora una coda di troppo, ~0.005
del picco a 50-150 ns, e nel MC gli impulsi del ch21 a 3-6 PE escono larghi
16.5 ns contro 15.1 nei dati (confronto_20261110/pmt_sipm_quieti.ipynb). Qui
non c'e' diafonia dei SiPM per costruzione, e le condizioni sono quelle di
adesso.

Si usa la funzione di daq-standalone, libreria_pmt.costruisci, letta e non
copiata. costruisci vuole uno scan a passi: gli eventi si riordinano cosi'
  - "scan del ch20": gli eventi in cui ha sparato solo il ch20 (impulso sopra
    SPARO_CONTEGGI attorno all'istante del trigger, il ch21 sotto);
  - "scan del ch21": idem per il ch21;
  - "scan dei SiPM" (gli eventi vuoti): tutti gli eventi, in coda. costruisci
    tiene come vuoti di un PMT quelli dove non ha niente, cioe' quelli dove ha
    sparato l'altro.
Il passo a offset 100, che costruisci usa solo per sogliaA (il bordo del
trigger della 0013, che digit_daq non usa), e' finto: lo stesso gruppo di
eventi; sogliaA non ha significato.

    python3 -I libreria_pmt_sipmoff.py RUN.h5.gz [--uscita F.npz] [--eventi N]
"""
import argparse
import importlib.util
import os
import sys

import numpy as np

QUI = os.path.dirname(os.path.abspath(__file__))
DAQ = os.path.expanduser("~/daq-standalone")
COSTRUTTORE = os.path.join(DAQ, "analisi", "20261104_forma_1pe_pmt", "libreria_pmt.py")
USCITA = os.path.join(QUI, "libreria_digit_pmt_20261110_sipmoff.npz")
SPARO_CONTEGGI = 30.0      # ~7 mV: la soglia a offset 10 (0.72 mV/offset)
FINESTRA_TRIG_NS = (200.0, 330.0)   # dove cade l'impulso che ha fatto scattare


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("run")
    ap.add_argument("--uscita", default=USCITA)
    ap.add_argument("--eventi", type=int, default=40000)
    a = ap.parse_args()

    spec = importlib.util.spec_from_file_location("libreria_pmt", COSTRUTTORE)
    lp = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lp)
    sys.path.insert(0, os.path.join(DAQ, "tools"))
    import padme
    h, _, d, _ = padme.leggi(a.run, lp.CANALI, a.eventi, serve_sic=False)
    dt = float(h["SamplingTime"]) * 1e9
    T = np.arange(d.shape[2]) * dt
    y = d.astype(np.float32)
    y -= y[:, :, T < lp.PED_NS].mean(2, keepdims=True)
    w = (T >= FINESTRA_TRIG_NS[0]) & (T < FINESTRA_TRIG_NS[1])
    A = y[:, :, w].max(2)
    spara = A >= SPARO_CONTEGGI
    ev20 = np.flatnonzero(spara[:, 0] & ~spara[:, 1])
    ev21 = np.flatnonzero(spara[:, 1] & ~spara[:, 0])
    print("%s: %d eventi letti; ha sparato solo il ch20 %d, solo il ch21 %d, tutti e due %d, nessuno %d" % (
        os.path.basename(a.run), len(d), len(ev20), len(ev21), (spara[:, 0] & spara[:, 1]).sum(), (~spara.any(1)).sum()))
    d2 = np.concatenate([d[ev20], d[ev21], d])
    n20, n21 = len(ev20), len(ev21)
    scans = {20: {"punti": [{"offset": 10.0, "eventi_da": 0, "eventi_a": n20},
                            {"offset": 100.0, "eventi_da": 0, "eventi_a": n20}]},
             21: {"punti": [{"offset": 10.0, "eventi_da": n20, "eventi_a": n20 + n21},
                            {"offset": 100.0, "eventi_da": n20, "eventi_a": n20 + n21}]},
             "sipm": [{"punti": [{"offset": 0.0, "eventi_da": n20 + n21, "eventi_a": len(d2)}]}]}
    out, ris = lp.costruisci(d2, dt, scans)
    out["run"] = os.path.basename(a.run)
    out["nota"] = ("dalla run coi SiPM spenti del 2026-11-10 (solo PMT, offset 10), libreria_pmt.costruisci con gli eventi "
                   "riordinati (libreria_pmt_sipmoff.py); sogliaA senza significato")
    for c in lp.CANALI:
        r = ris[c]
        print("ch%d: %d impulsi da 1 PE, %d tracce di rumore (sigma %.2f conteggi); PE in -10..+30 ns %.0f ADC*ns, "
              "K %.0f; t50 %.1f ns" % (c, r["n"], out["n_rumore_ch%d" % c], r["s_raw"], r["mu_q"], r["K"], r["t50"]))
    print("K medio %.0f" % out["K_adcns_per_pe"])
    np.savez_compressed(a.uscita, **out)
    print("scritto", a.uscita)


if __name__ == "__main__":
    main()
