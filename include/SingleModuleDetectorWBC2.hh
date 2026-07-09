#ifndef SingleModuleDetectorWBC2_h
#define SingleModuleDetectorWBC2_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

// WBC2: come single_wbc (pettini a x=330mm) ma con profilo XZ
// retta + quarto di cerchio R=45mm al posto della Bezier quadratica.
// Raggio minimo di curvatura: fibra centrale 45mm (era 30.4mm),
// fibra peggiore 17.9mm (era 9.3mm). Morphing y con smoothstep sulla
// frazione di lunghezza d'arco (stessa struttura collision-safe).
// Il numero di fibre e' parametrico: 37 (anelli esagonali 0-3, default),
// 19 (anelli 0-2) o 7 (anelli 0-1). Anche il passo del pettine e'
// parametrico (default 8 mm): con 19 fibre a passo 16 mm si mantiene
// la copertura ±144 mm della 37f con meta' del materiale WLS.
class SingleModuleDetectorWBC2 : public G4VUserDetectorConstruction
{
public:
  explicit SingleModuleDetectorWBC2(G4int nFibers = 37, G4double combPitchMM = 8.0);
  virtual ~SingleModuleDetectorWBC2();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();

private:
  G4int    fNFib;
  G4double fCombPitchMM;
};

#endif
