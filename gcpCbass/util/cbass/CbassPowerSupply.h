#ifndef GCP_UTIL_CBASSPOWERSUPPLY_H
#define GCP_UTIL_CBASSPOWERSUPPLY_H

/*
 *  CbassPowerDupply.h
 *  cbass_interface
 *
 *  Created by Stephen Muchovej on 10.12.2010
 *
 */

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include <vector>

#include "gcp/util/common/Directives.h"
#include "gcp/util/common/FdSet.h"

#if DIR_HAVE_USB
#include "usb.h"
#endif

#define PS_MAX_READ_BUF 128
#define BRATE B9600
#define PS_TIMEOUT_USEC 200000
#define PS_READ_ATTEMPTS_TIMEOUT 5
#define PS_SETVOLT_TIMEOUT 10
#define PS_BUF_LIMIT 'x'
#define PS_VOLT_STEPSIZE 0.02
#define PS_VOLT_GATE_ALLOW 0.03
#define PS_VOLT_CURRENT_ALLOW 0.03
#define PS_VOLT_WAIT 250  // ms



// DU to voltage conversions
// Measured empirically, Sept 30, 2010.
#define PS_VOLT_M_GATE  -0.002941 // volts/DU
#define PS_VOLT_B_GATE  0.6169 // volts
#define PS_VOLT_M_DRAIN 0.00982 // volts/DU
#define PS_VOLT_B_DRAIN 0.01089 // volts
#define PS_GAIN_M_CURRENT 0.0491 // mA/DU
#define PS_GAIN_B_CURRENT 0.1275 // mA

const float drain_stepsize[6] = {3.914,5.180,7.789,3.924,5.185,7.777}; // DU/Step
const float current_stepsize[6] = {4.33,4.37,7.36,4.35,4.38,7.43}; // DU/Step
const float max_drainvolt[6] = {3.730,4.930,7.399,3.730,4.930,7.409}; // Volts
const float mindraincurrent_slope[6] = {0,0,0,0,0,0}; // mA/V 
const float mindraincurrent_intercept[6] = {0,0,0,0,0,0}; // mA
const float maxdraincurrent_slope[6] = {4.74,4.67,4.61,4.66,4.58,4.64}; // mA/V 
const float maxdraincurrent_intercept[6] = {0.046,0.14,0.14,0.076,0.084,0.19}; // mA
const int nonlin_current = 5; // DU
const int nonlin_gate = 145; // DU


namespace gcp {
  namespace util {

    // Class to Communicate with the Power Supply

    class CbassPowerSupply { 
    public:

      /**
       *  Constructor
       */
      CbassPowerSupply();

      /**
       * Destructor 
       */ 
      virtual ~CbassPowerSupply();


      /**
       * The set of file descriptors to be watched for readability.
       */
      gcp::util::FdSet fdSet_;



      /**
       * if we are connected
       */ 
      bool connected_;

      /**
       * if we should be querying voltage
       */ 
      bool queryBias_;

      /**
       * file descriptor associated with power supply
       */ 
      int fd_;

      /**
       * return values
       */ 
      float retVal_;
      char  returnString_[PS_MAX_READ_BUF];
      float volts_[3];
      float allVolts_[36];

      /**
       * function to connect
       */ 
      void psConnect();

      /**
       * disconnect
       */
      void psDisconnect();

      /**
       * select
       */ 
      int waitForResponse();


      /**
       *  Low-level commands
       */
      int psWrite(char* command);
      int psWrite(char* command, int len);
      //     int psRead();
      //      int psRead2();
      int readArduino();

      /**
       *  Mid-level commands
       */ 
      void psSetModule(char* modStage);
      void psChangeVoltage(char* voltRequest);
      void psGetVoltage();
      void psGetVoltageOld();

      /** 
       * High-level commands
       */
      int psSetVoltage(int modsel, int stagesel, float gatevolt, float currvolt);
      int psGetAllVoltages();

      /**
       *  Command types
       */
      enum Command {
	INVALID           ,
      };

      /**
       *  Generalized pack&issue command 
       */
      int issueCommand(Command type);
      int issueCommand(Command type, unsigned char address);
      int issueCommand(Command type, unsigned char* period);
      int issueCommand(Command type, unsigned char address, unsigned char* period);

    private:
      void printBits(unsigned char feature);

    }; // End class CbassPowerSupply
  }; // End namespace util
}; // End namespace gcp
#endif
