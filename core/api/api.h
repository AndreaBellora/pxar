/**
 * pxar API class header
 * to be included by any executable linking to libpxar
 */

#ifndef PXAR_API_H
#define PXAR_API_H

/** Declare all classes that need to be included in shared libraries on Windows
 *  as class DLLEXPORT className
 */
#include "pxardllexport.h"

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef int int32_t;
typedef short int int16_t;
typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif


#include <string>
#include <vector>
#include <map>
#include "datatypes.h"
#include "exceptions.h"
#include "dut.h"

// PXAR Flags

/** Flag to force the API loop expansion to do tests ROC-by-ROC instead of looping over
 *  the full module in one go.
 */
#define FLAG_FORCE_SERIAL  0x0001

/** Flag to send the calibration pulses through the sensor pad instead of directly
 *  to the preamplifier.
 */
#define FLAG_CALS          0x0002

/** Flag to enable cross-talk measurements by pulsing the neighbor row instead of the
 *  row to be measured.
 */
#define FLAG_XTALK         0x0004

/** Flag to define the threshold edge (falling/rising edge) for scans.
 */
#define FLAG_RISING_EDGE   0x0008

/** Flag to force all pixels to masked state during tests. By this only the one pixel with
 *  calibrate pulse is enabled and trimmed.
 *  This flag is obsolete (since this is now the default behavior) and stays only for legacy
 *  reasons.
 */
#define FLAG_FORCE_MASKED   0x0010

/** Flag to desable the standard procedure of flipping DAC values before programming. This
 *  is done by default to flatten the response, taking into account the chip layout. This
 *  is implemented as NIOS lookup table.
 */
#define FLAG_DISABLE_DACCAL 0x0020

/** Flag to disable sorting of the API output data. This flag should only be used in specific
 *  cases requiring the original order of the data read out from the DUT.
 */
#define FLAG_NOSORT 0x0040

/** Flag to check the order in which the pixels appear in the raw event readout. Pixels appearing
 *  in a wrong position (e.g. expecting pixel 13,8 but receiving pixel 13,9) are flagged with a
 *  negative pulse height. This flag can be used e.g. for pixel address test to make sure all of them
 *  are answering with their correct address.
 *  When running with FLAG_FORCE_UNMASKED all noise or background pixel hits will be flagged as such
 *  by setting anegative pulse height. This allows separation into calibrate hit and noise hit maps.
 */
#define FLAG_CHECK_ORDER 0x0080

/** Flag to unmask and trim all pixels before the test starts. This might be a tad faster but one
 *  risks to get flooded with noise hits - which also might distrub the readout.
 */
#define FLAG_FORCE_UNMASKED   0x0100

/** Flag to dump all flawed events plus the surrounding ones to the logger on ERROR level.
 *  Only use this to debug readout since it slows down decoding considerably.
 */
#define FLAG_DUMP_FLAWED_EVENTS 0x0200

/** Flag to disable the collection of readback data from ROC headers. This might be desirable when sending
 *  several million triggers at once since the amount of readback data collected can be huge.
 */
#define FLAG_DISABLE_READBACK_COLLECTION 0x0400

/** Flag to disable cross-checking the TBM event id against a local counter.
 */
#define FLAG_DISABLE_EVENTID_CHECK 0x0800

/** Flag to enable the collection of XOR sums from the DESER400 modules for every event.
 */
#define FLAG_ENABLE_XORSUM_LOGGING 0x1000


/** Define a macro for calls to member functions through pointers
 *  to member functions (used in the loop expansion routines).
 *  Follows advice of http://www.parashift.com/c++-faq/macro-for-ptr-to-memfn.html
 */
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

namespace pxar {

  /** Forward declaration, implementation follows below...
   */
  class dut;

  /** Forward declaration, not including the header file!
   */
  class hal;


  /** Define typedefs to allow easy passing of member function
   *  addresses from the HAL class, used e.g. in loop expansion routines.
   *  Follows advice of http://www.parashift.com/c++-faq/typedef-for-ptr-to-memfn.html
   */
  typedef  std::vector<Event> (hal::*HalMemFnRocParallel)(std::vector<uint8_t> rocids, bool efficiency, std::vector<int32_t> parameter);
  typedef  std::vector<Event> (hal::*HalMemFnPixelParallel)(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter);
  typedef  std::vector<Event> (hal::*HalMemFnRocSerial)(uint8_t rocid, bool efficiency, std::vector<int32_t> parameter);
  typedef  std::vector<Event> (hal::*HalMemFnPixelSerial)(uint8_t rocid, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter);



