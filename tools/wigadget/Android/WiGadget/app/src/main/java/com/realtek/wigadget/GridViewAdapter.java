package com.realtek.wigadget;

import android.app.Activity;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

public class GridViewAdapter extends ArrayAdapter<DeviceIcon> {

    private Context context;
    private int layoutResourceId;
    private ArrayList<DeviceIcon> iconArrayList = new ArrayList<DeviceIcon>();

    public GridViewAdapter(Context context, int layoutResourceId, ArrayList<DeviceIcon> iconArrayList) {
        super(context, layoutResourceId, iconArrayList);
        this.layoutResourceId = layoutResourceId;
        this.context = context;
        this.iconArrayList = iconArrayList;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View row = convertView;
        ViewHolder holder;

        if (row == null) {
            LayoutInflater inflater = ((Activity) context).getLayoutInflater();
            row = inflater.inflate(layoutResourceId, parent, false);
            holder = new ViewHolder();
            holder.deviceDescription = (TextView) row.findViewById(R.id.text);
            holder.deviceIcon = (ImageView) row.findViewById(R.id.image);
            row.setTag(holder);
        } else {
            holder = (ViewHolder) row.getTag();
        }


        DeviceIcon item = iconArrayList.get(position);
        holder.deviceDescription.setText(item.getDeviceDescription());
        holder.deviceIcon.setImageBitmap(item.getDeviceIcon());
        return row;
    }

    static class ViewHolder {
        TextView deviceDescription;
        ImageView deviceIcon;
    }

    public DeviceIcon getDeviceIconAt(int idx){
        return iconArrayList.get(idx);
    }
}