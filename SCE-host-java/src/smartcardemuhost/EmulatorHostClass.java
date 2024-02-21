package smartcardemuhost;

import smartcardemuhost.pcsc.PcScClass;
import smartcardemuhost.pcsc.PcscException;
import gnu.io.CommPortIdentifier;
import gnu.io.NoSuchPortException;
import gnu.io.PortInUseException;
import gnu.io.SerialPort;
import gnu.io.UnsupportedCommOperationException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;

public class EmulatorHostClass implements Runnable {

 EmulatorHostRcvInterface notify = null;
 boolean isStoped = false;
 private Thread _thread;
 String serPortName;
 private InputStream serIn = null;
 private OutputStream serOut = null;
 private SerialPort serPort = null;
 //---------
 final static int CMD_SHOW_TEXT = 0x00FF;
 final static int CMD_DBG_DUMP = 0x0F1;
 final static int CMD_ATR_EVENT = 0x6600;
 final static int CMD_ATR_RQ = 0x6601;
 final static int CMD_ATR_DATA = 0x6602;
 final static int CMD_APDU_IFD_T0 = 0x7701;
 final static int CMD_APDU_W_ICC_T0 = 0x7702;
 final static int CMD_APDU_BODY_IFD_T0 = 0x7703;
 final static int CMD_APDU_SW_ICC_T0 = 0x7704;
 final static int CMD_APDU_R_ICC_T0 = 0x7705;
 final static int CMD_IFD_T1 = 0x5501;
 final static int CMD_ICC_T1 = 0x5502;
 //---------
 private String ATR;
 byte rf_RxBuffer[] = new byte[512];
 int rf_RxBufferPtr = 0;
 int rf_RxDataSz = 0;
 final static byte STX = 0x02;
 // T=1 params
 int IFSD_len = 32 - 4;
 private ArrayList<Byte> tiedBlocDataToSendT1 = new ArrayList<Byte>();
 private ArrayList<Byte> tiedBlkDataToRecvT1 = new ArrayList<Byte>();
 private static int sequenceSndBlockI = 0, sequenceRcvBlockI = 0;
 private boolean hasVerifyPIN_filter = false;

 public EmulatorHostClass(String serPortName, String ATR, boolean hasVerifyPIN_filter) {
  this.serPortName = serPortName;
  this.ATR = ATR;
  this.hasVerifyPIN_filter = hasVerifyPIN_filter;
 }

 public void setCallBack(EmulatorHostRcvInterface notify) {
  this.notify = notify;
 }

