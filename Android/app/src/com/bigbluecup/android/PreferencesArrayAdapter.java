package com.bigbluecup.android;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.TextView;

public class PreferencesArrayAdapter extends ArrayAdapter<PreferencesEntry>
{
		private final Context context;
		private final ArrayList<PreferencesEntry> values;

		public PreferencesArrayAdapter(Context context, ArrayList<PreferencesEntry> values)
		{
			super(context, R.layout.rowlayout, values);
			this.context = context;
			this.values = values;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			View rowView;
			
			PreferencesEntry entry = values.get(position);
			
			if (entry.flags.contains(PreferencesEntry.Flags.HEADER))
				rowView = inflater.inflate(R.layout.listheader, parent, false);
			else
			{
				if (entry.description == "")
					rowView = inflater.inflate(R.layout.rowlayout, parent, false);
				else
					rowView = inflater.inflate(R.layout.rowlayout2, parent, false);					
			}
			
		
			
			if (!entry.flags.contains(PreferencesEntry.Flags.HEADER))
			{
				TextView atextView = (TextView)rowView.findViewById(R.id.text1);
				atextView.setText(entry.caption);
				
				if (!entry.flags.contains(PreferencesEntry.Flags.ENABLED))
					atextView.setEnabled(false);
				
				if (entry.description != "")
				{
					TextView btextView = (TextView)rowView.findViewById(R.id.text2);
					btextView.setText(entry.description);
					
					if (!entry.flags.contains(PreferencesEntry.Flags.ENABLED))
						btextView.setEnabled(false);
				}
			}
			else
			{
				TextView textView = (TextView)rowView.findViewById(R.id.label);
				textView.setText(entry.caption);
			}
			
			if (entry.flags.contains(PreferencesEntry.Flags.CHECKABLE))
			{
				CheckBox checkBox = (CheckBox)rowView.findViewById(R.id.check);
				checkBox.setChecked(entry.value == 1);
				
				if (!entry.flags.contains(PreferencesEntry.Flags.ENABLED))
					checkBox.setEnabled(false);
			}
	
			
			if (!entry.flags.contains(PreferencesEntry.Flags.CHECKABLE))
			{
				CheckBox checkBox = (CheckBox)rowView.findViewById(R.id.check);
				if (checkBox != null)
					checkBox.setVisibility(IGNORE_ITEM_VIEW_TYPE);
			}
			
			if (!entry.flags.contains(PreferencesEntry.Flags.ENABLED))
				rowView.setEnabled(false);

			
			return rowView;
		}
		
		public boolean areAllItemsEnabled()
		{
			return false;
		}

		public boolean isEnabled(int position)
		{
			return !(values.get(position).flags.contains(PreferencesEntry.Flags.HEADER));
		}
}
