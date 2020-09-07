#include "PhysicsTools/Utilities/macros/setTDRStyle.C"

#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TF1.h"
#include "TFile.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TGraphAsymmErrors.h"

#include <map>
#include <string>
#include <vector>

class l1Plotter {
public:
    
    l1Plotter() {

        for(auto file : filenames_) { files_.push_back(TFile::Open(file.c_str())); }
    };

    void plotRes(std::string refName, std::string typeName, std::string xtitle, std::string region = "") {
    
        TF1 *fgaus = new TF1("g1","gaus");
        fgaus->SetRange(-1.,1.);
    
        std::string histo2Dname = refName + region;
        std::string histo1Dname_1 = histo2Dname + "_1"; std::string histo1Dname_yx_1 = histo2Dname + "_yx_1";
        std::string histo1Dname_2 = histo2Dname + "_2"; std::string histo1Dname_yx_2 = histo2Dname + "_yx_2";
    
        std::string histo1Dname_1_new_cond = histo1Dname_1 + "_" + region + "_new_cond";
        std::string histo1Dname_2_new_cond = histo1Dname_2 + "_" + region + "_new_cond";
    
        std::string histo1Dname_1_def = histo1Dname_1 + "_" + region + "_def";
        std::string histo1Dname_2_def = histo1Dname_2 + "_" + region + "_def";
    
        std::string meanName, sigmaName;
        if (region == "") {
            meanName = "res" + typeName + "_mean";
            sigmaName = "res" + typeName + "_sigma";
        } else {
            meanName = "res" + typeName + "_mean_" + region;
            sigmaName = "res" + typeName + "_sigma_" + region;
        }
    
        TH3F* deftemp = dynamic_cast<TH3F*>(files_.at(0)->Get(histo2Dname.c_str()));
        TH3F* newtemp = dynamic_cast<TH3F*>(files_.at(1)->Get(histo2Dname.c_str()));
    
        for (unsigned int irange = 0; irange < puRanges_.size(); irange++) {
    
            std::string custom = puRangeNames_[irange]; std::vector<int> bounds = puRanges_[irange];
            int lowBin = deftemp->GetZaxis()->FindBin(bounds[0]); int hiBin = deftemp->GetZaxis()->FindBin(bounds[1]);
    
            deftemp->GetZaxis()->SetRange(lowBin, hiBin); 
            newtemp->GetZaxis()->SetRange(lowBin, hiBin);
    
            TH2D* resHistos2D_def = dynamic_cast<TH2D*>(deftemp->Project3D("yx")); resHistos2D_def->RebinX(10);
            resHistos2D_def->FitSlicesY(fgaus);
    
            TH1D* resHistos1D_1_def = (TH1D*)gDirectory->Get((histo1Dname_yx_1).c_str()); resHistos1D_1_def->SetName((histo1Dname_1_def+custom).c_str());
            TH1D* resHistos1D_2_def = (TH1D*)gDirectory->Get((histo1Dname_yx_2).c_str()); resHistos1D_2_def->SetName((histo1Dname_2_def+custom).c_str());
    
            gROOT->cd();
    
            TH2D* resHistos2D_new_cond = dynamic_cast<TH2D*>(newtemp->Project3D("yx")); resHistos2D_new_cond->RebinX(10);
            resHistos2D_new_cond->FitSlicesY(fgaus);
            TH1D* resHistos1D_1_new_cond = (TH1D*)gDirectory->Get((histo1Dname_yx_1).c_str()); resHistos1D_1_new_cond->SetName((histo1Dname_1_new_cond+custom).c_str());
            TH1D* resHistos1D_2_new_cond = (TH1D*)gDirectory->Get((histo1Dname_yx_2).c_str()); resHistos1D_2_new_cond->SetName((histo1Dname_2_new_cond+custom).c_str());
            gROOT->cd();
      
            TCanvas* c1 = new TCanvas;
            c1->SetWindowSize(c1->GetWw(), 1.*c1->GetWh());
    
            gPad->SetGridx(); gPad->SetGridy();
       
            std::string newname(resHistos1D_1_new_cond->GetName()); newname += "_ratio";
            TH1D* resHistos1D_1_ratio = (TH1D*)resHistos1D_1_new_cond->Clone(newname.c_str());
            resHistos1D_1_ratio->Add(resHistos1D_1_def, -1.0);

            TH1D* resHistos1D_1_def_abs = (TH1D*)resHistos1D_1_def->Clone((newname+"_abs").c_str());
            for (int i = 1; i < resHistos1D_1_def->GetNbinsX()+1; i++) {
                
                resHistos1D_1_def_abs->SetBinContent(i, fabs(resHistos1D_1_def->GetBinContent(i)));
                resHistos1D_1_def_abs->SetBinError(i, resHistos1D_1_def->GetBinError(i));
            }

            resHistos1D_1_ratio->Divide(resHistos1D_1_def_abs);
    
            resHistos1D_1_ratio->SetTitle("");
            resHistos1D_1_ratio->SetMarkerSize(mSize_);
            resHistos1D_1_ratio->SetLineWidth(lWidth_);
            resHistos1D_1_ratio->SetMarkerColor(kBlue-2);
            resHistos1D_1_ratio->SetLineColor(kBlue-2);
            resHistos1D_1_ratio->GetYaxis()->SetTitle("#frac{New-Default}{Default}");
            resHistos1D_1_ratio->GetXaxis()->SetTitle(xtitle.c_str());
            resHistos1D_1_ratio->GetYaxis()->SetRangeUser(-0.8, 0.3);
            resHistos1D_1_ratio->GetYaxis()->SetNdivisions(3,5,0);
            resHistos1D_1_ratio->GetXaxis()->SetNdivisions(5,10,0);
            resHistos1D_1_ratio->GetYaxis()->SetTitleSize(resHistos1D_1_def->GetYaxis()->GetTitleSize()*(0.7/0.3));
            resHistos1D_1_ratio->GetXaxis()->SetTitleSize(resHistos1D_1_def->GetXaxis()->GetTitleSize()*(0.7/0.3));
            resHistos1D_1_ratio->GetYaxis()->SetLabelSize(resHistos1D_1_def->GetYaxis()->GetLabelSize()*(0.7/0.3));
            resHistos1D_1_ratio->GetXaxis()->SetLabelSize(resHistos1D_1_def->GetXaxis()->GetLabelSize()*(0.7/0.3));
            resHistos1D_1_ratio->GetXaxis()->SetTitleOffset(2.4/(0.7/0.3));
            resHistos1D_1_ratio->GetYaxis()->SetTitleOffset(1.3/(0.7/0.3));
    
            TPad* p1 = new TPad("p1", "p1", 0.0, 0.30, 1.0, 1.0);
            p1->Draw(); p1->cd();
    
            p1->SetGridx(); p1->SetGridy();
            p1->SetMargin(lMarginRes_, rMargin_, b1Margin_, t1Margin_);
    
            resHistos1D_1_def->Draw("");
            resHistos1D_1_def->SetTitle("");
            resHistos1D_1_def->GetXaxis()->SetNdivisions(5,10,0);
            resHistos1D_1_def->GetXaxis()->SetTitle(xtitle.c_str());
            resHistos1D_1_def->GetXaxis()->SetTitleOffset(xOff_*resHistos1D_1_def->GetXaxis()->GetTitleOffset());
            resHistos1D_1_def->GetYaxis()->SetTitleOffset(yOff_*resHistos1D_1_def->GetYaxis()->GetTitleOffset());
    
            resHistos1D_1_def->GetYaxis()->SetTitle(ySclTitle_.c_str());
            resHistos1D_1_def->GetYaxis()->SetRangeUser(yMinS_, yMaxS_);
            resHistos1D_1_def->SetMarkerSize(mSize_);
            resHistos1D_1_def->SetLineWidth(lWidth_);
            resHistos1D_1_def->SetMarkerStyle(24);
    
            resHistos1D_1_new_cond->Draw("same");
            resHistos1D_1_new_cond->SetLineColor(2);
            resHistos1D_1_new_cond->SetLineWidth(lWidth_);
            resHistos1D_1_new_cond->SetMarkerColor(2);
            resHistos1D_1_new_cond->SetMarkerSize(mSize_);
            resHistos1D_1_new_cond->SetMarkerStyle(20);
    
            TLegend* resLegend1 = new TLegend(0.63, 0.75, 0.83, 0.90);
            resLegend1->AddEntry(resHistos1D_1_def, "Default", "EP");
            resLegend1->AddEntry(resHistos1D_1_new_cond, "New", "EP");
            resLegend1->Draw("SAME");
    
            c1->Update(); c1->Modified();
    
            c1->cd();
            TPad* p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.3);
            p2->Draw(); p2->cd();
            p2->SetGridx(); p2->SetGridy();
            p2->SetMargin(lMarginRes_, rMargin_, b2Margin_, t2Margin_);
            resHistos1D_1_ratio->Draw("EP");
            c1->Update(); c1->Modified();
    
            c1->Print(Form("plots/%s_emu.pdf", (meanName+custom).c_str()));
    
            std::string newname2(resHistos1D_2_new_cond->GetName()); newname2 += "_ratio";
            TH1D* resHistos1D_2_ratio = (TH1D*)resHistos1D_2_new_cond->Clone(newname2.c_str());
            resHistos1D_2_ratio->Add(resHistos1D_2_def, -1.0);
            resHistos1D_2_ratio->Divide(resHistos1D_2_def);
    
            resHistos1D_2_ratio->SetTitle("");
            resHistos1D_2_ratio->SetMarkerSize(mSize_);
            resHistos1D_2_ratio->SetLineWidth(lWidth_);
            resHistos1D_2_ratio->SetMarkerColor(kBlue-2);
            resHistos1D_2_ratio->SetLineColor(kBlue-2);
            resHistos1D_2_ratio->GetYaxis()->SetTitle("#frac{New-Default}{Default}");
            resHistos1D_2_ratio->GetXaxis()->SetTitle(xtitle.c_str());
            resHistos1D_2_ratio->GetYaxis()->SetRangeUser(-0.8, 0.3);
            resHistos1D_2_ratio->GetYaxis()->SetNdivisions(3,5,0);
            resHistos1D_2_ratio->GetXaxis()->SetNdivisions(5,10,0);
            resHistos1D_2_ratio->GetYaxis()->SetTitleSize(resHistos1D_2_def->GetYaxis()->GetTitleSize()*(0.7/0.3));
            resHistos1D_2_ratio->GetXaxis()->SetTitleSize(resHistos1D_2_def->GetXaxis()->GetTitleSize()*(0.7/0.3));
            resHistos1D_2_ratio->GetYaxis()->SetLabelSize(resHistos1D_2_def->GetYaxis()->GetLabelSize()*(0.7/0.3));
            resHistos1D_2_ratio->GetXaxis()->SetLabelSize(resHistos1D_2_def->GetXaxis()->GetLabelSize()*(0.7/0.3));
            resHistos1D_2_ratio->GetXaxis()->SetTitleOffset(2.4/(0.7/0.3));
            resHistos1D_2_ratio->GetYaxis()->SetTitleOffset(1.3/(0.7/0.3));
    
            TCanvas* c2 = new TCanvas;
            c2->SetWindowSize(c2->GetWw(), 1.*c2->GetWh());
    
            TPad* p12 = new TPad("p12", "p12", 0.0, 0.3, 1.0, 1.0);
            p12->Draw(); p12->cd();
    
            p12->SetGridx(); p12->SetGridy();
            p12->SetMargin(lMarginRes_, rMargin_, b1Margin_, t1Margin_);
     
            resHistos1D_2_def->Draw("");
            resHistos1D_2_def->SetTitle("");
            resHistos1D_2_def->GetXaxis()->SetTitle(xtitle.c_str());
            resHistos1D_2_def->GetXaxis()->SetNdivisions(5,10,0);
            resHistos1D_2_def->GetXaxis()->SetTitleOffset(xOff_*resHistos1D_2_def->GetXaxis()->GetTitleOffset());
            resHistos1D_2_def->GetYaxis()->SetTitleOffset(yOff_*resHistos1D_2_def->GetYaxis()->GetTitleOffset());
    
            resHistos1D_2_def->GetYaxis()->SetTitle(yResTitle_.c_str());
            resHistos1D_2_def->GetYaxis()->SetRangeUser(yMinR_,yMaxR_);
            resHistos1D_2_def->SetMarkerSize(mSize_);
            resHistos1D_2_def->SetLineWidth(lWidth_);
            resHistos1D_2_def->SetMarkerStyle(24);
    
            resHistos1D_2_new_cond->Draw("same");
            resHistos1D_2_new_cond->SetLineColor(2);
            resHistos1D_2_new_cond->SetLineWidth(lWidth_);
            resHistos1D_2_new_cond->SetMarkerColor(2);
            resHistos1D_2_new_cond->SetMarkerSize(mSize_);
            resHistos1D_2_new_cond->SetMarkerStyle(20);
    
            TLegend* resLegend2 = new TLegend(0.33, 0.75, 0.53, 0.90);
            resLegend2->AddEntry(resHistos1D_2_def, "Default", "EP");
            resLegend2->AddEntry(resHistos1D_2_new_cond, "New", "EP");
            resLegend2->Draw("SAME");
            c2->Update(); c2->Modified();
    
            c2->cd();
            TPad* p22 = new TPad("p22", "p22", 0.0, 0.0, 1.0, 0.3);
            p22->Draw(); p22->cd();
            p22->SetGridx(); p22->SetGridy();
            p22->SetMargin(lMarginRes_, rMargin_, b2Margin_, t2Margin_);
            resHistos1D_2_ratio->Draw("EP");
            c2->Update(); c2->Modified();
    
            c2->Print(Form("plots/%s_emu.pdf", (sigmaName+custom).c_str()));
        }
    }  
    
    void plotEffs(std::vector<std::string>& types, std::string refName, int rebinF, std::string xtitle, std::string region = "") {
    
        refName += region;
    
        TH2F* deftemp = dynamic_cast<TH2F*>(files_.at(0)->Get(refName.c_str()));
        TH2F* newtemp = dynamic_cast<TH2F*>(files_.at(1)->Get(refName.c_str()));
        for (unsigned int irange = 0; irange < puRanges_.size(); irange++) {
    
            std::string custom = puRangeNames_[irange]; std::vector<int> bounds = puRanges_[irange];
    
            std::string newName = refName + custom;
            int lowBin = deftemp->GetYaxis()->FindBin(bounds[0]); int hiBin = deftemp->GetYaxis()->FindBin(bounds[1]);
    
            TH1D* refHists_def = deftemp->ProjectionX((newName+"_def").c_str(), lowBin, hiBin, ""); refHists_def->Rebin(rebinF);
            TH1D* refHists_new_cond = newtemp->ProjectionX((newName+"_new").c_str(), lowBin, hiBin, ""); refHists_new_cond->Rebin(rebinF);
    
            for (auto& type : types) {
                 TLegend* legend = new TLegend(0.71, 0.3, 0.91, 0.45);
    
                 std::string histName;
                 if (region == "Incl" or region == "") { histName = type; }
                 else { histName = std::string(type) + std::string("_") + std::string(region); }
    
                 std::string newName2 = histName + custom;
    
                 TH2F* deftemp2 = dynamic_cast<TH2F*>(files_.at(0)->Get(histName.c_str()));
                 TH2F* newtemp2 = dynamic_cast<TH2F*>(files_.at(1)->Get(histName.c_str()));
    
                 lowBin = deftemp->GetYaxis()->FindBin(bounds[0]); hiBin = deftemp->GetYaxis()->FindBin(bounds[1]);
    
                 TH1D* hists_def = deftemp2->ProjectionX((newName2+"_def").c_str(), lowBin, hiBin, ""); hists_def->Rebin(rebinF);
                 TH1D* hists_new_cond = newtemp2->ProjectionX((newName2+"_new").c_str(), lowBin, hiBin, ""); hists_new_cond->Rebin(rebinF);
    
                 TGraphAsymmErrors *Eff_def = new TGraphAsymmErrors();
                 TGraphAsymmErrors *Eff_new_cond = new TGraphAsymmErrors();
    
                 std::string newname(hists_new_cond->GetName()); newname += "_ratio";
                 std::string p1name(hists_new_cond->GetName()); p1name += "_p1";
                 std::string p2name(hists_new_cond->GetName()); p2name += "_p2";
    
                 TH1D *effHists_ratio = (TH1D*)hists_def->Clone((newname+"_ratio").c_str());
                 effHists_ratio->Reset();
    
                 Eff_def->BayesDivide(hists_def,refHists_def);
                 Eff_new_cond->BayesDivide(hists_new_cond,refHists_new_cond);
    
                 double iDErr = 0.0; double iNErr = 0.0;
                 Double_t xD; Double_t yD; Double_t xN; Double_t yN;
                 for (int i = 0; i < Eff_def->GetN(); i++) {
    
                     Eff_def->GetPoint(i, xD, yD); Eff_new_cond->GetPoint(i, xN, yN);
    
                     iDErr = (Eff_def->GetErrorYhigh(i) > Eff_def->GetErrorYlow(i)) ? Eff_def->GetErrorYhigh(i) : Eff_def->GetErrorYlow(i);
                     iNErr = (Eff_new_cond->GetErrorYhigh(i) > Eff_new_cond->GetErrorYlow(i)) ? Eff_new_cond->GetErrorYhigh(i) : Eff_new_cond->GetErrorYlow(i);
    
                     double sumErr = pow(iDErr*iDErr + iNErr*iNErr, 0.5);
                     double sum = yN-yD;
    
                     int ibin = effHists_ratio->FindBin(xD);
    
                     if (yD > 0.0) {
                         effHists_ratio->SetBinContent(ibin, sum/yD);
                         effHists_ratio->SetBinError(ibin, pow((iDErr*iDErr*yN*yN + sumErr*sumErr*yD*yD) / pow(yD,4), 0.5));
                     } else {
                         effHists_ratio->SetBinContent(ibin, -999);
                         effHists_ratio->SetBinError(ibin, 0.0);
                     }
                 }
    
                 effHists_ratio->SetTitle("");
                 effHists_ratio->SetMarkerSize(mSize_);
                 effHists_ratio->SetLineWidth(lWidth_);
                 effHists_ratio->SetMarkerColor(kBlue-2);
                 effHists_ratio->SetLineColor(kBlue-2);
                 effHists_ratio->GetYaxis()->SetTitle("#frac{New-Default}{Default}");
                 effHists_ratio->GetXaxis()->SetTitle(xtitle.c_str());
                 effHists_ratio->GetYaxis()->SetRangeUser(-0.8, 0.30);
                 effHists_ratio->GetYaxis()->SetNdivisions(3,5,0);
                 //effHists_ratio->GetXaxis()->SetNdivisions(10,10,0);
                 effHists_ratio->GetYaxis()->SetTitleSize(hists_def->GetYaxis()->GetTitleSize()*(0.7/0.3));
                 effHists_ratio->GetXaxis()->SetTitleSize(hists_def->GetXaxis()->GetTitleSize()*(0.7/0.3));
                 effHists_ratio->GetYaxis()->SetLabelSize(hists_def->GetYaxis()->GetLabelSize()*(0.7/0.3));
                 effHists_ratio->GetXaxis()->SetLabelSize(hists_def->GetXaxis()->GetLabelSize()*(0.7/0.3));
                 effHists_ratio->GetXaxis()->SetTitleOffset(2.4/(0.7/0.3));
                 effHists_ratio->GetYaxis()->SetTitleOffset(1.3/(0.7/0.3));
                 effHists_ratio->GetXaxis()->SetRangeUser(xMinR_,xMaxR_);
    
                 TCanvas* c = new TCanvas;
                 c->SetWindowSize(c->GetWw(), 1.*c->GetWh());
                 c->cd();
                 TPad* p2 = new TPad(p2name.c_str(), p2name.c_str(), 0.0, 0.0, 1.0, 0.3);
                 p2->SetMargin(lMarginEff_, rMargin_, b2Margin_, t2Margin_);
                 p2->Draw();
                 p2->cd();
                 p2->SetGridx(); p2->SetGridy();
                 effHists_ratio->Draw("EP");
                 p2->Update(); p2->Modified();
    
                 c->cd(0);
                 TPad* p1 = new TPad(p1name.c_str(), p1name.c_str(), 0.0, 0.30, 1.0, 1.0);
                 p1->SetMargin(lMarginEff_, rMargin_, b1Margin_, t1Margin_);
                 p1->Draw(); p1->cd();
                 p1->SetGridx(); p1->SetGridy();

                 TH1D* dummy = new TH1D("h1", "h1", effHists_ratio->GetNbinsX(), xMinR_, xMaxR_);
                 dummy->SetMinimum(yMinR_);
                 dummy->SetMaximum(yMaxR_);
                 dummy->GetXaxis()->SetLimits(xMinR_,xMaxR_);

                 Eff_def->SetTitle("");
                 Eff_def->GetXaxis()->SetLabelSize(0.0);
                 Eff_def->GetXaxis()->SetTitleOffset(1.17*hists_def->GetXaxis()->GetTitleOffset());
                 Eff_def->GetYaxis()->SetTitle("Efficiency");
               
                 Eff_def->SetMarkerColor(kBlack);
                 Eff_def->SetMarkerStyle(24);
                 Eff_new_cond->SetMarkerColor(kRed);
    
                 Eff_def->SetLineColor(kBlack);
                 Eff_new_cond->SetLineColor(kRed);
    
                 Eff_def->SetMarkerSize(0.9*mSize_);
                 Eff_new_cond->SetMarkerSize(mSize_);
    
                 Eff_def->SetLineWidth(lWidth_);
                 Eff_new_cond->SetLineWidth(lWidth_);
    
                 legend->AddEntry(Eff_def, "Default", "EP");
                 legend->AddEntry(Eff_new_cond, "New", "EP");
    
                 legend->SetTextSize(0.035);
    
                 dummy->Draw();
                 Eff_def->Draw("AP");
                 Eff_new_cond->Draw("P");
                 Eff_def->GetXaxis()->SetLimits(xMinR_,xMaxR_);
                 Eff_new_cond->GetXaxis()->SetLimits(xMinR_,xMaxR_);

                 Eff_def->SetMinimum(yMinR_);
                 Eff_new_cond->SetMaximum(yMaxR_);

                 Eff_def->SetMinimum(yMinR_);
                 Eff_new_cond->SetMaximum(yMaxR_);

                 legend->Draw("SAME");
                 p1->Update(); p1->Modified();
    
                 c->cd(0);
                 if (region == "") {
                    c->Print(Form("plots/%sEffs_emu%s_lin.pdf", type.c_str(), custom.c_str()));
                    c->SetLogx();
                    c->Print(Form("plots/%sEffs_emu%s_log.pdf", type.c_str(), custom.c_str()));
                 } else {
                    c->Print(Form("plots/%sEffs_emu_%s%s_lin.pdf", type.c_str(), region.c_str(), custom.c_str()));
                    c->SetLogx();
                    c->Print(Form("plots/%sEffs_emu_%s%s_log.pdf", type.c_str(), region.c_str(), custom.c_str()));
                }
    
                delete deftemp2;
                delete newtemp2;
                delete Eff_def;
                delete Eff_new_cond;
                delete effHists_ratio;
                delete c;
                delete dummy;
            }
        }
    }

