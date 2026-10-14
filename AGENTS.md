# WC Testbeam 2026 — Simulazione Geant4 (progetto unificato)

## Cos'è questo progetto

Simulazione Geant4 multi-thread di un rivelatore **Water Cherenkov (WC)** per il testbeam 2026 e gli studi di fondo a LNGS.
Supporta due geometrie e quattro tipi di sorgente selezionabili a runtime senza ricompilazione.

## Geometrie disponibili

### Modulo singolo (`single`)
- **Tank HDPE**: 980 × 360 × 120 mm, pareti 10 mm
- **Volume d'acqua**: 960 × 360 × 100 mm
- **Rivestimento interno**: Teflon (PTFE), riflettività Lambertian dipendente dall'energia (0.97 @ 450 nm → 0.91 @ 250 nm)
- **37 fibre WLS** (BCF-9995XL, r_core=0.47 mm, r_clad=0.5 mm, pitch=1.05 mm):
  - Tratto dritto in acqua fino al pettine HDPE a X=380 mm
  - Curva Bézier 3D (20 segmenti) verso il bundle verticale
  - Bundle verticale verso i 2 PMT sulla faccia +Z
- **2 PMT** (disco vetro r=3.8 mm) a Z=59.5 mm (faccia +Z)
- **Volume mondo**: 18 × 6 × 6 m (±9 m in X per coprire fasci inclinati fino a 75°)

### Modulo singolo WBC (`single_wbc`)
Variante Wide-Bend-Curve del modulo singolo — stesso tank, acqua e proprietà ottiche.
Motivazione: pettini più vicini al centro → Bézier più ampia → curva più morbida → minori perdite ottiche.

| Parametro | `single` | `single_wbc` |
|-----------|----------|--------------|
| `x_st1` (posizione pettine) | 380 mm | **330 mm** |
| `x_curve_start` | 388 mm | **338 mm** |
| `bezier_dX` | 52 mm | **102 mm** |
| `z_inizio_bundle` | 35 mm | **45 mm** |
| `bundle_in_water_len` | 15 mm | **5 mm** |
| Raggio min curva XZ (calc.) | ~27 mm | **~30 mm** |

File: `include/SingleModuleDetectorWBC.hh`, `src/SingleModuleDetectorWBC.cc`

### Modulo singolo WBC2 (`single_wbc2`)
Come `single_wbc` (pettini a 330 mm) ma con profilo XZ della curva **retta + quarto di cerchio R=45 mm** al posto della Bézier quadratica. Morphing y con smoothstep sulla frazione di lunghezza d'arco (stessa struttura collision-safe dell'originale). Nota: spostare i pettini oltre 330 mm NON migliora i raggi (vincolo verticale: R ≤ z_bundle ≈ 45–48 mm).

| Raggio min curvatura | `single` | `single_wbc` | `single_wbc2` |
|----------------------|----------|--------------|---------------|
| fibra centrale | 26.9 mm | 30.4 mm | **45.0 mm** |
| fibra peggiore (Δy=141 mm) | 5.7 mm | 9.3 mm | **17.9 mm** |

File: `include/SingleModuleDetectorWBC2.hh`, `src/SingleModuleDetectorWBC2.cc`

Il numero di fibre è parametrico (costruttore, default 37). Tag `single_wbc2_19f` = stessa geometria con **19 fibre** (anelli esagonali 0–2, pettine ±72 mm): serve a studiare il trade-off cattura (meno materiale WLS) vs trasporto (curve più morbide per le fibre di bordo, Δy max 70 mm invece di 141 mm). Tag `single_wbc2_19fw` = 19 fibre a passo 16 mm (copertura piena ±144 mm).

### Modulo singolo WBC3 (`single_wbc3`)
**Due layer di fibre** che convergono in un bundle unico da **45 fibre** su PMT Ø8 mm (r=4.0, H10721):
- Layer 1: 23 fibre a z=0, curva retta+cerchio R=45 mm
- Layer 2: 22 fibre a z=−45 mm, curva R=90 mm (più morbida, sfrutta la salita doppia)
- Pettini a x=±330 mm, passo 13 mm, griglie sfalsate di 6.5 mm → campionamento y combinato 6.5 mm su ±143
- Slot bundle partizionati in x (L1 dietro xb<0, L2 avanti xb>0) per evitare collisioni nella zona di merge (validato: dist min assi 0.98 mm)
- Posizioni bundle da impacchettamento numerico (min dist 1.02 mm), tabella hard-coded nel .cc

