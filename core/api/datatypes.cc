#include "datatypes.h"
#include "log.h"
#include "exceptions.h"
#include "constants.h"

namespace pxar {


  void pixel::decodeRaw(uint32_t raw, bool invert) {
    setValue(static_cast<double>((raw & 0x0f) + ((raw >> 1) & 0xf0)));
    if( (raw & 0x10) >0) {
      LOG(logDEBUGAPI) << "invalid pulse-height fill bit from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataDecoderError("Error decoding pixel raw value");
    }
    int c =    (raw >> 21) & 7;
    c = c*6 + ((raw >> 18) & 7);
    
    int r2 =    (raw >> 15) & 7;
    if(invert) { r2 ^= 0x7; }
    int r1 = (raw >> 12) & 7;
    if(invert) { r1 ^= 0x7; }
    int r0 = (raw >>  9) & 7;
    if(invert) { r0 ^= 0x7; }
    int r = r2*36 + r1*6 + r0;
    
    _row = 80 - r/2;
    _column = 2*c + (r&1);
    
    if (_row >= ROC_NUMROWS || _column >= ROC_NUMCOLS){
      LOG(logDEBUGAPI) << "invalid pixel from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataDecoderError("Error decoding pixel raw value");
    }
  }

  void statistics::print() {
    // Print out the full statistics:
    LOG(logINFO) << "Decoding statistics:";
    LOG(logINFO) << "  Decoding errors: \t     " << this->errors_decoding();
    LOG(logINFO) << "\t pixel address:         " << this->errors_decoding_pixel();
    LOG(logINFO) << "\t pulse height fill bit: " << this->errors_decoding_pulseheight();
    LOG(logINFO) << "\t buffer corruption:     " << this->errors_decoding_buffer_corrupt();
  }

  void statistics::clear() {
    //FIXME fill...
  }

  statistics& operator+=(statistics &lhs, const statistics &rhs) {
    lhs.m_errors_decoding_pixel += rhs.m_errors_decoding_pixel;
    lhs.m_errors_decoding_pulseheight += rhs.m_errors_decoding_pulseheight;
    lhs.m_errors_decoding_buffer_corrupt += rhs.m_errors_decoding_buffer_corrupt;
    // FIXME fill...
    return lhs;
  }

} // namespace pxar
