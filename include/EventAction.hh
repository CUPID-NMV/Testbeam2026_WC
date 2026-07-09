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
  void AddCerenkovByProcess(const G4String& proc, G4bool isPrimary);

  // pmtID: 0=L_Bot, 1=R_Bot, 2=L_Mid, 3=R_Mid, 4=L_Top, 5=R_Top
  void AddPMTHit(G4int id) { if (id >= 0 && id < 6) fPMTHits[id]++; }
  void RegisterPMTHit(G4double time);

  // Tipo di sorgente: 0=e- (testbeam), 1=gamma (fondo), 2=mu- (cosmico)
  void SetSourceType(G4int t) { fSourceType = t; }

  G4bool IsPhotonUnique(G4int tid) {
    if (fCountedPhotons.find(tid) == fCountedPhotons.end()) {
      fCountedPhotons.insert(tid);
      return true;
    }
    return false;
  }

private:
  G4double fEinit, fEdep, fTrackLength, fFirstHitTime;
  G4int    fNCerenkovProd;
  G4int    fNCerenkovEntrati;
  G4int    fPMTHits[6];
  G4int    fSourceType;
  G4int    fNCer_primary;   // da particella primaria (TrackID==1)
  G4int    fNCer_compton;   // da elettroni Compton o fotoelettrici
  G4int    fNCer_pair;      // da coppie e+/e- (pair production)
  G4int    fNCer_deltaEM;   // da delta-ray (ionizzazione secondaria EM)
  G4int    fNCer_nuclear;   // da prodotti di interazioni nucleari
  std::set<G4int> fCountedPhotons;
};

#endif
