#define Mpion 0.13957
#define Mkaon 0.493677
#define Mproton 0.93827231
#define Melectron 0.00051099907
#define PI 3.1415927

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include "sys/types.h"
#include "dirent.h"
#include <ctime>


#include "math.h"
#include "string.h"
#include <iomanip>

#ifndef __CINT__  
#include "TROOT.h"
#include "TFile.h"
#include "TChain.h"
#include "TMath.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TF1.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TProfile.h"
#include "TVector2.h"
#include "TTree.h"
#include "TNtuple.h"
#include "TRandom.h"
#include "TRandom3.h"
#include "TUnixSystem.h"
#include "TVector3.h"
//#include "StRefMultCorr.h"
//#include "CentralityMaker.h"

#include "mEvent.h"
#include "mBadRuns_Run18_27GeV.h"
#include "cuts.h"
using namespace std;
using std::cout;
using std::endl;
using std::setw;
//class StRefMultCorr;
//class CentralityMaker;
#endif
//StRefMultCorr* refmultCorrUtil;
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void     init(int mRoundIndex);
Int_t    getVzBinIdx(Double_t vz);
Int_t    getRunPeriodBinIdx(Int_t run);
Double_t getCorr_Lum(Int_t irunPd, Double_t zdcx);
Double_t getCorr_Vz(Int_t irunPd, Double_t Vz);
Double_t getShapeWeight(Int_t irunPd, Int_t ivz, Double_t refmult_corred);
Double_t getRefmultReWeight(Double_t refmult_corred);
bool     passEvent(mEvent* event, int mRoundIndex);
Bool_t   passnTofMatchRefmultCut(Double_t ntofmatch, Double_t refmult);
void     bookHistograms(char *outFile, int mRoundIndex);
void     writeHistograms();
void     clear();
//---------------------------------------------------------------------
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static const int nModes  = 6;
static const int nVzBins = 14;
static const int nRunPds = 6;
TFile   *outfile;
TH1F    *h_EvtCounter;
TH2F    *hnTofMatchvsRefmult;
TH2F    *hnTofMatchvsRefmult1;
TH1F    *hTPCVz_perRunPd[nRunPds]; 
TString mInFileName1;
TFile *inf1;
TF1   *f1_corr_lum[nRunPds];
TF1   *f1_corr_vz[nRunPds];
TF1   *f1_reweight;  
TH1F  *h_ShapeRatio[6][14];
TH2F  *hRefmultVsZdcX[nModes][nVzBins][nRunPds];
TH2F  *hRefmultVsZdcX_AllVz[nModes][nRunPds];
TH2F  *htem_RefmultVsVzCorr;
TH2F  *htem_VzVsVzCorr;
//TH2F *hRefMultvsRunIdx_tem[nVzBins];

TH1F* hcent9;
TH1F* hcent16;
TH1F* hcent9_aftwt;
TH1F* hcent16_aftwt;


//-------------------------------------
int mRoundIndex;

//---------------------------------------------------------------------
//---------------------------------------------------------------------

