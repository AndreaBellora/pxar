#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <bitset>

#include <cstdlib>
#include <stdio.h>

#include "dictionaries.h"
#include "log.h"

#include "ConfigParameters.hh"

using namespace std;
using namespace pxar;


ConfigParameters * ConfigParameters::fInstance = 0;

// ----------------------------------------------------------------------
ConfigParameters::ConfigParameters() {
  initialize();
}


// ----------------------------------------------------------------------
ConfigParameters::ConfigParameters(string filename) {
  initialize();
  readConfigParameterFile(filename);
}


// ----------------------------------------------------------------------
void ConfigParameters::initialize() {
  fReadTbParameters = fReadTbmParameters = fReadDacParameters = fReadRocPixelConfig = fReadReadbackCal = false;
  fnCol = 52;
  fnRow = 80;
  fnRocs = 16;
  fnTbms = 1;
  fnModules = 1;
  fHubIds.push_back(31);
  fI2cAddresses.clear();

  fHvOn = true;
  fTbmEnable = true;
  fTbmEmulator = false;
  fKeithleyRemote = false;
  fGuiMode = false;
  fTbmChannel = 0;
  fHalfModule = 0;

  fEmptyReadoutLength = 54;
  fEmptyReadoutLengthADC = 64;
  fEmptyReadoutLengthADCDual = 40;

  fTrimVcalSuffix                = "";
  fDACParametersFileName         = "defaultDACParameters";
  fTbmParametersFileName         = "defaultTBMParameters";
  fTBParametersFileName          = "defaultTBParameters.dat";
  fTrimParametersFileName        = "defaultTrimParameters";
  fTestParametersFileName        = "defaultTestParameters.dat";
  fMaskFileName                  = "defaultMaskFile.dat";
  fLogFileName                   = "log.txt";
  fDebugFileName                 = "debug.log";
  fRootFileName                  = "expert.root";
  fGainPedestalParameterFileName = "phCalibrationFitErr";
  fGainPedestalFileName          = "phCalibration";
  fReadbackCalFileName           = "readbackCal";

  ia = -1.;
  id = -1.;
  va = -1.;
  vd = -1.;

  fProbeA1 = "sdata1";
  fProbeA2 = "sdata2";
  fProbeD1 = "clk";
  fProbeD2 = "ctr";

  rocZeroAnalogCurrent = 0.0;
  fRocType = "psi46digv21respin";
  fTbmType = "";
  fHdiType = "bpix";
  fTBName = "*";

  fGuiX = fGuiY = 0;

  fMaskedPixels.clear();
}


// ----------------------------------------------------------------------
ConfigParameters*  ConfigParameters::Singleton() {
  if (fInstance == 0) {fInstance = new ConfigParameters();}
  return fInstance;
}


// ----------------------------------------------------------------------
void ConfigParameters::readAllConfigParameterFiles() {
  readTbParameters();
  readRocPixelConfig();
  readRocDacs();
  readTbmDacs();
  readReadbackCal();
}


// ----------------------------------------------------------------------
void ConfigParameters::writeAllFiles() {
  writeConfigParameterFile();
  writeTbParameterFile();
}



// ----------------------------------------------------------------------
bool ConfigParameters::readConfigParameterFile(string file) {
  ifstream _input(file.c_str());
  if (!_input.is_open())
    {
      LOG(logINFO) << "Can not open file '"  << file << "' to read Config Parameters.";

      return false;
    }

  // Read file by lines
  for (string _line; _input.good();)
    {
      getline(_input, _line);

      // Skip Empty Lines and Comments (starting from # or - )
      if (!_line.length()
	  || '#' == _line[0]
	  || '-' == _line[0]) continue;

      istringstream _istring(_line);
      string _name;
      string _value;

      _istring >> _name >> _value;

      // Skip line in case any errors occured while reading parameters
      if (_istring.fail() || !_name.length()) continue;

      int _ivalue = atoi(_value.c_str());
      double dvalue = atof(_value.c_str());

      if (0 == _name.compare("testboardName")) { fTBName = _value; }
      else if (0 == _name.compare("directory")) { fDirectory =  _value; }

      else if (0 == _name.compare("tbParameters")) { setTBParameterFileName(_value); }
      else if (0 == _name.compare("tbmParameters")) { setTbmParameterFileName(_value); }
      else if (0 == _name.compare("testParameters")) { setTestParameterFileName(_value); }
      else if (0 == _name.compare("dacParameters")) { setDACParameterFileName(_value); }
      else if (0 == _name.compare("rootFileName")) { setRootFileName(_value); }
      else if (0 == _name.compare("trimParameters")) { setTrimParameterFileName(_value); }
      else if (0 == _name.compare("maskFile")) { setMaskFileName(_value); }
      else if (0 == _name.compare("gainPedestalParameters")) {fGainPedestalParameterFileName = _value;}

      else if (0 == _name.compare("nModules")) { fnModules                  = _ivalue; }
      else if (0 == _name.compare("nRocs")) { readNrocs(_istring.str()); }
      else if (0 == _name.compare("nTbms")) { fnTbms                     = _ivalue; }
      else if (0 == _name.compare("hubId")) { readHubIds(_istring.str()); }
      else if (0 == _name.compare("halfModule")) { fHalfModule                = _ivalue; }
      else if (0 == _name.compare("emptyReadoutLength")) { fEmptyReadoutLength        = _ivalue; }
      else if (0 == _name.compare("emptyReadoutLengthADC")) { fEmptyReadoutLengthADC     = _ivalue; }
      else if (0 == _name.compare("emptyReadoutLengthADCDual")) { fEmptyReadoutLengthADCDual = _ivalue; }
      else if (0 == _name.compare("hvOn")) { fHvOn                      = (_ivalue>0); }
      else if (0 == _name.compare("keithleyRemote")) { fKeithleyRemote            = (_ivalue>0); }
      else if (0 == _name.compare("tbmEnable")) { fTbmEnable                 = (_ivalue>0); }
      else if (0 == _name.compare("tbmEmulator")) { fTbmEmulator             = (_ivalue>0); }
      else if (0 == _name.compare("tbmChannel")) { fTbmChannel                = _ivalue; }

      else if (0 == _name.compare("ia")) { ia = static_cast<float>((dvalue > 1000.?.001:1.) * dvalue); }
      else if (0 == _name.compare("id")) { id = static_cast<float>((dvalue > 1000.?.001:1.) * dvalue); }
      else if (0 == _name.compare("va")) { va = static_cast<float>((dvalue > 1000.?.001:1.) * dvalue); }
      else if (0 == _name.compare("vd")) { vd = static_cast<float>((dvalue > 1000.?.001:1.) * dvalue); }

      else if (0 == _name.compare("rocZeroAnalogCurrent")) { rocZeroAnalogCurrent = static_cast<float>(.001 * static_cast<float>(_ivalue)); }

      else if (0 == _name.compare("rocType")) { fRocType = _value; }
      else if (0 == _name.compare("tbmType")) { fTbmType = _value; }
      else if (0 == _name.compare("hdiType")) { fHdiType = _value; }

      else if (0 == _name.compare("probeA1")) { fProbeA1 = _value; }
      else if (0 == _name.compare("probeA2")) { fProbeA2 = _value; }
      else if (0 == _name.compare("probeD1")) { fProbeD1 = _value; }
      else if (0 == _name.compare("probeD2")) { fProbeD2 = _value; }

      else if (0 == _name.compare("guiX")) { fGuiX = _ivalue; }
      else if (0 == _name.compare("guiY")) { fGuiY = _ivalue; }


      else { LOG(logINFO) << "Did not understand '" << _name << "'."; }
    }

  _input.close();

  fTbPowerSettings.push_back(make_pair("ia", ia));
  fTbPowerSettings.push_back(make_pair("id", id));
  fTbPowerSettings.push_back(make_pair("va", va));
  fTbPowerSettings.push_back(make_pair("vd", vd));

  return true;
}


