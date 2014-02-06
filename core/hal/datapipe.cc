#include "datapipe.h"
#include "constants.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {
    pos = 0;
    do {
      dtbState = tb->Daq_Read(buffer, DTB_SOURCE_BLOCK_SIZE, dtbRemainingSize, channel);
      /*
	if (dtbRemainingSize < 100000) {
	if      (dtbRemainingSize > 1000) tb->mDelay(  1);
	else if (dtbRemainingSize >    0) tb->mDelay( 10);
	else                              tb->mDelay(100);
	}
	LOG(logDEBUGHAL) << "Buffer size: " << buffer.size();
      */
    
      if (buffer.size() == 0) {
	if (stopAtEmptyData) throw dsBufferEmpty();
	if (dtbState) throw dsBufferOverflow();
      }
    } while (buffer.size() == 0);

    /*
    LOG(logDEBUGHAL) << "----------------";
    LOG(logDEBUGHAL) << "Channel " << static_cast<int>(channel) << " DESER400: " << (int)tbm_present;
    LOG(logDEBUGHAL) << "----------------";
    std::stringstream os;
    for (unsigned int i = 0; i < buffer.size(); i++) {
      os << " " << std::setw(4) << std::setfill('0') << std::hex << (unsigned int)(buffer.at(i));
    }
    LOG(logDEBUGHAL) << os.str();
    LOG(logDEBUGHAL) << "----------------";
    */

    return lastSample = buffer[pos++];
  }


  rawEvent* dtbEventSplitter::SplitDeser400() {
    record.Clear();

    // If last one had event end marker, get a new sample:
    if (GetLast() & 0x00f0) { Get(); }

    // If new sample does not have start marker keep on reading until we find it:
    if (!(GetLast() & 0x0080)) {
      record.SetStartError();
      while (!(GetLast() & 0x0080)) Get();
    }

    // Else keep reading and adding samples until we find any marker.
    do {
      // If total event size is too big, break:
      if (record.GetSize() >= 40000) {
	record.SetOverflow();
	break;
      }

      // FIXME Very first event starts with 0xC - which srews up empty event detection here!
      // If the event start sample is also event end sample, write and quit:
      //if((GetLast() & 0xf000) == 0xf000) { break; }

      record.Add(GetLast() & 0x00ff);
    } while ((Get() & 0x00f0) != 0x00f0);

    // Check if the last read sample has event end marker:
    if (GetLast() & 0x00f0) record.Add(GetLast() & 0x00ff);
    // Else set event end error:
    else record.SetEndError();

    /*
    LOG(logDEBUGHAL) << "-------------------------";
    std::stringstream os;
    for (unsigned int i=0; i<record.GetSize(); i++)
      os << " " << std::setw(4) << std::setfill('0') << std::hex 
	 << static_cast<uint16_t>(record[i]);
    LOG(logDEBUGHAL) << os.str();
*/
    return &record;
  }


  rawEvent* dtbEventSplitter::SplitDeser160() {
    record.Clear();

    // If last one had event end marker, get a new sample:
    if (GetLast() & 0x4000) { Get(); }

    // If new sample does not have start marker keep on reading until we find it:
    if (!(GetLast() & 0x8000)) {
      record.SetStartError();
      while (!(GetLast() & 0x8000)) Get();
    }

    // Else keep reading and adding samples until we find any marker.
    do {
      // If total event size is too big, break:
      if (record.GetSize() >= 40000) {
	record.SetOverflow();
	break;
      }

      // FIXME Very first event starts with 0xC - which srews up empty event detection here!
      // If the event start sample is also event end sample, write and quit:
      if((GetLast() & 0xc000) == 0xc000) { break; }

      record.Add(GetLast() & 0x0fff);
    } while ((Get() & 0xc000) == 0);

    // Check if the last read sample has event end marker:
    if (GetLast() & 0x4000) record.Add(GetLast() & 0x0fff);
    // Else set event end error:
    else record.SetEndError();


    LOG(logDEBUGHAL) << "-------------------------";
    std::stringstream os;
    for (unsigned int i=0; i<record.GetSize(); i++)
      os << " " << std::setw(4) << std::setfill('0') << std::hex 
	 << static_cast<uint16_t>(record[i]);
    LOG(logDEBUGHAL) << os.str();

    return &record;
  }


  event* dtbEventDecoder::DecodeDeser400() {

    roc_event.header = 0;
    roc_event.trailer = 0;
    roc_event.pixels.clear();

    rawEvent *sample = Get();

    uint16_t hdr = 0, trl = 0;
    unsigned int raw = 0;

    // Get the right ROC id, channel 0: 0-7, channel 1: 8-15
    int16_t roc_n = -1 + GetChannel() * 8; 
    
    
    for (unsigned int i = 0; i < sample->GetSize(); i++) {
      int d = (*sample)[i] & 0xf;
      int q = ((*sample)[i]>>4) & 0xf;

      switch (q) {
      case  0: break;

      case  1: raw = d; break;
      case  2: raw = (raw<<4) + d; break;
      case  3: raw = (raw<<4) + d; break;
      case  4: raw = (raw<<4) + d; break;
      case  5: raw = (raw<<4) + d; break;
      case  6: raw = (raw<<4) + d;
	{
	pixel pix(raw);
	pix.roc_id = roc_n;
	roc_event.pixels.push_back(pix);
	break;
	}
      case  7: roc_n++; break;

      case  8: hdr = d; break;
      case  9: hdr = (hdr<<4) + d; break;
      case 10: hdr = (hdr<<4) + d; break;
      case 11: hdr = (hdr<<4) + d;
	roc_event.header = hdr;
	roc_n = -1 + GetChannel() * 8;
	break;

      case 12: trl = d; break;
      case 13: trl = (trl<<4) + d; break;
      case 14: trl = (trl<<4) + d; break;
      case 15: trl = (trl<<4) + d;
	roc_event.trailer = trl;
	break;
      }
    }

    // LOG(logDEBUGHAL) << roc_event;
    return &roc_event;
  }

  event* dtbEventDecoder::DecodeDeser160() {
    rawEvent *sample = Get();
    roc_event.header = 0;
    roc_event.pixels.clear();
    unsigned int n = sample->GetSize();
    if (n > 0) {
      if (n > 1) roc_event.pixels.reserve((n-1)/2);
      roc_event.header = (*sample)[0];
      unsigned int pos = 1;
      while (pos < n-1) {
	uint32_t raw = (*sample)[pos++] << 12;
	raw += (*sample)[pos++];
	pixel pix(raw);
	roc_event.pixels.push_back(pix);
      }
    }

    //LOG(logDEBUGHAL) << roc_event;
    return &roc_event;
  }
}
