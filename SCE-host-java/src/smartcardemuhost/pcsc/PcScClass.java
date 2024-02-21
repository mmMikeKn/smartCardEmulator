package smartcardemuhost.pcsc;

import java.util.StringTokenizer;
import java.util.Vector;

public class PcScClass {

  static private String state = null;


  static {
    try {
      System.loadLibrary("mmpcsc");
    } catch (UnsatisfiedLinkError ex) {
      state = "Error load PCSC library (path error) " + ex.toString();
    } catch (Exception ex) {
      state = "Error load PCSC library " + ex.toString();
    }
  }

  native private static String getReaderList();

  native private static String getLastError();

  native private static byte[] resetCard(String readerName);

  native private static void closeCard();

  native public static byte[] sendCmd(byte[] cmd);

  public static String getError() throws PcscException {
    if (state != null) {
      throw new PcscException(state);
    }
    return getLastError();
  }

  public static byte[] waitCard(String readerName) throws PcscException {
    if (state != null) {
      throw new PcscException(state);
    }
    byte resp[] = resetCard(readerName);
    if (resp == null) {
      throw new PcscException(getError());
    }
    return resp;
  }

  public static void closeReader() throws PcscException {
    if (state != null) {
      throw new PcscException(state);
    }
    closeCard();
  }

  public static String[] getReadersArray() throws PcscException {
    if (state != null) {
      throw new PcscException(state);
    }
    String s = PcScClass.getReaderList();
    if (s == null) {
      throw new PcscException(getError());
    }
    StringTokenizer st = new StringTokenizer(s, "|");
    Vector<String> list = new Vector<String>();
    while (st.hasMoreTokens()) {
      list.add(st.nextToken());
    }
    String tmp[] = new String[list.size()];
    return list.toArray(tmp);
  }

}