// ----------------------------------------------------------------------
vector<pair<string, uint8_t> > ConfigParameters::readDacFile(string fname) {
  vector<pair<string, uint8_t> > rocDacs;

  // -- read in file
  vector<string> lines;
  char  buffer[5000];
  ifstream is(fname.c_str());
  while (is.getline(buffer, 200, '\n')) {
    lines.push_back(string(buffer));
  }
  is.close();

  // -- parse lines
  unsigned int ival(0);
  uint8_t uval(0);
  //  unsigned char uval(0);

  string::size_type s1, s2;
  string str1, str2, str3;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    //    cout << lines[i] << endl;
    // -- remove tabs, adjacent spaces, leading and trailing spaces, and everything after (and including) a #
    cleanupString(lines[i]);
    if (lines[i].length() < 2) continue;
    s1 = lines[i].find(" ");
    s2 = lines[i].rfind(" ");
    if (s1 != s2) {
      // -- with register number
      str1 = lines[i].substr(0, s1);
      str2 = lines[i].substr(s1+1, s2-s1-1);
      str3 = lines[i].substr(s2+1);
    } else {
      // -- without register number
      str2 = lines[i].substr(0, s1);
      str3 = lines[i].substr(s1+1);
    }

    if (string::npos != str3.find("0x")) {
      sscanf(str3.c_str(), "%x", &ival);
    } else {
      ival = atoi(str3.c_str());
    }
    uval = ival;
    // LOG(logINFO) << "Reading line: " << str2 << " " << int(uval);
    rocDacs.push_back(make_pair(str2, uval));

  }

  return rocDacs;
}

// ----------------------------------------------------------------------
vector<pair<string, uint8_t> >  ConfigParameters::getTbParameters() {
  if (!fReadTbParameters) {
    readTbParameters();
  }
  return  fTbParameters;
}


// ----------------------------------------------------------------------
void ConfigParameters::readTbParameters() {
  if (!fReadTbParameters) {
    string filename = fDirectory + "/" + fTBParametersFileName;
    fTbParameters = readDacFile(filename);
    // Cannot use LOG(...) for this printout, as pxarCore is instantiated only afterwards ...
    for (unsigned int i = 0; i < fTbParameters.size(); ++i) {
      //      LOG(logDEBUG) << fTbParameters[i].first << ": " << (int)fTbParameters[i].second;
      LOG(logINFO) << "        " << fTbParameters[i].first << ": " << (int)fTbParameters[i].second;
    }
    fReadTbParameters = true;
  }
}

// ----------------------------------------------------------------------
vector<pair<string, double> >  ConfigParameters::getTbPowerSettings() {
  vector<pair<string, double> > v;
  if (ia < 0.) {
    LOG(logINFO) << "Read config files first!" << endl;
    return v;
  }
  return fTbPowerSettings;
}

// ----------------------------------------------------------------------
vector<pair<string, uint8_t> >  ConfigParameters::getTbSigDelays() {
  vector<pair<string, uint8_t> > a;

  RegisterDictionary *dict = RegisterDictionary::getInstance();
  vector<string> sigdelays = dict->getAllDTBNames();

  if (!fReadTbParameters) readTbParameters();
  for (unsigned int i = 0; i < fTbParameters.size(); ++i) {
    for (unsigned int j = 0; j < sigdelays.size(); ++j) {
      if (0 == fTbParameters[i].first.compare(sigdelays[j])) a.push_back(make_pair(sigdelays[j], fTbParameters[i].second));
    }
  }

  return a;
}

