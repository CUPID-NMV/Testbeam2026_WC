#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4Step.hh"
#include "G4EventManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpticalPhoton.hh"

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
    }
    return;
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
  }

  // ===================================================
  // 3. PRODUZIONE CHERENKOV (da qualsiasi particella carica)
  // ===================================================
  auto secondaries = step->GetSecondaryInCurrentStep();
  for (auto sec : *secondaries) {
    if (sec->GetCreatorProcess() &&
        sec->GetCreatorProcess()->GetProcessName() == "Cerenkov") {
      eventAction->AddCerenkovProd();
      G4bool isPrimary = (track->GetTrackID() == 1);
      G4String parentProc = (track->GetCreatorProcess()) ?
                             track->GetCreatorProcess()->GetProcessName() : "";
      eventAction->AddCerenkovByProcess(parentProc, isPrimary);
    }
  }
}
