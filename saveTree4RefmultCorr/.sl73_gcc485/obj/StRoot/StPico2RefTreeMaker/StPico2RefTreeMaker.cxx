#include "StPico2RefTreeMaker.h"
#include "StPicoDstMaker/StPicoDstMaker.h"
#include "StPicoEvent/StPicoDst.h"
#include "StPicoEvent/StPicoEvent.h"
#include "StPicoEvent/StPicoTrack.h"
#include "StPicoEvent/StPicoBTofHit.h"
#include "StPicoEvent/StPicoBTofPidTraits.h"
#include "StPicoEvent/StPicoArrays.h"
//#include "StPicoEvent/StPicoEmcTrigger.h"
//#include "StPicoEvent/StPicoMtdTrigger.h"
//#include "StPicoEvent/StPicoBbcTile.h"
//#include "StPicoEvent/StPicoEpdTile.h"
//#include "StPicoEvent/StPicoBTowHit.h"
//#include "StPicoEvent/StPicoMtdHit.h"
//#include "StPicoEvent/StPicoBEmcPidTraits.h"
//#include "StPicoEvent/StPicoMtdPidTraits.h"
#include "StThreeVectorF.hh"
#include "StEvent/StDcaGeometry.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"
#include "TROOT.h"
#include "TLorentzVector.h"
#include "TGeoManager.h"
#include <vector>

#include "TVector3.h"

#include <bitset>
#include "StPhysicalHelixD.hh"
#include "StarClassLibrary/SystemOfUnits.h"
#include "StarClassLibrary/PhysicalConstants.h"
#include "StBTofUtil/tofPathLength.hh"
#include "StBichsel/Bichsel.h"
#include "StMessMgr.h"

ClassImp(StPico2RefTreeMaker)
	//-----------------------------------------------------------------------------
	StPico2RefTreeMaker::StPico2RefTreeMaker(const char* name, StPicoDstMaker *picoMaker, const char* outName)
: StMaker(name)
{
	mPicoDstMaker = picoMaker;
	mPicoDst      = 0;
	mPicoEvent    = 0;
	mPicoTrack    = 0;
	mOutName      = outName;
	mOutName_hist = outName;

	mMaxNRunIds = 900;
	mRunList    = "./runnumber_run18_27GeV_all.list";

	runidx = -1;
}
//----------------------------------------------------------------------------- 
StPico2RefTreeMaker::~StPico2RefTreeMaker()
{ 
	/*  */ 
}

//----------------------------------------------------------------------------- 
Int_t StPico2RefTreeMaker::Init() 
{
	LoadRunList();
	
	if(mOutName_hist!="") 
	{
		mOutFile_hist = new TFile(mOutName_hist.Data(), "RECREATE");
	}
	else
	{
		mOutFile_hist = new TFile("test.root", "RECREATE");
	}
	
	mOutFile_hist->cd();
	DeclareHistograms();
	BookTree();
	return kStOK;
}
//----------------------------------------------------------------------------- 
Int_t StPico2RefTreeMaker::InitRun( const Int_t runNumber )
{
	return kStOK;
}
//----------------------------------------------------------------------------- 
Int_t StPico2RefTreeMaker::LoadRunList( )
{
	ifstream indata;
	indata.open( mRunList.Data() );
	mTotalRunId.clear();
	
	if( indata.is_open() )
	{
		cout << "read in total run number list and map it. runlist = "<<mRunList.Data()<<endl;
		Int_t oldId;
		Int_t newId=0;
		while(indata>>oldId)
		{
			mTotalRunId[oldId] = newId;
			newId++;
		}
		cout<<"[ok]"<<endl;
	}
	else
	{
		cout << "failed to load the runnumber list " << mRunList.Data() <<"!!!"<<endl;
		return kStErr;
	}

	indata.close();

	//for(map<Int_t,Int_t>::iterator iter=mTotalRunId.begin();iter!=mTotalRunId.end();iter++)
	//	cout<<iter->second<<" \t"<<iter->first<<endl;
	
	cout<<"Run Number list: "<<mRunList<<" was loaded"<<endl;
	cout<<"--------------------------------------------"<<endl;
	return kStOK;
}
//----------------------------------------------------------------------------- 
Int_t StPico2RefTreeMaker::Finish() 
{
	mOutFile_hist->Write();
	mOutFile_hist->Close();
	return kStOK;
}
//-----------------------------------------------------------------------------
void StPico2RefTreeMaker::DeclareHistograms() 
{
	TH1::SetDefaultSumw2();
	h_EvtCounter = new TH1F("h_EvtCounter", " ", 21, -0.5, 20.5);
}

