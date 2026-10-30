#ifndef FiberScintillation_h
#define FiberScintillation_h 1

#include "globals.hh"

class G4Material;
class G4MaterialPropertiesTable;
class G4GenericMessenger;

// Scintillazione del nucleo delle fibre BCF-9995XL (polistirene).
//
// Il polistirene scintilla da solo: ~1750 fotoni/MeV a 324 nm, con tempi
// 2.26 ns (60%), 11.8 ns (23%), 24.8 ns (17%) (polistirene non drogato,
// arXiv 2605.12861; un altro lavoro da' ~560 fotoni/MeV, arXiv 2210.10083).
// Il colorante della BCF-9995XL assorbe proprio li' (280-390 nm, picco
// 344 nm) e riemette in blu: la fibra e' una fibra scintillante debole.
// Misurato su Kuraray Y11: 23% di una BCF-12, ~1200 fotoni/MeV, effetto
// "tradizionalmente non considerato nelle simulazioni" (Alekseev et al.,
// arXiv 2604.14060). Al LNGS gli impulsi ad alta carica del prototipo hanno
// proprio la coda lenta di questa luce (daq-standalone,
// analisi/20261029_scint_fibre, 2026-10-29). Fino al 2026-10-29 il MC aveva
// la scintillazione spenta.
//
// Si regola da macro, PRIMA di /run/initialize:
//   /wc/fiber/scintYield 1750     fotoni per MeV (0 = spenta, come prima)
//   /wc/fiber/birks 0.126         costante di Birks del polistirene [mm/MeV]
class FiberScintillation {
public:
  static FiberScintillation& Get();

  // Aggiunge al nucleo le proprieta' di scintillazione (se la resa e' > 0)
  // e la costante di Birks. Da chiamare dove si costruisce il materiale.
  void Apply(G4Material* core, G4MaterialPropertiesTable* mpt) const;

  // |x| oltre cui la fibra e' "verso il PMT" (curve, fascio): la posizione
  // del pettine. Serve solo a dividere l'energia depositata nelle fibre.
  void SetEndX(G4double x) { fEndX = x; }
  G4double EndX() const { return fEndX; }
  G4double Yield() const { return fYield; }     // fotoni per MeV

private:
  FiberScintillation();
  G4double fYield;      // fotoni per MeV
  G4double fBirks;      // mm/MeV
  G4double fEndX;       // mm
  G4GenericMessenger* fMessenger;
};

#endif
