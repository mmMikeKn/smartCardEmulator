#ifndef COM_RF_IO_H_
#define COM_RF_IO_H_

void rf_procChar(unsigned char c);
void	rf_putChar();

enum {
	CMD_SHOW_TEXT = 0x0FF,
	CMD_DBG_DUMP = 0x0F1,
	CMD_ATR_EVENT = 0x6600,
	CMD_ATR_RQ = 0x6601,
	CMD_ATR_DATA = 0x6602,

	CMD_APDU_IFD_T0 = 0x7701,
	CMD_APDU_W_ICC_T0 = 0x7702,
	CMD_APDU_BODY_IFD_T0 = 0x7703,
	CMD_APDU_SW_ICC_T0 = 0x7704,
	CMD_APDU_R_ICC_T0 = 0x7705,

	CMD_IFD_T1 = 0x5501,
	CMD_ICC_T1 = 0x5502,
} RF_CMD_CODE;


#define RF_NO_CMD -1
#define RF_WRONG_DATA -2
#define RF_TIMEOUT_DATA -3

void rf_init();
unsigned char rf_isTxEmpty();
short rf_getCmdCode(void);
uint8_t rf_getCmdBody(uint8_t *data, int maxSz);

void cmd_sendMsg(const char* str, ...);
void cmd_APDU_IFD_T0(const uint8_t *data, int sz);
void cmd_APDU_BODY_IFD_T0(const uint8_t *data, int sz);

void cmd_IFD_T1(const uint8_t *data, int sz);
void cmd_ICC_T1(const uint8_t *data, int sz);
void cmd_eventATR(uint8_t isWarm);
void cmd_ATR_RQ();
void cmd_dbg_dump(const uint8_t *data, int sz);

#endif /* COM_RF_IO_H_ */
