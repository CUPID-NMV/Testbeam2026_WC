#ifndef SingleModuleDetectorWBC3_h
#define SingleModuleDetectorWBC3_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

// WBC3: due layer di fibre (23 a z=0 con curva R=45mm, 22 a z=-45mm
// con curva R=90mm), pettini a x=330mm passo 13mm sfalsati di 6.5mm,
// bundle unico da 45 fibre su PMT diam. 8mm (r=4.0mm, H10721).
// Slot bundle partizionati in x tra i layer (validato: dist. min
// tra assi 0.98mm, zero collisioni inter-layer).
class SingleModuleDetectorWBC3 : public G4VUserDetectorConstruction
{
public:
  SingleModuleDetectorWBC3();
  virtual ~SingleModuleDetectorWBC3();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();
};

#endif
