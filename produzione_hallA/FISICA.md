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

Da fare:
8. **Dopo**: tempi di Poisson, fusione delle sorgenti, digitalizzazione
   anche dei PMT e trigger. Le 24 ore di gamma danno ~2.7 x 10^8 eventi con
   energia nei pannelli: a ~670 eventi/s vanno divisi su piu' processi, o
   trattati a parte quelli piccoli.

## 9. Come si lancia

Dalla cartella `produzione_hallA/`, con l'ambiente di Geant4 (`conda
activate geant4_env`) e il programma compilato in `build-arm64/`:

```bash
bash produci.sh muoni 24          # ~155 muoni, mezzo minuto
bash produci.sh gamma 24          # 1.78e9 gamma in 356 pezzi da 5e6: 3-4 giorni
# la mappa di luce (~1 ora) e la sua validazione
bash ottica_accesa.sh produzione_hallA/mappa_elettroni.mac 7000 mappa
python3 -I mappa_luce.py costruisci ../data/produzione_hallA/ottica_mappa ../data/produzione_hallA/mappa_luce.npz
bash ottica_accesa.sh gun_gammas_background_righe_pannelli.mac 20000 valida
python3 -I mappa_luce.py valida ../data/produzione_hallA/ottica_valida ../data/produzione_hallA/mappa_luce.npz
# segnali dei SiPM, dalla mappa (la copia versionata e' produzione_hallA/mappa_luce.npz)
python3 -I ../Analysis_script/SiPM_maker.py mappa ../data/produzione_hallA/gamma_24h sipm_gamma.npz --mappa mappa_luce.npz
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
