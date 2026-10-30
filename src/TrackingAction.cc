#include "TrackingAction.hh"

#include "G4GenericMessenger.hh"
#include "G4OpticalPhoton.hh"
#include "G4Track.hh"
#include "G4TrackingManager.hh"

#include <algorithm>
#include <cmath>

namespace {
G4double gPhotonFraction = 1.0;
G4GenericMessenger* gMessenger = nullptr;
}

void TrackingAction::CreateCommands() {
  if (gMessenger) return;
  gMessenger = new G4GenericMessenger(&gPhotonFraction, "/wc/vis/", "visualizzazione");
  gMessenger->DeclareProperty("photonFraction", gPhotonFraction,
      "frazione dei fotoni ottici di cui salvare (e disegnare) la traiettoria: "
      "1 = tutti, 0.01 = uno ogni 100, 0 = nessuno. Non cambia la fisica.");
}

TrackingAction::TrackingAction() : G4UserTrackingAction(), fStore(0), fSpento(false) {}

void TrackingAction::PreUserTrackingAction(const G4Track* track) {
  if (gPhotonFraction >= 1.0 && !fSpento) return;   // il caso di sempre

  // La visualizzazione chiede le traiettorie con /tracking/storeTrajectory,
  // che vale per tutte le tracce: lo si spegne per i fotoni scartati e lo si
  // riaccende per le tracce dopo.
  if (!fSpento) fStore = fpTrackingManager->GetStoreTrajectory();
  G4bool tieni = true;
  if (track->GetDefinition() == G4OpticalPhoton::Definition()) {
    if (gPhotonFraction <= 0.) {
      tieni = false;
    } else if (gPhotonFraction < 1.) {
      const long ogni = std::max(1L, std::lround(1.0 / gPhotonFraction));
      tieni = (track->GetTrackID() % ogni == 0);
    }
  }
  fpTrackingManager->SetStoreTrajectory(tieni ? fStore : 0);
  fSpento = (!tieni && fStore != 0);
}
