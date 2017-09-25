# ShuiCast

ShuiCast is a continuation of the Oddcast/Edcast streaming software that can stream to either Icecast or SHOUTcast servers, integrating changes made by the AltaCast and Edcast-Reborn spinoffs.
It aims for unifying all the different spinoffs with a better object-oriented design and a clean codebase. It is work in progress (see Status).

# Features

# Changelog

*ShuiCast:*

* Code cleanup and better object-oriented design (work in progress)
* Update of help file (work in progress)

*AltaCast:*

* Made main window slightly taller to accommodate larger encoder window
* Items in main window spread out a little
* Configuration window now uses tabs like Shoutcast encoder instead of buttons (looks more professional).
* YP configuration is editable when the "Public" setting is disabled.
* The "OK" and "Cancel" buttons were moved to the bottom on either side to make it less likely to click the wrong one
* Server type drop down box enlarged
* Encoder Password field now hides the password
* Cleaned up the code to make it more compliant with today's standards

*Edcast-Reborn:*

* Allow changing YP info even for private stream
* Windows Vista/7 compatibility - user writable files (config/log) will default to \user\username\appdata\local\edcast`*` folder - not enabled yet
* For dsp versions in Windows Vista/7 compatibility - user writable files (config/log) will default to \user\username\appdata\roaming\winamp\plugins folder - not enabled yet
* Password no longer clobbered by sending metadata
* Fixed device initialisation bug at startup in STANDALONE and DSP
* Fix long standing edcast issue with Virtual Audio Cables (as reported by "Bojcha") - because VAC alllows simultaneous recording from multiple sources in it's recording device, this results in multiple log entries for "change" of recording devices (one for each block of audio processed), however no actual change has taken place. This fix simply reduces log size in such cases.Still working on a proper fix for this, by saving which record source is being used for the current record device, much as the ASIO version works. On change of audio device, the first "enabled" record source will be set as the record source in use. Once the record source is saved in the config, these multiple log entries will disappear.
* Fix internal buffer overflow when sending metadata 
* NEW DSP with Fraunhofer support - no support in standard DSP
* "Are you sure" prompt when closing down ShuiCast
* Attenuation per encoder
* Limiter with pre-emphasis and pre-gain
* Option to "Start in Tray" - i.e. Start minimized
* Improved audio handling
* Latest BASS.dll - fixes the 3 hour issue - not enabled yet, but contains alternative workaround for 3 hour bug
* DSP version: set encoders individually to "Always record from DSP" - allows 2 different sources for one edcast
* Don't log encoder speed unless logging level is set to DEBUG
* Sample rate selection for ASIO
* VU Meter in RMS mode also shows peak
* Fix MP3 settings - should be able to actually use some of the more esoteric settings.

* VBR and ABR modes still only settable by editing cfg file directly, but should work properly
* lame enc has two quality settings, VBRQuality (for VBR mode only) and Quality. Not sure what the Quality setting actually does, but it follows VBRQuality setting now.
* some "interesting" preset values shown in cfg file, however all presets (-1 to 12) are supported - documentation is sparse - look at http://openinnowhere.sourceforge.net/lameonj/LameDLLInterface.htm under nPreset. Note: LQP_NOPRESET is -1, LQP_NORMAL_QUALITY is 0, and so forth up to LQP_CBR is 12
* ASIO sample rate selection
* ASIO control panel - click the ASIO logo. Note: input will cease while ASIO control panel is shown.
* ASIO will now use 44100 sample rate if it's available
* Start of total rewrite - audio handling completely rewritten - 
* should be faster especially when you have a mix of encoders with different sample rates or mono/stereo etc
* NEW DSP with Fraunhofer support - no support in standard DSP
* Option to "Start in Tray" - i.e. Start minimized
* Changed limit level to at most -15dB - still pretty insane ;)
* Fixed initialisation of recording device combo boxes!
* Fixed the flakey damned slider controls in the DSP
* Even more audio speedups!!! For everyone
* More audio fixes - particularly ASIO version
* Fix Fraunhofer encoding for dsp_edcastfh.dll
* Support Winamp's Fraunhofer AAC+ encoder
* Fix internal buffer overflow when sending metadata
* (VERY) Long metadata could cause overflow and streaming password to be exposed recent played list on DNAS
* Fix Parametric Stereo flag: was always set to true when starting edcast
* Attenuation is now a free format floating point entry field ... 0 to infinity. Note, always shown as a positive number, if you enter negative, it will be changed to positive ... so no negative attenuation, i.e. gain, can NOT be applied
* Add per encoder attenuation control 0 to -10dB in 1dB steps - let me know if you think this should be different
* VU Meter now switches between OFF->RMS->PEAK->OFF
* All inconsistencies with the way Virtual Audio Cable devices are handled should be fixed now
* Fixed wassert dependancy in ASIO version
* Speed improvements - especially Standalone / Winamp
* Removed Common Controls v6, i.e. new look controls XP and above - could be borked
* In Windows Vista/7, use a subdir of LOCAL_APPDIR for config/default log files
* Fix metdata updates with sc_trans 2 beta DJ port
* Changes to installers

*Oddcast v3:

* Multiple encoders (you can simultaneously broadcast in multiple formats at the same time)
* Better Metadata (now you can pull metadata from not only the media player (i.e. winamp) but also from a file or a URL).
* Peak Meter
* Simplified config settings
* Support for Ogg Vorbis, LAME (lame_enc.dll), and AAC (libfaac.dll) - note lame_enc.dll and libfaac.dll must be downloaded separately in order to use these formats.
* Better Live Recording (now you can switch live recording on/off without disconnecting the encoder).

# Status

This is work in progress, not all features work, some are just stubs.
Merging altaCast and Edcast-Reborn may have broken some features.

* shuicast_standalone: compiles and runs, recording from mic tested, LAME encoding tested
* shuicast_winamp: compiles and runs, recording from DSP and mic tested, LAME encoding tested
* shuicast_radiodj: compiles, untested
* shucast_foobar: compiles, untested

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
* enable/disable metadata per encoder - edcast-reborn issue #15
* delay metadata per encoder - edcast-reborn issue #17
* Write an alternate HE-AAC encoder for DSP, and therefore once again have a legal HE-AAC encoder for the standalone versions
* Quality and bitrate management settings for AAC
* Lowpass filter specification (Advanced setting)
* Metadata edit should not accept newlines
* specification of extra string on metadata
* maybe save main (gMain) config after addEncoder and live rec switching.
* Deleting multiple encoders in the main window one at a time from the bottom up will crash it. As a workaround, the encoder window allows to select multiple encoders. It will still only delete one at a time, but selecting multiple encoders anyway keeps it from crashing.