  /** pxar API class definition
   *
   *  this is the central API through which all calls from tests and user space
   *  functions have to be routed in order to interact with the hardware.
   *
   *  The API level aims to provide a set of high-level function from which
   *  the "user" (or test implementation) can choose. This approach allows
   *  to hide hardware specific functions and calls from the user space code
   *  and automatize e.g. startup procedures.
   *
   *  All input from user space is checked before programming it to the
   *  devices. Register addresses have an internal lookup mechanism so the
   *  user only hast to provide e.g. the DAC name to be programmed as a std::string.
   *
   *  Unless otherwise specified (some DAQ functions allow this) all data
   *  returned from API functions is fully decoded and stored in C++ structures
   *  using std::vectors and std::pairs to ease its handling. Most functions will
   *  return a std::vector containing pxar::pixel elements storing the readout data.
   *
   *  Another concept implemented is the Device Under Test (DUT) which is a
   *  class (pxar::dut) representing the attached hardware to be tested. In order to change
   *  its configuration the user space code interacts with the _dut object and
   *  alters its settings. This is programmed into the devices automatically
   *  before the next test is executed. This approach allows both the efficient
   *  execution of many RPC calls at once and reading back the actual device
   *  configuration at any time during the tests.
   *
   *  Calls to test functions are automatically expanded in a way that they
   *  cover the full device in the most efficient way available. Instead of
   *  scanning 4160 pixels after another the code will select the function
   *  to scan a full ROC in one go automatically.
   */
  class DLLEXPORT pxarCore {

  public:

    /** Default constructor for the libpxar API
     *
     *  Fetches a new pxar::hal instance and opens the connection to the testboard
     *  specified in the "usbId" parameter. An asterisk as "usbId" acts as
     *  wildcard. If only one DTB is connected the algorithm will automatically
     *  connect to this board, if several are connected it will give a warning.
     *
     *  On issues with the USB connection, a pxar::UsbConnectionError is thrown.
     *  If the firmware on the DTB does not match the expected version for pxar,
     *  a pxar::FirmwareVersionMismatch exception is thrown.
     *
     */
      pxarCore(std::string usbId = "*", std::string logLevel = "WARNING", bool doDaq_MemReset = true);

    /** Default destructor for libpxar API
     *
     *  Will power down the DTB, disconnect properly from the testboard,
     *  and destroy the pxar::hal object.
     */
    ~pxarCore();

    /** Returns the version string for the pxar API.
     *
     *  When using a git checkout the version number will be calculated at
     *  compile time from the latest tagged version plus the number of commits
     *  on top of that. In this case the version number also contains the
     *  commit hash of the latest commit for reference.
     *
     *  In case of a tarball install the version number is hardcoded in the
     *  CMakeLists.txt file.
     */
    std::string getVersion();

    /** Initializer method for the testboard
     *
     *  Initializes the testboard with signal delay settings, and voltage/current
     *  limit settings (power_settings) and the initial pattern generator setup
     *  (pg_setup), all provided via vectors of pairs with descriptive name.
     *  The name lookup is performed via the central API dictionaries.
     *  Multiple pattern generator signals at once can be sent by separating their
     *  names with a semicolon.
     *
     *  All user inputs are checked for sanity. This includes range checks on
     *  the current limits set, a sanity check for the pattern generator command
     *  list (including a check for delay = 0 at the end of the list).
     *  If the settings are found to be out-of-range, a pxar::InvalidConfig
     *  exception is thrown.
     *
     *  In case of USB communication problems, pxar::UsbConnectionError is thrown.
     */
    bool initTestboard(std::vector<std::pair<std::string,uint8_t> > sig_delays,
                       std::vector<std::pair<std::string,double> > power_settings,
                       std::vector<std::pair<std::string, uint8_t> > pg_setup);

    /** Update method for testboard voltages and current limits. This method requires
     *  the testboard to be initialized once using pxar::initTestboard
     *  All user inputs are checked for sanity. This includes range checks on
     *  the current limits set. If the settings are found to be out-of-range,
     *  a pxar::InvalidConfig exception is thrown.
     *  The new settings are stored in the pxar::dut object for later reference.
     */
    void setTestboardPower(std::vector<std::pair<std::string,double> > power_settings);

    /** Update method for testboard signal delay settings. This method requires
     *  the testboard to be initialized once using pxar::initTestboard
     *  All user inputs are checked for sanity. This includes a register name lookup
     *  and range checks on the register values. If the settings are found to be
     *  out-of-range, a pxar::InvalidConfig exception is thrown.
     *  The new settings are stored in the pxar::dut object for later reference.
     */
    void setTestboardDelays(std::vector<std::pair<std::string,uint8_t> > sig_delays);

