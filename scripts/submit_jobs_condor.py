#!/usr/bin/env python

import sys, os, argparse, json, subprocess, shutil
from algo_weights import pfaWeightsMap as pwm
from time import strftime

def rangesFromList(alist):

    tempList = []
    for i in alist: tempList.append(int(i))

    tempList.sort()

    firstRange = 0; lastRange = 0; returnList = []
    for ilumi in xrange(len(tempList)):
        
        if ilumi == 0: firstRange = tempList[ilumi]
            
        elif tempList[ilumi] - tempList[ilumi-1] > 1:
            lastRange = tempList[ilumi-1]
            returnList.append([firstRange, lastRange])
            firstRange = tempList[ilumi]

            if ilumi == len(tempList) - 1: returnList.append([firstRange, firstRange])

        elif ilumi == len(tempList) - 1: returnList.append([firstRange, tempList[ilumi]])
            
    return returnList

def files4Run(dataset,run):

    proc = subprocess.Popen(['dasgoclient', '--query=file dataset=%s run=%s'%(dataset,run)], stdout=subprocess.PIPE)
    files = proc.stdout.readlines()
    files = [afile.rstrip() for afile in files]
    return files

def lumis4File(afile):

    # We can't do a dasgoclient query with a redirector in the path...
    cleanFile = afile.replace("root://cms-xrd-global.cern.ch//", "")

    lumiList = []
    proc = subprocess.Popen(['dasgoclient', '--query=lumi file=%s'%(cleanFile)], stdout=subprocess.PIPE)
    lumis = proc.stdout.readlines()

    rawRange = lumis[0][1:-2].split(",")

    lumiList = rangesFromList(rawRange)

    return lumiList

def files4Dataset(dataset):

    # Construct dasgoclient call and make it, reading back the standard out to get the list of files
    proc = subprocess.Popen(['dasgoclient', '--query=file dataset=%s'%(dataset)], stdout=subprocess.PIPE)
    files = proc.stdout.readlines();  files = [file.rstrip() for file in files]

    # Prep the files for insertion into cmsRun config by adding the global redirector
    returnFiles = []
    for file in files: returnFiles.append(file.replace("/store/", "root://cms-xrd-global.cern.ch///store/"))

    return returnFiles 

def getParents4File(file):

    # We can't do a dasgoclient query with a redirector in the path...
    cleanFile = file.replace("root://cms-xrd-global.cern.ch//", "")

    # Find the parent files (more than one!) for each file in the dataset
    proc = subprocess.Popen(['dasgoclient', '--query=parent file=%s'%(cleanFile)], stdout=subprocess.PIPE)
    parents = proc.stdout.readlines();  parents = [parent.rstrip() for parent in parents]

    # Prep the parents files for insertion into cmsRun config by adding the global redirector.
    returnParents = []
    for parent in parents:
        returnParent = "%s"%(parent)
        returnParents.append(returnParent.replace("/store/", "root://cms-xrd-global.cern.ch///store/"))

    return returnParents

# Run cmsDriver to make cmsRun config for generating L1 ntuples
def generate_ntuple_config(caloparams, globaltag, era, lumiSplit, isData, needParent, useRECO, isMiniAOD, customize):

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

    # Add in lumi mask
    if lumiSplit: lumiMask = 'process.source.lumisToProcess=cms.untracked.VLuminosityBlockRange("RUN:LUMI1-RUN:LUMI2")'

    # Some BS when using MINIAOD
    slimReco = ""
    if useRECO and isMiniAOD: slimReco = 'process.l1RecoTree.vtxToken = "offlineSlimmedPrimaryVertices"\\n' 

    # use correct CaloStage2Params; should only change if Layer2 calibration changes
    if(caloparams): commandList.append('--customise=L1Trigger/Configuration/customiseSettings.L1TSettingsToCaloParams_%s'%(caloparams))

    # need to use LUTGenerationMode = False because we are using L1TriggerObjects
    commandList.append("--customise_commands=process.HcalTPGCoderULUT.LUTGenerationMode=cms.bool(False)\\n"+lumiMask+"\\n"+slimReco+"\\n"+customize)

    # default input file
    commandList.append('--filein=CHILDFILE')

    if needParent: commandList.append('--secondfilein=PARENTFILE')

    commandList.append('--no_exec')
    commandList.append('--fileout=L1Ntuple.root')

    print "\n"
    print "Running cmsDriver like this:\n"
    print " ".join(commandList)
    print "\n"

    subprocess.call(commandList)

