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
  produces<Collection>("modifiedTimeSeed");
  produces<Collection>("modifiedTimeNotWeighted");
  produces<Collection>("modifiedTimeEnergyWeighted");
  produces<Collection>("modifiedTimeSqrEnergyWeighted");
  produces<Collection>("modifiedTimeSqrtEnergyWeighted");
  produces<Collection>("modifiedTimeResEnergyWeighted");
}

template <typename T>
void EcalTimingProducer<T>::produce(edm::StreamID, edm::Event& evt, edm::EventSetup const&) const
{
  edm::Handle<Collection> handle;
  evt.getByToken(token_, handle);

  edm::Handle<EcalRecHitCollection> ebRecHits;
  evt.getByToken(ebRecHitsToken_, ebRecHits);

  edm::Handle<EcalRecHitCollection> eeRecHits;
  evt.getByToken(eeRecHitsToken_, eeRecHits);


  // Seed crystal
  auto seedOutput = std::make_unique<Collection>(*handle);
  setEcalTime(*seedOutput, *ebRecHits, *eeRecHits, "TimeSeed");
  evt.put(std::move(seedOutput), "modifiedTimeSeed");
  
  // Not weighted
  auto notWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalTime(*notWeightedOutput, *ebRecHits, *eeRecHits, "Time");
  evt.put(std::move(notWeightedOutput), "modifiedTimeNotWeighted");

  // Energy weighted
  auto energyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalTime(*energyWeightedOutput, *ebRecHits, *eeRecHits, "TimeEnergy");
  evt.put(std::move(energyWeightedOutput), "modifiedTimeEnergyWeighted");
  
  // Sqrt energy weighted
  auto SqrtEnergyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalTime(*SqrtEnergyWeightedOutput, *ebRecHits, *eeRecHits, "TimeSqrtEnergy");
  evt.put(std::move(SqrtEnergyWeightedOutput), "modifiedTimeSqrtEnergyWeighted");

  // Energy square weighted
  auto SqrEnergyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalTime(*SqrEnergyWeightedOutput, *ebRecHits, *eeRecHits, "TimeSqrEnergy");
  evt.put(std::move(SqrEnergyWeightedOutput), "modifiedTimeSqrEnergyWeighted");
   
  // Energy resolution energy weighted
  auto ResEnergyWeightedOutput = std::make_unique<Collection>(*handle);
  setEcalTime(*ResEnergyWeightedOutput, *ebRecHits, *eeRecHits, "TimeResEnergy");
  evt.put(std::move(ResEnergyWeightedOutput), "modifiedTimeResEnergyWeighted");
}

using EcalTimingPhotonProducer = EcalTimingProducer<reco::Photon>;
using EcalTimingGsfElectronProducer = EcalTimingProducer<reco::GsfElectron>;
using EcalTimingPatPhotonProducer = EcalTimingProducer<pat::Photon>;
using EcalTimingPatElectronProducer = EcalTimingProducer<pat::Electron>;

DEFINE_FWK_MODULE(EcalTimingPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingGsfElectronProducer);
DEFINE_FWK_MODULE(EcalTimingPatPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingPatElectronProducer);

