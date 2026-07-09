================================================================================
  WCtestbeamSim — Unified Geant4 Simulation
  Water Cherenkov detector — Testbeam 2026 & LNGS background studies
================================================================================

DESCRIPTION
-----------
Geant4 multi-thread simulation of a Water Cherenkov (WC) detector.
Supports two detector geometries and three particle sources, all selectable
at runtime via command-line arguments and macro files (no recompilation needed).

Detector geometries:
  - single     : single WC module, 37 WLS fibres (BCF-9995XL) with 3D Bezier routing,
                 2 PMTs on the +Z face. Tank: 980x360x120 mm, water: 960x360x100 mm.
                 Fibre combs at X = ±380 mm, Bezier dX = 52 mm, z_bundle = 35 mm.
  - single_wbc : Wide-Bend-Curve variant of single. Identical tank and water volume.
                 Fibre combs moved to X = ±330 mm (50 mm closer to centre).
                 Bezier dX = 102 mm (doubled), z_bundle = 45 mm → softer fibre bending,
                 reduced optical losses at curves. Min bend radius ~30 mm vs ~27 mm.
  - single_wbc2: WBC2 variant. Same combs at ±330 mm, but the XZ curve profile is
                 straight run + quarter circle R = 45 mm instead of the quadratic
                 Bezier. Min bend radius: central fibre 45 mm (was 30.4), worst
                 fibre 17.9 mm (was 9.3). Note: moving combs further inward does
                 NOT improve radii (vertical budget caps R at ~45-48 mm).
  - triple     : three-module detector, 3x37 fibres (straight + arc routing),
                 6 lateral PMTs (Bottom/Mid/Top x L/R). Tank: 1000x940x120 mm.

Particle sources (selected via .mac file):
  - e-  500 MeV Gaussian beam     → testbeam 2026
  - gamma background (natural radioactivity, configurable energy)  → LNGS studies
  - mu- cosmic muons (Mei & Hime spectrum, LNGS Hall A depth)     → LNGS studies


REQUIREMENTS
------------
- Geant4 (installed in Conda environment: conda activate geant4_env)
- CMake >= 3.16
- C++17 compiler


BUILD
-----
  conda activate geant4_env
  cd Testbeam2026_WC_unified/build
  cmake ..
  make -j$(nproc)           # Linux
  make -j$(sysctl -n hw.logicalcpu)   # macOS

The executable is: build/WCtestbeamSim
New .cc files in src/ are picked up automatically by the glob in CMakeLists.txt —
no manual CMakeLists.txt edit required when adding a new geometry.


USAGE
-----
  # Batch mode (recommended for production)
  ./WCtestbeamSim <geometry> <macro>

  # Interactive mode (GUI visualisation)
  ./WCtestbeamSim <geometry>

  where <geometry> is one of: single | single_wbc | triple
  (default: "triple" if omitted)

Examples:
  ./WCtestbeamSim triple run_triple.mac           # testbeam electrons, triple
  ./WCtestbeamSim single run_single.mac           # testbeam electrons, single
  ./WCtestbeamSim single_wbc run_single.mac       # testbeam electrons, WBC geometry
  ./WCtestbeamSim triple                          # interactive GUI, triple
  ./WCtestbeamSim single                          # interactive GUI, single
  ./WCtestbeamSim single_wbc                      # interactive GUI, WBC geometry

To change particle source, edit the /control/execute line in the run macro:
  /control/execute gun_electrons_testbeam.mac    # e- 500 MeV
  /control/execute gun_muons_testbeam.mac        # mu- 2700 MeV (testbeam)
  /control/execute gun_gammas_background.mac     # gamma background (LNGS)
  /control/execute gun_muons_lngs.mac            # cosmic muons (Mei & Hime)

Production scan (parallel jobs, output in data/):
  cd src/
  bash Run_scan.sh single    e_tb      # electrons, single geometry
  bash Run_scan.sh single_wbc e_tb    # electrons, WBC geometry
  bash Run_scan.sh triple    mu_tb     # muons testbeam, triple geometry
  bash Run_scan.sh single    mu_lngs   # cosmic muons, single geometry
  bash Run_scan.sh single_wbc mu_lngs # cosmic muons, WBC geometry
  bash Run_scan.sh triple    gamma     # gamma background, triple geometry

  The script must be run inside the geant4_env conda environment:
    conda activate geant4_env
    cd src/
    bash Run_scan.sh <geo> <src>


