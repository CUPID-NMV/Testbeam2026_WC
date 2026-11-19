# Produzione su Bouchet: un anno di muoni di Hall A con gli angoli di AGATA, ottica dei pannelli accesa

Istruzioni per lanciare la produzione su Bouchet (SLURM, Apptainer). Sono scritte per
chi la esegue (Donatella e il suo assistente). I dettagli generali del container e
dello script sono in `docker/LEGGIMI.md` del repository, ramo `pannelli`.

## Perche' una produzione nuova

Il 18/11 sono stati corretti gli angoli dei muoni in `gun_muons_hallA.mac`
(commit `01cb91c`, "Muoni di Hall A: angoli allineati ad AGATA"; merge `9bbd4c2`):
prima c'erano troppi muoni quasi verticali. I muoni dei 4 mesi gia' prodotti
(`muoni_4mesi_unito.root`, immagine `dbadf65`) hanno gli angoli vecchi e non si possono
sommare ai nuovi. Serve un campione nuovo, da solo.

Il codice della simulazione e' lo stesso: fra `dbadf65` e `9bbd4c2` in `src/`,
`include/`, `sim.cc` e `CMakeLists.txt` non cambia nulla. Cambia solo la macro dei muoni.

## Parametri della produzione

| parametro | valore | perche' |
|---|---|---|
| sorgente | `muoni` (macro `gun_muons_hallA.mac`, quella con gli angoli di AGATA) | |
| durata | **8760 ore** (1 anno) | ~3200 muoni utili nel WC: efficienza con +-0.4% di statistica |
| rate | 0.001379 Hz, letto dallo script dalla macro | -> **43488 muoni, 44 pezzi** |
| eventi per pezzo | **1000** | ~13 s per muone con l'ottica accesa: ~3.6 ore a pezzo, dentro le 12 ore del job |
| ottica dei pannelli | **`OTTICA=true`** | luce dei pannelli simulata (scintillazione, fibre, SiPM) |
| righe di macro in piu' | **`/wc/fiber/scintYield 7000`** e **`/wc/output/photonOrigin true`** | INDISPENSABILI: servono a riscalare la scintillazione delle fibre del WC in digitalizzazione. Senza, il campione non si puo' usare |
| semi | **`SEMI=21`** | i 4 mesi vecchi hanno usato la base 20: con 21 la produzione nuova e' indipendente |

## Regole

- Non modificare la macro, lo script o l'immagine. Non cambiare altri parametri oltre a
  quelli della tabella.
- Lo script si lancia con `bash`, sul nodo di login. **Non con `sbatch`**: calcola i
  pezzi e sottomette se stesso come job array.
- Niente commit o push sul repository: e' pubblico.
- Prima la prova con un pezzo piccolo (punto 3). Mostrare il risultato della prova a
  Donatella prima di sottomettere la produzione vera.

## 1. Lo script

Serve `bouchet_produci.sh` nella versione con la variabile `OTTICA`. E' quella gia'
usata per i 4 mesi:

```bash
grep -c OTTICA bouchet_produci.sh     # deve dare un numero > 0
```

