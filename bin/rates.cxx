// Script for calculating rate histograms
// Originally from Aaron Bundock
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TChain.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "L1Trigger/L1TNtuples/interface/L1AnalysisEventDataFormat.h"
#include "L1Trigger/L1TNtuples/interface/L1AnalysisL1UpgradeDataFormat.h"
#include "L1Trigger/L1TNtuples/interface/L1AnalysisRecoVertexDataFormat.h"
#include "L1Trigger/L1TNtuples/interface/L1AnalysisCaloTPDataFormat.h"


/* TODO: put errors in rates...
creates the the rates and distributions for l1 trigger objects
How to use:
1. input the number of bunches in the run (~line 35)
2. change the variables "newConditionsNtuples" and "oldConditionsNtuples" to ntuple paths
3. If good run JSON is not applied during ntuple production, modify isGoodLumiSection()

Optionally, if you want to rescale to a given instantaneous luminosity:
1. input the instantaneous luminosity of the run (~line 32) [only if we scale to 2016 nominal]
2. select whether you rescale to L=1.5e34 (~line606??...) generally have it setup to rescale
nb: for 2&3 I have provided the info in runInfoForRates.txt
*/

// configurable parameters
double numBunch = 2544; //the number of bunches colliding for the run of interest
double runLum = 0.02; // 0.44: 275783  0.58:  276363 //luminosity of the run of interest (*10^34)
double expectedLum = 1.15; //expected luminosity of 2016 runs (*10^34)

// Run3 LHC parameters for normalizing rates
double instLumi = 2e34; // Hz/cm^2, https://indico.cern.ch/event/880508/contributions/3720014/attachments/1980197/3297287/CMS-Week_20200203_LHCStatus_Schaumann_v2.pdf
double mbXSec   = 6.92e-26; // cm^2, minimum bias cross section from Run2: https://twiki.cern.ch/twiki/bin/view/CMS/PileupJSONFileforData#Recommended_cross_section

void rates(bool isData, bool newConditions, const std::string& unique, const std::string& inputFileDirectory);

int main(int argc, char *argv[])
{
  bool newConditions = true;
  bool isData = true;
  std::string ntuplePath("");
  std::string unique("");

  if (argc <= 3) {
    std::cout << "Usage: rates.exe [mc/data] [new/def] [path to ntuples] [unique numeral]\n"
	      << "[mc/data] indicates running on mc or data\n"
	      << "[new/def] indicates new or default (existing) conditions" << std::endl;
    exit(1);
  }
  else {
    std::string par1(argv[2]);
    std::transform(par1.begin(), par1.end(), par1.begin(), ::tolower);
    if(par1.compare("new") == 0) newConditions = true;
    else if(par1.compare("def") == 0) newConditions = false;
    else {
      std::cout << "First parameter must be \"new\" or \"def\"" << std::endl;
      exit(1);
    }

    std::string par2(argv[1]);
    std::transform(par2.begin(), par2.end(), par2.begin(), ::tolower);
    if(par2.compare("mc") == 0) isData = false;
    else if(par2.compare("data") == 0) isData = true;
    else {
      std::cout << "First parameter must be \"mc\" or \"data\"" << std::endl;
      exit(1);
    }

    ntuplePath = argv[3];
    
    if (argc == 5) { unique     = argv[4];}
  }

  std::cout << "Running on ntuples: " << ntuplePath << std::endl;
  rates(isData, newConditions, unique, ntuplePath);

  return 0;
}

// only need to edit this section if good run JSON
// is not used during ntuple production
bool isGoodLumiSection(int lumiBlock)
{
  if (lumiBlock >= 1
      || lumiBlock <= 10000) {
    return true;
  }

  return false;
}