// ----------------------------------------------------------------------
vector<pair<string, uint8_t> >  ConfigParameters::getTbPgSettings() {

  vector<pair<string, uint8_t> > a;

  uint8_t wbc = 100;
  vector<vector<pair<string, uint8_t> > > dacs = getRocDacs();
  for (unsigned int idac = 0; idac < dacs[0].size(); ++idac) {
    if (!dacs[0][idac].first.compare("wbc")) {
      wbc = dacs[0][idac].second;
    }
  }

  uint8_t delay = 6;
  if((fRocType.find("proc600v3") != string::npos) || (fRocType.find("proc600_v3") != string::npos)){
    delay = 6;
  } else if((fRocType.find("proc600v4") != string::npos) || (fRocType.find("proc600_v4") != string::npos)){
    delay = 7;
    delay = 6; // go back to old value
  } else if(fRocType.find("dig") == string::npos) {
    delay = 5;
  }

  if (fnTbms < 1) {
    a.push_back(make_pair("resetroc",25));    // PG_RESR b001000
    a.push_back(make_pair("calibrate",wbc+delay)); // PG_CAL  b000100
    a.push_back(make_pair("trigger",16));    // PG_TRG  b000010
    a.push_back(make_pair("token",0));     // PG_TOK  b000001
  } else {
    a.push_back(make_pair("resetroc",15));    // PG_REST
    a.push_back(make_pair("calibrate",wbc+delay)); // PG_CAL
    a.push_back(make_pair("trigger;sync",0));     // PG_TRG PG_SYNC
  }

  return a;
}


// ----------------------------------------------------------------------
vector<vector<pxar::pixelConfig> > ConfigParameters::getRocPixelConfig() {
  if (!fReadRocPixelConfig) {
    readRocPixelConfig();
  }
  return fRocPixelConfigs;
}

// ----------------------------------------------------------------------
vector<pxar::pixelConfig> ConfigParameters::getRocPixelConfig(int i) {
  if (!fReadRocPixelConfig) {
    readRocPixelConfig();
  }
  return fRocPixelConfigs[i];


}

// ----------------------------------------------------------------------
void ConfigParameters::readRocPixelConfig() {
  string filename;
  // -- read one mask file containing entire DUT mask
  filename = fDirectory + "/" + fMaskFileName;
  vector<bool> rocmasked;
  for (unsigned int i = 0; i < fnRocs; ++i) rocmasked.push_back(false);

  fMaskedPixels = readMaskFile(filename);

  for (unsigned int i = 0; i < fMaskedPixels.size(); ++i) {
    vector<pair<int, int> > v = fMaskedPixels[i];
    if (v.size() > 0) {
      rocmasked[i] = true;
      for (unsigned int j = 0; j < v.size(); ++j) {
	LOG(logINFO) << "MASKED Roc " << i << " col/row: " << v[j].first << " " << v[j].second;
      }
    }
  }

  // -- read all trim files and create pixelconfig vector
  stringstream firstFile, lastFile;
  firstFile << fDirectory << "/" << fTrimParametersFileName << fTrimVcalSuffix << "_C" << 0 << ".dat";
  lastFile << fDirectory << "/" << fTrimParametersFileName << fTrimVcalSuffix << "_C" << fnRocs-1 << ".dat";
  LOG(logINFO) << "readTrimFile: " << firstFile.str() << " .. " << lastFile.str();

  for (unsigned int i = 0; i < fnRocs; ++i) {
    vector<pxar::pixelConfig> v;
    for (uint8_t ic = 0; ic < fnCol; ++ic) {
      for (uint8_t ir = 0; ir < fnRow; ++ir) {
	//	pxar::pixelConfig a(ic,ir,0,false,true);
	pxar::pixelConfig a(ic,ir,0,false,false);
	if (rocmasked[i]) {
	  vector<pair<int, int> > v = fMaskedPixels[i];
	  for (unsigned int j = 0; j < v.size(); ++j) {
	    if (v[j].first == ic && v[j].second == ir) {
	      LOG(logINFO) << "  masking Roc " << i << " col/row: " << v[j].first << " " << v[j].second;
	      a.setMask(true);
	    }
	  }
	}
	v.push_back(a);
      }
    }
    stringstream fname;
    fname << fDirectory << "/" << fTrimParametersFileName << fTrimVcalSuffix << "_C" << i << ".dat";
    readTrimFile(fname.str(), v);
    fRocPixelConfigs.push_back(v);
  }

  fReadRocPixelConfig = true;
}


// ----------------------------------------------------------------------
void ConfigParameters::readTrimFile(string fname, vector<pxar::pixelConfig> &v) {

  // -- read in file
  vector<string> lines;
  char  buffer[5000];
  ifstream is(fname.c_str());
  while (is.getline(buffer, 200, '\n')) {
    lines.push_back(string(buffer));
  }
  is.close();

  // -- parse lines
  unsigned int ival(0), irow(0), icol(0);
  uint8_t uval(0);

  string::size_type s1, s2;
  string str1, str2, str3;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    // -- remove tabs, adjacent spaces, leading and trailing spaces
    replaceAll(lines[i], "\t", " ");
    replaceAll(lines[i], "Pix", " ");
    string::iterator new_end = unique(lines[i].begin(), lines[i].end(), bothAreSpaces);
    lines[i].erase(new_end, lines[i].end());
    if (0 == lines[i].length()) continue;
    if (lines[i].substr(0, 1) == string(" ")) lines[i].erase(0, 1);
    if (lines[i].substr(lines[i].length()-1, 1) == string(" ")) lines[i].erase(lines[i].length()-1, 1);
    s1 = lines[i].find(" ");
    s2 = lines[i].rfind(" ");
    if (s1 != s2) {
      str1 = lines[i].substr(0, s1);
      str2 = lines[i].substr(s1+1, s2-s1);
      str3 = lines[i].substr(s2+1);
    } else {
      LOG(logINFO) << "could not read line -->" << lines[i] << "<--";
    }

    ival = atoi(str1.c_str());
    icol = atoi(str2.c_str());
    irow = atoi(str3.c_str());
    uval = ival;
    unsigned int index = icol*80+irow;
    if (index <= v.size()) {
      v[index].setTrim(uval);
    } else {
      LOG(logINFO) << " not matching entry in trim vector found for row/col = " << irow << "/" << icol;
    }
  }


}


