#ifndef EcalObjetTiming_Producers_EcalTimeTools_h
#define EcalObjetTiming_Producers_EcalTimeTools_h

#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/CaloRecHit/interface/CaloCluster.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlock.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElement.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/CaloJet.h"
#include "CondFormats/DataRecord/interface/EcalPedestalsRcd.h"
#include "CondFormats/EcalObjects/interface/EcalPedestals.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsRcd.h"
#include "CondFormats/EcalObjects/interface/EcalIntercalibConstants.h"
#include "CondFormats/DataRecord/interface/EcalADCToGeVConstantRcd.h"
#include "CondFormats/EcalObjects/interface/EcalADCToGeVConstant.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbRecord.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbService.h"
#include "CondFormats/DataRecord/interface/EcalTimeCalibConstantsRcd.h"
#include "CondFormats/EcalObjects/interface/EcalTimeCalibConstants.h"
#include "CondFormats/DataRecord/interface/EcalTimeOffsetConstantRcd.h"
#include "CondFormats/EcalObjects/interface/EcalTimeOffsetConstant.h"

#include <cmath>
#include <vector>
#include <algorithm>

struct EcalCaloTower {
    CaloTowerDetId towerId;
    std::vector<DetId> detIds;
    double energy = -999.;
};

struct EcalTimeResult {
  double time;
  double timeError;
  double timeNoOOTCorr;
  double timeNoOOTCorrError;
};

inline const EcalRecHitCollection& recHitsCollection(
    const DetId& id,
    const EcalRecHitCollection& recHitsEB,
    const EcalRecHitCollection& recHitsEE) {

  return (id.subdetId() == EcalBarrel) ? recHitsEB : recHitsEE;
}

inline const std::vector<reco::CaloCluster> getEcalClusters(const reco::SuperCluster& sc) {
  std::vector<reco::CaloCluster> ecalClusters;
  for (const auto& clusterPtr : sc.clusters()) {
       if (clusterPtr.isNonnull()) {
           if (clusterPtr->seed().subdetId() != EcalBarrel && clusterPtr->seed().subdetId() != EcalEndcap)
               continue;
           if (std::find(ecalClusters.begin(), ecalClusters.end(), *clusterPtr) == ecalClusters.end())     
               ecalClusters.push_back(*clusterPtr);
       }
  }
  return ecalClusters;
}

inline const std::vector<reco::CaloCluster> getEcalClusters(const reco::Photon& photon) {
  std::vector<reco::CaloCluster> ecalClusters = getEcalClusters(*photon.superCluster());
  return ecalClusters;   
}

inline const std::vector<reco::CaloCluster> getEcalClusters(const reco::GsfElectron& electron) {
  std::vector<reco::CaloCluster> ecalClusters = getEcalClusters(*electron.superCluster());
  return ecalClusters;   
}

inline const std::vector<reco::PFCluster> getEcalClusters(const reco::PFJet& pfJet) {
  std::vector<reco::PFCluster> ecalClusters;
  for (const auto& pfcand : pfJet.getPFConstituents()) {
       if (pfcand.isNull())
           continue;
       for (const auto& blockElement : pfcand->elementsInBlocks()) {
            const reco::PFBlockRef& blockRef = blockElement.first;
            const unsigned elementIndex = blockElement.second;
            if (blockRef.isNull())
                continue;
            if (elementIndex >= blockRef->elements().size())
                continue;
            const reco::PFBlockElement& element = blockRef->elements()[elementIndex];
            if (element.type() != reco::PFBlockElement::ECAL)
                continue;
            const reco::PFClusterRef& clusterRef = element.clusterRef();
            if (clusterRef.isNull())
                continue;
            if (clusterRef->layer() != PFLayer::ECAL_BARREL && clusterRef->layer() != PFLayer::ECAL_ENDCAP)    
                continue;
            if (std::find(ecalClusters.begin(), ecalClusters.end(), *clusterRef) == ecalClusters.end())     
                ecalClusters.push_back(*clusterRef);
       }
  }
  return ecalClusters;
}

