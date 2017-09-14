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