// ----------------------------------------------------------------------
vector<vector<pair<int, int> > > ConfigParameters::readMaskFile(string fname) {

  vector<vector<pair<int, int> > > v;
  vector<pair<int, int> > a;
  for (unsigned int i = 0; i < fnRocs; ++i) {
    v.push_back(a);
  }

  // -- read in file
  vector<string> lines;
  char  buffer[5000];
  LOG(logINFO) << "readMaskFile: " << fname;
  ifstream is(fname.c_str());
  while (is.getline(buffer, 200, '\n')) {
    lines.push_back(string(buffer));
  }
  is.close();

  // -- parse lines
  unsigned int iroc(0), irow(0), icol(0);

  string::size_type s1, s2, s3;
  string str1, str2, str3;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    //    cout << lines[i] << endl;
    if (lines[i].substr(0, 1) == string("#")) continue;
    // -- remove tabs, adjacent spaces, leading and trailing spaces
    cleanupString(lines[i]);
    if (0 == lines[i].length()) continue;

    s1 = lines[i].find("roc");
    if (string::npos != s1) {
      str3 = lines[i].substr(s1+4);
      iroc = atoi(str3.c_str());
      //      cout << "masking all pixels for ROC " << iroc << endl;
      if (iroc < fnRocs) {
	for (uint8_t ic = 0; ic < fnCol; ++ic) {
	  for (uint8_t ir = 0; ir < fnRow; ++ir) {
	    v[iroc].push_back(make_pair(ic, ir));
	  }
	}
      } else {
	LOG(logINFO) << "illegal ROC coordinates in line " << i << ": " << lines[i];
      }
      continue;
    }

    s1 = lines[i].find("row");
    if (string::npos != s1) {
      s1 += 4;
      s2 = lines[i].find(" ", s1) + 1;
      iroc = atoi(lines[i].substr(s1, s2-s1-1).c_str());
      irow = atoi(lines[i].substr(s2).c_str());
      //      cout << "-> MASKING ROC " << iroc << " row " << irow << endl;
      if (iroc < fnRocs && irow < fnRow) {
	for (unsigned int ic = 0; ic < fnCol; ++ic) {
	  v[iroc].push_back(make_pair(ic, irow));
	}
      } else {
	LOG(logINFO) << "illegal ROC/row coordinates in line " << i << ": " << lines[i];
      }
      continue;
    }

    s1 = lines[i].find("col");
    if (string::npos != s1) {
      s1 += 4;
      s2 = lines[i].find(" ", s1) + 1;
      iroc = atoi(lines[i].substr(s1, s2-s1-1).c_str());
      icol = atoi(lines[i].substr(s2).c_str());
      //      cout << "-> MASKING ROC " << iroc << " col " << icol << endl;
      if (iroc < fnRocs && icol < fnCol) {
	for (unsigned int ir = 0; ir < fnRow; ++ir) {
	  v[iroc].push_back(make_pair(icol, ir));
	}
      } else {
	LOG(logINFO) << "illegal ROC/col coordinates in line " << i << ": " << lines[i];
      }
      continue;
    }

    s1 = lines[i].find("pix");
    if (string::npos != s1) {
      s1 += 4;
      s2 = lines[i].find(" ", s1) + 1;
      s3 = lines[i].find(" ", s2) + 1;
      iroc = atoi(lines[i].substr(s1, s2-s1-1).c_str());
      icol = atoi(lines[i].substr(s2, s3-s2-1).c_str());
      irow = atoi(lines[i].substr(s3).c_str());
      //      cout << "-> MASKING ROC " << iroc << " col " << icol << " row " << irow << endl;
      if (iroc < fnRocs && icol < fnCol && irow < fnRow) {
	v[iroc].push_back(make_pair(icol, irow));
      } else {
	LOG(logINFO) << "illegal ROC/row/col coordinates in line " << i << ": " << lines[i];
      }
      continue;
    }
  }

  return v;
}


// ----------------------------------------------------------------------
bool ConfigParameters::writeMaskFile(vector<vector<pair<int, int> > > v, string name) {
  stringstream fname;
  if (!name.compare("")) {
    fname << fDirectory << "/" << fMaskFileName;
  } else {
    if (string::npos == name.find("/")) {
      fname << fDirectory << "/" << name;
    } else {
      fname << name;
    }
  }

  ofstream OutputFile;
  OutputFile.open((fname.str()).c_str());
  if (!OutputFile.is_open()) {
    LOG(logERROR) << "failed to open " << fname.str().c_str() << " as mask file";
    return false;
  } else {
    LOG(logINFO) << "write masked pixels into " << fname.str();
  }

  OutputFile << "# mask file generated by maskHotPixels" << endl;

  for (unsigned int iroc = 0; iroc < v.size(); ++iroc) {
    vector<pair<int, int> > a = v[iroc];
    for (unsigned int ipix = 0; ipix < a.size(); ++ipix) {
      OutputFile << "pix " << iroc << " "
		 << a[ipix].first << " " << a[ipix].second
		 << endl;
    }
  }
  OutputFile.close();
  return true;

}




// ----------------------------------------------------------------------
vector<vector<pair<string, uint8_t> > > ConfigParameters::getRocDacs() {
  if (!fReadDacParameters) {
    readRocDacs();
  }
  return fDacParameters;
}


// ----------------------------------------------------------------------
vector<string> ConfigParameters::getDacs() {
  if (!fReadDacParameters) {
    readRocDacs();
  }
  vector<string> names;
  vector<pair<string, uint8_t> > dacs = fDacParameters[0];
  for (unsigned int i = 0; i < dacs.size(); ++i) {
    names.push_back(dacs[i].first);
  }
  return names;
}


// ----------------------------------------------------------------------
void ConfigParameters::readRocDacs() {
  if (!fReadDacParameters) {

    stringstream firstFile, lastFile;
    firstFile << fDirectory << "/" << fDACParametersFileName << fTrimVcalSuffix << "_C" << 0 << ".dat";
    lastFile << fDirectory << "/" << fDACParametersFileName << fTrimVcalSuffix << "_C" << fnRocs-1 << ".dat";
    LOG(logINFO) << "readRocDacs: " << firstFile.str() << " .. " << lastFile.str();

    for (unsigned int i = 0; i < fnRocs; ++i) {
      stringstream filename;
      filename << fDirectory << "/" << fDACParametersFileName << fTrimVcalSuffix << "_C" << i << ".dat";
      vector<pair<string, uint8_t> > rocDacs = readDacFile(filename.str());
      fDacParameters.push_back(rocDacs);
    }
    fReadDacParameters = true;
  }
}