int main(int argc, char** argv)
{

	if(argc!=1&&argc!=4) return -1;

	char *inFile="test.list";
	char outFile[1024];
	sprintf(outFile,"test");
	
	if(argc==4)
	{
		inFile = argv[1];
		sprintf(outFile,"%s",argv[2]);
		mRoundIndex = atoi(argv[3]);
	}
	else
	{
		cout<<"wrong input, please run like ./refmultCorr test.list testout roundindex"<<endl;
		return 0;
	}

	//+---------------------------------+
	//| open files and add to the chain |
	//+---------------------------------+
	TChain *chain = new TChain("mEvent");
	Int_t ifile=0;
	char filename[512];
	ifstream *inputStream = new ifstream;
	inputStream->open(inFile);
	
	if (!(inputStream)) 
	{
		printf("can not open list file\n");
		return 0;
	}
	
	for(;inputStream->good();)
	{
		inputStream->getline(filename,512);
		if(inputStream->good()) 
		{
			TFile *ftmp = new TFile(filename);
			if(!ftmp||!(ftmp->IsOpen())||!(ftmp->GetNkeys())) 
			{
				cout<<"something wrong"<<endl;
			} 
			else 
			{
				cout<<"read in "<<ifile<<"th file: "<<filename<<endl;
				chain->Add(filename);
				ifile++;
			}
			delete ftmp;
		}
	}
	delete inputStream;

	bookHistograms(outFile, mRoundIndex);
	init( mRoundIndex );

	mEvent *mevent = new mEvent(chain);
	Int_t nEvents = chain->GetEntries();

	cout<<" nEvents = "<<nEvents<<endl;

	//BM = new TF1("BM","1.073e-4/pow(x,2.26)+0.997",0.2,10.);

	//+-------------+
	//| loop events |
	//+-------------+
	for(int i=0; i<nEvents; i++)
	{
		if( i%100000==0 )
		{
			cout << "begin " << i << "th entry...." << endl;
		}
		
		mevent->GetEntry(i);
		
		passEvent(mevent, mRoundIndex);
		
	} //end of event loop

	writeHistograms(); 
	//clear();
	delete chain;
	cout<<"end of program"<<endl;
	return 0;
}
//_____________________________________________________________________
bool passEvent(mEvent* event, int mRoundIndex)
{
	h_EvtCounter->Fill(0);
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	Int_t runid = (int) event->mrunId;
	const int mNBadRuns = sizeof(mBadRuns)/sizeof(int);
	for(int ir=0;ir<mNBadRuns;ir++){ if(runid == mBadRuns[ir]) return kFALSE; } // reject bad runs
	h_EvtCounter->Fill(1);
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	Double_t Vx    = event->mvx;
	Double_t Vy    = event->mvy;
	Double_t Vz    = event->mvz;
	Double_t Vr    = sqrt(Vx*Vx+Vy*Vy);
	
	Int_t   imVzBin    = getVzBinIdx(Vz);
	Int_t   imRunPdBin = getRunPeriodBinIdx(runid);
	
	hTPCVz_perRunPd[imRunPdBin]->Fill(Vz);

	Double_t Refmult   = 1.*event->mrefmult;
	Double_t Ntofmatch = 1.*event->mnTofMatch;
	Double_t zdcx      = 1.*event->mzdcX;

	//refmultCorrUtil = CentralityMaker::instance()->getRefMultCorr() ;
	//refmultCorrUtil -> init(runid);
	//refmultCorrUtil -> initEvent( (UShort_t)(event->mrefmult), (Double_t)Vz, (Double_t)zdcx) ;

	//int cent16 = refmultCorrUtil->getCentralityBin16() ;
	//int cent9  = refmultCorrUtil->getCentralityBin9() ;

	//double refmultcor = refmultCorrUtil->getRefMultCorr() ;
	//double reweight   = refmultCorrUtil->getWeight() ;
	//
	//cout<<"------------------"<<endl;
	//cout<<"Vz: "<<Vz<<endl;
	//cout<<"refmult: "<<Refmult<<endl;
	//cout<<"refmultcor: "<<refmultcor<<endl;
	//cout<<std::setprecision(5)<<"reweight: "<<reweight<<endl;
	//cout<<"------------------"<<endl;
	
	//------------------------------------------------------------------------------------------------------
	hnTofMatchvsRefmult ->Fill(Refmult, Ntofmatch);
	if( !passnTofMatchRefmultCut(Ntofmatch,Refmult) ) return kFALSE;
	h_EvtCounter->Fill(2);
	hnTofMatchvsRefmult1 ->Fill(Refmult, Ntofmatch);
	//------------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------------
	if(TMath::Abs(Vx)<1e-5&&TMath::Abs(Vy)<1e-5&&TMath::Abs(Vz)<1e-5) return kFALSE;
	h_EvtCounter->Fill(3);
	
	if(Vz<mVzCut[0] || Vz>mVzCut[1]) return kFALSE;
	h_EvtCounter->Fill(4);
	
	if(Vr>mVrCut) return kFALSE;
	h_EvtCounter->Fill(5);

	//------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------
	Refmult += gRandom->Rndm()-0.5; // this will remove the spike RefMult resulted from Int type numbers
	//------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------------
	hRefmultVsZdcX_AllVz[0][imRunPdBin]    -> Fill(zdcx, Refmult); //no correction
	hRefmultVsZdcX[0][imVzBin][imRunPdBin] -> Fill(zdcx, Refmult); //no correction
	//------------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------
	if(mRoundIndex>=1)//correct luminosity zdcx
	{
		Double_t lumi_corr = getCorr_Lum(imRunPdBin, zdcx);
		hRefmultVsZdcX_AllVz[1][imRunPdBin]    -> Fill(zdcx, (1.*Refmult)*lumi_corr); // apply lumi correction
		hRefmultVsZdcX[1][imVzBin][imRunPdBin] -> Fill(zdcx, (1.*Refmult)*lumi_corr); // apply lumi correction

		if(mRoundIndex>=2)//correct luminosity (zdcx) and Vz
		{
			Double_t vz_corr      = getCorr_Vz(imRunPdBin, Vz);
			Double_t corr_LumiVz  = vz_corr*lumi_corr;
			Double_t refmult_corr = (1.0*Refmult)*corr_LumiVz;

			//	if(imVzBin==1&&imRunPdBin==4)
			//	{
			//		//if( fabs(vz_corr-1.0305)>0.003 )
			//		//{
			//		//	cout<<"--------------------"<<endl;
			//		//	cout<<"--------------------"<<endl;
			//		//	cout<<std::setprecision(5)<<"Refmult: "<<Refmult<<endl;
			//		//	cout<<std::setprecision(5)<<"lumi_corr: "<<lumi_corr<<endl;
			//		//	cout<<std::setprecision(5)<<"Vz: "<<Vz<<endl;
			//		//	cout<<std::setprecision(10)<<"vz_corr: "<<vz_corr<<endl;
			//		//	cout<<std::setprecision(5)<<"corr_LumiVz: "<<corr_LumiVz<<endl;
			//		//	cout<<std::setprecision(5)<<"refmult_corr: "<<refmult_corr<<endl;
			//		//	cout<<"--------------------"<<endl;
			//		//	cout<<"--------------------"<<endl;
			//		//}
			//	}

			htem_RefmultVsVzCorr->Fill(Refmult, vz_corr);
			htem_VzVsVzCorr->Fill(Vz, vz_corr);
			
			hRefmultVsZdcX_AllVz[2][imRunPdBin]    -> Fill(zdcx, refmult_corr); // apply lumi correction and vz correction
			hRefmultVsZdcX[2][imVzBin][imRunPdBin] -> Fill(zdcx, refmult_corr); // apply lumi correction and vz correction
		
			//hRefMultvsRunIdx_tem[imVzBin]    ->Fill( runidx, refmult_corr );

			if(mRoundIndex>=3) // apply lumi correction and vz correction and weight the shape to center refmult shape
			{
				Double_t shape_weight = getShapeWeight(imRunPdBin, imVzBin, refmult_corr);
			
				hRefmultVsZdcX_AllVz[3][imRunPdBin]    -> Fill(zdcx, refmult_corr, shape_weight); 
				hRefmultVsZdcX[3][imVzBin][imRunPdBin] -> Fill(zdcx, refmult_corr, shape_weight); 
				
				if(mRoundIndex>=4)
				{
					Double_t refmult_reweight = getRefmultReWeight(refmult_corr);

					if(refmult_corr>=3) //apply a cut on refmultcorr
					{
						hRefmultVsZdcX_AllVz[4][imRunPdBin]    -> Fill(zdcx, refmult_corr, shape_weight*refmult_reweight); 
						hRefmultVsZdcX[4][imVzBin][imRunPdBin] -> Fill(zdcx, refmult_corr, shape_weight*refmult_reweight); 
					}
					
					if(mRoundIndex==5) //finally check centrality, refmultCorr_afterweight, for |Vz|<70 and subVz 
					{
						Float_t refCor    =   event->mrefmultCorr;
						Float_t rewit     =   event->mreweight;
						Int_t   cent9Idx  =   event->mcent9;
						Int_t   cent16Idx =   event->mcent16;

						//cout<<"refCor: "<<refCor<<endl;
						//cout<<"rewit: "<<rewit<<endl;
						//cout<<"cent9Idx: "<<cent9Idx<<endl;
						//cout<<"cent16Idx: "<<cent16Idx<<endl;

						if(refCor>=6.0)
						{
							hcent9       ->Fill(cent9Idx);
							hcent16      ->Fill(cent16Idx);
							hcent9_aftwt ->Fill(cent9Idx,  rewit);
							hcent16_aftwt->Fill(cent16Idx, rewit);
							hRefmultVsZdcX_AllVz[5][imRunPdBin]    -> Fill(zdcx, refCor, rewit);
							hRefmultVsZdcX[5][imVzBin][imRunPdBin] -> Fill(zdcx, refCor, rewit);
						}
					}
				}
			}
		}
	}

	return kTRUE;
}

