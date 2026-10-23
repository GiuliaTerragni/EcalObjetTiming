# EcalObjetTiming

1) Install:

    * scram project CMSSW_15_0_18
    * cd CMSSW_15_0_18/src/
    * cmsenv
    * git cms-init
    * git cms-checkout-topic -u GiuliaTerragni:15_0_18_EgammaTimeInfo
    * git clone git@github.com:bmarzocc/EcalObjetTiming.git
    * scram b -j 5

2) Run: 

    * voms-proxy-init --rfc --voms cms -valid 192:00 #Setup grid certificate
    * cd EcalObjetTiming/Producers/test/
    * cmsRun EcalObjetTiming_cfg.py #To produce the new MINIAODs
    * cmsRun EcalObjetTimingDumper_cfg.py #To produce the electron/photon ntuples using the dumper
    
3) Run on CRAB3 the Dumper

    * voms-proxy-init --rfc --voms cms -valid 192:00 #Setup grid certificate
    * cd EcalObjetTiming/Producers/crab/
    * crab submit config_crab.py #To submit the CRAB3 jobs, after updating the config accordingly
    * crab status CRAB-DIR #To check the status of the task
    * crab resubmit CRAB-DIR #To resubmit failed jobs of the task
    * crab kill CRAB-DIR #To kill all the CRAB3 jobs related to the task
    