// ----------------------------------------------------------------------
vector<vector<pair<string, uint8_t> > > ConfigParameters::getTbmDacs() {
  if (!fReadTbmParameters) {
    readTbmDacs();
  }
  return fTbmParameters;
}

// ----------------------------------------------------------------------
void ConfigParameters::readTbmDacs() {
  if (!fReadTbmParameters) {
    stringstream firstFile, lastFile;
    firstFile << fDirectory << "/" << fTbmParametersFileName << "_C" << "0a" << ".dat";
    lastFile << fDirectory << "/" << fTbmParametersFileName << "_C" << fnTbms-1 << "b" << ".dat";
    LOG(logINFO) << "readTbmDacs: " << firstFile.str() << " .. " << lastFile.str();

    string filename;
    for (unsigned int i = 0; i < fnTbms; ++i) {
      for (unsigned int ic = 0; ic < 2; ++ic) {
	stringstream filename;
	filename << fDirectory << "/" << fTbmParametersFileName << "_C" << i << (ic==0?"a":"b") << ".dat";
	vector<pair<string, uint8_t> > rocDacs = readDacFile(filename.str());
	fTbmParameters.push_back(rocDacs);
      }
    }
    fReadTbmParameters = true;
  }
}


// ----------------------------------------------------------------------
bool ConfigParameters::setTbParameter(string var, uint8_t val, bool appendIfNotFound) {
  for (unsigned int i = 0; i < fTbParameters.size(); ++i) {
    if (!fTbParameters[i].first.compare(var)) {
      fTbParameters[i].second = val;
      return true;
    }
  }
  if (appendIfNotFound) {
    fTbParameters.push_back(make_pair(var, val));
    return true;
  }
  return false;
}


// ----------------------------------------------------------------------
bool ConfigParameters::setTbmDac(string var, uint8_t val, int itbm) {
  bool changed(false);
  for (unsigned int i = 0; i < fTbmParameters.size(); ++i) {
    if (itbm < 0 || itbm == static_cast<int>(i)) {
      for (unsigned int idac = 0; idac < fTbmParameters[i].size(); ++idac) {
	if (!fTbmParameters[i][idac].first.compare(var)) {
	  fTbmParameters[i][idac].second = val;
	  changed = true;
	  break;
	}
      }
    }
  }
  if (changed) {
    return true;
  }
  return false;
}


// ----------------------------------------------------------------------
bool ConfigParameters::setRocDac(string var, uint8_t val, int iroc) {
  bool changed(false);
  for (unsigned int i = 0; i < fDacParameters.size(); ++i) {
    if (iroc < 0 || iroc == static_cast<int>(i)) {
      for (unsigned int idac = 0; idac < fDacParameters[i].size(); ++idac) {
	if (!fDacParameters[i][idac].first.compare(var)) {
	  fDacParameters[i][idac].second = val;
	  changed = true;
	  break;
	}
      }
    }
  }
  if (changed) {
    return true;
  }
  return false;
}


// ----------------------------------------------------------------------
bool ConfigParameters::setTbPowerSettings(string var, double val) {

  for (unsigned int i = 0; i < fTbPowerSettings.size(); ++i) {
    if (!fTbPowerSettings[i].first.compare(var)) {
      fTbPowerSettings[i].second = val;
      return true;
    }
  }
  return false;

}


// ----------------------------------------------------------------------
bool ConfigParameters::setTrimBits(int trim) {
  for (unsigned int iroc = 0; iroc < fRocPixelConfigs.size(); ++iroc) {
    for (unsigned int ipix = 0; ipix < fRocPixelConfigs[iroc].size(); ++ipix) {
      fRocPixelConfigs[iroc][ipix].setTrim(trim);
    }
  }
  return true;
}

// ----------------------------------------------------------------------
string ConfigParameters::vectorToString(vector<uint8_t> vec) {
  stringstream ss;
  for (vector<uint8_t>::iterator ivec = vec.begin(); ivec != vec.end() - 1; ivec++)
    ss << int(*ivec) << ", ";
  ss << int(vec.back());
  return ss.str();
}


// ----------------------------------------------------------------------
bool ConfigParameters::writeConfigParameterFile() {
  char filename[1000];
  sprintf(filename, "%s/configParameters.dat", fDirectory.c_str());
  FILE * file = fopen(filename, "w");
  if (!file)
    {
      LOG(logINFO) << "Can not open file '" << filename << "' to write configParameters.";
      return false;
    }

  LOG(logINFO) << "Writing Config-Parameters to '" << filename << "'.";

  fprintf(file, "testboardName %s\n\n", fTBName.c_str());

  fprintf(file, "-- parameter files\n\n");

  fprintf(file, "tbParameters %s\n",   fTBParametersFileName.c_str());
  fprintf(file, "dacParameters %s\n",  fDACParametersFileName.c_str());
  fprintf(file, "tbmParameters %s\n",  fTbmParametersFileName.c_str());
  fprintf(file, "trimParameters %s\n", fTrimParametersFileName.c_str());
  fprintf(file, "maskFile %s\n",       fMaskFileName.c_str());
  fprintf(file, "testParameters %s\n", fTestParametersFileName.c_str());
  fprintf(file, "rootFileName %s\n\n", fRootFileName.c_str());

  fprintf(file, "-- configuration\n\n");

  fprintf(file, "nModules %i\n", fnModules);
  fprintf(file, "nRocs %i\n", fnRocs);
  fprintf(file, "nTbms %i\n", fnTbms);
  fprintf(file, "hubId %s\n", vectorToString(fHubIds).c_str());
  fprintf(file, "tbmEnable %i\n", fTbmEnable);
  fprintf(file, "tbmEmulator %i\n", fTbmEmulator);
  fprintf(file, "hvOn %i\n", fHvOn);
  fprintf(file, "tbmChannel %i\n", fTbmChannel);
  fprintf(file, "rocType %s\n", fRocType.c_str());
  if (fnTbms > 0) fprintf(file, "tbmType %s\n", fTbmType.c_str());
  fprintf(file, "hdiType %s\n", fHdiType.c_str());

  fprintf(file, "\n");

  fprintf(file, "guiX %i\n", fGuiX);
  fprintf(file, "guiY %i\n", fGuiY);

  fprintf(file, "\n");

  fprintf(file, "-- voltages and current limits\n\n");

  fprintf(file, "ia %i\n"  , static_cast<int>(ia * 1000));
  fprintf(file, "id %i\n"  , static_cast<int>(id * 1000));
  fprintf(file, "va %i\n"  , static_cast<int>(va * 1000));
  fprintf(file, "vd %i\n\n", static_cast<int>(vd * 1000));

  fprintf(file, "probeA1 %s\n", fProbeA1.c_str());
  fprintf(file, "probeA2 %s\n", fProbeA2.c_str());
  fprintf(file, "probeD1 %s\n", fProbeD1.c_str());
  fprintf(file, "probeD2 %s\n", fProbeD2.c_str());

  fclose(file);
  return true;
}


