# ShuiCast

ShuiCast is a continuation of the Oddcast/Edcast streaming software that can stream to either IceCast or ShoutCast servers, integrating changes made by the AltaCast and Edcast-Reborn spinoffs.
It aims for unifying all the different spinoffs with a better object-oriented design and a clean codebase. It is work in progress (see Status).

# Features

* Stream to IceCast or ShoutCast
* Support for Ogg Vorbis, LAME (lame_enc.dll), and AAC (libfaac.dll) - note lame_enc.dll and libfaac.dll must be downloaded separately in order to use these formats
* Multiple encoders (you can simultaneously broadcast in multiple formats at the same time)
* Standalone application as well as plugin for Winamp, RadioDJ and Foobar2000
* Metadata not only from media player (i.e. Winamp) but also from a file, a URL or a window
* Peak and RMS Meter
* Live Recording (you can switch on/off without disconnecting the encoder)

# Changelog

*ShuiCast:*

* Compiles with Visual Studio 2013
* Code cleanup and better object-oriented design (work in progress)
* Update of help file (work in progress)
* Many small UI changes, partly taken over from AltaCast and Edcast-Reborn

*AltaCast:*

* Configuration window now uses tabs instead of buttons (looks more professional)
* YP configuration is editable when the "Public" setting is disabled
* Encoder Password field now hides the password
* Cleaned up the code to make it more compliant with today's standards

*Edcast-Reborn:*

* Allow changing YP info even for private stream
* Windows Vista/7 compatibility - user writable files (config/log) will default to \user\username\appdata\local\edcast`*` folder - not enabled yet
* For dsp versions in Windows Vista/7 compatibility - user writable files (config/log) will default to \user\username\appdata\roaming\winamp\plugins folder - not enabled yet
* Password no longer clobbered by sending metadata
* Fixed device initialisation bug at startup in STANDALONE and DSP
* Fix long standing edcast issue with Virtual Audio Cables by setting first "enabled" record source as the record source in use to prevent "change" of recording devices because VAC allows simultaneous recording from multiple sources.
* Fix internal buffer overflow when sending metadata (very long metadata could cause overflow and streaming password to be exposed recent played list on DNAS)
* NEW DSP dsp_edcastfh.dll with Fraunhofer support - no support in standard DSP
* "Are you sure" prompt when closing down ShuiCast
* Attenuation per encoder: 0 to infinity. Note, always shown as a positive number, if you enter negative, it will be changed to positive, so no negative attenuation, i.e. gain, can be applied
* Limiter with pre-emphasis and pre-gain
* Option to "Start in Tray" - i.e. Start minimized
* Improved audio handling, completely rewritten, fixes and speedups
* Latest BASS.dll - fixes the 3 hour issue - not enabled yet, but contains alternative workaround for 3 hour bug
* DSP version: set encoders individually to "Always record from DSP" - allows 2 different sources for one edcast
* Don't log encoder speed unless logging level is set to DEBUG
* Sample rate selection for ASIO
* VU Meter now switches between OFF->RMS->PEAK->OFF, also shows peak in RMS mode
* Fix MP3 settings - should be able to actually use some of the more esoteric settings.

* VBR and ABR modes still only settable by editing cfg file directly, but should work properly
* lame enc has two quality settings, VBRQuality (for VBR mode only) and Quality. Not sure what the Quality setting actually does, but it follows VBRQuality setting now.
* some "interesting" preset values shown in cfg file, however all presets (-1 to 12) are supported - documentation is sparse - look at http://openinnowhere.sourceforge.net/lameonj/LameDLLInterface.htm under nPreset. Note: LQP_NOPRESET is -1, LQP_NORMAL_QUALITY is 0, and so forth up to LQP_CBR is 12
* ASIO sample rate selection
* ASIO control panel - click the ASIO logo. Note: input will cease while ASIO control panel is shown.
* should be faster especially when you have a mix of encoders with different sample rates or mono/stereo etc
* Fixed initialisation of recording device combo boxes and slider controls
* Support Winamp's Fraunhofer AAC+ encoder
* Fix Parametric Stereo flag: was always set to true when starting edcast
* Fixed wassert dependancy in ASIO version
* Fix metdata updates with sc_trans 2 beta DJ port
* Changes to installers

# Status

This is work in progress, not all features work, some are just stubs.
Merging altaCast and Edcast-Reborn may have broken some features.

* shuicast_standalone: compiles and runs, recording from mic tested, LAME encoding tested
* shuicast_winamp: compiles and runs, recording from DSP and mic tested, LAME encoding tested
* shuicast_radiodj: compiles, untested
* shuicast_foobar: compiles, untested

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
