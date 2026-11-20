#include "PanelOnlyDetector.hh"
#include "ScintPanel.hh"

#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4SystemOfUnits.hh"

PanelOnlyDetector::PanelOnlyDetector(G4int nPannelli)
: G4VUserDetectorConstruction(), fNPannelli(nPannelli) {}

PanelOnlyDetector::~PanelOnlyDetector() {}

G4VPhysicalVolume* PanelOnlyDetector::Construct() {
  G4Material* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");

  // Mondo come in SingleModuleDetector: deve contenere il piano dei muoni
  // (x -1647..1244 mm, z = 100 mm) e la scatola dei gamma dei pannelli.
  // L'aria del mondo non ha proprieta' ottiche: un fotone che esce
  // dall'involucro di un pannello muore comunque sul suo bordo (ScintPanel::Place).
  G4Box* solidWorld = new G4Box("World", 9.*m, 3.5*m, 3.5*m);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
  logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0);

  // Posizione dei pannelli: la stessa di single_pan3, calcolata con gli
  // stessi numeri di SingleModuleDetector.cc (blocco "PANNELLI SCINTILLATORI
  // SOTTO IL SERBATOIO"). Il serbatoio non c'e', ma le sue semidimensioni
  // servono come riferimento delle misure fatte sul montaggio (2026-10-30).
  // Se cambia la posizione in SingleModuleDetector.cc, va cambiata anche qui.
  const G4double tankX = 490.0*mm, tankY = 190.0*mm, tankZ = 60.0*mm;
  const G4double dalLatoSenzaSiPM = 130.0*mm;
  const G4double dalBordoDestro   = 100.0*mm;
  const G4double sopraIlPannello  =  10.0*mm;
  const G4ThreeVector lo = ScintPanel::kHousingLo, hi = ScintPanel::kHousingHi;
  const G4double xc = tankX + dalLatoSenzaSiPM - hi.x();
  const G4double yc = -tankY - dalBordoDestro - lo.y();
  const G4double zc = -tankZ - sopraIlPannello - hi.z();
  const G4double passo = hi.z() - lo.z();   // housing a contatto: 63.5 mm

  // Copy number: 0 = il pannello piu' alto (in pan1 l'unico), 2 = il piu' basso
  for (G4int k = 0; k < fNPannelli; k++)
    ScintPanel::Get().Place(physWorld, G4ThreeVector(xc, yc, zc - k*passo), k);

  return physWorld;
}

void PanelOnlyDetector::ConstructSDandField() {
  // Niente PMT: i SiPM dei pannelli non hanno SD, li conta SteppingAction
}
