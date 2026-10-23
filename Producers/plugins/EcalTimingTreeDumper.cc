#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "EcalObjetTiming/Producers/interface/EcalTimeTools.h"

#include "TTree.h"

#include <string>
#include <vector>
#include <stdexcept>

struct Identifications {
  bool cutBasedloose = false;
  bool cutBasedmedium = false;
  bool cutBasedtight = false;
  float mvaScore = -1.;
};

namespace {
  // Must match the instance labels produced by EcalTimingProducer.
  const std::vector<std::string> kMethodNames = {
      "Seed",
      "NotWeighted",
      "EnergyWeighted",
      "SqrEnergyWeighted",
      "SqrtEnergyWeighted",
      "ResEnergyWeighted"
  };
}

template <typename T>
class EcalTimingTreeDumper
    : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit EcalTimingTreeDumper(const edm::ParameterSet&);
  ~EcalTimingTreeDumper() override = default;

  void analyze(const edm::Event&, const edm::EventSetup&) override;
  Identifications getIdentifications(const T&, const std::string&, const std::string&); 

private:
  using Collection = std::vector<T>;
  std::vector<edm::EDGetTokenT<Collection>> tokens_;
  edm::EDGetTokenT<reco::VertexCollection> vtxToken_;
  edm::EDGetTokenT<double> rhoToken_;
  edm::EDGetTokenT<EcalRecHitCollection> ebRecHitsToken_;
  edm::EDGetTokenT<EcalRecHitCollection> eeRecHitsToken_;
  edm::ESGetToken<EcalPedestals, EcalPedestalsRcd> pedToken_;
  edm::ESGetToken<EcalIntercalibConstants, EcalIntercalibConstantsRcd> icalToken_;
  edm::ESGetToken<EcalADCToGeVConstant, EcalADCToGeVConstantRcd> agcToken_;
  edm::ESGetToken<EcalLaserDbService, EcalLaserDbRecord> laserToken_;
  edm::ESGetToken<EcalTimeCalibConstants, EcalTimeCalibConstantsRcd> timeToken_;
  edm::ESGetToken<EcalTimeOffsetConstant, EcalTimeOffsetConstantRcd> offTimeToken_;

  TTree* tree_;
  std::string mvaID_;
  std::string cutBasedID_;

  // Event-level quantities: scalar branches.
  unsigned int run_ = 0;
  unsigned int lumi_ = 0;
  unsigned long long event_ = 0;
  unsigned int nVtx_ = 0;
  float rho_ = 0.;

  // Object-level quantities: vector branches.
  unsigned int nObjects_ = 0;
  std::vector<float> eta_;
  std::vector<float> phi_;
  std::vector<float> pt_;
  std::vector<int> nEcalHits_;
  std::vector<int> nEcalSeedClusterHits_;
  std::vector<int> nEcalClusters_;
  std::vector<float> energy_;
  std::vector<float> energyError_;
  std::vector<float> ecalEnergy_;
  std::vector<float> ecalEnergyError_;
  std::vector<float> ecalRawEnergy_;
  std::vector<float> ecalSeedClusterRawEnergy_;
  std::vector<float> ecalMaxEnergy_;
  std::vector<bool> isEcalDrivenSeed_;
  std::vector<bool> isTrackerDrivenSeed_;
  std::vector<bool> passCutBasedLooseID_;
  std::vector<bool> passCutBasedMediumID_;
  std::vector<bool> passCutBasedTightID_;
  std::vector<float> mvaIDScore_;

  // One vector per timing method.
  std::vector<std::vector<float>> time_;
  std::vector<std::vector<float>> timeError_;
  std::vector<std::vector<float>> timeNoOOTCorr_;
  std::vector<std::vector<float>> timeNoOOTCorrError_;
  std::vector<std::vector<float>> seedClusterTime_;
  std::vector<std::vector<float>> seedClusterTimeError_;
  std::vector<std::vector<float>> seedClusterTimeNoOOTCorr_;
  std::vector<std::vector<float>> seedClusterTimeNoOOTCorrError_;
  
  Identifications identifications_;
  EcalTimeResult timeResults_;
};


