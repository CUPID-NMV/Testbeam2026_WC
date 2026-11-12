#!/usr/bin/env python3
"""unisci_pezzi.py — unisce i pezzi di una produzione (pezzo_NNNN.root) in un
solo file ROOT senza perdere informazioni.

Perche' non basta hadd: in ogni pezzo l'EventID riparte da 0, quindi dopo hadd
non si sa piu' a quale evento di "Eventi" appartiene un fotone di "Fotoni".
Qui ogni riga tiene tutte le colonne originali, EventID compreso, e ne
guadagna due:
  Pezzo            indice del pezzo (dal nome pezzo_NNNN.root)
  EventID_globale  EventID + primari generati nei pezzi precedenti: unico su
                   tutta la produzione, uguale in "Eventi" e "Fotoni"
Nella ntupla "Run": Pezzo e i due semi del pezzo (Seme1, Seme2, dal suo .mac).
Nel file anche il testo di produzione.txt (TObjString "produzione").

Uso:  python3 unisci_pezzi.py <cartella_produzione> [file_uscita.root]
      (uscita di default: <cartella>/<nome_cartella>_unito.root)
Serve uproot (python3 -m pip install --user uproot). Un pezzo alla volta in
memoria. Alla fine controlla righe, generati, unicita' degli EventID e che
ogni fotone abbia il suo evento.
"""
import glob
import os
import re
import sys

import numpy as np
import uproot

if len(sys.argv) < 2:
    sys.exit(__doc__)
cartella = os.path.abspath(sys.argv[1])
nome = os.path.basename(cartella.rstrip("/"))
uscita = sys.argv[2] if len(sys.argv) > 2 else os.path.join(cartella, f"{nome}_unito.root")

pezzi = sorted(glob.glob(os.path.join(cartella, "pezzo_[0-9][0-9][0-9][0-9].root")))
if not pezzi:
    sys.exit(f"nessun pezzo_NNNN.root in {cartella}")
indici = [int(re.search(r"pezzo_(\d{4})\.root$", p).group(1)) for p in pezzi]

# Pezzi senza .ok (non finiti) e pezzi mancanti rispetto a produzione.txt
senza_ok = [p for p in pezzi if not os.path.exists(p[:-5] + ".ok")]
if senza_ok:
    sys.exit("pezzi senza .ok (non finiti o falliti), da rifare prima di unire:\n  "
             + "\n  ".join(os.path.basename(p) for p in senza_ok))
testo_prod = ""
f_prod = os.path.join(cartella, "produzione.txt")
if os.path.exists(f_prod):
    testo_prod = open(f_prod).read()
    m = re.search(r"^pezzi\s+(\d+)", testo_prod, re.M)
    if m and int(m.group(1)) != len(pezzi):
        mancano = sorted(set(range(int(m.group(1)))) - set(indici))
        sys.exit(f"produzione.txt dice {m.group(1)} pezzi, ne trovo {len(pezzi)}; "
                 f"mancano: {mancano}")


def semi(pezzo_root):
    mac = pezzo_root[:-5] + ".mac"
    if os.path.exists(mac):
        m = re.search(r"^/random/setSeeds\s+(\d+)\s+(\d+)", open(mac).read(), re.M)
        if m:
            return int(m.group(1)), int(m.group(2))
    return -1, -1


def tipi(albero):
    return {k: v.interpretation.to_dtype.newbyteorder("=") for k, v in albero.items()}


with uproot.open(pezzi[0]) as f0:
    tipi_ev, tipi_fo, tipi_run = tipi(f0["Eventi"]), tipi(f0["Fotoni"]), tipi(f0["Run"])
nuove = {"Pezzo": np.dtype("i4"), "EventID_globale": np.dtype("i8")}

offset = 0
tot = {"Eventi": 0, "Fotoni": 0, "N_generated": 0}
with uproot.recreate(uscita) as out:
    t_ev = out.mktree("Eventi", {**tipi_ev, **nuove})
    t_fo = out.mktree("Fotoni", {**tipi_fo, **nuove})
    t_run = out.mktree("Run", {**tipi_run, "Pezzo": np.dtype("i4"),
                               "Seme1": np.dtype("i8"), "Seme2": np.dtype("i8")})
    for k, p in zip(indici, pezzi):
        with uproot.open(p) as f:
            if tipi(f["Eventi"]) != tipi_ev or tipi(f["Fotoni"]) != tipi_fo:
                sys.exit(f"{os.path.basename(p)}: colonne diverse dal primo pezzo")
            run = f["Run"].arrays(library="np")
            ngen = int(run["N_generated"].sum())
            ev = f["Eventi"].arrays(library="np")
            fo = f["Fotoni"].arrays(library="np")
        if len(ev["EventID"]) and ev["EventID"].max() >= ngen:
            sys.exit(f"{os.path.basename(p)}: EventID oltre i generati ({ngen})")
        for dati in (ev, fo):
            n = len(dati["EventID"])
            dati["Pezzo"] = np.full(n, k, dtype="i4")
            dati["EventID_globale"] = dati["EventID"].astype("i8") + offset
        s1, s2 = semi(p)
        nrun = len(run["N_generated"])
        run["Pezzo"] = np.full(nrun, k, dtype="i4")
        run["Seme1"] = np.full(nrun, s1, dtype="i8")
        run["Seme2"] = np.full(nrun, s2, dtype="i8")
        t_ev.extend(ev)
        t_fo.extend(fo)
        t_run.extend(run)
        tot["Eventi"] += len(ev["EventID"])
        tot["Fotoni"] += len(fo["EventID"])
        tot["N_generated"] += ngen
        offset += ngen
        print(f"{os.path.basename(p)}: {ngen} generati, {len(ev['EventID'])} eventi, "
              f"{len(fo['EventID'])} fotoni")
    if testo_prod:
        out["produzione"] = testo_prod

# Controlli sul file scritto
with uproot.open(uscita) as u:
    eg = u["Eventi"]["EventID_globale"].array(library="np")
    fg = u["Fotoni"]["EventID_globale"].array(library="np")
    ngen = int(u["Run"]["N_generated"].array(library="np").sum())
    assert u["Eventi"].num_entries == tot["Eventi"], "righe di Eventi"
    assert u["Fotoni"].num_entries == tot["Fotoni"], "righe di Fotoni"
    assert ngen == tot["N_generated"], "N_generated"
    assert len(np.unique(eg)) == len(eg), "EventID_globale ripetuti in Eventi"
    orfani = np.setdiff1d(np.unique(fg), eg).size
    assert orfani == 0, f"{orfani} eventi di Fotoni senza riga in Eventi"
print(f"\n{len(pezzi)} pezzi -> {uscita}")
print(f"  N_generated {tot['N_generated']}, Eventi {tot['Eventi']}, Fotoni {tot['Fotoni']}")
print("  controlli OK: righe, generati, EventID_globale unici, ogni fotone ha il suo evento")