//-----------------------------------------------------------------------------
void StPico2RefTreeMaker::BookTree()
{
	anaTree = new TTree("mEvent","mEvent");
    anaTree->SetAutoSave(100000);
    anaTree->Branch("mtrigWord", &mtrigWord,   "mtrigWord/I");
    anaTree->Branch("mfield",    &mfield,      "mfield/F");
    anaTree->Branch("mzdcX",     &mzdcX,       "mzdcX/F");
    anaTree->Branch("mbbcX",     &mbbcX,       "mbbcX/F");
    anaTree->Branch("mrunId",    &mrunId,      "mrunId/I");
    anaTree->Branch("mrefmult",  &mrefmult,    "mrefmult/I");
    anaTree->Branch("mnTofMatch",&mnTofMatch,  "mnTofMatch/I");
    anaTree->Branch("mvx",       &mvx,         "mvx/F");
    anaTree->Branch("mvy",       &mvy,         "mvy/F");
    anaTree->Branch("mvz",       &mvz,         "mvz/F");
    anaTree->Branch("mvpdVz",    &mvpdVz,      "mvpdVz/F");
}
//----------------------------------------------------------------------------- 
void StPico2RefTreeMaker::Clear(Option_t *opt) 
{

}
//----------------------------------------------------------------------------- 
Int_t StPico2RefTreeMaker::Make() 
{
	if(!mPicoDstMaker) 
	{
		LOG_WARN << " No PicoDstMaker! Skip! " << endm;
		return kStWarn;
	}
	
	mPicoDst = mPicoDstMaker->picoDst();
	
	if(!mPicoDst) 
	{
		LOG_WARN << " No PicoDst! Skip! " << endm;
		return kStWarn;
	}
	
	//findFiredQT(mPicoDst);
	//mPicoDst->printTracks();

	//Load event
	mPicoEvent = (StPicoEvent*)mPicoDst->event();
	
	if(!mPicoEvent)
	{
		cerr<<"Error opening picoDst Event, skip!"<<endl;
		return kStWarn;
	}

	int passEvent = fillEvent(mPicoEvent);

	if( passEvent == 1 )
	{
		anaTree->Fill();
	}
	
	return kStOK;
}

