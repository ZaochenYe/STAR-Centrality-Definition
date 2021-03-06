#ifndef StPico2RefTreeMaker_h
#define StPico2RefTreeMaker_h
#include <vector>
#include "StPicoDstMaker/StPicoDstMaker.h"
#include "StPicoEvent/StPicoEvent.h"
#include "StPicoEvent/StPicoTrack.h"
#include "StPicoEvent/StPicoBTofHit.h"
#include "StPicoEvent/StPicoBTofPidTraits.h"
//#include "StPicoEvent/StPicoMtdPidTraits.h"
//#include "StPicoEvent/StPicoEmcTrigger.h"
//#include "StPicoEvent/StPicoMtdTrigger.h"
//#include "StPicoEvent/StPicoBbcTile.h"
//#include "StPicoEvent/StPicoEpdTile.h"
//#include "StPicoEvent/StPicoMtdHit.h"
//#include "StPicoEvent/StPicoBEmcPidTraits.h"
//#include "StPicoDstMaker/StPicoArrays.h"
//#include "StPicoDstMaker/StPicoDst.h"
//#include "StPicoEvent/StPicoBTowHit.h"
//#include "StMtdUtil/StMtdGeometry.h"
//#include "StMtdUtil/StMtdConstants.h"


using namespace std;

#include "StMaker.h"
class StPicoDst;
class StPicoDstMaker;
class TString;
class TH1F;
class TH2F;
class TTree;

class StPico2RefTreeMaker : public StMaker 
{
	public:
		StPico2RefTreeMaker(const char *name, StPicoDstMaker *picoMaker, const char *outName);
		virtual ~StPico2RefTreeMaker();

		virtual Int_t Init();
		virtual Int_t InitRun(const Int_t runNumber);
		virtual Int_t Make();
		virtual void  Clear(Option_t *opt="");
		virtual Int_t Finish();

		void    addTrigger(int tr, int id) { triggers[id].push_back(tr); };// put the triggerIDs into the vector containor
		//Int_t makeTriggerWord(StPicoEvent* ); 

		Int_t LoadRunList();
		void  DeclareHistograms();
		void  BookTree();
		void  WriteHistograms();

		Int_t fillEvent(StPicoEvent*);

		Double_t rotatePhi(Double_t phi) const;

	private:
		StPicoDstMaker *mPicoDstMaker;
		StPicoDst      *mPicoDst;
		StPicoEvent    *mPicoEvent;
		StPicoTrack	   *mPicoTrack;

		std::vector<int> triggers[7];   // 0-MB
		Int_t     mTriggerWord;
		Bool_t    checkTriggers(int);
		Bool_t    isMBAll();
		Bool_t    isMBtrg1();
		Bool_t    isMBtrg2();
		Bool_t    isMBtrg3();
		Bool_t    isMBtrg4();
		Bool_t    isMBtrg5();
		Bool_t    isMBtrg6();
		Int_t     getRunIndex(Int_t run);
		Int_t     getTrigWord();
		//-----------------------
		TString mOutName;
		TString mOutName_hist;
		TFile   *mOutFile;
		TFile   *mOutFile_hist;
		//-----------------------
		//--------------------------------------------
		//--------------------------------------------
		Int_t    mMaxNRunIds;
		TString  mRunList;
		map<Int_t, Int_t> mTotalRunId;
		//Int_t runIndex;
		Int_t runidx;
		//--------------------------------------------
		//--------------------------------------------
		TH1F *h_EvtCounter;
	
		//--------------------------------------------
		//--------------------------------------------
		TTree    *anaTree;
		Int_t    mtrigWord;
		Float_t  mfield;
		Float_t  mzdcX;
		Float_t  mbbcX;
		Int_t    mrunId;
		Int_t    mrefmult;
		Int_t    mnTofMatch;
		Float_t  mvx;
		Float_t  mvy;
		Float_t  mvz;
		Float_t  mvpdVz;
		//--------------------------------------------
		//--------------------------------------------
		ClassDef(StPico2RefTreeMaker, 1)
};

#endif
