#include "FiberScintillation.hh"

#include "G4GenericMessenger.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

FiberScintillation& FiberScintillation::Get() {
  static FiberScintillation istanza;
  return istanza;
}

FiberScintillation::FiberScintillation()
  : fYield(1750.), fBirks(0.126), fEndX(380.0), fMessenger(nullptr) {
  fMessenger = new G4GenericMessenger(this, "/wc/fiber/",
                                      "scintillazione del nucleo delle fibre (polistirene)");
  fMessenger->DeclareProperty("scintYield", fYield,
      "fotoni per MeV depositato nel nucleo delle fibre (0 = spenta). Prima di /run/initialize.");
  fMessenger->DeclareProperty("birks", fBirks,
      "costante di Birks del polistirene [mm/MeV]. Prima di /run/initialize.");
}

void FiberScintillation::Apply(G4Material* core, G4MaterialPropertiesTable* mpt) const {
  // Birks: la luce di un elettrone lento (alta perdita di energia per mm) e'
  // meno che proporzionale all'energia depositata. 0.126 mm/MeV e' il valore
  // usuale per gli scintillatori a base di polistirene.
  core->GetIonisation()->SetBirksConstant(fBirks * mm / MeV);
  if (fYield <= 0) {
    G4cout << "[fibre] scintillazione del nucleo SPENTA (/wc/fiber/scintYield 0)" << G4endl;
    return;
  }
  // Emissione del polistirene non drogato: picco a 324 nm (3.83 eV), da
  // ~290 a ~370 nm. Tutta dentro la banda d'assorbimento del colorante, che
  // la converte in blu come fa col Cherenkov ultravioletto.
  G4double e[] = {3.10*eV, 3.35*eV, 3.54*eV, 3.70*eV, 3.83*eV, 3.96*eV, 4.13*eV, 4.28*eV, 4.42*eV};
  G4double s[] = {0.00,    0.05,    0.35,    0.75,    1.00,    0.75,    0.30,    0.05,    0.00};
  const G4int n = 9;
  mpt->AddProperty("SCINTILLATIONCOMPONENT1", e, s, n);
  mpt->AddProperty("SCINTILLATIONCOMPONENT2", e, s, n);
  mpt->AddProperty("SCINTILLATIONCOMPONENT3", e, s, n);
  mpt->AddConstProperty("SCINTILLATIONYIELD", fYield / MeV);
  mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.26 * ns);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT2", 11.81 * ns);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT3", 24.76 * ns);
  mpt->AddConstProperty("SCINTILLATIONYIELD1", 0.60);
  mpt->AddConstProperty("SCINTILLATIONYIELD2", 0.23);
  mpt->AddConstProperty("SCINTILLATIONYIELD3", 0.17);
  G4cout << "[fibre] scintillazione del nucleo: " << fYield << " fotoni/MeV, Birks "
         << fBirks << " mm/MeV, tempi 2.26 / 11.8 / 24.8 ns (60 / 23 / 17%)" << G4endl;
}
