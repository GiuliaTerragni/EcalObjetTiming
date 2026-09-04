#ifndef EcalObjetTiming_Producers_EcalTimeTools_h
#define EcalObjetTiming_Producers_EcalTimeTools_h

#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"

// time: EcalRecHit::time() (OOT-corrected).
// timeNoOOTCorr: EcalRecHit::nonCorrectedTime() (not corrected for
// out-of-time pileup), read from the same crystal(s) as time.
struct EcalTimeResult {
  float time;
  float timeNoOOTCorr;
};

// A fixed placeholder value, used to check that the produce-and-set
// plumbing works, independently of any real timing computation.
EcalTimeResult ecalTimeFixed();

// Time of the seed crystal of the SuperCluster (the single crystal with the
// highest energy), read from whichever RecHit collection (EB or EE)
// actually contains that crystal.
EcalTimeResult ecalTimeSeedCrystal(const reco::SuperCluster& sc,
                                    const EcalRecHitCollection& recHitsEB,
                                    const EcalRecHitCollection& recHitsEE);

// Energy-weighted average time over all crystals of all BasicClusters in
// the SuperCluster (weight = fraction * crystal energy).
EcalTimeResult ecalTimeEnergyWeighted(const reco::SuperCluster& sc,
                                       const EcalRecHitCollection& recHitsEB,
                                       const EcalRecHitCollection& recHitsEE);

// Same as ecalTimeEnergyWeighted, but weighting by sqrt(energy) instead of
// energy, to reduce the influence of the most energetic crystals.
EcalTimeResult ecalTimeSqrtEnergyWeighted(const reco::SuperCluster& sc,
                                           const EcalRecHitCollection& recHitsEB,
                                           const EcalRecHitCollection& recHitsEE);

// Shared by pat::Photon, pat::Electron, reco::Photon and reco::GsfElectron,
// since they all expose a SuperCluster via superCluster().

#endif