 public void start() throws EmulatorHostException {
  CommPortIdentifier portId;
  try {
   portId = CommPortIdentifier.getPortIdentifier(serPortName);
  } catch (NoSuchPortException ex) {
   throw new EmulatorHostException("NoSuchPort Err: '" + serPortName + "' " + ex.getMessage());
  }
  try {
   serPort = (SerialPort) portId.open("smd", 100);
  } catch (PortInUseException ex) {
   throw new EmulatorHostException("PortInUse Err: '" + serPortName + "' " + ex.getMessage());
  }
  try {
   serPort.setSerialPortParams(115200, SerialPort.DATABITS_8, SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
   serPort.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
  } catch (UnsupportedCommOperationException ex) {
   throw new EmulatorHostException("UnsupportedCommOperation Err: '" + serPortName + "' " + ex.getMessage());
  }
  try {
   serIn = serPort.getInputStream();
   serOut = serPort.getOutputStream();
  } catch (IOException ex) {
   throw new EmulatorHostException("get Stream Err: '" + serPortName + "' " + ex.getMessage());
  }
  _thread = new Thread(this);
  _thread.setDaemon(true);
  _thread.start();
 }

 public void stop() {
  isStoped = true;
  if (_thread != null) {
   try {
    _thread.join();
   } catch (InterruptedException ine) {
   }
  }
 }

 @Override
 protected void finalize() throws Throwable {
  super.finalize();
  try {
   close();
  } finally {
  }
 }

 public void close() {
  if (serPort != null) {
   serPort.close();
  }
 }

 // STX || code[2] || sz[2] || body[sz] || lrc
 void sendCmd(int code, byte body[]) throws EmulatorHostException {
  byte rf_TxBuffer[] = new byte[512];
  int sz = body.length;
  rf_TxBuffer[0] = STX;
  rf_TxBuffer[1] = (byte) (code >> 8);
  rf_TxBuffer[2] = (byte) code;
  rf_TxBuffer[3] = (byte) (sz >> 8);
  rf_TxBuffer[4] = (byte) sz;
  System.arraycopy(body, 0, rf_TxBuffer, 5, sz);
  rf_TxBuffer[5 + sz] = 0;
  for (int i = 0; i < (sz + 4); i++) {
   rf_TxBuffer[5 + sz] ^= rf_TxBuffer[i + 1];
  }
  try {
   serOut.write(rf_TxBuffer, 0, 5 + sz + 1);
   serOut.flush();
   try {
    Thread.sleep(100);
   } catch (InterruptedException ex) {
   }
  } catch (IOException ex) {
   throw new EmulatorHostException("write to Stream Err:" + serPortName + " " + ex.getMessage());
  }
 }

 public void run() {
  while (!this.isStoped) {
   try {
    Thread.sleep(100);
   } catch (InterruptedException ex) {
   }
   try {
    while (serIn.available() > 0 && !this.isStoped) {
     byte c = (byte) serIn.read();
     if (rf_RxBufferPtr == 0 && c != STX) {
      continue;
     }
     rf_RxBuffer[rf_RxBufferPtr++] = c;
     if (rf_RxBufferPtr == 7) {
      rf_RxDataSz = (((int) rf_RxBuffer[5] << 8) & 0xFF00) | ((int) rf_RxBuffer[6] & 0x0ff);
     }
     int crcOfs = rf_RxDataSz + 7;
     if (rf_RxBufferPtr > crcOfs) {
      for (int i = 1; i < crcOfs; i++) {
       rf_RxBuffer[crcOfs] ^= rf_RxBuffer[i];
      }
      if (rf_RxBuffer[crcOfs] != 0) {
       notify.notifyErr("Wrong LRC of recived data ", toHexString(rf_RxBuffer, 0, rf_RxBufferPtr));
      } else {
       msgRcvCmd();
      }
      rf_RxBufferPtr = 0;
     }
    }
   } catch (Exception e) {
    e.printStackTrace();
   }
  }

 }

 // STX[1] || code[2] || time[2] || sz[2] || body[sz] || lrc
 void msgRcvCmd() {
  int sc_cmd = (((int) rf_RxBuffer[1] & 0x0FF) << 8) | ((int) rf_RxBuffer[2] & 0x0FF);
  int len = rf_RxBufferPtr, i;
  byte body[] = new byte[len - 8];
  int timeStamp = (((int) rf_RxBuffer[3] & 0x0FF) << 8) | ((int) rf_RxBuffer[4] & 0x0FF);
  System.arraycopy(rf_RxBuffer, 7, body, 0, rf_RxBufferPtr - 8);
//  System.out.print(rf_cmd+":"); System.out.println(new String(rf_RxBuffer));
  try {
   switch (sc_cmd) {
    case CMD_SHOW_TEXT:
     String msg = new String(body);
     notify.notifyRcvDebug("[" + timeStamp + "] MSG:'" + msg + "'\n");
     break;
    case CMD_DBG_DUMP:
     notify.notifyRcvDebug("[" + timeStamp + "] DBG DUMP:" + toHexString(body) + "\n");
     break;
    case CMD_APDU_IFD_T0:
     notify.notifyRcvDebug("[" + timeStamp + "] T0 APDU:" + toHexString(body));
     // TODO
     if ((body[1] == (byte) 0xA4 && body[2] == (byte) 0x04)
             || body[1] == (byte) 0xAE || body[1] == (byte) 0x88
             || body[1] == (byte) 0xA8) {
      byte[] data = new byte[1];
      data[0] = body[1];
      sendCmd(CMD_APDU_W_ICC_T0, data);
      notify.notifyRcvDebug("[" + timeStamp + "] W[" + toHexString(data) + "]:");
     } else {
      byte[] data = execAPDU(body);
      sendCmd(CMD_APDU_R_ICC_T0, data);
     }
     break;
    case CMD_APDU_BODY_IFD_T0:
     notify.notifyRcvDebug(" [" + timeStamp + "] DATA:" + toHexString(body));
     byte[] data = new byte[2];
     data[0] = (byte) 0x90;
     data[1] = (byte) 0x00;
     notify.notifyRcvDebug(" [" + timeStamp + "] SW:" + toHexString(data) + "\n");
     sendCmd(CMD_APDU_SW_ICC_T0, data);
     break;
    case CMD_ATR_EVENT:
     sequenceSndBlockI = sequenceRcvBlockI = 0;
     notify.notifyRcvDebug("[" + timeStamp + "] ATR EVENT: " + (body[0] == 0 ? "COLD" : "WARM") + "\n");
     break;
    case CMD_ATR_RQ:
     //byte atr[] = hex2bin("3B6B00000031C0643F680100079000"); // T=0
     byte atr[] = hex2bin(ATR); // T=1
     notify.notifyRcvDebug("ATR set:" + toHexString(atr) + "\n");
     sendCmd(CMD_ATR_DATA, atr);
     break;
    case CMD_IFD_T1:
     procT1(timeStamp, body);
     break;
    default:
     notify.notifyRcvDebug("Undefined [" + timeStamp + "] cmd:" + toHexString(rf_RxBuffer, 0, rf_RxBufferPtr));
   }
  } catch (Exception ex) {
   ex.printStackTrace();
  }
 }
 //----------------------------------------------------------------------------

 private byte[] execAPDU(byte[] apdu_req) throws PcscException {
  return PcScClass.sendCmd(apdu_req);
  /*
  byte apdu_resp[];
  if (apdu_req.length > 5) {
  apdu_resp = new byte[2];
  } else {
  apdu_resp = new byte[(((int) apdu_req[4]) & 0x0FF) + 2];
  for (int i = 0; i < apdu_resp.length; i++) {
  apdu_resp[i] = 0x55;
  }
  }
  apdu_resp[apdu_resp.length - 2] = (byte) 0x90;
  apdu_resp[apdu_resp.length - 1] = (byte) 0x00;
  return apdu_resp;
   */
 }

 void procT1(int timeStamp, byte t1_req[]) throws EmulatorHostException, PcscException {
  notify.notifyRcvDebug("[" + timeStamp + "] ICC <-IFD:" + toHexString(t1_req) + "     ");
  int len = (int) t1_req[2] & 0x0FF;
  if (len + 4 != t1_req.length) {
   notify.notifyRcvDebug("wrong T1.length\n");
  }
  byte lrc = 0;
  for (int i = 0; i < (len + 3); i++) {
   lrc ^= t1_req[i];
  }
  if (lrc != t1_req[t1_req.length - 1]) {
   notify.notifyRcvDebug("wrong T1.lrc\n");
  }
  byte t1_answ[] = null;
  if ((t1_req[1] & 0x80) == 0) { // I-block
//=============================================
   sequenceRcvBlockI++;
   if ((t1_req[1] & 0x20) != 0) { // Tied Block
    notify.notifyRcvDebug("\n tied block:" + toHexString(t1_req, 3, len) + "\n");
    for (int i = 0; i < len; i++) {
     tiedBlkDataToRecvT1.add(t1_req[3 + i]);
    }
    t1_answ = new byte[4];
    t1_answ[1] = (byte) 0x80; // R-block
    if ((sequenceRcvBlockI & 0x01) != 0) {
     t1_answ[1] |= (byte) 0x10; // seq bit
    }
   } else {
    byte apdu_req[] = new byte[tiedBlkDataToRecvT1.size() + len];
    for (int i = 0; i < tiedBlkDataToRecvT1.size(); i++) {
     apdu_req[i] = tiedBlkDataToRecvT1.get(i);
    }
    System.arraycopy(t1_req, 3, apdu_req, tiedBlkDataToRecvT1.size(), len);
    tiedBlkDataToRecvT1.clear();
    String ahex = toHexString(apdu_req);
    notify.notifyRcvDebug("APDU: " + ahex + "\n");
    byte apdu_resp[] = hasVerifyPIN_filter && ahex.startsWith("002000")
            ? new byte[]{(byte) 0x90, (byte) 0}
            : execAPDU(apdu_req);
    notify.notifyRcvDebug("APDU resp: " + toHexString(apdu_resp) + "\n");
    if (apdu_resp.length > IFSD_len) {
     t1_answ = new byte[4 + IFSD_len];
     System.arraycopy(apdu_resp, 0, t1_answ, 3, IFSD_len);
     t1_answ[1] |= 0x20; // chain bit
     for (int i = IFSD_len; i < apdu_resp.length; i++) {
      tiedBlocDataToSendT1.add(apdu_resp[i]);
     }
    } else {
     t1_answ = new byte[4 + apdu_resp.length];
     System.arraycopy(apdu_resp, 0, t1_answ, 3, apdu_resp.length);
    }
    if ((sequenceSndBlockI & 0x01) != 0) {
     t1_answ[1] |= 0x40; // block seq.
    }
    sequenceSndBlockI++;
   }
   //=============================================
  } else if ((t1_req[1] & 0xC0) == 0x80) { //R-block
   notify.notifyRcvDebug("R block\n");
   if (tiedBlocDataToSendT1.size() <= IFSD_len) {
    t1_answ = new byte[4 + tiedBlocDataToSendT1.size()];
   } else {
    t1_answ = new byte[4 + IFSD_len];
   }
   for (int i = 3; i < (t1_answ.length - 1); i++) {
    t1_answ[i] = tiedBlocDataToSendT1.get(0);
    tiedBlocDataToSendT1.remove(0);
   }
   if ((sequenceSndBlockI & 0x01) != 0) {
    t1_answ[1] |= 0x40; // block seq.
   }
   sequenceSndBlockI++;
  } else if ((t1_req[1] & 0xC0) == 0xC0) { // S-block
   switch (t1_req[1] & 0x3F) {
    case 0x00:
     t1_answ = new byte[4];
     notify.notifyRcvDebug("RESYNCH T1 rq\n");
     break;
    case 0x02:
     t1_answ = new byte[4];
     notify.notifyRcvDebug("ABORT T1 rq\n");
     break;
    case 0x03:
     t1_answ = new byte[4 + 1];
     notify.notifyRcvDebug("WTX T1 rq\n");
     t1_answ[3] = t1_req[3];
     break;
    case 0x01:
     t1_answ = new byte[4 + 1];
     notify.notifyRcvDebug("IFS T1 rq\n"); // T1 data size;
     IFSD_len = ((int) t1_req[3]) & 0x0FF - 4;
     t1_answ[3] = (byte) 0xFE;
     break;
    default:
     notify.notifyRcvDebug("undefined T1.PCB [11xx xxxx]\n"); // T1 data size;
    }
   t1_answ[1] = (byte) (t1_req[1] | 0x20);
  } else {
   notify.notifyRcvDebug("undefined T1.PCB\n");
  }

  t1_answ[0] = t1_req[0];
  t1_answ[2] = (byte) (t1_answ.length - 4);
  for (int i = 0; i < t1_answ.length - 1; i++) {
   t1_answ[t1_answ.length - 1] ^= t1_answ[i];
  }
  sendCmd(CMD_ICC_T1, t1_answ);
  notify.notifyRcvDebug("ICC -> IFD:" + toHexString(t1_answ) + "\n");
 }
//------------------------------------------------------------------------------
 static private final char[] _hexChars = {'0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

 public static String toHexString(byte src[]) {
  return toHexString(src, 0, src.length);
 }

 public static String toHexString(byte src[], int offset, int len) {
  if (src == null) {
   return "(null)";
  }

  StringBuilder out = new StringBuilder(256);

  for (int i = offset; i < (len + offset) && i < src.length; i++) {
   out.append(_hexChars[(src[i] >> 4) & 0x0f]);
   out.append(_hexChars[src[i] & 0x0f]);
  }
  return out.toString();
 }

 public static void hex2bin(String hex, byte[] out, int offset) {
  for (int i = 0; i < hex.length(); i += 2) {
   String toParse = hex.substring(i, i + 2);
   out[offset + (i / 2)] = (byte) Integer.parseInt(toParse, 16);
  }
 }

 public static byte[] hex2bin(String hex) {
  byte[] tmp = new byte[hex.length() / 2];
  hex2bin(hex, tmp, 0);
  return tmp;
 }
}
