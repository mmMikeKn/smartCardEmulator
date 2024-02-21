package smartcardemuhost;

import smartcardemuhost.pcsc.PcScClass;
import smartcardemuhost.pcsc.PcscException;

public class Main implements EmulatorHostRcvInterface {

 static String pcscReaderName;

 public static byte[] getCardATR() throws PcscException {
  System.out.println("source:'" + pcscReaderName + "'");
  return PcScClass.waitCard(pcscReaderName);
 }

 public static void main(String[] args) throws Exception {

  boolean hasVerifyPIN_filter = false;
  if (System.getProperty("VerifyPin-crack", "No").equals("Yes")) {
   hasVerifyPIN_filter = true;
  }
  String port = System.getProperty("com-port", "COM4");
  System.out.println("com port:" + port);

  try {
   pcscReaderName = System.getProperty("pcsc", "Identive Virtual Reader 0");
   String list[] = PcScClass.getReadersArray();
   System.out.println("reader list:");
   for (String r : list) {
    System.out.println(r);
   }
  } catch (Exception e) {
   System.out.println(PcScClass.getError());
   throw e;
  }
  String a = EmulatorHostClass.toHexString(getCardATR());
  String ATR = System.getProperty("atr", "-");
  if (ATR.length() < 3) {
   ATR = a;
  } else {
   System.out.println("Command line ATR:" + ATR);
  }
  EmulatorHostClass cmd = new EmulatorHostClass(port, ATR, hasVerifyPIN_filter);
  cmd.setCallBack(new Main());
  cmd.start();
  try {
   while (true) {
    Thread.sleep(1000);
   }
   // TODO
  } catch (InterruptedException ex) {
   cmd.stop();
  }
 }

 public void notifyRcvDebug(String msg) {
  System.out.print(msg);
 }

 public void notifyErr(String info, String dmp) {
  System.out.println("Error: " + info + " (" + dmp + ")");
 }
}