# Write .sh script to be run by Condor
def generate_job_steerer(workingDir, outputDir, needParent, copyLocal, CMSSW_VERSION):

    scriptFile = open("%s/runJob.sh"%(workingDir), "w")
    scriptFile.write("#!/bin/bash\n\n")
    scriptFile.write("JOB=$1\n");
    scriptFile.write("shift\n")
    scriptFile.write("RUN=$1\n");
    scriptFile.write("shift\n")
    scriptFile.write("LUMI1=$1\n");
    scriptFile.write("shift\n")
    scriptFile.write("LUMI2=$1\n");
    scriptFile.write("shift\n")

    scriptFile.write("CHILDPATH=$1\n");
    scriptFile.write("shift\n")
    scriptFile.write("CHILDFILE=$1\n");
    scriptFile.write("shift\n\n")

    if copyLocal:
        scriptFile.write("xrdcp --retry 3 ${CHILDPATH}/${CHILDFILE} . & pid=$!\n")
        scriptFile.write("PID_LIST+=\" $pid\"\n\n")


    if needParent:
        scriptFile.write("PARENTFILESTR=\"\"\n")
        scriptFile.write("PARENTPATH=$1\n");
        scriptFile.write("shift\n")
        scriptFile.write("PARENTFILES=$@\n\n")
        scriptFile.write("for PARENTFILE in ${PARENTFILES}; do {\n")

        if copyLocal:
            scriptFile.write("    xrdcp --retry 3 ${PARENTPATH}/${PARENTFILE} . & pid=$!\n")
            scriptFile.write("    PID_LIST+=\" $pid\";\n")
            scriptFile.write("    PARENTFILESTR=\"${PARENTFILESTR}'file://${PARENTFILE}',\";\n")

        else:
            scriptFile.write("    PARENTFILESTR=\"${PARENTFILESTR}'${PARENTPATH}/${PARENTFILE}',\";\n")

        scriptFile.write("} done\n\n")

    if copyLocal: scriptFile.write("wait $PID_LIST\n\n")

    scriptFile.write("export SCRAM_ARCH=slc7_amd64_gcc700\n\n")
    scriptFile.write("source /cvmfs/cms.cern.ch/cmsset_default.sh\n") 
    scriptFile.write("eval `scramv1 project CMSSW %s`\n\n"%(CMSSW_VERSION))
    scriptFile.write("tar -xf %s.tar.gz\n"%(CMSSW_VERSION))
    scriptFile.write("mv ntuple_maker.py %s/src\n"%(CMSSW_VERSION))
    scriptFile.write("mv *.root %s/src\n"%(CMSSW_VERSION))
    scriptFile.write("cd %s/src\n"%(CMSSW_VERSION))
    scriptFile.write("ls -lrth\n\n")
    scriptFile.write("scramv1 b ProjectRename\n")
    scriptFile.write("eval `scramv1 runtime -sh`\n")

    if copyLocal:
        scriptFile.write("sed -i \"s|CHILDFILE|file://${CHILDFILE}|g\" ntuple_maker.py\n")
    else:
        scriptFile.write("sed -i \"s|CHILDFILE|${CHILDPATH}/${CHILDFILE}|g\" ntuple_maker.py\n")

    scriptFile.write("sed -i \"s|'PARENTFILE'|${PARENTFILESTR}|g\" ntuple_maker.py\n\n")

    scriptFile.write("sed -i \"s|RUN|${RUN}|g\" ntuple_maker.py\n")
    scriptFile.write("sed -i \"s|LUMI1|${LUMI1}|g\" ntuple_maker.py\n")
    scriptFile.write("sed -i \"s|LUMI2|${LUMI2}|g\" ntuple_maker.py\n")

    scriptFile.write("cmsRun ntuple_maker.py\n\n")
    scriptFile.write("xrdcp -f L1Ntuple.root %s/L1Ntuple_${JOB}.root 2>&1\n\n"%(outputDir))
    scriptFile.write("cd ${_CONDOR_SCRATCH_DIR}\n")
    scriptFile.write("rm -r %s*\n"%(CMSSW_VERSION))
    scriptFile.close()

