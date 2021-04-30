package uk.co.adventuregamestudio.runtime;

public class PEHelper {
    public native boolean isAgsDatafile(Object object, String filename);

    public PEHelper()
    {
        System.loadLibrary("pe");
    }
}
