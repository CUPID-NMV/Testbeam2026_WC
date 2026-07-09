#include "SingleModuleDetectorWBC2.hh"
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

SingleModuleDetectorWBC2::SingleModuleDetectorWBC2(G4int nFibers, G4double combPitchMM)
: G4VUserDetectorConstruction(), fNFib(nFibers), fCombPitchMM(combPitchMM) {}

SingleModuleDetectorWBC2::~SingleModuleDetectorWBC2() {}

G4VPhysicalVolume* SingleModuleDetectorWBC2::Construct() {
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

  G4double tankX = 490.0*mm, tankY = 190.0*mm, tankZ = 60.0*mm;
  G4double waterX = 480.0*mm, waterY = 180.0*mm, waterZ = 50.0*mm;

  // =======================================================
  // --- PARAMETRI GEOMETRIA FIBRE E PMT (WBC2) ---
  // Pettini a x=330mm come single_wbc. Il profilo XZ della curva
  // e' retta orizzontale + quarto di cerchio R=45mm (invece della
  // Bezier quadratica): raggio minimo fibra centrale 45mm (era
  // 30.4mm), fibra peggiore 17.9mm (era 9.3mm). Il morphing y
  // usa smoothstep sulla frazione di lunghezza d'arco: stessa
  // struttura a skeleton comune della geometria originale, che
  // garantisce l'assenza di incroci tra fibre (dist. min tra assi
  // 0.78mm, contro 0.66mm della WBC attuale).
  // =======================================================
  const G4double x_st1           = 330.0 * mm;
  const G4double pettine_semiX   = 5.0 * mm;
  const G4double x_pettine_exit  = x_st1 + pettine_semiX;   // = 335 mm
  const G4double x_drittino      = 3.0 * mm;
  const G4double x_curve_start   = x_pettine_exit + x_drittino;  // = 338 mm
  const G4double X_PMT           = 440.0 * mm;
  const G4double Z_PMT_center    = tankZ - 0.5*mm;
  const G4double R_CURVE         = 45.0 * mm;    // raggio quarto di cerchio XZ
  const G4double z_inizio_bundle = R_CURVE;      // il bundle parte in cima all'arco
  const G4double bundle_in_water_len = waterZ - z_inizio_bundle;   // = 5 mm
  const G4double bundle_in_hdpe_len  = (Z_PMT_center - 0.5*mm) - waterZ;  // = 9 mm
  const G4double L_straight      = (X_PMT - R_CURVE) - x_curve_start;  // = 57 mm
  const G4double S_arc           = R_CURVE * CLHEP::halfpi;
  const G4double S_tot           = L_straight + S_arc;
  const G4double t_arc           = L_straight / S_tot;  // frazione arclength della retta
  const int N_CURVE_SEG = 28;

  // =======================================================
  // --- TANK CON FORI PMT ---
  // =======================================================
  G4VSolid* solidTank = new G4Box("BaseTankBox", tankX, tankY, tankZ);
  G4Tubs* subHole = new G4Tubs("SubHole", 0, 3.8*mm, 5.0*mm, 0, 360*deg);
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
  // --- PATTERN ESAGONALE (parametrico: 7 / 19 / 37 fibre) ---
  // anello k: 6k fibre a raggio k*pitch, passo angolare 360/(6k)
  // =======================================================
  std::vector<G4ThreeVector> hexPos37;
  hexPos37.push_back(G4ThreeVector(0, 0, 0));
  for (int ring = 1; (int)hexPos37.size() < fNFib; ring++) {
      int nring = 6 * ring;
      for (int i = 0; i < nring && (int)hexPos37.size() < fNFib; i++)
          hexPos37.push_back(G4ThreeVector(ring*pitch*std::cos(i*(360.0/nring)*deg),
                                           ring*pitch*std::sin(i*(360.0/nring)*deg), 0));
  }
  const G4double comb_top = 0.5 * (fNFib - 1);   // slot centrale a y=0

  G4LogicalVolume* lSt1      = makeStraight(x_pettine_exit, "F_St1");        // 335 mm
  G4LogicalVolume* lDrittino = makeStraight(x_drittino,     "F_Drittino");
  G4LogicalVolume* lBunWater = makeStraight(bundle_in_water_len, "F_BunWater");  // 5 mm
  G4LogicalVolume* lBunHDPE  = makeStraight(bundle_in_hdpe_len,  "F_BunHDPE");
  if (lSt1)      lSt1->SetVisAttributes(bundleVis);
  if (lDrittino) lDrittino->SetVisAttributes(bundleVis);
  if (lBunWater) lBunWater->SetVisAttributes(bundleVis);
  if (lBunHDPE)  lBunHDPE->SetVisAttributes(bundleVis);

  // =======================================================
  // --- SUPPORTO MECCANICO (PETTINE) ---
  // =======================================================
  G4Box* sSupportBase = new G4Box("SupportBase", pettine_semiX, 180.0*mm, 12.5*mm);
  G4VSolid* sSupport = sSupportBase;
  G4Tubs* sHole = new G4Tubs("FiberHole", 0.0, 0.6*mm, 6.0*mm, 0, 360*deg);
  G4RotationMatrix* rotHole = new G4RotationMatrix();
  rotHole->rotateY(90*deg);

  std::vector<int> fiberRank(fNFib);
  for (int i = 0; i < fNFib; i++) fiberRank[i] = i;
  std::sort(fiberRank.begin(), fiberRank.end(), [&](int a, int b){
      if (std::abs(hexPos37[a].y() - hexPos37[b].y()) > 1e-3) return hexPos37[a].y() > hexPos37[b].y();
      return hexPos37[a].x() > hexPos37[b].x();
  });
  std::vector<int> combPosForFiber(fNFib);
  for (int i = 0; i < fNFib; i++) combPosForFiber[fiberRank[i]] = i;

  for (int i = 0; i < fNFib; i++) {
      G4double holeY = (comb_top - i) * fCombPitchMM * mm;
      sSupport = new G4SubtractionSolid("Support_Hole_"+std::to_string(i), sSupport, sHole, rotHole, G4ThreeVector(0, holeY, 0));
  }

  G4LogicalVolume* lSupport = new G4LogicalVolume(sSupport, hdpe, "SupportLogic");
  new G4LogicalSkinSurface("SupportHDPESkin", lSupport, hdpeSurface);
  G4VisAttributes* suppVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 1.0));
  suppVis->SetForceSolid(true);
  lSupport->SetVisAttributes(suppVis);
  new G4PVPlacement(0, G4ThreeVector( x_st1, 0, 0), lSupport, "PhysSupport_R", logicWater, false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(-x_st1, 0, 0), lSupport, "PhysSupport_L", logicWater, false, 1, false);

  // =======================================================
  // --- ASSEMBLAGGIO FIBRE ---
  // =======================================================
  G4Transform3D trMirrorL = G4RotateZ3D(180*deg);

  for (int f = 0; f < fNFib; f++) {
      G4double y_comb       = (comb_top - combPosForFiber[f]) * fCombPitchMM * mm;
      G4double x_bun_offset = hexPos37[f].x();
      G4double y_bun        = hexPos37[f].y();

      // 1. Tratto pettine dritto
      if (lSt1) {
          G4Transform3D trSt1 = G4Translate3D(x_pettine_exit/2.0, y_comb, 0) * G4RotateY3D(90*deg);
          new G4PVPlacement(trSt1,            lSt1, "P_St1_R", logicWater, false, f, false);
          new G4PVPlacement(trMirrorL * trSt1, lSt1, "P_St1_L", logicWater, false, f, false);
      }

      // 2. Tratto drittino
      if (lDrittino) {
          G4Transform3D trDrt = G4Translate3D(x_pettine_exit + x_drittino/2.0, y_comb, 0) * G4RotateY3D(90*deg);
          new G4PVPlacement(trDrt,             lDrittino, "P_Drt_R", logicWater, false, f, false);
          new G4PVPlacement(trMirrorL * trDrt, lDrittino, "P_Drt_L", logicWater, false, f, false);
      }

      // 3. Curva 3D: skeleton retta + quarto di cerchio (parametro t =
      //    frazione di lunghezza d'arco), morphing y/x-offset smoothstep
      auto gamma = [&](G4double t) -> G4ThreeVector {
          G4double s = t * t * (3.0 - 2.0 * t);   // smoothstep
          G4double xsk, zsk;
          if (t <= t_arc) {
              xsk = x_curve_start + L_straight * (t / t_arc);
              zsk = 0.0;
          } else {
              G4double psi = (t - t_arc) / (1.0 - t_arc) * CLHEP::halfpi;
              xsk = x_curve_start + L_straight + R_CURVE * std::sin(psi);
              zsk = R_CURVE * (1.0 - std::cos(psi));
          }
          return G4ThreeVector(xsk + x_bun_offset * s,
                               y_comb + (y_bun - y_comb) * s,
                               zsk);
      };

      for (int k = 0; k < N_CURVE_SEG; k++) {
          G4ThreeVector Pa = gamma((G4double)k / N_CURVE_SEG);
          G4ThreeVector Pb = gamma((G4double)(k+1) / N_CURVE_SEG);
          G4String segName = "F_Bz_" + std::to_string(f) + "_" + std::to_string(k);
          placeTubeBetween(logicWater, Pa, Pb, segName, f);
      }

      // 4. Bundle verticale in acqua (5 mm)
      if (lBunWater) {
          G4double zCW = 0.5 * (z_inizio_bundle + waterZ);
          G4Transform3D trBW = G4Translate3D(X_PMT + x_bun_offset, y_bun, zCW);
          new G4PVPlacement(trBW,             lBunWater, "P_BunW_R", logicWater, false, f, false);
          new G4PVPlacement(trMirrorL * trBW, lBunWater, "P_BunW_L", logicWater, false, f, false);
      }

      // 5. Bundle verticale in HDPE
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
  G4VSolid* sPMT = new G4Tubs("SolidPMT", 0, 3.8*mm, 0.5*mm, 0, 360*deg);
  G4LogicalVolume* lPMT_R = new G4LogicalVolume(sPMT, glass, "LogicPMT_R");
  G4LogicalVolume* lPMT_L = new G4LogicalVolume(sPMT, glass, "LogicPMT_L");
  lPMT_R->SetVisAttributes(pmtVis);
  lPMT_L->SetVisAttributes(pmtVis);
  new G4PVPlacement(0, G4ThreeVector( X_PMT, 0, Z_PMT_center), lPMT_R, "PhysPMT_R", logicWorld, false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(-X_PMT, 0, Z_PMT_center), lPMT_L, "PhysPMT_L", logicWorld, false, 0, false);

  return physWorld;
}

void SingleModuleDetectorWBC2::ConstructSDandField() {
    SetSensitiveDetector("LogicPMT_R", new PMTSD("PMT_SD_R"), true);
    SetSensitiveDetector("LogicPMT_L", new PMTSD("PMT_SD_L"), true);
}
