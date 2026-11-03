// =============================================================================
// Pannello di scintillatore del LNGS: porting del MC della collega di Yale
// (DetectorConstructionCAD.cc, layout "Nested", e DefineMaterials() di
// DetectorConstruction.cc, zip del 2026-10-30). Vedi ScintPanel.hh per cosa
// e' cambiato nell'integrazione; il resto di questa intestazione e' la sua.
//
// Geometria fedele al CAD (AL_Housing V14, export STEP): groove 2x5mm
// riempiti di colla EJ-500, fibre appoggiate sul fondo e sollevate di un
// livello solo dove si sovrappongono, wrapping 3xTyvek + HDPE, housing di
// alluminio, bundle nel cilindro acrilico avvitato al plate, grasso ottico e
// SiPM S13360-6075PE.
//
// Sistema di riferimento: origine al centro dello scintillatore. Le quote del
// CAD (assieme AL_Housing.STEP) si convertono con p_sim = p_CAD - kCadOrigin:
// kCadOrigin e' il centro dello scintillatore nel CAD, alzato dello spessore
// del wrapping (nel CAD il pannello poggia nudo sulla schiuma, nella realta'
// ci poggia il wrapping).
//
// Ogni fibra e' descritta come UNA polilinea 3D continua, da un'estremita'
// della barra acrilica all'altra: percorso in pianta del CAD (rette e archi
// R60) con un profilo di quota z(s) lungo il percorso, poi uscita dal
// pannello, curva a S in aria fino al proprio posto nel bundle 1+7 e tratto
// dritto nel foro. La polilinea viene poi spezzata nei tratti che stanno in
// madri diverse (groove, Tyvek, HDPE, aria, colla del bundle) e ogni tratto e'
// piazzato come catena di G4CutTubs con giunti mitrati. Tutti i parametri dei
// percorsi sono stati verificati in Python prima di essere riportati qui; il
// codice ripete comunque il controllo delle distanze tra fibre (vedi
// CheckFiberClearance) a ogni costruzione.
// =============================================================================
#include "ScintPanel.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4CutTubs.hh"
#include "G4Polycone.hh"
#include "G4Polyhedra.hh"
#include "G4MultiUnion.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4VisAttributes.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "G4TwoVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "G4LogicalBorderSurface.hh"
#include "G4GenericMessenger.hh"
#include "G4VTouchable.hh"

namespace {

// ---------------------------------------------------------------------------
// Wrapping (non presente nel CAD)
// ---------------------------------------------------------------------------
const G4double kTyvekLayer = 0.1905*mm;          // Tyvek Uline S-10680: 7.5 mil
const G4double kTyvekThick = 3*kTyvekLayer;      // 3 strati
const G4double kHdpeThick  = 0.381*mm;           // HDPE 0.015"
const G4double kWrapThick  = kTyvekThick + kHdpeThick;

// Centro dello scintillatore nel CAD (pannello x -57.77..1192.23,
// y 77.131..627.131, z 236.449..286.449), alzato del wrapping.
const G4ThreeVector kCadOrigin(567.23*mm, 352.131*mm, 261.449*mm + kWrapThick);
G4ThreeVector Cad(G4double x, G4double y, G4double z) {
    return G4ThreeVector(x*mm, y*mm, z*mm) - kCadOrigin;
}

// ---------------------------------------------------------------------------
// Pannello, groove e fibre
// ---------------------------------------------------------------------------
const G4double kScintX = 625.0*mm, kScintY = 275.0*mm, kScintZ = 25.0*mm;
const G4double kGrooveBottom = 20.0*mm;          // groove profondi 5mm dalla faccia superiore
const G4double kGrooveHalfW  = 1.0*mm;           // groove larghi 2mm
const G4double kBendR = 60.0*mm;                 // raggio delle curve nel pannello (CAD)
const G4double kXEdge = -kScintX;                // bordo d'uscita delle fibre
const G4double kXU    = 505.0*mm;                // centro delle U

// BCF-91AXL, cladding singolo spesso il 3% del diametro (datasheet Luxium);
// la collega aveva 0.485 mm di nucleo
const G4double kCladR = 0.500*mm, kCoreR = 0.470*mm;
// Livelli in z: livello 0 = fibra appoggiata sul fondo del groove (10um di
// margine numerico), ogni livello sopra +1mm (+10um di margine). Il livello
// massimo usato e' 3 (asse a 23.54mm, superficie a 24.04mm < 25mm).
const G4double kZ0 = kGrooveBottom + kCladR + 0.01*mm;
const G4double kDZ = 2*kCladR + 0.01*mm;
// Rampe tra livelli: profilo a coseno con raggio di curvatura minimo 100mm
// (lunghezza pi*sqrt(R*dz/2): 22.3mm per un livello, 31.6mm per due).
const G4double kRampRmin = 100.0*mm;
const G4double kPlateau  = 2.0*mm;               // mezzo plateau attorno agli incroci
const G4double kLead     = 1.0*mm;               // anticipo della fibra superiore di una coppia impilata
const G4double kArcStep  = 2.5*deg;              // passo angolare della discretizzazione
const G4double kRampStep = 2.0*mm;               // passo lungo le rampe
const G4double kMinSeg   = 0.5*mm;               // segmento minimo

// Margine dei giunti mitrati. Dal DetectorConstruction.cc della collega:
// coincidenza esatta tra due G4CutTubs "gemelli" (stesso piano di taglio,
// nessuna fessura ne' sovrapposizione) e' numericamente fragile per il
// navigatore -- verificato: un fotone lanciato lungo l'asse della fibra
// attraverso una giunzione perfettamente mitrata usciva comunque, in modo
// deterministico, proprio li'. Una piccola sovrapposizione deliberata no.
const G4double kMiterOverlapCAD = 0.02*mm;

// ---------------------------------------------------------------------------
// Bundle e lettura (quote CAD)
// ---------------------------------------------------------------------------
const G4double kXTyvek = kXEdge - kTyvekThick;   // faccia esterna del Tyvek
const G4double kXWrap  = kXTyvek - kHdpeThick;   // faccia esterna dell'HDPE
const G4double kXMouth = Cad(-242.97, 0, 0).x(); // imbocco del BundleTop (-810.20)
const G4double kXFunnelEnd = Cad(-245.92, 0, 0).x(); // fine dell'imbuto
const G4double kXHeadBack  = Cad(-252.72, 0, 0).x(); // testa del cilindro / faccia del plate
const G4double kXRodEnd    = Cad(-264.07, 0, 0).x(); // estremita' della barra (fibre a filo)
const G4double kXRodChamfer = Cad(-264.07 + 1.125, 0, 0).x(); // smusso 45 gradi della barra
const G4double kBundleY = Cad(0, 351.166, 0).y();  // asse del bundle
const G4double kBundleZ = Cad(0, 0, 265.024).z();
const G4double kBoreR = 1.66*mm, kFunnelR = 2.85*mm, kHeadR = 9.55*mm;
const G4double kRodR = 5.0*mm, kRodEndR = 3.875*mm, kPlateHoleR = 5.1*mm;
// Impacchettamento 1+7 nel foro r=1.66: anello a 1.157mm -> 4.0um tra fibre
// vicine dell'anello, 3um dalla parete, 157um dalla fibra centrale.
const G4double kRingR = 1.157*mm;

// ---------------------------------------------------------------------------
// Percorsi in pianta
// ---------------------------------------------------------------------------
struct Prim {
    G4bool arc;
    G4TwoVector p0, p1;          // retta
    G4TwoVector c;               // arco: centro, raggio, angoli iniziale e finale
    G4double r, a0, a1;

    G4double Length() const { return arc ? std::abs(a1 - a0)*r : (p1 - p0).mag(); }
    G4TwoVector At(G4double t) const {
        if (!arc) return p0 + t*(p1 - p0);
        G4double a = a0 + t*(a1 - a0);
        return c + r*G4TwoVector(std::cos(a), std::sin(a));
    }
    // parametro t in [0,1] del punto piu' vicino a q
    G4double Closest(const G4TwoVector& q) const {
        if (!arc) {
            G4TwoVector d = p1 - p0;
            return std::clamp((q - p0).dot(d)/d.mag2(), 0.0, 1.0);
        }
        G4double sweep = a1 - a0;
        G4double phi = std::atan2(q.y() - c.y(), q.x() - c.x());
        G4double dl = std::remainder(phi - a0 - sweep/2, 2*CLHEP::pi) + sweep/2;
        G4double t = std::clamp(dl/sweep, 0.0, 1.0);
        G4double best = t, dBest = (At(t) - q).mag();
        for (G4double te : {0.0, 1.0})
            if ((At(te) - q).mag() < dBest) { best = te; dBest = (At(te) - q).mag(); }
        return best;
    }
};

Prim Line(G4double x0, G4double y0, G4double x1, G4double y1) {
    return Prim{false, G4TwoVector(x0, y0), G4TwoVector(x1, y1), G4TwoVector(), 0, 0, 0};
}
Prim Arc(G4double cx, G4double cy, G4double a0deg, G4double a1deg) {
    return Prim{true, G4TwoVector(), G4TwoVector(), G4TwoVector(cx, cy), kBendR, a0deg*deg, a1deg*deg};
}

class Path2D {
  public:
    explicit Path2D(std::vector<Prim> prims) : fPrims(std::move(prims)) {
        fS0.push_back(0);
        for (const auto& p : fPrims) fS0.push_back(fS0.back() + p.Length());
    }
    G4double Length() const { return fS0.back(); }
    const std::vector<Prim>& Prims() const { return fPrims; }
    const std::vector<G4double>& PrimStarts() const { return fS0; }

