#include "TripleModuleDetector.hh"
#include "PMTSD.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
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

TripleModuleDetector::TripleModuleDetector()
: G4VUserDetectorConstruction() {}

TripleModuleDetector::~TripleModuleDetector() {}

G4VPhysicalVolume* TripleModuleDetector::Construct()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* air     = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* hdpe    = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
  G4Material* water   = nist->FindOrBuildMaterial("G4_WATER");
  G4Material* glass   = nist->FindOrBuildMaterial("G4_GLASS_PLATE");
  G4Material* coreMat = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
  G4Material* cladMat = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
  G4Material* teflon  = nist->FindOrBuildMaterial("G4_TEFLON");

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
  G4double rIndexWater[]   = {1.331, 1.333, 1.338, 1.344, 1.357};
  G4double absWater[]      = {12.0*m, 10.0*m, 8.0*m, 3.0*m, 0.5*m};
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

  G4OpticalSurface* teflonSurface = new G4OpticalSurface("TeflonSurface");
  teflonSurface->SetType(dielectric_metal);
  teflonSurface->SetFinish(ground);
  teflonSurface->SetModel(unified);
  G4MaterialPropertiesTable* mptTeflon = new G4MaterialPropertiesTable();
  G4double energyTeflon[] = {2.755*eV, 3.099*eV, 4.132*eV, 4.959*eV};
  const G4int nTeflon = 4;
  G4double teflonRefl[] = {0.97, 0.96, 0.935, 0.91};
  mptTeflon->AddProperty("REFLECTIVITY", energyTeflon, teflonRefl, nTeflon);
  teflonSurface->SetMaterialPropertiesTable(mptTeflon);

  G4OpticalSurface* hdpeSurface = new G4OpticalSurface("HDPESurface");
  hdpeSurface->SetType(dielectric_metal);
  hdpeSurface->SetModel(unified);
  hdpeSurface->SetFinish(ground);
  G4MaterialPropertiesTable* mptHDPESurf = new G4MaterialPropertiesTable();
  G4double hdpeRefl[] = {0.90, 0.90};
  mptHDPESurf->AddProperty("REFLECTIVITY", energy, hdpeRefl, nEntries);
  hdpeSurface->SetMaterialPropertiesTable(mptHDPESurf);

  G4OpticalSurface* mylarSurface = new G4OpticalSurface("MylarSurface");
  mylarSurface->SetType(dielectric_metal);
  mylarSurface->SetFinish(polished);
  mylarSurface->SetModel(unified);
  G4MaterialPropertiesTable* mptMylar = new G4MaterialPropertiesTable();
  G4double mylarRefl[] = {0.98, 0.98};
  mptMylar->AddProperty("REFLECTIVITY", energy, mylarRefl, nEntries);
  mylarSurface->SetMaterialPropertiesTable(mptMylar);

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
  // --- MONDO E TANK A 3 MODULI ---
  // =======================================================
  // Y/Z = 3.5 m (non 3 m): il piano di generazione mu_lngs ha
  // semiestensione 3250 mm e deve restare dentro il volume mondo,
  // altrimenti i primari generati oltre ±3 m vanno in crash (G4Navigator
  // fuori volume).
  G4Box* solidWorld = new G4Box("World", 9.*m, 3.5*m, 3.5*m);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0);

  G4double yCenters[3] = {-306.5*mm, 0.0*mm, 306.5*mm};
  G4double tankX = 500.*mm, tankY = 470.*mm, tankZ = 60.*mm;
  G4double waterX = 490.*mm, waterY = 460.*mm, waterZ = 50.*mm;

  G4Box* outerTank = new G4Box("OuterTankBox", tankX, tankY, tankZ);
  G4Box* innerWaterBox = new G4Box("InnerWaterBox", waterX, waterY, waterZ);
  G4SubtractionSolid* hollowTank = new G4SubtractionSolid("HollowTank", outerTank, innerWaterBox);

  G4Tubs* subHoleLateral = new G4Tubs("SubHoleLat", 0, 3.8*mm, 6.0*mm, 0, 360*deg);
  G4RotationMatrix* rotY90_hole = new G4RotationMatrix();
  rotY90_hole->rotateY(90*deg);

  G4VSolid* solidTank = hollowTank;
  for (int s = 0; s < 3; s++) {
      G4ThreeVector hPosR( 495.0*mm, yCenters[s], 0);
      G4ThreeVector hPosL(-495.0*mm, yCenters[s], 0);
      solidTank = new G4SubtractionSolid("Tank_Hole_R_"+std::to_string(s), solidTank, subHoleLateral, rotY90_hole, hPosR);
      solidTank = new G4SubtractionSolid("Tank_Hole_L_"+std::to_string(s), solidTank, subHoleLateral, rotY90_hole, hPosL);
  }

  G4LogicalVolume* logicTank = new G4LogicalVolume(solidTank, hdpe, "TankLogic");
  G4VisAttributes* tankVis = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.3));
  tankVis->SetForceWireframe(true);
  logicTank->SetVisAttributes(tankVis);
  new G4PVPlacement(0, G4ThreeVector(), logicTank, "TankPhys", logicWorld, false, 0, true);
  new G4LogicalSkinSurface("TankHDPESkin", logicTank, hdpeSurface);

  G4LogicalVolume* logicWater = new G4LogicalVolume(innerWaterBox, water, "WaterLogic");
  G4VisAttributes* waterVis = new G4VisAttributes(G4Colour(0.0, 0.5, 1.0, 0.15));
  waterVis->SetForceSolid(true);
  logicWater->SetVisAttributes(waterVis);
  new G4PVPlacement(0, G4ThreeVector(), logicWater, "WaterPhys", logicWorld, false, 0, true);

  // Separatori HDPE
  G4Box* solidSeparator = new G4Box("SeparatorBox", 480.0*mm, 2.5*mm, waterZ);
  G4LogicalVolume* logicSeparator = new G4LogicalVolume(solidSeparator, hdpe, "SeparatorLogic");
  new G4LogicalSkinSurface("SepHDPESkin", logicSeparator, hdpeSurface);
  new G4PVPlacement(0, G4ThreeVector(0, -153.25*mm, 0), logicSeparator, "SeparatorPhys_0", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0,  153.25*mm, 0), logicSeparator, "SeparatorPhys_1", logicWater, false, 1, true);

  // Mylar
  G4Box* sMylarZ   = new G4Box("sMylarZ",   490.0*mm, 152.125*mm, 0.1*mm);
  G4Box* sMylarX   = new G4Box("sMylarX",   0.1*mm,  152.125*mm, 49.8*mm);
  G4Box* sMylarY   = new G4Box("sMylarY",   489.8*mm, 0.1*mm,    49.8*mm);
  G4Box* sMylarSep = new G4Box("sMylarSep", 480.0*mm, 0.1*mm,    49.8*mm);
  G4Box* holeX     = new G4Box("holeX",     0.5*mm,  148.0*mm,   40.0*mm);
  G4SubtractionSolid* sMylarX_holed = new G4SubtractionSolid("sMylarX_holed", sMylarX, holeX);

  G4LogicalVolume* lMylarZ   = new G4LogicalVolume(sMylarZ,        water, "lMylarZ");
  G4LogicalVolume* lMylarX   = new G4LogicalVolume(sMylarX_holed,  water, "lMylarX");
  G4LogicalVolume* lMylarY   = new G4LogicalVolume(sMylarY,        water, "lMylarY");
  G4LogicalVolume* lMylarSep = new G4LogicalVolume(sMylarSep,      water, "lMylarSep");

  new G4LogicalSkinSurface("MylarSkinZ",   lMylarZ,   mylarSurface);
  new G4LogicalSkinSurface("MylarSkinX",   lMylarX,   mylarSurface);
  new G4LogicalSkinSurface("MylarSkinY",   lMylarY,   mylarSurface);
  new G4LogicalSkinSurface("MylarSkinSep", lMylarSep, mylarSurface);

  G4VisAttributes* invVis = new G4VisAttributes(G4VisAttributes::GetInvisible());
  lMylarZ->SetVisAttributes(invVis); lMylarX->SetVisAttributes(invVis);
  lMylarY->SetVisAttributes(invVis); lMylarSep->SetVisAttributes(invVis);

  G4double mylar_y_center_top = 307.875*mm;
  new G4PVPlacement(0, G4ThreeVector(0, mylar_y_center_top, 49.9*mm), lMylarZ, "Myl_Z1_Top", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0, mylar_y_center_top,-49.9*mm), lMylarZ, "Myl_Z2_Top", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector( 489.9*mm, mylar_y_center_top, 0), lMylarX, "Myl_X1_Top", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(-489.9*mm, mylar_y_center_top, 0), lMylarX, "Myl_X2_Top", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0,  459.9*mm, 0), lMylarY, "Myl_Y_Top",  logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0,  155.85*mm,0), lMylarSep,"Myl_Sep_Top",logicWater, false, 0, true);

  G4double mylar_y_center_bot = -307.875*mm;
  new G4PVPlacement(0, G4ThreeVector(0, mylar_y_center_bot, 49.9*mm), lMylarZ, "Myl_Z1_Bot", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0, mylar_y_center_bot,-49.9*mm), lMylarZ, "Myl_Z2_Bot", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector( 489.9*mm, mylar_y_center_bot, 0), lMylarX, "Myl_X1_Bot", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(-489.9*mm, mylar_y_center_bot, 0), lMylarX, "Myl_X2_Bot", logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0, -459.9*mm, 0), lMylarY, "Myl_Y_Bot",  logicWater, false, 0, true);
  new G4PVPlacement(0, G4ThreeVector(0, -155.85*mm,0), lMylarSep,"Myl_Sep_Bot",logicWater, false, 0, true);

  // =======================================================
  // --- STRUMENTI PER LE FIBRE ---
  // =======================================================
  G4double cladR = 0.5*mm, coreR = 0.47*mm, pitch = 1.05*mm;

  G4VisAttributes* cladVis = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0, 0.3));
  cladVis->SetForceSolid(true);
  G4VisAttributes* coreVis = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 1.0));
  coreVis->SetForceSolid(true);

  auto makeStraight = [&](G4double len, G4String name) {
      G4Tubs* sClad = new G4Tubs(name+"_Clad", 0, cladR, len/2.0, 0, 360*deg);
      G4Tubs* sCore = new G4Tubs(name+"_Core", 0, coreR, len/2.0, 0, 360*deg);
      G4LogicalVolume* lClad = new G4LogicalVolume(sClad, cladMat, name+"_CladLog");
      G4LogicalVolume* lCore = new G4LogicalVolume(sCore, coreMat, name+"_CoreLog");
      new G4PVPlacement(0, G4ThreeVector(), lCore, "core", lClad, false, 0, false);
      lClad->SetVisAttributes(cladVis);
      lCore->SetVisAttributes(coreVis);
      return lClad;
  };

  auto makeArc = [&](G4double bendR, G4double startPhi, G4double dPhi, G4String name) {
      G4Torus* sClad = new G4Torus(name+"_Clad", 0, cladR, bendR, startPhi, dPhi);
      G4Torus* sCore = new G4Torus(name+"_Core", 0, coreR, bendR, startPhi, dPhi);
      G4LogicalVolume* lClad = new G4LogicalVolume(sClad, cladMat, name+"_CladLog");
      G4LogicalVolume* lCore = new G4LogicalVolume(sCore, coreMat, name+"_CoreLog");
      new G4PVPlacement(0, G4ThreeVector(), lCore, "core", lClad, false, 0, false);
      lClad->SetVisAttributes(cladVis);
      lCore->SetVisAttributes(coreVis);
      return lClad;
  };

  G4double lenW = 490.0 * 2.0 * mm;
  G4LogicalVolume* lCladW = makeStraight(lenW, "CladL");
  G4double lenE = 10.0 * mm;
  G4LogicalVolume* lCladE = makeStraight(lenE, "CladL_Ext");

  G4double dx_bend_c2    = 240.0 * mm;
  G4double x_straight_c2 = 250.0 * mm;
  G4LogicalVolume* fHalfCenterLog = makeStraight(x_straight_c2, "F_HalfCenter");
  G4LogicalVolume* fExtC2Log      = makeStraight(10.0*mm, "F_ExtC2");

  G4VSolid* sPMT_Unified = new G4Tubs("SolidPMT", 0, 3.8*mm, 0.5*mm, 0, 360*deg);
  G4VisAttributes* pmtVis = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 1.0));
  pmtVis->SetForceSolid(true);
  G4RotationMatrix* rotY90 = new G4RotationMatrix();
  rotY90->rotateY(90*deg);

  std::vector<G4ThreeVector> hexPos37(37);
  int hIdx = 0;
  hexPos37[hIdx++] = G4ThreeVector(0, 0, 0);
  for (int i = 0; i < 6;  i++) hexPos37[hIdx++] = G4ThreeVector(0, pitch*std::cos(i*60*deg),  pitch*std::sin(i*60*deg));
  for (int i = 0; i < 12; i++) hexPos37[hIdx++] = G4ThreeVector(0, 2*pitch*std::cos(i*30*deg), 2*pitch*std::sin(i*30*deg));
  for (int i = 0; i < 18; i++) hexPos37[hIdx++] = G4ThreeVector(0, 3*pitch*std::cos(i*20*deg), 3*pitch*std::sin(i*20*deg));

  // Pettine
  G4Box* sSupportBase = new G4Box("SupportBase", 5.0*mm, 150.0*mm, 37.5*mm);
  G4VSolid* sSupport = sSupportBase;
  G4Tubs* sHole = new G4Tubs("FiberHole", 0.0, 0.6*mm, 6.0*mm, 0, 360*deg);
  G4RotationMatrix* rotHole = new G4RotationMatrix();
  rotHole->rotateY(90*deg);

  std::vector<int> fiberRank(37);
  for (int i = 0; i < 37; i++) fiberRank[i] = i;
  std::sort(fiberRank.begin(), fiberRank.end(), [&](int a, int b){
      if (std::abs(hexPos37[a].y() - hexPos37[b].y()) > 1e-3) return hexPos37[a].y() > hexPos37[b].y();
      return hexPos37[a].z() > hexPos37[b].z();
  });
  std::vector<int> combPosForFiber(37);
  for (int i = 0; i < 37; i++) combPosForFiber[fiberRank[i]] = i;

  for (int i = 0; i < 37; i++) {
      G4double holeY = (18.0 - i) * 8.0 * mm;
      sSupport = new G4SubtractionSolid("Support_Hole_"+std::to_string(i), sSupport, sHole, rotHole, G4ThreeVector(0, holeY, 0));
  }

  G4LogicalVolume* lSupport = new G4LogicalVolume(sSupport, hdpe, "SupportLogic");
  new G4LogicalSkinSurface("SupportHDPESkin", lSupport, hdpeSurface);
  G4VisAttributes* suppVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 1.0));
  suppVis->SetForceSolid(true);
  lSupport->SetVisAttributes(suppVis);

  // =======================================================
  // --- ASSEMBLAGGIO DEI 3 MODULI ---
  // =======================================================
  for (int s = 0; s < 3; s++) {
      G4double yRel = yCenters[s];

      G4LogicalVolume* lPMT_R = new G4LogicalVolume(sPMT_Unified, glass, "LogicPMT_R_" + std::to_string(s));
      G4LogicalVolume* lPMT_L = new G4LogicalVolume(sPMT_Unified, glass, "LogicPMT_L_" + std::to_string(s));
      lPMT_R->SetVisAttributes(pmtVis);
      lPMT_L->SetVisAttributes(pmtVis);

      new G4PVPlacement(rotY90, G4ThreeVector( 500.5*mm, yRel, 0), lPMT_R, "PhysPMT_R_"+std::to_string(s), logicWorld, false, s, false);
      new G4PVPlacement(rotY90, G4ThreeVector(-500.5*mm, yRel, 0), lPMT_L, "PhysPMT_L_"+std::to_string(s), logicWorld, false, s, false);

      if (s == 1 || s == 2) {
          G4Transform3D shiftTr  = G4Translate3D(0, yRel, 0);
          G4Transform3D trMirrorL = G4RotateZ3D(180*deg);

          new G4PVPlacement(shiftTr * G4Translate3D( 230.0*mm, 0, 0), lSupport, "PhysSupport_R_"+std::to_string(s), logicWater, false, s*2,   false);
          new G4PVPlacement(shiftTr * G4Translate3D(-230.0*mm, 0, 0), lSupport, "PhysSupport_L_"+std::to_string(s), logicWater, false, s*2+1, false);

          for (int f = 0; f < 37; f++) {
              G4double y_flat = (18.0 - combPosForFiber[f]) * 8.0 * mm;
              G4double dy = hexPos37[f].y() - y_flat;
              G4double dz = hexPos37[f].z();
              G4double D  = std::sqrt(dy*dy + dz*dz);
              G4int cNo   = s * 100 + f;

              G4Transform3D trStX = G4Translate3D(x_straight_c2/2.0, y_flat, 0) * G4RotateY3D(90*deg);
              new G4PVPlacement(shiftTr * trStX,             fHalfCenterLog, "P_StX_R", logicWater, false, cNo, false);
              new G4PVPlacement(shiftTr * trMirrorL * trStX, fHalfCenterLog, "P_StX_L", logicWater, false, cNo, false);

              if (D > 1e-5) {
                  G4double R     = (dx_bend_c2*dx_bend_c2 + D*D) / (4.0 * D);
                  G4double theta = std::acos(1.0 - D / (2.0 * R));
                  G4double alpha = std::atan2(dz, dy);

                  G4LogicalVolume* lArc1 = makeArc(R, -90*deg, theta, "F_Arc1_"+std::to_string(f));
                  G4LogicalVolume* lArc2 = makeArc(R,  90*deg, theta, "F_Arc2_"+std::to_string(f));

                  G4RotationMatrix rot; rot.rotateX(alpha);
                  G4ThreeVector baseR(x_straight_c2, y_flat, 0);
                  G4Transform3D trArc1 = G4Translate3D(baseR + rot * G4ThreeVector(0, R, 0)) * G4RotateX3D(alpha);
                  G4Transform3D trArc2 = G4Translate3D(baseR + rot * G4ThreeVector(dx_bend_c2, D - R, 0)) * G4RotateX3D(alpha);

                  new G4PVPlacement(shiftTr * trArc1,             lArc1, "P_A1_R", logicWater, false, cNo, false);
                  new G4PVPlacement(shiftTr * trArc2,             lArc2, "P_A2_R", logicWater, false, cNo, false);
                  new G4PVPlacement(shiftTr * trMirrorL * trArc1, lArc1, "P_A1_L", logicWater, false, cNo, false);
                  new G4PVPlacement(shiftTr * trMirrorL * trArc2, lArc2, "P_A2_L", logicWater, false, cNo, false);
              } else {
                  G4LogicalVolume* lStBend = makeStraight(dx_bend_c2, "F_StBend_"+std::to_string(f));
                  G4Transform3D trStBend = G4Translate3D(x_straight_c2 + dx_bend_c2/2.0, y_flat, 0) * G4RotateY3D(90*deg);
                  new G4PVPlacement(shiftTr * trStBend,             lStBend, "P_StBend_R", logicWater, false, cNo, false);
                  new G4PVPlacement(shiftTr * trMirrorL * trStBend, lStBend, "P_StBend_L", logicWater, false, cNo, false);
              }

              G4Transform3D trExtBase = G4Translate3D(495.0*mm, hexPos37[f].y(), hexPos37[f].z()) * G4RotateY3D(90*deg);
              new G4PVPlacement(shiftTr * trExtBase,             fExtC2Log, "P_Ext_R", logicWorld, false, cNo, false);
              new G4PVPlacement(shiftTr * trMirrorL * trExtBase, fExtC2Log, "P_Ext_L", logicWorld, false, cNo, false);
          }

      } else { // s == 0: Modulo Bottom, bundle dritto 18 fibre
          int cNo = s * 100;
          auto placeBundle = [&](G4double dy, G4double dz, int copyNo, G4String baseName) {
              new G4PVPlacement(rotY90, G4ThreeVector(0, yRel+dy, dz),         lCladW, baseName,       logicWater, false, copyNo, false);
              new G4PVPlacement(rotY90, G4ThreeVector( 495.0*mm, yRel+dy, dz), lCladE, baseName+"_ER", logicWorld, false, copyNo, false);
              new G4PVPlacement(rotY90, G4ThreeVector(-495.0*mm, yRel+dy, dz), lCladE, baseName+"_EL", logicWorld, false, copyNo, false);
          };
          placeBundle(0, 0, cNo++, "FibC");
          for (int i = 0; i < 6;  i++) placeBundle(pitch*std::cos(i*60*deg),                  pitch*std::sin(i*60*deg),                  cNo++, "FibA1");
          for (int i = 0; i < 11; i++) placeBundle(2*pitch*std::cos(i*(360./11)*deg), 2*pitch*std::sin(i*(360./11)*deg), cNo++, "FibA2");
      }
  }

  return physWorld;
}

void TripleModuleDetector::ConstructSDandField()
{
    for (int s = 0; s < 3; s++) {
        G4String sStr = std::to_string(s);
        // PMT_SD_R_0 → pmtID 1, PMT_SD_L_0 → pmtID 0, ecc. (mappatura in PMTSD.cc)
        SetSensitiveDetector("LogicPMT_R_" + sStr, new PMTSD("PMT_SD_R_" + sStr), true);
        SetSensitiveDetector("LogicPMT_L_" + sStr, new PMTSD("PMT_SD_L_" + sStr), true);
    }
}
