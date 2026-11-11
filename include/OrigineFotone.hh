#ifndef OrigineFotone_h
#define OrigineFotone_h 1

#include "G4VUserTrackInformation.hh"
#include "globals.hh"

// Da dove viene un fotone ottico (2026-11-10). Ogni fotone ottico riceve alla
// nascita l'origine della sua catena; i fotoni riemessi dal WLS la ereditano
// dal fotone assorbito. Per la scintillazione del nucleo delle fibre del WC
// porta anche il ritardo di emissione: il tempo del fotone meno quello del
// passo che l'ha prodotto (decadimento della scintillazione, piu' la frazione
// del passo, ~ps).
//
// Serve a digit_daq.py per riscalare resa e tempi della scintillazione delle
// fibre senza rifare Geant4: si tiene ogni fotone di scintillazione con
// probabilita' S (resa S volte quella della produzione) e se ne ritira il
// ritardo da un altro profilo.
//
// Si accende con /wc/output/photonOrigin true (di serie spento: nell'ntupla
// "Fotoni" Origine = -1 e Ritardo_scint_ns = 0). Non usa numeri casuali:
// acceso o spento, la fisica e i fotoni sono gli stessi.
class OrigineFotone : public G4VUserTrackInformation {
public:
  enum Codice {
    Ignota         = -1,
    CherenkovAcqua =  0,   // Cherenkov nell'acqua (WaterPhys)
    ScintFibra     =  1,   // scintillazione nel nucleo delle fibre del WC ("core")
    CherenkovFibra =  2,   // Cherenkov nel nucleo delle fibre del WC
    Altro          =  3    // tutto il resto (rivestimenti, HDPE, pannelli, ...)
  };

  OrigineFotone(G4int origine, G4double ritardo)
    : G4VUserTrackInformation("OrigineFotone"), fOrigine(origine), fRitardo(ritardo) {}
  ~OrigineFotone() override = default;

  G4int    Origine() const { return fOrigine; }
  G4double Ritardo() const { return fRitardo; }   // unita' interne di Geant4

  // /wc/output/photonOrigin (definito in EventAction.cc, con /wc/output/onlyActive)
  static G4bool Attiva();

private:
  G4int    fOrigine;
  G4double fRitardo;
};

#endif