# Write Condor submit file 
def generate_condor_submit(workingDir, inputFileMap, childLumiMap, parentLumiMap, run, needParent):

    condorSubmit = open("%s/condorSubmit.jdl"%(workingDir), "w")
    condorSubmit.write("Executable           =  %s/runJob.sh\n"%(workingDir))
    condorSubmit.write("Universe             =  vanilla\n")
    condorSubmit.write("Requirements         =  OpSys == \"LINUX\" && Arch ==\"x86_64\"\n")
    condorSubmit.write("Request_Memory       =  2.5 Gb\n")
    condorSubmit.write("Output               =  %s/logs/$(Cluster)_$(Process).stdout\n"%(workingDir))
    condorSubmit.write("Error                =  %s/logs/$(Cluster)_$(Process).stderr\n"%(workingDir))
    condorSubmit.write("Log                  =  %s/logs/$(Cluster)_$(Process).log\n"%(workingDir))
    condorSubmit.write("Transfer_Input_Files =  %s/ntuple_maker.py, %s/runJob.sh, %s/%s.tar.gz\n"%(workingDir,workingDir,workingDir,os.getenv("CMSSW_VERSION")))
    condorSubmit.write("x509userproxy        =  $ENV(X509_USER_PROXY)\n\n")
    
    iJob = 0 
    for childFile, parentFiles in inputFileMap.iteritems():
        
        childName = childFile.split("/")[-1]; childPath = "/".join(childFile.split("/")[:-1])
        parentPath = "/".join(parentFiles[0].split("/")[:-1])

        if needParent:
            for lumiRange in childLumiMap[childFile]:
                condorSubmit.write("Arguments       = %d %d %d %d %s %s %s "%(iJob, run, int(lumiRange[0]), int(lumiRange[1]), childPath, childName, parentPath))
                for parent in parentFiles:
                    for pLumiRange in parentLumiMap[parent]:
                        if (int(pLumiRange[0]) <= int(lumiRange[1]) and int(pLumiRange[1]) >= int(lumiRange[0])):
                            condorSubmit.write("%s "%(parent.split("/")[-1]))
                            break

                condorSubmit.write("\nQueue\n\n")
                iJob += 1

        else:
            for lumiRange in childLumiMap[childFile]:
                if len(lumiRange) == 1:   condorSubmit.write("Arguments       = %d %d %d %d %s %s\n"%(iJob, run, int(lumiRange[0]), int(lumiRange[0]), childPath, childName))
                elif len(lumiRange) == 2: condorSubmit.write("Arguments       = %d %d %d %d %s %s\n"%(iJob, run, int(lumiRange[0]), int(lumiRange[1]), childPath, childName))
                condorSubmit.write("Queue\n\n")
                iJob += 1
    
    condorSubmit.close()

