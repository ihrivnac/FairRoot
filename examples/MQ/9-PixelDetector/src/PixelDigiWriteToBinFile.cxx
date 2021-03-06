/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * PixelDigiWriteToBinFile.h
 *
 *  Created on: 19.02.2016
 *      Author: R. Karabowicz
 */

#include "PixelDigiWriteToBinFile.h"

// Includes from base
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "FairLink.h"
#include "FairLogger.h"

// Includes from ROOT
#include "TClonesArray.h"
#include "TObjArray.h"
#include "TMath.h"
#include "TGeoManager.h"

#include "PixelDigi.h"

#include <map>

using std::pair;
using std::map;



// -----   Default constructor   ------------------------------------------
PixelDigiWriteToBinFile::PixelDigiWriteToBinFile()
  : PixelDigiWriteToBinFile("Pixel DigiWriter", 0)
{
}
// -------------------------------------------------------------------------



// -----   Standard constructor   ------------------------------------------
PixelDigiWriteToBinFile::PixelDigiWriteToBinFile(Int_t iVerbose) 
  : PixelDigiWriteToBinFile("Pixel DigiWriter", iVerbose)
{
}
// -------------------------------------------------------------------------



// -----   Constructor with name   -----------------------------------------
PixelDigiWriteToBinFile::PixelDigiWriteToBinFile(const char* name, Int_t iVerbose) 
  : FairTask(name, iVerbose)
  , fDigis(NULL)
  , fOutputFileName("test.dat")
  , fNofOutputFiles(0)
  , fOutputFiles()
  , fDivideLevel(0)
  , fRunId(0)
  , fMCEntryNo(0)
  , fPartNo(0)

{
  Reset();
}
// -------------------------------------------------------------------------



// -----   Destructor   ----------------------------------------------------
PixelDigiWriteToBinFile::~PixelDigiWriteToBinFile() { 
  Reset();
}
// -------------------------------------------------------------------------

// -----   Public method Exec   --------------------------------------------
void PixelDigiWriteToBinFile::Exec(Option_t* /*opt*/) {

  Reset();

  Int_t nofDigis = fDigis->GetEntriesFast();

  Int_t digisPerFile[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

  for ( Int_t iDigi = 0 ; iDigi < nofDigis ; iDigi++ ) {
    PixelDigi* currentDigi = static_cast<PixelDigi*>(fDigis->At(iDigi));

    short int detId     = (short int)currentDigi->GetDetectorID();    

    Int_t fileToSave = 0;
    if ( fDivideLevel == 1 ) {
      fileToSave = detId/256 - 1;
    }
    else if ( fDivideLevel == 2 ) {
      fileToSave = (detId/256 - 1)*4 + (detId%256 -1);
    }
    digisPerFile[fileToSave] += 1;
  }

  for ( Int_t ifile = 0 ; ifile < fNofOutputFiles ; ifile++ ) {
    fOutputFiles[ifile].write((char*)&fRunId             ,sizeof(fRunId));
    fOutputFiles[ifile].write((char*)&fMCEntryNo         ,sizeof(fMCEntryNo));
    fOutputFiles[ifile].write((char*)&ifile              ,sizeof(ifile));
    fOutputFiles[ifile].write((char*)&digisPerFile[ifile],sizeof(digisPerFile[ifile]));
  }
  
  for ( Int_t iDigi = 0 ; iDigi < nofDigis ; iDigi++ ) {
    PixelDigi* currentDigi = static_cast<PixelDigi*>(fDigis->At(iDigi));

    short int detId     = (short int)currentDigi->GetDetectorID();    
    short int feId      = (short int)currentDigi->GetFeID();
    short int col       = (short int)currentDigi->GetCol();
    short int row       = (short int)currentDigi->GetRow();

    Int_t fileToSave = 0;
    if ( fDivideLevel == 1 ) {
      fileToSave = detId/256 - 1;
    }
    else if ( fDivideLevel == 2 ) {
      fileToSave = (detId/256 - 1)*4 + (detId%256 -1);
    }
    fOutputFiles[fileToSave].write((char*)&detId,sizeof(detId));
    fOutputFiles[fileToSave].write((char*)&feId ,sizeof(feId ));
    fOutputFiles[fileToSave].write((char*)&col  ,sizeof(col  ));
    fOutputFiles[fileToSave].write((char*)&row  ,sizeof(row  ));
  }
  
  fMCEntryNo ++;
}
// -------------------------------------------------------------------------

// -----   Private method SetParContainers   -------------------------------
void PixelDigiWriteToBinFile::SetParContainers() {
}
// -------------------------------------------------------------------------

// -----   Private method Init   -------------------------------------------
InitStatus PixelDigiWriteToBinFile::Init() {

  // Get input array 
  FairRootManager* ioman = FairRootManager::Instance();

  if ( ! ioman ) LOG(FATAL) << "No FairRootManager" << FairLogger::endl;
  fDigis = static_cast<TClonesArray*>(ioman->GetObject("PixelDigis"));

  if ( !fDigis ) 
    LOG(WARNING) << "PixelDigiWriteToBinFile::Init() No input PixelDigis array!" << FairLogger::endl;

  LOG(INFO) << "-I- " << fName.Data() << "::Init(). Initialization succesfull." << FairLogger::endl;

  fRunId = ioman->GetRunId();

  if      ( fDivideLevel == 0 ) {
    fNofOutputFiles =  1;
    fOutputFiles[0].open(fOutputFileName.Data(),std::fstream::out|std::fstream::binary);
  }
  else {
    if ( fDivideLevel == 1 ) {
      fNofOutputFiles =  3; // 1 file per station (3 stations)
    }
    else if ( fDivideLevel == 2 ) {
      fNofOutputFiles = 12; // 1 file per sensor  (3 stations times 4 sensors)
    }
    else {
      LOG(FATAL) << "PixelDigiWriteToBinFile::Init(), fDivideLevel = " << fDivideLevel << " unknown, it has to be in the range <0,2>" << FairLogger::endl;
      return kFATAL;
    }
    for ( Int_t ifile = 0 ; ifile < fNofOutputFiles ; ifile++ ) {
      TString fileName = fOutputFileName;
      TString uniqFile = Form(".p%d.",ifile);
      fileName.Replace(fileName.Last('.'),1,uniqFile.Data());
      fOutputFiles[ifile].open(fileName.Data(),std::fstream::out);
    }
  }    
  
  return kSUCCESS;

}
// -------------------------------------------------------------------------



// -----   Private method ReInit   -----------------------------------------
InitStatus PixelDigiWriteToBinFile::ReInit() {
  FairRootManager* ioman = FairRootManager::Instance();

  if ( ! ioman ) LOG(FATAL) << "No FairRootManager found." << FairLogger::endl;

  fRunId = ioman->GetRunId();
  fMCEntryNo = 0;
 
  return kSUCCESS;
}
// -------------------------------------------------------------------------



// -----   Private method Reset   ------------------------------------------
void PixelDigiWriteToBinFile::Reset() {
}
// -------------------------------------------------------------------------

// -----   Public method Finish   ------------------------------------------
void PixelDigiWriteToBinFile::Finish() {
  for ( Int_t ifile = 0 ; ifile < fNofOutputFiles ; ifile++ ) {
    fOutputFiles[ifile].close();
  }
}
// -------------------------------------------------------------------------



ClassImp(PixelDigiWriteToBinFile)

