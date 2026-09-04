#include "EcalObjetTiming/Producers/interface/EcalTimeTools.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/Common/interface/Ptr.h"

#include <cmath>

namespace {

  const EcalRecHitCollection& recHitsFor(const DetId& id,
                                          const EcalRecHitCollection& recHitsEB,
                                          const EcalRecHitCollection& recHitsEE) {
    return (id.subdetId() == EcalBarrel) ? recHitsEB : recHitsEE;
  }

  // Energy-weighted average of time and timeNoOOTCorr over all crystals of
  // the SuperCluster, using weight = fraction * pow(crystal energy, energyPower).
  EcalTimeResult weightedEcalTime(const reco::SuperCluster& sc,
                                   const EcalRecHitCollection& recHitsEB,
                                   const EcalRecHitCollection& recHitsEE,
                                   float energyPower) {
    double sumWeight = 0.;
    double sumWeightedTime = 0.;
    double sumWeightedTimeNoOOTCorr = 0.;

    // sc.clustersBegin()/clustersEnd() throw if the underlying BasicCluster
    // PtrVector refers to a product that isn't available in this file (can
    // happen in MiniAOD). Checking availability on the whole PtrVector first
    // avoids that: per-element checks inside the loop are too late, since
    // begin()/end() themselves already resolve (and validate) every element.
    if (!sc.clusters().isAvailable())
      return {-999., -999.};

    for (reco::CaloCluster_iterator bc = sc.clustersBegin(); bc != sc.clustersEnd(); ++bc) {
      const edm::Ptr<reco::CaloCluster>& clusterPtr = *bc;

      const std::vector<std::pair<DetId, float>>& hitsAndFractions = clusterPtr->hitsAndFractions();
      for (const auto& hitAndFraction : hitsAndFractions) {
        const DetId& id = hitAndFraction.first;
        const float fraction = hitAndFraction.second;

        const EcalRecHitCollection& recHits = recHitsFor(id, recHitsEB, recHitsEE);
        EcalRecHitCollection::const_iterator it = recHits.find(id);
        if (it == recHits.end())
          continue;

        const float energy = fraction * it->energy();
        const float weight = std::pow(energy, energyPower);
        sumWeight += weight;
        sumWeightedTime += weight * it->time();
        sumWeightedTimeNoOOTCorr += weight * it->nonCorrectedTime();
      }
    }

    if (sumWeight <= 0.)
      return {-999., -999.};

    return {static_cast<float>(sumWeightedTime / sumWeight), static_cast<float>(sumWeightedTimeNoOOTCorr / sumWeight)};
  }

}  // namespace

EcalTimeResult ecalTimeFixed() { return {3.0, -3.0}; }

EcalTimeResult ecalTimeSeedCrystal(const reco::SuperCluster& sc,
                                    const EcalRecHitCollection& recHitsEB,
                                    const EcalRecHitCollection& recHitsEE) {
  if (!sc.seed().isAvailable())
    return {-999., -999.};

  const DetId seedId = sc.seed()->seed();

  const EcalRecHitCollection& recHits = recHitsFor(seedId, recHitsEB, recHitsEE);

  EcalRecHitCollection::const_iterator it = recHits.find(seedId);
  if (it == recHits.end())
    return {-999., -999.};

  return {it->time(), it->nonCorrectedTime()};
}

EcalTimeResult ecalTimeEnergyWeighted(const reco::SuperCluster& sc,
                                      const EcalRecHitCollection& recHitsEB,
                                      const EcalRecHitCollection& recHitsEE) {
  return weightedEcalTime(sc, recHitsEB, recHitsEE, 1.0);
}

EcalTimeResult ecalTimeSqrtEnergyWeighted(const reco::SuperCluster& sc,
                                          const EcalRecHitCollection& recHitsEB,
                                          const EcalRecHitCollection& recHitsEE) {
  return weightedEcalTime(sc, recHitsEB, recHitsEE, 0.5);
}
