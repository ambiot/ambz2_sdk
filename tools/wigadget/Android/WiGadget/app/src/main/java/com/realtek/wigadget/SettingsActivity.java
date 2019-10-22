package com.realtek.wigadget;

import android.media.MediaPlayer;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;

/**
 * Created by Wu Jinzhou on 6/8/2015.
 */
public class SettingsActivity extends PreferenceActivity{
    static private boolean canPlayHtAlarmSound = false;
    static private Constants c = Constants.getInstance();
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.app_settings);

        // bind preference summary for H&T
        canPlayHtAlarmSound = false;
        bindPreferenceSummaryToValue(findPreference(c.key_ht_sensor_data_update_frequency));
        bindPreferenceSummaryToValue(findPreference(c.key_ht_sensor_temperature_threshold));
        bindPreferenceSummaryToValue(findPreference(c.key_ht_sensor_humidity_threshold));
        bindPreferenceSummaryToValue(findPreference(c.key_ht_sensor_alarm_sound));
        canPlayHtAlarmSound =true;

    }

    private static void bindPreferenceSummaryToValue(Preference preference) {
        // Set the listener to watch for value changes.
        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);

        // Trigger the listener immediately with the preference's
        // current value
        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference, PreferenceManager.getDefaultSharedPreferences(preference.getContext()).getString(preference.getKey(), ""));
    }

    private static Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            String newValueStr = newValue.toString();

            if (preference instanceof ListPreference) {
                // For list preferences, look up the correct display newValue in
                // the preference's 'entries' list.
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(newValueStr);

                // Set the summary to reflect the new newValue.
                preference.setSummary(index >= 0 ? listPreference.getEntries()[index] : null);

            } else {
                // For all other preferences, set the summary to the newValue's
                // simple string representation.
                preference.setSummary(newValueStr);
            }

            //for H&T : play selected alarm sound
            if(preference.getKey().equals(c.key_ht_sensor_alarm_sound) && canPlayHtAlarmSound){
                String sound = newValueStr;
                MediaPlayer mediaPlayer;

                if(sound.equals(c.val_ht_sensor_alarm_sound_altair)){
                    mediaPlayer = MediaPlayer.create(preference.getContext(), R.raw.sound_altair);
                }
                else if(sound.equals(c.val_ht_sensor_alarm_sound_ariel)){
                    mediaPlayer = MediaPlayer.create(preference.getContext(),R.raw.sound_ariel);
                }
                else if(sound.equals(c.val_ht_sensor_alarm_sound_fomalhaut)){
                    mediaPlayer = MediaPlayer.create(preference.getContext(),R.raw.sound_fomalhaut);
                }
                else if(sound.equals(c.val_ht_sensor_alarm_sound_tinkerbell)){
                    mediaPlayer = MediaPlayer.create(preference.getContext(),R.raw.sound_tinkerbell);
                }
                else {
                    mediaPlayer = MediaPlayer.create(preference.getContext(),R.raw.sound_ding);
                }

                mediaPlayer.start();
            }
            return true;
        }
    };

}
