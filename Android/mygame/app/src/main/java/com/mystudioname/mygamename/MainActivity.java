package com.mystudioname.mygamename;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;

import uk.co.adventuregamestudio.runtime.AGSRuntimeActivity;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.storage.OnObbStateChangeListener;
import android.os.storage.StorageManager;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.util.Log;

import com.google.android.vending.expansion.downloader.Helpers;
import android.os.Environment;

public class MainActivity extends Activity {
    static final int OBB_FILE_VERSION = App.getContext().getResources().
            getInteger(R.integer.obbFileVersion);
    static final long OBB_FILE_SIZE = Long.parseLong(App.getContext().getResources().
            getString(R.string.obbFileSize)); // file size in bytes of expansion file
    private static final String OBB_KEY; // key used when generating expansion file
    private static final String PACKAGE_NAME = App.getContext().getPackageName();
    private static final String GAME_FILE_NAME = App.getContext().getResources()
            .getString(R.string.game_file_name);
    private static final String OBB_FILE_NAME = "main." + OBB_FILE_VERSION + "." + PACKAGE_NAME +
            ".obb";
    private static final boolean OBB_EMBEDDED = getObbEmbedded();
    private static final String OBB_FILE_EXTERNAL_PATH =
            Environment.getExternalStorageDirectory() + "/Android/obb/" + PACKAGE_NAME + "/" +
                    OBB_FILE_NAME;
    //private static final String OBB_FILE_EXTERNAL_PATH =
    //        App.getContext().getExternalFilesDir(null)+"/" +
    //                OBB_FILE_NAME;
    private static final int DOWNLOAD_REQUEST = 1;
    private static final int WRITE_EXTERNAL_STORAGE_REQUEST = 2;
    private OnObbStateChangeListener expansionListener;

    /**
     * This is a little helper class that demonstrates simple testing of an
     * Expansion APK file delivered by Market. You may not wish to hard-code
     * things such as file lengths into your executable... and you may wish to
     * turn this code off during application development.
     */
    private static class XAPKFile {
        public final boolean mIsMain;
        public final int mFileVersion;
        public final long mFileSize;

        XAPKFile(boolean isMain, int fileVersion, long fileSize) {
            mIsMain = isMain;
            mFileVersion = fileVersion;
            mFileSize = fileSize;
        }
    }

    /**
     * Here is where you place the data that the validator will use to determine
     * if the file was delivered correctly. This is encoded in the source code
     * so the application can easily determine whether the file has been
     * properly delivered without having to talk to the server. If the
     * application is using LVL for licensing, it may make sense to eliminate
     * these checks and to just rely on the server.
     */
    private static final XAPKFile[] xAPKS = {
        new XAPKFile(
            true, // true signifies a main file
                OBB_FILE_VERSION, // the version of the APK that the file was uploaded
            // against
                OBB_FILE_SIZE // the length of the file in bytes
        )
//        new XAPKFile(
//            false, // false signifies a patch file
//            4, // the version of the APK that the patch file was uploaded
//            // against
//            512860L // the length of the patch file in bytes
//        )
    };

    /**
     * Go through each of the APK Expansion files defined in the structure above
     * and determine if the files are present and match the required size. Free
     * applications should definitely consider doing this, as this allows the
     * application to be launched for the first time without having a network
     * connection present. Paid applications that use LVL should probably do at
     * least one LVL check that requires the network to be present, so this is
     * not as necessary.
     *
     * @return true if they are present.
     */
    boolean expansionFilesDelivered() {
        for (XAPKFile xf : xAPKS) {
            String fileName = Helpers.getExpansionAPKFileName(this, xf.mIsMain, xf.mFileVersion);
            if (!Helpers.doesFileExist(this, fileName, xf.mFileSize, false))
                return false;
        }
        return true;
    }

    static
    {
        String obbKey = App.getContext().getResources().getString(R.string.obbKey);
        OBB_KEY = obbKey.matches("@null") ? null : obbKey;
    }

    private static boolean getObbEmbedded() {
        try {
            return Arrays.asList(App.getContext().getResources().getAssets().list(""))
                    .contains(OBB_FILE_NAME);
        }
        catch (IOException e) {
            Log.d("INIT", "IOException occurred during initialization: " + e.getMessage());
            return false;
        }
    }

    // once the expansion file is mounted, this starts the activity that launches the game
    private void startGame(String fileName) {
        Intent intent = new Intent(this, AGSRuntimeActivity.class);
        Bundle b = new Bundle();
        b.putString("filename", fileName); // full path to game data
        b.putString("directory", getApplicationInfo().dataDir); // writable location (saves, etc.)
        b.putBoolean("loadLastSave", false); // TODO: auto-load last save?
        intent.putExtras(b);
        startActivity(intent);
        finish(); // TODO: do something other than just exit the app when the game exits?
    }

    // checks if the expansion file exists (must have matching size in bytes)
    // TODO: add a CRC check on expansion file to check for corruption
    private boolean obbFileExists() {
        File obbFile = new File(OBB_FILE_EXTERNAL_PATH);
        boolean isFile = obbFile.isFile();
        long obbFileSize = obbFile.length();
        return (isFile && (obbFileSize == OBB_FILE_SIZE));
    }