//---------------------------------------------------------------------
Double_t StPico2RefTreeMaker::rotatePhi(Double_t phi) const
{
	Double_t outPhi = phi;
	while(outPhi<0) outPhi += 2*TMath::Pi();
	while(outPhi>2*TMath::Pi()) outPhi -= 2*TMath::Pi();
	return outPhi;
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::checkTriggers(int trgTypeIndex)
{
	for(auto itrg = triggers[trgTypeIndex].begin(); itrg < triggers[trgTypeIndex].end(); ++itrg)
	{
		if(mPicoDst->event()->isTrigger(*itrg))
		{
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBAll()
{ 
	return ( checkTriggers(0) );
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBtrg1()
{ 
	return ( checkTriggers(1) );
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBtrg2()
{ 
	return ( checkTriggers(2) );
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBtrg3()
{ 
	return ( checkTriggers(3) );
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBtrg4()
{ 
	return ( checkTriggers(4) );
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBtrg5()
{ 
	return ( checkTriggers(5) );
}
//---------------------------------------------------------------------
Bool_t StPico2RefTreeMaker::isMBtrg6()
{ 
	return ( checkTriggers(6) );
}
//---------------------------------------------------------------------
Int_t StPico2RefTreeMaker::getRunIndex(Int_t runId)
{
	map<Int_t, Int_t>::iterator iter = mTotalRunId.find(runId);
	
	int runIndex = -1;
	
	if(iter != mTotalRunId.end()) 
	{
		runIndex = iter->second;
	}else
	{
		runIndex = -1;
		//cout<<"this run number is: "<<runId<<endl;
		//cout<<"Not found in the runnumber list!!!"<<endl;
	}
	
	return runIndex;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
Int_t StPico2RefTreeMaker::fillEvent(StPicoEvent* mPicoEvent)
{
	Int_t runid = (int) mPicoEvent->runId();
	runidx      = getRunIndex( runid );
	
	h_EvtCounter->Fill(0);

	if( isMBAll() )
	{
		h_EvtCounter->Fill(1);
		if(isMBtrg1()) h_EvtCounter->Fill(2);
		if(isMBtrg2()) h_EvtCounter->Fill(3);
		if(isMBtrg3()) h_EvtCounter->Fill(4);
		if(isMBtrg4()) h_EvtCounter->Fill(5);
		if(isMBtrg5()) h_EvtCounter->Fill(6);
		if(isMBtrg6()) h_EvtCounter->Fill(7);
	}
	else 
	{
		return 0;
	}
	
	Float_t vzvpd  = mPicoDst->event()->vzVpd();
	Float_t vztpc  = mPicoDst->event()->primaryVertex().z();
	Float_t vxtpc  = mPicoDst->event()->primaryVertex().x();
	Float_t vytpc  = mPicoDst->event()->primaryVertex().y();
	Float_t dvz    = vztpc-vzvpd;
	Float_t vr     = sqrt(vxtpc*vxtpc+vytpc*vytpc);
	
	Float_t bfield  = mPicoDst->event()->bField();
	Float_t Ranking = mPicoDst->event()->ranking();
	Float_t zdcx    = mPicoDst->event()->ZDCx()/1000.;
	Float_t bbcx    = mPicoDst->event()->BBCx()/1000.;
	
	Int_t Grefmult  = mPicoDst->event()->grefMult();
	Int_t Refmult   = mPicoDst->event()->refMult();
	Int_t NGtrks    = mPicoDst->event()->numberOfGlobalTracks();
	Int_t Ntofhits  = mPicoDst->numberOfBTofHits();
	Int_t Ntofmatch = mPicoDst->event()->nBTOFMatch();

	if( fabs(vztpc)>200. ) return 0;
	h_EvtCounter->Fill(8);
	if( fabs(vr)>2. ) return 0;
	h_EvtCounter->Fill(9);

	if(isMBtrg1()) h_EvtCounter->Fill(10);
	if(isMBtrg2()) h_EvtCounter->Fill(11);
	if(isMBtrg3()) h_EvtCounter->Fill(12);
	if(isMBtrg4()) h_EvtCounter->Fill(13);
	if(isMBtrg5()) h_EvtCounter->Fill(14);
	if(isMBtrg6()) h_EvtCounter->Fill(15);

	mtrigWord  = getTrigWord();
	mzdcX      = zdcx;
	mbbcX      = bbcx;
	mfield     = bfield;
	mrunId     = runid;
	mrefmult   = Refmult;
	mnTofMatch = Ntofmatch;
	mvx        = vxtpc;
	mvy        = vytpc;
	mvz        = vztpc;
	mvpdVz     = vzvpd;
	
	return 1;
}
//---------------------------------------------------------------------
//----------------------------------------------------------------------------
Int_t StPico2RefTreeMaker::getTrigWord( )
{
	Int_t isMB1 = isMBtrg1();
	Int_t isMB2 = isMBtrg2();
	Int_t isMB3 = isMBtrg3();
	Int_t isMB4 = isMBtrg4();
	Int_t isMB5 = isMBtrg5();
	Int_t isMB6 = isMBtrg6();

	Int_t trgword = (isMB1<<0) + (isMB2<<1) + (isMB3<<2) + (isMB4<<3) + (isMB5<<4) + (isMB6<<5);
	
	return trgword;
}
