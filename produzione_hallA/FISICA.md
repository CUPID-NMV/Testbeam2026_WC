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
  ~155 muoni generati sul piano in 24 ore, di cui solo ~18 toccano un
  rivelatore** (vedi la verifica qui sotto). Il piano e' dieci volte un
  pannello perche' deve prendere anche i muoni inclinati che entrano dai
  lati: l'88% dei muoni generati manca il setup.
- **Verifica** (20000 muoni, 2026-11-03): theta medio 30.2 gradi, lo 0.1%
  sotto 1 grado (prima il 13.9%), energia media 283 GeV e mediana 141. In 24
  ore attraversano il WC 8.7 muoni (107 mm d'acqua in media), ciascun
  pannello ~15.6, tutti e tre i pannelli 13.5, il WC e i tre pannelli 7.6,
  almeno un rivelatore 18. Controllo a mano: 3.22e-4 x 0.808 x 0.6875 m^2
  x 86400 s = 15.5 muoni al giorno dalla faccia superiore di un pannello.
  Nel campione di 24 ore (155 muoni, 2026-11-03) toccano un rivelatore 13
  muoni: fluttuazione in giu' rispetto a 18 (p ~ 0.14).
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
- il MC salva il baricentro dei depositi in ogni pannello (colonne
  `Pos_panel_*`).

**La mappa** (2026-11-03, `produzione_hallA/mappa_luce.npz`): 7000 elettroni
da 1 MeV con l'ottica accesa (55 minuti sul Mac), bin di 50 x 50 mm, da 14 a
40 elettroni per bin, nessun bin vuoto.

- 32.5 PE/MeV in media, da 23 a 52 secondo il punto. Lungo il pannello la
  luce scende da ~39 PE/MeV vicino al SiPM a ~26 all'estremo opposto;
  il massimo, ~50, e' vicino al bordo d'uscita delle fibre, a y ~ 0.
- Nessuna dipendenza dalla profondita' del deposito: 32.0-32.9 PE/MeV in
  cinque fette in z.
- Tempi d'arrivo dei fotoelettroni: picco a ~20 ns e coda fino a ~100 ns
  (WLS esponenziale da 12 ns); dall'estremo lontano arrivano ~4 ns dopo.

**Validazione** su 20000 gamma di Hall A con l'ottica accesa: PE della mappa
contro PE veri, 0.976 / 1.019 / 0.994 nei pannelli alto / mezzo / basso,
0.996 in tutto; depositi con almeno 3, 7 e 20 PE il 42.6 / 23.5 / 8.9% veri
e il 42.8 / 23.4 / 8.7% dalla mappa. La mappa sostituisce l'ottica completa
entro la statistica.

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
- **Produzione vera** (2026-11-03, modalita' a basso consumo accesa, prima a
  batteria e poi all'alimentatore): la prima tornata di 10 pezzi da 5e6 ha
  impiegato 4770-4990 s, cioe' ~1020 gamma/s per processo e ~10200 in tutto,
  senza rallentamenti nei primi 80 minuti: **24 ore di gamma in ~49 ore**.
  La modalita' a basso consumo lascia inattivo ~25% della CPU. Spenta
  all'alimentatore ("Solo con batteria", dalle ~20:20 del 2026-11-03) ogni
  pezzo impiega 2550-2660 s: **~19200 gamma/s, quasi il tempo reale** (20608
  Hz), stabile per ore anche senza ventola. Le 24 ore finiscono in ~28 ore.
- **Mappa di luce e validazione**: ~1 ora ciascuna con l'ottica accesa.
- **Muoni con l'ottica dei pannelli accesa** (2026-11-08, all'alimentatore):
  8500 in 10 processi da 850, 3.0-3.3 ore ciascuno. Il doppio della stima
  fatta su 150 muoni, che avevano meno luce per evento.
- **Muoni**: ~155 generati sul piano in 24 ore (~18 nei rivelatori), mezzo
  minuto: trascurabili.
- **Spazio**: 118 byte per gamma generato, 209 GB per 24 ore se si scrive
  ogni evento. Solo il 15.8% dei gamma lascia qualcosa in un rivelatore:
  scrivendo solo quelli, ~33 GB stimati. Nella produzione vera 140 MB per
  pezzo da 5e6, ~50 GB in tutto.

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

4. **Produzione**: `produci.sh` (sotto).
5. **Mappa di luce**: `mappa_elettroni.mac` (elettroni da 1 MeV uniformi
   nello scintillatore del pannello alto), `ottica_accesa.sh` (eventi con
   l'ottica dei pannelli accesa, in parallelo) e `mappa_luce.py` (costruisce
   la mappa e la valida contro l'ottica completa).

6. **Validazione della mappa**: sezione 5.
7. **Digitalizzazione dei SiPM**: `Analysis_script/SiPM_maker.py`, sul
   modello di `Signal_maker.ipynb`. SiPM S13360-6075PE dal datasheet
   Hamamatsu (KAPD1052E07): guadagno, crosstalk, dark count e PDE in
   funzione della sovratensione, saturazione dei 6400 pixel. Catena di lettura
   a parte: discesa 64 ns (1280 pF su 50 ohm), salita 3 ns, amplificatore
   tarato perche' 1 p.e. faccia 2.9 mV a Vop (l'unica misura del LNGS),
   V1742 a 750 MS/s con baseline a 420 conteggi. Ingressi: fotoni del MC
   con l'ottica accesa, mappa di luce, o solo buio. ~670 eventi/s.

   **Con `--libreria` (dal 2026-11-04) la catena e' quella MISURATA al LNGS**,
   canale per canale, dai dark count della run 0039 (54 V) e dal rumore della
   run a 51 V: `daq-standalone/analisi/20261104_forma_1pe_sipm/`, file
   `daq-standalone/measurements/libreria_sipm_0039.npz`. La forma di 1 p.e.
   vera e' ~3 volte piu' larga di quella del datasheet (FWHM ~175 ns invece
   di ~50, coda al 15% del picco a +400 ns); crosstalk 7.5%, dispersione del
   guadagno 5-6%, DCR 1.5 / 1.5 / 2.4 MHz, rumore vero invece che bianco, e
   la base abbassata come fa l'accoppiamento in alternata della catena. Le
   finestre al buio simulate cosi' riproducono la 0039 (verifica nella stessa
   cartella). Restano dal datasheet PDE e saturazione. **Senza l'opzione
   tutto resta come prima** (uscita identica, verificato); per toglierla si
   cancellano la classe `CatenaMisurata` e le righe marcate `# libreria`.