    /** Update method for testboard pattern generator. This method requires
     *  the testboard to be initialized once using pxar::initTestboard
     *  All user inputs are checked for sanity. This includes a full sanity check
     *  for the pattern generator command list, including a check for the total
     *  pattern length supplied as well as the delay = 0 at the end of the list.
     *  If the settings are found to be out-of-range, a pxar::InvalidConfig exception
     *  is thrown. The new settings are stored in the pxar::dut object for
     *  later reference.
     *  Multiple pattern generator signals at once can be sent by separating their
     *  names with a semicolon, e.g. "token;sync".
     */
    void setPatternGenerator(std::vector<std::pair<std::string, uint8_t> > pg_setup);

    /** Initializer method for the DUT (attached devices)
     *
     *  This function requires the types and DAC settings for all TBMs
     *  and ROCs contained in the setup. All values will be checked
     *  for validity (DAC ranges, position and number of pixels, etc.)
     *  and an pxar::InvalidConfig exception is thrown if any errors are
     *  encountered.
     *
     *  All parameters are supplied via vectors, the size of the vector
     *  represents the number of devices. DAC names and device types should be
     *  provided as strings. The respective register addresses will be looked up
     *  internally. Strings are checked case-insensitive, old and new DAC names
     *  are both supported.
     *
     *  In case of USB communication problems, pxar::UsbConnectionError is thrown.
     */
    bool initDUT(std::vector<uint8_t> hubIds,
		 std::string tbmtype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		 std::string roctype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		 std::vector<std::vector<pixelConfig> > rocPixels,
		 std::vector<uint8_t> rocI2Cs);

    /** Alternative initializer method for the DUT (attached devices).
     *
     *  As above, but automatically assumes consecutively numbered I2C addresses for
     *  all attached ROCs, starting from zero.
     */
    bool initDUT(std::vector<uint8_t> hubids,
		 std::string tbmtype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		 std::string roctype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		 std::vector<std::vector<pixelConfig> > rocPixels);

    /** Alternative initializer method for the DUT (attached devices).
     *
     *  As above, but only accepts one hub id for a single physical TBM
     */
    bool initDUT(uint8_t hubid,
		 std::string tbmtype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		 std::string roctype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		 std::vector<std::vector<pixelConfig> > rocPixels,
		 std::vector<uint8_t> rocI2Cs);

    /** Alternative initializer method for the DUT (attached devices).
     *
     *  As above, but automatically assumes consecutively numbered I2C addresses for
     *  all attached ROCs, starting from zero and only accepts one hub id for a
     *  single physical TBM.
     */
    bool initDUT(uint8_t hubid,
		 std::string tbmtype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		 std::string roctype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		 std::vector<std::vector<pixelConfig> > rocPixels);

    /** Programming method for the DUT (attached devices)
     *
     *  This function requires the DUT structure to be filled (initialized).
     *
     *  All parameters are taken from the DUT struct, the enabled devices are
     *  programmed. This function needs to be called after power cycling the
     *  testboard output (using Poff, Pon).
     *
     *  A DUT flag is set which prevents test functions to be executed if
     *  not programmed.
     */
    bool programDUT();


    // DTB functions

    /** Function to flash a new firmware onto the DTB via the USB connection.
     */
    bool flashTB(std::string filename);

    /** Function to read out analog DUT supply current on the testboard
     *  The current will be returned in SI units of Ampere
     */
    double getTBia();

    /** Function to read out analog DUT supply voltage on the testboard
     *  The voltage will be returned in SI units of Volts
     */
    double getTBva();

    /** Function to read out digital DUT supply current on the testboard
     *  The current will be returned in SI units of Ampere
     */
    double getTBid();

    /** Function to read out digital DUT supply voltage on the testboard
     *  The voltage will be returned in SI units of Volts
     */
    double getTBvd();

    /** turn off HV
     */
    void HVoff();

    /** turn on HV
     */
    void HVon();

    /** turn on power
     */
    void Pon();

    /** turn off power
     */
    void Poff();

     /** Selects "signal" as output for the DTB probe channel "probe"
      *  (digital or analog). Valid probe values are the names written
      *  on the case of the DTB, next to the respective outputs.
      *
      *  The signal identifier is checked against a dictionary to be valid.
      *  In case of an invalid signal identifier the output is turned off.
      *
      *  The channel variable allows to select a certain channel of the
      *  signal to be selected of applicable.
      */
    bool SignalProbe(std::string probe, std::string name, uint8_t channel = 0);

