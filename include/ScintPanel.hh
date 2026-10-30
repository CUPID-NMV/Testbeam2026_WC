#ifndef ScintPanel_h
#define ScintPanel_h 1

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4VTouchable.hh"   // in Geant4 11.4 e' un alias: niente dichiarazione anticipata

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;
class G4OpticalSurface;
class G4GenericMessenger;

// Un pannello di scintillatore del montaggio al LNGS, portato dal MC della
// collega di Yale (DetectorConstructionCAD.cc + DefineMaterials() di
// DetectorConstruction.cc, zip del 2026-10-30), layout "Nested", wrapping
// Tyvek: EJ-200 1250 x 550 x 50 mm, quattro fibre WLS a U nei groove 2 x 5 mm
// riempiti di EJ-500, 3 x Tyvek + HDPE, housing di alluminio con le schiume
// di neoprene, bundle 1+7 degli 8 capi su un SiPM S13360-6075PE.
//
// Differenze dal suo codice, tutte d'integrazione (la geometria e le
// proprieta' ottiche sono le sue):
//  - tutto e' costruito dentro un involucro d'aria, piazzato tre volte, e
//    non nel mondo;
//  - materiali propri, con prefisso "Pan_": lei imposta le tabelle ottiche
//    su G4_POLYSTYRENE e G4_AIR, che qui sono il core delle fibre del WC e
//    l'aria del mondo, e un materiale NIST ha una tabella sola;
//  - l'ottica si accende da macro (sotto); spenta, i materiali non hanno
//    tabelle ottiche e nei pannelli non nasce nessun fotone.
//
// Sistema di riferimento: origine al centro dello scintillatore, x lungo il
// lato lungo con il SiPM verso -x, z in alto (come il CAD della collega).
//
// Da macro, PRIMA di /run/initialize:
//   /wc/panel/optics true     scintillazione, WLS e SiPM (lento: ~10^5
//                             fotoni per muone per pannello)
//   /wc/panel/optics false    (default) solo energia depositata
class ScintPanel {
public:
  static ScintPanel& Get();

  // Piazza un pannello nel mondo con il centro dello scintillatore in
  // 'centre' e copy number 'copy'. Il pannello e' costruito alla prima
  // chiamata e poi riusato.
  void Place(G4VPhysicalVolume* world, const G4ThreeVector& centre, G4int copy);

  // Copy number del pannello che contiene il punto, -1 se fuori dai pannelli.
  G4int PanelIndex(const G4VTouchable* touchable) const;

  G4bool Optics() const { return fOptics; }

  // Per SteppingAction: il nucleo delle fibre e il SiPM dei pannelli
  // (nullptr finche' i pannelli non sono costruiti, o se non ci sono)
  const G4Material* FiberCore() const { return fFiberCore; }
  const G4LogicalVolume* SiPM() const { return fSiPM; }

  // Ingombro dell'housing (compresa la scatola del SiPM, che sporge a -x)
  // rispetto al centro dello scintillatore: e' anche l'involucro d'aria.
  static const G4ThreeVector kHousingLo, kHousingHi;

private:
  ScintPanel();
  G4LogicalVolume* Build();

  G4bool fOptics;
  G4LogicalVolume* fEnvelope;
  const G4Material* fFiberCore;
  const G4LogicalVolume* fSiPM;
  G4OpticalSurface* fOpBlack;
  G4GenericMessenger* fMessenger;
};

#endif