inline const std::vector<EcalCaloTower> getEcalClusters(const reco::CaloJet& caloJet) {
  std::vector<DetId> ecalIds;
  std::vector<EcalCaloTower> ecalClusters;
  for (const auto& tower : caloJet.getCaloConstituents()) {
       if (tower.isNull())
           continue;
       for (const auto& id : tower->constituents()) {
            if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
                continue;
            if (std::find(ecalIds.begin(), ecalIds.end(), id) == ecalIds.end()) 
                ecalIds.push_back(id);
       }     
       EcalCaloTower  ecalTower = {tower->id(),ecalIds,tower->emEnergy()};
       ecalClusters.push_back(ecalTower); 
  }
  return ecalClusters;
}

inline const std::vector<DetId> getEcalIds(const reco::CaloCluster& caloCluster) {
  std::vector<DetId> ecalIds;
  const std::vector<std::pair<DetId, float>>& hitsAndFractions = caloCluster.hitsAndFractions();
  for (const auto& hitAndFraction : hitsAndFractions) {
       const DetId id = hitAndFraction.first;
       if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
           continue;
       if (std::find(ecalIds.begin(), ecalIds.end(), id) == ecalIds.end())     
       ecalIds.push_back(id);
  }
  return ecalIds;   
}

inline const std::vector<DetId> getEcalIds(const reco::PFCluster& pfCluster) {
  std::vector<DetId> ecalIds;
  if (pfCluster.layer() != PFLayer::ECAL_BARREL && pfCluster.layer() != PFLayer::ECAL_ENDCAP) {
    return ecalIds;
  }
  for (const auto& rhf : pfCluster.recHitFractions()) {
       const reco::PFRecHitRef& hitRef = rhf.recHitRef();
       if (hitRef.isNonnull()) {
           if (std::find(ecalIds.begin(), ecalIds.end(), (hitRef.get())->detId()) == ecalIds.end()) 
               ecalIds.push_back((hitRef.get())->detId());
       }    
  }
  return ecalIds;   
}

inline const std::vector<DetId> getEcalIds(const std::vector<reco::CaloCluster>& ecalClusters) {
  std::vector<DetId> ecalIds;
  for (const auto& cluster : ecalClusters) {
       const std::vector<std::pair<DetId, float>>& hitsAndFractions = cluster.hitsAndFractions();
       for (const auto& hitAndFraction : hitsAndFractions) {
            const DetId id = hitAndFraction.first;
            if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
                continue;
            if (std::find(ecalIds.begin(), ecalIds.end(), id) == ecalIds.end()) 
                ecalIds.push_back(id);    
       } 
  }  
  return ecalIds;
}

inline const std::vector<DetId> getEcalIds(const std::vector<reco::PFCluster>& ecalPFClusters) {
  std::vector<DetId> ecalIds;
  for (const auto& pfCluster : ecalPFClusters) {
       if (pfCluster.layer() != PFLayer::ECAL_BARREL && pfCluster.layer() != PFLayer::ECAL_ENDCAP)    
           continue;
       for (const auto& rhf : pfCluster.recHitFractions()) {
            const reco::PFRecHitRef& hitRef = rhf.recHitRef();
            if (hitRef.isNonnull()) {
                if (std::find(ecalIds.begin(), ecalIds.end(), (hitRef.get())->detId()) == ecalIds.end()) 
                    ecalIds.push_back((hitRef.get())->detId());
            }    
       } 
  } 
  return ecalIds;
}

inline const std::vector<DetId> getEcalIds(const std::vector<EcalCaloTower>& ecalTowers) {
  std::vector<DetId> ecalIds;
  for (const auto& tower : ecalTowers) {
       for (const auto& id : tower.detIds) {
            if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
                continue;
            if (std::find(ecalIds.begin(), ecalIds.end(), id) == ecalIds.end()) 
                ecalIds.push_back(id);
       } 
  } 
  return ecalIds;
}