    std::vector<uint16_t> daqADC(std::string signal, uint8_t gain, uint16_t nSample, uint8_t source, uint8_t start);

    // TEST functions

    /** Set a DAC value on the DUT for one specific ROC
     *
     *  The "rocID" parameter can be used to select a specific ROC to program.
     *  The ROC is identified by its ID (counting all ROCs up from 0).
     *
     *  This function will both update the bookkeeping value in the pxar::dut
     *  struct and program the actual device.
     */
    bool setDAC(std::string dacName, uint8_t dacValue, uint8_t rocID);

    /** Set a DAC value on the DUT for all enabled ROC
     *
     *  This function will both update the bookkeeping value in the pxar::dut
     *  struct and program the actual device.
     */
    bool setDAC(std::string dacName, uint8_t dacValue);

    /** Set the Vcal range to high, leaving the other bits in ctrlreg untouched
     *
     *  This function will both update the bookkeeping value in the pxar::dut
     *  struct and program the actual device.
     */
    void setVcalHighRange();

    /** Set the Vcal range to low, leaving the other bits in ctrlreg untouched
     *
     *  This function will both update the bookkeeping value in the pxar::dut
     *  struct and program the actual device.
     */
    void setVcalLowRange();


    /** Get the valid range of a given DAC
     */
    uint8_t getDACRange(std::string dacName);

    /** Set a register value on a specific TBM of the DUT
     *
     *  The "tbmid" parameter can be used to select a specific TBM to program.
     *  This function will both update the bookkeeping value in the pxar::dut
     *  struct and program the actual device.
     *
     *  This function will set the respective register in the TBM core specified
     *  by the "tbmid". Be aware of the fact that TBM Alpha cores are numbered
     *  with even IDs (0,2,...) and TBM Beta cores with odd IDs (1,3,...).
     */
    bool setTbmReg(std::string regName, uint8_t regValue, uint8_t tbmid);

    /** Set a register value on all TBMs of the DUT
     *
     *  This function will both update the bookkeeping value in the pxar::dut
     *  struct and program the actual device.
     *
     *  This function will set the respective register in the TBM core specified
     *  by the "tbmid". Be aware of the fact that TBM Alpha cores are numbered
     *  with even IDs (0,2,...) and TBM Beta cores with odd IDs (1,3,...).
     */
    bool setTbmReg(std::string regName, uint8_t regValue);

    /** Select the RDA channel of a layer 1 module for tbm readback
    */
    void selectTbmRDA(uint8_t tbmid);

    /** Manually set the HUB_address in the testboard
     */
    void setHubID(uint8_t id);

