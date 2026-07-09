#include "RunAction.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"

RunAction::RunAction() : G4UserRunAction() {
  auto am = G4AnalysisManager::Instance();
  am->SetDefaultFileType("root");
  am->SetFileName("test_debug");
  am->SetNtupleMerging(true);

  // Ntuple 0: sommario per evento (13 colonne)
  am->CreateNtuple("Eventi", "Dati Evento");
  am->CreateNtupleDColumn("E_init_MeV");          // 0  energia cinetica primaria in acqua
  am->CreateNtupleDColumn("TrackL_water_mm");      // 1
  am->CreateNtupleDColumn("Edep_water_MeV");       // 2
  am->CreateNtupleIColumn("N_Cerenkov_prod");      // 3
  am->CreateNtupleIColumn("N_Cerenkov_entrati");   // 4
  am->CreateNtupleDColumn("T_FirstHit_ns");        // 5
  am->CreateNtupleIColumn("Hits_L_Bot");           // 6  (single: PMT_L)
  am->CreateNtupleIColumn("Hits_R_Bot");           // 7  (single: PMT_R)
  am->CreateNtupleIColumn("Hits_L_Mid");           // 8  (single: sempre 0)
  am->CreateNtupleIColumn("Hits_R_Mid");           // 9  (single: sempre 0)
  am->CreateNtupleIColumn("Hits_L_Top");           // 10 (single: sempre 0)
  am->CreateNtupleIColumn("Hits_R_Top");           // 11 (single: sempre 0)
  am->CreateNtupleIColumn("SourceType");           // 12 0=e-, 1=gamma, 2=mu-
  am->CreateNtupleIColumn("N_Cer_primary");        // 13 Cerenkov da primario diretto
  am->CreateNtupleIColumn("N_Cer_compton");        // 14 Cerenkov da e- Compton/fotoelettrico
  am->CreateNtupleIColumn("N_Cer_pair");           // 15 Cerenkov da coppie e+/e- (conv)
  am->CreateNtupleIColumn("N_Cer_deltaEM");        // 16 Cerenkov da delta-ray (ioniz. EM)
  am->CreateNtupleIColumn("N_Cer_nuclear");        // 17 Cerenkov da prodotti nucleari
  am->FinishNtuple(0);

  // Ntuple 1: dettaglio fotoni al PMT
  am->CreateNtuple("Fotoni", "Dati Fotoni");
  am->CreateNtupleIColumn("EventID");              // 0
  am->CreateNtupleIColumn("TrackID");              // 1
  am->CreateNtupleDColumn("E_Hit_eV");             // 2
  am->CreateNtupleDColumn("Arrival_Time_ns");      // 3
  am->CreateNtupleIColumn("PMT_ID");               // 4
  am->FinishNtuple(1);
}

RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*) {
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*) {
  auto am = G4AnalysisManager::Instance();
  am->Write();
  am->CloseFile();
}
