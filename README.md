# Testbeam 2026 — Water Cherenkov (simulazione Geant4 + analisi)

Simulazione **Geant4** multi-thread di un rivelatore **Water Cherenkov (WC)** per il
testbeam 2026 e gli studi di fondo a LNGS (veto muoni per **CUPID**), più i notebook di
analisi dei dati reali (DAQ) e Monte Carlo.

Due geometrie e quattro sorgenti primarie sono selezionabili **a runtime**, senza
ricompilare.

## Geometrie

| Tag | Descrizione |
|-----|-------------|
| `single` | Modulo singolo, tank HDPE 980×360×120 mm, 37 fibre WLS BCF-9995XL, 2 PMT |
| `single_wbc` / `single_wbc2` / `single_wbc3` | Varianti Wide-Bend-Curve (curve più morbide, meno perdite ottiche) |
| `triple` | Tre moduli impilati lungo Y, 6 PMT laterali |

## Sorgenti (macro GPS)

| Tag | Sorgente | Uso |
|-----|----------|-----|
| `e_tb` | e⁻ 500 MeV, fascio gaussiano | testbeam 2026 |
| `mu_tb` | μ⁻ 2700 MeV | confronto testbeam |
| `gamma` | γ isotropico (²⁰⁸Tl 2.614 MeV) | fondo LNGS |
| `mu_lngs` | μ⁻ cosmici (spettro Mei & Hime) | fondo LNGS Hall A |

## Compilazione

Richiede Geant4 (es. in un ambiente Conda `geant4_env`).

```bash
cd build
cmake ..
make -j$(sysctl -n hw.logicalcpu)   # macOS
```

Eseguibile: `build/WCtestbeamSim`.

## Esecuzione

```bash
cd build
./WCtestbeamSim single     run_single.mac   # modulo singolo (batch)
./WCtestbeamSim single_wbc run_single.mac   # variante WBC
./WCtestbeamSim triple     run_triple.mac   # triplo modulo
```

Produzione parallela con scan posizione/angolo:

```bash
cd src
bash Run_scan.sh single e_tb        # scan elettroni, modulo singolo
```

Gli output ROOT vanno in `data/` (esclusa dal versionamento).

## Analisi

Notebook Jupyter in `Analysis_script/WC_analisys_tool.ipynb` per l'analisi combinata
di dati reali (DAQ, waveform PMT/calo) e MC. Vedi [`CLAUDE.md`](CLAUDE.md) per i dettagli
di ntuple, canali DAQ e catena di analisi.

## Struttura del repository

```
sim.cc, CMakeLists.txt        main + build
include/  src/                geometrie, fisica, azioni Geant4
*.mac                         macro sorgenti e visualizzazione
Analysis_script*/             notebook di analisi
CLAUDE.md                     documentazione dettagliata del progetto
```

> **Nota**: dati (`data/`, `Data_TB/`), artefatti di build (`build/`) e output dei plot
> **non** sono versionati (vedi `.gitignore`). Sono troppo pesanti per GitHub e vanno
> tenuti/condivisi altrove.
