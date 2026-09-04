#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"

#include "EcalObjetTiming/Producers/interface/EcalTimeTools.h"

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
};

template <typename T>
EcalTimingProducer<T>::EcalTimingProducer(const edm::ParameterSet& cfg)
    : token_(consumes<Collection>(
      cfg.getParameter<edm::InputTag>("src"))),
      ebRecHitsToken_(consumes<EcalRecHitCollection>(
      cfg.getParameter<edm::InputTag>("ebRecHits"))),
      eeRecHitsToken_(consumes<EcalRecHitCollection>(
      cfg.getParameter<edm::InputTag>("eeRecHits")))
{
  produces<Collection>("modifiedTimeFixed");
  produces<Collection>("modifiedTimeSeed");
  produces<Collection>("modifiedTimeEnergyWeighted");
  produces<Collection>("modifiedTimeSqrtEnergyWeighted");
}

template <typename T>
void EcalTimingProducer<T>::produce(edm::StreamID,edm::Event& evt,edm::EventSetup const&) const
{
  edm::Handle<Collection> handle;
  evt.getByToken(token_, handle);

  edm::Handle<EcalRecHitCollection> ebRecHits;
  evt.getByToken(ebRecHitsToken_, ebRecHits);
  edm::Handle<EcalRecHitCollection> eeRecHits;
  evt.getByToken(eeRecHitsToken_, eeRecHits);

  auto putWithTime = [&](const char* instance, auto timeFunc) {
    auto out = std::make_unique<Collection>(*handle);
    for(auto& obj : *out) {
      const EcalTimeResult result = timeFunc(*obj.superCluster());
      obj.setEcalTime(result.time);
      obj.setEcalTimeNoOOTCorr(result.timeNoOOTCorr);
    }
    evt.put(std::move(out), instance);
  };

  putWithTime("modifiedTimeFixed", [](const reco::SuperCluster&) {
    return ecalTimeFixed();
  });
  putWithTime("modifiedTimeSeed", [&](const reco::SuperCluster& sc) {
    return ecalTimeSeedCrystal(sc, *ebRecHits, *eeRecHits);
  });
  putWithTime("modifiedTimeEnergyWeighted", [&](const reco::SuperCluster& sc) {
    return ecalTimeEnergyWeighted(sc, *ebRecHits, *eeRecHits);
  });
  putWithTime("modifiedTimeSqrtEnergyWeighted", [&](const reco::SuperCluster& sc) {
    return ecalTimeSqrtEnergyWeighted(sc, *ebRecHits, *eeRecHits);
  });
}

using EcalTimingPhotonProducer = EcalTimingProducer<reco::Photon>;
using EcalTimingGsfElectronProducer = EcalTimingProducer<reco::GsfElectron>;
using EcalTimingPatPhotonProducer = EcalTimingProducer<pat::Photon>;
using EcalTimingPatElectronProducer = EcalTimingProducer<pat::Electron>;

DEFINE_FWK_MODULE(EcalTimingPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingGsfElectronProducer);
DEFINE_FWK_MODULE(EcalTimingPatPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingPatElectronProducer);

