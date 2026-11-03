# Produzione Hall A, WC + pannelli, 24 ore equivalenti: la fisica

Scritta il 2026-11-03. Le decisioni dell'utente sono nella sezione 6; le
modifiche al codice, fatte e da fare, nella sezione 8.

## Scopo

Simulare quello che il setup vede in Hall A in 24 ore di presa dati: gamma
ambientali e muoni cosmici, ciascuno col suo rate assoluto, cosi' che le
proporzioni fra le sorgenti siano quelle vere. L'uscita va poi digitalizzata
col self-trigger dei cinque readout (3 SiPM + 2 PMT, un V1742 a 750 MS/s) e
confrontata con la presa dati.

## Geometria

- `single_pan3`: la `single` sopra i tre pannelli, nelle posizioni misurate.
  Fibre del WC BCF-9995XL, dei pannelli BCF-91AXL, riemissione del WLS
  esponenziale.
- Ottica del WC sempre accesa. Pannelli con l'ottica spenta e la luce da una
  mappa (sezione 5).
- **Manca l'orientazione del setup in Hall A**: l'angolo fra l'asse lungo
  del WC e l'asse della sala, e da che parte stanno i SiPM. Serve per
  l'azimut dei muoni, che sotto il Gran Sasso non e' uniforme.

## 1. Gamma ambientali

E' la sorgente che domina per numero di eventi e per tempo di calcolo.

- **Flusso**: J = 0.28 +- 0.02 gamma entranti per cm^2 di superficie al
  secondo (Haffke et al., arXiv:1101.5298, Tab. 1, Hall A). E' J = phi/4,
  verificato in `daq-standalone/analisi/20261029_normalizzazione_gamma`.
- **Generazione**: per un campo isotropo, legge del coseno verso l'interno
  dalla superficie di una scatola convessa che contiene tutto il setup:
  2000 x 1000 x 560 mm centrata in (-200, 15, -100) mm, A = 73600 cm^2
  (`gun_gammas_background_righe_pannelli.mac`).