//=======================================================================================
Bool_t passnTofMatchRefmultCut(Double_t ntofmatch, Double_t refmult)
{

	const Double_t min = 4.0;
	const Double_t max = 5.0;

	if(ntofmatch<=2)  return false;

	const double a0 = -0.704787625248525;
	const double a1 = 0.99026234637141;
	const double a2 = -0.000680713101607504;
	const double a3 = 2.77035215460421e-06;
	const double a4 = -4.04096185674966e-09;
	const double b0 = 2.52126730672253;
	const double b1 = 0.128066911940844;
	const double b2 = -0.000538959206681944;
	const double b3 = 1.21531743671716e-06;
	const double b4 = -1.01886685404478e-09;
	const double c0 = 4.79427731664144;
	const double c1 = 0.187601372159186;
	const double c2 = -0.000849856673886957;
	const double c3 = 1.9359155975421e-06;
	const double c4 = -1.61214724626684e-09;

	double refmultCenter = a0+a1*(ntofmatch)+a2*pow(ntofmatch,2)+a3*pow(ntofmatch,3)+a4*pow(ntofmatch,4);
	double refmultLower  = b0+b1*(ntofmatch)+b2*pow(ntofmatch,2)+b3*pow(ntofmatch,3)+b4*pow(ntofmatch,4);
	double refmultUpper  = c0+c1*(ntofmatch)+c2*pow(ntofmatch,2)+c3*pow(ntofmatch,3)+c4*pow(ntofmatch,4);
	
	double refmultCutMin = refmultCenter - min*refmultLower;
	double refmultCutMax = refmultCenter + max*refmultUpper;

	if( refmult>refmultCutMax || refmult<refmultCutMin ) return false;

	return true;
}

