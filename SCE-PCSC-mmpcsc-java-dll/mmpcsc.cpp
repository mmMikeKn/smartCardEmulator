// mmpcsc.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <io.h>
#include "winscard.h"
#include "smartcardemuhost_pcsc_PcScClass.h"


#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

SCARDCONTEXT	ContextHandle;
SCARDHANDLE	CardHandle;
DWORD ActiveProtocol = -77;
DWORD lastRet = SCARD_S_SUCCESS;

std::string bin2hex(const unsigned char *src, int size) {
 std::stringstream o;
 for(int i = 0; i < size; i++) 
  o << std::hex << std::setw(2) << std::uppercase << std::setfill('0') << (unsigned int)src[i];
 return o.str();
}


JNIEXPORT jstring JNICALL Java_smartcardemuhost_pcsc_PcScClass_getLastError(JNIEnv *env, jclass) {
 DWORD ret = lastRet;
 char err_msg[256];

 switch (ret) {
  case SCARD_E_CANCELLED: strcpy(err_msg,"The action was cancelled by an SCardCancel request."); break;
  case SCARD_E_CANT_DISPOSE: strcpy(err_msg,"The system could not dispose of the media in the requested manner."); break;
  case SCARD_E_CARD_UNSUPPORTED: strcpy(err_msg,"The smart card does not meet minimal requirements for support."); break;
  case SCARD_E_DUPLICATE_READER: strcpy(err_msg,"The reader driver didn't produce a unique reader name."); break;
  case SCARD_E_INSUFFICIENT_BUFFER: strcpy(err_msg,"The data buffer to receive returned data is too small for the returned data."); break;
  case SCARD_E_INVALID_ATR: strcpy(err_msg,"An ATR obtained from the registry is not a valid ATR string."); break;
  case SCARD_E_INVALID_HANDLE: strcpy(err_msg,"The supplied handle was invalid."); break;
  case SCARD_E_INVALID_PARAMETER: strcpy(err_msg,"One or more of the supplied parameters could not be properly interpreted."); break;
  case SCARD_E_INVALID_TARGET: strcpy(err_msg,"Registry startup information is missing or invalid."); break;
  case SCARD_E_INVALID_VALUE: strcpy(err_msg,"One or more of the supplied parametersT values could not be properly interpreted."); break;
  case SCARD_E_NOT_READY: strcpy(err_msg,"The reader or card is not ready to accept commands."); break;
  case SCARD_E_NOT_TRANSACTED: strcpy(err_msg,"An attempt was made to end a non-existent transaction."); break;
  case SCARD_E_NO_MEMORY: strcpy(err_msg,"Not enough memory available to complete this command."); break;
  case SCARD_E_NO_SERVICE: strcpy(err_msg,"The Smart card resource manager is not running."); break;
  case SCARD_E_NO_SMARTCARD: strcpy(err_msg,"The operation requires a smart card but no smart card is currently in the device."); break;
  case SCARD_E_PCI_TOO_SMALL: strcpy(err_msg,"The PCI Receive buffer was too small."); break;
  case SCARD_E_PROTO_MISMATCH: strcpy(err_msg,"The requested protocols are incompatible with the protocol currently in use with the card."); break;
  case SCARD_E_READER_UNAVAILABLE: strcpy(err_msg,"The specified reader is not currently available for use."); break;
  case SCARD_E_READER_UNSUPPORTED: strcpy(err_msg,"The reader driver does not meet minimal requirements for support."); break;
  case SCARD_E_SERVICE_STOPPED: strcpy(err_msg,"The Smart card resource manager has shut down."); break;
  case SCARD_E_SHARING_VIOLATION: strcpy(err_msg,"The card cannot be accessed because of other connections outstanding."); break;
  case SCARD_E_SYSTEM_CANCELLED: strcpy(err_msg,"The action was cancelled by the system presumably to log off or shut down."); break;
  case SCARD_E_TIMEOUT: strcpy(err_msg,"The user-specified timeout value has expired."); break;
  case SCARD_E_UNKNOWN_CARD: strcpy(err_msg,"The specified card name is not recognized."); break;
  case SCARD_E_UNKNOWN_READER: strcpy(err_msg,"The specified reader name is not recognized."); break;
  case SCARD_F_COMM_ERROR: strcpy(err_msg,"An internal communications error has been detected."); break;
  case SCARD_F_INTERNAL_ERROR: strcpy(err_msg,"An internal consistency check failed."); break;
  case SCARD_F_UNKNOWN_ERROR: strcpy(err_msg,"An internal error has been detected but the source is unknown."); break;
  case SCARD_F_WAITED_TOO_LONG: strcpy(err_msg,"An internal consistency timer has expired."); break;
  case SCARD_S_SUCCESS: strcpy(err_msg,"OK"); break;
  case SCARD_W_REMOVED_CARD: strcpy(err_msg,"The card has been removed so that further communication is not possible."); break;
  case SCARD_W_RESET_CARD: strcpy(err_msg,"The card has been reset so any shared state information is invalid."); break;
  case SCARD_W_UNPOWERED_CARD: strcpy(err_msg,"Power has been removed from the card so that further communication is not possible."); break;
  case SCARD_W_UNRESPONSIVE_CARD: strcpy(err_msg,"The card is not responding to a reset."); break;
  case SCARD_W_UNSUPPORTED_CARD: strcpy(err_msg,"The reader cannot communicate with the card due to ATR configuration conflicts."); break;
  default: sprintf(err_msg,"0x%LX error code.", ret);
 }
 return env->NewStringUTF(err_msg);
}