    /** Method to scan a DAC range and measure the pulse height
     *
     *  Returns a vector of pairs containing set dac value and a pxar::pixel vector,
     *  with the value of the pxar::pixel struct being the averaged pulse height
     *  over "nTriggers" triggers.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers);

    /** Method to scan a DAC range and measure the pulse height
     *
     *  Returns a vector of pairs containing set dac value and a pxar::pixel vector,
     *  with the value of the pxar::pixel struct being the averaged pulse height
     *  over "nTriggers" triggers.
     *  The dacStep parameter can be used to set the increment of the DAC scan, i.e.
     *  only sparsely scanning every nth DAC.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getPulseheightVsDAC(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTrigger);


    /** Method to scan a DAC range and measure the efficiency
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pxar::pixel struct being the number of hits in that
     *  pixel. Efficiency == 1 for nhits == nTriggers
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers);

    /** Method to scan a DAC range and measure the efficiency
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pxar::pixel struct being the number of hits in that
     *  pixel. Efficiency == 1 for nhits == nTriggers
     *  The dacStep parameter can be used to set the increment of the DAC scan, i.e.
     *  only sparsely scanning every nth DAC.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getEfficiencyVsDAC(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a DAC range and measure the pixel threshold
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pxar::pixel struct being the threshold value of that
     *  pixel.
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dacName, std::string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a DAC range and measure the pixel threshold
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pxar::pixel struct being the threshold value of that
     *  pixel.
     *
     *  This function allows to specify a range for the threshold DAC to be searched,
     *  this can be used to speed up the procedure by limiting the range.
     *  The dacstep parameters can be used to set the increments of the 2D DAC scan, i.e.
     *  only sparsely scanning every nth DAC. The threshold value returned is the lower
     *  bound (upper bound for FLAG_RISING_EDGE) of the interval in which the true
     *  threshold is found. The increment can be set independently for both scanning dimensions.
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a DAC range and measure the pixel threshold
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pxar::pixel struct being the threshold value of that
     *  pixel.
     *
     *  This function allows to specify a range for the threshold DAC to be searched,
     *  this can be used to speed up the procedure by limiting the range.
     *  The dacstep parameters can be used to set the increments of the 2D DAC scan, i.e.
     *  only sparsely scanning every nth DAC. The threshold value returned is the lower
     *  bound (upper bound for FLAG_RISING_EDGE) of the interval in which the true
     *  threshold is found. The increment can be set independently for both scanning dimensions.
     *
     *  The threshold can be adjusted to a percentage of efficienciy (i.e. threshold = 50 is the 50% efficiency
     *  niveau of the pixel).
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint8_t threshold, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the
     *  pulse height
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pxar::pixel vector. The value of the pxar::pixel struct is the
     *  averaged pulse height over "nTriggers" triggers.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the
     *  pulse height
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pxar::pixel vector. The value of the pxar::pixel struct is the
     *  averaged pulse height over "nTriggers" triggers.
     *  The dacStep parameters can be used to set the increment of the DAC scan, i.e.
     *  only sparsely scanning every nth DAC. Increment can be set independently
     *  for both scanning dimensions.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the efficiency
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pxar::pixel vector. The value of the pxar::pixel struct is the
     *  number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers);


    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the efficiency
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pxar::pixel vector. The value of the pxar::pixel struct is the
     *  number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     *  The dacStep parameters can be used to set the increment of the DAC scan, i.e.
     *  only sparsely scanning every nth DAC. Increment can be set independently
     *  for both scanning dimensions.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers);


    /** Method to get a map of the pulse height
     *
     *  Returns a vector of pixels, with the value of the pxar::pixel struct being
     *  the averaged pulse height over "nTriggers" triggers
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector<pixel> getPulseheightMap(uint16_t flags, uint16_t nTriggers);

    /** Method to get a map of the efficiency
     *
     *  Returns a vector of pixels, with the value of the pxar::pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector<pixel> getEfficiencyMap(uint16_t flags, uint16_t nTriggers);

    /** Method to get a map of the pixel threshold
     *
     *  Returns a vector of pixels, with the value of the pxar::pixel struct being
     *  the threshold value of that pixel.
     *
     *  This function allows to specify a range for the threshold DAC to be searched,
     *  this can be used to speed up the procedure by limiting the range.
     *  The dacStep parameter can be used to set the increment of the DAC scan, i.e.
     *  only sparsely scanning every nth DAC. The threshold value returned is the lower
     *  bound (upper bound for FLAG_RISING_EDGE) of the interval in which the true
     *  threshold is found.
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector<pixel> getThresholdMap(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers);

    /** Method to get a map of the pixel threshold
     *
     *  Returns a vector of pixels, with the value of the pxar::pixel struct being
     *  the threshold value of that pixel.
     *
     *  This function allows to specify a range for the threshold DAC to be searched,
     *  this can be used to speed up the procedure by limiting the range.
     *  The dacStep parameter can be used to set the increment of the DAC scan, i.e.
     *  only sparsely scanning every nth DAC. The threshold value returned is the lower
     *  bound (upper bound for FLAG_RISING_EDGE) of the interval in which the true
     *  threshold is found.
     *
     *  The threshold can be adjusted to a percentage of efficienciy (i.e. threshold = 50 is the 50% efficiency
     *  niveau of the pixel).
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector<pixel> getThresholdMap(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint8_t threshold, uint16_t flags, uint16_t nTriggers);

    /** Method to get a map of the pixel threshold
     *
     *  Returns a vector of pixels, with the value of the pxar::pixel struct being
     *  the threshold value of that pixel.
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel.
     *
     *  If the readout of the DTB is corrupt, a pxar::DataMissingEvent is thrown.
     *
     */
    std::vector<pixel> getThresholdMap(std::string dacName, uint16_t flags, uint16_t nTriggers);

    /** Enable or disable the external clock source of the DTB.
     *  This function will return "false" if no external clock is present,
     *  clock is then left on internal.
     */
    bool setExternalClock(bool enable);

    /** Set the clock stretch.
	FIXME missing documentation
	A width of 0 disables the clock stretch
     */
    void setClockStretch(uint8_t src, uint16_t delay, uint16_t width);