//=======================================================================================
void init( int mRoundIndex)
{
	if(mRoundIndex<4)
	{
		mInFileName1  = Form("./inputfiles/out_CorrParms_Refmult_Run18_27GeV_forRound%d.root", mRoundIndex);
	}
	else
	{
		mInFileName1  = "./inputfiles/out_CorrParms_Refmult_Run18_27GeV_forRound3.root";
	}
	//----------------------------------------------------------------------------- 
	TH1F* H_CorParms_lum[nRunPds];
	TH1F* H_CorParms_vz[nRunPds];
	
	if(mRoundIndex>0)
	{
		inf1 = new TFile(mInFileName1, "read");
		cout<<"================================"<<endl;
		cout<<"readin: "<<inf1->GetName()<<endl;
		cout<<"================================"<<endl;
		for(int irunpd=0; irunpd<nRunPds; irunpd++)
		{
			if(mRoundIndex>=1)
			{
				H_CorParms_lum[irunpd] = (TH1F*)inf1->Get( Form("H_CorrParms_Lumi_irunpd%d",irunpd) );

				//always use 1st bin, which is from all Vz
				Float_t a = H_CorParms_lum[irunpd]->GetBinContent(1); 
				Float_t b = H_CorParms_lum[irunpd]->GetBinError(  1);

				f1_corr_lum[irunpd] = new TF1(Form("f1_corr_lum_irunpd%d",irunpd), "[0]*x+[1]", 0., 100.);
				f1_corr_lum[irunpd] -> FixParameter(0, a);
				f1_corr_lum[irunpd] -> FixParameter(1, b);
				f1_corr_lum[irunpd] -> SetNpx(5000);
				
				delete H_CorParms_lum[irunpd];
			
				//----------------------------------------------------------------------------- 
				//----------------------------------------------------------------------------- 
				if(mRoundIndex>=2) 
				{
					//----------------------------------------------------------------------------- 
					//----------------------------------------------------------------------------- 
					H_CorParms_vz[irunpd]  = (TH1F*)inf1->Get( Form("H_CorrParms_Vz_irunpd%d",  irunpd) );
					f1_corr_vz[irunpd]     = new TF1(Form("f1_corr_vz_irunpd%d",irunpd), "pol6", -100.,100.);

					for(int ip=0; ip<7; ip++)
					{
						f1_corr_vz[irunpd]-> FixParameter( ip, H_CorParms_vz[irunpd]->GetBinContent(ip+1) );
					}
					
					f1_corr_vz[irunpd] -> SetNpx(5000);

					cout<<"------------------------------------------------"<<endl;
					for(int ip=0; ip<7; ip++)
					{
						cout<<std::setprecision(15)
						<<"irunpd: "<<irunpd<<" "
						<<"ip: "<<ip<<"  f1_corr_vz[irunpd]->GetParameter[ip]: "<<f1_corr_vz[irunpd]->GetParameter(ip)<<endl;
					}
					cout<<"------------------------------------------------"<<endl;

					delete H_CorParms_vz[irunpd];
					//----------------------------------------------------------------------------- 
					//----------------------------------------------------------------------------- 
					if(mRoundIndex>=3)
					{
						for(int ivz=0; ivz<nVzBins; ivz++)
						{
							TH1F* htem  = (TH1F*)inf1->Get( Form("HrefRatio_SubVz2Center_irunpd%d_ivz%d", irunpd, ivz) );
							h_ShapeRatio[irunpd][ivz] = (TH1F*)htem->Clone( Form("h_ShapeRatio_irunpd%d_ivz%d",irunpd, ivz) );
							h_ShapeRatio[irunpd][ivz] ->SetName(Form("h_ShapeRatio_irunpd%d_ivz%d", irunpd, ivz));
							cout<<"h_ShapeRatio[irunpd][ivz]->GetBinContent(10): "<<h_ShapeRatio[irunpd][ivz]->GetBinContent(10)<<endl;
						}

						if(mRoundIndex>=4)
						{
							TFile *inf_rwt      = new TFile("./inputfiles/out_RefmultReweightRatio_Run18_27GeV_forRound4.root", "read");
							TH1F* htem_parm_rwt = (TH1F*)inf_rwt->Get("Hparm_Ratio_for_RemultReweight");
							
							f1_reweight = new TF1("f1_reweight", "[0] + [1]/([2]*x+[3]) + [4]*([2]*x+[3]) + [6]/([2]*x+[3])^2 + [7]*([2]*x+[3])^2", 0, 100.);
							//f1_reweight->SetParameters(0.929801,20.3072,10.3554,-22.1373,0.000110748,0.0,0.00761714,-8.11779e-08);

							for(int iw=0; iw<8; iw++)
							{
								f1_reweight->FixParameter(iw, htem_parm_rwt->GetBinContent(iw+1));
								cout<<"f1_reweight->GetParameter(iw): "<<f1_reweight->GetParameter(iw)<<endl;
							}
							//delete inf_rwt;
							//delete htem_parm_rwt;
						}
					}
					//----------------------------------------------------------------------------- 
					//----------------------------------------------------------------------------- 
				}
			}
		}
		//delete inf1;
	}	
	//----------------------------------------------------------------------------- 
	return;
}
//=======================================================================================
void bookHistograms(char *outFile, int mRoundIndex)
{
	char buf[1024];
	sprintf(buf,"%s_round%d.histo.root", outFile, mRoundIndex);
	outfile = new TFile(buf,"recreate");
	outfile->cd();

	TH1::SetDefaultSumw2();
	h_EvtCounter = new TH1F("h_EvtCounter", " ", 21, -0.5, 20.5);
	hnTofMatchvsRefmult  = new TH2F("hnTofMatchvsRefmult",  "hnTofMatchvsRefmult; RefMult; nTofMatch",        500,0,500, 500,0,500);
	hnTofMatchvsRefmult1 = new TH2F("hnTofMatchvsRefmult1", "hnTofMatchvsRefmult_aftCut; RefMult; nTofMatch", 500,0,500, 500,0,500);

	for(int irun=0; irun<nRunPds; irun++)
	{
		hTPCVz_perRunPd[irun] = new TH1F(Form("hTPCVz_perRunPd_%d",irun), "hTPCVz_perRunPd;TPC Vz (cm)", 400,-200,200);
	}

	for(int im=0; im<nModes; im++)//modes==rounds
	{
		for(int irun=0; irun<nRunPds; irun++)
		{
			hRefmultVsZdcX_AllVz[im][irun] 
				= new TH2F(Form("hRefmultVsZdcX_im%d_AllVz_irun%d",im,irun), Form("hRefmultVsZdcX_im%d_AllVz_irun%d;zdcRate(kHz);Refmult;",im,irun), 500, 0, 10., 500,0.,500.);
			for(int ivz=0; ivz<nVzBins; ivz++)
			{
				hRefmultVsZdcX[im][ivz][irun] 
					= new TH2F(Form("hRefmultVsZdcX_im%d_ivz%d_irun%d",im,ivz,irun), Form("hRefmultVsZdcX_im%d_ivz%d_irun%d;zdcRate(kHz);Refmult;",im,ivz,irun), 500, 0., 10., 500,0.,500.);
			}//vz
		}//run
	}//mode

	htem_RefmultVsVzCorr = new TH2F("htem_RefmultVsVzCorr", "htem_RefmultVsVzCorr;refmult;vzCor;", 500, 0.0,  500., 500,1.020, 1.035);
	htem_VzVsVzCorr      = new TH2F("htem_VzVsVzCorr",      "htem_VzVsVzCorr;Vz;vzCor;",           500, -70., 70.,  500,1.020, 1.035);
	//for(int ivz=0; ivz<nVzBins; ivz++)
	//{
		//hRefMultvsRunIdx_tem[ivz]  = new TH2F(Form("hRefMultvsRunIdx_im2_ivz%d_tem",ivz),  Form("hRefMultvsRunIdx_im2_ivz%d_tem; runIndex; refMult",ivz),  mMaxNRunIds,-0.5,mMaxNRunIds-0.5,500,0+1.e-9,500.+1.e-9);
	//}//vz
	
	
	//to check the StRefmultCorr class
	hcent9        = new TH1F("hcent9",        "hcent9       ;CentIndex;nEvents;", 11, -1, 10);
	hcent16       = new TH1F("hcent16",       "hcent16      ;CentIndex;nEvents;", 21, -1, 20);
	hcent9_aftwt  = new TH1F("hcent9_aftwt",  "hcent9_aftwt ;CentIndex;nEvents;", 11, -1, 10);
	hcent16_aftwt = new TH1F("hcent16_aftwt", "hcent16_aftwt;CentIndex;nEvents;", 21, -1, 20);

}
//=======================================================================================
void writeHistograms()
{
	cout<<"outputfile: "<<outfile->GetName()<<endl;
	outfile->Write();
	outfile->Close();
}
//=======================================================================================
void clear()
{
	delete inf1;
	delete h_EvtCounter;
	delete hnTofMatchvsRefmult;
	delete hnTofMatchvsRefmult1;
	
	//---------------------------------------------------------------------
	for(int irun=0; irun<nRunPds; irun++)
	{
		delete f1_corr_lum[irun];
		delete f1_corr_vz[irun];

		for(int ivz=0; ivz<nVzBins; ivz++) 
		{
			delete h_ShapeRatio[irun][ivz];
		}
	}
	
	//---------------------------------------------------------------------
	for(int im=0; im<nModes; im++)
	{
		for(int irun=0; irun<nRunPds; irun++)
		{
			delete hRefmultVsZdcX_AllVz[im][irun];

			for(int ivz=0; ivz<nVzBins; ivz++) 
			{
				delete hRefmultVsZdcX[im][ivz][irun];
			}
		}
	}
}
//---------------------------------------------------------------------
Int_t getVzBinIdx(Double_t vz)
{
	Int_t iBinVz = -1;
	if(     vz>=-70.&&vz<-60.) iBinVz = 0;
	else if(vz>=-60.&&vz<-50.) iBinVz = 1;
	else if(vz>=-50.&&vz<-40.) iBinVz = 2;
	else if(vz>=-40.&&vz<-30.) iBinVz = 3;
	else if(vz>=-30.&&vz<-20.) iBinVz = 4;
	else if(vz>=-20.&&vz<-10.) iBinVz = 5;
	else if(vz>=-10.&&vz<0.0 ) iBinVz = 6;
	else if(vz>=0.0 &&vz<10. ) iBinVz = 7;
	else if(vz>=10. &&vz<20. ) iBinVz = 8;
	else if(vz>=20. &&vz<30. ) iBinVz = 9;
	else if(vz>=30. &&vz<40. ) iBinVz = 10;
	else if(vz>=40. &&vz<50. ) iBinVz = 11;
	else if(vz>=50. &&vz<60. ) iBinVz = 12;
	else if(vz>=60. &&vz<=70.) iBinVz = 13;
	else iBinVz = -1;

	return iBinVz;
}
//---------------------------------------------------------------------
Int_t getRunPeriodBinIdx(Int_t run)
{
	Int_t iBinRunPeriod = -1;
	
	if(     run>=19130078&&run<=19131031) iBinRunPeriod = 0;
	else if(run>=19131037&&run<=19140025) iBinRunPeriod = 1;
	else if(run>=19140030&&run<=19143017) iBinRunPeriod = 2;
	else if(run>=19144012&&run<=19144033) iBinRunPeriod = 3;
	else if(run>=19144036&&run<=19145031) iBinRunPeriod = 4;
	else if(run>=19145034&&run<=19168040) iBinRunPeriod = 5;
	else iBinRunPeriod = -1;
	
	return iBinRunPeriod;
}
//---------------------------------------------------------------------
Double_t getCorr_Lum(Int_t irunPd, Double_t zdcx)
{
	Double_t b         = f1_corr_lum[irunPd]->GetParameter(1);
	Double_t corr_lumi = 1.0*b/(f1_corr_lum[irunPd]->Eval(zdcx));
	return corr_lumi;
}
//---------------------------------------------------------------------
Double_t getCorr_Vz(Int_t irunPd, Double_t Vz)
{
	Double_t hValueBaseLine = f1_corr_vz[5]->GetParameter(0);//use runpd5 as base line
	Double_t corr_vz = 1.0;

	if( fabs(corr_vz)>70. )
	{
		corr_vz = 1.0;
	}
	else
	{
		corr_vz = hValueBaseLine/(1.*f1_corr_vz[irunPd]->Eval(Vz));
	}

	return corr_vz;
}