- **Rate**: R = J x A = **20608 Hz, cioe' 1.78 x 10^9 gamma in 24 ore**.
- **Spettro: le righe** (decisione dell'utente): 210 righe fra 0.019 e
  3.4 MeV (colleghi CUPID), senza il continuo dei gamma diffusi nella roccia.
  Il 29.9% sta sopra 0.42 MeV, la soglia del Cherenkov via Compton: e' il
  limite superiore per il WC. Lo spettro NaI (Haffke, Fig. 3, il 12.7% sopra
  0.42 MeV, limite inferiore) resta per un confronto.
- **Approssimazioni**: campo isotropo, con pavimento e pareti che pesano
  uguale; flusso misurato in un punto della sala.

## 2. Muoni cosmici

- **Flusso di MACRO** (decisione dell'utente: e' quello delle simulazioni
  ufficiali di CUPID). MUSUN e' stato scartato perche' difficile da avere:
  il codice si chiede all'autore, e la versione di LEGEND (`musun-gs`) sta
  in un container probabilmente riservato.
- **Normalizzazione**: 3.22 +- 0.08 x 10^-4 m^-2 s^-1 (MACRO, Hall B), per
  unita' di area perpendicolare alla direzione. Lo dice il confronto: LVD
  in Hall A da' 3.31, Borexino (sferico) in Hall C 3.43, entro il 6%; con
  definizioni diverse differirebbero del 19%. Attraverso il piano
  orizzontale di generazione va moltiplicato per <cos theta> = 0.808.
- **Angoli**: le distribuzioni zenitale (fino a 65 gradi) e azimutale di
  MACRO di `gun_muons_lngs.mac`, lette come intensita' per steradiante, con
  sin(theta) cos(theta) per il piano, bin uguali e il primo peso a zero.
  Theta medio 30.3 gradi. Lo zero dell'azimut rispetto al setup non e' noto
  (orientazione da misurare): lo script prende una rotazione phi0.
- **Energia**: Mei & Hime, eq. 3, h = 3.1 km.w.e., come dN/dE da 0.1 GeV a
  100 TeV: media 278 GeV, mediana 140.
- **Generazione**: `gun_muons_hallA.mac`, scritta da
  `produzione_hallA/muoni_macro.py`. Piano a z = +100 mm, 6.87 m^2, che
  copre ogni muone fino a 65 gradi che tocca il setup: **1.79 x 10^-3 Hz,
  ~155 muoni in 24 ore**.
- **Verifica** (20000 muoni, 2026-11-03): theta medio 30.2 gradi, lo 0.1%
  sotto 1 grado (prima il 13.9%), energia media 283 GeV e mediana 141. In 24
  ore attraversano il WC 8.7 muoni (107 mm d'acqua in media), ciascun
  pannello ~15.6, tutti e tre i pannelli 13.5, il WC e i tre pannelli 7.6.
- **Carica**: solo mu-; per i segnali non conta.
- **Non inclusi**: muoni multipli, sciami e neutroni prodotti nella roccia,
  i muoni oltre 65 gradi, la correlazione fra energia e angolo.

### Cosa non va nella macro attuale (`gun_muons_lngs.mac`)

Da correggere prima di usarla per il confronto:

1. **Il primo punto degli istogrammi in theta e phi diventa una delta**: il
   13.9% dei muoni e' esattamente verticale e il 5% ha phi = 0 (verificato il
   2026-10-31 su 1000 muoni).
2. **I valori in theta sembrano un'intensita' per steradiante** (massimo in
   verticale, 0.31 a 45 gradi, come ci si aspetta dall'attenuazione nella
   roccia), ma GPS li usa come probabilita' per bin di theta: mancano
   sin(theta) e cos(theta). Oggi theta medio vale 18.6 gradi; se i valori
   sono I(theta), attraverso un piano dovrebbe valere 30.3 gradi. Da
   verificare sulla figura di MACRO da cui vengono.
3. L'istogramma in phi ha bin larghi 10, 20 e 40 gradi trattati come uguali,
   e lo zero di phi rispetto al setup non e' definito.
4. **Energia**: 8 punti con il massimo a 270 GeV (la media di Mei & Hime
   presa per il picco), letti come dN/dE: l'energia media viene ~800 GeV
   invece di 278.
5. Il piano di 6.5 x 6.5 m a z = 1.5 m non copre i muoni piu' inclinati che
   colpiscono i bordi dei pannelli.

## 3. Radioattivita' dei PMT

Esclusa: irrilevante (decisione dell'utente).

## 4. Dai primari alla sequenza di 24 ore

- **Un primario per evento Geant4.** I tempi d'arrivo si assegnano dopo, come
  processo di Poisson col rate di ogni sorgente, e gamma e muoni si fondono
  in un'unica sequenza di 24 ore.
- **Il digitizzatore costruisce le finestre di acquisizione** (1024 campioni
  a 750 MS/s = 1.365 us) con tutti i depositi che ci cadono dentro, anche
  quelli sotto soglia: con 20.6 kHz di gamma, nel 2.8% delle finestre c'e' un
  secondo gamma. Per questo vanno simulati tutti i gamma.
- **Poi il self-trigger sui cinque readout come nella DAQ**: soglie dallo
  scan del 2026-10-30, DCR e crosstalk dei SiPM, impulsi propri dei PMT. E'
  il passo successivo, in `daq-standalone/analisi/20261030_digit_mc/digit_mc.py`.

## 5. La luce dei pannelli: una mappa

Con l'ottica dei pannelli accesa un gamma costa ~300 volte di piu' (sezione
7). Al suo posto:

- una **mappa di luce** di un pannello: PE medi per MeV e distribuzione dei
  tempi in funzione del punto, misurati una volta con l'ottica accesa (i tre
  pannelli sono uguali);
- per ogni deposito, il numero di PE si estrae con Poisson attorno alla
  media della mappa. E' la statistica giusta, perche' ogni fotone ha una
  piccola probabilita' di essere rivelato;
- la mappa si **valida** contro l'ottica completa su un campione di gamma:
  stesso spettro di PE;
- il MC deve salvare dove cadono i depositi nei pannelli (oggi solo il
  totale).

Con l'ottica accesa, i depositi dei gamma sopra 100 keV danno in media 33.2
PE/MeV (163 depositi), contro 35.1 dei muoni verticali in un punto: la mappa
media sara' attorno a quei valori.

## 6. Decisioni dell'utente (2026-11-03)

1. Spettro dei gamma: le righe.
2. Flusso dei muoni: MACRO, per unita' di area perpendicolare (sezione 2).
3. Muoni: il flusso e gli angoli di MACRO, come nelle simulazioni di CUPID;
   MUSUN scartato (prima scelto, poi abbandonato perche' difficile da avere).
4. Orientazione del setup in Hall A: **da misurare**.
5. Luce dei pannelli: mappa, con la validazione.
6. Radioattivita' dei PMT: no.
7. 24 ore di presa dati, sia per i gamma sia per i muoni.

## 7. Tempo di calcolo (misurato il 2026-11-03)

MacBook Air M4, 10 core (4 performance e 6 efficiency), 32 GB, senza
ventola. Gamma a righe sulla scatola dei pannelli:

| | per gamma | note |
|---|---|---|
| ottica dei pannelli spenta, 1 processo | 0.35 ms | inizializzazione 0.7 s |
| ottica dei pannelli spenta, 10 processi | **9470 gamma/s** in tutto | 3.3 volte un processo |
| ottica dei pannelli accesa, 1 processo | 84 ms | ~240 volte di piu' |

- **24 ore di gamma**: 1.78 x 10^9 / 9470 s^-1 = **~52 ore del Mac a pieno
  carico**. E' il ritmo dei primi 50 s: un MacBook Air senza ventola, sotto
  carico per giorni, rallenta, quindi conviene contare **3-4 giorni**.
- **Mappa di luce e validazione**: ~1 ora ciascuna con l'ottica accesa.
- **Muoni**: ~200 in 24 ore, trascurabili.
- **Spazio**: 118 byte per gamma generato, 209 GB per 24 ore se si scrive
  ogni evento. Solo il 15.8% dei gamma lascia qualcosa in un rivelatore:
  scrivendo solo quelli, ~33 GB.

Dalla stessa prova (500000 gamma, prima di soglie e digitizzazione): un
deposito sopra 200 keV in un pannello in totale a 1.0 kHz, cioe' ~340 Hz per
pannello; fotoni ai PMT del WC a 72 Hz, almeno 4 fotoni a 7.8 Hz; pannello
sopra 50 keV insieme a fotoni ai PMT a 21 Hz.

## 8. Cosa serve nel codice

Fatto il 2026-11-03:

1. **Muoni di MACRO**: `gun_muons_hallA.mac` da `muoni_macro.py`
   (sezione 2).
2. **Filtro in scrittura**: `/wc/output/onlyActive true` scrive solo gli
   eventi con fotoni ai PMT o energia in un pannello (i muoni sempre); la
   nuova ntupla `Run` conta i generati. Verificato su 20000 gamma: 3096 righe,
   le stesse degli eventi attivi della prova completa, e gli stessi fotoni.
3. **Posizione dei depositi nei pannelli**: colonne `Pos_panel_*_{X,Y,Z}_mm`,
   baricentro pesato con l'energia nel sistema dello scintillatore.

Da fare:

4. **Mappa di luce**: una macro e uno script che la costruiscono, e la
   validazione.
5. **Produzione**: lo script che divide i 1.78 x 10^9 gamma in pezzi (per
   esempio 10^7 a processo), con semi diversi e la possibilita' di
   ripartire.
6. **Dopo**: tempi di Poisson, fusione delle sorgenti e digitizzazione.

## Fonti

- Haffke et al., Background measurements in the Gran Sasso Underground
  Laboratory, NIM A 643 (2011) 36, arXiv:1101.5298.
- Mei & Hime, Phys. Rev. D 73 (2006) 053004.
- Borexino, JCAP 02 (2019) 046, arXiv:1808.04207; flussi di LVD e MACRO
  come riportati in arXiv:1406.6081.
- Kudryavtsev, MUSIC e MUSUN, Comput. Phys. Commun. 180 (2009) 339,
  arXiv:0810.4635; `musun-gs` in revertex (LEGEND).