8. **Le 24 ore come una presa dati, nel formato del DAQ** (2026-11-05,
   richiesta dell'utente: il MC si analizza con lo stesso strumento dei
   dati, `daq-standalone/tools/Distribuzioni_carica.ipynb`):
   `produzione_hallA/digit_daq.py`. Script nuovo, non tocca ne'
   `SiPM_maker.py` ne' `digit_mc.py`; per toglierlo si cancella.
   - **Uscita**: run `MC_HallA__54-0V_NNNN_750Ms_55PT.h5.gz` col `.log`
     accanto, in `data/produzione_hallA/daq_24h/run/`, 100000 eventi l'una
     come le run vere. HDF5 v3 come la DAQ (`/config` con gli stessi
     attributi, `/events/waveforms` int16 a 5 x 1012 campioni,
     `event_counter`, `trigger_time_tag`, `start_index_cell`); in piu' il
     gruppo `/mc` con la verita' (pezzo, EventID, sorgente, PE, energie),
     che gli strumenti dei dati non guardano. Il `.log` dichiara che e' MC e
     contiene le righe che il notebook legge (durata, eventi, DCOffset).
   - **Tempi**: un flusso di 24 ore, ogni gamma a EventID / 20608 Hz dal suo
     pezzo; i muoni di `muoni_2400h` che cadono nelle stesse 24 ore (~155,
     12 col trigger). Trigger Time Tag a 17.07 ns su 31 bit.
   - **PMT**: QE, TTS e risoluzione come `digit_mc.py`; forma del singolo PE,
     K e rumore vero dalla libreria con l'inverter
     (`libreria_digit_pmt_20261030.npz`).
   - **SiPM**: PE dalla mappa di luce, poi la `CatenaMisurata` di
     `SiPM_maker.py` (forma, crosstalk, guadagno, dark count della 0039;
     rumore della run a 51 V). Pannello alto, mezzo, basso = ch16, 17, 18,
     come in `SiPM_maker.py`: da misurare.
   - **Dai dati veri** (run 0130, 0132, 0134, 0135; comando `prepara`):
     soglie in conteggi dal bordo delle ampiezze, 813 / 1082 / 1082 / 221 /
     226 (la calibrazione del 30/10 da' 783 / 1038 / 1041 / 224 / 228, ma
     misura dalla mediana di tutta la traccia, che nei SiPM sta sopra la base
     per la coda lunga); livello di base per canale (entro 0.5 conteggi);
     dove cade nella finestra l'attraversamento della soglia del canale che
     scatta (SiPM 256.0 ns, PMT 259.4, sd ~5 ns); ritardo dei SiPM rispetto
     ai PMT, dai fronti negli eventi col trigger dei PMT e un segnale nel
     SiPM: +44.0 / +44.7 / +0.4 ns (ch18 ha un cavo, o una catena, ~44 ns
     piu' corto).
   - **Trigger**: si decide come il comparatore, sulla finestra CON rumore e
     buio: un canale scatta se la sua ampiezza misurata passa la soglia.
   - **Non c'e'**: tempo morto del DAQ, pile-up di eventi diversi, la
     sottoelongazione dei SiPM dopo un impulso grande, il rumore comune fra i
     canali, e il bordo morbido del trigger dei PMT (nei dati 210-250
     conteggi, nel MC netto a 221 / 226).
   - **Risultato** (2026-11-05, 7 run, 621650 eventi in 86400 s, 7.2 Hz
     contro 55.5 Hz dei dati): Distribuzioni_carica le legge come run vere
     (cancelli automatici entro pochi ns da quelli dei dati). Coi cancelli
     dei dati: PMT con la forma giusta ma bassi, 0.6 volte i dati sopra 1.5
     PE, 0.35 sopra 10 PE; SiPM 2.2 / 0.6 / 0.3 Hz sopra soglia contro 16.6
     / 16.8 / 15.3 Hz.
   - **La luce dei pannelli e' il punto debole**: con la mappa com'e' (32.5
     PE/MeV in media) le soglie dei SiPM valgono 2.3-2.9 MeV. Provato su 6
     pezzi (24 minuti di Hall A): a scala 2 i SiPM fanno 37 / 21 / 17 Hz
     sopra soglia (79 Hz di trigger), a 2.5 64 / 42 / 38 Hz (148 Hz); i dati
     vogliono ~1.7 per ch16 e ~1.9-2 per ch17 e ch18. Se nei dati ci sono
     sorgenti che il MC non ha (alluminio dell'housing, radon), la scala vera
     e' piu' bassa: a scala giusta deve tornare anche la forma dello spettro
     sopra soglia, non solo il rate. Con `prepara --scala-luce S` la luce si
     scala (le run cambiano nome); **la scala non e' stata scelta**.
   - **Non tararla sul rate di trigger**: nei dati il rate cambia da run a
     run. Da 55.3 Hz (0130, 2 novembre 23:15) sale a ~58 Hz in 7 ore, poi
     oscilla fra 57.5 e 59.8 (fino alla 0212, 4 novembre). Sono i SiPM
     (+5-7% fra le 0130-0135 e le 0154-0158; PMT +3%), con l'eccesso appena
     sopra soglia (+9-14% fra 1 e 1.1 volte la soglia, ~0 sopra 1.75). Il
     guadagno dei SiPM (1 p.e. dai dark count) e' fermo entro 0.5%, le soglie
     assolute e le basi in modo trasparente pure: sono eventi in piu' a
     bassa energia. Il radon e' un candidato per la VARIAZIONE (righe del
     Bi-214 a 1.76 e 2.2 MeV), da confrontare col monitor del radon di Hall
     A. **Il radon in se' nel MC c'e' gia'**, a livello fisso: le righe dei
     suoi figli (Pb-214, Bi-214) sono nello spettro a righe -- quelle del
     Bi-214 sopra 1.5 MeV (1764, 2204, 2448 keV) sono l'1.3% dei fotoni,
     piu' della riga a 2614 keV del Tl-208 (0.9%) -- e il flusso di Haffke e'
     una misura in sala, radon dell'aria compreso. Al MC mancano le sue
     variazioni, il radon dentro il setup e gli elettroni del Bi-214 (fino a
     3.27 MeV), che pero' darebbero al massimo ~1 Hz per pannello anche con
     1000 Bq/m^3 nell'aria dell'housing. Misure del 2026-11-05 nella
     sessione dei pannelli.
   - **Quanto radon serve per la variazione**: nel MC i gamma di Bi-214 e
     Pb-214 fanno ~30% dei trigger (SiPM alla scala che riproduce i dati,
     PMT a ogni scala). Il +5-7% dei SiPM vuole +15-25% di flusso del
     Bi-214, il +3% dei PMT +7-14%. Se l'aria della sala fa ~30-40% di quel
     flusso (stima grossolana: radon uniforme entro ~10 m, media di Hall A
     ~26 Bq/m^3 misurata da LVD), servono +25-80% di radon, cioe' da ~26 a
     ~33-47 Bq/m^3: variazioni comuni nelle sale (porte, ventilazione).
   - **Elettroni del radon dell'aria, provati con Geant4** (2026-11-05, in
     `data/produzione_hallA/radon_beta/`): elettroni con lo spettro beta
     del Bi-214 (70 rami di Geant4) dalle superfici esterne del serbatoio e
     della pila dei pannelli, emissione isotropa, poi lo stesso
     digitalizzatore del flusso di 24 ore. Per Bq/m^2 di Bi-214 depositato:
     WC 1.6e-5 Hz per PMT (con 1.4 MeV in acqua un elettrone fa ~0.3 PE per
     PMT: la resa e' ~0.7 PE/cm di muone); pannello alto 4.9e-3 Hz a scala
     della luce 2 (meta' dal fondo del serbatoio, 10 mm sopra), basso
     1.3e-3, mezzo ~0; a scala 1 cento volte meno. Con qualche decina di
     Bq/m^2: WC trascurabile, pannelli sotto l'1%. Non spiegano la
     variazione: il pannello di mezzo, che gli elettroni non raggiungono,
     sale quanto gli altri. Pannelli sigillati a Yale e acqua deionizzata
     dei LNF: radon dentro trascurabile (decisione dell'utente).
   - **Nemmeno la forma, per ora**: sopra ~1.5 volte la soglia lo spettro dei
     dati cade di 2-4 volte ogni 10% d'ampiezza, quello del MC molto piu'
     piano, a qualunque scala fra 1 e 2.5 (provato il 2026-11-05). Il bordo
     Compton del Tl-208 (2.38 MeV), che darebbe la scala senza dipendere dal
     numero di eventi, nel MC e' sbavato. Candidato da verificare: nella
     mappa la luce per MeV va da 23 a 52 PE secondo il punto del deposito.
   - Il codice riproduce le run bit per bit (verificato su un pezzo di gamma
     e sui muoni, 2026-11-05).
   - **Il difetto di luce non viene dalla mappa** (2026-11-07): sugli stessi
     20000 gamma della validazione, nel pannello piu' carico almeno 20 / 40 /
     60 / 75 PE in 389 / 97 / 15 / 7 eventi con l'ottica vera, 374 / 94 / 19
     / 5 dalla mappa; sopra la soglia di un SiPM (75 / 88 / 96 PE) 2 con
     l'ottica vera, 1.4 dalla mappa, 69 con la mappa raddoppiata. L'ottica
     completa non ha la luce che manca ai SiPM: il fattore ~2 sta nel modello
     del pannello o in sorgenti che il MC non ha.
   - **Un campione a parte, anche con l'ottica accesa** (2026-11-08, comando
     `campione`): un campione di `ottica_accesa.sh` digitalizzato coi
     parametri di un'uscita di `prepara`, in run sue (prefisso
     `MC_<campione>_<luce>__54-0V_`), un pezzo per file `.root`, gli eventi
     uno dopo l'altro al rate dato. Con `--luce fotoni` i PE dei SiPM sono i
     fotoni rivelati da Geant4 (PMT_ID 6-8, Detected = 1) coi loro tempi; con
     `--luce mappa`, di serie, vengono dalla mappa come nelle 24 ore. Il resto
     della catena e il trigger non cambiano. Le 24 ore restano identiche bit
     per bit (verificato su un pezzo di gamma e uno di muoni), `.log`
     compreso.
   - **Prova coi muoni** (2026-11-08): 8500 muoni di Hall A con l'ottica dei
     pannelli accesa (`ottica_muoni`, 55 giorni equivalenti): 1093 toccano un
     pannello (2.62 pannelli l'uno, 11.9 MeV mediani), 509 l'acqua. Stessi
     eventi, due digitalizzazioni: 1066 col trigger coi fotoni veri e 1067
     con la mappa, 1065 in comune, e lo stesso canale che scatta per primo
     (ch18, il SiPM col cavo corto, in meta' degli eventi). PE fotoni/mappa
     1.009 / 1.008 / 1.008 nei pannelli alto / mezzo / basso; 32.3 contro
     32.0 PE/MeV mediani, dispersione 12% contro 13-14%; la luce vera arriva
     ~1 ns dopo. Ampiezze e cariche uguali; il SiPM satura in piu' di meta'
     degli eventi. Distribuzioni_carica legge le run coi cancelli automatici:
     PMT entro 1-3 ns da quelli dei dati, SiPM da 196-240 ns invece che da
     152, perche' nei muoni scatta prima il ch18 e il ch16 e il ch17 arrivano
     44 ns dopo. Figura:
     `data/produzione_hallA/daq_ottica_muoni/confronto_fotoni_mappa.png`.
   - **Dati contro MC nelle stesse 24 ore** (2026-11-08): 50 run complete
     fra la 0130 e la 0196 (86506 s; fuori la 0181, la 0188 e la 0191, col
     rate 7-35% sotto le vicine: tempo morto del DAQ) contro le 7 run del MC
     (86400 s, mappa a scala 1). Cariche da una copia di Distribuzioni_carica,
     uguale per tutti e due, coi cancelli automatici dei dati. Sopra la
     soglia di trigger del MC, dati/MC: SiPM 8 / 32 / 63, PMT 1.7 / 1.5. Coi
     due PMT sopra 1.8 PE: 40514 eventi contro 20760 (1.95), spettri dei PMT
     della stessa forma; sono quasi tutti gamma, i muoni nel WC sono ~8 al
     giorno. Figure, numeri e script in
     `data/produzione_hallA/daq_24h/confronto_20261108/`.
   - **I muoni dei pannelli nei dati**: 24 al giorno con almeno 2 SiPM sopra
     0.6 milioni di ADC x ns, contro 16 attesi; dalle forme d'onda sono muoni
     veri (i SiPM saturano, ch16 e ch17 44 ns dopo il ch18). Solo 1 su 24 ha
     i due PMT sopra 1.8 PE e 17 nessun impulso nei PMT, contro il 46% del
     MC. Avevo concluso che il WC non stesse sui pannelli dove lo mette il MC:
     **e' sbagliato**. Il 2026-11-10 l'utente ha confermato, con le misure
     prese in caverna, che il WC sta rispetto ai pannelli esattamente come
     nel DetectorConstruction. La differenza fra dati e MC resta quindi da
     spiegare (luce nel WC per i muoni, selezione dei muoni dei pannelli,
     distribuzione angolare). Durante i grandi impulsi dei
     SiPM il ch21 raccoglie un'onda larga di ~30 conteggi. Nei dati i SiPM
     restano saturi piu' a lungo (mediana 137 / 209 / 167 ns contro 93 / 111
     / 105): segno di piu' luce che nel MC. Ma dopo la saturazione l'impulso
     dei dati scende piu' in fretta e ha la coda piu' bassa (la forma di 1
     p.e. del MC non descrive gli impulsi grandi): la carica di un muone non
     misura la luce.
   - **La durata degli impulsi dei PMT** (2026-11-08; 4.68 giorni di dati,
     run 0021-0283 tolte 14 col tempo morto; MC con la scintillazione delle
     fibre): larghezza (scarto quadratico dei tempi) e Laplace L(s)/L(0) con
     1/s = 10 ns. A parita' di carica i muoni del MC non durano di piu' dei
     gamma: sopra ~6 PE i piu' lunghi sono i gamma che depositano nel nucleo
     delle fibre (il 98% di quelli fra 6 e 25 PE; scintillazione a 11.8 e
     24.8 ns). Coi due PMT sopra N PE e tutti e due gli impulsi corti (sotto
     il picco a 3-6 PE + 0.8 ns, per campione e canale) i dati tornano sui
     gamma del MC: 164 / 31 / 6.6 eventi al giorno contro 140 / 30 / 9 a
     N = 6 / 8 / 10, mentre senza il taglio sono 3.6 / 5.3 / 7 volte il MC.
     L'eccesso dei dati ad alta carica sta tutto negli impulsi lenti. I muoni
     cosi' non si isolano: a N = 10 restano 6.6 eventi al giorno contro 0.9
     muoni (efficienza 10%). Il ch21 del MC e' ~1.5 ns piu' stretto del vero
     (era un baco della libreria dei PMT, corretto il 2026-11-09: vedi sotto).
     I muoni dei pannelli coi due PMT sopra soglia sono 7 in 4.7 giorni, a
     2-6 PE, con impulsi di pochi PE sparsi su ~30 ns.
   - **Dove i muoni dei pannelli attraversano il WC** (2026-11-08): x lungo
     le fibre dai due PMT (1 = ch20, 2 = ch21). T e' il primo segnale di
     ciascun PMT (primo campione sopra mezzo PE, fronte al 20% del primo
     impulso): quando l'impulso e' fatto di piu' PE, il fronte del picco
     piu' alto prende PE diversi nei due PMT. Q e' la carica da T - 5 a
     T + 100 ns. Taratura dai muoni del MC: T1 - T2 = 1.15 ns ogni 10 cm,
     ln(Q1/Q2) = -0.13 ogni 10 cm; con 2-6 PE la x viene dal tempo entro
     ~16 cm, dalla carica entro ~46. Spostamento dati - MC dal centro della
     nuvola dei gamma: T1 - T2 +6.0 ns (+6.1 fino alla 0212, +5.8 dalla
     0213; col fronte del picco piu' alto era +6.5, come fra A e B al
     testbeam), ln(Q1/Q2) +0.004 (guadagni bilanciati). I 7 muoni dei
     pannelli delle run normali (0022, 0023, 0029, 0107, 0192, 0247, 0265)
     stanno a x = -251, +280, +94, +4, +774, -268, +374 mm (+-153;
     negativo = lato del ch20); vincolato all'acqua il +774 diventa +437
     (68%: +373..+469). Sono sparsi su tutta la lunghezza, come se fossero
     uniformi: la sovrapposizione che manca col MC non e' quindi lungo le
     fibre ma di traverso (y), che i PMT non vedono.
   - **La run in maggioranza 5** (`LNGS_Majo5_54-0V_0000`, V812 coi cinque
     canali, 4-6/11, 43 ore, 7 eventi; tempo dal primo segnale): 3 sono
     muoni attraverso i tre pannelli (tre SiPM saturi), a x = -380 +- 113,
     -27 +- 153 e +583 +- 153 mm; vincolati all'acqua (mediana e 68%) -353
     (-434..-252), -27 (-179..+125), +407 (+315..+460). Gli altri 4 hanno
     impulsi veri e contemporanei in tutti e tre i SiPM (prima il ch18, poi
     ch16 e ch17 ~40 ns dopo, come i muoni), lontani dalla saturazione ma
     non piccoli: 140-780 conteggi, ~12-65 PE, ~0.4-2 MeV a pannello; nei
     PMT 2-8 PE. Stanno a +89, +856 (vincolato all'acqua +444), -98 e +1691
     mm. Nell'ultimo (1.8 PE nel ch20) la prima luce del ch20 arriva 27.6 ns
     dopo quella del ch21: con cosi' pochi PE il primo puo' arrivare tardi,
     e il tempo non da' la posizione.
   - **Il ch21 del MC stretto era un baco della libreria dei PMT** (2026-11-09,
     `data/produzione_hallA/daq_24h/confronto_20261109/baco_forma_ch21.ipynb`).
     `libreria_pmt.py` di daq-standalone prende gli eventi vuoti dei PMT dallo
     scan dei SiPM, ma nei suoi 40000 eventi c'e' solo lo scan del ch16. Li'
     il ch21 ha la diafonia del ch16, +1.6 conteggi a 240-500 ns. Effetti:
     - tolta dagli impulsi del ch21, la diafonia diventa un'undershoot finta,
       e a 3-6 PE gli impulsi del MC erano larghi 12.1 ns contro 15.3 nei dati;
     - nel rumore del MC del ch21 metteva +0.07 PE in ogni evento;
     - il PE del ch21 era sottostimato del 4% (1647 invece di 1712 ADC x ns,
       1709 nei dati).

     Geant4 e il resto della catena sono simmetrici fra i due PMT.
     `libreria_pmt_pulita.py` rifa' la libreria con la stessa funzione: il
     ch20 resta identico, il ch21 prende i vuoti dallo scan del ch20. MC
     ridigitalizzato in `daq_24h_lib1109` e `daq_ottica_muoni/mappa_lib1109`,
     con gli stessi eventi, soglie e ritardi; le produzioni vecchie restano.
     Risultati:
     - i trigger del ch21 salgono del 7.4% (+2.2% in tutto), gli altri canali
       non cambiano;
     - a 3-6 PE il ch21 del MC e' ora largo 16.5 ns, ~1 ns piu' dei dati: la
       coda della forma nuova a 50-150 ns sta ~0.005 del picco sopra quella
       degli impulsi del LNGS, mentre il ch20 coincide;
     - nelle 24 ore i due PMT ora hanno lo stesso rapporto dati/MC. Coi due
       PMT sopra 1.8 PE: 1.3 / 2.0 / 2.7 / 4.3 a 1.8-3 / 3-5 / 5-10 / 10-20
       PE sul ch21 (prima 1.3 / 2.1 / 3.0 / 4.9), 1.3 / 2.1 / 2.6 / 3.9 sul
       ch20;
     - divisi per durata, sul ch21 il conto torna come sul ch20: impulsi
       corti 1.2 / 1.7 / 2.1 / 2.5 volte il MC, lunghi 1.4 / 2.6 / 3.7 / 6.4
       (ch20: corti 1.1 / 1.6 / 1.7 / 2.0, lunghi 1.9 / 3.0 / 3.9 / 5.6).
       L'eccesso dei dati ad alta carica sta negli impulsi lunghi su tutti e
       due i PMT; quello dei corti, ~2 sopra 3 PE, resta.

     Notebook `confronto_pmt_24h_lib1109.ipynb` e
     `confronto_pmt_corti_lunghi_lib1109.ipynb` nella stessa cartella.

## 9. Come si lancia

Dalla cartella `produzione_hallA/`, con l'ambiente di Geant4 (`conda
activate geant4_env`) e il programma compilato in `build-arm64/`:

```bash
bash produci.sh muoni 24          # ~155 muoni sul piano (~18 nei rivelatori), mezzo minuto
bash produci.sh gamma 24          # 1.78e9 gamma in 356 pezzi da 5e6: 3-4 giorni
# la mappa di luce (~1 ora) e la sua validazione
bash ottica_accesa.sh produzione_hallA/mappa_elettroni.mac 7000 mappa
python3 -I mappa_luce.py costruisci ../data/produzione_hallA/ottica_mappa ../data/produzione_hallA/mappa_luce.npz
bash ottica_accesa.sh gun_gammas_background_righe_pannelli.mac 20000 valida
python3 -I mappa_luce.py valida ../data/produzione_hallA/ottica_valida ../data/produzione_hallA/mappa_luce.npz
# segnali dei SiPM, dalla mappa (la copia versionata e' produzione_hallA/mappa_luce.npz)
python3 -I ../Analysis_script/SiPM_maker.py mappa ../data/produzione_hallA/gamma_24h sipm_gamma.npz --mappa mappa_luce.npz
# idem con la catena misurata al LNGS
python3 -I ../Analysis_script/SiPM_maker.py mappa ../data/produzione_hallA/gamma_24h sipm_gamma.npz --mappa mappa_luce.npz \
    --libreria ~/daq-standalone/measurements/libreria_sipm_0039.npz
# le 24 ore nel formato del DAQ (sezione 8, punto 8): ~15 minuti in tutto
O=../data/produzione_hallA/daq_24h; L=~/nmv-copia/WC/LNGS
python3 -I digit_daq.py prepara $O $L/LNGS__54-0V_013{0,2,4,5}_750Ms_55PT.h5.gz
python3 -I digit_daq.py pezzi $O $(seq 0 39) --processi 9     # primo giro, per i ritardi
python3 -I digit_daq.py ritardi $O --scrivi
python3 -I digit_daq.py pezzi $O --rifai --processi 9
python3 -I digit_daq.py run $O
# 2026-11-09: la libreria dei PMT senza la diafonia ch16 -> ch21 (la vecchia la
# toglieva dagli impulsi del ch21 e la metteva nel suo rumore: impulsi MC del
# ch21 stretti, PE -4%; baco_forma_ch21.ipynb in daq_24h/confronto_20261109).
# Stessa produzione, cambia solo il ch21: i ritardi si copiano da $O
python3 -I libreria_pmt_pulita.py                      # -> libreria_digit_pmt_20261109.npz
O2=../data/produzione_hallA/daq_24h_lib1109
python3 -I digit_daq.py prepara $O2 $L/LNGS__54-0V_013{0,2,4,5}_750Ms_55PT.h5.gz --libreria-pmt libreria_digit_pmt_20261109.npz
#   (ritardo_ns di $O/parametri.json copiati in $O2/parametri.json; `ritardi $O2` li conferma entro 1 ns)
python3 -I digit_daq.py pezzi $O2 --processi 9
python3 -I digit_daq.py run $O2 --prefisso MC_HallA_lib1109__54-0V_
python3 -I digit_daq.py campione ../data/produzione_hallA/daq_ottica_muoni/mappa_lib1109 ../data/produzione_hallA/ottica_muoni \
    --parametri $O2 --rate 0.001789 --luce mappa --processi 10 --prefisso MC_ottica_muoni_mappa_lib1109__54-0V_
# 2026-11-10: il trigger dei PMT con l'efficienza in ampiezza misurata nello scan del 30/10
# (efficienza_trigger_pmt.ipynb in daq_24h/confronto_20261110: A50 259 / 274 conteggi, larghezza
# 25 / 29, contro il gradino a 221 / 226); stessa produzione, cambia solo il trigger dei PMT
O3=../data/produzione_hallA/daq_24h_lib1109eff
python3 -I digit_daq.py prepara $O3 $L/LNGS__54-0V_013{0,2,4,5}_750Ms_55PT.h5.gz --libreria-pmt libreria_digit_pmt_20261109.npz \
    --efficienza-pmt efficienza_trigger_pmt_20261030.json
#   (ritardo_ns copiati da $O2/parametri.json)
python3 -I digit_daq.py pezzi $O3 --processi 9
python3 -I digit_daq.py run $O3 --prefisso MC_HallA_lib1109eff__54-0V_
python3 -I digit_daq.py campione ../data/produzione_hallA/daq_muoni_2400h_lib1109eff ../data/produzione_hallA/muoni_2400h \
    --parametri $O3 --rate 0.001789 --luce mappa --processi 10 --prefisso MC_muoni_2400h_mappa_lib1109eff__54-0V_
# 2026-11-10: la libreria dei PMT dalla run coi SiPM spenti (LNGS__54-0V_PMTOnly_offset-10-10_HVoff_0000:
# solo i PMT nel trigger, offset 10, SiPM senza tensione): niente diafonia per costruzione, condizioni di
# adesso. Il ch21 torna sui dati (larghezza a 3-6 PE 15.3 contro 15.1 ns, rumore 364 contro 365 ADC*ns;
# libreria_sipmoff.ipynb in daq_24h/confronto_20261110)
python3 -I libreria_pmt_sipmoff.py ../data/produzione_hallA/run_speciali/LNGS__54-0V_PMTOnly_offset-10-10_HVoff_0000_750Ms_55PT.h5.gz
O4=../data/produzione_hallA/daq_24h_lib1110eff
python3 -I digit_daq.py prepara $O4 $L/LNGS__54-0V_013{0,2,4,5}_750Ms_55PT.h5.gz --libreria-pmt libreria_digit_pmt_20261110_sipmoff.npz \
    --efficienza-pmt efficienza_trigger_pmt_20261030.json
#   (ritardo_ns copiati da $O3/parametri.json)
python3 -I digit_daq.py pezzi $O4 --processi 9
python3 -I digit_daq.py run $O4 --prefisso MC_HallA_lib1110eff__54-0V_
python3 -I digit_daq.py campione ../data/produzione_hallA/daq_muoni_2400h_lib1110eff ../data/produzione_hallA/muoni_2400h \
    --parametri $O4 --rate 0.001789 --luce mappa --processi 10 --prefisso MC_muoni_2400h_mappa_lib1110eff__54-0V_
# 2026-11-10: produzione a resa alta per il fit della scintillazione delle fibre. Ogni fotone porta
# l'origine della sua catena e il ritardo di emissione della scintillazione (/wc/output/photonOrigin,
# OrigineFotone.hh; di serie spento, la fisica non cambia: verificato bit per bit coi semi uguali).
# Resa 7000 fotoni/MeV (4 x 1750), semi indipendenti dalle produzioni gia' fatte
export MAC_EXTRA=$'/wc/fiber/scintYield 7000\n/wc/output/photonOrigin true' ETICHETTA=_s7000
SEMI=4 bash produci.sh muoni 2400 10 1546      # -> muoni_2400h_s7000
SEMI=3 bash produci.sh gamma 6 9               # -> gamma_6h_s7000, 90 pezzi da 5e6
# digitalizzazione con resa S x 7000 e, volendo, altri tempi di scintillazione (campione):
python3 -I digit_daq.py campione USCITA ../data/produzione_hallA/gamma_6h_s7000 --parametri $O4 --rate 20608 \
    --processi 9 --scala-scint 0.25 [--tempi-scint 0.6,0.23,0.17/2.26,11.8,24.8]
# 2026-11-11, verifica di S = 0.25 contro gamma_24h a 1750 (verifica_s7000_S025.ipynb in daq_24h/confronto_20261111;
# uscita daq_gamma_6h_s7000_S025, prefisso MC_gamma_6h_s7000_S025__54-0V_): i gamma senza energia nelle fibre tornano
# (eventi col trigger 0.997 +- 0.003); quelli con le fibre hanno il 2.8 +- 0.9% di scintillazione in piu' a energia
# fissa, larghezze e parte lenta di Laplace uguali. La causa e' G4Scintillation (11.4.2, righe 405/431/450): con tre
# componenti il numero di fotoni di ognuna e' G4int(resa_k/somma*N), troncato, ~1.5 fotoni persi per step a ogni resa.
# La perdita relativa va come 1/resa: stima ~3.7% a 1750 e ~0.9% a 7000. Il diradamento e' giusto; le produzioni a
# 1750 hanno una resa efficace di ~1685 fotoni/MeV, e S vale ~7000 x 0.99 x S.
# 2026-11-11, fit di resa e tempi sui dati 0319-0362 (fit_scintillazione_fibre.ipynb in daq_24h/confronto_20261111;
# griglia digitalizzata con digit_griglia.sh, nella stessa cartella: per ogni etichetta TAG:S[:tempi] le uscite
# daq_gamma_6h_s7000_TAG e daq_muoni_2400h_s7000_TAG; S025..S100 coi tempi di Geant4, P<w4>t<tau4> a S = 0.40 con una
# quarta componente, es. P15t70 = --tempi-scint 0.51,0.1955,0.1445,0.15/2.26,11.81,24.76,70).
# Spettro selezionato fra 7000 e 85000 ADC*ns: S = 0.43 +- 0.01 (~2980 fotoni/MeV) con J x A, S = 0.39 +- 0.02 col fattore
# dei gamma libero (A = 1.1); fit non buoni (chi2/ndf 73/8, 39/7). Forma degli impulsi: +15% della scintillazione in una
# componente a ~70 ns (40-120) dimezza il chi2; restano i dati piu' larghi del 3-5% fra 1/s = 3 e 10 ns. Non spiegati:
# sotto 7000 ADC*ns il MC ha ~1.8 volte i dati (luce Cherenkov) con qualunque S; sopra 85000 (ampiezza satura) ~2 volte.
# 2026-11-11, i gamma col NaI (gamma_nai_righe.ipynb, stessa cartella): 2 ore di Hall A con la stessa scatola e lo stesso rate,
MACRO_GAMMA=gun_gammas_background_nai_pannelli.mac ETICHETTA=_nai_s7000 SEMI=5 \
  MAC_EXTRA=$'/wc/fiber/scintYield 7000\n/wc/output/photonOrigin true' caffeinate -i bash produci.sh gamma 2 10
# digitalizzata come la griglia (uscite daq_gamma_2h_nai_s7000_S043/S060/S080/S100). Lo spettro non spiega l'eccesso basso:
# il rapporto fra eventi sotto e sopra 7000 ADC*ns e' 0.56 nei dati, 1.06 a righe e 1.23 col NaI (S = 0.43); col NaI a J x A
# la zona bassa torna (0.92) ma sopra 7000 serve A = 2.07; alzando S il rapporto scende ma la forma sopra 7000 si guasta.
# i 100 giorni di muoni (ottica dei pannelli spenta), tutti, con la libreria nuova (~30 s):
# 1857 eventi col trigger, 904 nell'acqua
python3 -I digit_daq.py campione ../data/produzione_hallA/daq_muoni_2400h_lib1109 ../data/produzione_hallA/muoni_2400h \
    --parametri $O2 --rate 0.001789 --luce mappa --processi 10 --prefisso MC_muoni_2400h_mappa_lib1109__54-0V_
# un campione con l'ottica dei pannelli accesa, nel formato del DAQ, coi
# PE dei SiPM dai fotoni veri e, per confronto, dalla mappa (~3 ore + 1 minuto);
# --rate: i muoni generati al secondo sul piano di gun_muons_hallA.mac
bash ottica_accesa.sh gun_muons_hallA.mac 8500 muoni 10
for L in fotoni mappa; do
  python3 -I digit_daq.py campione ../data/produzione_hallA/daq_ottica_muoni/$L ../data/produzione_hallA/ottica_muoni \
      --parametri $O --rate 0.001789 --luce $L --processi 10
done
```

- Le uscite vanno in `data/produzione_hallA/` (fuori da git), un file per
  pezzo, con il log e un `.ok` a pezzo finito. `produzione.txt` riassume
  sorgente, rate, eventi e versione del codice.
- **Se la produzione si interrompe** (riavvio, Mac chiuso) si rilancia lo
  stesso comando: rifa' solo i pezzi senza `.ok`.
- Il Mac non va in stop da solo finche' gira (`caffeinate`), ma col coperchio
  chiuso si'. Su un MacBook Air senza ventola, sotto carico per giorni, i
  processi rallentano: nelle prove ogni processo e' passato da ~950 a ~710
  gamma/s.
- Lo script di Python vuole `uproot`: `~/venvs/jupyter-global/bin/python`.

## Fonti

- Haffke et al., Background measurements in the Gran Sasso Underground
  Laboratory, NIM A 643 (2011) 36, arXiv:1101.5298.
- Mei & Hime, Phys. Rev. D 73 (2006) 053004.
- Borexino, JCAP 02 (2019) 046, arXiv:1808.04207; flussi di LVD e MACRO
  come riportati in arXiv:1406.6081.
- Kudryavtsev, MUSIC e MUSUN, Comput. Phys. Commun. 180 (2009) 339,
  arXiv:0810.4635; `musun-gs` in revertex (LEGEND).