template <typename T>
EcalTimingTreeDumper<T>::EcalTimingTreeDumper(const edm::ParameterSet& cfg)
    : vtxToken_(consumes<reco::VertexCollection>(cfg.getParameter<edm::InputTag>("vertices"))),
      rhoToken_(consumes<double>(cfg.getParameter<edm::InputTag>("rhos"))),
      ebRecHitsToken_(consumes<EcalRecHitCollection>(cfg.getParameter<edm::InputTag>("ebRecHits"))),
      eeRecHitsToken_(consumes<EcalRecHitCollection>(cfg.getParameter<edm::InputTag>("eeRecHits"))),
      pedToken_(esConsumes<EcalPedestals, EcalPedestalsRcd>()),
      icalToken_(esConsumes<EcalIntercalibConstants, EcalIntercalibConstantsRcd>()),
      agcToken_(esConsumes<EcalADCToGeVConstant, EcalADCToGeVConstantRcd>()),
      laserToken_(esConsumes<EcalLaserDbService, EcalLaserDbRecord>()),
      timeToken_(esConsumes<EcalTimeCalibConstants, EcalTimeCalibConstantsRcd>()),
      offTimeToken_(esConsumes<EcalTimeOffsetConstant, EcalTimeOffsetConstantRcd>()) {

  const edm::InputTag srcTag = cfg.getParameter<edm::InputTag>("src");
  // Retrieve the collection corresponding to each timing method.
  for (const auto& method : kMethodNames) {
       const edm::InputTag tag(srcTag.label(), "modifiedTime" + method, srcTag.process()); 
       tokens_.push_back(consumes<Collection>(tag));
  }

  // Initialize timing vectors.
  time_.resize(kMethodNames.size());
  timeError_.resize(kMethodNames.size());
  timeNoOOTCorr_.resize(kMethodNames.size());
  timeNoOOTCorrError_.resize(kMethodNames.size());
  seedClusterTime_.resize(kMethodNames.size());
  seedClusterTimeError_.resize(kMethodNames.size());
  seedClusterTimeNoOOTCorr_.resize(kMethodNames.size());
  seedClusterTimeNoOOTCorrError_.resize(kMethodNames.size());

  usesResource("TFileService");
  edm::Service<TFileService> fs;
  const std::string treeName = cfg.getParameter<std::string>("treeName");
  tree_ = fs->make<TTree>(treeName.c_str(), treeName.c_str());
  
  mvaID_ = cfg.getParameter<std::string>("mvaID");
  cutBasedID_ = cfg.getParameter<std::string>("cutBasedID");

  // Event-level branches.
  tree_->Branch("run", &run_, "run/i");
  tree_->Branch("lumi", &lumi_, "lumi/i");
  tree_->Branch("event", &event_, "event/l");
  tree_->Branch("nVtx", &nVtx_, "nVtx/i");
  tree_->Branch("rho", &rho_, "rho/F");

  // Object-level branches.
  tree_->Branch("nObjects", &nObjects_, "nObjects/i"); 
  tree_->Branch("eta", &eta_);
  tree_->Branch("phi", &phi_);
  tree_->Branch("pt", &pt_);
  tree_->Branch("energy", &energy_);
  tree_->Branch("energyError", &energyError_);
  tree_->Branch("ecalEnergy", &ecalEnergy_);
  tree_->Branch("ecalEnergyError", &ecalEnergyError_);
  tree_->Branch("ecalRawEnergy", &ecalRawEnergy_);
  tree_->Branch("ecalSeedClusterRawEnergy", &ecalSeedClusterRawEnergy_);
  tree_->Branch("ecalMaxEnergy", &ecalMaxEnergy_);
  tree_->Branch("nEcalHits", &nEcalHits_);
  tree_->Branch("nEcalSeedClusterHits", &nEcalSeedClusterHits_);
  tree_->Branch("nEcalClusters", &nEcalClusters_);
  tree_->Branch("isEcalDrivenSeed", &isEcalDrivenSeed_);
  tree_->Branch("isTrackerDrivenSeed", &isTrackerDrivenSeed_);
  tree_->Branch("passCutBasedLooseID", &passCutBasedLooseID_);
  tree_->Branch("passCutBasedMediumID", &passCutBasedMediumID_);
  tree_->Branch("passCutBasedTightID", &passCutBasedTightID_);
  tree_->Branch("mvaIDScore", &mvaIDScore_);
  
  // Timing branches: one vector per method.
  for (size_t i = 0; i < kMethodNames.size(); ++i) {
       std::string method = kMethodNames[i];
       if(method=="Seed") method += "Xtal";
       tree_->Branch(("time_" + method).c_str(), &time_[i]);
       tree_->Branch(("timeError_" + method).c_str(), &timeError_[i]);
       tree_->Branch(("timeNoOOTCorr_" + method).c_str(), &timeNoOOTCorr_[i]);
       tree_->Branch(("timeNoOOTCorrError_" + method).c_str(), &timeNoOOTCorrError_[i]);
       tree_->Branch(("seedClusterTime_" + method).c_str(), &seedClusterTime_[i]);
       tree_->Branch(("seedClusterTimeError_" + method).c_str(), &seedClusterTimeError_[i]);
       tree_->Branch(("seedClusterTimeNoOOTCorr_" + method).c_str(), &seedClusterTimeNoOOTCorr_[i]);
       tree_->Branch(("seedClusterTimeNoOOTCorrError_" + method).c_str(), &seedClusterTimeNoOOTCorrError_[i]);
  }
}