private:

   double mSize_ = 0.8; double lWidth_ = 2.; double lMarginEff_ = 0.17; double lMarginRes_ = 0.19; double xOff_ = 1.14; double yOff_ = 1.40;
   double rMargin_ = 0.05; double b1Margin_ = 0.00; double b2Margin_ = 0.35; double t1Margin_ = 0.05; double t2Margin_ = 0.00;
   double yMinS_ = -0.9; double yMaxS_ = 0.9;
   double yMinR_ = -0.10; double yMaxR_ = 1.05;
   double xMinR_ = 0.0; double xMaxR_ = 500.0;
   std::string yResTitle_ = "#sigma#left(#frac{online - offline}{offline}#right)"; std::string ySclTitle_ = "#mu#left(#frac{online - offline}{offline}#right)";

   // default, then new conditions
   std::vector<std::string> filenames_ = {"l1analysis_def.root", "l1analysis_new_cond.root"};
   std::vector<std::string> l1Types_ = {"singleJet", "doubleJet", "tripleJet", "quadJet",
     				"htSum", "etSum", "metSum", "metHFSum"};
   
   std::vector<std::string> puRangeNames_ = {"_lowPU", "_midPU", "_highPU", ""};

   std::vector<std::vector<int> > puRanges_ = {{0,29}, {30,59}, {60,90}, {0,90}};
     
   std::vector<TFile*> files_;

};

