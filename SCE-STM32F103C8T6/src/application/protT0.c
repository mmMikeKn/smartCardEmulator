#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "com_rf_io.h"

#define DTIME_PING 500

static int getHostRespT0(uint8_t* data, int *sz) {
	uint32_t t;
	int cmd;
 for(t = _sysTicks; (cmd = rf_getCmdCode()) == RF_NO_CMD;) {
 	if(!chkVcc()) {	cmd_sendMsg("No resp");	return -1;	}
 	if((_sysTicks - t) > DTIME_PING) {
   int st = cardIO_sendByte(0x60); // NULL T=0 procedure
   if(st < 0) cmd_sendMsg("send NULL(0x60) st: %d", st);
 		t = _sysTicks;
 	}
 }
 *sz = rf_getCmdBody(data, *sz);
	return cmd;
}

void smartCardLifeT0() {
 while(TRUE) {
  uint8_t bufIn[256], bufOut[256];
  int st = cardIO_waitRvc(bufIn, 5, 300);
  if(st == ST_IO_SHUTDOWN) return;
  if(st < 0) { cmd_sendMsg("recv APDU(5) st: %d", st);	return;  }
  int cmd, sz;

  sz = sizeof(bufOut);
  cmd_APDU_IFD_T0(bufIn, 5);

  cmd = getHostRespT0(bufOut, &sz);
  int P3 = bufIn[4];

  switch(cmd) {
  case CMD_APDU_W_ICC_T0:
  	// IFD -> APDU[5]:cardEmu -> Host: CMD_APDU_IFD_T0
  	// IFD <- ADPU INS[1]:cardEmu <- Host:CMD_APDU_W_ICC_T0
  	// IFD -> APDU BODY[..]:cardEmu -> Host:CMD_APDU_BODY_IFD_T0
  	// IFD <- APDU SW:cardEmu <- Host:CMD_APDU_SW_ICC_T0
  	st = cardIO_sendData(bufOut, sz);
   if(st < 0) { cmd_sendMsg("send APDU(INS) err: %d", st);	return;  }
   st = cardIO_waitRvc(bufIn, P3, 300);
   if(st < 0) { cmd_sendMsg("rcv APDU data err: %d", st);	return;  }
   cmd_APDU_BODY_IFD_T0(bufIn, P3);
   sz = sizeof(bufOut);
   cmd = getHostRespT0(bufOut, &sz);
   if(cmd < 0) return;
   if(cmd != CMD_APDU_SW_ICC_T0) {
   	cmd_sendMsg("wrong cmd seq.(T0.W):%X", cmd);
   }
   cardIO_sendData(bufOut, sz);
   if(st < 0) { cmd_sendMsg("send APDU SW err: %d", st);	return;  }
  	break;
  case CMD_APDU_R_ICC_T0:
  	// IFD -> APDU[5]:cardEmu -> Host: CMD_APDU_IFD_T0
  	// IFD <- ADPU BODY+SW[..]:cardEmu <- Host:CMD_APDU_R_ICC_T0
  	cardIO_sendData(bufOut, sz);
   if(st < 0) { cmd_sendMsg("send APDU data+SW err: %d", st);	return;  }
  	break;
  case -1:
   return;
  default:
  	cmd_sendMsg("wrong cmd seq (T0.A):%X", cmd);
  	return;
  }
 }
}
