#!/usr/bin/env python
"""Vista RAW dei primi 8 canali di un file DAQ .h5 / .h5.gz.

Nessuna selezione, nessun filtro, nessuna sottrazione di baseline: mostra i
segnali cosi' come sono, per vedere che faccia hanno. Una figura per evento,
griglia 2x4. Solo conversione ADC -> mV (fattore di scala, non altera la forma).
"""

import os
import gzip
import shutil
import tempfile

import numpy as np
import h5py
import matplotlib.pyplot as plt

# --- CONFIG ---------------------------------------------------------------
FILE = os.path.expanduser(
    "~/Downloads/MV_PMT__selfTrigg_Top1400V_offset40_0018_2.5Gs_70PT.h5"
)
CHANNELS = list(range(8))   # primi 8 canali -> griglia 2 x 4
N_EVENTS = 6                # quanti eventi mostrare (una figura ciascuno)
NOMINAL_CHANNELS = 16    # modalita' a 16 canali (16192/16=1012 camp -> 405 ns)
ADC_TO_MV = 1000.0 / 4096.0

# --- LOAD -----------------------------------------------------------------
def open_h5(path):
    if path.endswith(".h5"):
        return path, None
    if path.endswith(".h5.gz"):
        tmp = tempfile.NamedTemporaryFile(suffix=".h5", delete=False)
        tmp.close()
        with gzip.open(path, "rb") as fi, open(tmp.name, "wb") as fo:
            shutil.copyfileobj(fi, fo)
        return tmp.name, tmp.name
    raise ValueError(f"Formato non supportato: {path}")


path, tmp = open_h5(FILE)
try:
    with h5py.File(path, "r") as f:
        cfg = dict(f["config"].attrs)
        dt_ns = float(cfg["SamplingTime"]) * 1e9
        ev = f["events"]
        keys = sorted(ev.keys(), key=lambda x: int(x.replace("event", "")))

        first = np.array(ev[keys[0]][:])
        spc = len(first) // NOMINAL_CHANNELS          # campioni per canale
        t = np.arange(spc) * dt_ns

        print(f"File   : {os.path.basename(FILE)}")
        print(f"Trigger: {cfg.get('TriggerMode')} | eventi: {len(keys)} | "
              f"campioni/canale: {spc} ({t[-1]:.0f} ns)")

        ncol = 4
        nrow = int(np.ceil(len(CHANNELS) / ncol))

        for ev_i in range(min(N_EVENTS, len(keys))):
            data = np.array(ev[keys[ev_i]][:]).reshape(NOMINAL_CHANNELS, spc)

            fig, axes = plt.subplots(nrow, ncol, figsize=(4.0 * ncol, 2.6 * nrow),
                                     sharex=True, squeeze=False)
            for k, ch in enumerate(CHANNELS):
                ax = axes[k // ncol][k % ncol]
                ax.plot(t, data[ch] * ADC_TO_MV, lw=0.8, color="crimson")
                ax.set_title(f"ch {ch}", fontsize=10)
                ax.grid(True, alpha=0.25)
            for c in range(ncol):
                axes[-1][c].set_xlabel("Time [ns]")
            for r in range(nrow):
                axes[r][0].set_ylabel("Raw [mV]")

            fig.suptitle(f"{os.path.basename(FILE)} — event {ev_i} (RAW, nessuna selezione)",
                         fontsize=11)
            plt.tight_layout()

        plt.show()
finally:
    if tmp and os.path.exists(tmp):
        os.remove(tmp)