    /** Set Signal Mode.
	Define any testboard signal (e.g. clk) to the state constant high, constant low,
	or normal oscillation.
	signal: "clk", "ctr", "sda", or "tin"
	mode: 0 (normal), 1 (low), 2 (high) or 3 (random)
	When selecting (random) the speed has to be specified.
     */
    void setSignalMode(std::string signal, uint8_t mode, uint8_t speed = 0);

    /** Set Signal Mode.
	Define any testboard signal (e.g. clk) to the state constant high, constant low,
	or normal oscillation.
	signal: "clk", "ctr", "sda", or "tin"
	mode: "normal", "low", "high" or "random"
	speed: only necessary for the "random" mode
     */
    void setSignalMode(std::string signal, std::string mode, uint8_t speed = 0);

    // DAQ functions

    /** Function to set up and initialize a new data acquisition session (DAQ).
     *  This function also programs all attached devices. Pixel configurations
     *  which are changed after calling this function will not be written to
     *  the devices, so make sure to mask/unmask and set test bits for all
     *  pixels in question before calling pxar::daqStart()!
     */
    bool daqStart();
    bool daqStart(const uint16_t flags);
    bool daqStart(const int bufsize, const bool init);
    bool daqStart(const uint16_t flags, const int bufsize, const bool init);

    /** Function used for the Deser160 phase scan in PixTestSetup
     *  Sending Ntrig triggers and reading back the raw data
     *  only 10 bytes per trigger. The rest would be corrupted data anyway
     */

    std::vector<rawEvent> Deser160PhaseScan(int Ntrig);

    /** Function to get back the DAQ status
     *
     *  For a running DAQ with free buffer memory left, this function returns
     *  TRUE. In case of a problem with the DAQ (not started, buffer overflow
     *  or full...) it returns FALSE.
     */
    bool daqStatus();

    /** Same Function as pxar::daqStatus() but provides additional parameter
     *  (pass-by-reference) to inform about the current fill status of the
     *  DAQ buffer in percent.
     */
    bool daqStatus(uint8_t & perFull);

    /** Function to select the trigger source for the DAQ
     *
     *  Select Pattern Generator, internal random/cyclic trigger or external
     *  synchronous or asynchronous trigger sources. The trigger source is
     *  looked up from the dictionary, and the corresponding decoding module
     *  is automatically selected.
     *
     *  The optional "timing" parameter is required for the DTB trigger generator
     *  (random, periodic) and represents the trigger period in BC.
     *
     *  The trigger is only activated at pxar::daqStart() and is stopped again at
     *  pxar::daqStop().
     */
    bool daqTriggerSource(std::string triggerSource, uint32_t rate = 0);

    /** Function to send a single (direct) signal to the DUT.
     *
     *  This first configures the trigger source for direct sending of single
     *  signals, routes the signal to the DUT and then resets the trigger source
     *  to the one previously configured. The possible trigger signals are matched
     *  against the pattern dictionary.
     */
    bool daqSingleSignal(std::string triggerSignal);

    /** Function to read out the earliest pxar::Event in buffer from the current
     *  data acquisition session.
     *
     *  This function can throw a pxar::DataDecodingError exception in case severe
     *  problems were encountered during the readout.
     *
     *  If no event is available the function will throw a pxar::DataNoEvent
     *  exception. Catching this allows constant polling for new events.
     */
    Event daqGetEvent();

    /** Function to read out the earliest raw data record in buffer from the
     *  current data acquisition session.
     *
     *  If no event is available the function will throw a pxar::DataNoEvent
     *  exception. Catching this allows constant polling for new events.
     */
    rawEvent daqGetRawEvent();

    /** Function to fire the previously defined pattern command list "nTrig"
     *  times, the function parameter defaults to 1.
     *  The function returns the triggering period actually used after cross-check
     *  with the pattern generator cycle length.
     */
    uint16_t daqTrigger(uint32_t nTrig = 1, uint16_t period = 0);

    /** Function to fire the previously defined pattern command list
     *  continuously every "period" clock cycles (default: 1000)
     *  The function returns the triggering period actually used after cross-check
     *  with the pattern generator cycle length.
     */
    uint16_t daqTriggerLoop(uint16_t period = 1000);

    /** Function to halt the pattern generator loop which has been started
     *  using daqTriggerLoop(). This stops triggering the devices.
     */
    void daqTriggerLoopHalt();

    /** Function to stop the running data acquisition
     */
    bool daqStop();
    bool daqStop(const bool init);

