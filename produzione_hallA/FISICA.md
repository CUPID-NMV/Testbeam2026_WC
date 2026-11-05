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
