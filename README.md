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
    * cmsRun EcalObjetTiming_cfg.py
