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

#include <cmath>
#include <vector>
#include <algorithm>

struct EcalTimeResult {
  double time;
  double timeError;
  double timeNoOOTCorr;
  double timeNoOOTCorrError;
};

const EcalRecHitCollection& recHitsCollection(
    const DetId& id,
    const EcalRecHitCollection& recHitsEB,
    const EcalRecHitCollection& recHitsEE) {

  return (id.subdetId() == EcalBarrel) ? recHitsEB : recHitsEE;
}

const std::vector<DetId> getEcalIds(const reco::CaloCluster& bc) {
  std::vector<DetId> ecalIds;
  const std::vector<std::pair<DetId, float>>& hitsAndFractions = bc.hitsAndFractions();
  for (const auto& hitAndFraction : hitsAndFractions) {
       const DetId id = hitAndFraction.first;
       if (id.det() != DetId::Ecal)
           continue;
       ecalIds.push_back(id);
  }
  return ecalIds;   
}

const std::vector<DetId> getEcalIds(const reco::PFCluster& pfCluster) {
  std::vector<DetId> ecalIds;
  if (pfCluster.layer() != PFLayer::ECAL_BARREL && pfCluster.layer() != PFLayer::ECAL_ENDCAP) {
    return ecalIds;
  }
  for (const auto& rhf : pfCluster.recHitFractions()) {
       const reco::PFRecHitRef& hitRef = rhf.recHitRef();
       if (hitRef.isNonnull()) {
           ecalIds.push_back((hitRef.get())->detId());
       }    
  }
  return ecalIds;   
}

const std::vector<DetId> getEcalIds(const reco::SuperCluster& sc) {
  std::vector<DetId> ecalIds;
  const auto& hitsAndFractions = sc.hitsAndFractions(); 
  for (const auto& hitAndFraction : hitsAndFractions) {
       const DetId id = hitAndFraction.first;
       if (id.det() != DetId::Ecal)
           continue;
       ecalIds.push_back(id);
  }
  return ecalIds;   
}

const std::vector<DetId> getEcalIds(const reco::Photon& photon) {
  std::vector<DetId> ecalIds = getEcalIds(*photon.superCluster());
  return ecalIds;   
}

const std::vector<DetId> getEcalIds(const reco::GsfElectron& electron) {
  std::vector<DetId> ecalIds = getEcalIds(*electron.superCluster());
  return ecalIds;   
}

const std::vector<DetId> getEcalIds(const reco::PFJet& pfJet) {
  std::vector<DetId> ecalIds;
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
            for (const auto& rhf : clusterRef->recHitFractions()) {
                 const reco::PFRecHitRef& hitRef = rhf.recHitRef();
                 if (hitRef.isNull())
                     continue;
                 DetId id = hitRef->detId();    
                 if (std::find(ecalIds.begin(), ecalIds.end(), id) == ecalIds.end())
                     ecalIds.push_back(id);
            }
       }
  }
  return ecalIds;
}

const std::vector<DetId> getEcalIds(const reco::CaloJet& caloJet) {
  std::vector<DetId> ecalIds;
  for (const auto& tower : caloJet.getCaloConstituents()) {
       if (tower.isNull())
           continue;
       for (const auto& id : tower->constituents()) {
            if (id.det() != DetId::Ecal)
                continue;
            if (std::find(ecalIds.begin(), ecalIds.end(), id) == ecalIds.end()) 
                ecalIds.push_back(id);
      }
  }
  return ecalIds;
}

const DetId getSeedId(const std::vector<DetId>& ecalIds, 
                      const EcalRecHitCollection& ebRecHits, 
                      const EcalRecHitCollection& eeRecHits) {
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
       if (recHit->energy() > maxEnergy) {
           maxEnergy = recHit->energy();
           maxHit = id;
       }
  }
  return maxHit;
}

