package com.realtek.wigadget;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActionBarDrawerToggle;
import android.support.v4.widget.DrawerLayout;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.Toast;

import org.json.JSONException;

import java.io.IOException;
import java.util.ArrayList;



public class MainActivity extends Activity {
	private DrawerLayout mDrawerLayout;
	private ListView mDrawerList;
	private ActionBarDrawerToggle mDrawerToggle;

	// nav drawer title
	private CharSequence mDrawerTitle;

	// used to store app title
	private CharSequence mTitle;

	// slide menu items
	private String[] navMenuTitles;
	private TypedArray navMenuIcons;

	private ArrayList<NavDrawerItem> navDrawerItems;
	private NavDrawerListAdapter adapter;

    private static final Constants c = Constants.getInstance();
	private DeviceList mDeviceList;
    private int tempListSize;
    private int savedListSize;
    private Handler mHandler;

    private boolean exitActivity =false;
    private boolean showToast = true;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

        try {
            mDeviceList = DeviceList.getInstance(getApplicationContext());
            tempListSize=0;
            savedListSize=0;
        } catch (JSONException e) {
            Log.e(c.tag_main_activity, "init mDeviceList JSONException e --" + e);
        } catch (IOException e) {
            Log.e(c.tag_main_activity, "init mDeviceList IOException e --" + e);
        }

		mTitle = mDrawerTitle = getTitle();

		// load slide menu items
		navMenuTitles = getResources().getStringArray(R.array.nav_drawer_items);

		// nav drawer icons from resources
		navMenuIcons = getResources().obtainTypedArray(R.array.nav_drawer_icons);

		mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
		mDrawerList = (ListView) findViewById(R.id.list_slidermenu);

		navDrawerItems = new ArrayList<>();

		// adding nav drawer items to array
        savedListSize=mDeviceList.size();
        tempListSize=mDeviceList.tempListSize();

        // my device
        if(savedListSize!=0){
            String number=""+savedListSize;
            navDrawerItems.add(new NavDrawerItem(navMenuTitles[0], navMenuIcons.getResourceId(0, -1),true,number));
        }
        else {
            navDrawerItems.add(new NavDrawerItem(navMenuTitles[0], navMenuIcons.getResourceId(0, -1)));
        }

        // find device
        if(tempListSize!=0){
            navDrawerItems.add(new NavDrawerItem(navMenuTitles[1], navMenuIcons.getResourceId(1, -1),true,"NEW"));
        }
        else {
            navDrawerItems.add(new NavDrawerItem(navMenuTitles[1], navMenuIcons.getResourceId(1, -1)));
        }

		// about
		navDrawerItems.add(new NavDrawerItem(navMenuTitles[2], navMenuIcons.getResourceId(2, -1)));

		// Recycle the typed array
		//navMenuIcons.recycle();

		mDrawerList.setOnItemClickListener(new SlideMenuClickListener());

		// setting the nav drawer list adapter
		adapter = new NavDrawerListAdapter(getApplicationContext(), navDrawerItems);
		mDrawerList.setAdapter(adapter);

		// enabling action bar app icon and behaving it as toggle button
		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setHomeButtonEnabled(true);

		mDrawerToggle = new ActionBarDrawerToggle(this, mDrawerLayout,
				R.drawable.ic_drawer, //nav menu toggle icon
				R.string.app_name, // nav drawer open - description for accessibility
				R.string.app_name // nav drawer close - description for accessibility
		) {
			public void onDrawerClosed(View view) {
				getActionBar().setTitle(mTitle);
				// calling onPrepareOptionsMenu() to show action bar icons
				invalidateOptionsMenu();
			}

			public void onDrawerOpened(View drawerView) {
				getActionBar().setTitle(mDrawerTitle);
				// calling onPrepareOptionsMenu() to hide action bar icons
				invalidateOptionsMenu();
			}
		};
		mDrawerLayout.setDrawerListener(mDrawerToggle);

		if (savedInstanceState == null) {
			// on first time display view for first nav item
			displayView(0);
		}


        //////////////////////////////////start nsd///////////////////////////////////////////
        try {
            NsdHelper nsdHelper = new NsdHelper(getApplicationContext());
            nsdHelper.execute();
        } catch (IOException e) {
            Log.e(c.tag_main_activity, "init NsdHelper IOException e --" + e);
        } catch (JSONException e) {
            Log.e(c.tag_main_activity, "init NsdHelper JSONException e --" + e);
        }
        //////////////////////////////////start nsd///////////////////////////////////////////

