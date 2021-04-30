package uk.co.adventuregamestudio.runtime;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Properties;

public class ReadOnlyINI {
    private Properties cfg;
    private String dir = "/";
    public ReadOnlyINI(String directory) {
        cfg = new Properties();
        dir = directory;
    }

    public boolean load() {
        boolean retval = false;

        FileInputStream fis = null;
        try {
            retval = true;
            String cfg_filename = "android.cfg";
            String filePath = dir + "/" + cfg_filename;
            fis = new FileInputStream(filePath);
            cfg.load(fis);
        } catch (FileNotFoundException e) {
            retval = false;
            System.out.println("Configuration error FileNotFound: " + e.getMessage());
        } catch (IOException e) {
            retval = false;
            System.out.println("Configuration error IOException: " + e.getMessage());
        } finally {
            if (null != fis)
            {
                try
                {
                    fis.close();
                }
                catch (Exception e)
                {
                    System.out.println("Configuration error Exception: " + e.getMessage());
                }
            }
        }

        return retval;
    }

    public String get(String key) {
        String retval = "0";

        String tmp = cfg.getProperty(key);
        if(tmp != null) {
            retval = tmp;
        }

        return retval;
    }
}
