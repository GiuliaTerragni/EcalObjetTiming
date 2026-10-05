#ifndef EcalObjetTiming_Producers_EcalTimeTools_h
#define EcalObjetTiming_Producers_EcalTimeTools_h

#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/Common/interface/Ptr.h"

#include <cmath>
#include <vector>


struct EcalTimeResult {
  float time;
  float timeError;
  float timeNoOOTCorr;
  float timeNoOOTCorrError;
};

// Apply a timing function to all objects in a collection.
const EcalRecHitCollection& recHitsCollection(
    const DetId& id,
    const EcalRecHitCollection& recHitsEB,
    const EcalRecHitCollection& recHitsEE) {

  return (id.subdetId() == EcalBarrel) ? recHitsEB : recHitsEE;
}

EcalTimeResult computeEcalTime(
    const reco::SuperCluster& sc,
    const EcalRecHitCollection& recHitsEB,
    const EcalRecHitCollection& recHitsEE,
    std::string method) {

  double sumWeight = 0.;
  double sumWeightedTime = 0.;
  double sumWeightedTimeVariance = 0.;
  double sumWeightNoOOTCorr = 0.;
  double sumWeightedTimeNoOOTCorr = 0.;
  double sumWeightedTimeNoOOTCorrVariance = 0.;

  if (!sc.clusters().isAvailable())
    return {-999., -999., -999., -999.};
  
  if(method=="TimeSeed") {  
  
     const DetId seedId = sc.seed()->seed();  
     const EcalRecHitCollection& recHitsSeed = recHitsCollection(seedId, recHitsEB, recHitsEE);
     EcalRecHitCollection::const_iterator it = recHitsSeed.find(seedId);
     if (it == recHitsSeed.end())
        return {-999., -999., -999., -999.};
        
     const float time = it->time();
     const float timeError = it->timeError();
     const float timeNoOOTCorr = it->nonCorrectedTime();
     const float timeNoOOTCorrError = std::abs(ecalcctiming::nonCorrectedSlope) * timeError;
     
     return {time, timeError, timeNoOOTCorr, timeNoOOTCorrError};
     
  } else {
  
     for (reco::CaloCluster_iterator bc = sc.clustersBegin(); bc != sc.clustersEnd(); ++bc) {

       const edm::Ptr<reco::CaloCluster>& clusterPtr = *bc;
       const std::vector<std::pair<DetId, float>>& hitsAndFractions = clusterPtr->hitsAndFractions();

       for (const auto& hitAndFraction : hitsAndFractions) {

         const DetId& id = hitAndFraction.first;
         const EcalRecHitCollection& recHits = recHitsCollection(id, recHitsEB, recHitsEE);
         EcalRecHitCollection::const_iterator it = recHits.find(id);
         if (it == recHits.end())
            continue;

         const double energy = it->energy();
         double weightEB = 1.;
         double weightEE = 1.;
         double weightNoOOTCorrEB = 1.;
         double weightNoOOTCorrEE = 1.;
         if (method=="Time") { 
             weightEB = 1.;
             weightEE = 1.;
             weightNoOOTCorrEB = 1.;   
             weightNoOOTCorrEE = 1.;   
         } else if (method=="TimeEnergy") { 
             weightEB = energy;
             weightEE = energy;
             weightNoOOTCorrEB = energy;
             weightNoOOTCorrEE = energy;
         } else if (method=="TimeSqrEnergy") { 
             weightEB = std::pow(energy,2.);
             weightEE = std::pow(energy,2.);
             weightNoOOTCorrEB = std::pow(energy,2.);
             weightNoOOTCorrEE = std::pow(energy,2.);
         } else if (method=="TimeSqrtEnergy") { 
             weightEB = std::pow(energy,0.5);
             weightEE = std::pow(energy,0.5);
             weightNoOOTCorrEB = std::pow(energy,0.5);
             weightNoOOTCorrEE = std::pow(energy,0.5);
         } else if (method=="TimeResEnergy") { 
             weightEB = energy*energy/(15.14*15.14 + 0.98*0.98*energy + 2*0.19*0.19*energy*energy);
             weightEE = energy*energy/(18.75*18.75 + 3.65*3.65*energy + 2*0.13*0.13*energy*energy);
             weightNoOOTCorrEB = energy*energy/(27.31*27.31 + 0.00*0.00*energy + 2*0.10*0.10*energy*energy);
             weightNoOOTCorrEE = energy*energy/(26.90*26.90 + 6.48*6.48*energy + 2*0.13*0.13*energy*energy);
         } else {
            std::cout << "WARNING EcalTimeResult::computeEcalTime wrong method " << method << "! Please choose among these: 'Time', 'TimeEnergy', 'TimeSqrEnergy', 'TimeSqrtEnergy', 'TimeResEnergy' or 'TimeSeed'. " << std::endl;
            return {-999., -999., -999., -999.};    
         }
         const double time = it->time();
         const double timeError = it->timeError();
         const double timeNoOOTCorr = it->nonCorrectedTime();
         // nonCorrectedTime = nonCorrectedSlope * time + constant
         const double timeNoOOTCorrError = std::abs(ecalcctiming::nonCorrectedSlope) * timeError;

         if (id.subdetId() == EcalBarrel) { 
            sumWeight += weightEB;
            sumWeightedTime += weightEB * time;
            sumWeightedTimeVariance += weightEB * weightEB * timeError * timeError;
            sumWeightNoOOTCorr += weightNoOOTCorrEB;
            sumWeightedTimeNoOOTCorr += weightNoOOTCorrEB * timeNoOOTCorr;
            sumWeightedTimeNoOOTCorrVariance += weightNoOOTCorrEB * weightNoOOTCorrEB * timeNoOOTCorrError * timeNoOOTCorrError;
         } else {
            sumWeight += weightEE;
            sumWeightedTime += weightEE * time;
            sumWeightedTimeVariance += weightEE * weightEE * timeError * timeError; 
            sumWeightNoOOTCorr += weightNoOOTCorrEE;
            sumWeightedTimeNoOOTCorr += weightNoOOTCorrEE * timeNoOOTCorr;
            sumWeightedTimeNoOOTCorrVariance += weightNoOOTCorrEE * weightNoOOTCorrEE * timeNoOOTCorrError * timeNoOOTCorrError;        
         }
       }
     }

     const float weightedTime = sumWeightedTime / sumWeight;
     const float weightedTimeError = std::sqrt(sumWeightedTimeVariance) / sumWeight;
     const float weightedTimeNoOOTCorr = sumWeightedTimeNoOOTCorr / sumWeightNoOOTCorr;
     const float weightedTimeNoOOTCorrError = std::sqrt(sumWeightedTimeNoOOTCorrVariance) / sumWeightNoOOTCorr;

     if (sumWeight <= 0. && sumWeightNoOOTCorr <= 0.)
        return {-999., -999., -999., -999.};
     else if (sumWeight > 0. && sumWeightNoOOTCorr <= 0.)   
        return {-999., -999., weightedTimeError, weightedTimeNoOOTCorrError};
     else if (sumWeight <= 0. && sumWeightNoOOTCorr > 0.)   
        return {weightedTime, weightedTimeNoOOTCorr, -999., -999.};
     else if (sumWeight > 0. && sumWeightNoOOTCorr > 0.)   
        return {weightedTime, weightedTimeNoOOTCorr, weightedTimeError, weightedTimeNoOOTCorrError};      
  }
  return {-999., -999., -999., -999.};
}

void setEcalTime(
    std::vector<reco::SuperCluster>& objects,
    const EcalRecHitCollection& recHitsEB,
    const EcalRecHitCollection& recHitsEE,
    std::string method) {

  for (auto& obj : objects) {
    const EcalTimeResult result = computeEcalTime(obj, recHitsEB, recHitsEE, method);
    obj.setEcalTime(result.time);
    obj.setEcalTimeError(result.timeError);
    obj.setEcalTimeNoOOTCorr(result.timeNoOOTCorr);
    obj.setEcalTimeNoOOTCorrError(result.timeNoOOTCorrError);
  }
}

template <typename T>
void setEcalTime(
    std::vector<T>& objects,
    const EcalRecHitCollection& recHitsEB,
    const EcalRecHitCollection& recHitsEE,
    std::string method) {

  for (auto& obj : objects) {
    const EcalTimeResult result = computeEcalTime(*obj.superCluster(), recHitsEB, recHitsEE, method);
    obj.setEcalTime(result.time);
    obj.setEcalTimeError(result.timeError);
    obj.setEcalTimeNoOOTCorr(result.timeNoOOTCorr);
    obj.setEcalTimeNoOOTCorrError(result.timeNoOOTCorrError);
  }
}

#endif
