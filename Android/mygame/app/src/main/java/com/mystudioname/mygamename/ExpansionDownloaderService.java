package com.mystudioname.mygamename;

import com.google.android.vending.expansion.downloader.impl.DownloaderService;

public class ExpansionDownloaderService extends DownloaderService {
    @Override
    public String getPublicKey() {
        return getString(R.string.RSA_public_key);
    }

    @Override
    public byte[] getSALT() {
        final int[] ints = getResources().getIntArray(R.array.ExpansionDownloaderServiceSALT);
        final byte[] bytes = new byte[ints.length];
        for (int i = 0; i < ints.length; ++i)
        {
            bytes[i] = (byte)ints[i];
        }
        return bytes;
    }

    @Override
    public String getAlarmReceiverClassName() {
        return getClass().getName();
    }
}
