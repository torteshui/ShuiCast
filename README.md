# ShuiCast

ShuiCast is a continuation of the Oddcast/Edcast streaming software that can stream to either Icecast or SHOUTcast servers, integrating changes made by the AltaCast and Edcast-Reborn spinoffs.

# Features

The following features / bug fixes have been added to Oddsock's edcast by edcast-reborn:

* Allow changing YP info even for private stream
* Windows Vista/7 compatibility - user writable files (config/log) will default to \user\username\appdata\local\edcast`*` folder - not enabled yet
* for dsp versions in Windows Vista/7 compatibility - user writable files (config/log) will default to \user\username\appdata\roaming\winamp\plugins folder - not enabled yet
* Password no longer clobbered by sending metadata
* Fixed device initialisation bug at startup in STANDALONE and DSP
* Fix long standing edcast issue with Virtual Audio Cables 
* Fix internal buffer overflow when sending metadata 
* NEW DSP with Fraunhofer support
* "Are you sure" prompt when closing down edcast
* Attenuation per encoder
* Limiter with pre-emphasis and pre-gain
* Start in tray
* Improved audio handling
* Latest BASS.dll - fixes the 3 hour issue - not enabled yet, but contains alternative workaround for 3 hour bug
* DSP version: set encoders individually to "Always record from DSP" - allows 2 different sources for one edcast
* Don't log encoder speed unless logging level is set to DEBUG
* Fix config file issues
* Sample rate selection for ASIO
* VU Meter in RMS mode also shows peak
* fix MP3 settings - should be able to actually use some of the more esoteric settings.

# Status

This is work in progress, not all features work, some are just stubs.

* shuicast_standalone: compiles and runs, untested
* shuicast_winamp: compiles and runs, recording from DSP and mic tested, LAME encoding tested
* shuicast_radiodj: compiles, untested
* shucast_foobar: doesn't compile yet

# Changelog (from edcast-reborn)