int main() {

    // include comparisons between HW and data TPs
    //bool includeHW = false;

    setTDRStyle();
    gROOT->ForceStyle();
    
    // Make the plotter object to run the show 
    l1Plotter thePlotter = l1Plotter();

    std::string pfMetXTitle = "pfMET [GeV]"; std::string caloMetXTitle = "Calo MET [GeV]"; std::string jetXTitle = "Offline Jet E_{T} [GeV]";

    //-----------------------------------------------------------------------
    // L1 Jet efficiencies
    //-----------------------------------------------------------------------

    gStyle->SetEndErrorSize(0.0);

    std::map<std::string, std::vector<std::string> > jetTypes = 
    {{"Incl", {"JetEt12","JetEt35","JetEt60","JetEt90","JetEt120","JetEt180"}},
    {"HB",   {"JetEt12","JetEt35","JetEt60","JetEt90","JetEt120","JetEt180"}},
    {"HE1",  {"JetEt12","JetEt35","JetEt60","JetEt90","JetEt120","JetEt180"}},
    {"HE2",  {"JetEt12","JetEt35","JetEt60","JetEt90","JetEt120","JetEt180"}},
    {"HE",   {"JetEt12","JetEt35","JetEt60","JetEt90","JetEt120","JetEt180"}}};

    std::vector<std::string> sumTypesPF = {"MET50_PF","MET100_PF","MET120_PF","MET150_PF"};
    std::vector<std::string> sumTypesCalo = {"MET50_Calo","MET100_Calo","MET120_Calo","MET150_Calo"};

    for (auto& [region, histos] : jetTypes) { thePlotter.plotEffs(histos, "RefmJet_", 5, jetXTitle, region); }

    //-----------------------------------------------------------------------
    // L1 ETM efficiencies
    //-----------------------------------------------------------------------

    thePlotter.plotEffs(sumTypesPF, "RefMET_PF", 10, pfMetXTitle);

    thePlotter.plotEffs(sumTypesCalo, "RefMET_Calo", 10, caloMetXTitle);

     //-----------------------------------------------------------------------
     // L1 Jet resolution summary plots
     //-----------------------------------------------------------------------
    
    
    // // Jet resolution
    std::vector<std::string> regions = {"Incl", "HE", "HB", "HE2", "HE1"}; 


    for (auto& region : regions) { thePlotter.plotRes("hresJet_", "Jet", jetXTitle, region); }

    //-----------------------------------------------------------------------
    // L1 ETM resolution summary plots
    //-----------------------------------------------------------------------
    
    thePlotter.plotRes("hResMET_Calo", "MET", caloMetXTitle);
    thePlotter.plotRes("hResMET_PF", "MET", pfMetXTitle);

    return 0;
}
