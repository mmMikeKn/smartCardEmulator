package smartcardemuhost;

public interface EmulatorHostRcvInterface {
 void notifyRcvDebug(String msg);
 void notifyErr(String info, String dmp);
}