//---------------------------------------------------------------------
Double_t getShapeWeight(Int_t irunPd, Int_t ivz, Double_t refmult_corred)
{
	if(refmult_corred<1.0||refmult_corred>=500.)
	{
		return 1.0;
	}
	else 
	{
		Double_t shapeWeight = 1.0;

		Int_t refmultBin  = h_ShapeRatio[irunPd][ivz]->GetXaxis()->FindBin(refmult_corred);
		if(refmultBin<1 || refmultBin>h_ShapeRatio[irunPd][ivz]->GetNbinsX()) return 1.0;
		
		Double_t tem_shapeRatio = h_ShapeRatio[irunPd][ivz]->GetBinContent(refmultBin);
		
		if(tem_shapeRatio<0.1)      shapeWeight = 1./0.1;
		else if(tem_shapeRatio>10.) shapeWeight = 1./10.;
		else                        shapeWeight = 1./tem_shapeRatio;
		
		return shapeWeight;
	}
	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Double_t getRefmultReWeight(Double_t refmult_corred)
{
	Double_t reweight = 1.0;

	if(refmult_corred>=3.&&refmult_corred<=60.)
	{
		reweight = f1_reweight->Eval(refmult_corred);
	}
	else
	{
		reweight = 1.0;
	}

	return reweight;
}

