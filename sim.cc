#include "SingleModuleDetector.hh"
#include "SingleModuleDetectorWBC.hh"
#include "SingleModuleDetectorWBC2.hh"
#include "SingleModuleDetectorWBC3.hh"
#include "TripleModuleDetector.hh"
#include "ActionInitialization.hh"
#include "PhysicsList.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

// Uso:
//   ./WCtestbeamSim                            → interattivo, geometria triple (default)
//   ./WCtestbeamSim single                     → interattivo, geometria single
//   ./WCtestbeamSim single_wbc                 → interattivo, geometria single WBC (pettini a 330mm)
//   ./WCtestbeamSim single_wbc2                → interattivo, WBC2 (retta+cerchio R=45mm)
//   ./WCtestbeamSim single_wbc2_19f            → interattivo, WBC2 con 19 fibre (passo 8mm)
//   ./WCtestbeamSim single_wbc2_19fw           → interattivo, WBC2 19 fibre passo 16mm (copertura piena)
//   ./WCtestbeamSim triple                     → interattivo, geometria triple
//   ./WCtestbeamSim run.mac                    → batch, geometria triple (default)
//   ./WCtestbeamSim single run_single.mac      → batch, geometria single
//   ./WCtestbeamSim single_wbc run_single.mac  → batch, geometria single WBC
//   ./WCtestbeamSim triple run_triple.mac      → batch, geometria triple

int main(int argc, char** argv)
{
  G4String geoType = "triple";
  G4String macroFile = "";
  bool interactive = false;

  if (argc == 1) {
    interactive = true;
  } else if (argc == 2) {
    G4String arg = argv[1];
    if (arg == "single" || arg == "single_wbc" || arg == "single_wbc2" ||
        arg == "single_wbc2_19f" || arg == "single_wbc2_19fw" ||
        arg == "single_wbc2_long" || arg == "single_wbc3_long" ||
        arg == "single_wbc2_short" || arg == "single_wbc2_mini" ||
        arg == "single_wbc3_short" || arg == "single_wbc3_mini" ||
        arg == "single_wbc3" || arg == "triple") {
      geoType = arg;
      interactive = true;
    } else {
      macroFile = arg;
    }
  } else if (argc >= 3) {
    geoType = argv[1];
    macroFile = argv[2];
  }

  G4UIExecutive* ui = interactive ? new G4UIExecutive(argc, argv) : nullptr;

  // Serial in batch: Run_scan.sh parallelises at process level; G4TaskRunManager
  // spawns 32 threads/process → OOM when 8 electron jobs compete simultaneously.
  auto* runManager = G4RunManagerFactory::CreateRunManager(
      ui ? G4RunManagerType::Default : G4RunManagerType::Serial);

  if (geoType == "single")
    runManager->SetUserInitialization(new SingleModuleDetector());
  else if (geoType == "single_wbc")
    runManager->SetUserInitialization(new SingleModuleDetectorWBC());
  else if (geoType == "single_wbc2")
    runManager->SetUserInitialization(new SingleModuleDetectorWBC2());
  else if (geoType == "single_wbc2_19f")
    runManager->SetUserInitialization(new SingleModuleDetectorWBC2(19));
  else if (geoType == "single_wbc2_19fw")
    runManager->SetUserInitialization(new SingleModuleDetectorWBC2(19, 16.0));
  else if (geoType == "single_wbc2_long")   // tank 3160 mm (semi-len 1580), tratto fibra ~1425 mm
    runManager->SetUserInitialization(new SingleModuleDetectorWBC2(37, 8.0, 1580.0));
  else if (geoType == "single_wbc2_short")  // tank 490 mm (semi-len 245), tratto fibra ~90 mm
    runManager->SetUserInitialization(new SingleModuleDetectorWBC2(37, 8.0, 245.0));
  else if (geoType == "single_wbc2_mini")   // tank 400 mm (semi-len 200), tratto fibra ~45 mm
    runManager->SetUserInitialization(new SingleModuleDetectorWBC2(37, 8.0, 200.0));
  else if (geoType == "single_wbc3_long")   // tank 3160 mm, WBC3 (2 layer)
    runManager->SetUserInitialization(new SingleModuleDetectorWBC3(1580.0));
  else if (geoType == "single_wbc3_short")  // tank 490 mm, WBC3 (2 layer)
    runManager->SetUserInitialization(new SingleModuleDetectorWBC3(245.0));
  else if (geoType == "single_wbc3_mini")   // tank 400 mm, WBC3 (2 layer)
    runManager->SetUserInitialization(new SingleModuleDetectorWBC3(200.0));
  else if (geoType == "single_wbc3")
    runManager->SetUserInitialization(new SingleModuleDetectorWBC3());
  else
    runManager->SetUserInitialization(new TripleModuleDetector());

  runManager->SetUserInitialization(new PhysicsList());
  runManager->SetUserInitialization(new ActionInitialization());

  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();

  G4UImanager* UImanager = G4UImanager::GetUIpointer();
  UImanager->ApplyCommand("/control/macroPath ../");

  if (!ui) {
    UImanager->ApplyCommand("/control/execute " + macroFile);
  } else {
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
