//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arParallelSwitchDriver.h"

/*============ pp_access.h =====================================

  PURPOSE - this module supports access to the parallel port. 
            It has several key uses-
                - saving the initial pp state.
                - restoring the pp state.
                - maintaining an image of the pp data and control
                  registers,  see the notes.
                - setting and testing bits and bytes within the pp
                  data, control, and status registers.

     NOTE - Bit manipulation can be done on a data and control
            image and then the image written to the port.  This
            ensure no intermediate states on the port and faster
            access as only one IO is required ( IO take 0.9 us).
            Note the macros at the end of the *.h file.
          - macros are used for IO to reduce the complexity for the
            user and to be fatser than function calls.
            For very highest speed just write direct to the registers.
          - this implmentation is only useful if just one serial
            port is being controlled.  An object solution would
            be much better for multiple ports.

   CHECK PP ACCESS  : there are several steps required to get parallel
                      port access-
          - check in BIOS and ensure that the pp is in SPP, compatible,
            or bidirectional mode.  It must not be in EPP or ECP mode.
          - IO access must be enabled either in the main application
            program or using lp_tty_start program
          
*/

#ifdef AR_USE_LINUX

#ifndef PP_ACCESS
#define PP_ACCESS

#include <sys/io.h>

//------ Data that defines ports and port content. --------------------------
int lp_base_addr ;                  // save base addr of pp port. 
#define status_offset  1
#define control_offset 2

unsigned char save_data ;           // save of original values.
unsigned char save_control ;
unsigned char image_data ;          // use image as master record of port.
unsigned char image_control ;

#define lp0   0x378                 // lp (LPT) base addresses.
#define lp1   0x278
#define lp2   0x3BC
#define lp_length   3


//------ Routines ------------------------------------------------------------
//--- initialise and restore printer port by number (0,1,2).
void lp_init(short lp_num) ;
void lp_restore() ;
//--- return based address of printer port.
int  lp_base() ;

//--- MACROs, faster than function calls.
#define TEST_Error    ( inb( lp_base_addr+status_offset) & 0x08) 

#define SET_nInit     outb((image_control |= 0x04),lp_base_addr+control_offset)
#define SET_nLinefeed outb((image_control &= 0xFD),lp_base_addr+control_offset) 
#define CLR_nLinefeed outb((image_control |= 0x02),lp_base_addr+control_offset) 

#define WR_pp_data( byte)   outb(( image_data = byte), lp_base_addr)
#define RD_pp_data          ( image_data = inb( lp_base_addr))

#endif 

/*============ pp_access.c =====================================

    See pp_access.h for details.          
*/

#include <stdio.h>
#include <unistd.h>
//#include "pp_access.h"

/*-------- init() --------------------------------------------

  PURPOSE - given lp number 0-2, get lp base address, 
            save registers, disable interrupts.
*/
void lp_init(short lp_num) 
 { switch ( lp_num)
    { case 2 : lp_base_addr = 0x3BC ; break ;
      case 1 : lp_base_addr = 0x278 ; break ;
      default: lp_base_addr = 0x378 ; break ;
    } 
   image_data    = save_data    = inb( lp_base_addr) ;
   image_control = save_control = inb( lp_base_addr+2) ;
   outb( (image_control &= 0xEF),  lp_base_addr + control_offset) ;
 }

/*--------- lp_restore() ------------------------------------

  PURPOSE - restore lp port to previous state.
*/
void lp_restore()
{  outb( save_data,    lp_base_addr) ;
   outb( save_control, lp_base_addr + control_offset) ;
}

//---------------- lp_base -----------------------------------

int lp_base()
 { return( lp_base_addr);
 }

//--------- main : example code ------------------------------

/*
int main(int argc, char *argv[])
{ //--- save all registers and disable interrupts.
    lp_init( 0) ;

  //---  clear data bit 0 and set data bit 1, keep image updated.
    outb( (image_data &= 0xFE), lp_base_addr) ;
    outb( (image_data |= 0x02), lp_base_addr) ;
    printf("   Pin 2 (d0) is now low, pin 3 (d1) is now high.\n") ;

  //--- test if select or busy (inverted) is high.
    if ( inb( lp_base_addr+status_offset) & 0x10)
         printf("   Input bit select ( pin 13) is low.\n") ;
    else printf("   Input bit select ( pin 13) is high.\n") ;
    if ( inb( lp_base_addr+status_offset) & 0x80)
         printf("   Input bit busy ( pin 11) is high.\n") ;
    else printf("   Input bit busy ( pin 11) is low.\n") ;

  //--- pause till key pressed so can measure outputs.
    printf("   Press enter to continue ===> ") ;
    getchar() ; 

  //--- restore control and data registers and exit. 
    lp_restore() ;
    exit(0) ;
}
*/
	     
#endif

DriverFactory(arParallelSwitchDriver, "arInputSource")