ROOT OUTPUT
-----------
Output files are written to data/ with the naming pattern:
  sim_<geometry>_Z2000_Y<Y>_X<X>_<angle>Deg.root

Each file contains two TTrees:

  TTree "Eventi"  (one row per primary particle)
    Col  0  E_init_MeV         Kinetic energy of primary at water entry [MeV]
    Col  1  TrackL_water_mm    Track length of primary in water [mm]
    Col  2  Edep_water_MeV     Energy deposited by primary in water [MeV]
    Col  3  N_Cerenkov_prod    Total Cherenkov photons produced
    Col  4  N_Cerenkov_entrati Cherenkov photons that entered a fibre
    Col  5  T_FirstHit_ns      Time of first PMT hit [ns] (-1 if none)
    Col  6  Hits_L_Bot         Photon hits on Left-Bottom PMT
    Col  7  Hits_R_Bot         Photon hits on Right-Bottom PMT
    Col  8  Hits_L_Mid         Photon hits on Left-Mid PMT  (0 for single)
    Col  9  Hits_R_Mid         Photon hits on Right-Mid PMT (0 for single)
    Col 10  Hits_L_Top         Photon hits on Left-Top PMT  (0 for single)
    Col 11  Hits_R_Top         Photon hits on Right-Top PMT (0 for single)
    Col 12  SourceType         0=e- (testbeam), 1=gamma (background), 2=mu- (cosmic)

  TTree "Fotoni"  (one row per photon reaching a PMT)
    Col  0  EventID            Event number
    Col  1  TrackID            Geant4 track ID of the photon
    Col  2  E_Hit_eV           Photon energy at arrival [eV]
    Col  3  Arrival_Time_ns    Global arrival time [ns]
    Col  4  PMT_ID             PMT index (0=L_Bot, 1=R_Bot, ..., 5=R_Top)


