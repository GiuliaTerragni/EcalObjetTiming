import FWCore.ParameterSet.Config as cms
import FWCore.Utilities.FileUtils as FileUtils
import FWCore.ParameterSet.VarParsing as VarParsing

options = VarParsing.VarParsing('standard')
options.register('outputFile', 
                  'step4_miniaod_newTime.root', 
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "Output file")
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
process.GlobalTag = GlobalTag(process.GlobalTag,'140X_dataRun3_v20','')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32( -1 ) )
process.MessageLogger.cerr.FwkReport.reportEvery = cms.untracked.int32( 1000 )
                                                                       
process.source = cms.Source("PoolSource",
    #skipEvents = cms.untracked.uint32(19),                       
    #fileNames = cms.untracked.vstring(options.inputFile),
    fileNames = cms.untracked.vstring('root://cms-xrd-global.cern.ch//store/data/Run2025F/EGamma2/MINIAOD/PromptReco-v1/000/397/596/00000/b5f6919e-147b-416b-b6fa-755cdc9512e2.root'),
    secondaryFileNames = cms.untracked.vstring()
)

process.MINIAODoutput = cms.OutputModule("PoolOutputModule",
    compressionAlgorithm = cms.untracked.string('LZMA'),
    compressionLevel = cms.untracked.int32(1),
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string('GEN'),
        filterName = cms.untracked.string('')
    ),
    eventAutoFlushCompressedSize = cms.untracked.int32(20971520),
    fileName = cms.untracked.string(options.outputFile),
    outputCommands = process.MINIAODEventContent.outputCommands,
    splitLevel = cms.untracked.int32(0)
)
process.MINIAODoutput.outputCommands.extend([
    "drop *",
    "keep *_TriggerResults_*_*",
    "keep *_fixedGrid*_*_*",
    "keep *_*Egamma*_*_*",
    "keep *_slimmedPhotons_*_*",
    "keep *_slimmedElectrons_*_*"
])

process.slimmedPhotons = cms.EDProducer("EcalTimingPatPhotonProducer",
    src = cms.InputTag("slimmedPhotons","","RECO" ),
    ebRecHits = cms.InputTag("reducedEgamma","reducedEBRecHits","RECO"),
    eeRecHits = cms.InputTag("reducedEgamma","reducedEERecHits","RECO"),
)
process.slimmedElectrons = cms.EDProducer("EcalTimingPatElectronProducer",
    src = cms.InputTag("slimmedElectrons","","RECO" ),
    ebRecHits = cms.InputTag("reducedEgamma","reducedEBRecHits","RECO"),
    eeRecHits = cms.InputTag("reducedEgamma","reducedEERecHits","RECO"),
)
process.producer_step = cms.Path(process.slimmedPhotons*process.slimmedElectrons)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.MiniAODoutput_step = cms.EndPath(process.MINIAODoutput)
process.schedule = cms.Schedule(process.producer_step,process.endjob_step,process.MiniAODoutput_step)