// ----------------------------------------------------------------------
bool ConfigParameters::writeTrimFile(int iroc, vector<pixelConfig> v) {
  stringstream fname;
  fname << fDirectory << "/" << fTrimParametersFileName << fTrimVcalSuffix << "_C" << iroc << ".dat";

  ofstream OutputFile;
  OutputFile.open((fname.str()).c_str());
  if (!OutputFile.is_open()) {
    return false;
  } else {
    LOG(logINFO) << "write trim parameters into " << fname.str();
  }

  for (vector<pixelConfig>::iterator ipix = v.begin(); ipix != v.end(); ++ipix) {
    OutputFile << setw(2) << static_cast<int>(ipix->trim())
	       << "   Pix " << setw(2)
	       << static_cast<int>(ipix->column()) << " " << setw(2) << static_cast<int>(ipix->row())
	       << endl;
  }

  OutputFile.close();
  return true;
}


// ----------------------------------------------------------------------
bool ConfigParameters::writeDacParameterFile(int iroc, vector<pair<string, uint8_t> > v) {

  stringstream fname;
  fname << fDirectory << "/" << getDACParameterFileName() << fTrimVcalSuffix << "_C" << iroc << ".dat";

  ofstream OutputFile;
  OutputFile.open((fname.str()).c_str());
  if (!OutputFile.is_open()) {
    return false;
  } else {
    LOG(logINFO) << "write dac parameters into " << fname.str();
  }

  RegisterDictionary *a = RegisterDictionary::getInstance();
  for (vector<pair<string,uint8_t> >::iterator idac = v.begin(); idac != v.end(); ++idac) {
    // LOG(logINFO) << "Writing line: " << right << setw(3) << static_cast<int>(a->getRegister(idac->first, ROC_REG)) << " " << left
    //      << setw(10) << idac->first << " " << setw(3) << static_cast<int>(idac->second);
    //    OutputFile << left << setw(10) << idac->first << " " << setw(3) << static_cast<int>(idac->second) << endl;
    OutputFile << right << setw(3) << static_cast<int>(a->getRegister(idac->first, ROC_REG)) << " " << left
	       << setw(10) << idac->first << " " << setw(3) << static_cast<int>(idac->second)
	       << endl;
  }

  OutputFile.close();
  return true;
}


// ----------------------------------------------------------------------
bool ConfigParameters::writeTbmParameterFile(int itbm, vector<pair<string, uint8_t> > vA, vector<uint8_t> tcA, vector<pair<string, uint8_t> > vB, vector<uint8_t> tcB) {

  vector<pair<string, uint8_t> > v;
  vector<uint8_t> tc;
  for (unsigned int ic = 0; ic < 2; ++ic) {
    stringstream fname;
    fname << fDirectory << "/" << fTbmParametersFileName << "_C" << itbm << (ic==0?"a":"b") << ".dat";

    ofstream OutputFile;
    OutputFile.open((fname.str()).c_str());
    if (!OutputFile.is_open()) {
      return false;
    } else {
      LOG(logINFO) << "write tbm parameters into " << (fname.str()).c_str();
    }

    if (0 == ic) {
      v = vA;
      tc = tcA;
    } else {
      v = vB;
      tc = tcB;
    }

    RegisterDictionary *a = RegisterDictionary::getInstance();
    for (vector<pair<string, uint8_t> >::iterator idac = v.begin(); idac != v.end(); ++idac) {
      OutputFile << right << setw(3) << setfill('0') << static_cast<int>(a->getRegister(idac->first, TBM_REG)) << " "
		 << idac->first
		 << "   0x" << setw(2) << setfill('0') << hex << static_cast<int>(idac->second)
		 << endl;
    }
    if (tc.size() > 0)
      OutputFile << right << setw(3) << setfill('0') << static_cast<int>(a->getRegister("nrocs1", TBM_REG)) << " "
                 << "nrocs1"
                 << "   0x" << setw(2) << setfill('0') << hex << static_cast<int>(tc.at(0))
                 << endl;
    if (tc.size() > 1)
      OutputFile << right << setw(3) << setfill('0') << static_cast<int>(a->getRegister("nrocs2", TBM_REG)) << " "
                 << "nrocs2"
                 << "   0x" << setw(2) << setfill('0') << hex << static_cast<int>(tc.at(1))
                 << endl;

    OutputFile.close();
  }
  return true;
}


// ----------------------------------------------------------------------
bool ConfigParameters::writeTbParameterFile() {
  string fname = fDirectory + "/" + getTBParameterFileName();
  ofstream OutputFile;
  string data;

  OutputFile.open(fname.c_str());
  if (!OutputFile.is_open()) {
    return false;
  } else {
    LOG(logINFO) << "write dtb parameters into " << fname.c_str();
  }

  RegisterDictionary *a = RegisterDictionary::getInstance();
  for (unsigned int idac = 0; idac < fTbParameters.size(); ++idac) {
    data = fTbParameters[idac].first;
    transform(data.begin(), data.end(), data.begin(), ::tolower);
    OutputFile << right << setw(3) << static_cast<int>(a->getRegister(fTbParameters[idac].first, DTB_REG)) << " "
	       << setw(15) << fTbParameters[idac].first << "  " << setw(3) << static_cast<int>(fTbParameters[idac].second)
	       << endl;
  }

  OutputFile.close();

  return true;
}

