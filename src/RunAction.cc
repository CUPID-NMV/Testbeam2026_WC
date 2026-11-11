#include "RunAction.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4Threading.hh"

RunAction::RunAction() : G4UserRunAction() {
  auto am = G4AnalysisManager::Instance();
  am->SetDefaultFileType("root");
  am->SetFileName("test_debug");
  am->SetNtupleMerging(true);

  // Ntuple 0: una riga per evento (= un primario), 55 colonne. Formato
  // unito con il MC dei pannelli della collega (Donatella): sue le colonne
  // del primario e quelle dei pannelli, una copia per pannello.
  // EventAction le riempie in quest'ordine.
  am->CreateNtuple("Eventi", "Dati Evento");
  // --- evento e primario alla generazione ---
  am->CreateNtupleIColumn("EventID");              // 0  stesso EventID di "Fotoni"
  am->CreateNtupleIColumn("SourceType");           // 1  0=e-, 1=gamma, 2=mu-
  am->CreateNtupleDColumn("E_gen_MeV");            // 2  energia cinetica al vertice
  am->CreateNtupleDColumn("Vtx_X_mm");             // 3
  am->CreateNtupleDColumn("Vtx_Y_mm");             // 4
  am->CreateNtupleDColumn("Vtx_Z_mm");             // 5
  am->CreateNtupleDColumn("Dir_X");                // 6  direzione del primario
  am->CreateNtupleDColumn("Dir_Y");                // 7
  am->CreateNtupleDColumn("Dir_Z");                // 8
  // --- WC ---
  am->CreateNtupleDColumn("E_init_MeV");           // 9  energia cinetica del primario all'ingresso in acqua
  am->CreateNtupleDColumn("TrackL_water_mm");      // 10
  am->CreateNtupleDColumn("Edep_water_MeV");       // 11 depositata dal solo primario
  am->CreateNtupleIColumn("N_Cerenkov_prod");      // 12 nel WC (non nei pannelli)
  am->CreateNtupleIColumn("N_Cerenkov_entrati");   // 13 nati nell'acqua, entrati in una fibra
  am->CreateNtupleIColumn("N_Cer_primary");        // 14 Cerenkov da primario diretto
  am->CreateNtupleIColumn("N_Cer_compton");        // 15 Cerenkov da e- Compton/fotoelettrico
  am->CreateNtupleIColumn("N_Cer_pair");           // 16 Cerenkov da coppie e+/e- (conv)
  am->CreateNtupleIColumn("N_Cer_deltaEM");        // 17 Cerenkov da delta-ray (ioniz. EM)
  am->CreateNtupleIColumn("N_Cer_nuclear");        // 18 Cerenkov da prodotti nucleari
  // Energia nel nucleo delle fibre (dove scintillano), divisa al pettine
  am->CreateNtupleDColumn("Edep_fiber_L_MeV");     // 19 verso il PMT L (x < -pettine)
  am->CreateNtupleDColumn("Edep_fiber_C_MeV");     // 20 tratto dritto
  am->CreateNtupleDColumn("Edep_fiber_R_MeV");     // 21 verso il PMT R (x > +pettine)
  // Fotoni entrati nel vetro dei PMT, prima della QE (la applica il digitizer)
  am->CreateNtupleIColumn("Hits_PMT_L");           // 22
  am->CreateNtupleIColumn("Hits_PMT_R");           // 23
  am->CreateNtupleDColumn("T_FirstHit_ns");        // 24 primo fotone su un PMT (-1 se nessuno)
  // --- pannelli (solo single_pan3, se no 0): top = il piu' vicino al serbatoio ---
  // Energia nell'EJ-200, tutte le particelle
  am->CreateNtupleDColumn("Edep_panel_top_MeV");   // 25
  am->CreateNtupleDColumn("Edep_panel_mid_MeV");   // 26
  am->CreateNtupleDColumn("Edep_panel_bot_MeV");   // 27
  // Percorso del primario nell'EJ-200
  am->CreateNtupleDColumn("TrackL_panel_top_mm");  // 28
  am->CreateNtupleDColumn("TrackL_panel_mid_mm");  // 29
  am->CreateNtupleDColumn("TrackL_panel_bot_mm");  // 30
  // Da qui solo con /wc/panel/optics true, se no 0 (-1 i tempi):
  // fotoni di scintillazione prodotti nell'EJ-200
  am->CreateNtupleIColumn("N_Scint_top");          // 31
  am->CreateNtupleIColumn("N_Scint_mid");          // 32
  am->CreateNtupleIColumn("N_Scint_bot");          // 33
  // fotoni entrati nel nucleo di una fibra (una volta per fotone)
  am->CreateNtupleIColumn("PhotonsInFiber_top");   // 34
  am->CreateNtupleIColumn("PhotonsInFiber_mid");   // 35
  am->CreateNtupleIColumn("PhotonsInFiber_bot");   // 36
  // fotoni arrivati al SiPM (dopo grasso e finestra), rivelati o no
  am->CreateNtupleIColumn("Hits_SiPM_top_Total");  // 37
  am->CreateNtupleIColumn("Hits_SiPM_mid_Total");  // 38
  am->CreateNtupleIColumn("Hits_SiPM_bot_Total");  // 39
  // di cui rivelati (PDE): fotoelettroni
  am->CreateNtupleIColumn("Hits_SiPM_top_Detected"); // 40
  am->CreateNtupleIColumn("Hits_SiPM_mid_Detected"); // 41
  am->CreateNtupleIColumn("Hits_SiPM_bot_Detected"); // 42
  // tempo del primo fotone rivelato (-1 se nessuno)
  am->CreateNtupleDColumn("T_FirstHit_SiPM_top_ns"); // 43
  am->CreateNtupleDColumn("T_FirstHit_SiPM_mid_ns"); // 44
  am->CreateNtupleDColumn("T_FirstHit_SiPM_bot_ns"); // 45
  // baricentro dei depositi nell'EJ-200, pesato con l'energia, nel sistema
  // dello scintillatore (origine al centro, x lungo il lato lungo con il SiPM
  // a -x, z in alto); 0 se nel pannello non c'e' energia. Per la mappa di luce.
  am->CreateNtupleDColumn("Pos_panel_top_X_mm");   // 46
  am->CreateNtupleDColumn("Pos_panel_top_Y_mm");   // 47
  am->CreateNtupleDColumn("Pos_panel_top_Z_mm");   // 48
  am->CreateNtupleDColumn("Pos_panel_mid_X_mm");   // 49
  am->CreateNtupleDColumn("Pos_panel_mid_Y_mm");   // 50
  am->CreateNtupleDColumn("Pos_panel_mid_Z_mm");   // 51
  am->CreateNtupleDColumn("Pos_panel_bot_X_mm");   // 52
  am->CreateNtupleDColumn("Pos_panel_bot_Y_mm");   // 53
  am->CreateNtupleDColumn("Pos_panel_bot_Z_mm");   // 54
  am->FinishNtuple(0);

  // Ntuple 1: una riga per fotone arrivato a un fotorivelatore
  am->CreateNtuple("Fotoni", "Dati Fotoni");
  am->CreateNtupleIColumn("EventID");              // 0
  am->CreateNtupleIColumn("TrackID");              // 1
  am->CreateNtupleDColumn("E_Hit_eV");             // 2
  am->CreateNtupleDColumn("Arrival_Time_ns");      // 3  tempo globale dall'inizio dell'evento
  am->CreateNtupleIColumn("PMT_ID");               // 4  0=PMT L, 1=PMT R (2-5 PMT di triple); 6/7/8 = SiPM top/mid/bot
  am->CreateNtupleIColumn("Detected");             // 5  SiPM: 1 rivelato, 0 no (PDE); PMT: -1 (QE nel digitizer)
  am->CreateNtupleIColumn("Origine");              // 6  origine della catena (OrigineFotone.hh): 0 Cherenkov in acqua,
                                                   //    1 scintillazione nel nucleo delle fibre del WC, 2 Cherenkov
                                                   //    nel nucleo, 3 altro; -1 senza /wc/output/photonOrigin
  am->CreateNtupleDColumn("Ritardo_scint_ns");     // 7  scintillazione: ritardo di emissione rispetto al passo; se no 0
  am->FinishNtuple(1);

  // Ntuple 2: una riga per run (in multithread una per thread di lavoro):
  // i primari generati, anche quelli non scritti con /wc/output/onlyActive.
  // Servono per la normalizzazione: tempo equivalente = generati / rate.
  am->CreateNtuple("Run", "Primari generati");
  am->CreateNtupleIColumn("N_generated");          // 0
  am->FinishNtuple(2);
}

RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*) {
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run* run) {
  auto am = G4AnalysisManager::Instance();
  // in multithread il master ha il totale dei thread: lo scrivono i thread,
  // e i generati sono la somma delle righe
  if (!G4Threading::IsMultithreadedApplication() || G4Threading::IsWorkerThread()) {
    am->FillNtupleIColumn(2, 0, run->GetNumberOfEvent());
    am->AddNtupleRow(2);
  }
  am->Write();
  am->CloseFile();
}
