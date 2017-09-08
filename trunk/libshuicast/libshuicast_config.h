#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>
#include <tchar.h>

typedef struct tagSaneConfig
{
	TCHAR appName[MAX_PATH];
	TCHAR currentDir[MAX_PATH];
	TCHAR exename[MAX_PATH];
	TCHAR firstConfig[MAX_PATH];
	TCHAR firstWritableFolder[MAX_PATH];
} saneConfig;

class seeker
{
public:
	seeker();
	~seeker();
	PTCHAR folder();
	bool exists();
	bool isWritable();
	bool hasConfig();
	void getfolder(int csidl, PTCHAR subdir, PTCHAR inConfigFile);
private:	
	TCHAR _cfgFile[MAX_PATH];
	TCHAR _folder[MAX_PATH];
	TCHAR _parent[MAX_PATH];
	bool _exists;
	bool _isWritable;
	bool _hasCfgFile;

	bool CheckIsWritable();
	bool CheckHasConfig();
};

class ConfigFinder
{
public:
	ConfigFinder();
	~ConfigFinder();
	ConfigFinder(PTCHAR inCfgFileName);	
	void getfolder(int csidl, PTCHAR subdir);
	void getfolder(int csidl, PTCHAR subdir1, PTCHAR subdir2);
	void getfolder(int csidl, PTCHAR subdir1, PTCHAR subdir2, PTCHAR subdir3);
	bool ConfigFinder::_getDirName(PTCHAR inDir, PTCHAR dst, int lvl=1);
	PTCHAR getWritableFolder();
	PTCHAR getConfigFolder();
private:
	TCHAR cfgFileName[MAX_PATH];

	seeker folders[20];
	seeker * firstWritableFolder;
	seeker * firstFolderWithConfig;
	
	int folderCount;
};
const saneConfig * saneLoadConfigs(PTCHAR cfgFileName, HMODULE hModule = NULL, bool bStripUnderscore = false);