PROJECT STRUCTURE
-----------------

  sim.cc
      Main entry point. Parses command-line arguments (geometry type + macro),
      instantiates the detector and physics, and starts the run manager.

  CMakeLists.txt
      CMake build configuration. Automatically sets Conda prefix path so
      Geant4 is found in the active Conda environment.

  --- GEOMETRY ---

  include/SingleModuleDetector.hh
  src/SingleModuleDetector.cc
      Single-module WC detector. Water volume 960x360x100 mm inside a 10 mm
      thick HDPE tank. 37 WLS fibres with 3D Bezier curves routing from a
      comb at X=±380 mm to a vertical bundle above 2 PMTs on the +Z face.
      Teflon (PTFE) lining on all 6 inner faces (energy-dependent reflectivity).

  include/SingleModuleDetectorWBC.hh
  src/SingleModuleDetectorWBC.cc
      Wide-Bend-Curve variant of SingleModuleDetector. Same tank, water volume
      and optical properties. Key differences:
        x_st1 (comb position): 380 mm → 330 mm  (50 mm closer to centre)
        bezier_dX:              52 mm → 102 mm   (doubled horizontal arc span)
        z_inizio_bundle:        35 mm → 45 mm    (more Z travel for softer bend)
        bundle_in_water_len:    15 mm → 5 mm
      Minimum bend radius in XZ plane improves from ~27 mm to ~30 mm.
      Motivated by reducing optical losses at the Bezier curve.

  include/TripleModuleDetector.hh
  src/TripleModuleDetector.cc
      Three-module WC detector. Three submodules stacked along Y, separated
      by 5 mm HDPE plates with Mylar reflectors. Bottom module uses 18 straight
      fibres; central and top modules use 37 fibres with arc routing. 6 lateral
      PMTs (pairs at X=±500.5 mm, one pair per module).

  --- PHYSICS ---

  include/PhysicsList.hh
  src/PhysicsList.cc
      Geant4 modular physics list. Includes EM standard physics, decay physics,
      and optical physics (Cerenkov, WLS, OpAbsorption, OpRayleigh, OpBoundary,
      Scintillation all active).

  --- PARTICLE GENERATION ---

  include/PrimaryGeneratorAction.hh
  src/PrimaryGeneratorAction.cc
      Wraps G4GeneralParticleSource (GPS). All particle properties (type,
      energy, position, direction, spectrum) are configured via macro.
      Automatically detects the particle type and sets SourceType in EventAction
      so it is saved in the output ntuple.

  --- USER ACTIONS ---

  include/ActionInitialization.hh
  src/ActionInitialization.cc
      Registers all user actions for master and worker threads (MT-safe).

  include/EventAction.hh
  src/EventAction.cc
      Accumulates per-event counters (energy deposit, track length, Cerenkov
      counts, PMT hits, first hit time, source type) and fills ntuple 0 at
      end of event.

  include/RunAction.hh
  src/RunAction.cc
      Opens/closes the ROOT file and defines the ntuple structure (13 + 5
      columns). Handles MT ntuple merging.

  include/SteppingAction.hh
  src/SteppingAction.cc
      Step-by-step tracking. Counts Cerenkov photons entering fibres, tracks
      the primary particle (any type, trackID==1) through water, and counts
      total Cerenkov production. Thread-safe: uses G4EventManager (not
      G4RunManager) to access EventAction.

  include/PMTSD.hh
  src/PMTSD.cc
      Sensitive detector for PMTs. Detects optical photon hits, maps SD name
      to PMT ID (0-5), updates EventAction counters, and fills ntuple 1.
      Handles both single-module naming (PMT_SD_L/R) and triple-module naming
      (PMT_SD_L/R_0/1/2). Kills the photon after detection to prevent
      re-entering the glass.

  --- MACRO FILES ---

  run_single.mac
      Production run macro for single-module geometry. Selects particle source
      via /control/execute. Default: gun_electrons_testbeam.mac.

  run_triple.mac
      Production run macro for triple-module geometry. Same structure as above.

  gun_electrons_testbeam.mac
      GPS configuration for 500 MeV electrons. Gaussian beam profile
      sigma_r=0.85 mm centred at (0,0,2000 mm), directed along -Z.

  gun_gammas_background.mac
      GPS configuration for gamma background (natural radioactivity).
      Isotropic surface source surrounding the detector. Default energy
      2.614 MeV (208Tl, highest line in Th chain). Commented-out section
      shows how to configure a mixed-energy spectrum (208Tl + 214Bi + 40K).

  gun_muons_lngs.mac
      GPS configuration for cosmic muons at LNGS Hall A (3800 m.w.e. depth).
      Energy spectrum approximates the Mei & Hime (2006) parameterisation
      (peak ~270 GeV). Angular distribution: cos^2(theta), max 60 deg from
      zenith. Source plane at Z=+1500 mm above the detector.

  init_vis.mac
      Macro for interactive visualisation mode. Initialises the geometry
      and loads vis.mac.

  vis.mac
      OpenGL visualisation settings: surface rendering, white background,
      yellow optical photon trajectories, geometry colours.

  --- PRODUCTION SCRIPTS ---

  src/Run_scan.sh
      Bash script for parallel production runs. Launches up to 8 parallel
      jobs (configurable), scanning X/Y positions and beam angles.
      Usage: bash Run_scan.sh <geo> <src>
        geo: single | single_wbc | triple
        src: e_tb | mu_tb | mu_lngs | gamma
      Beam sources (e_tb, mu_tb): full X/Y/angle scan.
      Distributed sources (mu_lngs, gamma): single run, no scan.
      Output ROOT files go to data/ with naming:
        sim_<geo>_<src>_X<x>mm_Y<y>mm_Th<theta>deg.root


OPTICAL PROPERTIES SUMMARY
---------------------------
  Water (deionised):
    Refractive index: dispersive 1.331 (700nm) to 1.357 (300nm)
    Absorption: 12 m (700nm) to 0.5 m (300nm)
    Rayleigh scattering: 5 m (700nm) to 0.1 m (300nm)

  Teflon (PTFE) lining — Lambertian reflector:
    Reflectivity: 0.97 (450nm) to 0.91 (250nm), energy-dependent

  WLS fibre BCF-9995XL (Saint-Gobain):
    Core: polystyrene, n=1.60, absorption 2.5 m
    Cladding: PMMA, n=1.49
    WLS absorption peak: ~385 nm (3.22 eV)
    WLS emission peak: ~500 nm (2.48 eV)
    Time constant: 2.7 ns
    Quantum efficiency (WLSMEANNUMBERPHOTONS): 0.75

  PMT window (glass):
    n=1.50, absorption 0.1 mm (fully absorbing — photon killed on hit)

================================================================================
