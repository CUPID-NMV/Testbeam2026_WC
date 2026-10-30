#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <set>

class EventAction : public G4UserEventAction {
public:
  EventAction();
  virtual ~EventAction();

  virtual void BeginOfEventAction(const G4Event*);
  virtual void EndOfEventAction(const G4Event*);

  void AddEdep(G4double edep)        { fEdep += edep; }
  void AddTrackLength(G4double L)    { fTrackLength += L; }
  void AddCerenkovProd()             { fNCerenkovProd++; }
  void AddCerenkovEntrati()          { fNCerenkovEntrati++; }
  void SetInitialEnergy(G4double e)  { fEinit = e; }
  // energia nel nucleo delle fibre: 0 = verso il PMT L (x<0), 1 = tratto
  // dritto, 2 = verso il PMT R (x>0)
  void AddEdepFiber(G4int r, G4double e) { if (r >= 0 && r < 3) fEdepFiber[r] += e; }
  void AddCerenkovByProcess(const G4String& proc, G4bool isPrimary);

  // pmtID: 0=L, 1=R; 2-5 = gli altri PMT di triple, che restano solo in "Fotoni"
  void AddPMTHit(G4int id) { if (id == 0 || id == 1) fPMTHits[id]++; }
  void RegisterPMTHit(G4double time);

  // Pannelli (geometria single_pan3), k = 0 alto, 1 mezzo, 2 basso. Le
  // grandezze sono quelle del MC della collega, una copia per pannello.
  void AddEdepPanel(G4int k, G4double e)   { if (Ok(k)) fEdepPanel[k] += e; }
  void AddTrackLPanel(G4int k, G4double L) { if (Ok(k)) fTrackLPanel[k] += L; }
  void AddScintPanel(G4int k)              { if (Ok(k)) fNScint[k]++; }
  void AddPhotonInFiber(G4int k)           { if (Ok(k)) fPhotonsInFiber[k]++; }
  void AddSiPMPhoton(G4int k, G4bool detected, G4double time);

  // Tipo di sorgente: 0=e- (testbeam), 1=gamma (fondo), 2=mu- (cosmico)
  void SetSourceType(G4int t) { fSourceType = t; }

  G4bool IsPhotonUnique(G4int tid) {
    if (fCountedPhotons.find(tid) == fCountedPhotons.end()) {
      fCountedPhotons.insert(tid);
      return true;
    }
    return false;
  }
  // primo ingresso di un fotone nel nucleo di una fibra dei pannelli
  G4bool IsPanelPhotonUnique(G4int tid) { return fCountedPanelPhotons.insert(tid).second; }

private:
  static G4bool Ok(G4int k) { return k >= 0 && k < 3; }

  G4double fEinit, fEdep, fTrackLength, fFirstHitTime;
  G4double fEdepFiber[3];
  G4int    fNCerenkovProd;
  G4int    fNCerenkovEntrati;
  G4int    fPMTHits[2];
  G4int    fSourceType;
  G4int    fNCer_primary;   // da particella primaria (TrackID==1)
  G4int    fNCer_compton;   // da elettroni Compton o fotoelettrici
  G4int    fNCer_pair;      // da coppie e+/e- (pair production)
  G4int    fNCer_deltaEM;   // da delta-ray (ionizzazione secondaria EM)
  G4int    fNCer_nuclear;   // da prodotti di interazioni nucleari
  std::set<G4int> fCountedPhotons;

  // pannelli
  G4double fEdepPanel[3];
  G4double fTrackLPanel[3];
  G4int    fNScint[3];
  G4int    fPhotonsInFiber[3];
  G4int    fSiPMTotal[3];      // fotoni arrivati al SiPM
  G4int    fSiPMDetected[3];   // di cui rivelati (PDE)
  G4double fFirstSiPM[3];      // tempo del primo rivelato
  std::set<G4int> fCountedPanelPhotons;
};

#endif
