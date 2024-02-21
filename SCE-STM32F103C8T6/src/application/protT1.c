#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "com_rf_io.h"

#define DTIME_PING 2000

static int getHostRespT1(uint8_t* data, int *sz) {
	uint32_t t;
	int cmd;
 for(t = _sysTicks; (cmd = rf_getCmdCode()) == RF_NO_CMD;) {
 	if(!chkVcc()) {	cmd_sendMsg("No resp (vcc off)");	return -1;	}
 	if((_sysTicks - t) > DTIME_PING) {
 		{	cmd_sendMsg("No resp");	return -1;	}
 		t = _sysTicks;
 	}
 }
 *sz = rf_getCmdBody(data, *sz);
	return cmd;
}


void smartCardLifeT1() {
	while(TRUE) {
  uint8_t bufIn[512], bufOut[512];
  int st = cardIO_waitRvc(bufIn, 3, 100);
  if(st == ST_IO_SHUTDOWN) return;
  if(st < 0) { cmd_sendMsg("recv T1 hd st: %d", st);	return;  }
  if(bufIn[0] == 0xff) { // PPS request
  	int pps_len = 3;
  	if((bufIn[1] & 0x10) != 0) {
  		cardIO_waitRvc(bufIn+3, 1, 100);
  		pps_len++;
  	}
  	if((bufIn[1] & 0x20) != 0) {
  		cardIO_waitRvc(bufIn+4, 1, 100);
  		pps_len++;
  	}
  	if((bufIn[1] & 0x40) != 0) {
  		cardIO_waitRvc(bufIn+5, 1, 100);
  		pps_len++;
  	}
  	delayMs(5);
  	cardIO_sendData(bufIn, pps_len);
  	continue;
  }

 	uint8_t len = bufIn[2];
 	st = cardIO_waitRvc(bufIn+3, len+1, 100); // Epilogue field = support LRC only
  if(st < 0) { cmd_sendMsg("recv T1 inf+lrc st: %d", st);	return;  }

  cmd_IFD_T1(bufIn, len+4);
  int sz = sizeof(bufOut);
  int cmd = getHostRespT1(bufOut, &sz);
  if(cmd < 0) return;
  if(cmd != CMD_ICC_T1) {
  	cmd_sendMsg("wrong cmd seq.(T1):%X", cmd);
  }

  cardIO_sendData(bufOut, sz);
  if(st < 0) { cmd_sendMsg("send T1 resp st: %d", st);	return;  }
	}
}
