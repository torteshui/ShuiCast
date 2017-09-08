
#define WIN32_LEAN_AND_MEAN
#include "saneEdcastConfig.h"
#include <ShlObj.h>
#include <stdio.h>

seeker::seeker() : _exists(false), _isWritable(false), _hasCfgFile(false)
{
}
seeker::~seeker()
{
}

PTCHAR seeker::folder()
{
	return _folder;
}

bool seeker::exists()
{
	return _exists;
}

bool seeker::isWritable()
{
	return _isWritable;
}

bool seeker::hasConfig()
{
	return _hasCfgFile;
}

void seeker::getfolder(int csidl, PTCHAR subdir, PTCHAR inConfigFile)
{
	_tcscpy(_cfgFile, inConfigFile);
	if(csidl)
	{
		HRESULT res = SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, _parent);
		res = SHGetFolderPathAndSubDir(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, subdir, _folder);
		if((_exists = (res == S_OK)))
		{
			if(CheckIsWritable())
			{
				if(CheckHasConfig())
				{
				}
			}
		}
	}
	else
	{

		HRESULT res = S_OK;
		_tcscpy(_parent, _T(""));
		_tcscpy(_folder, subdir);
		if((_exists = (res == S_OK)))
		{
			if(CheckIsWritable())
			{
				if(CheckHasConfig())
				{
				}
			}
		}
	}
}

bool seeker::CheckIsWritable()
{
	TCHAR tmpfile[MAX_PATH] = _T("");
	FILE *filep;
	wsprintf(tmpfile, _T("%s\\.tmp"), _folder);
	filep = _tfopen(tmpfile, _T("w"));

	if (filep == NULL) 
	{
		_isWritable = false;
	}
	else
	{
		fclose(filep);
		_tunlink(tmpfile);
		_isWritable = true;
	}

	return _isWritable;
}

bool seeker::CheckHasConfig()
{
	TCHAR tmpfile[MAX_PATH] = _T("");
	FILE *filep;
	wsprintf(tmpfile, _T("%s\\%s"), _folder, _cfgFile);
	filep = _tfopen(tmpfile, _T("r"));

	if (filep == NULL) 
	{
		_hasCfgFile = false;
	}
	else
	{
		fclose(filep);
		_hasCfgFile = true;
	}

	return _hasCfgFile;
}

ConfigFinder::ConfigFinder() : firstWritableFolder(NULL), firstFolderWithConfig(NULL), folderCount(0) 
{
}

ConfigFinder::ConfigFinder(PTCHAR inCfgFileName) : firstWritableFolder(NULL), firstFolderWithConfig(NULL)
{
	_tcscpy(cfgFileName, inCfgFileName);
}

ConfigFinder::~ConfigFinder()
{
}

void ConfigFinder::getfolder(int csidl, PTCHAR subdir)
{
	if(folderCount < 20)
	{
		seeker * folder = &folders[folderCount++];
		folder->getfolder(csidl, subdir, cfgFileName);

		if(!firstWritableFolder && folder->isWritable())
		{
			firstWritableFolder = folder;
		}

		if(!firstFolderWithConfig && folder->hasConfig())
		{
			firstFolderWithConfig = folder;
		}

		folderCount++;
	}
}

void ConfigFinder::getfolder(int csidl, PTCHAR subdir1, PTCHAR subdir2)
{
	TCHAR subdir[MAX_PATH];
	wsprintf(subdir, _T("%s\\%s"), subdir1, subdir2);
	getfolder(csidl, subdir);
}

void ConfigFinder::getfolder(int csidl, PTCHAR subdir1, PTCHAR subdir2, PTCHAR subdir3)
{
	TCHAR subdir[MAX_PATH];
	wsprintf(subdir, _T("%s\\%s\\%s"), subdir1, subdir2, subdir3);
	getfolder(csidl, subdir);
}

PTCHAR ConfigFinder::getWritableFolder()
{
	if(firstWritableFolder)
	{
		return firstWritableFolder->folder();
	}

	return NULL;
}

