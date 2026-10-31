#include "FiberWLS.hh"

#include "G4MaterialPropertiesTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

void FiberWLS::AddEmission(G4MaterialPropertiesTable* mpt) {
  // Ampiezza del grafico del datasheet (massimo 1), da 385 a 580 nm ogni 5 nm.
  const G4double lambda0 = 385.0, passo = 5.0;   // nm
  const G4double ampiezza[] = {
    0.000, 0.011, 0.048, 0.122, 0.239, 0.398, 0.582, 0.775, 0.904, 0.969,   // 385-430 nm
    1.000, 0.979, 0.927, 0.859, 0.784, 0.705, 0.613, 0.520, 0.458, 0.414,   // 435-480 nm
    0.362, 0.314, 0.273, 0.243, 0.217, 0.189, 0.159, 0.132, 0.112, 0.095,   // 485-530 nm
    0.079, 0.065, 0.054, 0.042, 0.031, 0.021, 0.014, 0.008, 0.005, 0.000};  // 535-580 nm
  const G4int n = 40;
  // Il grafico e' per unita' di lunghezza d'onda, Geant4 campiona in energia:
  // dN/dE = dN/dlambda * lambda^2 / hc. Senza il lambda^2 lo spettro simulato
  // viene piu' blu di quello del datasheet (e la QE efficace dell'H14603-103
  // 3% piu' alta). Le energie vanno in ordine crescente: lambda decrescente.
  G4double energia[n], valore[n];
  for (G4int i = 0; i < n; ++i) {
    const G4int k = n - 1 - i;
    const G4double lambda = lambda0 + passo * k;   // nm
    energia[i] = h_Planck * c_light / (lambda * nm);
    valore[i] = ampiezza[k] * (lambda / 435.0) * (lambda / 435.0);
  }
  mpt->AddProperty("WLSCOMPONENT", energia, valore, n);
}