arParallelSwitchDriver::arParallelSwitchDriver() :
  _eventThreadRunning(false),
  _stopped(false),
  _byteMask(255),
  _lastValue(0),
  _lastEventTime(0,0)
{}

arParallelSwitchDriver::~arParallelSwitchDriver() {
}

bool arParallelSwitchDriver::init(arSZGClient& SZGClient){
#ifndef AR_USE_LINUX
  ar_log_error() << "arParallelSwitchDriver on Linux only!\n";
  return false;
#else
  int i = 0;

  // Multiple serial ports aren't implemented.
  _comPortID = static_cast<unsigned int>(SZGClient.getAttributeInt("SZG_PARALLEL_SWITCH", "port"));
  if (_comPortID <= 0) {
    ar_log_warning() << "arParallelSwitchDriver: SZG_PARALLEL_SWITCH/port defaulting to 1.\n";
    _comPortID = 1;
  }
  const string eventType = SZGClient.getAttribute( "SZG_PARALLEL_SWITCH", "event_type", "|both|open|closed|" );
  ar_log_critical() << "Parallel switch event type = '" << eventType << "'.\n";
  _eventType = eventType == "closed" ? AR_CLOSED_PARA_SWITCH_EVENT :
	       eventType == "open"   ? AR_OPEN_PARA_SWITCH_EVENT :
	       AR_BOTH_PARA_SWITCH_EVENT;

  _byteMask = static_cast<unsigned char>(SZGClient.getAttributeInt("SZG_PARALLEL_SWITCH", "byte_mask"));
  ar_log_critical() << "Parallel switch byte mask = " << _byteMask << ".\n";

  // Report one axis, the time since the last event occurred.
  _setDeviceElements( 0, 1, 0 );

  ar_log_debug() << "arParallelSwitchDriver inited.\n";
  return true;
#endif
}

bool arParallelSwitchDriver::start() {
#ifndef AR_USE_LINUX
  ar_log_error() << "arParallelSwitchDriver on Linux only!\n";
  return false;
#else
  return _eventThread.beginThread(ar_ParallelSwitchDriverEventTask,this);
#endif
}

void ar_ParallelSwitchDriverEventTask(void* driver){
  ((arParallelSwitchDriver*)driver)->_eventloop();
}

void arParallelSwitchDriver::_eventloop() {
#ifndef AR_USE_LINUX
  ar_log_error() << "arParallelSwitchDriver on Linux only!\n";
  return false;
#else
  _eventThreadRunning = true;
  cerr << "Calling ioperm.\n";
  ioperm( 0x378, 8, 1 );
  cerr << "Calling lp_init().\n";
  try {
    lp_init(0);
  } catch(...) {
    ar_log_error() << "lp_init() failed.\n";
    _eventThreadRunning = false;
    stop();
  }
  cerr << "Called lp_init().\n";
  while (!_stopped) {
    //ar_usleep(10000);
    if (!_poll()) {
      ar_log_warning() << "arParallelSwitchDriver _poll() failed.\n";
      _stopped = true;
    }
  }
  _eventThreadRunning = false;
  try {
    lp_restore();
  } catch(...) {
    ar_log_error() << "lp_restore() failed.\n";
  }
  stop();
#endif
}

bool arParallelSwitchDriver::stop() {
#ifndef AR_USE_LINUX
  ar_log_error() << "arParallelSwitchDriver on Linux only!\n";
  return false;
#else
  _stopped = true;
  arSleepBackoff a(10, 30, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  ar_log_debug() << "arParallelSwitchDriver stopped.\n";
  return true;
#endif
}

bool arParallelSwitchDriver::_poll( void ) {
#ifndef AR_USE_LINUX
  ar_log_error() << "arParallelSwitchDriver on Linux only!\n";
  return false;
#else
  if (_stopped)
    return false;

  unsigned char value;
  try {
    value = inb( lp_base_addr +status_offset ) & _byteMask;
    //value = inb( lp_base_addr );
    //ar_log_debug() << "treadmill " << value << ar_endl;
  } catch(...) {
    ar_log_error() << "inb() failed.\n";
    return false;
  }

  if (value != _lastValue) {
    ar_log_debug() << _lastValue << ", " << value;
    arParallelSwitchEventType switchState = (arParallelSwitchEventType)
      ((value != 0)+1);
    ar_log_debug() << "; " << switchState << ", " << _eventType;
      //((value < 80)+1);
    if (switchState & _eventType) {
      if (_lastEventTime.zero()) {
        _lastEventTime = ar_time();
      } else {
        const ar_timeval now = ar_time();
        double diffTime = ar_difftime( now, _lastEventTime );
        if (switchState == AR_OPEN_PARA_SWITCH_EVENT) {
          diffTime = -diffTime;
        }
        float dt = float(diffTime*1.e-6);
        ar_log_debug() << ": " << dt;
        sendAxis( 0, dt );
        _lastEventTime = now;
      }
    }
    _lastValue = value;
    ar_log_debug() << ar_endl;
  }
  return true;
#endif
}

