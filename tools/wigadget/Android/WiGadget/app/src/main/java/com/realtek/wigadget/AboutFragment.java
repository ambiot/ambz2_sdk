package com.realtek.wigadget;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class AboutFragment extends Fragment {
    static Constants c = Constants.getInstance();
    final String tag= c.tag_about_fragment;

	public AboutFragment(){}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.about_fragment_title);
    }
	
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_about, container, false);
        TextView tv = (TextView)rootView.findViewById(R.id.txtLabel);
        String tvText = "Build on 08/06/2015";

        tv.setText(tvText);
        return rootView;
    }

}
