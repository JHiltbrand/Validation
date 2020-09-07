# Auto generated configuration file
# using: 
# Revision: 1.19 
# Source: /local/reps/CMSSW/CMSSW/Configuration/Applications/python/ConfigBuilder.py,v 
# with command line options: l1Ntuple -s RAW2DIGI --python_filename=ntuple_maker.py -n -1 --no_output --era=Run3 --mc --conditions=110X_mcRun3_2021_realistic_v6 --customise=L1Trigger/Configuration/customiseReEmul.L1TReEmulFromRAWsimHcalTP --customise=L1Trigger/L1TNtuples/customiseL1Ntuple.L1NtupleAODRAWEMU --customise=L1Trigger/Configuration/customiseSettings.L1TSettingsToCaloParams_2018_v1_3 --customise_commands=process.HcalTPGCoderULUT.LUTGenerationMode=cms.bool(False)\nprocess.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')\nprocess.simHcalTriggerPrimitiveDigis.numberOfPresamplesHBQIE11 = 1\nprocess.simHcalTriggerPrimitiveDigis.numberOfPresamplesHEQIE11 = 1\nprocess.HcalTPGCoderULUT.contain1TSHB = True\nprocess.HcalTPGCoderULUT.contain1TSHE = True\nprocess.HcalTPGCoderULUT.containPhaseNSHB = 3.0\nprocess.HcalTPGCoderULUT.containPhaseNSHE = 3.0\nprocess.simHcalTriggerPrimitiveDigis.weightsQIE11 = {"24": [-0.45, 1.0], "25": [-0.45, 1.0], "26": [-0.45, 1.0], "27": [-0.45, 1.0], "20": [-0.49, 1.0], "21": [-0.45, 1.0], "22": [-0.45, 1.0], "23": [-0.45, 1.0], "28": [-0.45, 1.0], "1": [-0.51, 1.0], "3": [-0.51, 1.0], "2": [-0.51, 1.0], "5": [-0.51, 1.0], "4": [-0.51, 1.0], "7": [-0.51, 1.0], "6": [-0.51, 1.0], "9": [-0.51, 1.0], "8": [-0.51, 1.0], "11": [-0.51, 1.0], "10": [-0.51, 1.0], "13": [-0.51, 1.0], "12": [-0.51, 1.0], "15": [-0.51, 1.0], "14": [-0.51, 1.0], "17": [-0.49, 1.0], "16": [-0.51, 1.0], "19": [-0.49, 1.0], "18": [-0.49, 1.0]} --filein=CHILDFILE.root --secondfilein=PARENTFILE.root --no_exec --fileout=L1Ntuple.root
import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Run3_cff import Run3

process = cms.Process('RAW2DIGI',Run3)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.RawToDigi_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1),
    output = cms.optional.untracked.allowed(cms.int32,cms.PSet)
)

# Input source
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('CHILDFILE.root'),
    secondaryFileNames = cms.untracked.vstring('PARENTFILE.root')
)

process.options = cms.untracked.PSet(
    FailPath = cms.untracked.vstring(),
    IgnoreCompletely = cms.untracked.vstring(),
    Rethrow = cms.untracked.vstring(),
    SkipEvent = cms.untracked.vstring(),
    allowUnscheduled = cms.obsolete.untracked.bool,
    canDeleteEarly = cms.untracked.vstring(),
    emptyRunLumiMode = cms.obsolete.untracked.string,
    eventSetup = cms.untracked.PSet(
        forceNumberOfConcurrentIOVs = cms.untracked.PSet(

        ),
        numberOfConcurrentIOVs = cms.untracked.uint32(1)
    ),
    fileMode = cms.untracked.string('FULLMERGE'),
    forceEventSetupCacheClearOnNewRun = cms.untracked.bool(False),
    makeTriggerResults = cms.obsolete.untracked.bool,
    numberOfConcurrentLuminosityBlocks = cms.untracked.uint32(1),
    numberOfConcurrentRuns = cms.untracked.uint32(1),
    numberOfStreams = cms.untracked.uint32(0),
    numberOfThreads = cms.untracked.uint32(1),
    printDependencies = cms.untracked.bool(False),
    sizeOfStackForThreadsInKB = cms.optional.untracked.uint32,
    throwIfIllegalParameter = cms.untracked.bool(True),
    wantSummary = cms.untracked.bool(False)
)

