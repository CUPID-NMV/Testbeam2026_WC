# La simulazione in un container, per Bouchet

Tre file:

| file | dove si usa | cosa fa |
|---|---|---|
| `Dockerfile` (nella radice) | Mac | ricetta dell'immagine: Geant4 11.4.2 (`carlomt/geant4`, pubblica su Docker Hub), i dataset di Geant4, la simulazione compilata e le macro `*.mac` |
| `docker/costruisci.sh` | Mac | costruisce l'immagine per linux/amd64 e la carica su Docker Hub |
| `docker/bouchet_produci.sh` | Bouchet | produzione Hall A a pezzi in un job array di SLURM, come `produzione_hallA/produci.sh` |

Il Dockerfile della collega (cuoremc, ramo `feature/dockerBuild`) non serve: costruisce
tutto l'ambiente CUORE (ROOT, cuoremc, MVPanelsSim) dal repository cuoremc, partendo da
un'immagine del registro INFN. Questa simulazione ha bisogno solo di Geant4.

## 1. Sul Mac: costruire e caricare l'immagine (una volta per versione del codice)

Docker Desktop acceso, poi, una volta sola:

```bash
docker login
```

Dalla radice del repository, ramo `pannelli`:

```bash
bash docker/costruisci.sh <utente_dockerhub>
```

- Bouchet e' x86_64 e il Mac M4 e' arm64: lo script costruisce per `linux/amd64`. La
  prima volta scarica la base e i dataset di Geant4 (~2 GB da cern.ch). Le volte dopo
  ricompila solo la simulazione: pochi minuti, perche' su arm64 l'amd64 e' emulato.
- Il tag e' il commit (es. `28c2c05`); se ci sono modifiche non committate in `src/`,
  `include/`, `sim.cc` o nelle macro, entrano nell'immagine e il tag diventa `28c2c05-mod`.
  Meglio committare prima: dal tag si risale al codice.
- Su Docker Hub il repository `wcsim` viene creato pubblico: dentro c'e' il codice
  sorgente. Per renderlo privato: Docker Hub -> repository -> Settings -> Visibility
  (l'account gratuito ne ha uno privato). Se e' privato, su Bouchet il pull vuole
  `--docker-login`.
- Prova sul Mac, prima di caricarla: `PIATTAFORME=linux/amd64,linux/arm64` e poi
  `docker run --rm -it <utente>/wcsim:<tag> bash`, dentro `wcsim single_pan3 <macro>`.

## 2. Su Bouchet: scaricare l'immagine

Apptainer, non Docker (sui cluster Docker non c'e'). La conversione in `.sif` e' pesante:
meglio in una sessione interattiva che sul nodo di login.

```bash
salloc --cpus-per-task=4 --mem=8G --time=1:00:00
```

```bash
mkdir -p ~/wcsim && cd ~/wcsim && apptainer pull wcsim_28c2c05.sif docker://<utente_dockerhub>/wcsim:28c2c05
```

Prova veloce (100 gamma):

```bash
printf '/wc/output/onlyActive true\n/analysis/setFileName prova\n/run/initialize\n/control/execute /opt/wcsim/gun_gammas_background_righe_pannelli.mac\n/run/beamOn 100\n' > prova.mac && apptainer exec --cleanenv ~/wcsim/wcsim_28c2c05.sif wcsim single_pan3 prova.mac
```

Deve uscire `prova.root` con le ntuple `Eventi`, `Fotoni`, `Run`.

Nell'immagine: `wcsim` e' il lanciatore (stessi argomenti di `WCtestbeamSim`, imposta i
dataset), le macro sono in `/opt/wcsim/`, il commit in `/opt/wcsim/GIT_COMMIT`.

## 3. Su Bouchet: la produzione

Copiare `docker/bouchet_produci.sh` su Bouchet e lanciarlo con `bash`, non con `sbatch`:
calcola i pezzi e sottomette se stesso come array.

```bash
SIF=~/wcsim/wcsim_28c2c05.sif OUT=~/scratch/wcsim/gamma_24h bash bouchet_produci.sh gamma 24
```

- `gamma 24` = 24 ore di gamma di Hall A, 20608 Hz, 1.78e9 eventi, 357 pezzi da 5e6.
  `muoni` per i muoni di `gun_muons_hallA.mac`.
- I semi partono da una base diversa da quella del Mac (10 gamma, 20 muoni contro 1 e
  2), quindi le produzioni sono indipendenti e si possono sommare. Per un'altra
  produzione ancora: `SEMI=11`.
- Partizione e tempo nelle righe `#SBATCH` in cima (default `day`, 6 ore, 2 GB).
  Verificare i nomi delle partizioni con `sinfo`; per cambiarle senza toccare il file:
  `SBATCH_EXTRA="--partition=... --time=..."`.
- Ottica dei pannelli spenta e `onlyActive true`, come sul Mac. `MAC_EXTRA` e
  `MACRO_GAMMA` funzionano come in `produci.sh`.
- Stato: `squeue --me`; pezzi finiti: `ls $OUT/*.ok | wc -l`. Se qualche pezzo fallisce
  o va oltre il tempo, rilanciare lo stesso comando: rifa' solo i pezzi senza `.ok`.
- I `.root` si analizzano come quelli del Mac (`digit_daq.py`, `SiPM_maker.py`).

## Quando il codice cambia

Commit, `bash docker/costruisci.sh <utente>` (nuovo tag), su Bouchet `apptainer pull` del
nuovo tag. Le produzioni gia' fatte restano legate alla loro immagine (`produzione.txt`
riporta il commit).