template <typename T>
void EcalTimingTreeDumper<T>::analyze(const edm::Event& evt,
                                      const edm::EventSetup& es) {

  // Clear all object-level vectors at the beginning of each event
  eta_.clear();
  phi_.clear();
  pt_.clear();
  nEcalHits_.clear();
  nEcalSeedClusterHits_.clear();
  nEcalClusters_.clear();
  energy_.clear();
  energyError_.clear();
  ecalEnergy_.clear();
  ecalEnergyError_.clear();
  ecalRawEnergy_.clear();
  ecalSeedClusterRawEnergy_.clear();
  ecalMaxEnergy_.clear();
  isEcalDrivenSeed_.clear();
  isTrackerDrivenSeed_.clear();
  passCutBasedLooseID_.clear();
  passCutBasedMediumID_.clear();
  passCutBasedTightID_.clear();
  mvaIDScore_.clear();
  for (size_t i = 0; i < kMethodNames.size(); ++i) {
       time_[i].clear();
       timeError_[i].clear();
       timeNoOOTCorr_[i].clear();
       timeNoOOTCorrError_[i].clear();
       seedClusterTime_[i].clear();
       seedClusterTimeError_[i].clear();
       seedClusterTimeNoOOTCorr_[i].clear();
       seedClusterTimeNoOOTCorrError_[i].clear();
  }
  
  edm::Handle<double> rho;
  evt.getByToken(rhoToken_,rho);
  if (!rho.isValid()) {
      throw cms::Exception("ProductNotFound") << "EcalTimingTreeDumper: rho collection is not valid.";
  }
  
  edm::Handle<reco::VertexCollection> vertices;
  evt.getByToken(vtxToken_,vertices);
  if (!vertices.isValid()) {
      throw cms::Exception("ProductNotFound") << "EcalTimingTreeDumper: vertices collection is not valid.";
  }
  
  // Retrieve the collections produced by the timing methods.
  std::vector<edm::Handle<Collection>> handles(tokens_.size());
  for (size_t i = 0; i < tokens_.size(); ++i) {
       evt.getByToken(tokens_[i], handles[i]);
       if (!handles[i].isValid()) {
           throw cms::Exception("ProductNotFound") << "EcalTimingTreeDumper: timing collection for method " << kMethodNames[i] << " is not valid.";
       }
  }

  // The first collection defines the objects to be stored.
  const size_t nObjects = handles[0]->size();
  for (size_t i = 1; i < handles.size(); ++i) {
       if (handles[i]->size() != nObjects) {
           throw cms::Exception("InconsistentCollectionSize") << "EcalTimingTreeDumper: collection for method " << kMethodNames[i] << " contains " << handles[i]->size() << " objects, while the reference collection contains " << nObjects << " objects.";
       }
  }

  // Retrieve ECAL rechits.
  edm::Handle<EcalRecHitCollection> ebRecHits;
  evt.getByToken(ebRecHitsToken_, ebRecHits);
  if (!ebRecHits.isValid()) {
      throw cms::Exception("ProductNotFound") << "EcalTimingTreeDumper: EB rechit collection is not valid.";
  }
  
  edm::Handle<EcalRecHitCollection> eeRecHits;
  evt.getByToken(eeRecHitsToken_, eeRecHits);
  if (!eeRecHits.isValid()) {
      throw cms::Exception("ProductNotFound") << "EcalTimingTreeDumper: EE rechit collection is not valid.";
  }
  
  const EcalPedestals& ped = es.getData(pedToken_);
  const EcalIntercalibConstants& ical = es.getData(icalToken_);
  const EcalADCToGeVConstant& agc = es.getData(agcToken_);
  const EcalLaserDbService& laser = es.getData(laserToken_);
  const EcalTimeCalibConstants& itimecal = es.getData(timeToken_); 
  const EcalTimeOffsetConstant& timeoffset = es.getData(offTimeToken_); 
  
  // Event identifiers.
  run_ = evt.id().run();
  lumi_ = evt.id().luminosityBlock();
  event_ = evt.id().event();
  nVtx_ = vertices->size();
  rho_ = *(rho.product());

  // Loop over objects and append their values to the vectors.
  nObjects_ = nObjects;
  for (size_t j = 0; j < nObjects; ++j) {

       const T& obj = handles[0]->at(j);
       auto ecalIds = getEcalIds(obj);
         
       // Kinematics.
       eta_.push_back(obj.eta());
       phi_.push_back(obj.phi());
       pt_.push_back(obj.pt());

       // Energies.
       energy_.push_back(obj.energy());
       if constexpr (std::is_same_v<T, pat::Electron>) {
            energyError_.push_back(obj.p4Error(reco::GsfElectron::P4_COMBINATION));
            ecalEnergy_.push_back(obj.ecalEnergy());
            ecalEnergyError_.push_back(obj.ecalEnergyError());
       } else {
            energyError_.push_back(obj.getCorrectedEnergyError(reco::Photon::regression2));
            ecalEnergy_.push_back(obj.energyCorrections().phoEcalEnergy);
            ecalEnergyError_.push_back(obj.energyCorrections().phoEcalEnergyError);
       }     
       ecalRawEnergy_.push_back(obj.superCluster()->rawEnergy());
       ecalSeedClusterRawEnergy_.push_back(obj.superCluster()->seed()->energy());
       ecalMaxEnergy_.push_back(getSeedEnergy(ecalIds, *ebRecHits, *eeRecHits));
       nEcalHits_.push_back(static_cast<int>(ecalIds.size()));
       nEcalSeedClusterHits_.push_back(static_cast<int>(obj.superCluster()->seed()->hitsAndFractions().size()));
       nEcalClusters_.push_back(static_cast<int>(obj.superCluster()->clusters().size()));
       
       identifications_ = getIdentifications(obj, mvaID_, cutBasedID_);
       passCutBasedLooseID_.push_back(identifications_.cutBasedloose);
       passCutBasedMediumID_.push_back(identifications_.cutBasedmedium);
       passCutBasedTightID_.push_back(identifications_.cutBasedtight);
       mvaIDScore_.push_back(identifications_.mvaScore);
       if constexpr (std::is_same_v<T, pat::Electron>) {
            isEcalDrivenSeed_.push_back(obj.ecalDrivenSeed());  
            isTrackerDrivenSeed_.push_back(obj.trackerDrivenSeed());  
       } else {
            isEcalDrivenSeed_.push_back(true);  
            isTrackerDrivenSeed_.push_back(false); 
       }

       // Timing information for each method.
       for (size_t i = 0; i < handles.size(); ++i) {
            const T& methodObj = handles[i]->at(j);
            time_[i].push_back(methodObj.ecalTime());
            timeError_[i].push_back(methodObj.ecalTimeError());
            timeNoOOTCorr_[i].push_back(methodObj.ecalTimeNoOOTCorr());
            timeNoOOTCorrError_[i].push_back(methodObj.ecalTimeNoOOTCorrError());
       }
       
       // Seed-Timing information of the seedCluster
       timeResults_ = computeEcalWeightedTime(*obj.superCluster()->seed(), *ebRecHits, *eeRecHits, ped, ical, agc, laser, itimecal, timeoffset, evt.time(), true, true);
       seedClusterTime_[0].push_back(timeResults_.time);
       seedClusterTimeError_[0].push_back(timeResults_.timeError);
       seedClusterTimeNoOOTCorr_[0].push_back(timeResults_.timeNoOOTCorr);
       seedClusterTimeNoOOTCorrError_[0].push_back(timeResults_.timeNoOOTCorrError);
       
       // NotWeighted-Timing information of the seedCluster 
       timeResults_ = computeEcalWeightedTime(*obj.superCluster()->seed(), *ebRecHits, *eeRecHits, ped, ical, agc, laser, itimecal, timeoffset, evt.time(), false, true, 0.);
       seedClusterTime_[1].push_back(timeResults_.time);
       seedClusterTimeError_[1].push_back(timeResults_.timeError);
       seedClusterTimeNoOOTCorr_[1].push_back(timeResults_.timeNoOOTCorr);
       seedClusterTimeNoOOTCorrError_[1].push_back(timeResults_.timeNoOOTCorrError);
       
       // EnergyWeighted-Timing information of the seedCluster 
       timeResults_ = computeEcalWeightedTime(*obj.superCluster()->seed(), *ebRecHits, *eeRecHits, ped, ical, agc, laser, itimecal, timeoffset,  evt.time(), true, true, 0.5);
       seedClusterTime_[2].push_back(timeResults_.time);
       seedClusterTimeError_[2].push_back(timeResults_.timeError);
       seedClusterTimeNoOOTCorr_[2].push_back(timeResults_.timeNoOOTCorr);
       seedClusterTimeNoOOTCorrError_[2].push_back(timeResults_.timeNoOOTCorrError);
       
       // SqrEnergyWeighted-Timing information of the seedCluster 
       timeResults_ = computeEcalWeightedTime(*obj.superCluster()->seed(), *ebRecHits, *eeRecHits, ped, ical, agc, laser, itimecal, timeoffset,  evt.time(), true, true, 1.);
       seedClusterTime_[3].push_back(timeResults_.time);
       seedClusterTimeError_[3].push_back(timeResults_.timeError);
       seedClusterTimeNoOOTCorr_[3].push_back(timeResults_.timeNoOOTCorr);
       seedClusterTimeNoOOTCorrError_[3].push_back(timeResults_.timeNoOOTCorrError);
       
       // SqrtEnergyWeighted-Timing information of the seedCluster 
       timeResults_ = computeEcalWeightedTime(*obj.superCluster()->seed(), *ebRecHits, *eeRecHits, ped, ical, agc, laser, itimecal, timeoffset,  evt.time(), true, true, 0.025);
       seedClusterTime_[4].push_back(timeResults_.time);
       seedClusterTimeError_[4].push_back(timeResults_.timeError);
       seedClusterTimeNoOOTCorr_[4].push_back(timeResults_.timeNoOOTCorr);
       seedClusterTimeNoOOTCorrError_[4].push_back(timeResults_.timeNoOOTCorrError);
       
       // ResEnergyWeighted-Timing information of the seedCluster 
       timeResults_ = computeEcalWeightedTime(*obj.superCluster()->seed(), *ebRecHits, *eeRecHits, ped, ical, agc, laser, itimecal, timeoffset,  evt.time(), true, true, 1., {15.14,27.31} , { 0.98, 0.00}, { 0.19, 0.10}, {18.75,26.90}, { 3.65, 6.48}, { 0.13, 0.13});
       seedClusterTime_[5].push_back(timeResults_.time);
       seedClusterTimeError_[5].push_back(timeResults_.timeError);
       seedClusterTimeNoOOTCorr_[5].push_back(timeResults_.timeNoOOTCorr);
       seedClusterTimeNoOOTCorrError_[5].push_back(timeResults_.timeNoOOTCorrError);
       
  }
  // Fill the tree exactly once per event.
  // This also stores events with zero objects as empty vectors.
  tree_->Fill();
}