    G4TwoVector At(G4double s) const {
        size_t i = std::upper_bound(fS0.begin(), fS0.end(), s) - fS0.begin();
        i = std::clamp<size_t>(i, 1, fPrims.size()) - 1;
        return fPrims[i].At((s - fS0[i])/fPrims[i].Length());
    }
    // ascissa curvilinea del punto del percorso piu' vicino a q
    G4double Project(const G4TwoVector& q) const {
        G4double best = 0, dBest = std::numeric_limits<G4double>::max();
        for (size_t i = 0; i < fPrims.size(); ++i) {
            G4double t = fPrims[i].Closest(q);
            G4double d = (fPrims[i].At(t) - q).mag();
            if (d < dBest) { dBest = d; best = fS0[i] + t*fPrims[i].Length(); }
        }
        return best;
    }

  private:
    std::vector<Prim> fPrims;
    std::vector<G4double> fS0;
};

// Profilo dei livelli lungo il percorso: nodi (s, livello) ordinati; tra due
// nodi a livello diverso la fibra sale/scende con una rampa a coseno.
class ZProfile {
  public:
    explicit ZProfile(std::vector<std::pair<G4double, G4double>> knots) : fK(std::move(knots)) {
        std::sort(fK.begin(), fK.end());
    }
    G4double Level(G4double s) const {
        if (s <= fK.front().first) return fK.front().second;
        for (size_t i = 0; i + 1 < fK.size(); ++i) {
            auto [sa, la] = fK[i];
            auto [sb, lb] = fK[i+1];
            if (s >= sa && s <= sb) {
                if (la == lb) return la;
                G4double u = (s - sa)/(sb - sa);
                return la + (lb - la)*0.5*(1 - std::cos(CLHEP::pi*u));
            }
        }
        return fK.back().second;
    }
    G4double Z(G4double s) const { return kZ0 + kDZ*Level(s); }
    std::vector<G4double> RampBreakpoints() const {
        std::vector<G4double> bp;
        for (size_t i = 0; i + 1 < fK.size(); ++i) {
            auto [sa, la] = fK[i];
            auto [sb, lb] = fK[i+1];
            if (la == lb) continue;
            G4int n = (G4int)std::ceil((sb - sa)/kRampStep);
            for (G4int k = 0; k <= n; ++k) bp.push_back(sa + (sb - sa)*k/n);
        }
        return bp;
    }
  private:
    std::vector<std::pair<G4double, G4double>> fK;
};

G4double RampLength(G4double dLevels) {
    return CLHEP::pi*std::sqrt(kRampRmin*std::abs(dLevels)*kDZ/2.0);
}

// Punti 3D della fibra nel pannello: confini delle primitive, passi angolari
// sugli archi e passi lungo le rampe.
std::vector<G4ThreeVector> DiscretizePanel(const Path2D& path, const ZProfile& prof) {
    const auto& s0 = path.PrimStarts();
    std::vector<std::pair<G4double, G4bool>> bp;          // (s, confine di primitiva)
    for (size_t i = 0; i < path.Prims().size(); ++i) {
        const Prim& p = path.Prims()[i];
        bp.push_back({s0[i], true});
        if (p.arc) {
            G4int n = (G4int)std::ceil(std::abs(p.a1 - p.a0)/kArcStep - 1e-9);
            for (G4int k = 1; k < n; ++k) bp.push_back({s0[i] + p.Length()*k/n, false});
        }
    }
    bp.push_back({path.Length(), true});
    for (G4double s : prof.RampBreakpoints()) bp.push_back({s, false});
    std::sort(bp.begin(), bp.end());

    std::vector<std::pair<G4double, G4bool>> out;
    for (const auto& b : bp) {
        if (!out.empty() && b.first - out.back().first < 1e-6) {
            out.back().second = out.back().second || b.second;
            continue;
        }
        if (!out.empty() && b.first - out.back().first < kMinSeg) {
            if (b.second && !out.back().second) out.back() = b;   // tiene il confine di primitiva
            else if (b.second) out.push_back(b);
            continue;
        }
        out.push_back(b);
    }
    std::vector<G4ThreeVector> pts;
    for (const auto& b : out) {
        G4TwoVector xy = path.At(b.first);
        pts.emplace_back(xy.x(), xy.y(), prof.Z(b.first));
    }
    return pts;
}

// Curva a S in aria: due archi uguali di raggio R nel piano che contiene -x e
// lo spostamento laterale uscita->posto, da x=x0 (tangente -x) a x=x0-Lx
// (tangente -x). R segue da Lx e dallo spostamento D: R = (Lx^2+D^2)/(4D).
std::vector<G4ThreeVector> SCurve(G4TwoVector exitYZ, G4TwoVector slotYZ, G4double x0, G4double xEnd) {
    G4TwoVector d = slotYZ - exitYZ;
    G4double D = d.mag();
    G4double Lx = x0 - xEnd;
    G4TwoVector e = d/D;
    G4double R = (Lx*Lx + D*D)/(4*D);
    G4double th = std::acos(1 - D/(2*R));
    G4int n = std::max(2, (G4int)std::ceil(th/kArcStep));
    std::vector<G4ThreeVector> pts;
    auto add = [&](G4double xl, G4double lat) {
        pts.emplace_back(xl, exitYZ.x() + lat*e.x(), exitYZ.y() + lat*e.y());
    };
    for (G4int k = 0; k <= n; ++k) {
        G4double ph = th*k/n;
        add(x0 - R*std::sin(ph), R - R*std::cos(ph));
    }
    for (G4int k = n - 1; k >= 0; --k) {
        G4double ph = th*k/n;
        add(x0 - Lx + R*std::sin(ph), D - (R - R*std::cos(ph)));
    }
    return pts;
}

// Distanza minima tra due segmenti 3D (Ericson, Real-Time Collision Detection)
G4double SegSegDist(const G4ThreeVector& p1, const G4ThreeVector& q1,
                    const G4ThreeVector& p2, const G4ThreeVector& q2, G4ThreeVector& where) {
    G4ThreeVector d1 = q1 - p1, d2 = q2 - p2, r = p1 - p2;
    G4double a = d1.mag2(), e = d2.mag2(), f = d2.dot(r), c = d1.dot(r), b = d1.dot(d2);
    G4double den = a*e - b*b;
    G4double s = (den > 1e-14) ? std::clamp((b*f - c*e)/den, 0.0, 1.0) : 0.0;
    G4double t = (b*s + f)/e;
    if (t < 0)      { t = 0; s = std::clamp(-c/a, 0.0, 1.0); }
    else if (t > 1) { t = 1; s = std::clamp((b - c)/a, 0.0, 1.0); }
    where = p1 + s*d1;
    return (p1 + s*d1 - p2 - t*d2).mag();
}

// ---------------------------------------------------------------------------
// Descrizione delle due geometrie
// ---------------------------------------------------------------------------
struct FiberSpec {
    G4String name;
    Path2D path;
    std::vector<std::pair<G4double, G4double>> knots;
    G4int slotA, slotB;          // posto nel bundle dei due capi (0 = centro, 1-7 = anello)
    G4double dEndA, dEndB;       // distanza dall'imbocco a cui il capo finisce la curva a S
};

struct LayoutSpec {
    std::vector<FiberSpec> fibers;
    G4double psi0;               // rotazione dell'anello del bundle
};

// Mezzo plateau a livello lev attorno all'ascissa s, con rampe di lunghezza
// rl dal livello base. lead>0 allarga il plateau: serve alla fibra superiore
// di una coppia impilata che sale insieme a quella sotto (altrimenti sulla
// rampa la distanza tra gli assi scenderebbe a 1.01mm*cos(pendenza)).
void Bump(std::vector<std::pair<G4double, G4double>>& k, G4double s, G4double lev,
          G4double base, G4double rl, G4double lead = 0) {
    k.push_back({s - kPlateau - lead - rl, base});
    k.push_back({s - kPlateau - lead, lev});
    k.push_back({s + kPlateau + lead, lev});
    k.push_back({s + kPlateau + lead + rl, base});
}


// Incrocio delle U di due loop adiacenti (centri a 60mm di distanza): (556.96, +-120)
const G4TwoVector kU1(kXU + std::sqrt(kBendR*kBendR - 30*mm*30*mm), 120*mm);


LayoutSpec NestedLayout() {
    const G4double R1 = RampLength(1), R2 = RampLength(2);
    const G4double X = kXEdge, U = kXU;
    // Percorsi dal CAD: ogni fibra va dal capo "out" (fori +-10) al capo "in"
    // (fori +-30). C e D sono le speculari di A e B.
    Path2D pA({Arc(X, 70, -90, 0), Line(-565, 70, -565, 150), Arc(-505, 150, 180, 90),
               Line(-505, 210, U, 210), Arc(U, 150, 90, -90), Line(U, 90, -445, 90),
               Arc(-445, 30, 90, 180), Arc(-565, 30, 0, -90), Line(-565, -30, X, -30)});
    Path2D pB({Arc(X, 70, -90, 0), Line(-565, 70, -565, 90), Arc(-505, 90, 180, 90),
               Line(-505, 150, U, 150), Arc(U, 90, 90, -90), Line(U, 30, X, 30)});
    Path2D pC({Arc(X, -70, 90, 0), Line(-565, -70, -565, -150), Arc(-505, -150, 180, 270),
               Line(-505, -210, U, -210), Arc(U, -150, -90, 90), Line(U, -90, -445, -90),
               Arc(-445, -30, -90, -180), Arc(-565, -30, 0, 90), Line(-565, 30, X, 30)});
    Path2D pD({Arc(X, -70, 90, 0), Line(-565, -70, -565, -90), Arc(-505, -90, 180, 270),
               Line(-505, -150, U, -150), Arc(U, -90, -90, 90), Line(U, -30, X, -30)});

    // Punti notevoli
    const G4TwoVector X1(X + kBendR*std::sqrt(1 - 4.0/9.0), 30*mm);   // incrocio a 4 fibre (-580.28, 30)
    const G4double sX    = pA.Project(X1);                            // uguale per A,B (X1) e C,D (X2)
    const G4double sDep  = pA.Project(G4TwoVector(-565*mm, (90 + 60*std::sin(10.5*deg))*mm)); // B si stacca da A
    const G4double sU    = pA.Project(kU1);                           // U: A sopra B (C sopra D)
    const G4double sABin = pA.Project(G4TwoVector(-505*mm, 30*mm));   // A_in incrocia B_in
    const G4double sJoin = sABin + kBendR*79.5*deg;                   // A_in a <1.01mm da D_in (y=-30)

    // A: A_out sopra B_out (liv. 1) nel groove condiviso, sale a 3 sopra
    //    l'incrocio a 4 fibre, scende a 0 quando B si stacca; liv. 1 sulla U
    //    (sopra B); A_in sopra B_in (1), sotto C_in (0, incrocio a (-513,0)),
    //    sopra D_in (1) fino al foro -30.
    std::vector<std::pair<G4double, G4double>> kA = {{0, 1}};
    Bump(kA, sX, 3, 1, R2, kLead);
    kA.push_back({sDep + 1.5*mm, 1}); kA.push_back({sDep + 1.5*mm + R1, 0});
    Bump(kA, sU, 1, 0, R1);
    Bump(kA, sABin, 1, 0, R1);
    kA.push_back({sJoin - kPlateau - R1, 0}); kA.push_back({sJoin - kPlateau, 1});
    kA.push_back({pA.Length(), 1});
    // B: sempre sul fondo, tranne sopra l'incrocio a 4 fibre (liv. 2: sotto
    //    A_out e sopra C_in, che passa sotto a liv. 1 sopra B_in).
    std::vector<std::pair<G4double, G4double>> kB = {{0, 0}};
    Bump(kB, sX, 2, 0, R2);
    kB.push_back({pB.Length(), 0});
    // C: come A fino a C_in; C_in sale a 1 prima di incrociare D_in e resta a
    //    1 fino al foro +30 (sopra A_in all'incrocio e sopra B_in nel groove
    //    condiviso). Dove A_in e C_in si incrociano una delle due deve stare
    //    sopra: per questo A e C non sono esattamente speculari.
    std::vector<std::pair<G4double, G4double>> kC = {{0, 1}};
    Bump(kC, sX, 3, 1, R2, kLead);
    kC.push_back({sDep + 1.5*mm, 1}); kC.push_back({sDep + 1.5*mm + R1, 0});
    Bump(kC, sU, 1, 0, R1);
    kC.push_back({sABin - kPlateau - R1, 0}); kC.push_back({sABin - kPlateau, 1});
    kC.push_back({pC.Length(), 1});
    std::vector<std::pair<G4double, G4double>> kD = {{0, 0}};
    Bump(kD, sX, 2, 0, R2);
    kD.push_back({pD.Length(), 0});

    // Posti nel bundle (capo out, capo in) e fine delle curve a S: ricerca +
    // ottimizzazione in Python, raggio minimo delle curve esterne 198mm.
    LayoutSpec L;
    L.fibers.push_back({"A", pA, kA, 7, 3,  7.0*mm,  3.0*mm});
    L.fibers.push_back({"B", pB, kB, 0, 6, 57.0*mm, 13.5*mm});
    L.fibers.push_back({"C", pC, kC, 2, 1,  3.0*mm,  3.0*mm});
    L.fibers.push_back({"D", pD, kD, 5, 4,  4.5*mm,  3.5*mm});
    L.psi0 = 26*deg;
    return L;
}

G4TwoVector SlotYZ(G4int slot, G4double psi0) {
    G4TwoVector c(kBundleY, kBundleZ);
    if (slot == 0) return c;
    G4double a = psi0 + 2*CLHEP::pi*(slot - 1)/7.0;
    return c + kRingR*G4TwoVector(std::cos(a), std::sin(a));
}

// Tratto esterno di un capo, dal bordo dello scintillatore all'estremita' della barra
std::vector<G4ThreeVector> External(const G4ThreeVector& exitPt, G4int slot, G4double psi0, G4double dEnd) {
    G4TwoVector e(exitPt.y(), exitPt.z());
    G4TwoVector s = SlotYZ(slot, psi0);
    std::vector<G4ThreeVector> pts = {exitPt, G4ThreeVector(kXTyvek, e.x(), e.y())};
    for (const auto& p : SCurve(e, s, kXWrap, kXMouth + dEnd)) pts.push_back(p);
    pts.emplace_back(kXMouth, s.x(), s.y());
    pts.emplace_back(kXRodEnd, s.x(), s.y());
    return pts;
}

// Catena di G4CutTubs (cladding + core) lungo la polilinea p (coordinate della
// madre). nLow/nHigh: normali esterne dei piani di taglio agli estremi (le
// facce delle madri); i giunti interni sono mitrati sulla bisettrice e
// allungati di kMiterOverlapCAD, gli estremi no (non devono sporgere dalla madre).
void PlaceChain(const std::vector<G4ThreeVector>& p, const G4ThreeVector& nLow, const G4ThreeVector& nHigh,
                G4LogicalVolume* mother, const G4String& tag, G4Material* cladMat, G4Material* coreMat,
                G4VisAttributes* cladVis, G4VisAttributes* coreVis, G4bool check) {
    const size_t n = p.size() - 1;
    std::vector<G4ThreeVector> dirs(n);
    for (size_t s = 0; s < n; ++s) dirs[s] = (p[s+1] - p[s]).unit();

    for (size_t s = 0; s < n; ++s) {
        const G4ThreeVector& d = dirs[s];
        G4double len = (p[s+1] - p[s]).mag();
        G4double extLow  = (s > 0)     ? kMiterOverlapCAD : 0.;
        G4double extHigh = (s < n - 1) ? kMiterOverlapCAD : 0.;
        G4double halfLen = 0.5*(len + extLow + extHigh);
        G4ThreeVector center = 0.5*(p[s] + p[s+1]) + 0.5*(extHigh - extLow)*d;

        G4RotationMatrix rot;                    // rotazione attiva z -> d
        if (d.z() < 1.0 - 1e-9) {
            G4ThreeVector axis = G4ThreeVector(0, 0, 1).cross(d);
            if (axis.mag() > 1e-9) rot.rotate(std::acos(d.z()), axis.unit());
            else rot.rotateX(180*deg);
        }
        G4ThreeVector gLow  = (s == 0)     ? nLow  : -(dirs[s-1] + d).unit();
        G4ThreeVector gHigh = (s == n - 1) ? nHigh :  (d + dirs[s+1]).unit();
        G4ThreeVector lLow  = rot.inverse()*gLow;
        G4ThreeVector lHigh = rot.inverse()*gHigh;

        G4String name = tag + "_S" + std::to_string(s);
        auto* sClad = new G4CutTubs(name + "_SClad", 0, kCladR, halfLen, 0, 360*deg, lLow, lHigh);
        auto* sCore = new G4CutTubs(name + "_SCore", 0, kCoreR, halfLen, 0, 360*deg, lLow, lHigh);
        auto* lClad = new G4LogicalVolume(sClad, cladMat, name + "_LClad");
        auto* lCore = new G4LogicalVolume(sCore, coreMat, name + "_LCore");
        lClad->SetVisAttributes(cladVis);
        lCore->SetVisAttributes(coreVis);
        new G4PVPlacement(nullptr, G4ThreeVector(), lCore, name + "_PCore", lClad, false, 0, false);
        // inversa: convenzione passiva di G4PVPlacement(G4RotationMatrix*)
        new G4PVPlacement(new G4RotationMatrix(rot.inverse()), center, lClad, "p" + name, mother, false, 0, check);
    }
}

// Controllo delle distanze tra gli assi di tutte le fibre (anche tra tratti
// lontani della stessa fibra, es. l'auto-incrocio di B nel Nested): stampa la
// distanza minima e segnala ogni sovrapposizione.
void CheckFiberClearance(const std::map<G4String, std::vector<G4ThreeVector>>& fibers) {
    struct Seg { G4String f; G4double s0, s1; G4ThreeVector a, b, lo, hi; };
    std::vector<Seg> segs;
    for (const auto& [name, p] : fibers) {
        G4double s = 0;
        for (size_t k = 0; k + 1 < p.size(); ++k) {
            G4double l = (p[k+1] - p[k]).mag();
            G4ThreeVector lo(std::min(p[k].x(), p[k+1].x()), std::min(p[k].y(), p[k+1].y()), std::min(p[k].z(), p[k+1].z()));
            G4ThreeVector hi(std::max(p[k].x(), p[k+1].x()), std::max(p[k].y(), p[k+1].y()), std::max(p[k].z(), p[k+1].z()));
            segs.push_back({name, s, s + l, p[k], p[k+1], lo, hi});
            s += l;
        }
    }
    const G4double reach = 2*kCladR + 0.05*mm;
    G4double dMin = std::numeric_limits<G4double>::max();
    G4String pairMin; G4ThreeVector whereMin;
    G4int nOverlap = 0;
    for (size_t i = 0; i < segs.size(); ++i) {
        for (size_t j = i + 1; j < segs.size(); ++j) {
            const Seg& A = segs[i]; const Seg& B = segs[j];
            if (A.lo.x() > B.hi.x() + reach || B.lo.x() > A.hi.x() + reach ||
                A.lo.y() > B.hi.y() + reach || B.lo.y() > A.hi.y() + reach ||
                A.lo.z() > B.hi.z() + reach || B.lo.z() > A.hi.z() + reach) continue;
            if (A.f == B.f && std::max(A.s0, B.s0) - std::min(A.s1, B.s1) < 5*mm) continue;
            G4ThreeVector w;
            G4double d = SegSegDist(A.a, A.b, B.a, B.b, w);
            if (d < 2*kCladR) {
                ++nOverlap;
                G4cerr << "ATTENZIONE: fibre " << A.f << " e " << B.f << " sovrapposte di "
                       << (2*kCladR - d)/um << " um a " << w << G4endl;
            }
            if (d < dMin) { dMin = d; pairMin = A.f + "-" + B.f; whereMin = w; }
        }
    }
    G4cout << "----> Controllo fibre: " << segs.size() << " segmenti, distanza minima tra le superfici = "
           << (dMin - 2*kCladR)/um << " um (" << pairMin << " a " << whereMin << " mm), "
           << nOverlap << " sovrapposizioni" << G4endl;
}


// ---------------------------------------------------------------------------
// Materiali del pannello, creati una sola volta. Tutti "Pan_" e con
// composizione esplicita: copiati da un NIST con BuildMaterialWithNewDensity
// ne erediterebbero la tabella ottica, e qui G4_POLYSTYRENE, G4_PLEXIGLASS e
// G4_POLYETHYLENE hanno quelle del WC. Le proprieta' ottiche sono quelle
// della collega (DefineMaterials() e GetOrBuildMaterials()), tranne il nucleo
// delle fibre (BCF-91AXL, AddBCF91AXL), e ci sono solo con /wc/panel/optics true.
// ---------------------------------------------------------------------------
const std::vector<G4double> kE = {2.00*eV, 2.30*eV, 2.50*eV, 2.70*eV, 2.90*eV, 3.10*eV, 3.30*eV, 3.50*eV};

G4MaterialPropertiesTable* ConstMPT(G4double rindex, G4double absl) {
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", kE, std::vector<G4double>(kE.size(), rindex));
    mpt->AddProperty("ABSLENGTH", kE, std::vector<G4double>(kE.size(), absl));
    return mpt;
}

// Il WLS delle fibre dei pannelli, BCF-91AXL (l'utente, 2026-11-03: il codice
// della collega aveva i valori di una BCF-92, emissione di 4 punti e 2.7 ns).
// Spettri dal grafico della brochure Luxium (PSWS Fibers 2023, pag. 4),
// digitalizzati a passo 5 nm; tempo di decadimento del datasheet.
void AddBCF91AXL(G4MaterialPropertiesTable* mpt) {
    const G4double passo = 5.0;   // nm
    // Assorbimento, ampiezza del grafico (massimo 1, a 423 nm) da 355 a 465 nm.
    // Il datasheet da' solo la forma: al picco 0.1 mm, il valore della
    // collega. Sopra 465 nm il grafico resta a ~0.007, lo spessore della
    // linea: un assorbimento cosi' nel verde contraddirebbe i >4 m
    // d'attenuazione, e vale zero. Deve restare zero fino al minimo
    // dell'emissione (600 nm): Geant4 11.4 si ferma (G4OpWLS, WSL01) se un
    // fotone assorbito non puo' essere riemesso a energia piu' bassa.
    const G4double lambdaAss = 355.0;
    const G4double ass[] = {
        0.051, 0.069, 0.093, 0.130, 0.190, 0.252, 0.300, 0.377, 0.503,   // 355-395 nm
        0.612, 0.611, 0.668, 0.822, 0.966, 0.940, 0.702, 0.645, 0.730,   // 400-440 nm
        0.817, 0.749, 0.332, 0.109, 0.020};                              // 445-465 nm
    const G4int nAss = 23;
    const G4double picco = 0.1*mm;
    std::vector<G4double> energia = {1.5*eV, h_Planck*c_light/((lambdaAss + passo*nAss)*nm)};   // 470 nm
    std::vector<G4double> lunghezza = {1000.*km, 1000.*km};
    for (G4int k = nAss - 1; k >= 0; --k) {      // energie crescenti
        energia.push_back(h_Planck*c_light/((lambdaAss + passo*k)*nm));
        lunghezza.push_back(picco/ass[k]);
    }
    mpt->AddProperty("WLSABSLENGTH", energia, lunghezza);

    // Emissione da 455 a 600 nm, picco a 490 nm (la tabella della brochure
    // dice 494). Il grafico e' per nm, Geant4 campiona in energia:
    // dN/dE = dN/dlambda * lambda^2 / hc.
    const G4double lambdaEm = 455.0;
    const G4double em[] = {
        0.000, 0.044, 0.194, 0.463, 0.699, 0.852, 0.941, 1.000, 0.906, 0.713,   // 455-500 nm
        0.555, 0.468, 0.412, 0.366, 0.325, 0.298, 0.264, 0.238, 0.210, 0.168,   // 505-550 nm
        0.132, 0.109, 0.087, 0.063, 0.038, 0.021, 0.012, 0.006, 0.005, 0.000};  // 555-600 nm
    const G4int nEm = 30;
    std::vector<G4double> energiaEm, valore;
    for (G4int k = nEm - 1; k >= 0; --k) {
        const G4double lambda = lambdaEm + passo*k;
        energiaEm.push_back(h_Planck*c_light/(lambda*nm));
        valore.push_back(em[k]*(lambda/490.0)*(lambda/490.0));
    }
    mpt->AddProperty("WLSCOMPONENT", energiaEm, valore);
    mpt->AddConstProperty("WLSTIMECONSTANT", 12.0*ns);
}

// Stessi elementi e frazioni in massa di un materiale NIST, senza legami con lui
G4Material* CopyOf(const G4String& name, const G4String& nistName, G4double density) {
    const G4Material* base = G4NistManager::Instance()->FindOrBuildMaterial(nistName);
    auto* m = new G4Material(name, density, (G4int)base->GetNumberOfElements());
    for (G4int i = 0; i < (G4int)base->GetNumberOfElements(); ++i)
        m->AddElement(const_cast<G4Element*>(base->GetElement(i)), base->GetFractionVector()[i]);
    return m;
}

G4Material* Plastic(const G4String& name, G4double density, G4int nC, G4int nH, G4int nO) {
    G4NistManager* nist = G4NistManager::Instance();
    auto* p = new G4Material(name, density, 3);
    p->AddElement(nist->FindOrBuildElement("C"), nC);
    p->AddElement(nist->FindOrBuildElement("H"), nH);
    p->AddElement(nist->FindOrBuildElement("O"), nO);
    return p;
}

G4Material* PanelMaterial(const G4String& name, G4bool optics) {
    if (auto* found = G4Material::GetMaterial(name, false)) return found;
    G4NistManager* nist = G4NistManager::Instance();

    G4Material* mat = nullptr;
    G4MaterialPropertiesTable* mpt = nullptr;
    if (name == "Pan_EJ200") {                   // scintillatore EJ-200 (polivinil-toluene)
        mat = CopyOf(name, "G4_PLASTIC_SC_VINYLTOLUENE", 1.032*g/cm3);
        if (optics) {
            mpt = ConstMPT(1.58, 3.8*m);
            mpt->AddProperty("SCINTILLATIONCOMPONENT1", kE, {0.00, 0.05, 0.20, 0.60, 1.00, 0.50, 0.10, 0.00});
            mpt->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV);
            mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.1*ns);
            mpt->AddConstProperty("SCINTILLATIONRISETIME1", 0.9*ns);
            mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
            mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
        }
    } else if (name == "Pan_FiberCore") {        // core delle fibre WLS BCF-91AXL (polistirene)
        mat = CopyOf(name, "G4_POLYSTYRENE", 1.05*g/cm3);
        if (optics) {
            // attenuazione "oltre 4 m" nel datasheet (fibra da 1 mm); la collega 3.5 m
            mpt = ConstMPT(1.60, 4.0*m);
            AddBCF91AXL(mpt);
            mpt->AddConstProperty("WLSMEANNUMBERPHOTONS", 0.86);
        }
    } else if (name == "Pan_FiberCladding") {    // cladding (PMMA, n=1.49)
        mat = Plastic(name, 1.19*g/cm3, 5, 8, 2);
        if (optics) mpt = ConstMPT(1.49, 10.0*m);
    } else if (name == "Pan_OpticalGrease") {    // grasso ottico (silicone, n=1.465)
        mat = new G4Material(name, 1.03*g/cm3, 4);
        mat->AddElement(nist->FindOrBuildElement("Si"), 1);
        mat->AddElement(nist->FindOrBuildElement("C"), 2);
        mat->AddElement(nist->FindOrBuildElement("H"), 6);
        mat->AddElement(nist->FindOrBuildElement("O"), 1);
        if (optics) mpt = ConstMPT(1.465, 10.0*m);
    } else if (name == "Pan_EJ500") {            // colla ottica EJ-500 (epoxy, n=1.57)
        mat = Plastic(name, 1.17*g/cm3, 21, 24, 4);           // epoxy (bisfenolo A)
        if (optics) mpt = ConstMPT(1.57, 5.0*m);
    } else if (name == "Pan_Acrylic") {          // cilindro del bundle (PMMA, n=1.49)
        mat = Plastic(name, 1.19*g/cm3, 5, 8, 2);
        if (optics) mpt = ConstMPT(1.49, 5.0*m);
    } else if (name == "Pan_SiPMWindowEpoxy") {  // finestra del package S13360-xxxxPE (n=1.55)
        mat = Plastic(name, 1.18*g/cm3, 21, 24, 4);
        if (optics) mpt = ConstMPT(1.55, 5.0*m);
    } else if (name == "Pan_HDPE") {             // strato esterno del wrapping
        mat = CopyOf(name, "G4_POLYETHYLENE", 0.94*g/cm3);
    } else if (name == "Pan_Tyvek") {            // HDPE spunbonded, ~0.4 g/cm3
        mat = CopyOf(name, "G4_POLYETHYLENE", 0.40*g/cm3);
    } else if (name == "Pan_NeopreneSponge") {   // spugna di neoprene adesiva (densita' stimata)
        mat = CopyOf(name, "G4_NEOPRENE", 0.20*g/cm3);
    } else if (name == "Pan_PackageEpoxy") {     // corpo del package del SiPM (opaco: niente RINDEX)
        mat = new G4Material(name, 1.20*g/cm3, 3);
        mat->AddElement(nist->FindOrBuildElement("C"), 11);
        mat->AddElement(nist->FindOrBuildElement("H"), 12);
        mat->AddElement(nist->FindOrBuildElement("O"), 3);
    } else if (name == "Pan_FR4") {              // PCB: 52.8% vetro + 47.2% epoxy
        mat = new G4Material(name, 1.86*g/cm3, 2);
        mat->AddMaterial(nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE"), 0.528);
        mat->AddMaterial(PanelMaterial("Pan_PackageEpoxy", optics), 0.472);
    }
    if (mat && mpt) mat->SetMaterialPropertiesTable(mpt);
    return mat;
}

} // namespace

