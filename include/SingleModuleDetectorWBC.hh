#ifndef SingleModuleDetectorWBC_h
#define SingleModuleDetectorWBC_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

// Wide-Bend-Curve variant: combs at x=330mm (vs 380mm), z_inizio_bundle=45mm (vs 35mm).
// Wider Bezier arc (dX=102mm vs 52mm) + greater Z travel → softer fiber bending.
class SingleModuleDetectorWBC : public G4VUserDetectorConstruction
{
public:
  SingleModuleDetectorWBC();
  virtual ~SingleModuleDetectorWBC();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();
};

#endif