* MP3 quality setting config hint wrongly described quality
* insane handling of MP3 settings made saner
* VBR and ABR modes still only settable by editing cfg file directly, but should work properly
* lame enc has two quality settings, VBRQuality (for VBR mode only) and Quality. Not sure what the Quality setting actually does, but it follows VBRQuality setting now.
* some "interesting" preset values shown in cfg file, however all presets (-1 to 12) are supported - documentation is sparse - look at http://openinnowhere.sourceforge.net/lameonj/LameDLLInterface.htm under nPreset. Note: LQP_NOPRESET is -1, LQP_NORMAL_QUALITY is 0, and so forth up to LQP_CBR is 12
* Fix Winamp DSP plugin initialisation
* ASIO sample rate selection
* Add pre GAIN to limiter - currently only functions while limiter is active - will become independant
* ASIO control panel - click the ASIO logo. Note: input will cease while ASIO control panel is shown.
* Fix limiter to limit on PEAK not RMS ... derp
* ASIO will now use 44100 sample rate if it's available
* Use latest BASS.dll - this apparently fixes the 3 hour issue
* Fixed DSP issues.
* In DSP, each encoder can be set to record ONLY from DSP - see advanced TAB
* Option to "Start in Tray" - i.e. Start minimized
* Start of total rewrite - audio handling completely rewritten - 
* should be faster especially when you have a mix of encoders with different sample rates or mono/stereo etc
* Limiter with pre-emphasis - pretty much overkill for the dB slider ... who'd want to limit at -15dB ;)
* NEW DSP with Fraunhofer support - no support in standard DSP
* Option to "Start in Tray" - i.e. Start minimized
* Changed limit level to at most -15dB - still pretty insane ;)
* Not a fix, just a heads up ... I think the pre-emphasis routine is broken.
* If you want to use the limiter, I wouldn't set pre-emphasis above 0 at the moment. 
* Fixed initialisation of recording device combo boxes!
* Fixed the flakey damned slider controls in the DSP
* Even more audio speedups!!! For everyone
* OK - limiter level and pre-emphasis sliders - pretty much overkill for the dB slider ... who'd want to limit at -30dB ;)
* More audio fixes - particularly ASIO version
* Fix Fraunhofer encoding for dsp_edcastfh.dll
* Limiter code rewrite. Faster and hopefully better. Still hardocoded values of -3dB limit and 55usec pre-emphasis.
* NEW DSP with Fraunhofer support - no support in standard DSP
* Support Winamp's Fraunhofer AAC+ encoder
* I got a report that a stream was having issues with high volumes distorting. While this is easily fixed by fixing the source music, there's no fix if the Mic input is too high - again, this could be fixed outside of edcast, but I thought I'd try my hand at adding a fairly basic "limiter" to the source stream. It can be switched on/off on the main window.
* Fix internal buffer overflow when sending metadata
* (VERY) Long metadata could cause overflow and streaming password to be exposed recent played list on DNAS
* Fix Parametric Stereo flag: was always set to true when starting edcast
* Attenuation is now a free format floating point entry field ... 0 to infinity. Note, always shown as a positive number, if you enter negative, it will be changed to positive ... so no negative attenuation, i.e. gain, can NOT be applied
* Add warning dialog when exiting edcast
* Add per encoder attenuation control 0 to -10dB in 1dB steps - let me know if you think this should be different
* VU Meter now switches between OFF->RMS->PEAK->OFF
* All inconsistencies with the way Virtual Audio Cable devices are handled should be fixed now
* Fix long standing edcast issue with Virtual Audio Cables (as reported by "Bojcha") - because VAC alllows simultaneous recording from multiple sources in it's recording device, this results in multiple log entries for "change" of recording devices (one for each block of audio processed), however no actual change has taken place. This fix simply reduces log size in such cases.Still working on a proper fix for this, by saving which record source is being used for the current record device, much as the ASIO version works. On change of audio device, the first "enabled" record source will be set as the record source in use. Once the record source is saved in the config, these multiple log entries will disappear.
* Slight speed improvement
* Fixed device initialisation bug at startup in STANDALONE and DSP
* Fixed wassert dependancy in ASIO version
* Sanity checking encoders
* Speed improvements - especially Standalone / Winamp
* Removed Common Controls v6, i.e. new look controls XP and above - could be borked
* In Windows Vista/7, use a subdir of LOCAL_APPDIR for config/default log files
* fix metdata updates with sc_trans 2 beta DJ port
* Changes to installers

# TODO

* DNAS v2 support
* better SC_TRANS DJ v2 support
* On Air timer (per encoder? overall? both?)
* Log file size limit
* use WASAPI where possible. This will allow you to use a windows output devices loop-back as a source - WASAPI loop-back may be the ONLY alternative to non-existent "stereo mix" device in windows vista/7
* Winamp DSP: Mixing mic/winamp, rather than one or the other. Allows nice fades and "voice over music"
* Multiple simultaneous audio devices - standalone version only
* dump metadata history to a metadata log file - edcast-reborn issue #1
* user/pass for icecast2 - edcast-reborn issue #2
* start DSP in soundcard mode - edcast-reborn issue #5
* preconfig - details in edcast-reborn issue #8
* Foobar 2k plugin - edcast-reborn issue #10
* translations - edcast-reborn issue #11
* enable/disable metadata per encoder - edcast-reborn issue #15
* mp3PRO support - edcast-reborn issue #16
* delay metadata per encoder - edcast-reborn issue #17
* VST plugin - edcast-reborn issue #18
* Write an alternate HE-AAC encoder for DSP, and therefore once again have a legal HE-AAC encoder for the standalone versions

* Quality and bitrate management settings for AAC
* Lowpass filter specification (Advanced setting)
* Metadata edit should not accept newlines
* specification of extra string on metadata
* maybe save main (gMain) config after addEncoder and live rec switching.

* Deleting multiple encoders in the main window one at a time from the bottom up will crash it. As a workaround, the encoder window allows to select multiple encoders. It will still only delete one at a time, but selecting multiple encoders anyway keeps it from crashing.