const G4ThreeVector ScintPanel::kHousingLo = Cad(-424.03, 46.441, 233.274);
const G4ThreeVector ScintPanel::kHousingHi = Cad(1219.155, 655.891, 296.774);

ScintPanel& ScintPanel::Get() {
    static ScintPanel istanza;
    return istanza;
}

ScintPanel::ScintPanel()
    : fOptics(false), fEnvelope(nullptr), fFiberCore(nullptr), fSiPM(nullptr), fOpBlack(nullptr), fMessenger(nullptr) {
    fMessenger = new G4GenericMessenger(this, "/wc/panel/", "pannelli di scintillatore del LNGS");
    fMessenger->DeclareProperty("optics", fOptics,
        "true: scintillazione, WLS e SiPM nei pannelli (lento); false: solo energia depositata. "
        "Prima di /run/initialize.");
}

void ScintPanel::Place(G4VPhysicalVolume* world, const G4ThreeVector& centre, G4int copy) {
    if (!fEnvelope) fEnvelope = Build();
    auto* pv = new G4PVPlacement(nullptr, centre + 0.5*(kHousingLo + kHousingHi), fEnvelope, "Pan_Envelope",
                                 world->GetLogicalVolume(), false, copy, true);
    // Un fotone che esce dall'involucro muore: nel MC della collega andava
    // perso nel mondo, qui potrebbe arrivare ai PMT del WC.
    new G4LogicalBorderSurface("Pan_Escape_" + std::to_string(copy), pv, world, fOpBlack);
}

