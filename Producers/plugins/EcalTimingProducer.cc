#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"

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
};

template <typename T>
EcalTimingProducer<T>::EcalTimingProducer(const edm::ParameterSet& cfg)
    : token_(consumes<Collection>(
      cfg.getParameter<edm::InputTag>("src"))) 
{
  produces<Collection>("modifiedTime");
}

template <typename T>
void EcalTimingProducer<T>::produce(edm::StreamID,edm::Event& evt,edm::EventSetup const&) const 
{
  edm::Handle<Collection> handle;
  evt.getByToken(token_, handle);

  auto out = std::make_unique<Collection>(*handle);
  for(auto& obj : *out) {
      obj.setEcalTime(3.0);
      obj.setEcalTimeNoOOTCorr(-3.0);
  }
  evt.put(std::move(out),"modifiedTime");
}

using EcalTimingPhotonProducer = EcalTimingProducer<reco::Photon>;
using EcalTimingGsfElectronProducer = EcalTimingProducer<reco::GsfElectron>;
using EcalTimingPatPhotonProducer = EcalTimingProducer<pat::Photon>;
using EcalTimingPatElectronProducer = EcalTimingProducer<pat::Electron>;

DEFINE_FWK_MODULE(EcalTimingPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingGsfElectronProducer);
DEFINE_FWK_MODULE(EcalTimingPatPhotonProducer);
DEFINE_FWK_MODULE(EcalTimingPatElectronProducer);

