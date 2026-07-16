#include "SingleModuleDetectorWBC3.hh"
#include "PMTSD.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4Tubs.hh"
#include "G4Torus.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "G4SubtractionSolid.hh"
#include "G4SDManager.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4VisAttributes.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4GeometryManager.hh"

#include <algorithm>
#include <cmath>
#include <vector>

SingleModuleDetectorWBC3::SingleModuleDetectorWBC3(G4double tankHalfLenX)
: G4VUserDetectorConstruction(), fTankHalfX(tankHalfLenX) {}

SingleModuleDetectorWBC3::~SingleModuleDetectorWBC3() {}

G4VPhysicalVolume* SingleModuleDetectorWBC3::Construct() {
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* air    = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* hdpe   = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
  G4Material* water  = nist->FindOrBuildMaterial("G4_WATER");
  G4Material* glass  = nist->FindOrBuildMaterial("G4_GLASS_PLATE");
  G4Material* coreMat = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
  G4Material* cladMat = nist->FindOrBuildMaterial("G4_PLEXIGLASS");

  // =======================================================
  // --- PROPRIETA' OTTICHE ---
  // =======================================================
  G4double energy[] = {2.0*eV, 5.0*eV};
  const G4int nEntries = 2;

  G4double rIndexAir[] = {1.00, 1.00};
  G4MaterialPropertiesTable* mptAir = new G4MaterialPropertiesTable();
  mptAir->AddProperty("RINDEX", energy, rIndexAir, nEntries);
  air->SetMaterialPropertiesTable(mptAir);

  G4double energyWater[] = {1.77*eV, 2.07*eV, 2.48*eV, 3.10*eV, 4.13*eV};
  const G4int nWater = 5;
  G4double rIndexWater[] = {1.331, 1.333, 1.338, 1.344, 1.357};
  G4double absWater[]    = {12.0*m, 10.0*m, 8.0*m, 3.0*m, 0.5*m};
  G4double rayleighWater[] = {5.0*m, 3.0*m, 1.5*m, 0.5*m, 0.1*m};
  G4MaterialPropertiesTable* mptWater = new G4MaterialPropertiesTable();
  mptWater->AddProperty("RINDEX",    energyWater, rIndexWater,   nWater);
  mptWater->AddProperty("ABSLENGTH", energyWater, absWater,      nWater);
  mptWater->AddProperty("RAYLEIGH",  energyWater, rayleighWater, nWater);
  water->SetMaterialPropertiesTable(mptWater);

  G4double rIndexGlass[] = {1.50, 1.50};
  G4double absGlass[]    = {0.1*mm, 0.1*mm};
  G4MaterialPropertiesTable* mptGlass = new G4MaterialPropertiesTable();
  mptGlass->AddProperty("RINDEX", energy, rIndexGlass, nEntries);
  mptGlass->AddProperty("ABSLENGTH", energy, absGlass, nEntries);
  glass->SetMaterialPropertiesTable(mptGlass);

  G4double rIndexHDPE[] = {1.50, 1.50};
  G4double absHDPE[]    = {0.001*mm, 0.001*mm};
  G4MaterialPropertiesTable* mptHDPE = new G4MaterialPropertiesTable();
  mptHDPE->AddProperty("RINDEX", energy, rIndexHDPE, nEntries);
  mptHDPE->AddProperty("ABSLENGTH", energy, absHDPE, nEntries);
  hdpe->SetMaterialPropertiesTable(mptHDPE);

  G4OpticalSurface* hdpeSurface = new G4OpticalSurface("HDPESurface");
  hdpeSurface->SetType(dielectric_metal);
  hdpeSurface->SetModel(unified);
  hdpeSurface->SetFinish(ground);
  G4MaterialPropertiesTable* mptHDPESurf = new G4MaterialPropertiesTable();
  G4double hdpeRefl[] = {0.90, 0.90};
  mptHDPESurf->AddProperty("REFLECTIVITY", energy, hdpeRefl, nEntries);
  hdpeSurface->SetMaterialPropertiesTable(mptHDPESurf);

  G4OpticalSurface* teflonSurface = new G4OpticalSurface("TeflonSurface");
  teflonSurface->SetType(dielectric_metal);
  teflonSurface->SetModel(unified);
  teflonSurface->SetFinish(groundfrontpainted);
  G4MaterialPropertiesTable* mptTeflon = new G4MaterialPropertiesTable();
  G4double energyTeflon[] = {2.755*eV, 3.099*eV, 4.132*eV, 4.959*eV};
  const G4int nTeflon = 4;
  G4double teflonRefl[] = {0.97, 0.96, 0.935, 0.91};
  mptTeflon->AddProperty("REFLECTIVITY", energyTeflon, teflonRefl, nTeflon);
  teflonSurface->SetMaterialPropertiesTable(mptTeflon);

  G4double rIndexClad[] = {1.49, 1.49};
  G4double absClad[]    = {10.0*m, 10.0*m};
  G4MaterialPropertiesTable* mptClad = new G4MaterialPropertiesTable();
  mptClad->AddProperty("RINDEX", energy, rIndexClad, nEntries);
  mptClad->AddProperty("ABSLENGTH", energy, absClad, nEntries);
  cladMat->SetMaterialPropertiesTable(mptClad);

  G4double energyWLS[] = {2.066*eV, 2.480*eV, 2.818*eV, 3.139*eV, 3.220*eV, 3.542*eV, 4.133*eV, 4.428*eV};
  const G4int nWLS = 8;
  G4double rIndexCore[] = {1.60, 1.60, 1.60, 1.60, 1.60, 1.60, 1.60, 1.60};
  G4double absCore[]    = {2.5*m, 2.5*m, 2.5*m, 2.5*m, 2.5*m, 2.5*m, 2.5*m, 2.5*m};
  G4double wlsAbsCore[] = {10.0*m, 10.0*m, 10.0*m, 1.0*m, 0.3*mm, 0.01*mm, 0.01*mm, 0.01*mm};
  G4double wlsEmitCore[] = {0.0, 0.25, 1.0, 0.1, 0.0, 0.0, 0.0, 0.0};
  G4MaterialPropertiesTable* mptCore = new G4MaterialPropertiesTable();
  mptCore->AddProperty("RINDEX", energyWLS, rIndexCore, nWLS);
  mptCore->AddProperty("ABSLENGTH", energyWLS, absCore, nWLS);
  mptCore->AddProperty("WLSABSLENGTH", energyWLS, wlsAbsCore, nWLS);
  mptCore->AddProperty("WLSCOMPONENT", energyWLS, wlsEmitCore, nWLS);
  mptCore->AddConstProperty("WLSTIMECONSTANT", 2.7*ns);
  mptCore->AddConstProperty("WLSMEANNUMBERPHOTONS", 0.75);
  coreMat->SetMaterialPropertiesTable(mptCore);

  // =======================================================
  // --- MONDO E TANK ---
  // =======================================================
  G4Box* solidWorld = new G4Box("World", 9.*m, 3.5*m, 3.5*m);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0);

  // Lunghezza (X) parametrica; larghezza (Y) e profondita' (Z) come il prototipo.
  G4double tankX = fTankHalfX*mm,          tankY = 190.0*mm, tankZ = 60.0*mm;
  G4double waterX = (fTankHalfX-10.0)*mm,  waterY = 180.0*mm, waterZ = 50.0*mm;

  // =======================================================
  // --- PARAMETRI GEOMETRIA FIBRE E PMT (WBC3: 2 layer, 45 fibre) ---
  // Layer 1: 23 fibre a z=0,     curva retta + quarto cerchio R=45mm
  // Layer 2: 22 fibre a z=-45mm, curva R=90mm (piu' morbida)
  // Pettini a x=330mm per entrambi, passo 13mm, sfalsati di 6.5mm
  // (campionamento y combinato: 6.5mm su +-143mm).
  // Bundle unico da 45 su PMT r=4.0mm (diam. 8mm = H10721 reale):
  // posizioni da impacchettamento numerico (min dist 1.02mm, r<=3.48),
  // partizione in x: layer1 -> slot xb<0, layer2 -> slot xb>0
  // (evita collisioni nella zona di merge; validato: dist min 0.98mm).
  // =======================================================
  // pettine e PMT ancorati all'estremita' del tank (offset fissi): default
  // fTankHalfX=490 -> x_st1=330, X_PMT=440 (invariato); allungando il tank
  // cresce il tratto dritto di fibra in acqua (= x_pettine_exit).
  const G4double x_st1           = (fTankHalfX - 160.0) * mm;
  const G4double pettine_semiX   = 5.0 * mm;
  const G4double x_pettine_exit  = x_st1 + pettine_semiX;
  const G4double x_drittino      = 3.0 * mm;
  const G4double x_curve_start   = x_pettine_exit + x_drittino;
  const G4double X_PMT           = (fTankHalfX - 50.0) * mm;
  const G4double R_PMT           = 4.0 * mm;     // fotocatodo diam. 8 mm
  const G4double Z_PMT_center    = tankZ - 0.5*mm;
  const G4double z_inizio_bundle = 45.0 * mm;
  const G4double bundle_in_water_len = waterZ - z_inizio_bundle;
  const G4double bundle_in_hdpe_len  = (Z_PMT_center - 0.5*mm) - waterZ;
  const int N_FIB = 45;

  const G4double LAY_Z0[2] = {   0.0*mm, -45.0*mm };
  const G4double LAY_R [2] = {  45.0*mm,  90.0*mm };
  const int      N_SEG [2] = {  28, 36 };

  // tabella fibre: {layer(1/2), y_pettine[mm], xb[mm], yb[mm]}
  struct FibRow { int lay; double yc, xb, yb; };
  static const FibRow FIB[45] = {
      {1,   143.00,  -1.3369,   3.2119},
      {1,   130.00,  -0.3233,   3.1137},
      {1,   117.00,  -2.2094,   2.6874},
      {1,   104.00,  -1.2050,   2.1232},
      {1,    91.00,  -0.1864,   2.1032},
      {1,    78.00,  -2.1220,   1.6732},
      {1,    65.00,  -3.1289,   1.5211},
      {1,    52.00,  -0.7107,   1.2305},
      {1,    39.00,  -1.6254,   0.7800},
      {1,    26.00,  -2.6336,   0.6317},
      {1,    13.00,  -3.4785,   0.0603},
      {1,     0.00,   0.0266,   0.0028},
      {1,   -13.00,  -0.9926,  -0.0182},
      {1,   -26.00,  -2.0002,  -0.1664},
      {1,   -39.00,  -3.3469,  -0.9495},
      {1,   -52.00,  -0.3175,  -0.9569},
      {1,   -65.00,  -1.3368,  -0.9770},
      {1,   -78.00,  -2.3436,  -1.1263},
      {1,   -91.00,  -0.8021,  -1.8550},
      {1,  -104.00,  -1.8117,  -1.9956},
      {1,  -117.00,  -2.8308,  -2.0223},
      {1,  -130.00,  -0.7326,  -2.8721},
      {1,  -143.00,  -1.7412,  -3.0119},
      {2,   136.50,   1.6685,   3.0528},
      {2,   123.50,   0.6661,   2.8735},
      {2,   110.50,   1.8079,   2.0448},
      {2,    97.50,   2.8314,   2.0216},
      {2,    84.50,   0.8071,   1.8632},
      {2,    71.50,   2.2995,   1.1519},
      {2,    58.50,   3.3148,   1.0562},
      {2,    45.50,   0.2775,   0.9885},
      {2,    32.50,   1.2967,   0.9678},
      {2,    19.50,   2.0499,   0.1622},
      {2,     6.50,   3.4790,   0.0497},
      {2,    -6.50,   1.0454,  -0.0194},
      {2,   -19.50,   2.7086,  -0.6177},
      {2,   -32.50,   1.7042,  -0.7951},
      {2,   -45.50,   0.7030,  -0.9790},
      {2,   -58.50,   3.1136,  -1.5520},
      {2,   -71.50,   2.1114,  -1.7324},
      {2,   -84.50,   0.2153,  -1.8771},
      {2,   -97.50,   1.1907,  -2.1727},
      {2,  -110.50,   2.1309,  -2.7501},
      {2,  -123.50,   0.2871,  -2.8931},
      {2,  -136.50,   1.2416,  -3.2503},
  };

  // =======================================================
  // --- TANK CON FORI PMT ---
  // =======================================================
  G4VSolid* solidTank = new G4Box("BaseTankBox", tankX, tankY, tankZ);
  G4Tubs* subHole = new G4Tubs("SubHole", 0, R_PMT, 5.0*mm, 0, 360*deg);
  G4ThreeVector holePosR( X_PMT, 0, 55.0*mm);
  G4ThreeVector holePosL(-X_PMT, 0, 55.0*mm);
  solidTank = new G4SubtractionSolid("Tank_Hole_R", solidTank, subHole, nullptr, holePosR);
  solidTank = new G4SubtractionSolid("Tank_Hole_L", solidTank, subHole, nullptr, holePosL);

  G4LogicalVolume* logicTank = new G4LogicalVolume(solidTank, hdpe, "TankLogic");
  G4VisAttributes* tankVis = new G4VisAttributes(G4Colour(0.7, 0.7, 0.7, 0.3));
  tankVis->SetForceWireframe(true);
  logicTank->SetVisAttributes(tankVis);
  G4VPhysicalVolume* physTank = new G4PVPlacement(0, G4ThreeVector(), logicTank, "TankPhys", logicWorld, false, 0, true);
  new G4LogicalSkinSurface("TankHDPESkin", logicTank, hdpeSurface);

  G4VSolid* solidWater = new G4Box("WaterBox", waterX, waterY, waterZ);
  G4LogicalVolume* logicWater = new G4LogicalVolume(solidWater, water, "WaterLogic");
  G4VisAttributes* waterVis = new G4VisAttributes(G4Colour(0.0, 0.5, 1.0, 0.15));
  waterVis->SetForceSolid(true);
  logicWater->SetVisAttributes(waterVis);
  G4VPhysicalVolume* physWater = new G4PVPlacement(0, G4ThreeVector(), logicWater, "WaterPhys", logicTank, false, 0, true);

  new G4LogicalBorderSurface("TeflonBorder", physWater, physTank, teflonSurface);

  // =======================================================
  // --- STRUMENTI PER LE FIBRE ---
  // =======================================================
  G4double cladR = 0.5*mm, coreR = 0.47*mm, pitch = 1.05*mm;

  auto makeStraight = [&](G4double len, G4String name) -> G4LogicalVolume* {
      if (len <= 0) return nullptr;
      G4Tubs* sClad = new G4Tubs(name+"_Clad", 0, cladR, len/2.0, 0, 360*deg);
      G4Tubs* sCore = new G4Tubs(name+"_Core", 0, coreR, len/2.0, 0, 360*deg);
      G4LogicalVolume* lClad = new G4LogicalVolume(sClad, cladMat, name+"_CladLog");
      G4LogicalVolume* lCore = new G4LogicalVolume(sCore, coreMat, name+"_CoreLog");
      new G4PVPlacement(0, G4ThreeVector(), lCore, "core", lClad, false, 0, false);
      lCore->SetVisAttributes(G4VisAttributes::GetInvisible());
      return lClad;
  };

  G4VisAttributes* bundleVis = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0, 1.0));
  bundleVis->SetForceSolid(true);

  auto placeTubeBetween = [&](G4LogicalVolume* mother,
                              const G4ThreeVector& P1, const G4ThreeVector& P2,
                              const G4String& name, int copyId) {
      G4ThreeVector delta = P2 - P1;
      G4double L = delta.mag();
      if (L < 1e-6) return;
      G4ThreeVector dir = delta.unit();
      G4ThreeVector zaxis(0, 0, 1);
      G4ThreeVector axis = zaxis.cross(dir);
      G4RotationMatrix rot;
      if (axis.mag() > 1e-9) {
          G4double dot = std::max(-1.0, std::min(1.0, zaxis.dot(dir)));
          rot.rotate(std::acos(dot), axis.unit());
      }
      G4ThreeVector center = 0.5 * (P1 + P2);
      G4LogicalVolume* lTube = makeStraight(L, name);
      if (!lTube) return;
      lTube->SetVisAttributes(bundleVis);
      G4Transform3D trR(rot, center);
      new G4PVPlacement(trR, lTube, name+"_R", mother, false, copyId, false);
      G4Transform3D trMirrorL = G4RotateZ3D(180*deg);
      new G4PVPlacement(trMirrorL * trR, lTube, name+"_L", mother, false, copyId, false);
  };

  // =======================================================
  // --- SUPPORTI MECCANICI: un pettine per layer e per lato ---
  // =======================================================
  G4Tubs* sHole = new G4Tubs("FiberHole", 0.0, 0.6*mm, 6.0*mm, 0, 360*deg);
  G4RotationMatrix* rotHole = new G4RotationMatrix();
  rotHole->rotateY(90*deg);
  G4VisAttributes* suppVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 1.0));
  suppVis->SetForceSolid(true);

  for (int lay = 0; lay < 2; lay++) {
      G4double semiZ = (lay == 0) ? 12.5*mm : 2.0*mm;   // il basso e' sottile (fondo acqua a -50)
      G4Box* sBase = new G4Box("SupportBase_L"+std::to_string(lay+1),
                               pettine_semiX, 180.0*mm, semiZ);
      G4VSolid* sSup = sBase;
      int nh = 0;
      for (int f = 0; f < N_FIB; f++) {
          if (FIB[f].lay != lay+1) continue;
          sSup = new G4SubtractionSolid("SupHole_L"+std::to_string(lay+1)+"_"+std::to_string(nh++),
                                        sSup, sHole, rotHole,
                                        G4ThreeVector(0, FIB[f].yc*mm, 0));
      }
      G4LogicalVolume* lSup = new G4LogicalVolume(sSup, hdpe,
                                  "SupportLogic_L"+std::to_string(lay+1));
      new G4LogicalSkinSurface("SupportHDPESkin_L"+std::to_string(lay+1), lSup, hdpeSurface);
      lSup->SetVisAttributes(suppVis);
      new G4PVPlacement(0, G4ThreeVector( x_st1, 0, LAY_Z0[lay]), lSup,
                        "PhysSupport_R_L"+std::to_string(lay+1), logicWater, false, 0, false);
      new G4PVPlacement(0, G4ThreeVector(-x_st1, 0, LAY_Z0[lay]), lSup,
                        "PhysSupport_L_L"+std::to_string(lay+1), logicWater, false, 1, false);
  }

  // =======================================================
  // --- ASSEMBLAGGIO FIBRE (2 layer -> bundle unico) ---
  // =======================================================
  G4Transform3D trMirrorL = G4RotateZ3D(180*deg);
  G4LogicalVolume* lSt1      = makeStraight(x_pettine_exit, "F_St1");
  G4LogicalVolume* lDrittino = makeStraight(x_drittino,     "F_Drittino");
  G4LogicalVolume* lBunWater = makeStraight(bundle_in_water_len, "F_BunWater");
  G4LogicalVolume* lBunHDPE  = makeStraight(bundle_in_hdpe_len,  "F_BunHDPE");
  if (lSt1)      lSt1->SetVisAttributes(bundleVis);
  if (lDrittino) lDrittino->SetVisAttributes(bundleVis);
  if (lBunWater) lBunWater->SetVisAttributes(bundleVis);
  if (lBunHDPE)  lBunHDPE->SetVisAttributes(bundleVis);

  for (int f = 0; f < N_FIB; f++) {
      const int      il     = FIB[f].lay - 1;
      const G4double z0     = LAY_Z0[il];
      const G4double R_C    = LAY_R[il];
      const G4double y_comb = FIB[f].yc * mm;
      const G4double x_bun_offset = FIB[f].xb * mm;
      const G4double y_bun        = FIB[f].yb * mm;
      const G4double L_str = (X_PMT - R_C) - x_curve_start;
      const G4double S_arc = R_C * CLHEP::halfpi;
      const G4double S_tot = L_str + S_arc;
      const G4double t_arc = L_str / S_tot;

      if (lSt1) {
          G4Transform3D trSt1 = G4Translate3D(x_pettine_exit/2.0, y_comb, z0) * G4RotateY3D(90*deg);
          new G4PVPlacement(trSt1,             lSt1, "P_St1_R", logicWater, false, f, false);
          new G4PVPlacement(trMirrorL * trSt1, lSt1, "P_St1_L", logicWater, false, f, false);
      }
      if (lDrittino) {
          G4Transform3D trDrt = G4Translate3D(x_pettine_exit + x_drittino/2.0, y_comb, z0) * G4RotateY3D(90*deg);
          new G4PVPlacement(trDrt,             lDrittino, "P_Drt_R", logicWater, false, f, false);
          new G4PVPlacement(trMirrorL * trDrt, lDrittino, "P_Drt_L", logicWater, false, f, false);
      }

      auto gamma = [&](G4double t) -> G4ThreeVector {
          G4double s = t * t * (3.0 - 2.0 * t);
          G4double xsk, zsk;
          if (t <= t_arc) {
              xsk = x_curve_start + L_str * (t / t_arc);
              zsk = z0;
          } else {
              G4double psi = (t - t_arc) / (1.0 - t_arc) * CLHEP::halfpi;
              xsk = x_curve_start + L_str + R_C * std::sin(psi);
              zsk = z0 + R_C * (1.0 - std::cos(psi));
          }
          return G4ThreeVector(xsk + x_bun_offset * s,
                               y_comb + (y_bun - y_comb) * s,
                               zsk);
      };
      for (int k = 0; k < N_SEG[il]; k++) {
          G4ThreeVector Pa = gamma((G4double)k / N_SEG[il]);
          G4ThreeVector Pb = gamma((G4double)(k+1) / N_SEG[il]);
          G4String segName = "F_Bz_" + std::to_string(f) + "_" + std::to_string(k);
          placeTubeBetween(logicWater, Pa, Pb, segName, f);
      }

      if (lBunWater) {
          G4double zCW = 0.5 * (z_inizio_bundle + waterZ);
          G4Transform3D trBW = G4Translate3D(X_PMT + x_bun_offset, y_bun, zCW);
          new G4PVPlacement(trBW,             lBunWater, "P_BunW_R", logicWater, false, f, false);
          new G4PVPlacement(trMirrorL * trBW, lBunWater, "P_BunW_L", logicWater, false, f, false);
      }
      if (lBunHDPE) {
          G4double zCH = waterZ + bundle_in_hdpe_len/2.0;
          G4Transform3D trBH = G4Translate3D(X_PMT + x_bun_offset, y_bun, zCH);
          new G4PVPlacement(trBH,             lBunHDPE, "P_BunH_R", logicWorld, false, f, false);
          new G4PVPlacement(trMirrorL * trBH, lBunHDPE, "P_BunH_L", logicWorld, false, f, false);
      }
  }

  // =======================================================
  // --- PMT ---
  // =======================================================
  G4VisAttributes* pmtVis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0, 0.8));
  pmtVis->SetForceSolid(true);
  G4VSolid* sPMT = new G4Tubs("SolidPMT", 0, R_PMT, 0.5*mm, 0, 360*deg);
  G4LogicalVolume* lPMT_R = new G4LogicalVolume(sPMT, glass, "LogicPMT_R");
  G4LogicalVolume* lPMT_L = new G4LogicalVolume(sPMT, glass, "LogicPMT_L");
  lPMT_R->SetVisAttributes(pmtVis);
  lPMT_L->SetVisAttributes(pmtVis);
  new G4PVPlacement(0, G4ThreeVector( X_PMT, 0, Z_PMT_center), lPMT_R, "PhysPMT_R", logicWorld, false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(-X_PMT, 0, Z_PMT_center), lPMT_L, "PhysPMT_L", logicWorld, false, 0, false);

  return physWorld;
}

void SingleModuleDetectorWBC3::ConstructSDandField() {
    SetSensitiveDetector("LogicPMT_R", new PMTSD("PMT_SD_R"), true);
    SetSensitiveDetector("LogicPMT_L", new PMTSD("PMT_SD_L"), true);
}