File: `include/SingleModuleDetectorWBC3.hh`, `src/SingleModuleDetectorWBC3.cc`

### Tank a lunghezza variabile (`single_wbc2_long/short/mini`, `single_wbc3_long`)
Varianti a **lunghezza del tank parametrica** per studiare la raccolta di luce (riflessioni Teflon + attenuazione WLS) in funzione della taglia del tank, dai più piccoli ai più lunghi del muon veto reale. Cambia **solo la lunghezza (X)**; larghezza (360 mm acqua) e profondità (100 mm) restano quelle del prototipo → isola l'effetto taglia.

| Tag (WBC2 e WBC3) | `tankHalfLenX` | Tank (mm) | Tratto fibra in acqua |
|-----|----------------|-----------|-----------------------|
| `single_wbc{2,3}_mini`  | 200  | 400  | ~45 mm |
| `single_wbc{2,3}_short` | 245  | 490  | ~90 mm |
| `single_wbc2` / `single_wbc3` (default) | 490 | 980 | ~335 mm |
| `single_wbc{2,3}_long` | 1580 | 3160 | ~1425 mm |

(WBC3 `mini` è al limite: la curva layer-2 R=90 mm sta in un tank da 400 mm con margine ~12 mm → warning ottico innocuo.)

- Implementazione **parametrica** (nessuna duplicazione): costruttore `SingleModuleDetectorWBC2(nFib, combPitch, tankHalfLenX)` e `SingleModuleDetectorWBC3(tankHalfLenX)`, default `tankHalfLenX=490` mm = geometria attuale invariata. Pettine e PMT ancorati all'estremità (offset fissi): `x_st1 = tankHalfLenX − 160`, `X_PMT = tankHalfLenX − 50`. Minimo realistico ~`tankHalfLenX=200` (x_st1 > 0).
- Le fibre sono lette **ai due estremi** (2 PMT). Confrontando ⟨PE⟩ e l'efficienza tra le taglie si misura direttamente la raccolta-luce-vs-lunghezza (fattore ~30 sul tratto fibra tra mini e long).

### Triplo modulo (`triple`)
- Tre moduli impilati lungo Y, centrati a Y = −306.5, 0, +306.5 mm
- Stesso volume acqua per modulo (960 × 360 × 100 mm)
- Modulo inferiore: 18 fibre dritte; moduli centrale e superiore: 37 fibre con routing ad arco
- Separatori HDPE + riflettori Mylar tra i moduli
- **6 PMT laterali** a X = ±500.5 mm (una coppia per modulo: Bot/Mid/Top × L/R)
- **Volume mondo**: 18 × 6 × 6 m (±9 m in X per coprire fasci inclinati fino a 75°)

## TODO / idee da valutare

