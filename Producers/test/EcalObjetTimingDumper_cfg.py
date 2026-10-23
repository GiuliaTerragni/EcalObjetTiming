import FWCore.ParameterSet.Config as cms
import FWCore.Utilities.FileUtils as FileUtils
import FWCore.ParameterSet.VarParsing as VarParsing

options = VarParsing.VarParsing('standard')
options.register('outputFile', 
                  'step4_miniaod_newTime.root', 
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "Output file")
options.register('ntupleFile',
                  'ecalTiming_ntuple.root',
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "Flat ntuple output file (electronTree/photonTree)")                  
options.parseArguments()
print(options)

process = cms.Process("NEWTIME")

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.Generator_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag,'150X_dataRun3_Prompt_v1','')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32( -1 ) )
process.MessageLogger.cerr.FwkReport.reportEvery = cms.untracked.int32( 1000 )
                                                                       
process.source = cms.Source("PoolSource",
    #skipEvents = cms.untracked.uint32(19),                       
    #fileNames = cms.untracked.vstring(options.inputFile),
    fileNames = cms.untracked.vstring('root://cms-xrd-global.cern.ch//store/data/Run2025F/EGamma2/MINIAOD/PromptReco-v1/000/397/596/00000/b5f6919e-147b-416b-b6fa-755cdc9512e2.root'),
    secondaryFileNames = cms.untracked.vstring()
)

process.slimmedPhotons = cms.EDProducer("EcalTimingPatPhotonProducer",
    src = cms.InputTag("slimmedPhotons",""),
    ebRecHits = cms.InputTag("reducedEgamma","reducedEBRecHits"),
    eeRecHits = cms.InputTag("reducedEgamma","reducedEERecHits"),
)

process.slimmedElectrons = cms.EDProducer("EcalTimingPatElectronProducer",
    src = cms.InputTag("slimmedElectrons","","RECO" ),
    ebRecHits = cms.InputTag("reducedEgamma","reducedEBRecHits"),
    eeRecHits = cms.InputTag("reducedEgamma","reducedEERecHits"),
)

process.TFileService = cms.Service("TFileService",
    fileName = cms.string(options.ntupleFile)
)

process.electronTree = cms.EDAnalyzer("EcalTimingPatElectronTreeDumper",
    src = cms.InputTag("slimmedElectrons","","NEWTIME"),
    vertices = cms.InputTag("offlineSlimmedPrimaryVertices",""), 
    rhos = cms.InputTag("fixedGridRhoAll",""),
    ebRecHits = cms.InputTag("reducedEgamma","reducedEBRecHits"),
    eeRecHits = cms.InputTag("reducedEgamma","reducedEERecHits"),
    mvaID = cms.string("ElectronMVAEstimatorRun2RunIIIWinter22IsoV1Values"),
    cutBasedID = cms.string("cutBasedElectronID-RunIIIWinter22-V1"),
    treeName = cms.string("electronTree"),
)
process.photonTree = cms.EDAnalyzer("EcalTimingPatPhotonTreeDumper",
    src = cms.InputTag("slimmedPhotons","","NEWTIME"),
    vertices = cms.InputTag("offlineSlimmedPrimaryVertices",""), 
    rhos = cms.InputTag("fixedGridRhoAll",""),
    ebRecHits = cms.InputTag("reducedEgamma","reducedEBRecHits"),
    eeRecHits = cms.InputTag("reducedEgamma","reducedEERecHits"),
    mvaID = cms.string("PhotonMVAEstimatorRunIIIWinter22v1Values"),
    cutBasedID = cms.string("cutBasedPhotonID-RunIIIWinter22-122X-V1"),
    treeName = cms.string("photonTree"),
)

process.producer_step = cms.Path(process.slimmedPhotons*process.slimmedElectrons)
process.ntuple_step = cms.EndPath(process.electronTree*process.photonTree)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.schedule = cms.Schedule(process.producer_step,process.ntuple_step,process.endjob_step)

