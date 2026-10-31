#ifndef FiberWLS_h
#define FiberWLS_h 1

class G4MaterialPropertiesTable;

// Spettro di emissione del WLS delle fibre BCF-9995XL (WLSCOMPONENT).
//
// Digitalizzato dal grafico di Luxium (brochure "Plastic Scintillating and
// Wavelength-Shifting Fibers", 2023, pag. 4), da 385 a 580 nm a passo 5 nm:
// picco a 435 nm, meta' altezza 413-471 nm. La tabella della stessa brochure
// da' pero' il picco a 450 nm; spostato di 15 nm, lo spettro darebbe il 9% di
// fotoelettroni in meno con l'H14603-103.
// Fino al 2026-10-31 c'erano 4 punti fra 2.07 e 3.14 eV: Geant4 emette con
// densita' costante fra due punti, e lo spettro ai PMT veniva a gradini.
namespace FiberWLS {
  // Aggiunge WLSCOMPONENT al nucleo. Da chiamare dove si costruisce il materiale.
  void AddEmission(G4MaterialPropertiesTable* mpt);
}

#endif