        /////////////////////////////////update ui thread/////////////////////////////////////
        mHandler=new Handler();
        startUpdateUi();
        /////////////////////////////////update ui thread/////////////////////////////////////
	}

	/**
	 * Slide menu item click listener
	 * */
	private class SlideMenuClickListener implements
			ListView.OnItemClickListener {
		@Override
		public void onItemClick(AdapterView<?> parent, View view, int position,
				long id) {
			// display view for selected nav drawer item
			displayView(position);
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// toggle nav drawer on selecting action bar app icon/title
		if (mDrawerToggle.onOptionsItemSelected(item)) {
			return true;
		}
		// Handle action bar actions click
		switch (item.getItemId()) {
		case R.id.action_settings:
			Log.v(c.tag_main_activity,"onOptionsItemSelected() : open SettingsActivity");
            startActivity(new Intent(this,SettingsActivity.class));
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	/* *
	 * Called when invalidateOptionsMenu() is triggered
	 */
	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		// if nav drawer is opened, hide the action items
		boolean drawerOpen = mDrawerLayout.isDrawerOpen(mDrawerList);
		menu.findItem(R.id.action_settings).setVisible(!drawerOpen);
		return super.onPrepareOptionsMenu(menu);
	}

	/**
	 * Diplaying fragment view for selected nav drawer list item
	 * */
	private void displayView(int position) {
		// update the main content by replacing fragments
		Fragment fragment = null;
		switch (position) {
		case 0:
			fragment = new MyDeviceFragment();
			break;
		case 1:
			fragment = new FindDeviceFragment();
			break;
		case 2:
			fragment = new AboutFragment();
			break;
		default:
			break;
		}

		if (fragment != null) {
			FragmentManager fragmentManager = getFragmentManager();
			fragmentManager.beginTransaction().replace(R.id.frame_container, fragment).commit();

			// update selected item and title, then close the drawer
			mDrawerList.setItemChecked(position, true);
			mDrawerList.setSelection(position);
			setTitle(navMenuTitles[position]);
			mDrawerLayout.closeDrawer(mDrawerList);
		} else {
			// error in creating fragment
			Log.e(c.tag_main_activity, "displayView() Error in creating fragment");
		}
	}

	@Override
	public void setTitle(CharSequence title) {
		mTitle = title;
		getActionBar().setTitle(mTitle);
	}

	/**
	 * When using the ActionBarDrawerToggle, you must call it during
	 * onPostCreate() and onConfigurationChanged()...
	 */

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);
		// Sync the toggle state after onRestoreInstanceState has occurred.
		mDrawerToggle.syncState();
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		// Pass any configuration change to the drawer toggls
		mDrawerToggle.onConfigurationChanged(newConfig);
	}



    ///////////////////////////update ui////////////////////////////

    Runnable updateUiCallback = new Runnable() {
        @Override
        public void run() {

            ////////////////////update ui////////////////////
            navDrawerItems = new ArrayList<NavDrawerItem>();

            // adding nav drawer items to array
            savedListSize=mDeviceList.size();
            tempListSize=mDeviceList.tempListSize();

            // my device
            if(savedListSize!=0){
                String number=""+savedListSize;
                navDrawerItems.add(new NavDrawerItem(navMenuTitles[0], navMenuIcons.getResourceId(0, -1),true,number));
            }
            else {
                navDrawerItems.add(new NavDrawerItem(navMenuTitles[0], navMenuIcons.getResourceId(0, -1)));
            }

            // find device
            if(tempListSize!=0){
                navDrawerItems.add(new NavDrawerItem(navMenuTitles[1], navMenuIcons.getResourceId(1, -1),true,"NEW"));
            }
            else {
                navDrawerItems.add(new NavDrawerItem(navMenuTitles[1], navMenuIcons.getResourceId(1, -1)));
            }

            // about
            navDrawerItems.add(new NavDrawerItem(navMenuTitles[2], navMenuIcons.getResourceId(2, -1)));

            // setting the nav drawer list adapter
            adapter = new NavDrawerListAdapter(getApplicationContext(), navDrawerItems);
            mDrawerList.setAdapter(adapter);
            ////////////////////update ui////////////////////

            mHandler.postDelayed(updateUiCallback, c.val_ui_update_period_ms);
        }
    };

    void startUpdateUi() {
        updateUiCallback.run();
    }

    void stopUpdateUi() {
        mHandler.removeCallbacks(updateUiCallback);
    }

    ///////////////////////////update ui////////////////////////////


    //////////////////////////double click to quit//////////////////////////
    @Override
    public void onBackPressed() {

        if(showToast){
            Toast.makeText(this, "Double-Click to Quit", Toast.LENGTH_LONG).show();
            showToast =false;
        }

        if (exitActivity) {
            super.onBackPressed();
        }

        exitActivity = true;
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                exitActivity =false;
            }
        }, c.val_ui_double_click_interval);
    }
    //////////////////////////double click to quit//////////////////////////

}