G4int ScintPanel::PanelIndex(const G4VTouchable* t) const {
    if (!t || !fEnvelope) return -1;
    for (G4int d = 0; d < t->GetHistoryDepth(); ++d)
        if (t->GetVolume(d)->GetLogicalVolume() == fEnvelope) return t->GetCopyNumber(d);
    return -1;
}

// =============================================================================
// Il pannello completo dentro il suo involucro d'aria: e' il
// ConstructCADPanel(nested = true) della collega, con l'involucro al posto del
// mondo (le quote del pannello si spostano di -kEnvC, il centro
// dell'involucro) e i materiali "Pan_".
G4LogicalVolume* ScintPanel::Build() {
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* air      = nist->FindOrBuildMaterial("G4_AIR");   // l'aria del mondo, con il RINDEX del WC
    G4Material* ej200    = PanelMaterial("Pan_EJ200", fOptics);
    G4Material* coreMat  = PanelMaterial("Pan_FiberCore", fOptics);
    G4Material* cladMat  = PanelMaterial("Pan_FiberCladding", fOptics);
    G4Material* greaseMat = PanelMaterial("Pan_OpticalGrease", fOptics);
    G4Material* alMat    = nist->FindOrBuildMaterial("G4_Al");
    G4Material* steelMat = nist->FindOrBuildMaterial("G4_Fe");
    G4Material* siMat    = nist->FindOrBuildMaterial("G4_Si");
    G4Material* hdpeMat  = PanelMaterial("Pan_HDPE", fOptics);
    G4Material* tyvekMat = PanelMaterial("Pan_Tyvek", fOptics);
    G4Material* glueMat  = PanelMaterial("Pan_EJ500", fOptics);
    G4Material* acrylMat = PanelMaterial("Pan_Acrylic", fOptics);
    G4Material* foamMat  = PanelMaterial("Pan_NeopreneSponge", fOptics);
    G4Material* pkgMat   = PanelMaterial("Pan_PackageEpoxy", fOptics);
    G4Material* winMat   = PanelMaterial("Pan_SiPMWindowEpoxy", fOptics);
    G4Material* fr4Mat   = PanelMaterial("Pan_FR4", fOptics);
    fFiberCore = coreMat;
    G4cout << "[pannelli] ottica " << (fOptics ? "ACCESA: scintillazione, WLS e SiPM"
                                               : "SPENTA: solo energia depositata (/wc/panel/optics false)")
           << G4endl;

    // CheckOverlaps sui ~2000 segmenti di fibra e' lento e segnala per
    // costruzione i 20um dei giunti mitrati: lo si attiva solo a richiesta.
    const G4bool checkFibers = std::getenv("PTB_CHECK_FIBERS") != nullptr;
    const G4bool check = true;

    auto* scintVis = new G4VisAttributes(G4Colour(0.2, 0.4, 0.6, 0.15));
    auto* glueVis  = new G4VisAttributes(G4Colour(1.0, 0.8, 0.2, 0.3));
    auto* coreVis  = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 1.0));
    coreVis->SetForceSolid(true);
    auto* cladVis  = new G4VisAttributes(false);
    auto* wrapVis  = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.1));
    auto* alVis    = new G4VisAttributes(G4Colour(0.7, 0.7, 0.75, 0.25));
    auto* foamVis  = new G4VisAttributes(G4Colour(0.15, 0.15, 0.15, 0.2));
    auto* acrylVis = new G4VisAttributes(G4Colour(0.6, 0.9, 1.0, 0.35));
    auto* steelVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.6));
    auto* pcbVis   = new G4VisAttributes(G4Colour(0.0, 0.5, 0.2, 0.6));
    auto* sipmVis  = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0, 1.0));

    // --- Involucro d'aria (al posto del mondo della collega) ----------------
    // Esattamente l'ingombro dell'housing: impilati a contatto, gli involucri
    // si toccano senza sovrapporsi.
    const G4ThreeVector kEnvC = 0.5*(kHousingLo + kHousingHi);
    const G4ThreeVector envH = 0.5*(kHousingHi - kHousingLo);
    auto* lEnv = new G4LogicalVolume(new G4Box("Pan_Envelope", envH.x(), envH.y(), envH.z()), air, "Pan_Envelope");
    lEnv->SetVisAttributes(G4VisAttributes::GetInvisible());

    // --- Pannello: HDPE > Tyvek > scintillatore > groove (colla) > fibre ----
    auto* lHdpe = new G4LogicalVolume(new G4Box("Hdpe_wrap", kScintX + kWrapThick, kScintY + kWrapThick, kScintZ + kWrapThick),
                                      hdpeMat, "Hdpe_wrap");
    lHdpe->SetVisAttributes(wrapVis);
    new G4PVPlacement(nullptr, -kEnvC, lHdpe, "pHdpe_wrap", lEnv, false, 0, check);
    // HDPE: superficie assorbente (come il vecchio nastro nero). La stessa
    // superficie chiude l'involucro verso il mondo (Place).
    fOpBlack = new G4OpticalSurface("HdpeSurface", unified, ground, dielectric_metal);
    auto* mptBlack = new G4MaterialPropertiesTable();
    mptBlack->AddProperty("REFLECTIVITY", std::vector<G4double>{2.0*eV, 3.5*eV}, std::vector<G4double>{0., 0.});
    fOpBlack->SetMaterialPropertiesTable(mptBlack);
    new G4LogicalSkinSurface("HdpeSkin", lHdpe, fOpBlack);

    // Tyvek (il wrapping "Tyvek" di SetWrappingType, il suo default)
    auto* opTyvek = new G4OpticalSurface("WrappingSurface");
    opTyvek->SetType(dielectric_metal);
    opTyvek->SetModel(unified);
    opTyvek->SetFinish(ground);
    opTyvek->SetSigmaAlpha(0.2);
    auto* mptTyvek = new G4MaterialPropertiesTable();
    // Tyvek 8740D a DOPPIO strato raggiunge il 98% di riflettivita'
    mptTyvek->AddProperty("REFLECTIVITY", kE, std::vector<G4double>(kE.size(), 0.98));
    opTyvek->SetMaterialPropertiesTable(mptTyvek);

    auto* lTyvek = new G4LogicalVolume(new G4Box("Tyvek_wrap", kScintX + kTyvekThick, kScintY + kTyvekThick, kScintZ + kTyvekThick),
                                       tyvekMat, "Tyvek_wrap");
    lTyvek->SetVisAttributes(wrapVis);
    new G4PVPlacement(nullptr, G4ThreeVector(), lTyvek, "pTyvek_wrap", lHdpe, false, 0, check);
    new G4LogicalSkinSurface("TyvekSkin", lTyvek, opTyvek);

    // Il nome "Pan_ScintPhys" e' quello che SteppingAction cerca per l'Edep
    auto* lScint = new G4LogicalVolume(new G4Box("ScintBox", kScintX, kScintY, kScintZ), ej200, "ScintLogic");
    lScint->SetVisAttributes(scintVis);
    new G4PVPlacement(nullptr, G4ThreeVector(), lScint, "Pan_ScintPhys", lTyvek, false, 0, check);

    LayoutSpec layout = NestedLayout();

    // Groove: unione di blocchi dritti e settori di anello (r 59-61mm) lungo i
    // percorsi delle fibre, dal fondo (z=20) alla faccia superiore (z=25).
    // Non e' scintillatore: la colla non entra nell'Edep del pannello.
    auto* grooveSolid = new G4MultiUnion("Ej500_groove");
    {
        std::set<std::string> done;
        G4double hz = 0.5*(kScintZ - kGrooveBottom), zc = 0.5*(kScintZ + kGrooveBottom);
        G4int k = 0;
        for (const auto& f : layout.fibers) {
            for (const auto& p : f.path.Prims()) {
                std::ostringstream key;
                key.precision(4);
                if (p.arc) key << "A" << p.c.x() << "," << p.c.y() << "," << std::min(p.a0, p.a1) << "," << std::max(p.a0, p.a1);
                else key << "L" << std::min(p.p0.x(), p.p1.x()) << "," << std::min(p.p0.y(), p.p1.y()) << ","
                         << std::max(p.p0.x(), p.p1.x()) << "," << std::max(p.p0.y(), p.p1.y());
                if (!done.insert(key.str()).second) continue;
                if (p.arc) {
                    auto* t = new G4Tubs("groove_arc" + std::to_string(k++), kBendR - kGrooveHalfW, kBendR + kGrooveHalfW, hz,
                                         std::min(p.a0, p.a1), std::abs(p.a1 - p.a0));
                    grooveSolid->AddNode(t, G4Transform3D(G4RotationMatrix(), G4ThreeVector(p.c.x(), p.c.y(), zc)));
                } else {
                    G4TwoVector d = p.p1 - p.p0, mid = 0.5*(p.p0 + p.p1);
                    auto* b = new G4Box("groove_line" + std::to_string(k++), 0.5*d.mag(), kGrooveHalfW, hz);
                    G4RotationMatrix r; r.rotateZ(std::atan2(d.y(), d.x()));
                    grooveSolid->AddNode(b, G4Transform3D(r, G4ThreeVector(mid.x(), mid.y(), zc)));
                }
            }
        }
        grooveSolid->Voxelize();
    }
    auto* lGroove = new G4LogicalVolume(grooveSolid, glueMat, "Ej500_groove");
    // Invisibile nella visualizzazione: per disegnare un G4MultiUnion Geant4
    // unisce i poligoni dei ~25 pezzi uno alla volta e, quando un'unione
    // fallisce ("too many edges"), riprova cambiando l'ordine dei pezzi:
    // con questo numero di pezzi la GUI resta bloccata. La navigazione delle
    // particelle non usa il poligono e non ne risente. Non renderlo visibile.
    lGroove->SetVisAttributes(G4VisAttributes::GetInvisible());
    new G4PVPlacement(nullptr, G4ThreeVector(), lGroove, "pEj500_groove", lScint, false, 0, check);

    // --- Cilindro acrilico del bundle (BundleTop + gambo M10) con il foro
    //     riempito di EJ-500. Asse locale z = asse x del mondo.
    G4RotationMatrix rotToX; rotToX.rotateY(90*deg);           // attiva: z locale -> x
    const G4ThreeVector bundlePos(0, kBundleY, kBundleZ);
    {
        const G4double zA[5] = {kXRodEnd, kXRodChamfer, kXHeadBack, kXHeadBack, kXMouth};
        const G4double rI[5] = {0, 0, 0, 0, 0};
        const G4double rA[5] = {kRodEndR, kRodR, kRodR, kHeadR, kHeadR};
        auto* lAcryl = new G4LogicalVolume(new G4Polycone("Acrylic_holder", 0, 360*deg, 5, zA, rI, rA),
                                           acrylMat, "Acrylic_holder");
        lAcryl->SetVisAttributes(acrylVis);
        new G4PVPlacement(G4Transform3D(rotToX, bundlePos - kEnvC), lAcryl, "pAcrylic_holder", lEnv, false, 0, check);

        const G4double zG[3] = {kXRodEnd, kXFunnelEnd, kXMouth};
        const G4double rIG[3] = {0, 0, 0};
        const G4double rG[3] = {kBoreR, kBoreR, kFunnelR};
        auto* lBGlue = new G4LogicalVolume(new G4Polycone("Ej500_bundle", 0, 360*deg, 3, zG, rIG, rG),
                                           glueMat, "Ej500_bundle");
        lBGlue->SetVisAttributes(glueVis);
        new G4PVPlacement(nullptr, G4ThreeVector(), lBGlue, "pEj500_bundle", lAcryl, false, 0, check);

        // --- Fibre ----------------------------------------------------------
        // Madri dei tratti: groove (x > bordo), Tyvek, HDPE, aria, colla del bundle.
        // Tutte sono centrate nell'origine senza rotazione tranne la colla del
        // bundle, il cui sistema locale si ottiene con l'inversa di (rotToX, bundlePos),
        // e l'aria dell'involucro, centrata in kEnvC.
        enum Region { kGlueB = 0, kAir, kHdpe, kTyv, kGroove };
        auto regionOf = [&](G4double x) {
            if (x < kXMouth) return kGlueB;
            if (x < kXWrap)  return kAir;
            if (x < kXTyvek) return kHdpe;
            if (x < kXEdge)  return kTyv;
            return kGroove;
        };
        G4LogicalVolume* motherOf[5] = {lBGlue, lEnv, lHdpe, lTyvek, lGroove};
        const G4RotationMatrix toLocal = rotToX.inverse();

        std::map<G4String, std::vector<G4ThreeVector>> allFibers;
        for (const auto& f : layout.fibers) {
            ZProfile prof(f.knots);
            std::vector<G4ThreeVector> panel = DiscretizePanel(f.path, prof);
            std::vector<G4ThreeVector> extA = External(panel.front(), f.slotA, layout.psi0, f.dEndA);
            std::vector<G4ThreeVector> extB = External(panel.back(),  f.slotB, layout.psi0, f.dEndB);
            std::vector<G4ThreeVector> pts(extA.rbegin(), extA.rend());
            pts.insert(pts.end(), panel.begin() + 1, panel.end());
            pts.insert(pts.end(), extB.begin() + 1, extB.end());
            allFibers[f.name] = pts;

            // Spezza la polilinea nei tratti che stanno nella stessa madre
            size_t start = 0;
            G4int piece = 0;
            for (size_t k = 0; k + 1 < pts.size(); ++k) {
                Region r = regionOf(0.5*(pts[k].x() + pts[k+1].x()));
                G4bool last = (k + 2 == pts.size()) || regionOf(0.5*(pts[k+1].x() + pts[k+2].x())) != r;
                if (!last) continue;
                std::vector<G4ThreeVector> sub(pts.begin() + start, pts.begin() + k + 2);
                // estremi del tratto sulle facce x=cost delle madri
                G4ThreeVector d0 = (sub[1] - sub[0]).unit(), d1 = (sub.back() - sub[sub.size()-2]).unit();
                G4ThreeVector nLow(d0.x() > 0 ? -1 : 1, 0, 0), nHigh(d1.x() > 0 ? 1 : -1, 0, 0);
                if (r == kGlueB) {
                    for (auto& q : sub) q = toLocal*(q - bundlePos);
                    nLow = toLocal*nLow; nHigh = toLocal*nHigh;
                } else if (r == kAir) {
                    for (auto& q : sub) q -= kEnvC;
                }
                PlaceChain(sub, nLow, nHigh, motherOf[r], "Fib_" + f.name + "_P" + std::to_string(piece++),
                           cladMat, coreMat, cladVis, coreVis, checkFibers);
                start = k + 1;
            }
        }
        CheckFiberClearance(allFibers);
    }

    // --- Lettura: grasso ottico, SiPM S13360-6075PE, PCB ------------------
    // Box a partire dagli spigoli CAD (minimo, massimo); senza madre va
    // nell'involucro
    auto placeBox = [&](const G4String& name, G4Material* mat, G4VisAttributes* vis,
                        G4ThreeVector a, G4ThreeVector b, G4LogicalVolume* mother = nullptr,
                        G4ThreeVector motherCenter = G4ThreeVector()) {
        if (!mother) { mother = lEnv; motherCenter = kEnvC; }
        G4ThreeVector h = 0.5*(b - a);
        auto* lv = new G4LogicalVolume(new G4Box(name, h.x(), h.y(), h.z()), mat, name);
        lv->SetVisAttributes(vis);
        new G4PVPlacement(nullptr, 0.5*(a + b) - motherCenter, lv, "p" + name, mother, false, 0, check);
        return lv;
    };
    // Grasso tra l'estremita' della barra (fibre a filo) e la faccia del package
    auto* greaseVis = new G4VisAttributes(G4Colour(0.9, 0.9, 0.2, 0.4));
    placeBox("Optical_grease", greaseMat, greaseVis, Cad(-264.694, 347.741, 261.099), Cad(-264.07, 354.591, 268.449));
    // Package 6.85x7.35mm (opaco), finestra di epoxy 0.3mm e area attiva 6x6mm
    G4ThreeVector pkgA = Cad(-266.764, 347.741, 261.099), pkgB = Cad(-264.694, 354.591, 268.449);
    auto* lPkg = placeBox("Sensor_package", pkgMat, pcbVis, pkgA, pkgB);
    G4ThreeVector pkgC = 0.5*(pkgA + pkgB);
    placeBox("Sensor_window", winMat, greaseVis, Cad(-264.994, 348.166, 262.024), Cad(-264.694, 354.166, 268.024), lPkg, pkgC);
    G4ThreeVector siA = Cad(-265.094, 348.166, 262.024), siB = Cad(-264.994, 354.166, 268.024);
    G4ThreeVector siH = 0.5*(siB - siA);
    // I fotoni che arrivano al SiPM li conta SteppingAction, come nel MC della
    // collega: arrivato = il passo finisce qui, rivelato = Detection (PDE)
    auto* lSiPM = new G4LogicalVolume(new G4Box("SiPM", siH.x(), siH.y(), siH.z()), siMat, "Pan_SiPM");
    fSiPM = lSiPM;
    lSiPM->SetVisAttributes(sipmVis);
    new G4PVPlacement(nullptr, 0.5*(siA + siB) - pkgC, lSiPM, "pSiPM", lPkg, false, 0, check);

    auto* opSiPM = new G4OpticalSurface("SiPMSurface", unified, polished, dielectric_metal);
    auto* mptSiPM = new G4MaterialPropertiesTable();
    std::vector<G4double> eSiPM = {2.00*eV, 2.30*eV, 2.50*eV, 2.70*eV, 2.90*eV, 3.10*eV, 3.30*eV, 3.50*eV};
    // PDE S13360-6075PE (picco 50% a 450nm): curva della -6050 usata nelle
    // altre geometrie riscalata di 50/40 (stessa forma, picco del datasheet).
    std::vector<G4double> pde = {0.19, 0.31, 0.375, 0.475, 0.50, 0.44, 0.31, 0.125};
    mptSiPM->AddProperty("REFLECTIVITY", eSiPM, std::vector<G4double>(eSiPM.size(), 0.));
    mptSiPM->AddProperty("EFFICIENCY", eSiPM, pde);
    opSiPM->SetMaterialPropertiesTable(mptSiPM);
    new G4LogicalSkinSurface("SiPMSkin", lSiPM, opSiPM);

    placeBox("Pcb_board", fr4Mat, pcbVis, Cad(-268.764, 331.166, 245.024), Cad(-266.764, 371.166, 285.024));

    // --- Dado sottile M10 (17mm sulle chiavi, 5mm), a filo dell'estremita' della barra
    {
        const G4double zN[2] = {kXRodEnd, Cad(-259.07, 0, 0).x()};
        const G4double rIN[2] = {0, 0}, rON[2] = {8.5*mm, 8.5*mm};
        auto* hex = new G4Polyhedra("nut_hex", 0, 360*deg, 6, 2, zN, rIN, rON);
        auto* hole = new G4Tubs("nut_hole", 0, kRodR, 10*mm, 0, 360*deg);
        auto* sNut = new G4SubtractionSolid("Steel_nut_m10", hex, hole, nullptr, G4ThreeVector(0, 0, 0.5*(zN[0] + zN[1])));
        auto* lNut = new G4LogicalVolume(sNut, steelMat, "Steel_nut_m10");
        lNut->SetVisAttributes(steelVis);
        new G4PVPlacement(G4Transform3D(rotToX, bundlePos - kEnvC), lNut, "pSteel_nut_m10", lEnv, false, 0, check);
    }

    // --- Housing di alluminio e schiume di neoprene (quote CAD) -------------
    // Fogli e schiume sopra e sotto
    const G4double zSheets[4][2] = {{233.274, 234.861}, {295.186, 296.774},     // alluminio
                                    {234.861, 236.449}, {293.599, 295.186}};    // neoprene
    for (G4int i = 0; i < 4; ++i)
        placeBox(i < 2 ? "Al_sheet" : "Neoprene_foam_sheet", i < 2 ? alMat : foamMat, i < 2 ? alVis : foamVis,
                 Cad(-281.295, 46.441, zSheets[i][0]), Cad(1219.155, 655.891, zSheets[i][1]));
    // Long U-channel ai lati (anima verso il pannello, ali verso l'esterno)
    for (G4int side = 0; side < 2; ++side) {
        G4double yWebIn  = side == 0 ? 68.666 : 633.666;          // faccia interna dell'anima
        G4double yWebOut = side == 0 ? 65.491 : 636.841;
        G4double yOuter  = side == 0 ? 46.441 : 655.891;
        placeBox("Al_long_uchannel_web", alMat, alVis, Cad(-281.295, std::min(yWebIn, yWebOut), 239.624),
                 Cad(1219.155, std::max(yWebIn, yWebOut), 290.424));
        for (G4double z : {236.449, 290.424})
            placeBox("Al_long_uchannel_flange", alMat, alVis, Cad(-281.295, std::min(yWebIn, yOuter), z),
                     Cad(1219.155, std::max(yWebIn, yOuter), z + 3.175));
        // schiuma sull'anima, tra i due short U-channel
        G4double yFoam = side == 0 ? 70.254 : 632.079;
        placeBox("Neoprene_foam_side", foamMat, foamVis, Cad(-259.07, std::min(yWebIn, yFoam), 236.449),
                 Cad(1196.93, std::max(yWebIn, yFoam), 293.599));
    }
    // Short U-channel lontano dal bundle, con la sua schiuma
    placeBox("Al_short_uchannel_web", alMat, alVis, Cad(1196.93, 68.666, 239.624), Cad(1200.105, 633.666, 290.424));
    for (G4double z : {236.449, 290.424})
        placeBox("Al_short_uchannel_flange", alMat, alVis, Cad(1196.93, 68.666, z), Cad(1219.155, 633.666, z + 3.175));
    placeBox("Neoprene_foam_end", foamMat, foamVis, Cad(1195.342, 70.254, 236.449), Cad(1196.93, 632.079, 293.599));
    // Short U-channel con la finestra 160x49.5mm (angoli raggiati R6.35 nel CAD, qui retti)
    {
        G4ThreeVector a = Cad(-262.245, 68.666, 239.624), b = Cad(-259.07, 633.666, 290.424);
        G4ThreeVector wa = Cad(-262.245, 271.166, 240.259), wb = Cad(-259.07, 431.166, 289.789);
        G4ThreeVector h = 0.5*(b - a), wh = 0.5*(wb - wa);
        auto* web = new G4Box("window_web", h.x(), h.y(), h.z());
        auto* win = new G4Box("window_cut", wh.x() + 1*mm, wh.y(), wh.z());
        auto* sWeb = new G4SubtractionSolid("Al_window_uchannel_web", web, win, nullptr, 0.5*(wa + wb) - 0.5*(a + b));
        auto* lWeb = new G4LogicalVolume(sWeb, alMat, "Al_window_uchannel_web");
        lWeb->SetVisAttributes(alVis);
        new G4PVPlacement(nullptr, 0.5*(a + b) - kEnvC, lWeb, "pAl_window_uchannel_web", lEnv, false, 0, check);
        for (G4double z : {236.449, 290.424})
            placeBox("Al_window_uchannel_flange", alMat, alVis, Cad(-281.295, 68.666, z), Cad(-259.07, 633.666, z + 3.175));
    }
    // Staffe angolari (L 3mm) agli angoli del bordo d'uscita del pannello
    placeBox("Al_corner_bracket", alMat, alVis, Cad(-135.77, 70.254, 240.024), Cad(-60.77, 73.254, 290.024));
    placeBox("Al_corner_bracket", alMat, alVis, Cad(-63.77, 73.254, 240.024), Cad(-60.77, 120.254, 290.024));
    placeBox("Al_corner_bracket", alMat, alVis, Cad(-135.77, 629.079, 240.024), Cad(-60.77, 632.079, 290.024));
    placeBox("Al_corner_bracket", alMat, alVis, Cad(-63.77, 582.079, 240.024), Cad(-60.77, 629.079, 290.024));
    // Plate del bundle (200x55x6.35mm) con il foro r=5.1mm per il gambo M10
    {
        G4ThreeVector a = Cad(-259.07, 251.166, 237.524), b = Cad(-252.72, 451.166, 292.524);
        G4ThreeVector h = 0.5*(b - a), c = 0.5*(a + b);
        auto* hole = new G4Tubs("plate_hole", 0, kPlateHoleR, h.x() + 1*mm, 0, 360*deg);
        G4RotationMatrix* rh = new G4RotationMatrix(); rh->rotateY(90*deg);
        auto* sPlate = new G4SubtractionSolid("Al_fiber_plate", new G4Box("plate_box", h.x(), h.y(), h.z()), hole, rh,
                                              G4ThreeVector(0, kBundleY - c.y(), kBundleZ - c.z()));
        auto* lPlate = new G4LogicalVolume(sPlate, alMat, "Al_fiber_plate");
        lPlate->SetVisAttributes(alVis);
        new G4PVPlacement(nullptr, c - kEnvC, lPlate, "pAl_fiber_plate", lEnv, false, 0, check);
    }
    // Cup dietro la finestra (U-channel e coperchi) e plate della PCB
    placeBox("Al_cup_end_web", alMat, alVis, Cad(-424.03, 302.081, 244.389), Cad(-420.855, 400.256, 285.664));
    for (G4double z : {244.389, 282.489})
        placeBox("Al_cup_end_flange", alMat, alVis, Cad(-420.855, 305.256, z), Cad(-410.855, 397.081, z + 3.175));
    placeBox("Al_cup_side_web", alMat, alVis, Cad(-420.855, 302.081, 244.389), Cad(-259.07, 305.256, 285.664));
    placeBox("Al_cup_side_web", alMat, alVis, Cad(-420.855, 397.081, 244.389), Cad(-259.07, 400.256, 285.664));
    for (G4double z : {244.389, 282.489}) {
        placeBox("Al_cup_side_flange", alMat, alVis, Cad(-410.855, 305.256, z), Cad(-259.07, 315.256, z + 3.175));
        placeBox("Al_cup_side_flange", alMat, alVis, Cad(-410.855, 387.081, z), Cad(-259.07, 397.081, z + 3.175));
    }
    placeBox("Al_cup_cover", alMat, alVis, Cad(-424.03, 302.081, 242.801), Cad(-259.07, 400.256, 244.389));
    placeBox("Al_cup_cover", alMat, alVis, Cad(-424.03, 302.081, 285.664), Cad(-259.07, 400.256, 287.251));
    {
        G4ThreeVector a = Cad(-274.657, 323.166, 245.024), b = Cad(-273.07, 379.166, 285.024);
        G4ThreeVector wa = Cad(-274.657, 337.166, 257.024), wb = Cad(-273.07, 365.166, 273.024);
        G4ThreeVector h = 0.5*(b - a), wh = 0.5*(wb - wa);
        auto* s = new G4SubtractionSolid("Al_pcb_plate", new G4Box("pcbplate_box", h.x(), h.y(), h.z()),
                                         new G4Box("pcbplate_cut", wh.x() + 1*mm, wh.y(), wh.z()), nullptr,
                                         0.5*(wa + wb) - 0.5*(a + b));
        auto* lv = new G4LogicalVolume(s, alMat, "Al_pcb_plate");
        lv->SetVisAttributes(alVis);
        new G4PVPlacement(nullptr, 0.5*(a + b) - kEnvC, lv, "pAl_pcb_plate", lEnv, false, 0, check);
    }

    return lEnv;
}