JNIEXPORT jstring JNICALL Java_smartcardemuhost_pcsc_PcScClass_getReaderList(JNIEnv *env, jclass) {
 char list_drv[2048];
 DWORD l = sizeof(list_drv);
 int n = 0;
 
 memset(list_drv, 0, sizeof(list_drv));
 lastRet = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &ContextHandle);
 if(lastRet != SCARD_S_SUCCESS) {
  std::cout << "SCardEstablishContext result = 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << lastRet << std::endl;
  return NULL;
 }
 lastRet = SCardListReaders(ContextHandle, 0, list_drv, &l);
 if(lastRet != SCARD_S_SUCCESS) {
  std::cout << "SCardListReaders result = 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << lastRet << std::endl;
  return NULL;
 }
 SCardReleaseContext(ContextHandle);
 for(int i = 0; i < ((int)l-1); i++) 
  if(list_drv[i] == 0) list_drv[i] = '|';
 return env->NewStringUTF(list_drv);
}

JNIEXPORT jbyteArray JNICALL Java_smartcardemuhost_pcsc_PcScClass_resetCard(JNIEnv *env, jclass, jstring rname) {
 jboolean iscopy;
 jbyteArray jb;

 DWORD AtrLen, card_st, len;
 char pResponseBuffer[256+2];
 unsigned char pAtr[256+6];

 SCardDisconnect(CardHandle, SCARD_UNPOWER_CARD);
 lastRet = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &ContextHandle);
 if(lastRet != SCARD_S_SUCCESS) {
  std::cout << "SCardEstablishContext result = 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << lastRet << std::endl;
  return NULL;
 }

 int len_out = 0;
 const char *tmp = env->GetStringUTFChars(rname, &iscopy);
 lastRet = SCardConnect(ContextHandle, tmp, SCARD_SHARE_EXCLUSIVE, 
                    SCARD_PROTOCOL_T1 | SCARD_PROTOCOL_T0, 
                    &CardHandle, &ActiveProtocol);
 env->ReleaseStringUTFChars(rname, tmp);
 if(lastRet == SCARD_W_REMOVED_CARD) { 
  jb= env->NewByteArray(0);
  return jb; 
 } 
 if(lastRet != SCARD_S_SUCCESS) { 
  std::cout << "SCardConnect result = 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << lastRet << std::endl;
  return NULL; 
 }
 len = sizeof(pResponseBuffer);
 AtrLen = sizeof(pAtr);
 lastRet = SCardStatus(CardHandle, pResponseBuffer, &len, &card_st, &ActiveProtocol, pAtr, &AtrLen);
 if(lastRet != SCARD_S_SUCCESS) { 
  std::cout << "SCardStatus result = 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << lastRet << std::endl;
  return NULL; 
 }
 if(card_st == SCARD_SPECIFIC) {
  std::cout << std::endl << "ATR:" << bin2hex(pAtr, AtrLen) << " " << std::flush; 

  jb= env->NewByteArray(AtrLen);
  env->SetByteArrayRegion(jb, 0, AtrLen, (jbyte *)pAtr);
  AtrLen; 
 } else {
  jb= env->NewByteArray(0);
 }
 return jb; 
}

JNIEXPORT void JNICALL Java_smartcardemuhost_pcsc_PcScClass_closeCard(JNIEnv *, jclass) {
 SCardDisconnect(CardHandle, SCARD_UNPOWER_CARD);
}


JNIEXPORT jbyteArray JNICALL Java_smartcardemuhost_pcsc_PcScClass_sendCmd(JNIEnv *env, jclass, jbyteArray jcmd) {
 unsigned char cmd[512];
 unsigned char pResponseBuffer[512];
 DWORD ResponseLength;

 unsigned short len = env->GetArrayLength(jcmd); 
 jbyte *p = env->GetByteArrayElements(jcmd, 0);
 memcpy(cmd, p, len);
 env->ReleaseByteArrayElements(jcmd, p, 0);

 std::cout << std::endl << "cmd:" << bin2hex(cmd, len) << " " << std::flush; 
 ResponseLength = sizeof(pResponseBuffer);
 lastRet = SCardTransmit(CardHandle, (ActiveProtocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0: SCARD_PCI_T1,
                          cmd, len, NULL, pResponseBuffer, &ResponseLength);
 if(lastRet != SCARD_S_SUCCESS) {
  std::cout << "SCardTransmit result = 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << lastRet << std::endl;
  return NULL;
 }
 std::cout << "ret:" << bin2hex(pResponseBuffer, ResponseLength) << std::endl;
 jbyteArray jb = env->NewByteArray(ResponseLength);
 env->SetByteArrayRegion(jb, 0, ResponseLength, (jbyte*)pResponseBuffer);
 return jb;
}


#ifdef _MANAGED
#pragma managed(pop)
#endif