# Production Info
process.configurationMetadata = cms.untracked.PSet(
    annotation = cms.untracked.string('l1Ntuple nevts:-1'),
    name = cms.untracked.string('Applications'),
    version = cms.untracked.string('$Revision: 1.19 $')
)

# Output definition

# Additional output definition

# Other statements
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '110X_mcRun3_2021_realistic_v6', '')

# Path and EndPath definitions
process.raw2digi_step = cms.Path(process.RawToDigi)
process.endjob_step = cms.EndPath(process.endOfProcess)

# Schedule definition
process.schedule = cms.Schedule(process.raw2digi_step,process.endjob_step)
from PhysicsTools.PatAlgos.tools.helpers import associatePatAlgosToolsTask
associatePatAlgosToolsTask(process)

# customisation of the process.

# Automatic addition of the customisation function from L1Trigger.Configuration.customiseReEmul
from L1Trigger.Configuration.customiseReEmul import L1TReEmulFromRAWsimHcalTP 

#call to customisation function L1TReEmulFromRAWsimHcalTP imported from L1Trigger.Configuration.customiseReEmul
process = L1TReEmulFromRAWsimHcalTP(process)

# Automatic addition of the customisation function from L1Trigger.L1TNtuples.customiseL1Ntuple
from L1Trigger.L1TNtuples.customiseL1Ntuple import L1NtupleAODRAWEMU 

#call to customisation function L1NtupleAODRAWEMU imported from L1Trigger.L1TNtuples.customiseL1Ntuple
process = L1NtupleAODRAWEMU(process)

# Automatic addition of the customisation function from L1Trigger.Configuration.customiseSettings
from L1Trigger.Configuration.customiseSettings import L1TSettingsToCaloParams_2018_v1_3 

#call to customisation function L1TSettingsToCaloParams_2018_v1_3 imported from L1Trigger.Configuration.customiseSettings
process = L1TSettingsToCaloParams_2018_v1_3(process)

# End of customisation functions

# Customisation from command line

process.HcalTPGCoderULUT.LUTGenerationMode=cms.bool(False)
process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')
process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHBQIE11 = 1
process.simHcalTriggerPrimitiveDigis.numberOfPresamplesHEQIE11 = 1
process.HcalTPGCoderULUT.contain1TSHB = True
process.HcalTPGCoderULUT.contain1TSHE = True
process.HcalTPGCoderULUT.containPhaseNSHB = 3.0
process.HcalTPGCoderULUT.containPhaseNSHE = 3.0
process.simHcalTriggerPrimitiveDigis.weightsQIE11 = {"24": [-0.45, 1.0], "25": [-0.45, 1.0], "26": [-0.45, 1.0], "27": [-0.45, 1.0], "20": [-0.49, 1.0], "21": [-0.45, 1.0], "22": [-0.45, 1.0], "23": [-0.45, 1.0], "28": [-0.45, 1.0], "1": [-0.51, 1.0], "3": [-0.51, 1.0], "2": [-0.51, 1.0], "5": [-0.51, 1.0], "4": [-0.51, 1.0], "7": [-0.51, 1.0], "6": [-0.51, 1.0], "9": [-0.51, 1.0], "8": [-0.51, 1.0], "11": [-0.51, 1.0], "10": [-0.51, 1.0], "13": [-0.51, 1.0], "12": [-0.51, 1.0], "15": [-0.51, 1.0], "14": [-0.51, 1.0], "17": [-0.49, 1.0], "16": [-0.51, 1.0], "19": [-0.49, 1.0], "18": [-0.49, 1.0]}
#Have logErrorHarvester wait for the same EDProducers to finish as those providing data for the OutputModule
from FWCore.Modules.logErrorHarvester_cff import customiseLogErrorHarvesterUsingOutputCommands
process = customiseLogErrorHarvesterUsingOutputCommands(process)

# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion
