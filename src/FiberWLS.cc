#include "FiberWLS.hh"

#include "G4MaterialPropertiesTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

void FiberWLS::AddAbsorption(G4MaterialPropertiesTable* mpt) {
  // Ampiezza del grafico del datasheet (massimo 1), da 280 a 400 nm ogni 5 nm.
  const G4double lambda0 = 280.0, passo = 5.0;   // nm
  const G4double ampiezza[] = {
    0.141, 0.152, 0.171, 0.204, 0.256, 0.328, 0.403, 0.485, 0.603, 0.730,   // 280-325 nm
    0.838, 0.928, 0.984, 1.000, 0.974, 0.916, 0.806, 0.684, 0.551, 0.367,   // 330-375 nm
    0.220, 0.123, 0.057, 0.014, 0.008};                                     // 380-400 nm
  const G4int n = 25;
  // Lunghezza = picco / ampiezza: 0.07 mm a 280 nm, 1.2 mm a 400 nm, dove la
  // fibra (0.94 mm) assorbe ancora meta' della luce. Sotto 280 nm Geant4 tiene
  // il valore di 280 nm.
  const G4double picco = 0.01 * mm;
  // Sopra 400 nm il colorante non assorbe (1000 km). Deve restare cosi' fino
  // al minimo dello spettro di emissione (580 nm): Geant4 11.4 si ferma
  // (G4OpWLS, WSL01, eccezione fatale) quando un fotone assorbito non puo'
  // essere riemesso a energia piu' bassa. Energie in ordine crescente.
  G4double energia[n + 2], lunghezza[n + 2];
  energia[0] = 1.5 * eV;
  energia[1] = h_Planck * c_light / ((lambda0 + passo * n) * nm);   // 405 nm
  lunghezza[0] = lunghezza[1] = 1000. * km;
  for (G4int i = 0; i < n; ++i) {
    const G4int k = n - 1 - i;
    energia[i + 2] = h_Planck * c_light / ((lambda0 + passo * k) * nm);
    lunghezza[i + 2] = picco / ampiezza[k];
  }
  mpt->AddProperty("WLSABSLENGTH", energia, lunghezza, n + 2);
}

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
  // Tempo di decadimento misurato: 2.41 +- 0.03 ns (Yamazumi et al., PTEP
  // 2026, arXiv 2601.06784). Il datasheet dice 2.7 ns, usati fino al 2026-11-01.
  // Esponenziale dal 2026-11-03 (PhysicsList.cc); prima era un ritardo fisso.
  mpt->AddConstProperty("WLSTIMECONSTANT", 2.41 * ns);
}
