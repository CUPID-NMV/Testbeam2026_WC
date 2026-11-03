# WC Testbeam 2026 — Simulazione Geant4 (progetto unificato)

## Cos'è questo progetto

Simulazione Geant4 multi-thread di un rivelatore **Water Cherenkov (WC)** per il testbeam 2026 e gli studi di fondo a LNGS.
Supporta due geometrie e quattro tipi di sorgente selezionabili a runtime senza ricompilazione.

**A cosa serve.** Il WC sara' parte del **muon veto di CUPID** ai LNGS
(bolometri, 0nbb in 100Mo). La cosa da ottimizzare **non e' il rapporto
eps_mu/eps_gamma**: e' la massima efficienza sui muoni sotto il vincolo del
tempo morto, che vale R_fake x tau_veto. Confonderle porta a scartare
configurazioni ottime.

Configurazione di riferimento decisa a luglio 2026: geometria `single_wbc2` e
PMT **H10721-210** (UBA, HV integrato). L'H14603-103 e' fuori produzione; il
SiPM S14160-6075HS e' stato scartato perche' a parita' di eps_mu fa passare 15
volte i gamma, fra dark count, crosstalk e PDE.

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

### WC sopra i tre pannelli del LNGS (`single_pan3`, ramo `pannelli`, 2026-10-30)
- La `single` sopra **tre pannelli di scintillatore identici** (`ScintPanel`,
  porting del MC della collega di Yale, layout **Nested**, wrapping Tyvek):
  EJ-200 1250 x 550 x 50 mm, 4 fibre WLS a U nei groove, housing di
  alluminio 1500 x 609 x 63.5 mm piu' la scatola del SiPM che sporge a -x
  (ingombro 1643 mm), un SiPM S13360-6075PE per pannello.
- **Housing a contatto** (passo 63.5 mm), **SiPM tutti a -x**. Il WC resta
  nell'origine. Posizione misurata sul montaggio, dai bordi esterni:
  - il lato corto del serbatoio a +x sta **130 mm dentro** il lato corto
    dell'housing senza SiPM;
  - il lato lungo a -y sta **100 mm dentro** il bordo destro dell'housing,
    destro guardando i pannelli dal lato dei SiPM;
  - il fondo del serbatoio sta **10 mm sopra** l'alluminio del pannello alto.

  Nel sistema del WC: housing x -1023..+620, y -290..+319, z -260.5..-70
  mm; scintillatori centrati in x = -31.9, y = +15.7 mm, a z = -104.4,
  -167.9, -231.4 mm. Verificato con geantini, 53 controlli di
  sovrapposizione OK.
- **`/wc/panel/optics false`** (default, prima di `/run/initialize`): solo
  energia depositata e percorso nello scintillatore (`Edep_panel_*`, `TrackL_panel_*`), ~2 s per muone
  verticale. **`true`**: scintillazione (10000 fotoni/MeV), WLS e SiPM
  (`Hits_SiPM_*_Total/_Detected`, righe di "Fotoni" con PMT_ID 6-8),
  ~65 s per muone.
- Prima prova, 5 mu- verticali da 3 GeV con l'ottica accesa: **~21.5 PE/MeV,
  ~200 PE per muone** (~9.5 MeV), uguale nei tre pannelli. Va confrontato
  coi numeri della collega.
- Gamma di Hall A: `gun_gammas_background_righe_pannelli.mac`, scatola 2000
  x 1000 x 560 mm centrata in (-200, 15, -100) mm, 73600 cm^2. Rate:
  J x A = 0.28 x 73600 = **20608 Hz**.
- La vecchia `single_pan` (tre scatole di EJ-200 senza fibre, dimensioni
  sbagliate) e' **tolta** in questo ramo, per decisione dell'utente
  (2026-10-30); su `main` resta nel commit 0246573.

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

## Lavoro in corso: i pannelli di Yale nel MC (dal 2026-10-30)

Questa sezione e' scritta per chi riprende il lavoro in una chat nuova. Il
resto della storia (analisi del fondo LNGS, DAQ, scan) sta in
`~/daq-standalone/CLAUDE.md` e si fa in un'altra chat: qui si fa solo
l'integrazione dei pannelli nel MC.

### Cosa si vuole, e chi fa cosa

- **Obiettivo finale dell'utente**: veto dei muoni integrato **WC + pannelli
  scintillatori**. Al LNGS il WC sta sopra tre pannelli EJ-200 impilati, letti
  da un'estremita' coi SiPM.
- **I pannelli li studia una collega di Yale**, anche in simulazione. Deciso
  con lei (2026-10-30): **si uniscono i due MC**. Lei passa il suo
  DetectorConstruction del pannello singolo con fibre e routing corretti, noi
  lo inseriamo in questo progetto con la geometria vera del montaggio, poi
  **la produzione la lancia lei su un cluster di Yale**.
