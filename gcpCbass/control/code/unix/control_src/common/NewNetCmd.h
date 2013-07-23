// $Id: NewNetCmd.h,v 1.1.1.1 2009/07/06 23:57:07 eml Exp $

#ifndef GCP_CONTROL_NEWNETCMD_H
#define GCP_CONTROL_NEWNETCMD_H

/**
 * @file NewNetCmd.h
 * 
 * Tagged: Fri Jul  8 16:17:58 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:07 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetStruct.h"
#include "gcp/util/common/NetUnion.h"

#include "genericregs.h"

namespace gcp {
  namespace control {

    // Define an allocator for an experiment-specific network command
    // This must be defined by inheritors

    class NewNetCmd;
    NewNetCmd* newSpecificNewNetCmd();

    /*
     * The first command sent to the controller after startup must be
     * the following. Note that this command can not be sent at any
     * other time.  When a connection is first initiated to the
     * translator layer, the scheduler automatically runs its
     * initialization script.  On start-up, the controller will send
     * this message with start=true, and after the scheduler has
     * completely sent the initialization script, it will queue this
     * command to be sent with start=false. 
     */
    class NewNetInitCmd : public gcp::util::NetStruct {
    public:
      bool start;

      NewNetInitCmd() {
	NETSTRUCT_BOOL(start);
      }

    };
    
    //------------------------------------------------------------
    // Define the contents of the shutdown command.
    //------------------------------------------------------------

    class NewNetShutdownCmd : public gcp::util::NetStruct {
    public:
      
      // Enumerate control-system shutdown options.

      enum RtcShutdownMethod {
	HARD_RESTART,  // Restart the control system via a reboot 
	SOFT_RESTART,  // Re-initialize the running control-system 
	HARD_SHUTDOWN, // Reboot the CPU and wait at the boot prompt 
	SOFT_SHUTDOWN  // Terminate the control system but don't reboot 
      };

      unsigned int method;  // A RtcShutdownMethod enumeration identifier 

      NewNetShutdownCmd() {
	NETSTRUCT_UINT(method); 
      }

    };
    
    //------------------------------------------------------------
    // The feature command conveys a bit-mask to be added or removed
    // from the set of feature bits to be recorded with one or more
    // subsequent archive frames. Once a feature bit has been added to
    // the transient or persistent set of feature bits, it is
    // guaranteed to be recorded in at least one frame. For transient
    // markers the feature bits are recorded in the next frame only,
    // whereas persistent feature markers will continue to appear in
    // subsequent frames until they are cancelled.
    //------------------------------------------------------------

    class NewNetFeatureCmd : public gcp::util::NetStruct {
    public:

      enum FeatureMode {
	FEATURE_ADD,    // Add the new set of feature bits to those
			// that are to be recorded in subsequent
			// frames.
	FEATURE_REMOVE, // Remove the specified set of features from
			// those that have previously been registered
			// with FEATURE_ADD
	FEATURE_ONE     // Add the new set of feature bits to the
			// transient set which is to be recorded just
			// in the next frame.
      };

      unsigned int seq;    // The mark-command sequence number of this message 
      unsigned int mode;   // What to do with the bit mask 
      unsigned long mask;  // The bit-mask to merge with any existing bit mask. 

      NewNetFeatureCmd() {
	NETSTRUCT_UINT(seq);    
	NETSTRUCT_UINT(mode);   
	NETSTRUCT_ULONG(mask);  
      }

    };

    //------------------------------------------------------------
    // Set the location of an antenna
    //------------------------------------------------------------

    class NewNetLocationCmd : public gcp::util::NetStruct {
    public:

      double north;    // The north offset of an antenna 
      double east;     // The east offset of an antenna 
      double up;       // The up offset of an antenna 

      NewNetLocationCmd() {
	NETSTRUCT_DOUBLE(north);    // The north offset of an antenna 
	NETSTRUCT_DOUBLE(east);     // The east offset of an antenna 
	NETSTRUCT_DOUBLE(up);       // The up offset of an antenna 
      }

    };


    //------------------------------------------------------------
    // Set the site of this experiment
    //------------------------------------------------------------

    class NewNetSiteCmd : public gcp::util::NetStruct {
    public:

      long lon;        // The longitude (east +ve) [-pi..pi]
		       // (milli-arcsec)
      long lat;        // The latitude [-pi/2..pi/2]
		       // (milli-arcsec)
      long alt;        // The altitude (mm) 
     
      NewNetSiteCmd() {
	NETSTRUCT_LONG(lon);        
	NETSTRUCT_LONG(lat);        
	NETSTRUCT_LONG(alt);        
      }
 
    };
    

    //------------------------------------------------------------
    // The getreg command reads the value of a specified register out
    // of the register map
    //------------------------------------------------------------
    
    class NewNetGetregCmd : public gcp::util::NetStruct {
    public:

      unsigned short board;      // The host-board of the register
      unsigned short block;      // The block number of the register
      unsigned short index;      // The index of the first element to
				 // be set

      NewNetGetregCmd() {
	NETSTRUCT_USHORT(board);      
	NETSTRUCT_USHORT(block);      
	NETSTRUCT_USHORT(index);      
      }

    };

    //------------------------------------------------------------
    // The setreg command sets the value of a specified register to a
    // given value.
    //------------------------------------------------------------

    class NewNetSetregCmd : public gcp::util::NetStruct {
    public:

      unsigned long value;       // The value to write to the register
      unsigned short board;      // The host-board of the register
      unsigned short block;      // The block number of the register
      unsigned short index;      // The index of the first element to
				 // be set
      unsigned short nreg;       // The number of elements to be set
      unsigned long seq;         // The sequence number of this
				 // transaction

      NewNetSetregCmd() {
	NETSTRUCT_ULONG(value);       
	NETSTRUCT_USHORT(board);      
	NETSTRUCT_USHORT(block);      
	NETSTRUCT_USHORT(index);      
	NETSTRUCT_USHORT(nreg);       
	NETSTRUCT_ULONG(seq);         
      }

    };
    
    //------------------------------------------------------------
    // The following object is sent to halt the telescope.
    //------------------------------------------------------------

    class NewNetHaltCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;      // The tracker sequence number of this command 
     
      NewNetHaltCmd() {
	NETSTRUCT_UINT(seq);      
      }

    };
    
    //------------------------------------------------------------
    // The NetScanCmd is used to command the telescope to perform a scan.
    // Sequential commands containing the same scan name will be combined
    // to form a scrolling table of offsets of up to SCAN_NET_NPT points
    // at a time.
    //
    // A non-zero sequence number will indicate the start of a new
    // scan.
    //------------------------------------------------------------

    class NewNetScanCmd : public gcp::util::NetStruct {
    public:

      static const unsigned SCAN_NET_NPT=10;
    
      // Members

      char name[SCAN_LEN]; // The name of the scan
      unsigned int seq;    // The sequence number of this command
      unsigned int index[SCAN_NET_NPT];
      unsigned int npt;    // The number of points sent with this
			   // command
      long azoff[SCAN_NET_NPT];
      long eloff[SCAN_NET_NPT];
      long dkoff[SCAN_NET_NPT];
     
      NewNetScanCmd() {
	NETSTRUCT_CHAR_ARR(name, SCAN_LEN); 
	NETSTRUCT_UINT(seq);    
	NETSTRUCT_UINT_ARR(index, SCAN_NET_NPT);
	NETSTRUCT_UINT(npt);    
	NETSTRUCT_LONG_ARR(azoff, SCAN_NET_NPT);
	NETSTRUCT_LONG_ARR(eloff, SCAN_NET_NPT);
	NETSTRUCT_LONG_ARR(dkoff, SCAN_NET_NPT);
      }

    };
    
    
    //------------------------------------------------------------
    // The following object is used to request a telescope slew to a
    // given mount position.
    //------------------------------------------------------------

    class NewNetSlewCmd : public gcp::util::NetStruct {
    public:

      enum DriveAxis { 
	DRIVE_AZ_AXIS=1,    // Slew the azimuth axis
	DRIVE_EL_AXIS=2,    // Slew the elevation axis
	DRIVE_DK_AXIS=4,    // Slew the deck axis
	DRIVE_ALL_AXES = DRIVE_AZ_AXIS | DRIVE_EL_AXIS | DRIVE_DK_AXIS
      };
      
      char source[SRC_LEN]; // The name of the source
      unsigned int number;  // The catalog number of this source
      unsigned int seq;     // The sequence number of this command
      unsigned int mask;    // A bitwise union of DriveAxes enumerated
			    // bits used to specify which of the
			    // following axis positions are to be
			    // used.
      long az;              // The target azimuth (0..360 degrees in
			    // mas)
      long el;              // The target elevation (0..90 degrees in
			    // mas)
      long dk;              // The target deck angle (-180..180 in
			    // mas)
      unsigned int type;    // A gcp::util::Tracking::Type enumerator


      NewNetSlewCmd() {
	NETSTRUCT_CHAR_ARR(source, SRC_LEN); 
	NETSTRUCT_UINT(number);  
	NETSTRUCT_UINT(seq);     
	NETSTRUCT_UINT(mask);    
	NETSTRUCT_LONG(az);              
	NETSTRUCT_LONG(el);              
	NETSTRUCT_LONG(dk);              
	NETSTRUCT_UINT(type);    
      }
    };
    //------------------------------------------------------------
    // The NetTrackCmd is used to command the telescope to track a source.
    // Sequential commands containing the same source name will be
    // combined to form a scrolling interpolation table of up to 3 points
    // at a time.
    //
    // On source changes the control program is expected to send three
    // ephemeris entries for the source, one preceding the current time
    // and two following it. It is thereafter expected to send a new
    // ephemeris entry whenever the current time passes that of the
    // second point in the table. The new entry must be for a later time
    // than the existing 3rd entry.
    //
    // On receipt of a track command with a new source name, the
    // tracker task will immediately command a slew to the first
    // position received.  It is anticipated that by the time the slew
    // ends, the control program will have had more than enough time
    // to send two more entries. If only one more entry has been
    // received, linear interpolation will be used. If no new entries
    // have been received then the details of the single entry will be
    // used without any interpolation.
    //------------------------------------------------------------
    
    class NewNetTrackCmd : public gcp::util::NetStruct {
    public:

      char source[SRC_LEN]; // The name of the source
      unsigned int number;  // The catalog number of the source
      unsigned int srcType; // The source type
      unsigned int seq;     // The sequence number of this command
      long mjd;             // The Terrestrial Time at which ra,dec
			    // are valid, as a Modified Julian Day
			    // number
      long tt;              // The number of TT milliseconds into day
			    // 'mjd'
      long ra;              // The desired apparent Right Ascension
			    // (0..360 degrees in mas)
      long dec;             // The desired apparent Declination
			    // (-180..180 degrees in mas)
      long dist;            // The distance to the source if it is
			    // near enough for parallax to be
			    // significant. Specify the distance in
			    // micro-AU Send 0 for distant sources.
      unsigned int type;    // A gcp::util::Tracking::Type enumerator
     
      NewNetTrackCmd() {
	NETSTRUCT_CHAR_ARR(source, SRC_LEN); 
	NETSTRUCT_UINT(number);  
	NETSTRUCT_UINT(srcType); 
	NETSTRUCT_UINT(seq);     
	NETSTRUCT_LONG(mjd);             
	NETSTRUCT_LONG(tt);              
	NETSTRUCT_LONG(ra);              
	NETSTRUCT_LONG(dec);             
	NETSTRUCT_LONG(dist);            
	NETSTRUCT_UINT(type);    
      }

    };
        
    //------------------------------------------------------------
    // The following command establishes new horizon pointing offsets.
    //------------------------------------------------------------

    // The following enumerators specify how new offsets are to
    // effect existing offsets.
    
    enum OffsetMode {
      OFFSET_ADD,     // Add the new offsets to any existing offsets
      OFFSET_SET      // Replace the existing offsets with the new
		      // offsets
    };
    
    class NewNetMountOffsetCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;   // The tracker sequence number of this command
      unsigned int axes;   // The set of axes to offset, as a union of
			   // SkyAxis enumerators.
      unsigned int mode;   // The effect of the offsets on existing
			   // offsets, chosen from the above
			   // enumerators.
      double az,el,dk;       // The offsets for the azimuth, elevation
			   // and deck axes. Only those values that
			   // correspond to axes included in the
			   // 'axes' set will be used.

      NewNetMountOffsetCmd() {

	NETSTRUCT_UINT(seq);   
	NETSTRUCT_UINT(axes);   
	NETSTRUCT_UINT(mode);   
	NETSTRUCT_LONG(az);
	NETSTRUCT_LONG(el);
	NETSTRUCT_LONG(dk);       
      }

    };
    
    //------------------------------------------------------------
    // The following command establishes new equatorial pointing
    // offsets.
    //------------------------------------------------------------

    class NewNetEquatOffsetCmd : public gcp::util::NetStruct {
    public:

      enum EquatAxis {    // The set of offsetable equatorial axes
	EQUAT_RA_AXIS=1,  // The Right-Ascension axis
	EQUAT_DEC_AXIS=2  // The declination axis
      };

      unsigned int seq;   // The tracker sequence number of this command
      unsigned int axes;  // The set of equatorial axes to offset, as
			  // a union of EquatAxis enumerators
      unsigned int mode;  // The effect of the offsets on existing
			  // offsets, chosen from OffsetMode
			  // enumerators.
      long ra,dec;        // The offsets for the right-ascension and
		          // declination axes. Only those values that
		          // correspond to axes included in the 'axes' set
		          // will be used.

      NewNetEquatOffsetCmd() {

	NETSTRUCT_UINT(seq);   
	NETSTRUCT_UINT(axes);  
	NETSTRUCT_UINT(mode);  
	NETSTRUCT_LONG(ra);
	NETSTRUCT_LONG(dec);        
      }

    };
    
    
    //------------------------------------------------------------
    // The following command asks the tracker to add to the az and el
    // tracking offsets such that the image on the tv monitor of the
    // optical-pointing telescope moves by given amounts horizontally
    // and vertically.
    //------------------------------------------------------------

    class NewNetTvOffsetCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq; // The tracker sequence number of this command
      long up;      // The amount to move the image up on the display
		    // (mas)
      long right;   // The amount to move the image right on the
		    // display (mas)

      NewNetTvOffsetCmd() {
	NETSTRUCT_UINT(seq); 
	NETSTRUCT_LONG(up);      
	NETSTRUCT_LONG(right);   
      }

    };
    
    
    //------------------------------------------------------------
    // The following command sets the deck angle at which the vertical
    // direction on the tv monitor of the optical telescope matches
    // the direction of increasing topocentric elevation.
    //------------------------------------------------------------

    class NewNetTvAngleCmd : public gcp::util::NetStruct {
    public:

      long angle;   // The deck angle at which the camera image is
		    // upright (mas)

      NewNetTvAngleCmd() {
	NETSTRUCT_LONG(angle);   
      }

    };
    
    //------------------------------------------------------------
    // The SkyOffset command tells the tracker to track a point at a
    // given fixed sky offset from the normal pointing center,
    // regardless of elevation or declination. This is used primarily
    // for making beam maps.
    //------------------------------------------------------------

    class NewNetSkyOffsetCmd : public gcp::util::NetStruct {
    public:

      enum  SkyXYAxes {   // Set members for the NetSkyOffsetCmd
			  // axes member.
	SKY_X_AXIS = 1,   // The NetSkyOffsetCmd::x axis
	SKY_Y_AXIS = 2,   // The NetSkyOffsetCmd::y axis
      };

      unsigned int seq; // The tracker sequence number of this command
      unsigned int axes; // The set of axes to offset, as a union of
			 // SkyXYAxes enumerators.
      unsigned int mode; // The effect of the new offsets on any
			 // existing offsets, chosen from OffsetMode
			 // enumerators.
      long x,y;     // The 2-dimensional angular offset, expressed as
		    // distances along two great circles that meet at
		    // right angles at the normal pointing center. The
		    // y offset is directed along the great circle
		    // that joins the normal pointing center to the
		    // zenith. The x offset increases along the
		    // perpendicular great circle to this, increasing
		    // from east to west. Both offsets are measured in
	
      NewNetSkyOffsetCmd() {
	NETSTRUCT_UINT(seq); 
	NETSTRUCT_UINT(axes); 
	NETSTRUCT_UINT(mode); 
	NETSTRUCT_LONG(x);
	NETSTRUCT_LONG(y);     
      }
      
    };

    //------------------------------------------------------------
    // The NetDeckModeCmd command tells the track task how to position
    // the deck axis while tracking a source.
    //------------------------------------------------------------

    class NewNetDeckModeCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;     // The tracker sequence number of this command 
      unsigned int mode;     // A DeckMode enumerator from genericregs.h
     
      NewNetDeckModeCmd() {
	NETSTRUCT_UINT(seq);     
	NETSTRUCT_UINT(mode);     
      }

    };

    //------------------------------------------------------------
    // The atmosphere command is used to supply atmospheric parameters
    // for refraction computations in the weather-station task. It is
    // not needed when the weather station is functioning.
    //------------------------------------------------------------

    class NewNetAtmosCmd : public gcp::util::NetStruct {
    public:

      double temperature;         // The outside temperature (mC)
      double humidity;            // The relative humidity (0-1)*1000
      double pressure;            // The atmospheric pressure
				  // (micro-bar)
      NewNetAtmosCmd() {
	NETSTRUCT_DOUBLE(temperature);         
	NETSTRUCT_DOUBLE(humidity);            
	NETSTRUCT_DOUBLE(pressure);            
      }

    };

    //------------------------------------------------------------
    // The NetUt1UtcCmd and NetEqnEqxCmd commands are used to send occasional
    // updates of variable earth orientation parameters.
    //
    // For each command the control system retains a table of the 3
    // most recently received updates. These three values are
    // quadratically interpolated to yield orientation parameters for
    // the current time.  On connection to the control system, the
    // control program is expected to send values for the start of the
    // current day, the start of the following day and the start of
    // the day after that. Thereafter, at the start of each new day,
    // it should send parameters for a time two days in the future.
    //
    // On startup of the control system, requests for ut1utc and eqex
    // will return zero. On receipt of the first earth-orientation
    // command, requests for orientation parameters will return the
    // received values.  On the receipt of the second, requesters will
    // receive a linear interpolation of the parameters. On receipt of
    // the third and subsequent commands, requesters will receive
    // quadratically interpolated values using the parameters of the
    // three most recently received commands.
    //------------------------------------------------------------

    class NewNetUt1UtcCmd : public gcp::util::NetStruct {
    public:

      long mjd;          // The UTC to which this command refers as a
			 // Julian Day number
      long utc;          // The number of UTC milliseconds into day
			 // 'mjd'
      long ut1utc;       // The value of ut1 - utc (us)     

      NewNetUt1UtcCmd() {
	NETSTRUCT_LONG(mjd);          
	NETSTRUCT_LONG(utc);          
	NETSTRUCT_LONG(ut1utc);       
      }

    };
    
    class NewNetEqnEqxCmd : public gcp::util::NetStruct {
    public:

      long mjd;          // The Terrestrial Time to which this command
			 // refers, as a Modified Julian day number
      long tt;           // The number of TT milliseconds into day
			 // 'mjd'
      long eqneqx;       // The equation of the equinoxes (mas) */

      NewNetEqnEqxCmd() {
	NETSTRUCT_LONG(mjd);          
	NETSTRUCT_LONG(tt);           
	NETSTRUCT_LONG(eqneqx);       
      }

    };
    
    
    //------------------------------------------------------------
    // The NetEncoderCalsCmd is used to calibrate the scales and
    // directions of the telescope encoders.
    //------------------------------------------------------------

    class NewNetEncoderCalsCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;  // The tracker sequence number of this
			 // command
      long az;           // Azimuth encoder counts per turn
      long el;           // Elevation encoder counts per turn
      long dk;           // Deck encoder counts per turn
     
      NewNetEncoderCalsCmd() {
	NETSTRUCT_UINT(seq);      
	NETSTRUCT_LONG(az);           
	NETSTRUCT_LONG(el);           
	NETSTRUCT_LONG(dk);           
      }

    };

    //------------------------------------------------------------
    // The NetEncoderLimitsCmd command tells the tracker task what the
    // limits on encoder values are.
    //------------------------------------------------------------

    class NewNetEncoderLimitsCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;     // The tracker sequence number of this command
      long az_min;      // The lower azimuth limit (encoder counts)
      long az_max;      // The upper azimuth limit (encoder counts)
      long el_min;      // The lower elevation limit (encoder counts)
      long el_max;      // The upper elevation limit (encoder counts)
      long pa_min;      // The lower pa limit (encoder counts)
      long pa_max;      // The upper pa limit (encoder counts)
     
      NewNetEncoderLimitsCmd() {
	NETSTRUCT_UINT(seq);     
	NETSTRUCT_LONG(az_min);      
	NETSTRUCT_LONG(az_max);      
	NETSTRUCT_LONG(el_min);      
	NETSTRUCT_LONG(el_max);      
	NETSTRUCT_LONG(pa_min);      
	NETSTRUCT_LONG(pa_max);      
      }

    };

    //------------------------------------------------------------
    // The NetEncoderZerosCmd is used to set the zero points of the
    // telescope encoders. The angles are measured relative to the
    // position at which the encoders show zero counts.
    //------------------------------------------------------------

    class NewNetEncoderZerosCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq; // The tracker sequence number of this command 
      double az;    // Azimuth encoder angle at zero azimuth, measured
		    // in the direction of increasing azimuth
		    // (radians).
      double el;    // Elevation encoder angle at zero elevation,
		    // measured in the direction of increasing
		    // elevation (radians).
      double dk;    // Deck encoder angle at the deck reference
		    // position, measured clockwise when looking
      // towards the sky (radians).
	
      NewNetEncoderZerosCmd() {
	NETSTRUCT_UINT(seq); 
	NETSTRUCT_DOUBLE(az);    
	NETSTRUCT_DOUBLE(el);    
	NETSTRUCT_DOUBLE(dk);    
      }

    };

    //------------------------------------------------------------
    // The NetSlewRateCmd is used to set the slew speeds of each of
    // the telescope axes. The speed is specified as a percentage of
    // the maximum speed available.
    //------------------------------------------------------------

    class NewNetSlewRateCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;  // The tracker sequence number of this
			 // command
      unsigned int mask; // A bitwise union of DriveAxes
			 // enumeratedbits, used to specify which of
			 // the following axis rates are to be
			 // applied.
      long az;       // Azimuth slew rate (0-100)
      long el;       // Elevation slew rate (0-100)
      long dk;       // Deck slew rate (0-100)
     
      NewNetSlewRateCmd() {
	NETSTRUCT_UINT(seq);  
	NETSTRUCT_UINT(mask);  
	NETSTRUCT_LONG(az);       
	NETSTRUCT_LONG(el);       
	NETSTRUCT_LONG(dk);       
      }

    };

    //------------------------------------------------------------
    // The NetTiltsCmd is used to calibrate the axis tilts of the
    // telescope.
    //------------------------------------------------------------

    class NewNetTiltsCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq; // The tracker sequence number of this command
      long ha;          // The hour-angle component of the azimuth-axis
		        // tilt (mas)
      long lat;         // The latitude component of the azimuth-axis
		        // tilt (mas)
      long el;          // The tilt of the elevation axis
		        // perpendicular to the azimuth ring, measured
		        // clockwise around the direction of the
		        // azimuth vector (mas)
	
      NewNetTiltsCmd() {
	NETSTRUCT_UINT(seq);  
	NETSTRUCT_LONG(ha);       
	NETSTRUCT_LONG(lat);      
	NETSTRUCT_LONG(el);       
      }

    };

    //------------------------------------------------------------
    // The NetFlexureCmd is used to calibrate the gravitational
    // flexure of the telescope.
    //------------------------------------------------------------

    class NewNetFlexureCmd : public gcp::util::NetStruct {
    public:


      unsigned int seq;  // The tracker sequence number of this command
      unsigned int mode;  // An sza::util::PointingMode enumeration
      long sFlexure; // Gravitational flexure (milli-arcsec per sine
		     // elevation)
      long cFlexure; // Gravitational flexure (milli-arcsec per
		     // cosine elevation)

      NewNetFlexureCmd() {

	NETSTRUCT_UINT(seq);  
	NETSTRUCT_UINT(mode);  
	NETSTRUCT_LONG(sFlexure); 
	NETSTRUCT_LONG(cFlexure); 
      }

    };

    //------------------------------------------------------------
    // The NetCollimateCmd command is used to calibrate the
    // collimation of the optical or radio axes.
    //------------------------------------------------------------

    class NewNetCollimateCmd : public gcp::util::NetStruct {
    public:


      unsigned int seq;     // The tracker sequence number of this command
      unsigned int mode;    // An sza::util::PointingMode enumeration
      long x;               // The magnitude of the azimuthal offset (mas)
      long y;               // The magnitude of the elevation offset
			    // (mas)
      unsigned int addMode; // The effect of the new offsets on any
			    // existing offsets, chosen from
			    // OffsetMode enumerators.

      NewNetCollimateCmd() {

	NETSTRUCT_UINT(seq);    
	NETSTRUCT_UINT(mode);    
	NETSTRUCT_LONG(x);          
	NETSTRUCT_LONG(y);          
	NETSTRUCT_UINT(addMode); 
      }

    };

    //------------------------------------------------------------
    // The NetModelCmd command selects between the optical and radio
    // pointing models.
    //------------------------------------------------------------
    
    class NewNetModelCmd : public gcp::util::NetStruct {
    public:

      unsigned int seq;     // The tracker sequence number of this command
      unsigned int mode;    // A PointingMode enumeration

      NewNetModelCmd() {
	NETSTRUCT_UINT(seq);     
	NETSTRUCT_UINT(mode);     
      }

    };

    //------------------------------------------------------------
    // The NetYearCmd command tells the control system what the
    // current year is. This is necessary because the gps time-code
    // reader doesn't supply year information.
    //------------------------------------------------------------

    class NewNetYearCmd : public gcp::util::NetStruct {
    public:


      short year;       // The current Gregorian year

      NewNetYearCmd() {
	NETSTRUCT_SHORT(year);       
      }

    };

    
    //------------------------------------------------------------
    // Gpib commands
    //------------------------------------------------------------

    // Specify the maximum size of a GPIB data message.  Note that
    // this effects both the size of network communications buffers
    // and the size of some message queue nodes, so it shouldn't be
    // made too large.

    enum {GPIB_MAX_DATA=80};
    
    // The gpib-send command tells the GPIB control task to try to
    // send the specified message to a given GPIB device.

    class NewNetGpibSendCmd : public gcp::util::NetStruct {
    public:


      unsigned short device;          // The generic address of the
				      // target GPIB device (0..30).
      char message[GPIB_MAX_DATA+1];  // The message to be sent.

      NewNetGpibSendCmd() {
	NETSTRUCT_USHORT(device);          
	NETSTRUCT_CHAR_ARR(message, GPIB_MAX_DATA+1);  
      }

    };

    // The gpib-read command tells the GPIB control task to try to
    // read a message from a given GPIB device.

    class NewNetGpibReadCmd : public gcp::util::NetStruct {
    public:


      unsigned short device;          // The generic address of the
				      // source GPIB device (0..30).

      NewNetGpibReadCmd() {
	NETSTRUCT_USHORT(device);          
      }

    };

    //------------------------------------------------------------
    // Turns power on/off to one or more antenna breakers
    //------------------------------------------------------------

    class NewNetPowerCmd : public gcp::util::NetStruct {
    public:


      unsigned int breaker;
      bool power;

      NewNetPowerCmd() {
	NETSTRUCT_UINT(breaker);
	NETSTRUCT_BOOL(power);
      }

    };

    //------------------------------------------------------------
    // The pager command controls the 'pager'
    //------------------------------------------------------------

    class NewNetPagerCmd : public gcp::util::NetStruct {
    public:

      // Enumerate the supported pager states.

      enum PagerState {
	PAGER_ON,     // Turn the pager on
	PAGER_OFF,    // Turn the pager off
	PAGER_IP,     // For internet devices, set an IP address
	PAGER_EMAIL,  // For email pagers, set an email address
	PAGER_ENABLE, // Enable paging
	PAGER_DISABLE // Disable paging
      };

      unsigned int state;              // A PagerState enumerator

      NewNetPagerCmd() {
	NETSTRUCT_UINT(state);              
      }

    };

    //------------------------------------------------------------
    // The NetOptCamCntlCmd controls the optical camera
    //------------------------------------------------------------r.

    class NewNetOptCamCntlCmd : public gcp::util::NetStruct {
    public:


      unsigned int target;  // Which device to control
      bool on;              // Turn device on/off?

      NewNetOptCamCntlCmd() {
	NETSTRUCT_UINT(target);    
	NETSTRUCT_BOOL(on);        
      }

    };

    //------------------------------------------------------------
    // The NetConfigureFrameGrabberCmd configures the frame grabber.
    //------------------------------------------------------------

    class NewNetConfigureFrameGrabberCmd : public gcp::util::NetStruct {
    public:
      unsigned int mask;     // The mask of parameters to configurec
      unsigned int channel;  // The channel number to selec
      unsigned int nCombine; // The number of frames to combine
      bool flatfield;        // If true, flatfield the image

      NewNetConfigureFrameGrabberCmd() {
	NETSTRUCT_UINT(mask);      // The mask of parameters to configurec
	NETSTRUCT_UINT(channel);   // The channel number to selec
	NETSTRUCT_UINT(nCombine);  // The number of frames to combine
	NETSTRUCT_BOOL(flatfield); // If true, flatfield the image
      }
    };

    //------------------------------------------------------------
    // The NetFlatFieldCmd toggles frame grabber flat fielding
    //------------------------------------------------------------

    class NewNetFlatFieldCmd : public gcp::util::NetStruct {
    public:


      bool on;  // If true, flat field frame grabber images 

      NewNetFlatFieldCmd() {
	NETSTRUCT_BOOL(on);  
      }

    };

    //============================================================
    // A class for collecting together various commands that will be
    // sent across the network
    //============================================================

    class NewNetCmd : public gcp::util::NetUnion {
    public:

      // Enumerate structs that are part of this message

      enum {
	NET_INIT_CMD,       // Mandatory controller initialization command 
	NET_SHUTDOWN_CMD,   // A request to shutdown the control system 
	NET_STROBE_CMD,     // A request to take a register snapshot 
	NET_FEATURE_CMD,    // Change the set of feature markers to be
			    // recorded in one or more subsequent
			    // archive frames.

	// Set up commands

	NET_LOCATION_CMD,    // Specify the location of an antenna
	NET_SITE_CMD,        // Specify the location of the experiment

	// Get/Set the value of a register

	NET_GETREG_CMD,      // Change the value of a given register 
	NET_SETREG_CMD,      // Change the value of a given register 

	// Tracking commands

	NET_HALT_CMD,        // Halt the telescope drives 
	NET_STOP_CMD,        // put the telescope in STOP mode
	NET_SCAN_CMD,        // Scan the telescope
	NET_SLEW_CMD,        // Slew to a given az,el,dk 
	NET_TRACK_CMD,       // Append to the track of the current source 
	NET_MOUNT_OFFSET_CMD,// Adjust the az,el,dk tracking offsets 
	NET_EQUAT_OFFSET_CMD,// Adjust the equatorial ra and dec tracking offsets 
	NET_TV_OFFSET_CMD,   // Adjust the tracking offsets to move a
			     // star by a on the tv monitor of the
			     // optical telescope.
	NET_TV_ANGLE_CMD,    // The deck angle at which the vertical
			     // direction on the tv monitor of the
			     // optical telescope matches the
			     // direction of increasing topocentric
			     // elevation.
	NET_SKY_OFFSET_CMD,  // Tell the tracker to continually adjust
			     // the tracking offsets to maintain the
			     // telescope pointed at a given fixed sky
			     // offset from the normal pointing
			     // center.
	NET_DECK_MODE_CMD,   // Tell the tracker how to position the
			     // deck axis while tracking sources.

	// Tracking related

	NET_ATMOS_CMD,       // Manually supply atmospheric conditions
			     // to weather task
	NET_UT1UTC_CMD,      // Append to the UT1-UTC interpolation table 
	NET_EQNEQX_CMD,      // Append to the
			     // equation-of-the-equinoxes
			     // interpolation table

	// Pointing commands

	NET_ENCODER_CALS_CMD,  // Tell the tracker task the current
			       // encoder scales
	NET_ENCODER_LIMITS_CMD,// Tell the tracker task the current
			       // encoder limits
	NET_ENCODER_ZEROS_CMD, // Set the zero points of the telescope
			       // encoders
	NET_SLEW_RATE_CMD,  // Set the slew rate of each telescope axis 
	NET_TILTS_CMD,      // Calibrate the axis tilts of the telescope 
	NET_FLEXURE_CMD,    // Calibrate the axis flexure of the telescope 
	NET_COLLIMATE_CMD,  // Calibrate the collimation of the telescope 
	NET_MODEL_CMD,      // Select the optical or radio pointing model 
	NET_YEAR_CMD,       // Tell the control system what the current year is 

	// Random hardware commands

	NET_GPIB_SEND_CMD,  // Send a message to a given GPIB device  
	NET_GPIB_READ_CMD,  // Read a message from a given GPIB device  
	NET_POWER_CMD,      // Turn the power on/off to an antennas power strip
	NET_PAGER_CMD,      // Turn the pager on or off 

	// Optical camera/frame grabber commands

	NET_OPTCAM_CNTL_CMD, // Control the optical camera
	NET_CONFIGURE_FG_CMD,// Configure the frame grabber 
	NET_FLATFIELD_CMD,   // Toggle flat fielding of frame grabber frames 
      };
      
      // Control commands

      NewNetInitCmd init;          // Mandatory controller
				   // initialization command
      NewNetShutdownCmd shutdown;  // A request to shutdown the control system 
      NewNetFeatureCmd feature;    // Change the set of feature
				   // markers to be recorded in one or
				   // more archive frames. 

      // Set-up commands

      NewNetLocationCmd location;  // Specify the location of an antenna
      NewNetSiteCmd site;          // Specify the location of the experiment

      // Get/Set the value of a register

      NewNetGetregCmd getreg;      // Change the value of a given register 
      NewNetSetregCmd setreg;      // Change the value of a given register 

      // Tracking commands

      NewNetHaltCmd halt;          // Halt the telescope drives 
      NewNetScanCmd scan;          // Scan the telescope
      NewNetSlewCmd slew;          // Slew to a given az,el,dk 
      NewNetTrackCmd track;        // Append to the track of the current source 
      NewNetMountOffsetCmd mount_offset;// Adjust the az,el,dk tracking offsets 
      NewNetEquatOffsetCmd equat_offset;// Adjust the equatorial ra
					// and dec tracking offsets
      NewNetTvOffsetCmd tv_offset;   // Adjust the tracking offsets to
				     // move a star by a on the tv
				     // monitor of the optical
				     // telescope.
      NewNetTvAngleCmd tv_angle;     // The deck angle at which the
				     // vertical direction on the tv
				     // monitor of the optical
				     // telescope matches the direction
				     // of increasing topocentric
				     // elevation.
      NewNetSkyOffsetCmd sky_offset; // Tell the tracker to
				     // continually adjust the
				     // tracking offsets to maintain
				     // the telescope pointed at a
				     // given fixed sky offset from
				     // the normal pointing center.
      NewNetDeckModeCmd deck_mode;   // Tell the tracker how to
				     // position the deck axis while
				     // tracking sources.

      // Tracking related

      NewNetAtmosCmd atmos;         // Manually supply atmospheric
				    // conditions to weather task
      NewNetUt1UtcCmd ut1utc;       // Append to the UT1-UTC
				    // interpolation table
      NewNetEqnEqxCmd eqneqx;       // Append to the
				    // equation-of-the-equinoxes
				    // interpolation table

      // Pointing commands

      NewNetEncoderCalsCmd encoder_cals;    // Tell the tracker task the
					    // encoder scales
      NewNetEncoderLimitsCmd encoder_limits;// Tell the tracker task
					    // the current encoder
					    // limits
      NewNetEncoderZerosCmd encoder_zeros;  // Set the zero points of
					    // the telescope encoders
      NewNetSlewRateCmd slew_rate;  // Set the slew rate of each telescope axis 
      NewNetTiltsCmd tilts;         // Calibrate the axis tilts of the
				    // telescope
      NewNetFlexureCmd flexure;     // Calibrate the axis flexure of
				    // the telescope
      NewNetCollimateCmd collimate; // Calibrate the collimation of
				    // the telescope
      NewNetModelCmd model;         // Select the optical or radio
				    // pointing model
      NewNetYearCmd year;           // Tell the control system what
				    // the current year is

      // Random hardware commands

      NewNetGpibSendCmd gpib_send;  // Send a message to a given GPIB device  
      NewNetGpibReadCmd gpib_read;  // Read a message from a given GPIB device  
      NewNetPowerCmd power;         // Turn the power on/off to an
				    // antennas power strip
      NewNetPagerCmd pager;         // Turn the pager on or off 

      // Optical camera/frame grabber commands

      NewNetOptCamCntlCmd optcam_cntl;// Control the optical camera
      NewNetConfigureFrameGrabberCmd configureFrameGrabber; // Configure
							    // the
							    // frame
							    // grabber
      /**
       * Constructor.
       */
      NewNetCmd();

      /**
       * Destructor.
       */
      virtual ~NewNetCmd();


    }; // End class NewNetCmd

  } // End namespace control
} // End namespace gcp



#endif // End #ifndef GCP_CONTROL_NEWNETCMD_H