// ----------------------------------------------------------------------
bool ConfigParameters::writeTestParameterFile(string whichTest) {
  string bla = "no implementation for dumping test parameters " +  whichTest;
  LOG(logINFO) << bla;
  return true;
}


// ----------------------------------------------------------------------
void ConfigParameters::readGainPedestalParameters() {

  string bname = getGainPedestalParameterFileName();

  fGainPedestalParameters.clear();

  // -- read in file
  vector<string> lines;
  char  buffer[5000];
  ifstream is;
  vector<gainPedestalParameters> rocPar;

  stringstream firstFile, lastFile;
  firstFile << fDirectory << "/" << bname << fTrimVcalSuffix << "_C0" << ".dat";
  lastFile << fDirectory << "/" << bname << fTrimVcalSuffix << "_C" << fnRocs-1 << ".dat";
  LOG(logINFO) << "readGainPedestalParameters " << firstFile.str() << " .. " << lastFile.str();

  for (unsigned int iroc = 0; iroc < fnRocs; ++iroc) {
    lines.clear();
    rocPar.clear();
    stringstream fname;
    fname.str(string());
    fname << fDirectory << "/" << bname << fTrimVcalSuffix << "_C" << iroc << ".dat";
    is.open((fname.str()).c_str());
    if (!is.is_open()) {
      LOG(logERROR) << "cannot open " << (fname.str()) << " for reading PH calibration constants";
      return;
    }

    while (is.getline(buffer, 200, '\n')) {
      lines.push_back(string(buffer));
    }
    is.close();

    // -- parse lines
    double p0, p1, p2, p3;
    int icol, irow;
    string pix;

    for (unsigned int i = 3; i < lines.size(); ++i) {
      istringstream istring(lines[i]);
      istring >> p0 >> p1 >> p2 >> p3 >> pix >> icol >> irow;
      gainPedestalParameters a = {p0, p1, p2, p3};
      rocPar.push_back(a);
    }
    fGainPedestalParameters.push_back(rocPar);
  }
}

// ----------------------------------------------------------------------
void ConfigParameters::writeGainPedestalParameters() {

  stringstream fname;

  for (unsigned int iroc = 0; iroc < fGainPedestalParameters.size(); ++iroc) {
    fname.str(string());
    fname << fDirectory << "/" << getGainPedestalParameterFileName() << fTrimVcalSuffix << "_C" << iroc << ".dat";
    ofstream OutputFile;
    OutputFile.open((fname.str()).c_str());
    if (!OutputFile.is_open()) {
      LOG(logERROR) << "Could not open " << fname.str();
      return;
    } else {
      LOG(logINFO) << "write gain/ped parameters into " << fname.str();
    }

    OutputFile << "Parameters of the vcal vs. pulse height fits" << endl;
    //    OutputFile << "par[3] + par[2] * TMath::TanH(par[0]*x[0] - par[1])" << endl << endl;
    OutputFile << "par[3]*(TMath::Erf((x[0]-par[0])/par[1])+par[2])" << endl << endl;

    vector<gainPedestalParameters> pars = fGainPedestalParameters[iroc];
    for (unsigned ipix = 0; ipix < pars.size(); ++ipix) {
      OutputFile << scientific
		 << pars[ipix].p0 << " "
		 << pars[ipix].p1 << " "
		 << pars[ipix].p2 << " "
		 << pars[ipix].p3;
      OutputFile.unsetf(ios::fixed | ios::scientific);
      OutputFile << "     Pix "
		 << setw(2) << ipix/80 << " " << setw(2) << ipix%80
		 << endl;
    }
    OutputFile.close();
  }

}

// ----------------------------------------------------------------------
void ConfigParameters::setGainPedestalParameters(vector<vector<gainPedestalParameters> >v) {
  fGainPedestalParameters.clear();
  for (unsigned int i = 0; i < v.size(); ++i) {
    fGainPedestalParameters.push_back(v[i]);
  }
}

// ----------------------------------------------------------------------
vector<vector<gainPedestalParameters> > ConfigParameters::getGainPedestalParameters() {
  if (fGainPedestalParameters.size() == 0) {
    readGainPedestalParameters();
  }
  return fGainPedestalParameters;
}


void ConfigParameters::setProbe(string probe, string value) {

   transform(probe.begin(), probe.end(), probe.begin(), ::tolower);

   if (probe == "a1") fProbeA1 = value;
   else if (probe == "a2") fProbeA2 = value;
   else if (probe == "d1") fProbeD1 = value;
   else if (probe == "d2") fProbeD2 = value;
}

// ----------------------------------------------------------------------
string ConfigParameters::getProbe(string probe) {
   transform(probe.begin(), probe.end(), probe.begin(), ::tolower);
   if (probe == "a1") return fProbeA1;
   else if (probe == "a2") return fProbeA2;
   else if (probe == "d1") return fProbeD1;
   else if (probe == "d2") return fProbeD2;
   else return "";
}


// ----------------------------------------------------------------------
void ConfigParameters::cleanupString(string &s) {
  replaceAll(s, "\t", " ");
  string::size_type s1 = s.find("#");
  if (string::npos != s1) s.erase(s1);
  if (0 == s.length()) return;
  string::iterator new_end = unique(s.begin(), s.end(), bothAreSpaces);
  s.erase(new_end, s.end());
  if (s.substr(0, 1) == string(" ")) s.erase(0, 1);
  if (s.substr(s.length()-1, 1) == string(" ")) s.erase(s.length()-1, 1);
}

// ----------------------------------------------------------------------
bool ConfigParameters::bothAreSpaces(char lhs, char rhs) {
  return (lhs == rhs) && (lhs == ' ');
}