inline const std::vector<DetId> getEcalIds(const reco::SuperCluster& sc) {
  std::vector<DetId> ecalIds;
  if (sc.hitsAndFractions().size()>sc.seed()->hitsAndFractions().size()) {
      const auto& hitsAndFractions = sc.hitsAndFractions(); 
      for (const auto& hitAndFraction : hitsAndFractions) {
           const DetId id = hitAndFraction.first;
           if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
               continue;
           ecalIds.push_back(id);
      }
  } else {
     const auto& hitsAndFractions = sc.seed()->hitsAndFractions(); 
      for (const auto& hitAndFraction : hitsAndFractions) {
           const DetId id = hitAndFraction.first;
           if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
               continue;
           ecalIds.push_back(id);
      }
  }       
  return ecalIds;   
}

inline const std::vector<DetId> getEcalIds(const reco::Photon& photon) {
  std::vector<DetId> ecalIds = getEcalIds(*photon.superCluster());
  return ecalIds;   
}

inline const std::vector<DetId> getEcalIds(const reco::GsfElectron& electron) {
  std::vector<DetId> ecalIds = getEcalIds(*electron.superCluster());
  return ecalIds;   
}

inline const double getSeedEnergy(const std::vector<reco::CaloCluster>& ecalClusters) {
  double maxEnergy = -999.;
  for (const auto& cluster : ecalClusters) {
       if (cluster.seed().subdetId() != EcalBarrel && cluster.seed().subdetId() != EcalEndcap)
           continue;
       double energy = cluster.energy();
       if (energy > maxEnergy) {
           maxEnergy = energy;
       }
  }
  return maxEnergy;                      
}

inline const double getSeedEnergy(const std::vector<reco::PFCluster>& ecalPFClusters) {
  double maxEnergy = -999.;
  for (const auto& pfCluster : ecalPFClusters) {
       if (pfCluster.layer() != PFLayer::ECAL_BARREL && pfCluster.layer() != PFLayer::ECAL_ENDCAP)    
           continue;
       double energy = pfCluster.energy();
       if (energy > maxEnergy) {
           maxEnergy = energy;
       }
  }
  return maxEnergy;                      
}

inline const double getSeedEnergy(const std::vector<EcalCaloTower>& ecalTowers) {
  double maxEnergy = -999.;
  for (const auto& tower : ecalTowers) {
       double energy = tower.energy;
       if (energy > maxEnergy) {
           maxEnergy = energy;
       }
  }
  return maxEnergy;                      
}

inline const double getSeedEnergy(const std::vector<DetId>& ecalIds, 
                                  const EcalRecHitCollection& ebRecHits, 
                                  const EcalRecHitCollection& eeRecHits) {
  double maxEnergy = -999.;
  for (const auto& id : ecalIds) {
       if (id.subdetId() != EcalBarrel && id.subdetId() != EcalEndcap)
           continue;
       const EcalRecHit* recHit = nullptr;
       if (id.subdetId() == EcalBarrel) {
           auto it = ebRecHits.find(id);
           if (it != ebRecHits.end())
               recHit = &(*it);
       } else if (id.subdetId() == EcalEndcap) {
           auto it = eeRecHits.find(id);
           if (it != eeRecHits.end())
               recHit = &(*it);
       }
       if (recHit == nullptr)
           continue;
       double energy = recHit->energy();
       if (energy > maxEnergy) {
           maxEnergy = energy;
       }
  }
  return maxEnergy;                      
}

inline const double getAmpitudeOverNoise(const EcalRecHit& rechit, 
                                         const DetId& id,
                                         const EcalPedestals& pedestals,
                                         const EcalIntercalibConstants& ics, 
                                         const EcalADCToGeVConstant& adcToGeV,
                                         const EcalLaserDbService& laserCorrections,
                                         const edm::Timestamp& evtTime) {
  double energy = rechit.energy();                                
  double calibration = 1.;
  double noise = 1.;
  double icalconst = 1.;
  auto icalIt = ics.getMap().find(id);
  if (icalIt != ics.getMap().end())
      icalconst = *icalIt;
  double laserCalib = laserCorrections.getLaserCorrection(id,evtTime);  
  double adcToGeVconst = 1.;
  if (id.subdetId() == EcalBarrel)
      adcToGeVconst = adcToGeV.getEBValue();
  else if (id.subdetId() == EcalEndcap)
      adcToGeVconst = adcToGeV.getEEValue();  
  calibration = adcToGeVconst * laserCalib * icalconst;
  int pedestalGain = 1;  // G12
  if (rechit.checkFlag(EcalRecHit::kHasSwitchToGain6))
      pedestalGain = 2;  // G6
  else if (rechit.checkFlag(EcalRecHit::kHasSwitchToGain1))
      pedestalGain = 3;  // G1
  auto pedIt = pedestals.getMap().find(id);
  if (pedIt != pedestals.getMap().end())
      noise = pedIt->rms(pedestalGain);
  energy /= (calibration * noise);    
  return energy;           
}    
         
