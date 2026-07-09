#ifndef TripleModuleDetector_h
#define TripleModuleDetector_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class TripleModuleDetector : public G4VUserDetectorConstruction
{
public:
  TripleModuleDetector();
  virtual ~TripleModuleDetector();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();
};

#endif
