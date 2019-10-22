package com.realtek.wigadget;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

/**
 * Created by Wu Jinzhou on 3/24/2015.
 */
public class ImageListAdapter extends ArrayAdapter<String> {
    private final Context context;
    private final String[] values;

    public ImageListAdapter(Context context, String[] values) {
        super(context, R.layout.layout_popup_dialog_row_cell, values);
        this.context = context;
        this.values = values;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        View rowView = inflater.inflate(R.layout.layout_popup_dialog_row_cell, parent, false);

        TextView textView = (TextView) rowView.findViewById(R.id.label);

        ImageView imageView = (ImageView) rowView.findViewById(R.id.icon);

        textView.setText(values[position]);

        // Change the icon for different control options
        String s = values[position];
        if (s.equals(getContext().getResources().getString(R.string.option_rename))) {
            imageView.setImageResource(R.drawable.ic_edit);
        }
        if(s.equals(getContext().getResources().getString(R.string.option_remove)))
        {
            imageView.setImageResource(R.drawable.ic_delete);
        }
        if(s.equals(getContext().getResources().getString(R.string.option_request_share))){
            imageView.setImageResource(R.drawable.ic_share);
        }
        if(s.equals(getContext().getResources().getString(R.string.option_pair_device))){
            imageView.setImageResource(R.drawable.ic_pair);
        }

        return rowView;
    }
}