PTCHAR ConfigFinder::getConfigFolder()
{
	if(firstFolderWithConfig)
	{
		return firstFolderWithConfig->folder();
	}

	return NULL;
}

bool ConfigFinder::_getDirName(PTCHAR inDir, PTCHAR dst, int lvl) // base
{
	// inDir = ...\winamp\Plugins
	PTCHAR dir = _tcsdup(inDir);
	bool retval = false;
	// remove trailing slash

	if(dir[_tcslen(dir)-1] == '\\')
	{
		dir[_tcslen(dir)-1] = '\0';
	}

	PTCHAR p1;

	for(p1 = dir + _tcslen(dir) - 1; p1 >= dir; p1--)
	{
		if(*p1 == '\\')
		{
			if(--lvl > 0)
			{
				*p1 = '\0';
			}
			else
			{
				_tcscpy(dst, p1 + 1);
				retval = true;
				break;
			}
		}
	}
	free(dir);
	return retval;
}
/* 
	usage: DLL
		int initedcast(struct winampDSPModule *this_mod) 
		{
			PTCHAR myname = getBaseModuleName(this_mod->hDllInstance); // , true if you want to strip the dsp_ and _v3 from dsp_name_v3
		}
	usage: EXE
		int initedcast() 
		{
			PTCHAR myname = getBaseModuleName(NULL); // , true if you want to strip the dsp_ and _v3 from dsp_name_v3
		}
*/
static void getBaseModuleName(PTCHAR myExeName, int len, HMODULE hModule, bool bStripUnderscore = false)
{
	TCHAR filename[MAX_PATH];

	GetModuleFileName(hModule, filename, sizeof(filename));

	PTCHAR p = _tcsrchr(filename, '\\');

	if (p) 
	{
		*(p++) = '\000';
	}
	// p = start of name
	PTCHAR p1 = _tcsrchr(p, '.');

	if (p1)
	{
		*p1 = '\000';
	}

	if(bStripUnderscore)
	{
		PTCHAR p2 = _tcsrchr(p, '_');

		if(p2) // look for xxx_blah_yyy or xxx_blah
		{
			p1 = _tcschr(p, '_');

			if(p1 != p2) // xxx_blah_yyy
			{
				p = p1;
				*p2 = '\0';
			}
			else // xxx_blah
			{
				p = p1;
			}
		}
	}
	_tcscpy(myExeName, p);
}

/*
	Order of folders to look in

ALL
current-dir/												current-dir/
DLL ONLY
/Program Files/Winamp-dir/Plugins/dllname/					/Program Files/Winamp-dir/Plugins/dllname/
/Program Files/Winamp-dir/Plugins/							/Program Files/Winamp-dir/Plugins/
/Users/username/AppData/Roaming/winamp-dir/Plugins/dllname/	/Documents And Settings/username/Application Data/winamp-dir/Plugins/dllname/
/Users/username/AppData/Roaming/winamp-dir/Plugins/			/Documents And Settings/username/Application Data/winamp-dir/Plugins/
/Users/username/AppData/Roaming/winamp-dir/dllname/			/Documents And Settings/username/Application Data/winamp-dir/dllname/
/Users/username/AppData/Roaming/winamp-dir/					/Documents And Settings/username/Application Data/winamp-dir/
/Users/username/AppData/Local/winamp-dir/Plugins/dllname/	/Documents And Settings/username/Local Settings/Application Data/winamp-dir/Plugins/dllname/
/Users/username/AppData/Local/winamp-dir/Plugins/			/Documents And Settings/username/Local Settings/Application Data/winamp-dir/Plugins/
/Users/username/AppData/Local/winamp-dir/dllname/			/Documents And Settings/username/Local Settings/Application Data/winamp-dir/dllname/
/Users/username/AppData/Local/winamp-dir/					/Documents And Settings/username/Local Settings/Application Data/winamp-dir/
STANDALONE ONLY
/Program Files/install-dir/									/Program Files/install-dir/
ALL
/Users/username/AppData/Local/install-dir/					/Documents And Settings/username/Local Settings/Application Data/install-dir/
/Users/username/AppData/Roaming/install-dir/				/Documents And Settings/username/Application Data/install-dir/
/ProgramData/install-dir/									/Documents and Settings/All Users/Application Data/install-dir/

*/
static saneConfig saneconfig;