# This function injects config code fragments into the cmsRun config that was generated
# These things cannot be specified during the cmsDriver execution
def customCommand(scheme, isData):

    custom = ""
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

    parser = argparse.ArgumentParser()
    parser.add_argument('-g', '--globalTag' , type=str , default='110X_mcRun3_2021_realistic_v9')
    parser.add_argument('-l', '--lumiSplit' , default=False, action="store_true") 
    parser.add_argument('-d', '--dataset'   , type=str , default="NULL")
    parser.add_argument('-a', '--scheme'    , type=str , default="NULL")
    parser.add_argument('-n', '--noSubmit'  , default=False, action="store_true")
    parser.add_argument('-c', '--caloParams', type=str , default='2018_v1_3')
    parser.add_argument('-e', '--era'       , type=str , default='Run3')
    parser.add_argument('-f', '--filelist'  , type=str , default='NULL')
    parser.add_argument('-p', '--needParent', default=False, action="store_true") 
    parser.add_argument('-r', '--useRECO'   , default=False, action="store_true") 
    parser.add_argument('-y', '--copyLocal' , default=False, action="store_true") 
    parser.add_argument('-u', '--run'       , type=int , default=1)
    parser.add_argument('-s', '--resubmit'  , nargs="*", default=["NULL"])
    args = parser.parse_args()

    lumiSplit    = args.lumiSplit
    filelist     = args.filelist
    copyLocal    = args.copyLocal
    resubmitDir  = args.resubmit[0]
    resubmitJobs = args.resubmit[1:]
    era          = args.era
    run          = args.run
    scheme       = args.scheme
    needParent   = args.needParent
    useRECO      = args.useRECO
    dataset      = args.dataset
    globalTag    = args.globalTag
    caloParams   = args.caloParams   

    # Do resubmission rather than going through everything else
    if resubmitDir != "NULL":
        
        resubTime = strftime("%Y%m%d_%H%M%S")
        resubFile = open("%s/condorReSubmit_%s.jdl"%(resubmitDir,resubTime), "w")
        subFile   = open("%s/condorSubmit.jdl"%(resubmitDir), "r")

        lines = subFile.readlines()

        for line in lines:
            
            if "Arguments" not in line and "Queue" not in line and line[0] != "#" and len(line) > 1: resubFile.write(line)

            if "Arguments" in line:
                
                job = line.split("=")[-1].split(" ")[1]
                if job in resubmitJobs:

                    resubFile.write(line)
                    resubFile.write("Queue\n\n")

        resubFile.close()
        subFile.close()

        #os.system("condor_submit %s/condorReSubmit_%s.jdl"%(resubmitDir,resubTime))

    else:

        HOME = os.getenv("HOME")
        USER = os.getenv("USER")
        CMSSW_BASE = os.getenv("CMSSW_BASE")
        CMSSW_VERSION = os.getenv("CMSSW_VERSION")

        # From the dataset specified, determine if we are running on data or mc
        isData = True;
        if "mcRun" in dataset: isData = False

        # Do some incompatible arguments checks
        if isData and "mcRun" in globalTag:
            print "STOP! Trying to run on data with an MC global tag!"
            print "Exiting..."
            quit()

        if not isData and "dataRun" in globalTag:
            print "STOP! Trying to run on MC with a data global tag!"
            print "Exiting..."
            quit()

        # Use date and time as the base directory for the workspace
        taskDir = strftime("%Y%m%d_%H%M%S")

        # Get the physics process from the input dataset name to use for setting up the workspace structure.
        physProcess = dataset.split("/")[1].split("_")[0]
        
        # Setup the output directory and working directory given the pass parameters
        outputDir = "root://cmseos.fnal.gov//store/user/%s/HCAL_Trigger_Study/L1Ntuples/%s/%s"%(USER, physProcess,scheme)
        workingDir = "%s/condor/%s_%s_%s"%(os.getcwd(),physProcess,scheme,taskDir)
        
        # Use proper eos syntax to make a new directory there
        subprocess.call(["eos", "root://cmseos.fnal.gov", "mkdir", "-p", outputDir[23:]])
        os.makedirs(workingDir)
        
        if outputDir.split("/")[-1] == "":  outputDir  = outputDir[:-1]
        if workingDir.split("/")[-1] == "": workingDir = workingDir[:-1]

        # Create directories to save logs
        logDir = "%s/logs"%(workingDir);  os.makedirs(logDir)

        # From the input dataset get the list of files to run on and also the parent files if applicable
        datasetFiles = []
        if ".txt" in filelist: datasetFiles = open(filelist, "r").read().splitlines() 
        else:                  datasetFiles = files4Dataset(dataset)

        isMiniAOD = "MINIAOD" in datasetFiles[0]

        childParentFilePair = {}; childFileLumiPair = {}; parentFileLumiPair = {}
        for file in datasetFiles:

            if lumiSplit: childFileLumiPair[file] = lumis4File(file)
            else:         childFileLumiPair[file] = [[1,10000]]

            if needParent:
                childFileLumiPair[file] = lumis4File(file)
                parents = getParents4File(file)
                needGparents = "RAW" not in parents[0]

                if needGparents:
                    gparents = []
                    for parent in parents: gparents += getParents4File(parent)
                    
                    for gparent in gparents: parentFileLumiPair[gparent] = lumis4File(gparent)

                    childParentFilePair[file] = gparents
                else:
                    for parent in parents:
                        if lumiSplit: parentFileLumiPair[parent] = lumis4File(parent)
                        else:         parentFileLumiPair[parent] = [[1,10000]]
                    childParentFilePair[file] = parents
                    
            else:
                childParentFilePair[file] = "NULL"

        # From freshly created cmsRun config, hack it and inject some extra code...
        custom = customCommand(scheme, isData, workingDir)

        # Generate cmsDriver commands
        generate_ntuple_config(caloParams, globalTag, era, lumiSplit, isData, needParent, useRECO, isMiniAOD, custom)

        shutil.copy2("ntuple_maker.py", workingDir)

        # Write the sh file used to run the show on the worker node
        generate_job_steerer(workingDir, outputDir, needParent, copyLocal, CMSSW_VERSION)    

        # Write the condor submit file for condor to do its thing
        generate_condor_submit(workingDir, childParentFilePair, childFileLumiPair, parentFileLumiPair, run, needParent)    

        subprocess.call(["chmod", "+x", "%s/runJob.sh"%(workingDir)])

        subprocess.call(["tar", "--exclude-caches-all", "--exclude-vcs", "-zcf", "%s/%s.tar.gz"%(workingDir,CMSSW_VERSION), "-C", "%s/.."%(CMSSW_BASE), CMSSW_VERSION, \
        "--exclude=src/HcalTrigger", "--exclude=tmp", "--exclude=bin"])
        
        if not args.noSubmit: os.system("condor_submit %s/condorSubmit.jdl"%(workingDir))