    // ensures that the folders where the expansion file is copied/downloaded exist
    // also creates an empty file for the expansion file to be copied over to help
    // ensure it is created without exceptions being thrown
    private void ensureObbExternalPathExists() {

        int result = ContextCompat.checkSelfPermission(App.getContext(), android.Manifest.permission.WRITE_EXTERNAL_STORAGE);

        if(result == android.content.pm.PackageManager.PERMISSION_DENIED){
            ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},WRITE_EXTERNAL_STORAGE_REQUEST);
        }

        File obbFile = new File(OBB_FILE_EXTERNAL_PATH);
        obbFile.getParentFile().mkdirs();
        try {
            obbFile.createNewFile();
        }
        catch (IOException e) {
        }
    }

    // copies the embedded expansion file to external storage
    // while not ideal, this is necessary for the expansion file to be mounted
    // you most likely want to upload the expansion file separately and use
    // the downloader interface instead
    private void copyEmbeddedObbToExternalStorage() {
        if ((!OBB_EMBEDDED) || (obbFileExists())) {
            return;
        }
        final int BUFFER_SIZE = 102400; // update as needed, your mileage may vary
        InputStream is = null;
        OutputStream os = null;
        try {
            is = getResources().getAssets().open(OBB_FILE_NAME);
            if(is!=null){
                Log.d("ITWORKED",""+is.toString());
            }
            ensureObbExternalPathExists();
            os = new FileOutputStream(OBB_FILE_EXTERNAL_PATH);
            byte[] buffer = new byte[BUFFER_SIZE];
            for (int len; (len = is.read(buffer)) != -1; ) {
                os.write(buffer, 0, len);
            }
        }
        catch (FileNotFoundException e) {
            Log.d("OBB_COPY", "File not found exception occurred copying expansion file: " + e.getMessage());
            finish();
        }
        catch (IOException e) {
            Log.d("OBB_COPY", "IOException occurred copying expansion file: " + e.getMessage());
            finish();
        }
        finally {
            try {
                if (is != null) {
                    is.close(); // FFS, Java, I'm CLOSING a stream... why can this throw an exception???
                }
            }
            catch (IOException e) {
                Log.d("OBB_COPY", "Exception occurred closing input file: " + e.getMessage());
            }
            try {
                if (os != null) {
                    os.close();
                }
            }
            catch (IOException e) {
                Log.d("OBB_COPY", "Exception occurred closing output file: " + e.getMessage());
            }
        }
    }

    // Requests to download the expansion file from Google Play
    private void downloadExpansionFile() {
        if (OBB_EMBEDDED) {
            return;
        }
        if (obbFileExists()) {
            mountExpansionAndStartGame(OBB_KEY);
            return;
        }
        Intent intent = new Intent(this, ExpansionDownloaderActivity.class);
        startActivityForResult(intent, DOWNLOAD_REQUEST);
    }

    // helper to copy or download the expansion file
    // the expansion file will be mounted when finished, and the game will start
    private void copyOrDownloadExpansionFile() {
        if (OBB_EMBEDDED) {
            copyEmbeddedObbToExternalStorage();
            mountExpansionAndStartGame(OBB_KEY);
        }
        else{
            if(expansionFilesDelivered()) {
                mountExpansionAndStartGame(OBB_KEY);
            } else {
                downloadExpansionFile();
                // expansion mounted when download finishes (async)
            }
        }
    }

    // mounts the expansion file and starts the game (async)
    private void mountExpansionAndStartGame(String key) {
        final StorageManager storageManager =
                (StorageManager)getApplicationContext().getSystemService(Context.STORAGE_SERVICE);
        String filePath = OBB_FILE_EXTERNAL_PATH;
        final File mainFile = new File(filePath);
        if (!storageManager.isObbMounted(mainFile.getAbsolutePath())) {
            if (mainFile.exists()) {
                if (storageManager.mountObb(mainFile.getAbsolutePath(), null, expansionListener)) {
                    Log.d("STORAGE_MNT", "SUCCESSFULLY QUEUED");
                } else {
                    Log.d("STORAGE_MNT", "FAILED");
                }
            } else {
                Log.d("STORAGE", "Expansion file " + filePath + " not found!");
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_main);
        expansionListener = new OnObbStateChangeListener() {
            @Override
            public void onObbStateChange(String path, int state) {
                super.onObbStateChange(path, state);
                final StorageManager storageManager =
                        (StorageManager) getApplicationContext()
                                .getSystemService(Context.STORAGE_SERVICE);
                Log.d("PATH = ", path);
                Log.d("STATE = ", state + "");
                if (state == OnObbStateChangeListener.MOUNTED) {
                    String mountedPath = storageManager.getMountedObbPath(path);
                    Log.d("STORAGE", "-->MOUNTED");
                    startGame(new File(mountedPath, GAME_FILE_NAME).getPath());
                } else {
                    Log.d("##", "Path: " + path + "; state: " + state);
                }
            }
        };
    }

    @Override
    protected void onResume() {
        super.onResume();
        copyOrDownloadExpansionFile();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // Check which request we're responding to
        if (requestCode == DOWNLOAD_REQUEST) {
            if (resultCode == RESULT_OK) {
                // Download was successful
                mountExpansionAndStartGame(OBB_KEY);
            }
            else {
                // The download failed! Try again... we can't do anything it!
                downloadExpansionFile();
            }
        }
    }
}