const saneConfig * saneLoadConfigs(PTCHAR cfgFileName, HMODULE hModule, bool bStripUnderscore)
{
	// exename = edcastASIO | edcastStandalone | (dsp_edcast_v3 => edcast ... xxx_blah_xxx => blah)
	bool bDll = hModule != NULL;

	getBaseModuleName(saneconfig.exename, MAX_PATH, hModule, bStripUnderscore);

	GetCurrentDirectory(MAX_PATH, saneconfig.currentDir);

	bool bPlugins = bDll;

	ConfigFinder * cfg = new ConfigFinder(cfgFileName);
	
	cfg->getfolder(0, saneconfig.currentDir);												// .

	cfg->_getDirName(saneconfig.currentDir, saneconfig.appName, 1);
	if (!_tcscmp(_tcsupr(saneconfig.appName), _T("PLUGINS")))
	{
		cfg->_getDirName(saneconfig.currentDir, saneconfig.appName, 2);
	}

	if (bDll) // appName = winamp, exename = dsp_edcast_v3
	{
		if(bPlugins)
		{
			cfg->getfolder(CSIDL_APPDATA, saneconfig.appName, _T("Plugins"), saneconfig.exename);		// /Users/username/AppData/Roaming/winamp/Plugins/edcast/
			cfg->getfolder(CSIDL_APPDATA, saneconfig.appName, _T("Plugins"));							// /Users/username/AppData/Roaming/winamp/Plugins/
		}

		cfg->getfolder(CSIDL_APPDATA, saneconfig.appName, saneconfig.exename);						// /Users/username/AppData/Roaming/winamp/edcast/
		cfg->getfolder(CSIDL_APPDATA, saneconfig.appName);											// /Users/username/AppData/Roaming/winamp/

		if(bPlugins)
		{
			cfg->getfolder(CSIDL_LOCAL_APPDATA, saneconfig.appName, _T("Plugins"), saneconfig.exename);	// /Users/username/AppData/Local/winamp/Plugins/edcast/
			cfg->getfolder(CSIDL_LOCAL_APPDATA, saneconfig.appName, _T("Plugins"));						// /Users/username/AppData/Local/winamp/Plugins/
		}

		cfg->getfolder(CSIDL_LOCAL_APPDATA, saneconfig.appName, saneconfig.exename);				// /Users/username/AppData/Local/winamp/edcast/
		cfg->getfolder(CSIDL_LOCAL_APPDATA, saneconfig.appName);									// /Users/username/AppData/Local/winamp/
	}
	else // appName = installDir exename = edcastASIO | edcastStandalone etc
	{
		//cfg->getfolder(CSIDL_PROGRAM_FILES, saneconfig.appName);									// /Program Files/install-dir/
	}

	cfg->getfolder(CSIDL_LOCAL_APPDATA, saneconfig.appName);										// /Users/username/AppData/Local/install-dir/
	cfg->getfolder(CSIDL_APPDATA, saneconfig.appName);												// /Users/username/AppData/Roaming/install-dir/
	cfg->getfolder(CSIDL_COMMON_APPDATA, saneconfig.appName);										// /ProgramData/install-dir/

	if(!cfg->getConfigFolder())
	{
		if(cfg->getWritableFolder())
		{
			//selectFolder(startup = cfg->getWritableFolder(), exists=true);
		}
		else
		{
			//selectFolder(startup = folders[0].folder, exists=false);
		}
	}

	if(cfg->getConfigFolder())
	{
		_tcscpy(saneconfig.firstConfig, cfg->getConfigFolder());
	}
	else
	{
		_tcscpy(saneconfig.firstConfig, _T(""));
	}

	if(cfg->getWritableFolder())
	{
		_tcscpy(saneconfig.firstWritableFolder, cfg->getWritableFolder());
	}
	else
	{
		_tcscpy(saneconfig.firstWritableFolder, _T(""));
	}

	delete cfg;

	return &saneconfig;
}