inline const double getRawTime(const double time, 
              const double timeCalibration) {
  return ((time-timeCalibration)/25.);
}

inline const double getCalibratedTimeNoOOTCorr(const EcalRecHit& rechit, 
                                               const DetId& id,
       					       const EcalTimeCalibConstants& timeics,
                         	               const EcalTimeOffsetConstant& timeoffset) {
  double time = rechit.nonCorrectedTime();                                
  /*double icalconst = 1.;
  auto icalIt = timeics.getMap().find(id);
  if (icalIt != timeics.getMap().end())
      icalconst = *icalIt;
  double timeOffsetConst = 0.;
  if (id.subdetId() == EcalBarrel)
      timeOffsetConst = timeoffset.getEBValue();
  else if (id.subdetId() == EcalEndcap)
      timeOffsetConst = timeoffset.getEEValue();  
  //calibrate time...
  time += icalconst + timeOffsetConst;
  // */ 
  return time;                         	        
}

inline const DetId getSeedId(const std::vector<DetId>& ecalIds, 
                      	     const EcalRecHitCollection& ebRecHits, 
                             const EcalRecHitCollection& eeRecHits,
                             const EcalPedestals& pedestals,
                             const EcalIntercalibConstants& ics, 
                             const EcalADCToGeVConstant& adcToGeV,
                             const EcalLaserDbService& laserCorrections,
                             const edm::Timestamp& evtTime,
                             bool useAmplitudeOverNoise = false) {
  DetId maxHit;
  double maxEnergy = -999.;
  for (const auto& id : ecalIds) {
       const EcalRecHit* recHit = nullptr;
       if (id.subdetId() == EcalBarrel) {
           auto it = ebRecHits.find(id);
           if (it != ebRecHits.end())
               recHit = &(*it);
       } else if (id.subdetId() == EcalEndcap) {
           auto it = eeRecHits.find(id);
           if (it != eeRecHits.end())
               recHit = &(*it);
       }
       if (recHit == nullptr)
           continue;
       double energy = recHit->energy();
       if (useAmplitudeOverNoise)
           energy = getAmpitudeOverNoise(*recHit,id, pedestals, ics, adcToGeV, laserCorrections, evtTime); 
       if (energy > maxEnergy) {
           maxEnergy = energy;
           maxHit = id;
       }
  }
  return maxHit;
}

template <typename T>
EcalTimeResult computeEcalSeedTime(const T& object, 
    				   const EcalRecHitCollection& ebRecHits, 
    				   const EcalRecHitCollection& eeRecHits,
    				   const EcalPedestals& pedestals,
                         	   const EcalIntercalibConstants& ics, 
                         	   const EcalADCToGeVConstant& adcToGeV,
                         	   const EcalLaserDbService& laserCorrections,
                         	   const EcalTimeCalibConstants& timeics,
                         	   const EcalTimeOffsetConstant& timeoffset,
                         	   const edm::Timestamp& evtTime,
                         	   bool useAmplitudeOverNoise = false,
				   bool calibrateTimeNoOOTCorr = false) {
  const std::vector<DetId> ecalIds = getEcalIds(object);  
  const DetId seedId = getSeedId(ecalIds, ebRecHits, eeRecHits, pedestals, ics, adcToGeV, laserCorrections, evtTime, useAmplitudeOverNoise);  
  const EcalRecHitCollection& recHits = recHitsCollection(seedId, ebRecHits, eeRecHits);
  EcalRecHitCollection::const_iterator it = recHits.find(seedId);
  if (it == recHits.end()) 
      return {-99., -99., -99., -99.};  
  const double time = it->time();
  const double timeError = it->timeError();
  const double timeNoOOTCorr = getCalibratedTimeNoOOTCorr(*it, seedId, timeics, timeoffset);
  const double timeNoOOTCorrError = std::abs(ecalcctiming::nonCorrectedSlope) * timeError;
  return {time, timeError, timeNoOOTCorr, timeNoOOTCorrError};    
}