- **Il trigger da simulare** (dopo, nella digitalizzazione): self-trigger su
  tutti e 5 i readout, 3 SiPM + 2 PMT, su UN solo V1742 a **750 MS/s** (la
  frequenza di campionamento e' una per scheda). I segnali dei PMT sono
  **invertiti** con un inverter dal 2026-10-30, quindi tutti positivi.

### Regole per questo lavoro

- **Nessuna modifica sostanziale senza un ok esplicito dell'utente.** Prima
  si propone il piano (cosa si tocca, perche'), poi si implementa. I pannelli
  in `single_pan` sono stati aggiunti senza chiedere e l'utente l'ha detto
  chiaramente: non ripetere.
- **Non riprogettare i pannelli**: sono il lavoro della collega. Si integra
  il suo codice cosi' com'e'; le domande per lei passano dall'utente.
- **Il lavoro si fa nel ramo `pannelli`**, worktree
  `~/Testbeam2026_WC_pannelli` (build in `build-arm64/` li'), perche' `main`
  in `~/Testbeam2026_WC_unified` resta in uso cosi' com'e'. La variante
  veloce senza ottica ora e' `single_pan3` con `/wc/panel/optics false`.
- Lo stato di git e' **sporco di lavoro buono**: `FiberScintillation`, il fix
  delle WLS, le colonne delle fibre e dei pannelli, `Run_gamma_parallel.sh`,
  le macro `gun_gammas_pmt_*.mac` (sorgenti nei moduli dei PMT, generate da
  `daq-standalone/analisi/20261029_scint_fibre/macro_sorgente_pmt.py`). Non
  e' committato; **non va annullato**. La build usata e' `build-arm64/`.

### Il codice della collega e cosa ne e' entrato (2026-10-30)

Da `~/Downloads/src_YALE.zip` e `include_YALE.zip` (i due file sciolti
`*_YALE.cc/.hh` sono le stesse versioni, l'header piu' vecchio). In
`include/ScintPanel.hh` + `src/ScintPanel.cc`:

- **Entrato cosi' com'e'**: `DetectorConstructionCAD.cc` (geometria dal CAD,
  layout `Nested`, controllo delle distanze fra le fibre: 4 um, 0
  sovrapposizioni), le proprieta' ottiche del suo `DefineMaterials()` e
  `GetOrBuildMaterials()`, il wrapping `Tyvek` (il suo default), la PDE del
  SiPM come EFFICIENCY della superficie.
- **Cambiato per l'integrazione**: tutto dentro un involucro d'aria grande
  quanto l'housing, piazzato tre volte, invece che nel mondo; materiali propri
  `Pan_*`; ottica solo a richiesta; una superficie nera fra involucro e
  mondo, perche' un fotone uscito dal pannello non arrivi ai PMT del WC.
- **Non portato**: layout `Standard`, geometrie `*_old` e spirale, wrapping
  `Mylar`, il suo `DetectorMessenger` e `GeometryDump`.
- L'header nello zip dichiara una modalita' `Nested3` (`nPanels`) che il .cc
  non implementa: non serve, l'impilamento e' in `SingleModuleDetector`.
- **Differenze rimaste fra i due MC**, da tenere presenti nel confronto: la
  sua physics list e' `G4EmStandardPhysics_option4` + decadimenti radioattivi
  + `QGSP_BERT_HP`, qui resta quella del WC (`G4EmStandardPhysics` +
  `G4EmExtraPhysics`); i suoi tempi partono dal primo deposito nello
  scintillatore, qui sono globali (riferimento comune a WC e pannelli); il suo
  `Edep` comprende anche le fibre, qui solo l'EJ-200. Nessuno dei due
  ha la costante di Birks per l'EJ-200; `SCINTILLATIONRISETIME1` non ha
  effetto senza `SetScintFiniteRiseTime(true)`, ne' da lei ne' qui.
- **Le fibre dei pannelli sono BCF-91AXL** (l'utente, 2026-11-02), ma le
  tabelle della collega sono quelle di una BCF-92, su 8 punti fra 2.0 e
  3.5 eV. Da chiarire con lei, senza toccare `ScintPanel.cc`:
  - decadimento del WLS 2.7 ns (BCF-92) invece di 12 ns: i PE non cambiano,
    la forma degli impulsi dei SiPM si';
  - attenuazione: `ABSLENGTH` 3.5 m, piu' `WLSABSLENGTH` 3.5 m nel verde e
    una rampa lineare fino a 0.1 mm fra 496 e 459 nm, dove cade il 45%
    dell'emissione. La luce del WLS vede ~1.5 m invece dei >4 m del
    datasheet, e ai SiPM arriva circa la meta' della luce (stima a mano, non
    simulata). E' lo stesso difetto che avevano le fibre del WC;
  - emissione di 5 punti, col 7% fra 428 e 459 nm, dove la BCF-91AXL non
    emette e la fibra la riassorbe subito; assorbimento piatto sotto 459 nm
    (la BCF-91AXL ha tre picchi, a 400, 422 e 447 nm), ma il bordo a ~460 nm
    e' giusto.

### Scelte fatte nell'integrazione

- **Formato unito** (2026-10-30, richiesta dell'utente): le ntuple sono
  quelle del WC piu' le grandezze del MC della collega, una copia per
  pannello; vedi "Output ROOT". Le colonne dei PMT `Mid`/`Top` di `triple`
  sono tolte: i PMT 2-5 di `triple` restano solo in `Fotoni`.
- **SiPM contati come li conta la collega** (SteppingAction, niente SD): un
  fotone e' *arrivato* quando il suo passo finisce sul SiPM, dopo grasso e
  finestra; e' *rivelato* se il processo di bordo da' `Detection` (la PDE e'
  l'EFFICIENCY della superficie). Verificato: i rivelati coincidono evento
  per evento con i PE contati prima con lo SD.
- **PMT del WC**: contano i fotoni che entrano nel vetro, con cui le fibre
  sono in contatto diretto (n = 1.50, accoppiamento ideale, equivalente a un
  grasso). La QE la applica **solo il digitizer** (`Signal_maker`, PMT di
  riferimento H14603-103, correzione dell'utente del 2026-10-30): in
  `Fotoni` i PMT hanno `Detected = -1`.
- `T_FirstHit_ns` e' il primo fotone sui PMT del WC; `T_FirstHit_SiPM_*` il
  primo fotone **rivelato** di ogni SiPM. Tutti i tempi sono globali.
- Il Cherenkov nato nei pannelli (solo con l'ottica accesa) **non** entra in
  `N_Cerenkov_prod` e nelle `N_Cer_*`.
- `FiberScintillation` **non** e' applicata alle fibre dei pannelli: scelta
  da discutere, non da fare per default.
- Il MC del SiPM conta fotoni: **dark count e crosstalk** vanno messi nella
  digitalizzazione. A 54 V sono misurati (DCR ~1.4 MHz a 0.5 p.e., crosstalk
  0.18-0.26: `daq-standalone/CLAUDE.md`).

### Misure del montaggio in caverna (2026-10-30)

- Presa: posizione del WC sui pannelli e distanze verticali (sopra, nella
  sezione `single_pan3`); SiPM tutti dallo stesso lato; layout Nested.
- **Ancora da prendere**: da che parte stanno i SiPM rispetto a PMT20 e
  PMT21 (nel MC i SiPM sono a -x, il lato del PMT L); quale canale legge
  quale pannello (ch16, ch17, ch18 -> alto, mezzo, basso).

### Ordine di lavoro (da concordare con l'utente)

1. ~~Integrazione del pannello e geometria del montaggio~~ (fatto,
   2026-10-30, ramo `pannelli`, non ancora committato).
2. **Confronto con la collega**: i suoi PE per MIP col pannello da solo,
   contro i ~200 PE per muone verticale di qui.
3. Prove brevi con i gamma, poi la produzione passa a lei.
4. Dopo, e solo dopo averne parlato: il self-trigger sui 5 readout nella
   digitalizzazione (`daq-standalone/analisi/20261030_digit_mc/digit_mc.py`),
   con le soglie dallo scan per canale del 2026-10-30
   (`daq-standalone/plots/noise_scan_20261030_*_chNN.json` sul PC DAQ; a
   750 MS/s l'offset non e' ancora calibrato in mV).

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
| Scintillation | ✅ solo nel nucleo delle fibre (dal 2026-10-29) |

**Scintillazione del nucleo delle fibre** (`include/FiberScintillation.hh`).
Il nucleo della BCF-9995XL e' polistirene, che scintilla da solo (~1750
fotoni/MeV a 324 nm, tempi 2.26 / 11.8 / 24.8 ns con pesi 60 / 23 / 17%,
arXiv 2605.12861), e il colorante della fibra assorbe proprio li': la fibra e'
una fibra scintillante debole (su Kuraray Y11 misurato 23% di una BCF-12,
arXiv 2604.14060). Scoperta sui dati del prototipo al LNGS: gli impulsi ad
alta carica hanno la coda lenta di questa luce (daq-standalone,
`analisi/20261029_scint_fibre`). Si regola da macro **prima di
`/run/initialize`**: `/wc/fiber/scintYield 1750` (fotoni/MeV; **0 = spenta,
come fino al 2026-10-29**) e `/wc/fiber/birks 0.126` (mm/MeV). Prova del
2026-10-29, 1 M gamma di Hall A (righe): la classe ad alta carica passa da 0
a ~0.03 Hz, contro ~0.30 Hz misurati.

**Fibra, assorbimento ed emissione del WLS** (2026-10-31): `WLSABSLENGTH` e
`WLSCOMPONENT` vengono dal grafico del datasheet Luxium della BCF-9995XL
(brochure 2023, pag. 4), digitalizzato a passo 5 nm, in `src/FiberWLS.cc` per
tutte e 5 le geometrie del WC (prima ognuna aveva la sua copia delle tabelle).
- Emissione da 385 a 580 nm. Prima erano 4 punti e lo spettro ai PMT veniva a
  gradini. Il grafico e' per nm e Geant4 campiona in energia: i valori sono
  moltiplicati per lambda^2. Il picco del grafico e' a 435 nm, ma la tabella
  della brochure dice 450.
- Assorbimento da 280 a 400 nm, con la forma del datasheet e la lunghezza al
  picco di prima, 0.01 mm (il datasheet da' solo la forma). Prima finiva a
  ~387 nm, e fra 400 e 500 nm restavano 2-10 m che il datasheet non ha: la
  luce del WLS veniva riassorbita lungo la fibra. Sopra 400 nm deve restare
  nullo: con Geant4 11.4.2 un fotone assorbito sotto il minimo dello spettro
  di emissione ferma il programma (G4OpWLS, WSL01; successo il 2026-10-29).
- `ABSLENGTH` del nucleo: 4 m, il limite del datasheet ("oltre 4 m", fibra
  da 1 mm, fotodiodo al silicio). Fino al 2026-10-31 era 2.5 m.
- Prova con 2000 muoni di Hall A su `single_pan3`, contro le tabelle vecchie:
  fotoni ai PMT +34%, QE efficace dell'H14603-103 da 0.245 a 0.259,
  fotoelettroni +41 +- 3% (emissione 6%, assorbimento 21%, attenuazione 10%).
  Il 5% dei fotoni ai PMT non passa dal WLS: e' luce Cherenkov portata dalla
  fibra, misurata con `/process/inactivate OpWLS`, e sono tutti quelli sopra
  580 nm. Tolta quella, lo spettro ai PMT coincide col datasheet fra 405 e
  575 nm (chi2/ndf = 22.7/16).
- **Due scelte che il datasheet non decide, tenute come sono** (decisione
  dell'utente, 2026-11-01): la lunghezza d'assorbimento al picco, 0.01 mm
  (con 0.1 mm i fotoelettroni calano del 19%), e il picco di emissione a
  435 nm del grafico (a 450 nm, come nella tabella della brochure, calano del
  10%). Le alternative danno tutte e due meno luce: i fotoelettroni simulati
  sono il valore alto, e con tutte e due insieme sarebbero circa il 27% in
  meno (prodotto dei due effetti, non simulato). Si cambiano solo con un dato
  nuovo (Luxium, o una misura).
- Tempo di decadimento del WLS: 2.41 +- 0.03 ns, misurato da Yamazumi et al.
  (PTEP 2026, arXiv 2601.06784), dal 2026-11-01; prima i 2.7 ns del
  datasheet. E' anche lui in `src/FiberWLS.cc`.

**Compilazione su Apple Silicon** (2026-10-29): `build-arm64/`, Geant4 11.4.2
dell'ambiente `geant4_env` (servono anche i pacchetti `expat` e `zlib` per gli
header). `build/` e' il vecchio binario Intel di luglio (Geant4 11.3.2), che
non parte piu'. Produzione a piu' processi con semi diversi:
`src/Run_gamma_parallel.sh <geo> <macro> <tag> <eventi> [processi] [fotoni/MeV]`
(il programma in batch e' sequenziale). Spettro dei gamma a righe (CUPID) in
`gun_gammas_background_righe.mac`, limite superiore; quello NaI di
`gun_gammas_background.mac` e' il limite inferiore.

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

**Ntuple 0 — "Eventi"** (una riga per evento = un primario, 46 colonne).
Formato unito con il MC dei pannelli della collega (2026-10-30); i notebook
leggono le colonne per nome.

| col | nome | unità | note |
|-----|------|-------|------|
| 0 | EventID | — | stesso EventID di "Fotoni" |
| 1 | SourceType | — | 0=e− (testbeam), 1=γ (fondo), 2=μ− (cosmici/testbeam) |
| 2 | E_gen_MeV | MeV | energia cinetica del primario al vertice |
| 3-5 | Vtx_X/Y/Z_mm | mm | vertice del primario |
| 6-8 | Dir_X/Y/Z | — | direzione del primario |
| 9 | E_init_MeV | MeV | energia cinetica del primario all'entrata in acqua |
| 10 | TrackL_water_mm | mm | lunghezza traccia primaria in acqua |
| 11 | Edep_water_MeV | MeV | energia depositata in acqua dal solo primario (0 per gamma) |
| 12 | N_Cerenkov_prod | — | fotoni Cherenkov prodotti nel WC (non nei pannelli) |
| 13 | N_Cerenkov_entrati | — | fotoni Cherenkov nati nell'acqua ed entrati in una fibra |
| 14 | N_Cer_primary | — | Cherenkov da particella primaria diretta |
| 15 | N_Cer_compton | — | Cherenkov da elettroni Compton o fotoelettrici |
| 16 | N_Cer_pair | — | Cherenkov da coppie e+/e− (pair production) |
| 17 | N_Cer_deltaEM | — | Cherenkov da delta-ray (ionizzazione EM secondaria) |
| 18 | N_Cer_nuclear | — | Cherenkov da prodotti di interazioni nucleari |
| 19 | Edep_fiber_L_MeV | MeV | energia nel nucleo delle fibre del WC verso il PMT L (x < -pettine) |
| 20 | Edep_fiber_C_MeV | MeV | energia nel nucleo delle fibre del WC, tratto dritto |
| 21 | Edep_fiber_R_MeV | MeV | energia nel nucleo delle fibre del WC verso il PMT R (x > +pettine) |
| 22 | Hits_PMT_L | — | fotoni entrati nel vetro del PMT L, prima della QE (prima `Hits_L_Bot`) |
| 23 | Hits_PMT_R | — | idem, PMT R (prima `Hits_R_Bot`) |
| 24 | T_FirstHit_ns | ns | primo fotone su un PMT del WC (−1 se nessuno) |
| 25-27 | Edep_panel_{top,mid,bot}_MeV | MeV | energia nell'EJ-200 di ogni pannello, tutte le particelle (solo `single_pan3`, se no 0) |
| 28-30 | TrackL_panel_{top,mid,bot}_mm | mm | percorso del primario nell'EJ-200 |
| 31-33 | N_Scint_{top,mid,bot} | — | fotoni di scintillazione prodotti nell'EJ-200 (solo con `/wc/panel/optics true`, se no 0) |
| 34-36 | PhotonsInFiber_{top,mid,bot} | — | fotoni entrati nel nucleo di una fibra del pannello (una volta per fotone) |
| 37-39 | Hits_SiPM_{top,mid,bot}_Total | — | fotoni arrivati al SiPM, dopo grasso e finestra |
| 40-42 | Hits_SiPM_{top,mid,bot}_Detected | — | di cui rivelati (PDE): fotoelettroni |
| 43-45 | T_FirstHit_SiPM_{top,mid,bot}_ns | ns | primo fotone rivelato del SiPM (−1 se nessuno) |

`top` = il pannello piu' vicino al serbatoio.

**Ntuple 1 — "Fotoni"** (una riga per fotone arrivato a un fotorivelatore):
| col | nome | unità | note |
|-----|------|-------|------|
| 0 | EventID | — | |
| 1 | TrackID | — | |
| 2 | E_Hit_eV | eV | lambda[nm] = 1239.84 / E |
| 3 | Arrival_Time_ns | ns | tempo globale dall'inizio dell'evento |
| 4 | PMT_ID | — | 0 = PMT L, 1 = PMT R (2-5: altri PMT di `triple`); 6/7/8 = SiPM top/mid/bot |
| 5 | Detected | — | SiPM: 1 rivelato, 0 arrivato ma non rivelato (PDE); PMT: −1 (la QE la applica il digitizer) |

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

# GUI dopo una macro propria, al posto di init_vis.mac: per i comandi da dare
# prima di /run/initialize. WC + pannelli con l'ottica accesa, poi /run/beamOn 1
# (~1 minuto per un muone; si disegna un fotone ottico ogni 1000,
# /wc/vis/photonFraction: tutti sono ~10^5 e pesano ~5 GB):
./WCtestbeamSim single_pan3 vis_pannelli.mac gui
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
| `src/ScintPanel.cc` | un pannello del LNGS (porting dalla collega di Yale), piazzato tre volte da `SingleModuleDetector` in `single_pan3`; `/wc/panel/optics` |
| `src/TrackingAction.cc` | solo visualizzazione: salva la traiettoria di una frazione dei fotoni ottici (`/wc/vis/photonFraction`, default 1 = tutti; scelta dal numero di traccia, non cambia la fisica) |
| `src/PrimaryGeneratorAction.cc` | wrapper G4GeneralParticleSource, imposta SourceType in EventAction |
| `src/EventAction.cc` | accumula contatori per evento, riempie ntuple 0 (46 colonne) |
| `src/PMTSD.cc` | sensitive detector PMT unificato (mappa SD name → PMT ID 0–5) |
| `src/RunAction.cc` | apre/chiude file ROOT, definisce struttura ntuple (46 + 6 colonne) |
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
- **Edep_water_MeV** (col 11) è sempre ~0 per eventi gamma: il primario γ non ionizza direttamente, i secondari (Compton, pair) depositano energia ma vengono tracciati separatamente.
- **N_Cer_primary…N_Cer_nuclear** (col 14–18): somma ≤ N_Cerenkov_prod (alcuni fotoni Cherenkov vengono da processi EM non categorizzati come eBrem).
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
| `RUN_SEL` | `None` | selezione run: `None`=tutti / `1339`=singolo / `(1339,1350)`=range / `(1339,None)`=da run N in poi |
| `MODULE_SEL` | `0` | selezione modulo: `None`=tutti / `0`=singolo / `[0,2]`=lista moduli |
| `ANALYSIS_MODE` | `"BOTH"` | `"DATA"` / `"MC"` / `"BOTH"` — sorgente dati |
| `CALO_THR` | `-40` | soglia ampiezza minima calo (ADC) |
| `CALO_Q_MIN` | `-35500` | carica minima calo (ADC·sample) |
| `CALO_Q_MAX` | `-10500` | carica massima calo (ADC·sample) |
| `PROCESS_ALL_SUBEVENTS` | `True` | vedi sezione "Sotto-eventi multipli per blocco MIDAS" sotto |
| `APPLY_PADME_CORRECTION` | `False` | vedi sezione "Correzione piedistallo per-capacitor (PADME)" sotto (default di `wclib.py` resta `True`; nel notebook al momento è spenta) |
| `SUBTRACT_PED_FIXED_GATE` | `True` (prova in corso) | sottrae il picco del piedistallo (fit gaussiano) dalla carica del gate fisso A/B — vedi "Offset di ln(Q_A/Q_B) col gate fisso" |
| `PMT_BL_LEFT_NS` | `(0.0, 100.0)` | regione sinistra per baseline PMT (ns assoluti) |
| `PMT_BL_RIGHT_NS` | `(300.0, 400.0)` | regione destra per baseline PMT (ns assoluti) |
| `CALO_BL_LEFT_NS` | `(0.0, 100.0)` | regione sinistra per baseline Calo (ns assoluti) |
| `CALO_BL_RIGHT_NS` | `(200.0, 400.0)` | regione destra per baseline Calo (ns assoluti) |

**Dizionario `DEBUG`** (finestre di analisi, tutte in ns assoluti dall'inizio della traccia):

| Chiave | Valore attuale | Uso |
|--------|---------------|-----|
| `pmt_sig_win_ns` | `(100.0, 240.0)` | ROI segnale PMT (selezione efficienza + aggancio gate Q) |
| `pmt_ped_win_ns` | `(250.0, 260.0)` | finestra piedistallo PMT per il gate dinamico (regione post-segnale) |
| `calo_sig_win_ns` | `(120.0, 250.0)` | finestra integrazione carica calo |
| `calo_ped_win_ns` | `(20.0, 80.0)` | finestra piedistallo calo per istogramma Q |
| `wf_xlim_pmt_ns` | `(50.0, 250.0)` | range asse X per i pannelli "PMT Signals" / "Segnali QA/QB" nei plot di debug |
| `event_index` | `None` | se impostato a un intero: il plot di debug (`plot_waveforms`) mostra **solo** l'evento con quell'indice assoluto (`n_gen`, 1-based dall'inizio del run, un incremento per sotto-evento), ignorando `max_events` — utile per puntare allo stesso evento fisico confrontato con `Analysis_script/SIC_RawView.ipynb` (stessa convenzione di conteggio) |

### Due linee di analisi COMPLETAMENTE indipendenti

Dopo la sottrazione baseline (preprocessing comune) l'analisi si separa in due linee che **non condividono nessun parametro**. Cambiare un parametro di una linea non tocca l'altra.

| | **LINEA TIMING** | **LINEA CARICA (Q)** |
|--|--|--|
| Segnale | `sigA`/`sigB` = `apply_pmt_filter(...)` | `sigA_q`/`sigB_q` = `apply_charge_filter(...)` |
| Flag filtri | `PMT_FILTER_SAVGOL`, `PMT_FILTER_LOWPASS` | `PMT_Q_FILTER_SAVGOL`, `PMT_Q_FILTER_LOWPASS` |
| Param SavGol | `PMT_SMOOTH_WINDOW`, `PMT_SMOOTH_POLYORDER` | `PMT_Q_SMOOTH_WINDOW`, `PMT_Q_SMOOTH_POLYORDER` |
| Param LP | `PMT_LP_ORDER`, `PMT_LP_CUTOFF` | `PMT_Q_LP_ORDER`, `PMT_Q_LP_CUTOFF` |
| Soglia | `SIG_THR_A/B` (da `get_calib`: `SIG_THR_A_DATA`=`SIG_THR_B_DATA`=−13 dati, `SIG_THR_MC`=−10 MC) | `Q_GATE_THR_A`, `Q_GATE_THR_B` (−2) |
| Aggancio fronte | `GATE_LEAD_NS` (60 ns) | `Q_GATE_LEAD_NS` (20 ns) |
| Altri | `CFD_DELAY_NS`, `CFD_SEARCH_WIN_NS`, `CFD_FRACTION` | `ADC_GATE_START_NS` (10 ns), `PULSE_WINDOW_NS` (100 ns), `PMT_Q_LOCAL_PED*`, `Q_PED_NSIGMA` (3.0) |
| Prodotti | `t_res=(t_A−t_B)/2`, efficienza-mV `n_A/n_B/n_any` | `Q_A/Q_B`, efficienza in carica `n_*_q` e `n_*_qfix`, `ln(Q_A/Q_B)` |

Il blocco parametri in cima alla cella è raggruppato con header: `# --- COMUNI/PREPROCESSING ---`, `# --- LINEA TIMING ---`, `# --- LINEA CARICA / Q ---`, `# --- CALORIMETRO ---`.

### Elaborazione del segnale

1. **Sottrazione baseline PMT** (comune): interpolazione lineare tra mediana regione sx (`PMT_BL_LEFT_NS`) e dx (`PMT_BL_RIGHT_NS`) → retta sottratta dall'intera traccia
2. **Modo comune per-canale** (opzionale): `CM_REF_CHANNEL_A/B` — sottrae la WF raw di un canale di riferimento dal canale PMT corrispondente (`None` = off). **Attualmente `None` (disattivato)**: dopo l'introduzione della correzione PADME per-capacitor (vedi sotto), sottrarre un canale di riferimento debolmente correlato *aumenta* il rumore invece di ridurlo (verificato su dati reali: +10-18% di std) — il piedistallo per-capacitor era la componente di rumore condivisa che il modo comune stava in parte intercettando.
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
- Integra **sempre** in finestra fissa `[GATE_MIN_A_NS, GATE_MAX_A_NS]` = [100, 200] ns (DATA, uguale per A e B) o `[MC_GATE_MIN_A_NS, MC_GATE_MAX_A_NS]` = [130, 200] ns (MC)
- Tutti gli eventi calo-validi contribuiscono a `_qa_fix`/`_qb_fix` (anche quelli senza segnale → coda al piedistallo)
- Denominatore = tutti i calo-validi → istogramma completo segnale+rumore
- Parametri gate fisso: `GATE_MIN/MAX_A/B_NS`, `GATE_PED_START/END_NS` = [250, 260] ns
- Parametri MC: `MC_GATE_MIN/MAX_A/B_NS`, `MC_GATE_PED_START/END_NS`

#### Correzione CM no-calo (`GATE_NOCALO_CM_CORR`, attualmente `False`)
Quando attivo: la media del gate fisso sugli eventi **senza** segnale calo (`_qa_nocalo_fix`) viene sottratta da tutte le distribuzioni del gate fisso (calo-validi e no-calo stessi). Serve a rimuovere l'offset sistematico da modo comune nel gate. La distribuzione no-calo corretta viene poi usata come piedistallo per la soglia del gate fisso al posto di `_pa_fix`.

#### Offset di ln(Q_A/Q_B) col gate fisso (studio 30/07/2026)

Sui DATI la retta `ln(Q_A/Q_B)` vs X del gate fisso non passa per zero, mentre col gate dinamico sì (e l'MC è quasi pulito). Cosa si è capito:
- `Q_EQ_FACTOR_B` è calibrato sul gate **dinamico**: non garantisce di azzerare anche l'intercetta del gate fisso.
- **Asimmetria temporale reale tra i canali**: `t_A − t_B` = +6.5 ns in media (mediana +4.8 ns, std 18.6 ns, 1461 eventi, run 1339); t_A ≈ 134 ns, t_B ≈ 127 ns. Il gate dinamico si aggancia al fronte di ciascun canale e non ne risente; una finestra fissa uguale per A e B può tagliare la coda di A. L'MC non modella ritardi relativi A/B, quindi non riproduce l'effetto.
- Le finestre allora asimmetriche ([110,150] A, [110,160] B) sono state portate a [100,200] ns per entrambi.
- **Sottrazione del piedistallo dal gate fisso** (`SUBTRACT_PED_FIXED_GATE`): testata, **aumenta** l'offset sui DATI invece di ridurlo → il piedistallo non è la causa dominante. Il flag resta per ulteriori prove.
- Plot di debug `DEBUG['plot_ped_fix_dist']` (default `False`): istogrammi dei piedistalli del gate fisso A e B con fit gaussiano.

### Piedistallo per la soglia di efficienza

Entrambi i metodi usano lo **stesso** piedistallo: integrale trapezoidale diretto `np_trapz` su `[GATE_PED_START_NS, GATE_PED_END_NS]` = [250, 260] ns (10 ns, regione post-segnale ben separata dall'impulso).
- Per il gate **dinamico**: `_pa_calo` = stesso `np_trapz` su `[_pp0:_pp1]` (campioni corrispondenti a 250–260 ns)
- Per il gate **fisso**: `_pa_fix` = stesso calcolo; se `GATE_NOCALO_CM_CORR=True` e DATA, viene sostituito dalla distribuzione no-calo
- Soglia: `Q > μ_ped + Q_PED_NSIGMA·σ_ped` (attualmente `Q_PED_NSIGMA=3.0`) da fit gaussiano
- `PED_WINDOW_NS=10` ns è la larghezza della finestra piedistallo del gate **dinamico** (indipendente da `PULSE_WINDOW_NS`)

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

### Correzione piedistallo per-capacitor (metodo PADME)

Il digitizer V1742/DRS4 ha un artefatto di piedistallo legato alla specifica **cella fisica**
del buffer circolare (1024 capacitor per canale), non un semplice offset DC: e' una struttura
riproducibile in funzione di `p = (t + SIC) mod 1024` (SIC = Start Index Cell, letto dall'header
`DGH0`, bank valido per schede `1742`). Verificato su dati reali (run 1339): fino a ~15 mV di
spread su canali guasti/saturi, ma presente anche sui canali PMT A/B sani a livello di ~1 mV.

**Implementata in `Analysis_script/wclib.py`** (trasparente, nessuna modifica a valle richiesta):
- `open_mid()` calibra automaticamente una tabella di piedistallo per-capacitor `E_p[canale][cella]`
  (mediana per cella sui primi `pedestal_calib_events` eventi del run, default 1000 — parametro
  `PEDESTAL_CALIB_EVENTS`) ogni volta che un run viene aperto. Parametri: `pedestal_correction=True`
  (default) e `pedestal_calib_events=1000`.
- `daq_dgz_full2array()` applica `S_cor(t) = S_meas(t) - E_p[(t+SIC) mod 1024]` usando la
  calibrazione corrente se disponibile; passare `pedestal_correction=False` per il dato grezzo.
- `correct_waveforms()` implementa la correzione (in precedenza chiamata ma mai definita in
  `wclib.py` — funzione morta, mai attivata).
- In `WC_analisys_tool.ipynb`: flag `APPLY_PADME_CORRECTION` (default `True`) attiva/disattiva
  la correzione passandolo a `wc.open_mid(..., pedestal_correction=APPLY_PADME_CORRECTION)`.

Validato con split calibrazione/validazione su eventi disgiunti (run 1339): riduzione std
per-capacitor **~4.3-4.4×** su ch A/B; nessun miglioramento reale sul canale trigger (segnale
vero, non piedistallo — atteso).

**Script diagnostico dedicato**: `Analysis_script/SIC_RawView.ipynb` — calibrazione/validazione
indipendente (non riusa lo stato di `wclib.py`, self-contained), vista eventi con overlay
SIC/posizione capacitor, targeting per indice assoluto evento (`EVENT_INDEX`, stessa convenzione
di `n_gen`/`DEBUG['event_index']` in `WC_analisys_tool.ipynb`) per confrontare lo stesso evento
fisico tra i due script.

Riferimento: Georgiev et al., *"Offline noise calibration of the CAEN V1742 ADCs at the PADME
experiment"*, J. Phys. Conf. Ser. 2255 (2022) 012008 — formula `p = (t + SIC) mod 1024`, metodo
piedistallo per-capacitor (qui con mediana anziche' moda, robusto a impulsi reali occasionali).

**Nota su `leading_edge_crossing`**: la funzione cerca il minimo globale nella ROI poi backtracka
fino a `lead_samples` prima di quel minimo per trovare l'inizio del "primo lobo" (utile per impulsi
strutturati/doppi). Se il vero inizio dell'impulso e' piu' lontano dal minimo globale di quanto
`lead_samples` permetta (es. `Q_GATE_LEAD_NS=10 ns` su un impulso a doppio lobo largo >10 ns), il
gate si aggancia a una crossing intermedia sbagliata, non al vero fronte — verificato su un caso
reale (run 1339). `GATE_LEAD_NS=60 ns` (linea timing) e' molto piu' permissivo e non ha lo stesso
problema sugli stessi eventi.

### Studi FFT in `SIC_RawView.ipynb` (31/07 – 03/08/2026)

Sezioni aggiunte in coda a `Analysis_script/SIC_RawView.ipynb` (run 1339, canali A=27/B=26, calo=16 con lo stesso criterio di `WC_analisys_tool.ipynb`). La FFT è sempre calcolata sulla **traccia intera** (escluse le ultime `N_DROP_LAST_FFT=20` celle); le finestre temporali servono solo a selezionare gli eventi.

**DATI — calo presente vs assente**
- Spettro medio |FFT| (finestre rettangolare e Hann): con calo c'è un eccesso a bassa frequenza (0–300 MHz, impulso largo decine di ns); le curve convergono verso ~300–400 MHz. Alcuni picchi a banda stretta compaiono sia con sia senza calo, anche senza PADME (la correzione li rende più visibili, non li crea); non sono simmetrici tra A e B → non è un clock comune della scheda. Non indagati oltre (non incidono sulle prestazioni). `SHOW_RAW_FFT=False` spegne il confronto senza PADME.
- **Potenza integrata 0–300 MHz come discriminante** (2716 eventi): AUC 0.87 (A) / 0.90 (B), ma al 99% di efficienza passa il 94–96% degli eventi senza calo (purezza ≈ 68%, pari a nessun taglio). Buona purezza (95–96%) solo accettando efficienza 76–82%. **Conclusione: non sostituisce la soglia in ampiezza**; utile al più come diagnostica o per scegliere il cutoff dei filtri passa-basso.
- **Pre-selezione in ampiezza** `AMP_THR_MV` in `AMP_WIN_NS=(100,200)` ns (su A o B): a −8 mV sopravvivono 1599 eventi, 1570 con calo e 29 senza (purezza 98.2%). Lo spettro dei 29 è indistinguibile da quello con calo e, all'ispezione visiva (cella dedicata), sono **impulsi reali** → inefficienza/soglia del calo, non falsi positivi del PMT. A −1 mV passa praticamente tutto (2715/2716). Valore attuale nel notebook: `AMP_THR_MV=-1.0`.
- Accumulo 2D (frequenza vs log|FFT|) di tutte le FFT individuali, oltre alla media, per vedere gli outlier.

**MC — γ (Compton) vs μ testbeam** (file `data/sim_single_gamma_X0mm_Y0mm_Th0deg.npz` e `sim_single_mu_tb_X0mm_Y0mm_Th0deg.npz` prodotti da `Signal_maker.ipynb`, canali `ch_0`/`ch_1`)
- Senza soglia gli spettri sono diversi; normalizzando al picco la differenza resta. Il dominio del tempo chiarisce: i μ hanno impulsi veri (larghezza locale ~1.6 ns), mentre l'86% dei γ con "picco" ≥12 ADC è un **singolo campione di rumore** (0.4 ns). Atteso da rumore puro (RMS 3.5 ADC): 26.6% degli eventi; osservato 41.9% → piccolo eccesso fisico, solo ~6% dei γ ha un impulso risolto su più campioni. **La differenza FFT γ/μ riflette soprattutto rumore vs impulso, non una diversa forma del Cherenkov.**
- Soglia `MC_AMP_THR_MV=-3` in `MC_AMP_WIN_NS=(100,200)` ns, con celle che mostrano le forme d'onda γ (zoom 100–300 ns) e μ (0–410 ns) sopravvissute.

### Sotto-eventi multipli per blocco MIDAS

Alcuni run (circa **913–1354/1376**, un terzo abbondante dei blocchi MIDAS in quel range,
verificato su campione distribuito 913-1554) impacchettano **piu' trigger fisici in un solo
blocco di readout** (`header.ntriggers[0] > 1`, fino a 12 sotto-eventi osservati). Run successivi
(≥ ~1376, verificato fino all'ultimo disponibile, 1554) sono praticamente puliti (`n_sub=1`) —
probabile cambio di configurazione DAQ.

Ogni sotto-evento ha il proprio SIC, TTT e forma d'onda completa su tutti i canali — dati
**buoni**, non corrotti (verificato: piedistallo/rumore/range trigger statisticamente identici
tra isub=0 e isub>0). Prima del fix, `get_events_generator` processava **solo il primo
sotto-evento (isub=0)** di ogni blocco, scartando silenziosamente in media **~26% degli eventi
reali** sui run affetti (verificato su run 1339: 2001 blocchi MIDAS, 2716 sotto-eventi reali).

Controllato con `PROCESS_ALL_SUBEVENTS` (default `True` = fix, tutti i sotto-eventi vengono
processati; `False` = comportamento storico/rollback, solo isub=0 — per confronto o se serve
riprodurre risultati precedenti al fix).

### Altri script di analisi

| file | ruolo |
|------|-------|
| `Analysis_script/Signal_maker.ipynb` | digitalizzazione MC: dai fotoni Geant4 ai segnali PMT (`.npz` in `data/`). PMT H14603-103, valori attuali `G=7.5e5`, `CABLE_LENGTH_M=6`, integrazione rapida 50–310 ns, `SRC_SEL="mu_tb"`. (valore di prova; working point testbeam G=6e5) |
| `Analysis_script/gain_scan_logQ_vs_X.ipynb` | scan del guadagno PMT in MC: stessa realizzazione fotonica per evento, si scala solo il guadagno e si ricalcola la carica (gate dinamico) → effetto del guadagno su ln(Q_A/Q_B) vs X |
| `Analysis_script/SIC_RawView.ipynb` | diagnostica PADME/SIC + studi FFT (vedi sopra) |
| `WCFastCheck.ipynb` (root) | scan HV dei pannelli di scintillatore EJ-200 (100×50×2.5 cm, distanza 125 cm): un pannello fa da trigger a 1100 V, l'altro in scan 800–1700 V; PMT sotto studio sempre su ch 0 |
| `RawView.ipynb`, `raw_channels_view.py` (root) | vista grezza dei primi 8 canali di file DAQ `.h5`/`.h5.gz` (nessun filtro né baseline) |
| `Analysis_script_background/view_cad_real.ipynb` | vista 3D del CAD reale del veto; usa **plotly**, da eseguire col kernel `jupyter-global` (`~/venvs/jupyter-global`, dove plotly è installato) |

I CSV in root (`anode_current_*`, `saturation_*`, `counts_*`, `signal_counts_*`, `charge_results_by_event.csv`, `mpv_landau_fit_results.csv`, `rate_curve_by_channel.csv`, 21–23/07/2026) sono output di questi studi, non versionati.

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

---

## Cose misurate e decise, da non ri-derivare

### Al testbeam a 45 gradi e' inclinato il MODULO, non il fascio

E' la cosa che si sbaglia per prima, e produce un disaccordo DATI/MC che non
esiste. Il modulo sta su uno stage movimentato e ruota di 45 gradi attorno a Y;
la **X del logbook e' la posizione dello stage**, con il fascio su modulo-(0,0)
a X=0.

- Per i **DATI** la lunghezza di traccia va calcolata con `L_moduletilt`
  (intersezione raggio-scatola nel frame del modulo, box acqua |X|<=480,
  |Y|<=180, |Z|<=50 mm).
- `_track_length_mm` modella il **fascio** inclinato ed e' corretta **solo per
  l'MC**, dove e' la simulazione a inclinare il gun.
- Nella cella principale `_L_geom(is_mc, ...)` smista: DATA -> module-tilt,
  MC -> beam-tilt.

Il modello vecchio teneva il bulk fino a X=380 mm e L=0 a 530, e cosi'
**contava i run di bordo come tracce piene**. Con la geometria giusta il bulk a
L=14.14 cm arriva solo fino a |X_stage| ~ 304 mm e L va a zero a ~375 mm
(foreshortening di cos45).

**I punti a L corta non sono luce-contro-L pulita**, e vanno guardati sapendolo:

- il **pettine delle fibre** sta a X_mod ~ 380 mm (stage ~ 270 mm), quindi i run
  a X_stage >~ 270 (1479-1486, 1453...) hanno il fascio quasi sulle fibre e
  carica ed efficienza gonfiate. Si usa la **mediana, non la media**;
- una **fibra rotta** sul lato -X (colonna X ~ -48 cm) deprime luce ed
  efficienza li';
- l'**offset dell'origine del prototipo** (~1-2 cm, `X_ORIGIN_OFFSET_MM`)
  **non si stima dai <Q> di bordo**: quei punti sono avvelenati dal pettine e
  non sono tracce in acqua. Una stima "~5 cm" ricavata da li' era circolare.
  Serve una misura metrologica, e l'incertezza >=1 cm va messa come barra
  orizzontale sui punti di bordo.

**Risultato:** con la geometria corretta l'apparente disaccordo DATI/MC contro
L **sparisce** -- tutti e due ~100% per L >~ 5 cm. I DATI non possono campionare
il turn-on (L < 5 cm) perche' traccia corta vuol dire bordo, cioe' pettine: il
turn-on si vede solo nell'MC.

### Le figure ufficiali del prototipo, e una da non rifare a mano

I risultati del prototipo al testbeam stanno in **`WC_TestBeam_DataAnalysis.pptx`**
(root, e' uno zip: le immagini sono in `ppt/media/`).

| | |
|---|---|
| `image3.png` | schema del modulo, vista dall'alto e di lato, fibre e PMT A/B |
| `image10.jpg` | mappa 2D di efficienza, 98-99% uniforme su tutta la superficie |
| `image12.png` | **Efficiency vs Track Length (DATA+MC)** |

Nella `image12` i **DATI sono ~99% a tutte le L misurate** (L >= 5 cm), e il
rolloff sotto i 4 cm e' **MC, non un degrado da fibra rotta**. Un Eff-vs-L
rifatto a mano con punti DATA al 76% e il commento "fibra rotta a 45 gradi" e'
gia' stato prodotto una volta ed era fuorviante. Per le slide si parte da
queste figure, non si rigenerano dagli output dell'analisi, che sono sparsi e
dipendono dalla configurazione.

Il prototipo dimostra efficienza muonica **98-99%** (A OR B, due PMT).

### Il veto reale a settori, e il tempo morto

Geometria vera dal CAD `cad/20260713-Assy-WC-Y.stp` (3646 solidi): **48 tank,
96 PMT**, box 3.78 x 3.87 x 1.64 m. I tank sono **gusci cavi**: il bounding box
e' l'inviluppo esterno e **l'acqua e' la cavita' interna** (meno 10 mm di
parete, come il prototipo). I **fori sono impliciti**: dove non c'e'
contenitore non c'e' acqua, quindi non serve nessun `holes.json`.

- `cad/water_sectors_real.csv` -- **84 settori d'acqua**, contenitori reali
  insettati e segmentati a <= 40 cm. E' la sorgente attiva del modello.
- `cad/newcad_mesh.npz` mesh dei contenitori, `cad/pmt_centers.npy` i 96 centri
  PMT. Visualizzatore: `Analysis_script_background/view_cad_real.ipynb`.
- La ricostruzione vecchia (STEP "No Holes", 74 settori piu' fori analitici) e'
  **superata** ma dava risultati coerenti.
- **Il reader CAF/XCAF crasha** (`Standard_NullObject`) su questa build di
  pythonocc: i nomi delle parti si leggono dal testo dello STEP
  (`grep PRODUCT`) e i solidi si filtrano per geometria -- contenitori con
  spessore 60-250 mm, PMT dalla firma 20x68x68.

**Risultati con la geometria reale:** eps_box (A+B) = 98.8% con WBC2 e 99.0%
con WBC3; R_mu ~ 390 muoni al giorno.

**Tempo morto** (cella in `Background_final.ipynb`): dt = (gamma fake + mu
veri) x tau, ed e' **dominato dai gamma fake** (~0.5 Hz tarati) -- la parte dei
muoni e' trascurabile. Viene ~0.05% a tau = 1 ms e ~2.5% a tau = 50 ms.

### La normalizzazione dei gamma, e il punto di lavoro che ne segue

> **Attenzione, 2026-10-29: la formula di questa sezione conta due volte il
> fattore 4.** Il "flusso" di Haffke et al. (arXiv:1101.5298, Tab. 1, Hall A
> 0.28 +- 0.02 cm^-2 s^-1) e' gia' J = phi/4, i gamma che ENTRANO per cm^2 di
> superficie: lo dicono le curve della Fig. 2 (conteggi/flusso = 247 cm^2, la
> superficie del NaI 3"x3"). Quindi phi = 1.12 e il rate giusto per la
> scatola del tank e' **J x A = 0.28 x 49400 = 13832 Hz**, 3.2 volte i 4.3 kHz
> scritti sotto. Conto e verifica in
> `daq-standalone/analisi/20261029_normalizzazione_gamma/` e in
> `daq-standalone/CLAUDE.md`, dove `digit_mc.py` e le sovrapposizioni sono gia'
> corretti. **Tempo morto e punto di lavoro qui sotto NON sono ancora stati
> rifatti** col rate giusto: vanno rifatti, non corretti a mano.

Il flusso integrale in Hall A sta **nello stesso articolo della forma
spettrale** (arXiv:1101.5298): phi = 0.3-0.4 cm^-2 s^-1 sotto 3 MeV, da cui
`RATE_GAMMA_HZ = (phi/4) * A_gen(4.94 m^2)` ~ **4.3 kHz**. Il segnaposto che si
usava prima era 100 Hz: **un fattore 43**.

La conseguenza e' operativa. Con tau_veto ~ 0.2 ms (ipotesi legata al sampling
dei light detector di CUPID; i colleghi CUPID, luglio 2026), la selezione lasca
**A unione B > 1PE** costa ~3% di esposizione. Il punto di lavoro consigliato
diventa **A intersezione B > 1PE**, che da' eps_box ~ 99.6% perdendo ~0.3%.

Per riferimento, CROSS (arXiv:2510.27406) usa +-1 ms fra bolometri e veto e
soffre ~18% di tempo morto, perche' i suoi scintillatori triggerano sui gamma
ambientali a ~100 Hz. La scalatura dal tank al box con A_box/A_tank resta
valida: il campo e' isotropo, quindi il rate va come l'area.