- **Pettine centrale unico (2 PMT, 1 supporto)** — variante di routing fibre da implementare e simulare.
  Idea: tenere i **due PMT** alle estremità ma sostituire i **due pettini** di supporto attuali (a ±x_st1)
  con **un solo pettine centrale**. Punto chiave: far **convergere le fibre in Y gradualmente** lungo il
  tratto (dal centro all'estremità) invece di restare "spread" (y_comb, ±144 mm) fino alla curva finale.
  Così la curva finale è **solo in Z** (salita al PMT, R=45 mm) → si elimina il morphing Y stretto che oggi
  dà la fibra peggiore a **R=17.9 mm**. Benefici attesi: meno HDPE in acqua (marginale +luce), meccanica
  più semplice, curve più morbide (grande sui tank medio-lunghi, minore sui mini). NB: non basta togliere
  un pettine — serve cambiare il routing (convergenza Y). Da fare: variante geometria + confronto ⟨PE⟩ e
  raggio minimo di curvatura per fibra vs design attuale. (Rilevante soprattutto per WBC3 e tank corti.)

## Sorgenti primarie (selezionabili da macro)

| Macro | Tag script | Sorgente | Uso |
|-------|-----------|----------|-----|
| `gun_electrons_testbeam.mac` | `e_tb` | e− 500 MeV, fascio gaussiano σ=0.85 mm da Z=2000 mm | testbeam 2026 |
| `gun_muons_testbeam.mac` | `mu_tb` | μ− 2700 MeV, stesso fascio degli elettroni | confronto testbeam |
| `gun_gammas_background.mac` | `gamma` | γ isotropico, sorgente superficiale, default 2.614 MeV (²⁰⁸Tl) | studi fondo LNGS |
| `gun_muons_lngs.mac` | `mu_lngs` | μ− cosmici, spettro Mei & Hime (picco ~270 GeV), distribuzione cos²θ, piano 6500×6500 mm | studi fondo LNGS Hall A |

La sorgente si seleziona cambiando il `/control/execute` nel macro principale (run_single.mac o run_triple.mac).

## Fisica attiva

### Processi EM (`G4EmStandardPhysics` + `G4EmExtraPhysics`)
- Ionizzazione, bremsstrahlung, scattering multiplo (tutte le particelle cariche)
- Compton, effetto fotoelettrico, pair production (gamma)
- **Muon-nuclear** e **gamma-nuclear** (via `G4EmExtraPhysics`) — rilevanti per μ− cosmici a ~270 GeV

### Processi ottici (`G4OpticalPhysics`)
| Processo | Stato |
|----------|-------|
| Cerenkov | ✅ attivo |
| OpAbsorption | ✅ attivo |
| OpRayleigh | ✅ attivo |
| OpBoundary | ✅ attivo (riflessione Teflon/Mylar) |
| OpWLS | ✅ attivo (fibre BCF-9995XL) |
| Scintillation | ❌ disabilitata |

## Output ROOT

I file ROOT vengono scritti nella cartella `data/`.

**Naming convention** (produzione con Run_scan.sh):
`sim_<geo>_<src>_X<x>mm_Y<y>mm_Th<theta>deg.root`

Esempi:
```
sim_single_e_tb_X0mm_Y0mm_Th0deg.root
sim_triple_mu_tb_X200mm_Y0mm_Th15deg.root
sim_single_mu_lngs_X0mm_Y0mm_Th0deg.root
```

I run di test diretti (`./WCtestbeamSim single run_single.mac`) scrivono `build/test_debug.root`.

**Ntuple 0 — "Eventi"** (una riga per particella primaria, 18 colonne):
| col | nome | unità | note |
|-----|------|-------|------|
| 0 | E_init_MeV | MeV | energia cinetica primaria all'entrata in acqua |
| 1 | TrackL_water_mm | mm | lunghezza traccia primaria in acqua |
| 2 | Edep_water_MeV | MeV | energia depositata in acqua (0 per gamma: deposita tramite secondari) |
| 3 | N_Cerenkov_prod | — | fotoni Cherenkov prodotti totali |
| 4 | N_Cerenkov_entrati | — | fotoni Cherenkov entrati in una fibra |
| 5 | T_FirstHit_ns | ns | tempo primo hit PMT (−1 se nessun hit) |
| 6 | Hits_L_Bot | — | PMT Left-Bottom |
| 7 | Hits_R_Bot | — | PMT Right-Bottom |
| 8 | Hits_L_Mid | — | PMT Left-Mid (0 per geometria single) |
| 9 | Hits_R_Mid | — | PMT Right-Mid (0 per geometria single) |
| 10 | Hits_L_Top | — | PMT Left-Top (0 per geometria single) |
| 11 | Hits_R_Top | — | PMT Right-Top (0 per geometria single) |
| 12 | SourceType | — | 0=e− (testbeam), 1=γ (fondo), 2=μ− (cosmici/testbeam) |
| 13 | N_Cer_primary | — | Cherenkov da particella primaria diretta |
| 14 | N_Cer_compton | — | Cherenkov da elettroni Compton o fotoelettrici |
| 15 | N_Cer_pair | — | Cherenkov da coppie e+/e− (pair production) |
| 16 | N_Cer_deltaEM | — | Cherenkov da delta-ray (ionizzazione EM secondaria) |
| 17 | N_Cer_nuclear | — | Cherenkov da prodotti di interazioni nucleari |

**Ntuple 1 — "Fotoni"** (una riga per fotone arrivato al PMT):
| col | nome | unità |
|-----|------|-------|
| 0 | EventID | — |
| 1 | TrackID | — |
| 2 | E_Hit_eV | eV |
| 3 | Arrival_Time_ns | ns |
| 4 | PMT_ID | 0=L_Bot, 1=R_Bot, 2=L_Mid, 3=R_Mid, 4=L_Top, 5=R_Top |

## Compilazione

Richiede Geant4 installato nell'ambiente Conda.

```bash
conda activate geant4_env
cd build
cmake ..
make -j$(nproc)          # Linux
make -j$(sysctl -n hw.logicalcpu)  # macOS
```

L'eseguibile è `build/WCtestbeamSim`.

> I nuovi file `.cc` in `src/` vengono inclusi automaticamente dal glob in `CMakeLists.txt` — non serve modificare il CMakeLists quando si aggiunge una nuova geometria.

## Esecuzione

```bash
conda activate geant4_env
cd build

# Modalità batch (output: data/sim_<geo>_<src>_*.root)
./WCtestbeamSim single     run_single.mac   # modulo singolo
./WCtestbeamSim single_wbc run_single.mac   # variante WBC
./WCtestbeamSim triple     run_triple.mac   # triplo modulo

# GUI interattiva (poi nel terminale Geant4: /run/initialize, /control/execute <mac>)
./WCtestbeamSim single
./WCtestbeamSim single_wbc
./WCtestbeamSim triple
```

Per cambiare sorgente nel test rapido, editare la riga `/control/execute` in `run_single.mac` o `run_triple.mac`.

**Produzione parallela con scan posizione/angolo** (output in `data/`, max 8 core):
```bash
conda activate geant4_env
cd src

bash Run_scan.sh single     e_tb      # scan elettroni, modulo singolo
bash Run_scan.sh single_wbc e_tb      # scan elettroni, variante WBC
bash Run_scan.sh triple     mu_tb     # scan muoni testbeam, triplo modulo
bash Run_scan.sh single     mu_lngs   # cosmici LNGS, run singolo
bash Run_scan.sh single_wbc mu_lngs   # cosmici LNGS, variante WBC
bash Run_scan.sh triple     gamma     # gamma background, run singolo
```

Parametri di scan configurabili in cima a `Run_scan.sh`:
- `EVENTS` — eventi per run
- `X_POSITIONS` — posizioni X in mm (default: -400 a +400, step 100)
- `Y_POSITIONS` — posizioni Y in mm
- `ANGLES` — angoli fascio in gradi nel piano XZ (default: 0, 15, 30, 45, 60, 75)

## File principali

| file | ruolo |
|------|-------|
| `sim.cc` | main: parsing argv (geometria + macro), inizializza Geant4 MT |
| `src/SingleModuleDetector.cc` | geometria modulo singolo (pettini a X=±380 mm, Bézier dX=52 mm) |
| `src/SingleModuleDetectorWBC.cc` | variante WBC (pettini a X=±330 mm, Bézier dX=102 mm, z_bundle=45 mm) |
| `src/SingleModuleDetectorWBC2.cc` | variante WBC2 (pettini a 330 mm, profilo retta+cerchio R=45 mm) |
| `src/TripleModuleDetector.cc` | geometria triplo modulo (3 moduli, Mylar, 6 PMT laterali) |
| `src/PrimaryGeneratorAction.cc` | wrapper G4GeneralParticleSource, imposta SourceType in EventAction |
| `src/EventAction.cc` | accumula contatori per evento, riempie ntuple 0 (18 colonne) |
| `src/PMTSD.cc` | sensitive detector PMT unificato (mappa SD name → PMT ID 0–5) |
| `src/RunAction.cc` | apre/chiude file ROOT, definisce struttura ntuple (18+5 colonne) |
| `src/SteppingAction.cc` | tracking step-by-step, conta e categorizza Cherenkov per processo |
| `src/PhysicsList.cc` | fisica EM + extra (muon/gamma nuclear) + ottica |
| `gun_electrons_testbeam.mac` | GPS: e− 500 MeV, fascio gaussiano |
| `gun_muons_testbeam.mac` | GPS: μ− 2700 MeV, stesso fascio degli elettroni |
| `gun_gammas_background.mac` | GPS: γ isotropico, sorgente superficiale |
| `gun_muons_lngs.mac` | GPS: μ− cosmici LNGS, spettro Mei & Hime, piano 6500×6500 mm |
| `run_single.mac` / `run_triple.mac` | macro test rapido (seleziona sorgente con /control/execute) |
| `init_vis.mac` / `vis.mac` | visualizzazione OpenGL interattiva |
| `src/Run_scan.sh` | produzione parallela con scan X/Y/angolo, naming automatico |

## Note tecniche

- **Multi-thread**: `ActionInitialization::Build()` crea `EventAction` per ogni worker. `PMTSD` e `SteppingAction` usano `G4EventManager::GetEventManager()` (thread-local, non il RunManager globale).
- **GPS**: tutta la configurazione della sorgente (tipo, energia, posizione, spettro) è nei file `.mac` — nessun parametro hard-coded nel codice C++. In modalità GUI, `/run/initialize` deve precedere qualsiasi comando `/gps/*`.
- **macroPath** impostato a `../` dal main: nelle macro i file `.mac` si referenziano per nome senza percorso.
- **SourceType** viene rilevato automaticamente dal nome della particella GPS prima della generazione del vertice. Nota: μ− testbeam e μ− LNGS danno entrambi SourceType=2 — si distinguono dal nome del file ROOT.
- **Edep_water_MeV** (col 2) è sempre ~0 per eventi gamma: il primario γ non ionizza direttamente, i secondari (Compton, pair) depositano energia ma vengono tracciati separatamente.
- **N_Cer_primary…N_Cer_nuclear** (col 13–17): somma ≤ N_Cerenkov_prod (alcuni fotoni Cherenkov vengono da processi EM non categorizzati come eBrem).
- **Volume mondo ±9 m in X**: necessario per il piano di generazione dei fasci inclinati. A θ=75°, X_START ≈ X − 7464 mm.
- I file ROOT di output vanno nella cartella `data/`.

---

## Notebook di analisi dati reali + MC

**File**: `Analysis_script/WC_analisys_tool.ipynb`

Notebook Jupyter (singola cella di codice) per l'analisi dei dati DAQ reali e della simulazione MC.
Richiede il virtual environment `~/venvs/jupyter-global`.

### DAQ — parametri hardware

- **Frequenza di campionamento**: 2500 MHz → dt = 0.4 ns, 1024 campioni/traccia (~410 ns totali)
- **Canali reali**:

| Modulo | ch_A (PMT sx) | ch_B (PMT dx) | ch_trig | ch_calo |
|--------|--------------|--------------|---------|---------|
| Single mod0 | 27 | 26 | 17 | 16 |
| Triple Top mod1 | 19 | 18 | 17 | 16 |
| Triple Mid mod2 | 21 | 20 | 17 | 16 |
| Triple Bot mod3 | 23 | 22 | 17 | 16 |

### Parametri principali (sezione in cima alla cella)

| Parametro | Valore attuale | Significato |
|-----------|---------------|-------------|
| `RUN_SEL` | `None` / `1339` / `(1339,1350)` / `(1339,None)` | tutti / singolo / range / da run N in poi |
| `MODULE_SEL` | `None` / `0` / `[0,2]` | tutti / singolo / lista moduli |
| `ANALYSIS_MODE` | `"DATA"` / `"MC"` / `"BOTH"` | sorgente dati |
| `CALO_THR` | `-40` | soglia ampiezza minima calo (ADC) |
| `CALO_Q_MIN` | `-37500` | carica minima calo (ADC·sample) |
| `CALO_Q_MAX` | `-5500` | carica massima calo (ADC·sample) |
| `PMT_BL_LEFT_NS` | `(0.0, 100.0)` | regione sinistra per baseline PMT (ns assoluti) |
| `PMT_BL_RIGHT_NS` | `(300.0, 400.0)` | regione destra per baseline PMT (ns assoluti) |
| `CALO_BL_LEFT_NS` | `(0.0, 100.0)` | regione sinistra per baseline Calo (ns assoluti) |
| `CALO_BL_RIGHT_NS` | `(200.0, 400.0)` | regione destra per baseline Calo (ns assoluti) |

**Dizionario `DEBUG`** (finestre di analisi, tutte in ns assoluti dall'inizio della traccia):

| Chiave | Valore attuale | Uso |
|--------|---------------|-----|
| `pmt_sig_win_ns` | `(100.0, 180.0)` | ROI segnale PMT (selezione efficienza + aggancio gate Q) |
| `pmt_ped_win_ns` | `(240.0, 250.0)` | ancora per ped gate dinamico — ped reale = `[ped[0]+PED_WINDOW_NS, ped[0]+2·PED_WINDOW_NS]` = [250, 270] ns (post-segnale) |
| `calo_sig_win_ns` | `(120.0, 250.0)` | finestra integrazione carica calo |
| `calo_ped_win_ns` | `(20.0, 80.0)` | finestra piedistallo calo per istogramma Q |

### Due linee di analisi COMPLETAMENTE indipendenti

Dopo la sottrazione baseline (preprocessing comune) l'analisi si separa in due linee che **non condividono nessun parametro**. Cambiare un parametro di una linea non tocca l'altra.

| | **LINEA TIMING** | **LINEA CARICA (Q)** |
|--|--|--|
| Segnale | `sigA`/`sigB` = `apply_pmt_filter(...)` | `sigA_q`/`sigB_q` = `apply_charge_filter(...)` |
| Flag filtri | `PMT_FILTER_SAVGOL`, `PMT_FILTER_LOWPASS` | `PMT_Q_FILTER_SAVGOL`, `PMT_Q_FILTER_LOWPASS` |
| Param SavGol | `PMT_SMOOTH_WINDOW`, `PMT_SMOOTH_POLYORDER` | `PMT_Q_SMOOTH_WINDOW`, `PMT_Q_SMOOTH_POLYORDER` |
| Param LP | `PMT_LP_ORDER`, `PMT_LP_CUTOFF` | `PMT_Q_LP_ORDER`, `PMT_Q_LP_CUTOFF` |
| Soglia | `SIG_THR_A/B` (da `get_calib`: −12/−14 dati, −11 MC) | `Q_GATE_THR_A`, `Q_GATE_THR_B` (−2) |
| Aggancio fronte | `GATE_LEAD_NS` (60 ns) | `Q_GATE_LEAD_NS` (20 ns) |
| Altri | `CFD_DELAY_NS`, `CFD_SEARCH_WIN_NS`, `CFD_FRACTION` | `ADC_GATE_START_NS`, `PULSE_WINDOW_NS` (60 ns), `PMT_Q_LOCAL_PED*`, `Q_PED_NSIGMA` (2.0) |
| Prodotti | `t_res=(t_A−t_B)/2`, efficienza-mV `n_A/n_B/n_any` | `Q_A/Q_B`, efficienza in carica `n_*_q` e `n_*_qfix`, `ln(Q_A/Q_B)` |

Il blocco parametri in cima alla cella è raggruppato con header: `# --- COMUNI/PREPROCESSING ---`, `# --- LINEA TIMING ---`, `# --- LINEA CARICA / Q ---`, `# --- CALORIMETRO ---`.

### Elaborazione del segnale

1. **Sottrazione baseline PMT** (comune): interpolazione lineare tra mediana regione sx (`PMT_BL_LEFT_NS`) e dx (`PMT_BL_RIGHT_NS`) → retta sottratta dall'intera traccia
2. **Modo comune per-canale** (opzionale): `CM_REF_CHANNEL_A/B` — sottrae la WF raw di un canale di riferimento dal canale PMT corrispondente (`None` = off). Attualmente impostato a ch 19 (stesso gruppo digitizer dei PMT ch 26/27: gruppo 16–23).
3. **Modo comune A+B** (opzionale, solo DATA): `PMT_Q_CM_AB=False` — sottrae `(sigA_raw+sigB_raw)/2` interpolata fuori dalla finestra segnale dalla WF di carica di ogni canale, prima di `apply_charge_filter`. Attualmente disabilitato.
4. **Filtri timing** (`apply_pmt_filter`): LP Butterworth e/o SavGol con i parametri `PMT_*`, applicati a `sigA/sigB`
5. **Filtri carica** (`apply_charge_filter`): LP e/o SavGol con i parametri `PMT_Q_*`, applicati **solo** a `sigA_q/sigB_q`. FIR high-pass (`PMT_Q_FILTER_HIGHPASS=False`) attualmente disabilitato.
6. **Trigger / Calo**: SavGol fisso + interpolazione baseline propria

### Due metodi di efficienza in carica

#### Metodo 1 — Gate dinamico
- Il **gate** è ancorato sul fronte di `sigA_q`/`sigB_q`: `leading_edge_crossing(sigA_q, Q_GATE_THR_A, lead=Q_GATE_LEAD_NS)` nella ROI `pmt_sig_win_ns`
- Gate: `[t_in − ADC_GATE_START_NS, t_in − ADC_GATE_START_NS + PULSE_WINDOW_NS]`
- Solo eventi con impulso visibile sopra soglia contribuiscono a `_qa_calo`/`_qb_calo`
- Denominatore = n. eventi calo-validi con segnale nel canale (no A&B obbligatoria)

#### Metodo 2 — Gate fisso
- Integra **sempre** in finestra fissa `[GATE_MIN_A_NS, GATE_MAX_A_NS]` = [115, 140] ns (DATA) o `[MC_GATE_MIN_A_NS, MC_GATE_MAX_A_NS]` = [150, 190] ns (MC)
- Tutti gli eventi calo-validi contribuiscono a `_qa_fix`/`_qb_fix` (anche quelli senza segnale → coda al piedistallo)
- Denominatore = tutti i calo-validi → istogramma completo segnale+rumore
- Parametri gate fisso: `GATE_MIN/MAX_A/B_NS`, `GATE_PED_START/END_NS` = [250, 260] ns
- Parametri MC: `MC_GATE_MIN/MAX_A/B_NS`, `MC_GATE_PED_START/END_NS`

#### Correzione CM no-calo (`GATE_NOCALO_CM_CORR`, attualmente `False`)
Quando attivo: la media del gate fisso sugli eventi **senza** segnale calo (`_qa_nocalo_fix`) viene sottratta da tutte le distribuzioni del gate fisso (calo-validi e no-calo stessi). Serve a rimuovere l'offset sistematico da modo comune nel gate. La distribuzione no-calo corretta viene poi usata come piedistallo per la soglia del gate fisso al posto di `_pa_fix`.

### Piedistallo per la soglia di efficienza

Entrambi i metodi usano lo **stesso** piedistallo: integrale trapezoidale diretto `np_trapz` su `[GATE_PED_START_NS, GATE_PED_END_NS]` = [250, 260] ns (10 ns, regione post-segnale ben separata dall'impulso).
- Per il gate **dinamico**: `_pa_calo` = stesso `np_trapz` su `[_pp0:_pp1]` (campioni corrispondenti a 250–260 ns)
- Per il gate **fisso**: `_pa_fix` = stesso calcolo; se `GATE_NOCALO_CM_CORR=True` e DATA, viene sostituito dalla distribuzione no-calo
- Soglia: `Q > μ_ped + Q_PED_NSIGMA·σ_ped` (attualmente `Q_PED_NSIGMA=2.0`) da fit gaussiano
- `PED_WINDOW_NS=20` ns è usato solo per il ped del gate **dinamico** tramite `DEBUG['pmt_ped_win_ns']` (ped gate reale = [250, 270] ns)

### Equalizzazione guadagno PMT (`Q_EQ_FACTOR_A/B`)

`ln(Q_A/Q_B)` vs X è una retta la cui **intercetta a X=0** è lo sbilanciamento di guadagno: `ln(Q_A/Q_B)|X=0 = ln(g_A/g_B)`. Per equalizzare i due PMT:

- `Q_EQ_FACTOR_A`/`Q_EQ_FACTOR_B` scalano l'intero segnale di carica (`sigB_q *= Q_EQ_FACTOR_B`). Scalando segnale+rumore+ped insieme, **l'efficienza in carica resta invariata** (soglia μ+Nσ scala coerentemente); cambia solo `ln(Q_A/Q_B)`.
- Valore corrente: `Q_EQ_FACTOR_B = 0.66` (calibrato da `ln(QA/QB)|X=0 ≈ −0.41 → exp = 0.66`)
- **Procedura**: lancia con 1.0 → leggi stampa `[EQ] … Q_EQ_FACTOR_B suggerito` → imposta → rilancia. Converge in 1–2 iterazioni.

### Catena di selezione eventi

1. **Trigger**: primo fronte di discesa sotto `TRIGGER_THR` (`crossing_time`)
2. **Calo valido**: `min(calo) ≤ CALO_THR` AND `CALO_Q_MIN < ∫calo < CALO_Q_MAX` sulla finestra `calo_sig_win_ns` → eventi a 1 elettrone
3. **Efficienza-mV (timing)**: `leading_edge_crossing(sigA, SIG_THR_A)` nella ROI `pmt_sig_win_ns`; A&B richiesta solo per `t_res`/`t_len` (CFD)
4. **Efficienza in carica gate dinamico**: `Q > μ_ped + Q_PED_NSIGMA·σ_ped` per canale (ped da `_pa_calo`)
5. **Efficienza in carica gate fisso**: stessa soglia ma ped da `_pa_fix` (o da no-calo se `GATE_NOCALO_CM_CORR`)

**Efficienza-mV** = `n_any / n_calo_valid`. **Efficienza in carica** = `n_AB_q / n_calo_valid` (A∪B, gate dinamico) o `n_AB_qfix / n_calo_valid` (gate fisso).

### Logbook e convenzioni coordinate

- Le coordinate X, Y nel logbook (spreadsheet) sono in **mm** → convertite in **cm** al caricamento (`÷ 10`)
- Le righe con `BAD` nella seconda colonna del logbook vengono escluse automaticamente
- `LEFT` nel riepilogo efficienza = X ≤ 0; `RIGHT` = X > 0
- Plot efficienza: asse Y in **%** (0–110%), linea tratteggiata al 99%

### Output plot principali

| File PDF | Contenuto |
|----------|-----------|
| `Average_Physical_Variables_vs_X.pdf` | 2 pannelli: (t_A−t_B)/2 e ln(Q_A/Q_B) gate dinamico vs X |
| `Charge_LogRatio_FIXEDGATE_vs_X.pdf` | ln(Q_A/Q_B) metodo gate fisso vs X |
| `Charge_Efficiency_<cfg>.pdf` | Efficienza in carica gate dinamico: Totale A∪B, PMT A, PMT B vs X e vs Y |
| `Charge_Efficiency_FIXEDGATE_<cfg>.pdf` | Efficienza in carica gate fisso: Totale A∪B, PMT A, PMT B vs X e vs Y |
| `Total_Average_Efficiency_vs_Y.pdf` | Efficienza-mV totale vs Y [cm] |
| `PMT_Efficiency_vs_X_<cfg>.pdf` | Efficienza-mV vs X (Tot/A/B), una curva per ogni Y |
| `Efficiency_2D_Map_<cfg>.pdf` | Mappa 2D efficienza-mV su griglia X×Y |
| `Efficiency_vs_Angle_Mod<id>.pdf` | Efficienza vs angolo fascio |
| `Efficiency_vs_TrackLength_Mod<id>.pdf` | Efficienza vs lunghezza traccia in acqua |
| `AvgWF_NoCalo_DATA.pdf` / `AvgWF_NoCalo_MC.pdf` | WF media eventi senza segnale calo (diagnostica rumore/CM) |

**Plot di debug** (in `output/debug/`, attivati dal dict `DEBUG`):
- `charge_dist`: pannelli QA e QB sovrapposti — gate variabile (riempito) + gate fisso (step nero/arancione) + ped (viola) + soglie. Legenda include n. entries per ogni distribuzione.
- `nocalo_q_dist`: istogramma Q gate fisso no-calo (grigio) vs calo-validi, attivato da `DEBUG['plot_nocalo_q_dist']`
- `time_dist`: solo `(t_A−t_B)/2`
- WF per-evento: `waveforms`, `qab_event`, `lowq_fix_event`, `nocalo_event`, `nocalo_q_event`
