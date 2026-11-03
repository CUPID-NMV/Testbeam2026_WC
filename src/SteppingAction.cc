#include "SteppingAction.hh"
#include "EventAction.hh"
#include "FiberScintillation.hh"
#include "ScintPanel.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4AnalysisManager.hh"

namespace {
// Esito del processo di bordo nel passo appena fatto (il processo e' uno per thread)
G4OpBoundaryProcessStatus StatoBordo() {
  static G4ThreadLocal G4OpBoundaryProcess* bordo = nullptr;
  if (!bordo) {
    G4ProcessVector* pv = G4OpticalPhoton::Definition()->GetProcessManager()->GetPostStepProcessVector();
    for (G4int j = 0; j < (G4int)pv->entries(); ++j)
      if ((*pv)[j]->GetProcessName() == "OpBoundary") {
        bordo = dynamic_cast<G4OpBoundaryProcess*>((*pv)[j]);
        break;
      }
  }
  return bordo ? bordo->GetStatus() : Undefined;
}
}

SteppingAction::SteppingAction() : G4UserSteppingAction() {}
SteppingAction::~SteppingAction() {}

void SteppingAction::UserSteppingAction(const G4Step* step) {

  auto track = step->GetTrack();
  auto eventAction = static_cast<EventAction*>(
      G4EventManager::GetEventManager()->GetUserEventAction());

  // ===================================================
  // 1. FOTONI OTTICI: conteggio entrata nelle fibre
  // ===================================================
  if (track->GetDefinition() == G4OpticalPhoton::Definition()) {
    auto prePoint  = step->GetPreStepPoint();
    auto postPoint = step->GetPostStepPoint();
    if (prePoint->GetPhysicalVolume() && postPoint->GetPhysicalVolume()) {
      G4String preName  = prePoint->GetPhysicalVolume()->GetName();
      G4String postName = postPoint->GetPhysicalVolume()->GetName();
      bool isFiber = (postName.find("P_")    != std::string::npos ||
                      postName.find("Fib")   != std::string::npos ||
                      postName.find("Fiber") != std::string::npos);
      if (preName == "WaterPhys" && isFiber) {
        if (track->GetCreatorProcess() &&
            track->GetCreatorProcess()->GetProcessName() == "Cerenkov") {
          if (eventAction->IsPhotonUnique(track->GetTrackID()))
            eventAction->AddCerenkovEntrati();
        }
      }

      // Pannelli (single_pan3), come nel MC della collega:
      //  - un fotone che entra nel nucleo di una fibra (PhotonsInFiber);
      //  - un fotone che arriva al SiPM, dopo grasso e finestra: arrivato
      //    sempre, rivelato se il processo di bordo da' Detection (la PDE e'
      //    l'EFFICIENCY della superficie del SiPM).
      const ScintPanel& pan = ScintPanel::Get();
      if (pan.SiPM() && postPoint->GetStepStatus() == fGeomBoundary) {
        const G4LogicalVolume* preLV  = prePoint->GetPhysicalVolume()->GetLogicalVolume();
        const G4LogicalVolume* postLV = postPoint->GetPhysicalVolume()->GetLogicalVolume();
        if (postLV->GetMaterial() == pan.FiberCore() && preLV->GetMaterial() != pan.FiberCore()) {
          if (eventAction->IsPanelPhotonUnique(track->GetTrackID()))
            eventAction->AddPhotonInFiber(pan.PanelIndex(postPoint->GetTouchable()));
        } else if (postLV == pan.SiPM() && preLV != pan.SiPM()) {
          const G4int k = pan.PanelIndex(postPoint->GetTouchable());
          const G4bool rivelato = (StatoBordo() == Detection);
          const G4double t = postPoint->GetGlobalTime();
          eventAction->AddSiPMPhoton(k, rivelato, t);
          auto am = G4AnalysisManager::Instance();
          am->FillNtupleIColumn(1, 0, G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID());
          am->FillNtupleIColumn(1, 1, track->GetTrackID());
          am->FillNtupleDColumn(1, 2, track->GetKineticEnergy()/eV);
          am->FillNtupleDColumn(1, 3, t/ns);
          am->FillNtupleIColumn(1, 4, 6 + k);
          am->FillNtupleIColumn(1, 5, rivelato ? 1 : 0);
          am->AddNtupleRow(1);
          track->SetTrackStatus(fStopAndKill);   // gia' assorbito dalla superficie
        }
      }
    }
    return;
  }

  // ===================================================
  // 1b. ENERGIA NEL NUCLEO DELLE FIBRE (dove scintillano), divisa fra il
  //     tratto dritto e le due estremita' oltre il pettine (curve, fascio):
  //     dice dove nasce la luce di scintillazione
  // ===================================================
  G4double edepFib = step->GetTotalEnergyDeposit();
  if (edepFib > 0) {
    auto pvFib = step->GetPreStepPoint()->GetPhysicalVolume();
    if (pvFib && pvFib->GetName() == "core") {
      G4double x  = step->GetPreStepPoint()->GetPosition().x();
      G4double xe = FiberScintillation::Get().EndX();
      eventAction->AddEdepFiber(x < -xe ? 0 : (x > xe ? 2 : 1), edepFib);
    }
    // 1c. energia nello scintillatore dei pannelli (geometria single_pan3),
    //     con il punto di mezzo del passo nel sistema dello scintillatore
    if (pvFib && pvFib->GetName() == "Pan_ScintPhys") {
      const G4StepPoint* pre = step->GetPreStepPoint();
      const G4ThreeVector centro = 0.5*(pre->GetPosition() + step->GetPostStepPoint()->GetPosition());
      const G4ThreeVector locale = pre->GetTouchable()->GetHistory()->GetTopTransform().TransformPoint(centro);
      eventAction->AddEdepPanel(ScintPanel::Get().PanelIndex(pre->GetTouchable()), edepFib, locale);
    }
  }

  // ===================================================
  // 2. PARTICELLA PRIMARIA NELL'ACQUA (qualsiasi tipo)
  //    trackID==1: e- per testbeam, gamma per fondo, mu- per cosmici
  // ===================================================
  if (track->GetTrackID() == 1) {
    auto volume = track->GetVolume();
    if (volume && volume->GetName() == "WaterPhys") {
      if (step->IsFirstStepInVolume())
        eventAction->SetInitialEnergy(step->GetPreStepPoint()->GetKineticEnergy());
      eventAction->AddEdep(step->GetTotalEnergyDeposit());
      eventAction->AddTrackLength(step->GetStepLength());
    }
    // percorso del primario nello scintillatore di ogni pannello
    else if (volume && volume->GetName() == "Pan_ScintPhys")
      eventAction->AddTrackLPanel(ScintPanel::Get().PanelIndex(step->GetPreStepPoint()->GetTouchable()),
                                  step->GetStepLength());
  }

  // ===================================================
  // 3. PRODUZIONE CHERENKOV (da qualsiasi particella carica) e, nei
  //    pannelli, SCINTILLAZIONE
  // ===================================================
  auto secondaries = step->GetSecondaryInCurrentStep();
  G4int pannello = -2;   // -2: non ancora guardato
  for (auto sec : *secondaries) {
    if (!sec->GetCreatorProcess()) continue;
    const G4String& creatore = sec->GetCreatorProcess()->GetProcessName();
    if (creatore == "Scintillation") {
      // N_Scint dei pannelli; la scintillazione delle fibre del WC no
      if (pannello == -2) pannello = ScintPanel::Get().PanelIndex(step->GetPreStepPoint()->GetTouchable());
      if (pannello >= 0) eventAction->AddScintPanel(pannello);
      continue;
    }
    if (creatore == "Cerenkov") {
      // il Cherenkov dei pannelli (c'e' solo con /wc/panel/optics true) non
      // e' del WC
      if (pannello == -2) pannello = ScintPanel::Get().PanelIndex(step->GetPreStepPoint()->GetTouchable());
      if (pannello >= 0) continue;
      eventAction->AddCerenkovProd();
      G4bool isPrimary = (track->GetTrackID() == 1);
      G4String parentProc = (track->GetCreatorProcess()) ?
                             track->GetCreatorProcess()->GetProcessName() : "";
      eventAction->AddCerenkovByProcess(parentProc, isPrimary);
    }
  }
}