template <typename T>
EcalTimeResult computeEcalWeightedTime(const T& object, 
				       const EcalRecHitCollection& ebRecHits, 
				       const EcalRecHitCollection& eeRecHits, 
				       const EcalPedestals& pedestals,
                         	       const EcalIntercalibConstants& ics, 
                         	       const EcalADCToGeVConstant& adcToGeV,
                         	       const EcalLaserDbService& laserCorrections,
                         	       const EcalTimeCalibConstants& timeics,
                         	       const EcalTimeOffsetConstant& timeoffset,
                         	       const edm::Timestamp& evtTime,
                         	       bool useAmplitudeOverNoise = false, 
				       bool calibrateTimeNoOOTCorr = false,
				       const double power = 1.,
				       const std::vector<double> ebA = {1.,1.}, //{15.14,27.31} 
				       const std::vector<double> ebB = {0.,0.}, //{ 0.98, 0.00} 
				       const std::vector<double> ebC = {0.,0.}, //{ 0.19, 0.10} 
				       const std::vector<double> eeA = {1.,1.}, //{18.75,26.90}  
				       const std::vector<double> eeB = {0.,0.}, //{ 3.65, 6.48} 
				       const std::vector<double> eeC = {0.,0.}  //{ 0.13, 0.13}
				      ) {
  double sumWeight = 0.;
  double sumWeightedTime = 0.;
  double sumWeightedTimeVariance = 0.;
  double sumWeightNoOOTCorr = 0.;
  double sumWeightedTimeNoOOTCorr = 0.;
  double sumWeightedTimeNoOOTCorrVariance = 0.;                                    
  const std::vector<DetId> ecalIds = getEcalIds(object);  
  for (const auto& id : ecalIds) {
       const EcalRecHitCollection& recHits = recHitsCollection(id, ebRecHits, eeRecHits);
       EcalRecHitCollection::const_iterator it = recHits.find(id);
       if (it == recHits.end())
           continue;
       double energy = it->energy();
       if (useAmplitudeOverNoise)
           energy = getAmpitudeOverNoise(*it, id, pedestals, ics, adcToGeV, laserCorrections, evtTime);     
       double sigmaEB = std::pow(std::sqrt(ebA[0]*ebA[0]/(energy*energy) + ebB[0]*ebB[0]/energy + 2*ebC[0]*ebC[0]),power);
       double sigmaEE = std::pow(std::sqrt(eeA[0]*eeA[0]/(energy*energy) + eeB[0]*eeB[0]/energy + 2*eeC[0]*eeC[0]),power);
       double sigmaNoOOTCorrEB = std::pow(std::sqrt(ebA[1]*ebA[1]/(energy*energy) + ebB[1]*ebB[1]/energy + 2*ebC[1]*ebC[1]),power);
       double sigmaNoOOTCorrEE = std::pow(std::sqrt(eeA[1]*eeA[1]/(energy*energy) + eeB[1]*eeB[1]/energy + 2*eeC[1]*eeC[1]),power);
       double weightEB = 1./(sigmaEB*sigmaEB);
       double weightEE = 1./(sigmaEE*sigmaEE);
       double weightNoOOTCorrEB = 1./(sigmaNoOOTCorrEB*sigmaNoOOTCorrEB); 
       double weightNoOOTCorrEE = 1./(sigmaNoOOTCorrEE*sigmaNoOOTCorrEE);
       const double time = it->time();
       const double timeError = it->timeError();
       const double timeNoOOTCorr = getCalibratedTimeNoOOTCorr(*it, id, timeics, timeoffset);
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
  const double weightedTime = sumWeightedTime / sumWeight;
  const double weightedTimeError = std::sqrt(sumWeightedTimeVariance) / sumWeight;
  const double weightedTimeNoOOTCorr = sumWeightedTimeNoOOTCorr / sumWeightNoOOTCorr;
  const double weightedTimeNoOOTCorrError = std::sqrt(sumWeightedTimeNoOOTCorrVariance) / sumWeightNoOOTCorr;
  if (sumWeight <= 0. && sumWeightNoOOTCorr <= 0.) {
      return {-99., -99., -99., -99.};
  } 
  else if (sumWeight > 0. && sumWeightNoOOTCorr <= 0.) {
      return {weightedTime, weightedTimeError, -99., -99.};
  } 
  else if (sumWeight <= 0. && sumWeightNoOOTCorr > 0.) {
      return {-99., -99., weightedTimeNoOOTCorr, weightedTimeNoOOTCorrError};
  } 
  else {
      return {weightedTime, weightedTimeError, weightedTimeNoOOTCorr, weightedTimeNoOOTCorrError};
  }    
}

template <typename T>
void setEcalSeedTime(std::vector<T>& objects,
                     const EcalRecHitCollection& recHitsEB,
                     const EcalRecHitCollection& recHitsEE,
                     const EcalPedestals& pedestals,
                     const EcalIntercalibConstants& ics, 
                     const EcalADCToGeVConstant& adcToGeV,
                     const EcalLaserDbService& laserCorrections,
                     const EcalTimeCalibConstants& timeics,
                     const EcalTimeOffsetConstant& timeoffset,
                     const edm::Timestamp& evtTime,
                     bool useAmplitudeOverNoise = false,
                     bool calibrateTimeNoOOTCorr = false) {
  for (auto& obj : objects) {
       const EcalTimeResult result = computeEcalSeedTime(obj, recHitsEB, recHitsEE, pedestals, ics, adcToGeV, laserCorrections, timeics, timeoffset, evtTime, useAmplitudeOverNoise, calibrateTimeNoOOTCorr);
       obj.setEcalTime(result.time);
       obj.setEcalTimeError(result.timeError);
       obj.setEcalTimeNoOOTCorr(result.timeNoOOTCorr);
       obj.setEcalTimeNoOOTCorrError(result.timeNoOOTCorrError);
  }
}

template <typename T>
void setEcalWeightedTime(std::vector<T>& objects,
                         const EcalRecHitCollection& recHitsEB,
                         const EcalRecHitCollection& recHitsEE,
                         const EcalPedestals& pedestals,
                         const EcalIntercalibConstants& ics, 
                         const EcalADCToGeVConstant& adcToGeV,
                         const EcalLaserDbService& laserCorrections,
                         const EcalTimeCalibConstants& timeics,
                         const EcalTimeOffsetConstant& timeoffset,
                         const edm::Timestamp& evtTime,
                         bool useAmplitudeOverNoise = false, 
	                 bool calibrateTimeNoOOTCorr = false,
                         const double power = 1.,
	                 const std::vector<double> ebA = {1.,1.}, //{15.14,27.31} 
	                 const std::vector<double> ebB = {0.,0.}, //{ 0.98, 0.00} 
		         const std::vector<double> ebC = {0.,0.}, //{ 0.19, 0.10} 
		         const std::vector<double> eeA = {1.,1.}, //{18.75,26.90}  
		         const std::vector<double> eeB = {0.,0.}, //{ 3.65, 6.48} 
	                 const std::vector<double> eeC = {0.,0.}  //{ 0.13, 0.13},
                        ) {
  for (auto& obj : objects) {
       const EcalTimeResult result = computeEcalWeightedTime(obj, recHitsEB, recHitsEE, pedestals, ics, adcToGeV, laserCorrections, timeics, timeoffset, evtTime, useAmplitudeOverNoise, calibrateTimeNoOOTCorr, power, ebA, ebB, ebC, eeA, eeB, eeC);
       obj.setEcalTime(result.time);
       obj.setEcalTimeError(result.timeError);
       obj.setEcalTimeNoOOTCorr(result.timeNoOOTCorr);
       obj.setEcalTimeNoOOTCorrError(result.timeNoOOTCorrError);
  }
}

#endif