    /** Function to return the full currently available raw event buffer from
     *  the testboard RAM. No decoding is performed, the data stream is just
     *  split into single pxar::rawEvent objects. This function returns the
     *  raw events from either of the deserializer modules.
     *
     *  If no raw events are available the function will throw a pxar::DataNoEvent
     *  exception. Catching this allows constant polling for new events.
     */
    std::vector<rawEvent> daqGetRawEventBuffer();

    /** Function to return the full currently available raw data buffer from the
     *  testboard RAM. Neither decoding nor splitting is performed, this function
     *  returns the raw data blob from either of the deserializer modules.
     *
     *  If no data is available the function will throw a pxar::DataNoEvent
     *  exception. Catching this allows constant polling for new data.
     */
    std::vector<uint16_t> daqGetBuffer();

    /** Function to return the full currently available pxar::Event buffer from the
     *  testboard RAM. All data is decoded and the function returns decoded pixels
     *  separated in pxar::Events with additional header information available.
     *
     *  This function can throw a pxar::DataDecodingError exception in case severe
     *  problems were encountered during the readout.
     *
     *  If no events are available the function will throw a pxar::DataNoEvent
     *  exception. Catching this allows constant polling for new events.
     */
    std::vector<Event> daqGetEventBuffer();

    /** Function to return the full currently available ROC slow readback value
     *  buffer. The data is stored until a new DAQ session or test is called and
     *  can be fetched once (deleted at read time). The return vector contains
     *  one vector of readback values for every ROC found in the readout chain.
     */
    std::vector<std::vector<uint16_t> > daqGetReadback();

    /** Function to return the recorded XOR sum of the DESER400 module for the
     *  selected channel. The data is stored until a new DAQ session or test is
     *  called and can be fetched once (deleted at read time). The return vector
     *  contains all recorded XOR sum values of the DAQ channel.
     */
    std::vector<uint8_t> daqGetXORsum(uint8_t channel);

    /** Function that returns a class object of the type pxar::statistics
     *  containing all collected error statistics from the last (non-raw)
     *  DAQ readout or API test call. Statistics can be fetched once and
     *  are then reset.
     *
     *  It is meant for one-time-reading which means after calling
     *  pxarCore::getStatistics() the object will be returned and the internal
     *  statistics reset. Calling getStatistics() a second time will yield an
     *  empty object. This implies that if you want to check different values
     *  of the struct you have to fetch it and store it locally before
     *  accessing class members such as statistics::pixel_errors() instead of
     *  calling pxarCore::getStatistics().pixel_errors().
     *
     *  Note that the statistics class object gets filled by the decoder
     *  pipeworks. This has an implication for people running their own DAQ
     *  sessions using pxarCore::daqStart() and pxarCore::daqStop().
     *  If you fetch your data via pxarCore::daqGetRawEvent() or
     *  pxarCore::daqGetRawEventBuffer() the statistics object will remain empty
     *  because the data has not been passed through the decoder. If you run
     *  pxarCore::daqGetEvent() or pxarCore::daqGetEventBuffer() the statistics
     *  object will be properly filled. Events will continue to be added to
     *  these numbers until you either read them out (reading statistics resets
     *  the counters) or you re-started a new DAQ session (pxarCore::daqStart()
     *  initialises the counters to zero).
     */
    statistics getStatistics();

    /** DUT object for book keeping of settings
     */
    dut * _dut;

    /** Status function for the API
     *
     *  Returns true if everything is setup correctly for operation
     */
    bool status();

    /** Get ADC value
     */
    uint16_t GetADC( uint8_t rpc_par1 ) ;

    void setReportingLevel(std::string logLevel);

    std::string getReportingLevel();

  private:

    /** Private HAL object for the API to access hardware routines
     */
    hal * _hal;

    /** Routine to loop over all active ROCs/pixels and call the
     *  appropriate pixel, ROC or module HAL methods for execution.
     *
     *  This provides the central functionality of the DUT concept, expandLoop
     *  will check for the most efficient way to carry out a test requested by
     *  the user, i.e. select the full-ROC test instead of the pixel-by-pixel
     *  function, all depending on the configuration of the DUT.
     */
    std::vector<Event> expandLoop(HalMemFnPixelSerial pixelfn, HalMemFnPixelParallel multipixelfn, HalMemFnRocSerial rocfn, HalMemFnRocParallel multirocfn, std::vector<int32_t> param, bool efficiency, uint16_t flags = 0);


    /** Repacks map data from (possibly) several ROCs into one long vector
     *  of pixels.
     */
    std::vector<pixel> repackMapData (std::vector<Event> &data, uint16_t flags);

