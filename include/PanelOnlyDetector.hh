#ifndef PanelOnlyDetector_h
#define PanelOnlyDetector_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

// Solo i pannelli di scintillatore del LNGS (ScintPanel), senza il WC.
// Geometrie "pan1" (un pannello) e "pan3" (tre pannelli impilati).
// I pannelli stanno esattamente dove sono in "single_pan3": stesso sistema di
// riferimento (origine al centro dell'acqua del WC, che qui non c'e'), cosi'
// le macro di muoni e gamma di single_pan3 valgono senza modifiche e i
// risultati si confrontano direttamente. pan1 e' il pannello piu' alto
// (copy number 0, colonne "top" in uscita).
class PanelOnlyDetector : public G4VUserDetectorConstruction
{
public:
  explicit PanelOnlyDetector(G4int nPannelli);   // 1 oppure 3
  virtual ~PanelOnlyDetector();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();

private:
  G4int fNPannelli;
};

#endif
