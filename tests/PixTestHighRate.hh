#ifndef PIXTESTHIGHRATE_H
#define PIXTESTHIGHRATE_H

#include "PixTest.hh"
#include "PHCalibration.hh"

#include <TProfile2D.h>


class DLLEXPORT PixTestHighRate: public PixTest {
public:
  PixTestHighRate(PixSetup *, std::string);
  PixTestHighRate();
  virtual ~PixTestHighRate();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  std::string toolTip(std::string what);
  void bookHist(std::string);

  void runCommand(std::string command);
  void doTest();
  void doCalDelScan();
  void doXPixelAlive();
  void doXNoiseMaps();
  void doRunDaq();
  void doRunMaskHotPixels();
  void doRunTrimHotPixels();
  void doStop();

  void doHitMap(int nseconds, std::vector<TH2D*>);
  void fillMap(std::vector<TH2D*>);

private:

  int           fParTriggerFrequency;
  int           fParRunSeconds;
  int           fParTriggerDelay;
  bool          fParFillTree;
  bool	        fParDelayTBM;
  uint16_t      fParNtrig;
  int           fParVcal, fParDacLo, fParDacHi, fParDacsPerStep;

  std::string   fParMaskFileName;
  int           fParSaveMaskedPixels;

  bool          fPhCalOK;
  PHCalibration fPhCal;

  bool    fParSaveTrimbits;
  bool    fParMaskUntrimmable;
  int     fParRunSecondsHotPixels;
  int     fParTrimHotPixelThr;

  bool          fDaq_loop;

  std::vector<TH2D*> fHitMap;

  ClassDef(PixTestHighRate, 1)

};
#endif
