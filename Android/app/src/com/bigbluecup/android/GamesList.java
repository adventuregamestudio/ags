package com.bigbluecup.android;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;

import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Environment;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

public class GamesList extends ListActivity
{
	String filename = null;
	private ProgressDialog dialog;
	private native boolean isAgsDatafile(Object object, String filename); 

	private ArrayList<String> folderList;
	private ArrayList<String> filenameList;
	
	private String baseDirectory;
	
	private void showMessage(String message)
	{
		Toast.makeText(this, message, Toast.LENGTH_LONG).show();
	}
	
	public void onCreate(Bundle bundle)
	{
		super.onCreate(bundle);
		
		System.loadLibrary("pe");
		
		filename = searchForGames();
		
		if (filename != null)
		  startGame(filename);
		
		if ((folderList != null) && (folderList.size() > 0))
		{
			this.setListAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, folderList));
		}
		else
		{
			showMessage("No games found.");
		}
		
		registerForContextMenu(getListView());
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId())
		{
			case R.id.credits:
				return true;
			case R.id.preferences:
				showPreferences(-1);
				return true;
			default:
				return super.onOptionsItemSelected(item);
		}
	}
	
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		super.onCreateContextMenu(menu, v, menuInfo);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.game_context_menu, menu);
		
		menu.setHeaderTitle(folderList.get((int)((AdapterContextMenuInfo)menuInfo).id));
	}	
	
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		switch (item.getItemId())
		{
			case R.id.preferences:
				showPreferences((int)info.id);
				return true;
			case R.id.start:
				startGame(filenameList.get((int)info.id));
				return true;
			default:
				return super.onContextItemSelected(item);
		}
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		super.onListItemClick(l, v, position, id);
		
		startGame(filenameList.get(position));
	}

	private void showPreferences(int position)
	{
		Intent intent = new Intent(this, PreferencesActivity.class);
		Bundle b = new Bundle();
		b.putString("name", (position < 0) ? "" : folderList.get(position));
		b.putString("filename", (position < 0) ? null : filenameList.get(position));
		b.putString("directory", baseDirectory);
		intent.putExtras(b);
		startActivity(intent);
	}
	
	private void startGame(String filename)
	{
		Intent intent = new Intent(this, AgsEngine.class);
		Bundle b = new Bundle();
		b.putString("filename", filename);
		b.putString("directory", baseDirectory);
		intent.putExtras(b);
		startActivity(intent);
		finish();
	}
	
	private String searchForGames()
	{
		String[] tempList = null;
		
		baseDirectory = Environment.getExternalStorageDirectory() + "/ags";
		
		// Check for ac2game.dat in the base directory
		File ac2game = new File(baseDirectory + "/ac2game.dat");
		if (ac2game.isFile())
			return baseDirectory + "/ac2game.dat";
		
		// Check for games in folders
		File agsDirectory = new File(baseDirectory);
		if (agsDirectory.isDirectory())
		{
			tempList = agsDirectory.list(new FilenameFilter()
			{
				public boolean accept(File dir, String filename)
				{
					return new File(dir + "/" + filename).isDirectory();
				}
			});
		}
		
		if (tempList != null)
		{
			java.util.Arrays.sort(tempList);
			
			folderList = new ArrayList<String>();
			filenameList = new ArrayList<String>();
			
			int i;
			for (i = 0; i < tempList.length; i++)
			{
				if ((new File(agsDirectory + "/" + tempList[i] + "/ac2game.dat").isFile())
						&& (isAgsDatafile(this, agsDirectory + "/" + tempList[i] + "/ac2game.dat")))
				{
					folderList.add(tempList[i]);
					filenameList.add(agsDirectory + "/" + tempList[i] + "/ac2game.dat");
				}
				else
				{
					File directory = new File(agsDirectory + "/" + tempList[i]);
					String[] list = directory.list(new FilenameFilter() {
						
						private boolean found = false;
						
						public boolean accept(File dir, String filename) {
							
							if (found)
								return false;
							else
							{
								if ((filename.indexOf(".exe") > 0) 
										&& (isAgsDatafile(this, dir.getAbsolutePath() + "/" + filename)))
								{
									found = true;
									return true;
								}
							}
							
							return false;
						}
					});
					
					if ((list != null) && (list.length > 0))
					{
						folderList.add(tempList[i]);
						filenameList.add(agsDirectory + "/" + tempList[i] + "/" + list[0]);
					}
				}
			}
		}
		
		return null;
	}

	// Prevent the activity from being destroyed on a configuration change
	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
	}	
}