Se manca, si prende dal clone del repository (in `LEGGIMI.md` e' `~/Testbeam2026_WC`):

```bash
cd ~/Testbeam2026_WC && git fetch origin && git show 9bbd4c2:docker/bouchet_produci.sh > ~/wcsim/bouchet_produci.sh
```

## 2. L'immagine

Serve un'immagine che contenga la macro con gli angoli di AGATA. Quella dei 4 mesi
(`wcsim_dbadf65.sif`) **non** va bene cosi' com'e'.

**Strada A (da preferire): immagine nuova `9bbd4c2`.** La costruisce Luigi sul Mac
(`bash docker/costruisci.sh <utente_dockerhub>` da un checkout pulito del ramo
`pannelli`). L'account Docker Hub e' lo stesso dell'immagine `dbadf65`: si ritrova nella
cronologia, `grep -h "apptainer pull" ~/.bash_history`. Il pull va fatto in una sessione
interattiva, non sul nodo di login:

```bash
salloc --cpus-per-task=4 --mem=8G --time=1:00:00
cd ~/wcsim && apptainer pull wcsim_9bbd4c2.sif docker://<utente_dockerhub>/wcsim:9bbd4c2
```

Controllo, obbligatorio:

```bash
apptainer exec ~/wcsim/wcsim_9bbd4c2.sif cat /opt/wcsim/GIT_COMMIT                        # 9bbd4c2, senza "-mod"
apptainer exec ~/wcsim/wcsim_9bbd4c2.sif grep -c AGATA /opt/wcsim/gun_muons_hallA.mac    # > 0
apptainer exec ~/wcsim/wcsim_9bbd4c2.sif grep "Rate:" /opt/wcsim/gun_muons_hallA.mac     # ... = 0.001379 Hz
```

Va bene anche un tag piu' recente, purche' passi gli stessi controlli. Se il tag non esiste
su Docker Hub, fermarsi e chiedere a Luigi di costruirlo, oppure usare la strada B.

**Strada B (ripiego, solo se l'immagine nuova non c'e'): immagine `dbadf65` con la macro
nuova montata sopra quella vecchia.** Il codice e' identico, quindi il risultato e' lo
stesso. La macro si prende dal commit `9bbd4c2`, non da un ramo che puo' essere andato
avanti:

```bash
cd ~/Testbeam2026_WC && git fetch origin && git show 9bbd4c2:gun_muons_hallA.mac > ~/wcsim/gun_muons_hallA_agata.mac
export APPTAINER_BIND=$HOME/wcsim/gun_muons_hallA_agata.mac:/opt/wcsim/gun_muons_hallA.mac
apptainer exec ~/wcsim/wcsim_dbadf65.sif grep "Rate:" /opt/wcsim/gun_muons_hallA.mac     # deve dire 0.001379 Hz
```

`APPTAINER_BIND` deve restare esportata nella shell da cui si lanciano la prova e la
produzione: lo script la passa ai job (`sbatch --export=ALL`). In questo caso, nei comandi
qui sotto `SIF=~/wcsim/wcsim_dbadf65.sif`. Dopo ogni sottomissione si aggiunge una riga
a `produzione.txt` della cartella d'uscita, perche' quel file riporterebbe il commit
`dbadf65`:

```bash
echo "macro_montata   gun_muons_hallA.mac di 9bbd4c2 (angoli AGATA) via APPTAINER_BIND" >> $OUT/produzione.txt
```

## 3. Prova: un pezzo da 20 ore (99 muoni, ~20 minuti)

Stessi parametri della produzione, in una cartella a parte. La terza riga di
`MAC_EXTRA` scrive nel log del pezzo la riga del rate della macro che il job vede
davvero:

```bash
SIF=~/wcsim/wcsim_9bbd4c2.sif OUT=~/scratch/wcsim/prova_agata OTTICA=true SEMI=21 \
MAC_EXTRA=$'/wc/fiber/scintYield 7000\n/wc/output/photonOrigin true\n/control/shell grep Rate: /opt/wcsim/gun_muons_hallA.mac' \
bash bouchet_produci.sh muoni 20 1000
```

Quando c'e' `~/scratch/wcsim/prova_agata/pezzo_0000.ok`:

```bash
cat ~/scratch/wcsim/prova_agata/produzione.txt
grep "Rate:" ~/scratch/wcsim/prova_agata/pezzo_0000.log
python3 - ~/scratch/wcsim/prova_agata/pezzo_0000.root <<'EOF'
import sys, numpy as np, uproot
f = uproot.open(sys.argv[1])
fo = f["Fotoni"].arrays(["PMT_ID", "Origine"], library="np")
wc = np.isin(fo["PMT_ID"], (0, 1))
sipm = np.isin(fo["PMT_ID"], (6, 7, 8))
print("eventi scritti:", f["Eventi"].num_entries)
print("fotoni ai PMT del WC:", wc.sum(), "- di cui con Origine = -1:", (fo["Origine"][wc] == -1).sum())
print("fotoni ai SiPM dei pannelli:", sipm.sum())
EOF
```

La prova e' buona se:

- `produzione.txt` dice `rate_Hz 0.001379`, `ottica_pannelli true`, `semi base 21`, e
  `macro_extra` contiene `scintYield 7000` e `photonOrigin true`;
- nel log c'e' la riga `# Rate: ... = 0.001379 Hz`. Se dice 0.001789 il job usa la macro
  vecchia: fermarsi;
- ci sono fotoni ai PMT del WC e **nessuno** ha `Origine = -1` (se no `photonOrigin` e'
  spento);
- ci sono fotoni ai SiPM dei pannelli (se no l'ottica dei pannelli e' spenta).

Se un controllo non passa, non lanciare la produzione: riferire a Donatella cosa e'
uscito. La cartella `prova_agata` non va unita alla produzione e si puo' cancellare.

## 4. La produzione

```bash
SIF=~/wcsim/wcsim_9bbd4c2.sif OUT=~/scratch/wcsim/muoni_agata_1anno OTTICA=true SEMI=21 \
MAC_EXTRA=$'/wc/fiber/scintYield 7000\n/wc/output/photonOrigin true' \
bash bouchet_produci.sh muoni 8760 1000
```

Deve stampare `muoni: 43488 eventi (8760 h a 0.001379 Hz) in 44 pezzi`. Poi:

- stato: `squeue --me`; pezzi finiti: `ls ~/scratch/wcsim/muoni_agata_1anno/*.ok | wc -l` (alla fine 44);
- se qualche pezzo fallisce o va oltre le 12 ore, si rilancia **lo stesso comando**
  (stessa shell, stesse variabili): rifa' solo i pezzi senza `.ok`. Un pezzo che supera
  le 12 ore si rilancia con `SBATCH_EXTRA="--time=1-00:00:00 --partition=week"`
  (controllare i nomi con `sinfo`).

## 5. Unire i pezzi

Come per i 4 mesi, con `unisci_pezzi.py` del repository (non `hadd`: l'`EventID` riparte
da 0 in ogni pezzo):

```bash
python3 ~/Testbeam2026_WC/docker/unisci_pezzi.py ~/scratch/wcsim/muoni_agata_1anno
```

Scrive `muoni_agata_1anno_unito.root` nella stessa cartella (circa 300 MB). Lo script
rifiuta di unire se manca un pezzo e alla fine controlla righe, generati e legame
fotoni-eventi: il suo riepilogo va riportato.

## 6. Cosa mandare a Luigi

- `muoni_agata_1anno_unito.root`, per la stessa via di `muoni_4mesi_unito.root`;
- il testo di `produzione.txt`, con la riga `macro_montata` se si e' usata la strada B;
- il riepilogo di `unisci_pezzi.py`;
- se ci sono stati pezzi rilanciati o problemi, quali.
