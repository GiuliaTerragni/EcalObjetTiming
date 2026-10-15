#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "CondFormats/DataRecord/interface/EcalPedestalsRcd.h"
#include "CondFormats/EcalObjects/interface/EcalPedestals.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsRcd.h"
#include "CondFormats/EcalObjects/interface/EcalIntercalibConstants.h"
#include "CondFormats/DataRecord/interface/EcalADCToGeVConstantRcd.h"
#include "CondFormats/EcalObjects/interface/EcalADCToGeVConstant.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbRecord.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbService.h"

#include "EcalObjetTiming/Producers/interface/EcalTimeTools.h"
//#include "RecoEgamma/EgammaTools/interface/EcalTimeTools.h"

#include <memory>
#include <vector>


template <typename T>
class EcalTimingProducer : public edm::global::EDProducer<> {
public:
  explicit EcalTimingProducer(const edm::ParameterSet&);
  ~EcalTimingProducer() override = default;
  void produce(edm::StreamID,edm::Event&,edm::EventSetup const&) const override;

private:
  using Collection = std::vector<T>;
  edm::EDGetTokenT<Collection> token_;
  edm::EDGetTokenT<EcalRecHitCollection> ebRecHitsToken_;
  edm::EDGetTokenT<EcalRecHitCollection> eeRecHitsToken_;
  edm::ESGetToken<EcalPedestals, EcalPedestalsRcd> pedToken_;
  edm::ESGetToken<EcalIntercalibConstants, EcalIntercalibConstantsRcd> icalToken_;
  edm::ESGetToken<EcalADCToGeVConstant, EcalADCToGeVConstantRcd> agcToken_;
  edm::ESGetToken<EcalLaserDbService, EcalLaserDbRecord> laserToken_;
};

template <typename T>
EcalTimingProducer<T>::EcalTimingProducer(const edm::ParameterSet& cfg)
    : token_(consumes<Collection>(cfg.getParameter<edm::InputTag>("src"))),
      ebRecHitsToken_(consumes<EcalRecHitCollection>(cfg.getParameter<edm::InputTag>("ebRecHits"))),
      eeRecHitsToken_(consumes<EcalRecHitCollection>(cfg.getParameter<edm::InputTag>("eeRecHits"))),
      pedToken_(esConsumes<EcalPedestals, EcalPedestalsRcd>()),
      icalToken_(esConsumes<EcalIntercalibConstants, EcalIntercalibConstantsRcd>()),
      agcToken_(esConsumes<EcalADCToGeVConstant, EcalADCToGeVConstantRcd>()),
      laserToken_(esConsumes<EcalLaserDbService, EcalLaserDbRecord>())
{
  produces<Collection>("modifiedTimeSeed");
  produces<Collection>("modifiedTimeNotWeighted");
  produces<Collection>("modifiedTimeEnergyWeighted");
  produces<Collection>("modifiedTimeSqrEnergyWeighted");
  produces<Collection>("modifiedTimeSqrtEnergyWeighted");
  produces<Collection>("modifiedTimeResEnergyWeighted");
}

template <typename T>
void EcalTimingProducer<T>::produce(edm::StreamID, edm::Event& evt, edm::EventSetup const& es) const
{
  edm::Handle<Collection> handle;
  evt.getByToken(token_, handle);

  edm::Handle<EcalRecHitCollection> ebRecHits;
  evt.getByToken(ebRecHitsToken_, ebRecHits);

  edm::Handle<EcalRecHitCollection> eeRecHits;
  evt.getByToken(eeRecHitsToken_, eeRecHits);

  const EcalPedestals& ped = es.getData(pedToken_);
  const EcalIntercalibConstants& ical = es.getData(icalToken_);
  const EcalADCToGeVConstant& agc = es.getData(agcToken_);
  const EcalLaserDbService& laser = es.getData(laserToken_);

  // Seed crystal
  auto seedOutput = std::make_unique<Collection>(*handle);
  setEcalSeedTime(*seedOutput, *ebRecHits, *eeRecHits);
  evt.put(std::move(seedOutput), "modifiedTimeSeed");
  
  // Not weighted
  auto notWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalWeightedTime(*notWeightedOutput, *ebRecHits, *eeRecHits, ped, ical, agc, laser, evt.time(), false, 0.);
  evt.put(std::move(notWeightedOutput), "modifiedTimeNotWeighted");
  
  // Energy weighted
  auto energyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalWeightedTime(*energyWeightedOutput, *ebRecHits, *eeRecHits, ped, ical, agc, laser, evt.time(), true, 0.5);
  evt.put(std::move(energyWeightedOutput), "modifiedTimeEnergyWeighted");
  
  // Sqrt energy weighted
  auto sqrtEnergyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalWeightedTime(*sqrtEnergyWeightedOutput, *ebRecHits, *eeRecHits, ped, ical, agc, laser, evt.time(), true, 0.025);
  evt.put(std::move(sqrtEnergyWeightedOutput), "modifiedTimeSqrtEnergyWeighted");
  
  // Energy square weighted
  auto sqrEnergyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalWeightedTime(*sqrEnergyWeightedOutput, *ebRecHits, *eeRecHits, ped, ical, agc, laser, evt.time(), true, 1.);
  evt.put(std::move(sqrEnergyWeightedOutput), "modifiedTimeSqrEnergyWeighted");

  // Energy resolution energy weighted
  auto resEnergyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalWeightedTime(*resEnergyWeightedOutput, *ebRecHits, *eeRecHits, ped, ical, agc, laser, evt.time(), true, 1., {15.14,27.31} , { 0.98, 0.00}, { 0.19, 0.10}, {18.75,26.90}, { 3.65, 6.48}, { 0.13, 0.13});
  evt.put(std::move(resEnergyWeightedOutput), "modifiedTimeResEnergyWeighted");
}

using EcalTimingPhotonProducer = EcalTimingProducer<reco::Photon>;
using EcalTimingGsfElectronProducer = EcalTimingProducer<reco::GsfElectron>;
using EcalTimingPatPhotonProducer = EcalTimingProducer<pat::Photon>;
using EcalTimingPatElectronProducer = EcalTimingProducer<pat::Electron>;

DEFINE_FWK_MODULE(EcalTimingPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingGsfElectronProducer);
DEFINE_FWK_MODULE(EcalTimingPatPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingPatElectronProducer);

