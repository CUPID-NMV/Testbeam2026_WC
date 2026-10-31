#ifndef FiberWLS_h
#define FiberWLS_h 1

class G4MaterialPropertiesTable;

// Il WLS delle fibre BCF-9995XL: assorbimento ed emissione dal grafico di
// Luxium (brochure "Plastic Scintillating and Wavelength-Shifting Fibers",
// 2023, pag. 4), digitalizzati a passo 5 nm.
//
// Emissione (WLSCOMPONENT), da 385 a 580 nm: picco a 435 nm, meta' altezza
// 413-471 nm. La tabella della stessa brochure da' pero' il picco a 450 nm;
// spostato di 15 nm, lo spettro darebbe il 9% di fotoelettroni in meno con
// l'H14603-103. Fino al 2026-10-31 c'erano 4 punti fra 2.07 e 3.14 eV: Geant4
// emette con densita' costante fra due punti, e lo spettro ai PMT veniva a
// gradini.
//
// Assorbimento (WLSABSLENGTH), da 280 a 400 nm, picco a 345 nm. Il datasheet
// da' solo la forma: la lunghezza al picco, 0.01 mm, e' quella che c'era.
// Fino al 2026-10-31 l'assorbimento finiva a ~387 nm, e fra 400 e 500 nm
// restavano 2-10 m che il datasheet non ha.
namespace FiberWLS {
  // Aggiungono WLSABSLENGTH e WLSCOMPONENT al nucleo. Da chiamare dove si
  // costruisce il materiale.
  void AddAbsorption(G4MaterialPropertiesTable* mpt);
  void AddEmission(G4MaterialPropertiesTable* mpt);
}

#endif
