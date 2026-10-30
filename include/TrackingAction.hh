#ifndef TrackingAction_h
#define TrackingAction_h 1

#include "G4UserTrackingAction.hh"
#include "globals.hh"

// Solo per la visualizzazione: salva la traiettoria di una parte dei fotoni
// ottici. Con l'ottica dei pannelli accesa un muone ne fa ~10^5 e la GUI li
// disegna tutti (~5 GB). Si regola da macro, anche a GUI aperta:
//   /wc/vis/photonFraction 0.01   uno ogni 100, scelti dal numero di traccia
//   /wc/vis/photonFraction 1      tutti (default, come prima)
// Non tocca la fisica: decide solo quali traiettorie si salvano, senza usare
// numeri casuali.
class TrackingAction : public G4UserTrackingAction {
public:
  TrackingAction();
  virtual ~TrackingAction() {}

  virtual void PreUserTrackingAction(const G4Track*);

  // Crea /wc/vis/: in main, prima che girino le macro
  static void CreateCommands();

private:
  G4int  fStore;     // il /tracking/storeTrajectory chiesto dalla visualizzazione
  G4bool fSpento;    // messo a 0 da qui, per il fotone precedente
};

#endif