template <typename T>
EcalTimeResult computeEcalSeedTime(const T& object, 
    				   const EcalRecHitCollection& ebRecHits, 
    				   const EcalRecHitCollection& eeRecHits) {
  const std::vector<DetId> ecalIds = getEcalIds(object);  
  const DetId seedId = getSeedId(ecalIds, ebRecHits, eeRecHits);  
  const EcalRecHitCollection& recHits = recHitsCollection(seedId, ebRecHits, eeRecHits);
  EcalRecHitCollection::const_iterator it = recHits.find(seedId);
  if (it == recHits.end()) 
      return {-99., -99., -99., -99.};  
  const double time = it->time();
  const double timeError = it->timeError();
  const double timeNoOOTCorr = it->nonCorrectedTime();
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
                         	       const edm::Timestamp& evtTime,
                         	       bool useAmplitude = false, 
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
       double calibration = 1.;
       double noise = 1.;
       if (useAmplitude) {
           double icalconst = 1.0;
           auto icalIt = ics.getMap().find(id);
           if (icalIt != ics.getMap().end())
               icalconst = *icalIt;
           double laserCalib = laserCorrections.getLaserCorrection(id,evtTime);  
           double adcToGeVconst = 0.0;
           if (id.subdetId() == EcalBarrel)
               adcToGeVconst = adcToGeV.getEBValue();
           else if (id.subdetId() == EcalEndcap)
               adcToGeVconst = adcToGeV.getEEValue();  
           calibration = adcToGeVconst * laserCalib * icalconst;
           int pedestalGain = 1;  // G12
           if (it->checkFlag(EcalRecHit::kHasSwitchToGain6))
               pedestalGain = 2;  // G6
           else if (it->checkFlag(EcalRecHit::kHasSwitchToGain1))
               pedestalGain = 3;  // G1
           auto pedIt = pedestals.getMap().find(id);
           if (pedIt != pedestals.getMap().end())
               noise = pedIt->rms(pedestalGain);
           //std::cout << "Energy = " << it->energy() << " - adcToGeVconst = " << adcToGeVconst << " - laserCalib = " << laserCalib << " - icalconst = " << icalconst << " - calibration = " << calibration << " - amplitude = " << it->energy()/calibration << std::endl;    
           //std::cout << "Amplitude: " << energy/calibration << " - noise = " << noise << std::endl;
       }
       energy /= (calibration * noise); 
       double weightEB = std::pow(energy*energy/(ebA[0]*ebA[0] + ebB[0]*ebB[0]*energy + 2*ebC[0]*ebC[0]*energy*energy),power);
       double weightEE = std::pow(energy*energy/(eeA[0]*eeA[0] + eeB[0]*eeB[0]*energy + 2*eeC[0]*eeC[0]*energy*energy),power);
       double weightNoOOTCorrEB = std::pow(energy*energy/(ebA[1]*ebA[1] + ebB[1]*ebB[1]*energy + 2*ebC[1]*ebC[1]*energy*energy),power);
       double weightNoOOTCorrEE = std::pow(energy*energy/(eeA[1]*eeA[1] + eeB[1]*eeB[1]*energy + 2*eeC[1]*eeC[1]*energy*energy),power);
       const double time = it->time();
       const double timeError = it->timeError();
       const double timeNoOOTCorr = it->nonCorrectedTime();
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
                     const EcalRecHitCollection& recHitsEE) {
  for (auto& obj : objects) {
       const EcalTimeResult result = computeEcalSeedTime(obj, recHitsEB, recHitsEE);
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
                         const edm::Timestamp& evtTime,
                         bool useAmplitude = false, 
                         const double power = 1.,
	                 const std::vector<double> ebA = {1.,1.}, //{15.14,27.31} 
	                 const std::vector<double> ebB = {0.,0.}, //{ 0.98, 0.00} 
		         const std::vector<double> ebC = {0.,0.}, //{ 0.19, 0.10} 
		         const std::vector<double> eeA = {1.,1.}, //{18.75,26.90}  
		         const std::vector<double> eeB = {0.,0.}, //{ 3.65, 6.48} 
	                 const std::vector<double> eeC = {0.,0.}  //{ 0.13, 0.13}
                        ) {
  for (auto& obj : objects) {
       const EcalTimeResult result = computeEcalWeightedTime(obj, recHitsEB, recHitsEE, pedestals, ics, adcToGeV, laserCorrections, evtTime, useAmplitude, power, ebA, ebB, ebC, eeA, eeB, eeC);
       obj.setEcalTime(result.time);
       obj.setEcalTimeError(result.timeError);
       obj.setEcalTimeNoOOTCorr(result.timeNoOOTCorr);
       obj.setEcalTimeNoOOTCorrError(result.timeNoOOTCorrError);
  }
}

#endif
