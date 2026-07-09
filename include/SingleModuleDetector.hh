#ifndef SingleModuleDetector_h
#define SingleModuleDetector_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class SingleModuleDetector : public G4VUserDetectorConstruction
{
public:
  SingleModuleDetector();
  virtual ~SingleModuleDetector();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();
};

#endif
