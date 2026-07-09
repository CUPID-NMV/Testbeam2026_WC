#ifndef PMTSD_h
#define PMTSD_h 1

#include "G4VSensitiveDetector.hh"
#include "globals.hh"

class G4Step;
class G4HCofThisEvent;

class PMTSD : public G4VSensitiveDetector {
public:
  PMTSD(G4String name);
  virtual ~PMTSD();

  virtual void Initialize(G4HCofThisEvent*);
  virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*);
  virtual void EndOfEvent(G4HCofThisEvent*);
};

#endif