    /** Repacks map data from (possibly) several ROCs into one long vector
     *  of pixels and returns the threshold value.
     */
    std::vector<pixel> repackThresholdMapData (std::vector<Event> &data, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint8_t thresholdlevel, uint16_t nTriggers, uint16_t flags);


    /** Repacks DAC scan data into pairs of DAC values with fired pxar::pixel vectors.
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > repackDacScanData (std::vector<Event> &data, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags);


    /** Repacks DAC scan data into pairs of DAC values with fired pxar::pixel vectors and return the threshold value.
     */
    std::vector<std::pair<uint8_t,std::vector<pixel> > > repackThresholdDacScanData (std::vector<Event> &data, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint8_t thresholdlevel, uint16_t nTriggers, uint16_t flags);


    /** repacks (2D) DAC-DAC scan data into pairs of DAC values with
     *  vectors of the fired pixels.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > repackDacDacScanData (std::vector<Event> &data, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags);


    /** Helper function for conversion from string to register value
     *
     *  Type tells it whether it is a DTB, TBM or ROC register to look for.
     *  This function also performs a range check for DAC and testboard register
     *  values. This function return the value itself if it is within the valid
     *  range or upper/lower range boundary in case of over/underflow.
     */
    bool verifyRegister(std::string name, uint8_t &id, uint8_t &value, uint8_t type);


    /** Helper function for conversion from device type string to code
     */
    uint8_t stringToDeviceCode(std::string name);


    /** Routine to loop over all ROCs/pixels and update the NIOS cache of trim
     *  and mask bits with the current test configuration. This cache is used
     *  for the trimming done on the test trigger loops unless FLAG_FORCE_UNMASKED
     *  is activated.
     */
    void MaskAndTrimNIOS();

    /** Routine to loop over all ROCs/pixels and figure out the most efficient
     *  way to (un)mask and trim them for the upcoming test according to the
     *  information stored in the DUT struct.
     *
     *  This function programs all attached devices with the needed trimming and
     *  masking bits for the Pixel Unit Cells (PUC). It sets both the needed PUC
     *  mask&trim bits and the DCOL enable bits. If the bool parameter "trim" is
     *  set to "false" it will just mask all ROCs.
     */
    void MaskAndTrim(bool trim);

    /** Routine to select one ROCs with all pixels and figure out the most efficient
     *  way to (un)mask and trim them for the upcoming test according to the
     *  information stored in the DUT struct.
     *
     *  This function programs all attached devices with the needed trimming and
     *  masking bits for the Pixel Unit Cells (PUC). It sets both the needed PUC
     *  mask&trim bits and the DCOL enable bits. If the bool parameter "trim" is
     *  set to "false" it will just mask all ROCs.
     */
    void MaskAndTrim(bool trim, std::vector<rocConfig>::iterator rocit);

  public:

    /** Helper function to setup the attached devices for operation using
     *  calibrate pulses.
     *
     *  It sets the Pixel Unit Cell (PUC) Calibrate bit for
     *  every pixels enabled in the test range (those for which the "enable"
     *  flag has been set using the dut::testPixel() functions)
     *
     * Disclaimer: use at your own risk, don't rely on this method staying public
     */
    void SetCalibrateBits(bool enable);

   private:

    /** Helper function to check validity of the pattern generator settings
     *  coming from the user space.
     *
     *  Right now this only checks for wrong or dangerous delay settings either
     *  stopping the PG too early or not at all (missing delay = 0 at the end
     *  of the pattern command list)
     *  For non-correctable problems a InvalidConfig exception is thrown.
     */
    void verifyPatternGenerator(std::vector<std::pair<std::string,uint8_t> > &pg_setup);

    /** Helper function to check validity of testboard power settings (voltages and
     *  current limits) coming from the user space.
     */
    void checkTestboardPower(std::vector<std::pair<std::string,double> > power_settings);

    /** Helper function to check validity of testboard dsignal delay settings (register name
     *  lookup, range validation) coming from the user space.
     */
    void checkTestboardDelays(std::vector<std::pair<std::string,uint8_t> > sig_delays);

    /** Helper function to return the sum of all pattern generator delays
     */
    uint32_t getPatternGeneratorDelaySum(std::vector<std::pair<uint16_t,uint8_t> > &pg_setup);

    /** Status of the DAQ
     */
    bool _daq_running;

    /** Allocated memory size on the DTB for the currently running DAQ session
     */
    uint32_t _daq_buffersize;

    /** Warned the user about not initializing the DUT */
    bool _daq_startstop_warning;

  }; // class pxarCore


} //namespace pxar

#endif /* PXAR_API_H */
