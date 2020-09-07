#!/usr/bin/env python
"""Create CRAB submission script for L1 rate change 
when L1TriggerObjects conditions are updated"""
import subprocess, sys, os, argparse, json
from algo_weights import pfaWeightsMap as pwm

def check_setup():
    if not ("CMSSW_BASE" in os.environ):
        sys.exit("Please issue 'cmsenv' before running")
    if not ("crabclient" in os.environ['PATH']):
        sys.exit("Please set up crab environment before running")

# Run cmsDriver to make cmsRun config for generating L1 ntuples
def generate_ntuple_config(caloparams, globaltag, era, isData, needParent, useRECO, customize):

    commandList = ['cmsDriver.py', 'l1Ntuple', '-s', 'RAW2DIGI']

    commandList.append('--python_filename=ntuple_maker.py')
    commandList.append('-n'); commandList.append('-1')

    # suppresses the (unneeded) the RAW2DIGI output
    commandList.append('--no_output')

    # should always be set to Run2_2018 for 2018 data
    commandList.append('--era=%s'%(era))

    # validations are always run on data, not MC
    if isData: commandList.append('--data')
    else:      commandList.append('--mc')

    # default conditions
    commandList.append('--conditions=%s'%(globaltag))

    # run re-emulation including re-emulation of HCAL TPs
    commandList.append('--customise=L1Trigger/Configuration/customiseReEmul.L1TReEmulFromRAWsimHcalTP')

    # include emulated quantities in L1Ntuple
    if useRECO:   commandList.append('--customise=L1Trigger/L1TNtuples/customiseL1Ntuple.L1NtupleAODRAWEMU')
    else:         commandList.append('--customise=L1Trigger/L1TNtuples/customiseL1Ntuple.L1NtupleRAWEMU')

    # use correct CaloStage2Params; should only change if Layer2 calibration changes
    if(caloparams): commandList.append('--customise=L1Trigger/Configuration/customiseSettings.L1TSettingsToCaloParams_%s'%(caloparams))

    # need to use LUTGenerationMode = False because we are using L1TriggerObjects
    commandList.append("--customise_commands=process.HcalTPGCoderULUT.LUTGenerationMode=cms.bool(False)\\n"+customize)

    # default input file
    commandList.append('--filein=CHILDFILE.root')

    if needParent: commandList.append('--secondfilein=PARENTFILE.root')

    commandList.append('--no_exec')
    commandList.append('--fileout=L1Ntuple.root')

    print "\n"
    print "Running cmsDriver like this:\n"
    print " ".join(commandList)
    print "\n"

    subprocess.call(commandList)

# This function injects config code fragments into the cmsRun config that was generated
# These things cannot be specified during the cmsDriver execution
def customCommand(scheme, isData):

    #custom = 'process.SiteLocalConfigService = cms.Service("SiteLocalConfigService",overrideSourceCacheHintDir = cms.untracked.string("lazy-download"),)\\n'
    custom = ''
    if scheme == "PFA2": return custom 

    custom += "process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')\\n"
    
    if "PFA2p" in scheme:
        custom += "process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHBQIE11 = 1\\n"
        custom += "process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHEQIE11 = 1\\n"

    elif "PFA1p" in scheme:
        custom += "process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHBQIE11 = 1\\n"
        custom += "process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHEQIE11 = 1\\n"
        custom += "process.HcalTPGCoderULUT.contain1TSHB = True\\n"
        custom += "process.HcalTPGCoderULUT.contain1TSHE = True\\n"
        if isData:
            custom += "process.HcalTPGCoderULUT.containPhaseNSHB = 0.0\\n"
            custom += "process.HcalTPGCoderULUT.containPhaseNSHE = 0.0\\n"
        else:
            custom += "process.HcalTPGCoderULUT.containPhaseNSHB = 3.0\\n"
            custom += "process.HcalTPGCoderULUT.containPhaseNSHE = 3.0\\n"

    elif "PFA1" in scheme:
        custom += "process.HcalTPGCoderULUT.contain1TSHB = True\\n"
        custom += "process.HcalTPGCoderULUT.contain1TSHE = True\\n"
        if isData:
            custom += "process.HcalTPGCoderULUT.containPhaseNSHB = 0.0\\n"
            custom += "process.HcalTPGCoderULUT.containPhaseNSHE = 0.0\\n"
        else:
            custom += "process.HcalTPGCoderULUT.containPhaseNSHB = 3.0\\n"
            custom += "process.HcalTPGCoderULUT.containPhaseNSHE = 3.0\\n"

    elif "PFAX1" in scheme:
        custom += "process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHEQIE11 = 1\\n"
        custom += "process.HcalTPGCoderULUT.contain1TSHE = True\\n"
        if isData: custom += "process.HcalTPGCoderULUT.containPhaseNSHE = 0.0\\n"
        else:      custom += "process.HcalTPGCoderULUT.containPhaseNSHE = 3.0\\n"

    elif "PFAX2" in scheme:
        custom += "process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHEQIE11 = 1\\n"

    wStr = str(pwm[scheme])
    wStr = wStr.replace("'", r'"')
    custom += "process.simHcalTriggerPrimitiveDigis.weightsQIE11 = %s"%(wStr)

    return custom

