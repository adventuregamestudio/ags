package com.mystudioname.mygamename;

import android.app.Application;
import android.content.Context;

import java.lang.ref.WeakReference;

/// Helper class to get static info from resource files
public class App extends Application {
    private static WeakReference<Context> mContext;

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = new WeakReference<Context>(this);
    }

    public static Context getContext() {
        return mContext.get();
    }
}