// ----------------------------------------------------------------------
void ConfigParameters::replaceAll(string& str, const string& from, const string& to) {
  if (from.empty()) return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

// ----------------------------------------------------------------------
void ConfigParameters::readNrocs(string line) {
  cleanupString(line);
  string::size_type s0 = line.find(" ");
  string nrocs = line.substr(s0);
  fnRocs = atoi(nrocs.c_str());
  string::size_type s1 = line.find("i2c:");
  if (string::npos == s1) {
    return;
  } else {
    string i2cstring = line.substr(s1+5);
    s0 = i2cstring.find(",");
    string i2c(""), leftover("");
    while (string::npos != s0) {
      i2c = i2cstring.substr(0, s0);
      fI2cAddresses.push_back(atoi(i2c.c_str()));
      i2cstring = i2cstring.substr(s0+1);
      s0 = i2cstring.find(",");
    }
    //  -- get the last one as well
    fI2cAddresses.push_back(atoi(i2cstring.c_str()));
    if (fnRocs != fI2cAddresses.size()) {
      LOG(logWARNING) << "mismatch between number of i2c addresses and nRocs! Resetting nRocs to "
		      <<  fI2cAddresses.size();
      fnRocs =  fI2cAddresses.size();
    }
  }
}

// ----------------------------------------------------------------------
void ConfigParameters::readHubIds(string line) {
  cleanupString(line);
  string::size_type s0 = line.find(" ");
  fHubIds.clear();
  if (string::npos == s0) {
    return;
  } else {
    string hubidstring = line.substr(6);
    s0 = hubidstring.find(",");
    string hubid("");
    while (string::npos != s0) {
      hubid = hubidstring.substr(0, s0);
      fHubIds.push_back(atoi(hubid.c_str()));
      hubidstring = hubidstring.substr(s0+1);
      s0 = hubidstring.find(",");
    }
    //  -- get the last one as well
    fHubIds.push_back(atoi(hubidstring.c_str()));
    if (fnTbms > 0 && fnTbms != fHubIds.size()) {
      LOG(logCRITICAL) << "We have " << static_cast<int>(fnTbms) << "TBMs, but "
              << static_cast<int>(fHubIds.size()) << " HubIds provided ";
      throw InvalidConfig("Mismatch between number of TBMs and HubIds");
    }
  }
}

// ----------------------------------------------------------------------
void ConfigParameters::readReadbackCal() {
  if (!fReadReadbackCal) {
    stringstream firstFile, lastFile;
    firstFile << fDirectory << "/" << fReadbackCalFileName << "_C" << 0 << ".dat";
    lastFile << fDirectory << "/" << fReadbackCalFileName << "_C" << fnRocs-1 << ".dat";
    LOG(logINFO) << "readReadbackCal: " << firstFile.str() << " .. " << lastFile.str();

    for (unsigned int i = 0; i < fnRocs; ++i) {
      stringstream filename;
      filename << fDirectory << "/" << fReadbackCalFileName << "_C" << i << ".dat";
      vector<pair<string, double> > rbCal = readReadbackFile(filename.str());
      fReadbackCal.push_back(rbCal);
    }
    fReadReadbackCal = true;
  }
}

// ----------------------------------------------------------------------
vector<pair<string, double> > ConfigParameters::readReadbackFile(string fname) {
  vector<pair<string, double> > rocRb;

  // -- read in file
  vector<string> lines;
  char  buffer[5000];
  ifstream is(fname.c_str());
  while (is.getline(buffer, 200, '\n')) {
    lines.push_back(string(buffer));
  }
  is.close();

  // -- parse lines
  double ival(0);
  double uval(0);
  //  unsigned char uval(0);

  string::size_type s1;
  string str1, str2;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    //cout << lines[i] << endl;
    // -- remove tabs, adjacent spaces, leading and trailing spaces
    cleanupString(lines[i]);
    if (lines[i].length() < 2) continue;
    s1 = lines[i].find(" ");
    str1 = lines[i].substr(0, s1);
    str2 = lines[i].substr(s1+1);

    transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
    ival = atof(str2.c_str());

    uval = ival;
    rocRb.push_back(make_pair(str1, uval));

  }

  return rocRb;
}

// ----------------------------------------------------------------------
vector<vector<pair<string, double> > > ConfigParameters::getReadbackCal() {
  if (!fReadReadbackCal) {
    readReadbackCal();
  }
  return  fReadbackCal;
}

// ----------------------------------------------------------------------
bool ConfigParameters::writeReadbackFile(int iroc, vector<pair<string, double> > v) {

  stringstream fname;
  fname << fDirectory << "/" << fReadbackCalFileName << "_C" << iroc << ".dat";

  ofstream OutputFile;
  OutputFile.open((fname.str()).c_str());
  if (!OutputFile.is_open()) {
    return false;
  } else {
    LOG(logINFO) << "write readback calibration parameters into " << fname.str();
  }

  //  RegisterDictionary *a = RegisterDictionary::getInstance();
  for (vector<pair<string,double> >::iterator idac = v.begin(); idac != v.end(); ++idac) {
    OutputFile << left << setw(3) << idac->first << " " << setw(3) << static_cast<double>(idac->second)
	       << endl;
  }

  OutputFile.close();
  return true;
}


// ----------------------------------------------------------------------
void ConfigParameters::setTrimVcalSuffix(string name, bool nocheck) {

  if (nocheck) {
    fTrimVcalSuffix = name;
    return;
  }

  stringstream fname;

  fname << fDirectory << "/" << fTrimParametersFileName << name << "_C0.dat";
  ifstream InputFile;
  InputFile.open((fname.str()).c_str());
  //  cout << "check for " << fname.str() << endl;

  if (!InputFile.is_open()) {
    LOG(logERROR) << "Did not find file " << fname.str() << ", no trim VCAL value used";
    fTrimVcalSuffix = "";
  } else {
    fTrimVcalSuffix = name;
  }
  InputFile.close();
}


// ----------------------------------------------------------------------
bool ConfigParameters::isMaskedPixel(int roc, int col, int row) {
  vector<pair<int, int> > v = fMaskedPixels[roc];
  for (unsigned int j = 0; j < v.size(); ++j) {
    if (v[j].first == col && v[j].second == row) {
      return true;
    }
  }
  return false;
}