if __name__ == '__main__':

    JOB_TYPE = 'def'
    
    # Relatively stable parameters can have defaults.
    # era should never change within a year
    ERA = 'Run3'
    
    # current data global tag
    CONDITIONS = '110X_mcRun3_2021_realistic_v6'
    
    # dummy value needed so that cmsDriver.py will
    # assume that there is an input file
    DEFAULTINPUT = '/store/express/Run2017B/ExpressPhysics/FEVT/Express-v1/000/297/562/00000/EE1F5F26-145B-E711-A146-02163E019C23.root'
    
    # frontier database (needs to be specified when overriding conditions)
    FRONTIER = 'frontier://FrontierProd/CMS_CONDITIONS'

    PARSER = argparse.ArgumentParser()
    PARSER.add_argument('-g', '--globaltag', default='110X_mcRun3_2021_realistic_v6')
    PARSER.add_argument('-t', '--tag', required=False)
    PARSER.add_argument('-l', '--lumimask', required=True)
    PARSER.add_argument('-d', '--dataset', required=True)
    PARSER.add_argument('-s', '--scheme', required=True)
    PARSER.add_argument('-o', '--outputsite', default="T3_US_FNALLPC")
    PARSER.add_argument('-e', '--era', default="Run3")
    PARSER.add_argument('-p', '--needParent', default=False, action="store_true")
    PARSER.add_argument('-r', '--useRECO', default=False, action="store_true")
    PARSER.add_argument('-n', '--no_exec', default=False, action="store_true")
    PARSER.add_argument('-c', '--caloparams', default='2018_v1_3')
    ARGS = PARSER.parse_args()
    
    isData = False
    if "Run20" in ARGS.dataset: isData = True

    if isData: 
        CONDITIONS = "110X_dataRun2_v12"
        ERA = "Run2_2018"
    
    # check environment setup
    #check_setup()
    
    tmpfile = 'submit_tmp.py'
    crab_submit_script = open(tmpfile, 'w')

    if JOB_TYPE == 'def': crab_submit_script.write("NEWCONDITIONS = False\n")
    else:                crab_submit_script.write("NEWCONDITIONS = True\n")

    crab_submit_script.write("OUTPUTSITE = '" + ARGS.outputsite + "'\n")
    crab_submit_script.write("LUMIMASK = '" + ARGS.lumimask + "'\n")
    crab_submit_script.write("PFA = '" + ARGS.scheme + "'\n")
    crab_submit_script.write("DATASET = '" + ARGS.dataset + "'\n\n")
    crab_submit_script.write("USEPARENT = %r"%(ARGS.needParent) + "\n\n")
    crab_submit_script.close()
    
    # concatenate crab submission file with template
    filename = 'submit_run_' + JOB_TYPE + '.py'
    command = "cat submit_tmp.py ntuple_submit_template.py > " + filename
    os.system(command)
    os.remove(tmpfile)
    
    custom = customCommand(ARGS.scheme, isData)
    
    # generate cmsDriver commands
    generate_ntuple_config(ARGS.caloparams, CONDITIONS, ERA, isData, ARGS.needParent, ARGS.useRECO, custom)

    if not ARGS.no_exec:
        crabcmd = "crab submit " + filename
        os.system(crabcmd)