void rates(bool isData, bool newConditions, const std::string& unique, const std::string& inputFileDirectory){
  
  bool hwOn = true;   //are we using data from hardware? (upgrade trigger had to be running!!!)
  bool emuOn = true;  //are we using data from emulator?

  if (hwOn==false && emuOn==false){
    std::cout << "exiting as neither hardware or emulator selected" << std::endl;
    return;
  }

  std::string inputFile(inputFileDirectory);
  inputFile += "/L1Ntuple_"; inputFile += unique; inputFile += "*.root";
  std::string outputDirectory = "emu";  //***runNumber, triggerType, version, hw/emu/both***MAKE SURE IT EXISTS
  std::string outputFilename = "rates_def"; outputFilename += unique; outputFilename += ".root";
  if (newConditions) {
      outputFilename = "rates_new_cond";
      outputFilename += unique;
      outputFilename += ".root";
  }

  TFile* kk = TFile::Open( outputFilename.c_str() , "recreate");

  // make trees
  std::cout << "Loading up the TChain..." << std::endl;
  TChain * treeL1emu = new TChain("l1UpgradeEmuTree/L1UpgradeTree");
  if (emuOn){
    treeL1emu->Add(inputFile.c_str());
  }
  TChain * treeL1hw = new TChain("l1UpgradeTree/L1UpgradeTree");
  if (hwOn){
    treeL1hw->Add(inputFile.c_str());
  }
  TChain * eventTree = new TChain("l1EventTree/L1EventTree");
  eventTree->Add(inputFile.c_str());

  // In case you want to include PU info
  TChain * vtxTree = new TChain("l1RecoTree/RecoTree");
  int code = vtxTree->Add(inputFile.c_str());

  bool useReco = false;
  if (code != 0) useReco = true;
  else std::cout << "Will not be using RecoTree for vertex info!" << std::endl;

  TChain * treeL1TPemu = new TChain("l1CaloTowerEmuTree/L1CaloTowerTree");
  if (emuOn){
    treeL1TPemu->Add(inputFile.c_str());
  }

  TChain * treeL1TPhw = new TChain("l1CaloTowerTree/L1CaloTowerTree");
  if (hwOn){
    treeL1TPhw->Add(inputFile.c_str());
  }

  L1Analysis::L1AnalysisL1UpgradeDataFormat    *l1emu_ = new L1Analysis::L1AnalysisL1UpgradeDataFormat();
  treeL1emu->SetBranchAddress("L1Upgrade", &l1emu_);
  L1Analysis::L1AnalysisL1UpgradeDataFormat    *l1hw_ = new L1Analysis::L1AnalysisL1UpgradeDataFormat();
  treeL1hw->SetBranchAddress("L1Upgrade", &l1hw_);
  L1Analysis::L1AnalysisEventDataFormat    *event_ = new L1Analysis::L1AnalysisEventDataFormat();
  eventTree->SetBranchAddress("Event", &event_);

  L1Analysis::L1AnalysisRecoVertexDataFormat    *vtx_ = new L1Analysis::L1AnalysisRecoVertexDataFormat();
  if (useReco) vtxTree->SetBranchAddress("Vertex", &vtx_);

  L1Analysis::L1AnalysisCaloTPDataFormat    *l1TPemu_ = new L1Analysis::L1AnalysisCaloTPDataFormat();
  treeL1TPemu->SetBranchAddress("CaloTP", &l1TPemu_);
  L1Analysis::L1AnalysisCaloTPDataFormat    *l1TPhw_ = new L1Analysis::L1AnalysisCaloTPDataFormat();
  treeL1TPhw->SetBranchAddress("CaloTP", &l1TPhw_);


  // get number of entries
  Long64_t nentries;
  if (emuOn) nentries = treeL1emu->GetEntries();
  else nentries = treeL1hw->GetEntries();
  int goodLumiEventCount = 0;

  std::string outputTxtFilename = "output_rates/" + outputDirectory + "/extraInfo.txt";
  std::ofstream myfile; // save info about the run, including rates for a given lumi section, and number of events we used.
  myfile.open(outputTxtFilename.c_str());
  eventTree->GetEntry(0);
  myfile << "run number = " << event_->run << std::endl;

  // set parameters for histograms
  // jet bins
  int nJetBins = 400;
  float jetLo = 0.;
  float jetHi = 400.;
  float jetBinWidth = (jetHi-jetLo)/nJetBins;

  // EG bins
  int nEgBins = 300;
  float egLo = 0.;
  float egHi = 300.;
  float egBinWidth = (egHi-egLo)/nEgBins;

  // tau bins
  int nTauBins = 300;
  float tauLo = 0.;
  float tauHi = 300.;
  float tauBinWidth = (tauHi-tauLo)/nTauBins;

  // htSum bins
  int nHtSumBins = 1000;
  float htSumLo = 0.;
  float htSumHi = 1000.;
  float htSumBinWidth = (htSumHi-htSumLo)/nHtSumBins;

  // mhtSum bins
  int nMhtSumBins = 300;
  float mhtSumLo = 0.;
  float mhtSumHi = 300.;
  float mhtSumBinWidth = (mhtSumHi-mhtSumLo)/nMhtSumBins;

  // etSum bins
  int nEtSumBins = 1000;
  float etSumLo = 0.;
  float etSumHi = 1000.;
  float etSumBinWidth = (etSumHi-etSumLo)/nEtSumBins;

  // metSum bins
  int nMetSumBins = 300;
  float metSumLo = 0.;
  float metSumHi = 300.;
  float metSumBinWidth = (metSumHi-metSumLo)/nMetSumBins;

  // metHFSum bins
  int nMetHFSumBins = 300;
  float metHFSumLo = 0.;
  float metHFSumHi = 300.;
  float metHFSumBinWidth = (metHFSumHi-metHFSumLo)/nMetHFSumBins;

  // tp bins
  int nTpBins = 100;
  float tpLo = 0.;
  float tpHi = 100.;

  int nPVbins = 201;
  float pvLow = -0.5;
  float pvHi  = 200.5;

  std::string axR = ";Threshold E_{T} (GeV);nPV;rate (Hz)";
  std::string axET = ";E_{T} (GeV);nPV;Events / bin";
  std::string axHT = ";H_{T} (GeV);nPV;Events / bin";
  std::string axMET = ";MET (GeV);nPV;Events / bin";
  std::string axMHT = ";MHT (GeV);nPV;Events / bin";
  std::string axMETHF = ";MET HF (GeV);nPV;Events / bin";

  std::string axD = ";E_{T} (GeV);nPV;Events / bin"; 

  TH2F* singleJetRates_emu = new TH2F("singleJetRates_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* doubleJetRates_emu = new TH2F("doubleJetRates_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* tripleJetRates_emu = new TH2F("tripleJetRates_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* quadJetRates_emu = new TH2F("quadJetRates_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);

  TH2F* singleJetRatesHEall_emu = new TH2F("singleJetRatesHEall_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* doubleJetRatesHEall_emu = new TH2F("doubleJetRatesHEall_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* tripleJetRatesHEall_emu = new TH2F("tripleJetRatesHEall_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* quadJetRatesHEall_emu = new TH2F("quadJetRatesHEall_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);

  TH2F* singleJetRatesHEtag_emu = new TH2F("singleJetRatesHEtag_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* doubleJetRatesHEtag_emu = new TH2F("doubleJetRatesHEtag_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* tripleJetRatesHEtag_emu = new TH2F("tripleJetRatesHEtag_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* quadJetRatesHEtag_emu = new TH2F("quadJetRatesHEtag_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);

  TH2F* singleJetRatesHEtagLead_emu = new TH2F("singleJetRatesHEtagLead_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* doubleJetRatesHEtagLead_emu = new TH2F("doubleJetRatesHEtagLead_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* tripleJetRatesHEtagLead_emu = new TH2F("tripleJetRatesHEtagLead_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* quadJetRatesHEtagLead_emu = new TH2F("quadJetRatesHEtagLead_emu", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);

  TH2F* singleEgRates_emu = new TH2F("singleEgRates_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* doubleEgRates_emu = new TH2F("doubleEgRates_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* singleTauRates_emu = new TH2F("singleTauRates_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* doubleTauRates_emu = new TH2F("doubleTauRates_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* singleISOEgRates_emu = new TH2F("singleISOEgRates_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* doubleISOEgRates_emu = new TH2F("doubleISOEgRates_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* singleISOTauRates_emu = new TH2F("singleISOTauRates_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* doubleISOTauRates_emu = new TH2F("doubleISOTauRates_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);

  TH2F* singleEgRatesHEall_emu = new TH2F("singleEgRatesHEall_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* doubleEgRatesHEall_emu = new TH2F("doubleEgRatesHEall_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* singleTauRatesHEall_emu = new TH2F("singleTauRatesHEall_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* doubleTauRatesHEall_emu = new TH2F("doubleTauRatesHEall_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* singleISOEgRatesHEall_emu = new TH2F("singleISOEgRatesHEall_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* doubleISOEgRatesHEall_emu = new TH2F("doubleISOEgRatesHEall_emu", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* singleISOTauRatesHEall_emu = new TH2F("singleISOTauRatesHEall_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* doubleISOTauRatesHEall_emu = new TH2F("doubleISOTauRatesHEall_emu", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);

  TH2F* htSumRates_emu = new TH2F("htSumRates_emu",axR.c_str(), nHtSumBins, htSumLo, htSumHi, nPVbins, pvLow, pvHi);
  TH2F* mhtSumRates_emu = new TH2F("mhtSumRates_emu",axR.c_str(), nMhtSumBins, mhtSumLo, mhtSumHi, nPVbins, pvLow, pvHi);
  TH2F* etSumRates_emu = new TH2F("etSumRates_emu",axR.c_str(), nEtSumBins, etSumLo, etSumHi, nPVbins, pvLow, pvHi);
  TH2F* metSumRates_emu = new TH2F("metSumRates_emu",axR.c_str(), nMetSumBins, metSumLo, metSumHi, nPVbins, pvLow, pvHi); 
  TH2F* metHFSumRates_emu = new TH2F("metHFSumRates_emu",axR.c_str(), nMetHFSumBins, metHFSumLo, metHFSumHi, nPVbins, pvLow, pvHi); 

  TH2F* htSum_emu = new TH2F("htSum_emu",axHT.c_str(), nHtSumBins, htSumLo, htSumHi, nPVbins, pvLow, pvHi);
  TH2F* mhtSum_emu = new TH2F("mhtSum_emu",axMHT.c_str(), nMhtSumBins, mhtSumLo, mhtSumHi, nPVbins, pvLow, pvHi);
  TH2F* etSum_emu = new TH2F("etSum_emu",axET.c_str(), nEtSumBins, etSumLo, etSumHi, nPVbins, pvLow, pvHi);
  TH2F* metSum_emu = new TH2F("metSum_emu",axMET.c_str(), nMetSumBins, metSumLo, metSumHi, nPVbins, pvLow, pvHi); 
  TH2F* metHFSum_emu = new TH2F("metHFSum_emu",axMETHF.c_str(), nMetHFSumBins, metHFSumLo, metHFSumHi, nPVbins, pvLow, pvHi); 
  
  TH2F* singleJetRates_hw = new TH2F("singleJetRates_hw", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* doubleJetRates_hw = new TH2F("doubleJetRates_hw", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* tripleJetRates_hw = new TH2F("tripleJetRates_hw", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* quadJetRates_hw = new TH2F("quadJetRates_hw", axR.c_str(), nJetBins, jetLo, jetHi, nPVbins, pvLow, pvHi);
  TH2F* singleEgRates_hw = new TH2F("singleEgRates_hw", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* doubleEgRates_hw = new TH2F("doubleEgRates_hw", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* singleTauRates_hw = new TH2F("singleTauRates_hw", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* doubleTauRates_hw = new TH2F("doubleTauRates_hw", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* singleISOEgRates_hw = new TH2F("singleISOEgRates_hw", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* doubleISOEgRates_hw = new TH2F("doubleISOEgRates_hw", axR.c_str(), nEgBins, egLo, egHi, nPVbins, pvLow, pvHi);
  TH2F* singleISOTauRates_hw = new TH2F("singleISOTauRates_hw", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* doubleISOTauRates_hw = new TH2F("doubleISOTauRates_hw", axR.c_str(), nTauBins, tauLo, tauHi, nPVbins, pvLow, pvHi);
  TH2F* htSumRates_hw = new TH2F("htSumRates_hw",axR.c_str(), nHtSumBins, htSumLo, htSumHi, nPVbins, pvLow, pvHi);
  TH2F* mhtSumRates_hw = new TH2F("mhtSumRates_hw",axR.c_str(), nMhtSumBins, mhtSumLo, mhtSumHi, nPVbins, pvLow, pvHi);
  TH2F* etSumRates_hw = new TH2F("etSumRates_hw",axR.c_str(), nEtSumBins, etSumLo, etSumHi, nPVbins, pvLow, pvHi);
  TH2F* metSumRates_hw = new TH2F("metSumRates_hw",axR.c_str(), nMetHFSumBins, metHFSumLo, metHFSumHi, nPVbins, pvLow, pvHi); 
  TH2F* metHFSumRates_hw = new TH2F("metHFSumRates_hw",axR.c_str(), nMetHFSumBins, metHFSumLo, metHFSumHi, nPVbins, pvLow, pvHi); 

  TH2F* hcalTP_emu = new TH2F("hcalTP_emu", ";TP E_{T};nPV;# Entries", nTpBins, tpLo, tpHi, nPVbins, pvLow, pvHi);
  TH2F* ecalTP_emu = new TH2F("ecalTP_emu", ";TP E_{T};nPV;# Entries", nTpBins, tpLo, tpHi, nPVbins, pvLow, pvHi);

  TH2F* hcalTP_hw = new TH2F("hcalTP_hw", ";TP E_{T};nPV;# Entries", nTpBins, tpLo, tpHi,nPVbins, pvLow, pvHi);
  TH2F* ecalTP_hw = new TH2F("ecalTP_hw", ";TP E_{T};nPV;# Entries", nTpBins, tpLo, tpHi,nPVbins, pvLow, pvHi);

  /////////////////////////////////
  // loop through all the entries//
  /////////////////////////////////
  int maxEntry = -1; int procEntries = 0;
  for (Long64_t jentry=0; jentry<nentries; jentry++){
      if((jentry%10000)==0) std::cout << "Done " << jentry  << " events of " << nentries << std::endl;

      if (jentry == maxEntry) { break; } 

      //int Lumi = event_->lumi; int Run = event_->run;
      //lumi break clause
      eventTree->GetEntry(jentry);
      if (useReco) vtxTree->GetEntry(jentry);

      //skip the corresponding event
      //if (!isGoodLumiSection(event_->lumi)) continue;
      //goodLumiEventCount++;

      //std::vector<TString> trigPassPaths = event_->hlt;
      //bool passesTrig = false;
      //for (unsigned int iTrig = 0; iTrig < trigPassPaths.size(); iTrig++) {

      //  if (trigPassPaths[iTrig].Contains("HLT_ZeroBias_v")) {
      //      passesTrig = true;
      //      break;
      //  }

      //}

      //if (!passesTrig) continue;
      
      //int tempLumi = event_->lumi;
      //std::cout << "EVENT: " << event_->event << std::endl;
      //if (tempLumi != theLumi) {
      //  theLumi = tempLumi;
      //  //std::cout << "LUMI: " << tempLumi << std::endl;
      //}
      //continue;

      // Some royal BS to avoid run/lumi combos that are not present for new and default
      //if ((Run == 320824 and ((Lumi >= 301 and Lumi <= 306))) or
      //    (Run == 322252 and ((Lumi >= 1257 and Lumi <= 1265) or (Lumi >= 1269 and Lumi <= 1270) or (Lumi == 1272))) or
      //    (Run == 325170 and ((Lumi == 847) or (Lumi >= 849 and Lumi <= 853) or (Lumi >= 892 and Lumi <= 893) or (Lumi == 896) or (Lumi == 918) or (Lumi >= 920 and Lumi <= 921))) or
      //    (Run == 325170 and ((Lumi >= 931 and Lumi <= 933) or (Lumi >= 944 and Lumi <= 946) or (Lumi >= 983 and Lumi <= 985) or (Lumi >= 1007 and Lumi <= 1009) or (Lumi >= 1164 and Lumi <= 1165))) or
      //    (Run == 322252 and ((Lumi >= 642 and Lumi <= 644) or (Lumi == 647) or (Lumi >= 649 and Lumi <= 650) or (Lumi >= 675 and Lumi <= 676) or (Lumi == 680))) or
      //    (Run == 322252 and ((Lumi == 895) or (Lumi >= 897 and Lumi <= 898) or (Lumi >= 902 and Lumi <= 907))) or
      //    (Run == 320934 and ((Lumi == 759) or (Lumi == 764) or (Lumi == 768) or (Lumi == 775) or (Lumi == 779) or (Lumi == 784) or (Lumi >= 811 and Lumi <= 813) or (Lumi >= 820 and Lumi <= 821) or (Lumi == 824))) or
      //    (Run == 322252 and ((Lumi >= 557 and Lumi <= 562) or (Lumi >= 660 and Lumi <= 661) or (Lumi == 664))) or
      //    (Run == 320824 and ((Lumi >= 304 and Lumi <= 306) or (Lumi >= 319 and Lumi <= 324) or (Lumi >= 331 and Lumi <= 333) or (Lumi == 349) or (Lumi >= 352 and Lumi <= 353))) or
      //    (Run == 320824 and ((Lumi >= 259 and Lumi <= 261) or (Lumi >= 280 and Lumi <= 282) or (Lumi >= 289 and Lumi <= 291) or (Lumi >= 364 and Lumi <= 366) or (Lumi >= 391 and Lumi <= 393))) or
      //    (Run == 320824 and ((Lumi >= 388 and Lumi <= 390) or (Lumi >= 436 and Lumi <= 438) or (Lumi >= 526 and Lumi <= 528) or (Lumi >= 562 and Lumi <= 564) or (Lumi >= 583 and Lumi <= 585))) or
      //    (Run == 322252 and ((Lumi >= 1436 and Lumi <= 1441) or (Lumi >= 1453 and Lumi <= 1455) or (Lumi >= 1457 and Lumi <= 1459))) or
      //    (Run == 320824 and ((Lumi >= 388 and Lumi <= 390) or (Lumi >= 436 and Lumi <= 438) or (Lumi >= 526 and Lumi <= 528) or (Lumi >= 562 and Lumi <= 564) or (Lumi >= 583 and Lumi <= 585))) or
      //    (Run == 320824 and ((Lumi >= 847 and Lumi <= 852) or (Lumi >= 874 and Lumi <= 876) or (Lumi >= 886 and Lumi <= 888) or (Lumi >= 913 and Lumi <= 915) or (Lumi >= 929 and Lumi <= 931))) or 
      //    (Run == 325170 and ((Lumi == 831) or (Lumi >= 834 and Lumi <= 839) or (Lumi == 843) or (Lumi == 848) or (Lumi >= 874 and Lumi <= 876))) or
      //    (Run == 320934 and ((Lumi >= 206 and Lumi <= 207) or (Lumi == 210) or (Lumi == 229) or (Lumi >= 232 and Lumi <= 233) or (Lumi >= 244 and Lumi <= 245) or (Lumi == 248)))) { 

      //   continue;
      //}


      procEntries++;

      int nPV = 0;
      if (isData or useReco) nPV = vtx_->nVtx;
      else nPV = event_->nPV;

      //do routine for L1 emulator quantites
      if (emuOn){

          treeL1TPemu->GetEntry(jentry);
          double tpEt(0.);
        
          for(int i=0; i < l1TPemu_->nHCALTP; i++){
	          tpEt = l1TPemu_->hcalTPet[i];
              hcalTP_emu->Fill(tpEt, nPV);
          }
          for(int i=0; i < l1TPemu_->nECALTP; i++){
	          tpEt = l1TPemu_->ecalTPet[i];
	          ecalTP_emu->Fill(tpEt, nPV);
          }

          treeL1emu->GetEntry(jentry);

          // get jetEt*, egEt*, tauEt, htSum, mhtSum, etSum, metSum

          double jetEt_1 = -1.0; double jetEt_1_HEall = -1.0; double jetEt_1_HEtag = -1.0; double jetEt_1_HEtagLead = -1.0;
          double jetEt_2 = -1.0; double jetEt_2_HEall = -1.0; double jetEt_2_HEtag = -1.0; double jetEt_2_HEtagLead = -1.0;
          double jetEt_3 = -1.0; double jetEt_3_HEall = -1.0; double jetEt_3_HEtag = -1.0; double jetEt_3_HEtagLead = -1.0;
          double jetEt_4 = -1.0; double jetEt_4_HEall = -1.0; double jetEt_4_HEtag = -1.0; double jetEt_4_HEtagLead = -1.0;

          if (l1emu_->nJets>0) jetEt_1 = l1emu_->jetEt[0];
          if (l1emu_->nJets>1) jetEt_2 = l1emu_->jetEt[1];
          if (l1emu_->nJets>2) jetEt_3 = l1emu_->jetEt[2];
          if (l1emu_->nJets>3) jetEt_4 = l1emu_->jetEt[3];

          bool jet1HE = (l1emu_->nJets>0) ? (fabs(l1emu_->jetEta[0]) > 1.392 and fabs(l1emu_->jetEta[0]) < 3.0) : false;
          bool jet2HE = (l1emu_->nJets>1) ? (fabs(l1emu_->jetEta[1]) > 1.392 and fabs(l1emu_->jetEta[1]) < 3.0) : false;
          bool jet3HE = (l1emu_->nJets>2) ? (fabs(l1emu_->jetEta[2]) > 1.392 and fabs(l1emu_->jetEta[2]) < 3.0) : false;
          bool jet4HE = (l1emu_->nJets>3) ? (fabs(l1emu_->jetEta[3]) > 1.392 and fabs(l1emu_->jetEta[3]) < 3.0) : false;

          bool jetHEtag = jet1HE or jet2HE or jet3HE or jet4HE;
          bool jetHEtagLead = jet1HE;

          if (l1emu_->nJets>0 and jet1HE)                                  jetEt_1_HEall = l1emu_->jetEt[0];
          if (l1emu_->nJets>1 and jet1HE and jet2HE)                       jetEt_2_HEall = l1emu_->jetEt[1];
          if (l1emu_->nJets>2 and jet1HE and jet2HE and jet3HE)            jetEt_3_HEall = l1emu_->jetEt[2];
          if (l1emu_->nJets>3 and jet1HE and jet2HE and jet3HE and jet4HE) jetEt_4_HEall = l1emu_->jetEt[3];

          if (l1emu_->nJets>0 and jetHEtag) jetEt_1_HEtag = l1emu_->jetEt[0];
          if (l1emu_->nJets>1 and jetHEtag) jetEt_2_HEtag = l1emu_->jetEt[1];
          if (l1emu_->nJets>2 and jetHEtag) jetEt_3_HEtag = l1emu_->jetEt[2];
          if (l1emu_->nJets>3 and jetHEtag) jetEt_4_HEtag = l1emu_->jetEt[3];

          if (l1emu_->nJets>0 and jetHEtagLead) jetEt_1_HEtagLead = l1emu_->jetEt[0];
          if (l1emu_->nJets>1 and jetHEtagLead) jetEt_2_HEtagLead = l1emu_->jetEt[1];
          if (l1emu_->nJets>2 and jetHEtagLead) jetEt_3_HEtagLead = l1emu_->jetEt[2];
          if (l1emu_->nJets>3 and jetHEtagLead) jetEt_4_HEtagLead = l1emu_->jetEt[3];

          // Time for EGs //////////////////////

          double egEt_1 = -1.0; double egEta_1 = -1.0; double egEt_1_HEall = -1.0;
          double egEt_2 = -1.0; double egEta_2 = -1.0; double egEt_2_HEall = -1.0;
          //EG pt's are not given in descending order...bx?
          for (UInt_t c=0; c<l1emu_->nEGs; c++){
            if (l1emu_->egEt[c] > egEt_1){
              egEt_2 = egEt_1; egEta_2 = egEta_1;
              egEt_1 = l1emu_->egEt[c]; egEta_1 = l1emu_->egEta[c];
            }
            else if (l1emu_->egEt[c] <= egEt_1 && l1emu_->egEt[c] > egEt_2){
              egEt_2 = l1emu_->egEt[c]; egEta_2 = l1emu_->egEta[c];
            }
          }

          bool eg1HE = (l1emu_->nEGs>0) ? (fabs(egEta_1) > 1.392 and fabs(egEta_1) < 3.0) : false;
          bool eg2HE = (l1emu_->nEGs>1) ? (fabs(egEta_2) > 1.392 and fabs(egEta_2) < 3.0) : false;

          if (eg1HE) egEt_1_HEall = egEt_1;
          if (eg2HE) egEt_2_HEall = egEt_2;

          // Time for taus //////////////////////

          double tauEt_1 = -1.0; double tauEta_1 = -1.0; double tauEt_1_HEall = -1.0;
          double tauEt_2 = -1.0; double tauEta_2 = -1.0; double tauEt_2_HEall = -1.0;
          //tau pt's are not given in descending order
          for (UInt_t c=0; c<l1emu_->nTaus; c++){
            if (l1emu_->tauEt[c] > tauEt_1){
              tauEt_2 = tauEt_1; tauEta_2 = tauEta_1;
              tauEt_1 = l1emu_->tauEt[c]; tauEta_1 = l1emu_->tauEta[c];
            }
            else if (l1emu_->tauEt[c] <= tauEt_1 && l1emu_->tauEt[c] > tauEt_2){
              tauEt_2 = l1emu_->tauEt[c]; tauEta_2 = l1emu_->tauEta[c];
            }
          }

          bool tau1HE = (l1emu_->nTaus>0) ? (fabs(tauEta_1) > 1.392 and fabs(tauEta_1) < 3.0) : false;
          bool tau2HE = (l1emu_->nTaus>1) ? (fabs(tauEta_2) > 1.392 and fabs(tauEta_2) < 3.0) : false;

          if (tau1HE) tauEt_1_HEall = tauEt_1;
          if (tau2HE) tauEt_2_HEall = tauEt_2;

          // Time for ISO EGs ///////////////////

          double egISOEt_1 = -1.0; double egISOEta_1 = -1.0; double egISOEt_1_HEall = -1.0;
          double egISOEt_2 = -1.0; double egISOEta_2 = -1.0; double egISOEt_2_HEall = -1.0;
          //EG pt's are not given in descending order...bx?
          for (UInt_t c=0; c<l1emu_->nEGs; c++){
            if (l1emu_->egEt[c] > egISOEt_1 && l1emu_->egIso[c]==1){
              egISOEt_2 = egISOEt_1; egISOEta_2 = egISOEta_1;
              egISOEt_1 = l1emu_->egEt[c]; egISOEta_1 = l1emu_->egEta[c];
            }
            else if (l1emu_->egEt[c] <= egISOEt_1 && l1emu_->egEt[c] > egISOEt_2 && l1emu_->egIso[c]==1){
              egISOEt_2 = l1emu_->egEt[c]; egISOEta_2 = l1emu_->egEta[c];
            }
          }

          bool egISO1HE = (l1emu_->nEGs>0) ? (fabs(egISOEta_1) > 1.392 and fabs(egISOEta_1) < 3.0) : false;
          bool egISO2HE = (l1emu_->nEGs>1) ? (fabs(egISOEta_2) > 1.392 and fabs(egISOEta_2) < 3.0) : false;

          if (egISO1HE) egISOEt_1_HEall = egISOEt_1;
          if (egISO2HE) egISOEt_2_HEall = egISOEt_2;

          // Time for ISO taus //////////////////

          double tauISOEt_1 = -1.0; double tauISOEta_1 = -1.0; double tauISOEt_1_HEall = -1.0;
          double tauISOEt_2 = -1.0; double tauISOEta_2 = -1.0; double tauISOEt_2_HEall = -1.0;
          //tau pt's are not given in descending order
          for (UInt_t c=0; c<l1emu_->nTaus; c++){
            if (l1emu_->tauEt[c] > tauISOEt_1 && l1emu_->tauIso[c]>0){
              tauISOEt_2 = tauISOEt_1; tauISOEta_2 = tauISOEta_1;
              tauISOEt_1 = l1emu_->tauEt[c]; tauISOEta_1 = l1emu_->tauEta[c];
            }
            else if (l1emu_->tauEt[c] <= tauISOEt_1 && l1emu_->tauEt[c] > tauISOEt_2 && l1emu_->tauIso[c]>0){
              tauISOEt_2 = l1emu_->tauEt[c]; tauISOEta_2 = l1emu_->tauEta[c];
            }
          }

          bool tauISO1HE = (l1emu_->nTaus>0) ? (fabs(tauISOEta_1) > 1.392 and fabs(tauISOEta_1) < 3.0) : false;
          bool tauISO2HE = (l1emu_->nTaus>1) ? (fabs(tauISOEta_2) > 1.392 and fabs(tauISOEta_2) < 3.0) : false;

          if (tauISO1HE) tauISOEt_1_HEall = tauISOEt_1;
          if (tauISO2HE) tauISOEt_2_HEall = tauISOEt_2;

          double htSum = -1.0;
          double mhtSum = -1.0;
          double etSum = -1.0;
          double metSum = -1.0;
          double metHFSum = -1.0;
          for (unsigned int c=0; c<l1emu_->nSums; c++){
              if( l1emu_->sumBx[c] != 0 ) continue;
              if( l1emu_->sumType[c] == L1Analysis::kTotalEt ) etSum = l1emu_->sumEt[c];
              if( l1emu_->sumType[c] == L1Analysis::kTotalHt ) htSum = l1emu_->sumEt[c];
              if( l1emu_->sumType[c] == L1Analysis::kMissingEt ) metSum = l1emu_->sumEt[c];
	          if( l1emu_->sumType[c] == L1Analysis::kMissingEtHF ) metHFSum = l1emu_->sumEt[c];
              if( l1emu_->sumType[c] == L1Analysis::kMissingHt ) mhtSum = l1emu_->sumEt[c];
          }
          htSum_emu->Fill(htSum, nPV);
          mhtSum_emu->Fill(mhtSum, nPV);
          etSum_emu->Fill(etSum, nPV);
          metSum_emu->Fill(metSum, nPV);
          metHFSum_emu->Fill(metHFSum, nPV);

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_1 ) >= jetLo + (bin*jetBinWidth) ) {
               singleJetRates_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_2) >= jetLo + (bin*jetBinWidth) ) {
             doubleJetRates_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_3) >= jetLo + (bin*jetBinWidth) ) {
                tripleJetRates_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_4) >= jetLo + (bin*jetBinWidth) ) {
                quadJetRates_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  
                 
          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_1_HEall ) >= jetLo + (bin*jetBinWidth) ) {
               singleJetRatesHEall_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_2_HEall ) >= jetLo + (bin*jetBinWidth) ) {
             doubleJetRatesHEall_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_3_HEall ) >= jetLo + (bin*jetBinWidth) ) {
                tripleJetRatesHEall_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_4_HEall ) >= jetLo + (bin*jetBinWidth) ) {
                quadJetRatesHEall_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_1_HEtag ) >= jetLo + (bin*jetBinWidth) ) {
               singleJetRatesHEtag_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_2_HEtag ) >= jetLo + (bin*jetBinWidth) ) {
             doubleJetRatesHEtag_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_3_HEtag ) >= jetLo + (bin*jetBinWidth) ) {
                tripleJetRatesHEtag_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_4_HEtag ) >= jetLo + (bin*jetBinWidth) ) {
                quadJetRatesHEtag_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_1_HEtagLead ) >= jetLo + (bin*jetBinWidth) ) {
               singleJetRatesHEtagLead_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_2_HEtagLead ) >= jetLo + (bin*jetBinWidth) ) {
             doubleJetRatesHEtagLead_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_3_HEtagLead ) >= jetLo + (bin*jetBinWidth) ) {
                tripleJetRatesHEtagLead_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nJetBins; bin++){
            if( (jetEt_4_HEtagLead ) >= jetLo + (bin*jetBinWidth) ) {
                quadJetRatesHEtagLead_emu->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nEgBins; bin++){
            if( (egEt_1) >= egLo + (bin*egBinWidth) ) {
             singleEgRates_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egEt_2) >= egLo + (bin*egBinWidth) ) {
             doubleEgRates_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauEt_1) >= tauLo + (bin*tauBinWidth) ) {
             singleTauRates_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauEt_2) >= tauLo + (bin*tauBinWidth) ) {
             doubleTauRates_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egISOEt_1) >= egLo + (bin*egBinWidth) ) {
             singleISOEgRates_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egISOEt_2) >= egLo + (bin*egBinWidth) ) {
             doubleISOEgRates_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauISOEt_1) >= tauLo + (bin*tauBinWidth) ) {
             singleISOTauRates_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauISOEt_2) >= tauLo + (bin*tauBinWidth) ) {
             doubleISOTauRates_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egEt_1_HEall) >= egLo + (bin*egBinWidth) ) {
             singleEgRatesHEall_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egEt_2_HEall) >= egLo + (bin*egBinWidth) ) {
             doubleEgRatesHEall_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauEt_1_HEall) >= tauLo + (bin*tauBinWidth) ) {
             singleTauRatesHEall_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauEt_2_HEall) >= tauLo + (bin*tauBinWidth) ) {
             doubleTauRatesHEall_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egISOEt_1_HEall) >= egLo + (bin*egBinWidth) ) {
             singleISOEgRatesHEall_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          } 

          for(int bin=0; bin<nEgBins; bin++){
            if( (egISOEt_2_HEall) >= egLo + (bin*egBinWidth) ) {
             doubleISOEgRatesHEall_emu->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
            }
          }  

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauISOEt_1_HEall) >= tauLo + (bin*tauBinWidth) ) {
             singleISOTauRatesHEall_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          }

          for(int bin=0; bin<nTauBins; bin++){
            if( (tauISOEt_2_HEall) >= tauLo + (bin*tauBinWidth) ) {
             doubleISOTauRatesHEall_emu->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
            }
          } 


          for(int bin=0; bin<nHtSumBins; bin++){
            if( (htSum) >= htSumLo+(bin*htSumBinWidth) ) htSumRates_emu->Fill(htSumLo+(bin*htSumBinWidth),nPV); //GeV
          }

          for(int bin=0; bin<nMhtSumBins; bin++){
            if( (mhtSum) >= mhtSumLo+(bin*mhtSumBinWidth) ) mhtSumRates_emu->Fill(mhtSumLo+(bin*mhtSumBinWidth),nPV); //GeV           
          }

          for(int bin=0; bin<nEtSumBins; bin++){
            if( (etSum) >= etSumLo+(bin*etSumBinWidth) ) etSumRates_emu->Fill(etSumLo+(bin*etSumBinWidth),nPV); //GeV           
          }

          for(int bin=0; bin<nMetSumBins; bin++){
            if( (metSum) >= metSumLo+(bin*metSumBinWidth) ) metSumRates_emu->Fill(metSumLo+(bin*metSumBinWidth),nPV); //GeV           
          }
          for(int bin=0; bin<nMetHFSumBins; bin++){
            if( (metHFSum) >= metHFSumLo+(bin*metHFSumBinWidth) ) metHFSumRates_emu->Fill(metHFSumLo+(bin*metHFSumBinWidth),nPV); //GeV           
          }


    }// closes if 'emuOn' is true


    //do routine for L1 hardware quantities
    if (hwOn){

      treeL1TPhw->GetEntry(jentry);
      double tpEt(0.);
      
      for(int i=0; i < l1TPhw_->nHCALTP; i++){
	tpEt = l1TPhw_->hcalTPet[i];
	hcalTP_hw->Fill(tpEt,nPV);
      }
      for(int i=0; i < l1TPhw_->nECALTP; i++){
	tpEt = l1TPhw_->ecalTPet[i];
	ecalTP_hw->Fill(tpEt,nPV);
      }


      treeL1hw->GetEntry(jentry);
      // get jetEt*, egEt*, tauEt, htSum, mhtSum, etSum, metSum
      // ***INCLUDES NON_ZERO bx*** can't just read values off
      double jetEt_1 = 0;
      double jetEt_2 = 0;
      double jetEt_3 = 0;
      double jetEt_4 = 0;
      for (UInt_t c=0; c<l1hw_->nJets; c++){
        if (l1hw_->jetBx[c]==0 && l1hw_->jetEt[c] > jetEt_1){
          jetEt_4 = jetEt_3;
          jetEt_3 = jetEt_2;
          jetEt_2 = jetEt_1;
          jetEt_1 = l1hw_->jetEt[c];
        }
        else if (l1hw_->jetBx[c]==0 && l1hw_->jetEt[c] <= jetEt_1 && l1hw_->jetEt[c] > jetEt_2){
          jetEt_4 = jetEt_3;
          jetEt_3 = jetEt_2;      
          jetEt_2 = l1hw_->jetEt[c];
        }
        else if (l1hw_->jetBx[c]==0 && l1hw_->jetEt[c] <= jetEt_2 && l1hw_->jetEt[c] > jetEt_3){
          jetEt_4 = jetEt_3;     
          jetEt_3 = l1hw_->jetEt[c];
        }
        else if (l1hw_->jetBx[c]==0 && l1hw_->jetEt[c] <= jetEt_3 && l1hw_->jetEt[c] > jetEt_4){   
          jetEt_4 = l1hw_->jetEt[c];
        }
      }

      double egEt_1 = 0;
      double egEt_2 = 0;
      for (UInt_t c=0; c<l1hw_->nEGs; c++){
        if (l1hw_->egBx[c]==0 && l1hw_->egEt[c] > egEt_1){
          egEt_2 = egEt_1;
          egEt_1 = l1hw_->egEt[c];
        }
        else if (l1hw_->egBx[c]==0 && l1hw_->egEt[c] <= egEt_1 && l1hw_->egEt[c] > egEt_2){
          egEt_2 = l1hw_->egEt[c];
        }
      }

      double tauEt_1 = 0;
      double tauEt_2 = 0;
      //tau pt's are not given in descending order
      for (UInt_t c=0; c<l1hw_->nTaus; c++){
        if (l1hw_->tauBx[c]==0 && l1hw_->tauEt[c] > tauEt_1){
          tauEt_1 = l1hw_->tauEt[c];
        }
        else if (l1hw_->tauBx[c]==0 && l1hw_->tauEt[c] <= tauEt_1 && l1hw_->tauEt[c] > tauEt_2){
          tauEt_2 = l1hw_->tauEt[c];
        }
      }

      double egISOEt_1 = 0;
      double egISOEt_2 = 0;
      //EG pt's are not given in descending order...bx?
      for (UInt_t c=0; c<l1hw_->nEGs; c++){
        if (l1hw_->egBx[c]==0 && l1hw_->egEt[c] > egISOEt_1 && l1hw_->egIso[c]==1){
          egISOEt_2 = egISOEt_1;
          egISOEt_1 = l1hw_->egEt[c];
        }
        else if (l1hw_->egBx[c]==0 && l1hw_->egEt[c] <= egISOEt_1 && l1hw_->egEt[c] > egISOEt_2 && l1hw_->egIso[c]==1){
          egISOEt_2 = l1hw_->egEt[c];
        }
      }

      double tauISOEt_1 = 0;
      double tauISOEt_2 = 0;
      //tau pt's are not given in descending order
      for (UInt_t c=0; c<l1hw_->nTaus; c++){
        if (l1hw_->tauBx[c]==0 && l1hw_->tauEt[c] > tauISOEt_1 && l1hw_->tauIso[c]>0){
          tauISOEt_2 = tauISOEt_1;
          tauISOEt_1 = l1hw_->tauEt[c];
        }
        else if (l1hw_->tauBx[c]==0 && l1hw_->tauEt[c] <= tauISOEt_1 && l1hw_->tauEt[c] > tauISOEt_2 && l1hw_->tauIso[c]>0){
          tauISOEt_2 = l1hw_->tauEt[c];
        }
      }

      double htSum = 0;
      double mhtSum = 0;
      double etSum = 0;
      double metSum = 0;
      double metHFSum = 0;
      // HW includes -2,-1,0,1,2 bx info (hence the different numbers, could cause a seg fault if this changes)
      for (unsigned int c=0; c<l1hw_->nSums; c++){
          if( l1hw_->sumBx[c] != 0 ) continue;
          if( l1hw_->sumType[c] == L1Analysis::kTotalEt ) etSum = l1hw_->sumEt[c];
          if( l1hw_->sumType[c] == L1Analysis::kTotalHt ) htSum = l1hw_->sumEt[c];
          if( l1hw_->sumType[c] == L1Analysis::kMissingEt ) metSum = l1hw_->sumEt[c];
	      if( l1hw_->sumType[c] == L1Analysis::kMissingEtHF ) metHFSum = l1hw_->sumEt[c];
          if( l1hw_->sumType[c] == L1Analysis::kMissingHt ) mhtSum = l1hw_->sumEt[c];
      }

      // for each bin fill according to whether our object has a larger corresponding energy
      for(int bin=0; bin<nJetBins; bin++){
        if( (jetEt_1) >= jetLo + (bin*jetBinWidth) ) singleJetRates_hw->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
      } 

      for(int bin=0; bin<nJetBins; bin++){
        if( (jetEt_2) >= jetLo + (bin*jetBinWidth) ) doubleJetRates_hw->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
      }  

      for(int bin=0; bin<nJetBins; bin++){
        if( (jetEt_3) >= jetLo + (bin*jetBinWidth) ) tripleJetRates_hw->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
      }  

      for(int bin=0; bin<nJetBins; bin++){
        if( (jetEt_4) >= jetLo + (bin*jetBinWidth) ) quadJetRates_hw->Fill(jetLo+(bin*jetBinWidth),nPV);  //GeV
      }  
             
      for(int bin=0; bin<nEgBins; bin++){
        if( (egEt_1) >= egLo + (bin*egBinWidth) ) singleEgRates_hw->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
      } 

      for(int bin=0; bin<nEgBins; bin++){
        if( (egEt_2) >= egLo + (bin*egBinWidth) ) doubleEgRates_hw->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
      }  

      for(int bin=0; bin<nTauBins; bin++){
        if( (tauEt_1) >= tauLo + (bin*tauBinWidth) ) singleTauRates_hw->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
      } 

      for(int bin=0; bin<nTauBins; bin++){
        if( (tauEt_2) >= tauLo + (bin*tauBinWidth) ) doubleTauRates_hw->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
      } 

      for(int bin=0; bin<nEgBins; bin++){
        if( (egISOEt_1) >= egLo + (bin*egBinWidth) ) singleISOEgRates_hw->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
      } 

      for(int bin=0; bin<nEgBins; bin++){
        if( (egISOEt_2) >= egLo + (bin*egBinWidth) ) doubleISOEgRates_hw->Fill(egLo+(bin*egBinWidth),nPV);  //GeV
      }  

      for(int bin=0; bin<nTauBins; bin++){
        if( (tauISOEt_1) >= tauLo + (bin*tauBinWidth) ) singleISOTauRates_hw->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
      }

      for(int bin=0; bin<nTauBins; bin++){
        if( (tauISOEt_2) >= tauLo + (bin*tauBinWidth) ) doubleISOTauRates_hw->Fill(tauLo+(bin*tauBinWidth),nPV);  //GeV
      } 

      for(int bin=0; bin<nHtSumBins; bin++){
        if( (htSum) >= htSumLo+(bin*htSumBinWidth) ) htSumRates_hw->Fill(htSumLo+(bin*htSumBinWidth),nPV); //GeV
      }

      for(int bin=0; bin<nMhtSumBins; bin++){
        if( (mhtSum) >= mhtSumLo+(bin*mhtSumBinWidth) ) mhtSumRates_hw->Fill(mhtSumLo+(bin*mhtSumBinWidth),nPV); //GeV           
      }

      for(int bin=0; bin<nEtSumBins; bin++){
        if( (etSum) >= etSumLo+(bin*etSumBinWidth) ) etSumRates_hw->Fill(etSumLo+(bin*etSumBinWidth),nPV); //GeV           
      }

      for(int bin=0; bin<nMetSumBins; bin++){
        if( (metSum) >= metSumLo+(bin*metSumBinWidth) ) metSumRates_hw->Fill(metSumLo+(bin*metSumBinWidth),nPV); //GeV           
      } 
      for(int bin=0; bin<nMetHFSumBins; bin++){
        if( (metHFSum) >= metHFSumLo+(bin*metHFSumBinWidth) ) metHFSumRates_hw->Fill(metHFSumLo+(bin*metHFSumBinWidth),nPV); //GeV           
      } 

    }// closes if 'hwOn' is true

  }// closes loop through events

  std::cout << "Processed " << procEntries << " entries." << std::endl;

  //  TFile g( outputFilename.c_str() , "new");
  kk->cd();
  // normalisation factor for rate histograms (11kHz is the orbit frequency)
  // double norm = 11246*(numBunch/goodLumiEventCount); // no lumi rescale
  //  double norm = 11246*(numBunch/goodLumiEventCount)*(expectedLum/runLum); //scale to nominal lumi
  //  Run3 normalization ==> inst lumi * min bias xsec / <PU>
  double norm = instLumi * mbXSec / procEntries;

  if (emuOn){
    singleJetRates_emu->Scale(norm);
    doubleJetRates_emu->Scale(norm);
    tripleJetRates_emu->Scale(norm);
    quadJetRates_emu->Scale(norm);

    singleJetRatesHEall_emu->Scale(norm);
    doubleJetRatesHEall_emu->Scale(norm);
    tripleJetRatesHEall_emu->Scale(norm);
    quadJetRatesHEall_emu->Scale(norm);

    singleJetRatesHEtag_emu->Scale(norm);
    doubleJetRatesHEtag_emu->Scale(norm);
    tripleJetRatesHEtag_emu->Scale(norm);
    quadJetRatesHEtag_emu->Scale(norm);

    singleJetRatesHEtagLead_emu->Scale(norm);
    doubleJetRatesHEtagLead_emu->Scale(norm);
    tripleJetRatesHEtagLead_emu->Scale(norm);
    quadJetRatesHEtagLead_emu->Scale(norm);

    singleEgRates_emu->Scale(norm);
    doubleEgRates_emu->Scale(norm);
    singleTauRates_emu->Scale(norm);
    doubleTauRates_emu->Scale(norm);
    singleISOEgRates_emu->Scale(norm);
    doubleISOEgRates_emu->Scale(norm);
    singleISOTauRates_emu->Scale(norm);
    doubleISOTauRates_emu->Scale(norm);

    singleEgRatesHEall_emu->Scale(norm);
    doubleEgRatesHEall_emu->Scale(norm);
    singleTauRatesHEall_emu->Scale(norm);
    doubleTauRatesHEall_emu->Scale(norm);
    singleISOEgRatesHEall_emu->Scale(norm);
    doubleISOEgRatesHEall_emu->Scale(norm);
    singleISOTauRatesHEall_emu->Scale(norm);
    doubleISOTauRatesHEall_emu->Scale(norm);

    htSumRates_emu->Scale(norm);
    mhtSumRates_emu->Scale(norm);
    etSumRates_emu->Scale(norm);
    metSumRates_emu->Scale(norm);
    metHFSumRates_emu->Scale(norm);

    htSum_emu->Write();
    mhtSum_emu->Write();
    etSum_emu->Write();
    metSum_emu->Write();
    metHFSum_emu->Write();

    //set the errors for the rates
    //want error -> error * sqrt(norm) ?

    hcalTP_emu->Write();
    ecalTP_emu->Write();

    singleJetRates_emu->Write();
    doubleJetRates_emu->Write();
    tripleJetRates_emu->Write();
    quadJetRates_emu->Write();

    singleJetRatesHEall_emu->Write();
    doubleJetRatesHEall_emu->Write();
    tripleJetRatesHEall_emu->Write();
    quadJetRatesHEall_emu->Write();

    singleJetRatesHEtag_emu->Write();
    doubleJetRatesHEtag_emu->Write();
    tripleJetRatesHEtag_emu->Write();
    quadJetRatesHEtag_emu->Write();

    singleJetRatesHEtagLead_emu->Write();
    doubleJetRatesHEtagLead_emu->Write();
    tripleJetRatesHEtagLead_emu->Write();
    quadJetRatesHEtagLead_emu->Write();

    singleEgRates_emu->Write();
    doubleEgRates_emu->Write();
    singleTauRates_emu->Write();
    doubleTauRates_emu->Write();
    singleISOEgRates_emu->Write();
    doubleISOEgRates_emu->Write();
    singleISOTauRates_emu->Write();
    doubleISOTauRates_emu->Write();

    singleEgRatesHEall_emu->Write();
    doubleEgRatesHEall_emu->Write();
    singleTauRatesHEall_emu->Write();
    doubleTauRatesHEall_emu->Write();
    singleISOEgRatesHEall_emu->Write();
    doubleISOEgRatesHEall_emu->Write();
    singleISOTauRatesHEall_emu->Write();
    doubleISOTauRatesHEall_emu->Write();

    htSumRates_emu->Write();
    mhtSumRates_emu->Write();
    etSumRates_emu->Write();
    metSumRates_emu->Write();
    metHFSumRates_emu->Write();
  }

  if (hwOn){

    singleJetRates_hw->Scale(norm);
    doubleJetRates_hw->Scale(norm);
    tripleJetRates_hw->Scale(norm);
    quadJetRates_hw->Scale(norm);
    singleEgRates_hw->Scale(norm);
    doubleEgRates_hw->Scale(norm);
    singleTauRates_hw->Scale(norm);
    doubleTauRates_hw->Scale(norm);
    singleISOEgRates_hw->Scale(norm);
    doubleISOEgRates_hw->Scale(norm);
    singleISOTauRates_hw->Scale(norm);
    doubleISOTauRates_hw->Scale(norm);
    htSumRates_hw->Scale(norm);
    mhtSumRates_hw->Scale(norm);
    etSumRates_hw->Scale(norm);
    metSumRates_hw->Scale(norm);
    metHFSumRates_hw->Scale(norm);

    hcalTP_hw->Write();
    ecalTP_hw->Write();
    singleJetRates_hw->Write();
    doubleJetRates_hw->Write();
    tripleJetRates_hw->Write();
    quadJetRates_hw->Write();
    singleEgRates_hw->Write();
    doubleEgRates_hw->Write();
    singleTauRates_hw->Write();
    doubleTauRates_hw->Write();
    singleISOEgRates_hw->Write();
    doubleISOEgRates_hw->Write();
    singleISOTauRates_hw->Write();
    doubleISOTauRates_hw->Write();
    htSumRates_hw->Write();
    mhtSumRates_hw->Write();
    etSumRates_hw->Write();
    metSumRates_hw->Write();
    metHFSumRates_hw->Write();
  }
  myfile << "using the following ntuple: " << inputFile << std::endl;
  myfile << "number of colliding bunches = " << numBunch << std::endl;
  myfile << "run luminosity = " << runLum << std::endl;
  myfile << "expected luminosity = " << expectedLum << std::endl;
  myfile << "norm factor used = " << norm << std::endl;
  myfile << "number of good events = " << goodLumiEventCount << std::endl;
  myfile.close(); 
}//closes the function 'rates'
