#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "TTree.h"

#include <string>
#include <vector>

namespace {
  // Must match the instance labels produced by EcalTimingProducer
  // ("modifiedTime" + method name).
  const std::vector<std::string> kMethodNames = {
      "Seed", "NotWeighted", "EnergyWeighted", "SqrEnergyWeighted", "SqrtEnergyWeighted", "ResEnergyWeighted"};
}  // namespace

// Dumps, for every object in "src" and every timing method, a flat ntuple
// with pt/eta/phi plus time/timeError/timeNoOOTCorr/timeNoOOTCorrError
// (one branch per method), so the result of EcalTimingProducer can be
// studied over many files without re-running FWLite each time.
template <typename T>
class EcalTimingTreeDumper : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit EcalTimingTreeDumper(const edm::ParameterSet&);
  ~EcalTimingTreeDumper() override = default;
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  using Collection = std::vector<T>;

  std::vector<edm::EDGetTokenT<Collection>> tokens_;

  TTree* tree_;

  unsigned int run_, lumi_;
  unsigned long long event_;
  float pt_, eta_, phi_;
  std::vector<float> time_, timeError_, timeNoOOTCorr_, timeNoOOTCorrError_;
};

template <typename T>
EcalTimingTreeDumper<T>::EcalTimingTreeDumper(const edm::ParameterSet& cfg)
    : time_(kMethodNames.size()),
      timeError_(kMethodNames.size()),
      timeNoOOTCorr_(kMethodNames.size()),
      timeNoOOTCorrError_(kMethodNames.size()) {
  const edm::InputTag srcTag = cfg.getParameter<edm::InputTag>("src");
  for (const auto& method : kMethodNames) {
    const edm::InputTag tag(srcTag.label(), "modifiedTime" + method, srcTag.process());
    tokens_.push_back(consumes<Collection>(tag));
  }

  usesResource("TFileService");
  edm::Service<TFileService> fs;
  const std::string treeName = cfg.getParameter<std::string>("treeName");
  tree_ = fs->make<TTree>(treeName.c_str(), treeName.c_str());

  tree_->Branch("run", &run_, "run/i");
  tree_->Branch("lumi", &lumi_, "lumi/i");
  tree_->Branch("event", &event_, "event/l");
  tree_->Branch("pt", &pt_, "pt/F");
  tree_->Branch("eta", &eta_, "eta/F");
  tree_->Branch("phi", &phi_, "phi/F");

  for (size_t i = 0; i < kMethodNames.size(); ++i) {
    const std::string& method = kMethodNames[i];
    tree_->Branch(("time_" + method).c_str(), &time_[i]);
    tree_->Branch(("timeError_" + method).c_str(), &timeError_[i]);
    tree_->Branch(("timeNoOOTCorr_" + method).c_str(), &timeNoOOTCorr_[i]);
    tree_->Branch(("timeNoOOTCorrError_" + method).c_str(), &timeNoOOTCorrError_[i]);
  }
}

template <typename T>
void EcalTimingTreeDumper<T>::analyze(const edm::Event& evt, const edm::EventSetup&) {
  std::vector<edm::Handle<Collection>> handles(tokens_.size());
  for (size_t i = 0; i < tokens_.size(); ++i) {
    evt.getByToken(tokens_[i], handles[i]);
  }

  const size_t nObjects = handles[0]->size();
  for (size_t j = 0; j < nObjects; ++j) {
    run_ = evt.id().run();
    lumi_ = evt.id().luminosityBlock();
    event_ = evt.id().event();

    const T& obj = handles[0]->at(j);
    pt_ = obj.pt();
    eta_ = obj.eta();
    phi_ = obj.phi();

    for (size_t i = 0; i < tokens_.size(); ++i) {
      const T& methodObj = handles[i]->at(j);
      time_[i] = methodObj.ecalTime();
      timeError_[i] = methodObj.ecalTimeError();
      timeNoOOTCorr_[i] = methodObj.ecalTimeNoOOTCorr();
      timeNoOOTCorrError_[i] = methodObj.ecalTimeNoOOTCorrError();
    }

    tree_->Fill();
  }
}

using EcalTimingPatPhotonTreeDumper = EcalTimingTreeDumper<pat::Photon>;
using EcalTimingPatElectronTreeDumper = EcalTimingTreeDumper<pat::Electron>;

DEFINE_FWK_MODULE(EcalTimingPatPhotonTreeDumper);
DEFINE_FWK_MODULE(EcalTimingPatElectronTreeDumper);
