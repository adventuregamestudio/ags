package com.bigbluecup.android;

public class PEHelper
{
	public native boolean isAgsDatafile(Object object, String filename);
	
	public PEHelper()
	{
		System.loadLibrary("pe");
	}
}
