#ifndef SingleModuleDetector_h
#define SingleModuleDetector_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class SingleModuleDetector : public G4VUserDetectorConstruction
{
public:
  // pannelli = true: sotto il serbatoio i tre scintillatori del montaggio al
  // LNGS (geometria "single_pan"); di serie non ci sono
  SingleModuleDetector(G4bool pannelli = false);
  virtual ~SingleModuleDetector();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();

private:
  G4bool fPannelli;
};

#endif
