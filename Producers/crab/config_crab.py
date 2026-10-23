from WMCore.Configuration import Configuration
from CRABClient.UserUtilities import config
import FWCore.ParameterSet.Config as cms

config = Configuration()

config.section_('General')
config.General.requestName       = 'EGamma2_Run2024E-2024CDEReprocessing-v1_AOD'
config.General.transferLogs      = True
config.General.transferOutputs   = True

config.section_('JobType')
config.JobType.pluginName        = 'Analysis'
# Name of the CMSSW configuration file
config.JobType.psetName          = '../test/EcalObjetTimingDumper_cfg.py'
#config.JobType.priority          = 30
#config.JobType.maxJobRuntimeMin  = 2750
#config.JobType.numCores          = 4
config.JobType.allowUndistributedCMSSW = True
#config.JobType.sendPythonFolder = True

config.section_('Data')
config.Data.inputDataset         = '/EGamma2/Run2025F-PromptReco-v1/MINIAOD'
config.Data.outputDatasetTag     = 'Run2025F-PromptReco-v1-150X_dataRun3_Prompt_v1'
config.Data.inputDBS             = 'global'
config.Data.splitting            = 'FileBased' #'LumiBased'
config.Data.unitsPerJob          = 1 #30000
config.Data.outLFNDirBase        = '/store/group/dpg_ecal/alca_ecalcalib/bmarzocc/EcalTimingObject/Test/'
config.Data.publication          = False
#config.Data.publishDBS           = 'phys03'

# GRID
config.section_('Site')
config.Site.storageSite         = 'T2_CH_CERN'
#config.Site.whitelist           = ['T2_US_Wisconsin','T2_CH_CERN','T2_IT_Pisa','T2_IT_Rome','T2_DE_DESY','T2_CH_CSCS','T2_US_MIT','T1_US_FNAL','T2_CN_Beijing']