template <typename T>
Identifications EcalTimingTreeDumper<T>::getIdentifications(const T& obj, const std::string& mvaID, const std::string& cutBasedID) {
  Identifications results;
  if constexpr (std::is_same_v<T, pat::Electron>) {
      results.mvaScore       = obj.userFloat(mvaID.c_str()); 
      results.cutBasedloose  = (obj.isElectronIDAvailable((cutBasedID+"-loose").c_str())  && obj.electronID((cutBasedID+"-loose").c_str()) > 0.5);
      results.cutBasedmedium = (obj.isElectronIDAvailable((cutBasedID+"-medium").c_str())  && obj.electronID((cutBasedID+"-medium").c_str()) > 0.5);
      results.cutBasedtight  = (obj.isElectronIDAvailable((cutBasedID+"-tight").c_str())  && obj.electronID((cutBasedID+"-tight").c_str()) > 0.5);
  } else if constexpr (std::is_same_v<T, pat::Photon>){
      results.mvaScore       = obj.userFloat(mvaID.c_str());
      results.cutBasedloose  = (obj.isPhotonIDAvailable((cutBasedID+"-loose").c_str())  && obj.photonID((cutBasedID+"-loose").c_str()) > 0.5);
      results.cutBasedmedium = (obj.isPhotonIDAvailable((cutBasedID+"-medium").c_str())  && obj.photonID((cutBasedID+"-medium").c_str()) > 0.5);
      results.cutBasedtight  = (obj.isPhotonIDAvailable((cutBasedID+"-tight").c_str())  && obj.photonID((cutBasedID+"-tight").c_str()) > 0.5);
  }
  return results;
}

// Module definitions.
using EcalTimingPatPhotonTreeDumper = EcalTimingTreeDumper<pat::Photon>;
using EcalTimingPatElectronTreeDumper = EcalTimingTreeDumper<pat::Electron>;

DEFINE_FWK_MODULE(EcalTimingPatPhotonTreeDumper);
DEFINE_FWK_MODULE(EcalTimingPatElectronTreeDumper);
