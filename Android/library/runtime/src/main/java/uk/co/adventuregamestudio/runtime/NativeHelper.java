package uk.co.adventuregamestudio.runtime;

public class NativeHelper {
    public native String findGameDataInDirectory(String path);

    public NativeHelper()
    {
        System.loadLibrary("engine");
    }
}
