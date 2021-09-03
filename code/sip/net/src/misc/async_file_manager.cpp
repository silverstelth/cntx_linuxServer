/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/file.h"
#include "misc/path.h"
#include "misc/async_file_manager.h"

using namespace std;

namespace SIPBASE
{

//CAsyncFileManager *CAsyncFileManager::_Singleton = NULL;
SIPBASE_SAFE_SINGLETON_IMPL(CAsyncFileManager);


// ***************************************************************************

/*CAsyncFileManager::CAsyncFileManager()
{
}
*/
// ***************************************************************************

/*CAsyncFileManager &CAsyncFileManager::getInstance()
{
	if (_Singleton == NULL)
	{
		_Singleton = new CAsyncFileManager();
	}
	return *_Singleton;
}
*/
// ***************************************************************************

void CAsyncFileManager::terminate ()
{
	if (_Instance != NULL)
	{
		ISipContext::getInstance().releaseSingletonPointer("CAsyncFileManager", _Instance);
		delete _Instance;
		_Instance = NULL;
	}
}


void CAsyncFileManager::addLoadTask(IRunnable *ploadTask)
{
	addTask(ploadTask);
}

bool CAsyncFileManager::cancelLoadTask(const CAsyncFileManager::ICancelCallback &callback)
{
	CSynchronized<list<CWaitingTask> >::CAccessor acces(&_TaskQueue);
	list<CWaitingTask> &rTaskQueue = acces.value ();
	list<CWaitingTask>::iterator it = rTaskQueue.begin();

	while (it != rTaskQueue.end())
	{
		IRunnable *pR = it->Task;

		// check the task with the cancel callback.
		if (callback.callback(pR))
		{
			// Delete the load task
			delete pR;
			rTaskQueue.erase (it);
			return true;
		}
		++it;
	}

	// If not found, the current running task may be the one we want to cancel. Must wait it.
	// Beware that this code works because of the CSynchronized access we made above (ensure that the 
	// taskmanager will end just the current task async (if any) and won't start an other one.
	waitCurrentTaskToComplete ();

	return false;
}

// ***************************************************************************
/*	
void CAsyncFileManager::loadMesh(const std::string& meshName, IShape **ppShp, IDriver *pDriver)
{
	addTask (new CMeshLoad(meshName, ppShp, pDriver));
}
*/
// ***************************************************************************
/*
bool CAsyncFileManager::cancelLoadMesh(const std::string& sMeshName)
{
	CSynchronized<list<IRunnable *> >::CAccessor acces(&_TaskQueue);
	list<IRunnable*> &rTaskQueue = acces.value ();
	list<IRunnable*>::iterator it = rTaskQueue.begin();

	while (it != rTaskQueue.end())
	{
		IRunnable *pR = *it;
		CMeshLoad *pML = dynamic_cast<CMeshLoad*>(pR);
		if (pML != NULL)
		{
			if (pML->MeshName == sMeshName)
			{
				// Delete mesh load task
				delete pML;
				rTaskQueue.erase (it);
				return true;
			}
		}
		++it;
	}
	return false;
}
*/
// ***************************************************************************
/*	
void CAsyncFileManager::loadIG (const std::string& IGName, CInstanceGroup **ppIG)
{
	addTask (new CIGLoad(IGName, ppIG));
}

// ***************************************************************************
	
void CAsyncFileManager::loadIGUser (const std::string& IGName, UInstanceGroup **ppIG)
{
	addTask (new CIGLoadUser(IGName, ppIG));
}
*/
// ***************************************************************************
	
void CAsyncFileManager::loadFile (const std::string& sFileName, uint8 **ppFile)
{
	addTask (new CFileLoad (sFileName, ppFile));
}

// ***************************************************************************

void CAsyncFileManager::loadFiles (const std::vector<std::string> &vFileNames, const std::vector<uint8**> &vPtrs)
{
	addTask (new CMultipleFileLoad (vFileNames, vPtrs));
}

// ***************************************************************************

void CAsyncFileManager::signal (bool *pSgn)
{
	addTask (new CSignal (pSgn));
}

// ***************************************************************************

void CAsyncFileManager::cancelSignal (bool *pSgn)
{
	CSynchronized<list<CWaitingTask> >::CAccessor acces(&_TaskQueue);
	list<CWaitingTask> &rTaskQueue = acces.value ();
	list<CWaitingTask>::iterator it = rTaskQueue.begin();

	while (it != rTaskQueue.end())
	{
		IRunnable *pR = it->Task;
		CSignal *pS = dynamic_cast<CSignal*>(pR);
		if (pS != NULL)
		{
			if (pS->Sgn == pSgn)
			{
				// Delete signal task
				delete pS;
				rTaskQueue.erase (it);
				return;
			}
		}
		++it;
	}
}

// ***************************************************************************
// FileLoad
// ***************************************************************************

// ***************************************************************************
CAsyncFileManager::CFileLoad::CFileLoad (const std::string& sFileName, uint8 **ppFile)
{
	_FileName = sFileName;
	_ppFile = ppFile;
}

// ***************************************************************************
void CAsyncFileManager::CFileLoad::run (void)
{
	FILE *f = fopen (_FileName.c_str(), "rb");
	if (f != NULL)
	{
		uint8 *ptr;
		long filesize=CFileA::getFileSize (f);
		//fseek (f, 0, SEEK_END);
		//long filesize = ftell (f);
		//sipSleep(5);
		//fseek (f, 0, SEEK_SET);
		ptr = new uint8[filesize];
		fread (ptr, filesize, 1, f);
		fclose (f);

		*_ppFile = ptr;
	}
	else
	{
		sipwarning ("AFM: Couldn't load '%s'", _FileName.c_str());
		*_ppFile = (uint8*)-1;
	}
}

// ***************************************************************************
void CAsyncFileManager::CFileLoad::getName (std::string &result) const
{
	result = "FileLoad (" + _FileName + ")";
}

// ***************************************************************************
// MultipleFileLoad
// ***************************************************************************

// ***************************************************************************
CAsyncFileManager::CMultipleFileLoad::CMultipleFileLoad (const std::vector<std::string> &vFileNames, 
														 const std::vector<uint8**> &vPtrs)
{
	_FileNames = vFileNames;
	_Ptrs = vPtrs;
}

// ***************************************************************************
void CAsyncFileManager::CMultipleFileLoad::run (void)
{
	for (uint32 i = 0; i < _FileNames.size(); ++i)
	{
		FILE *f = fopen (_FileNames[i].c_str(), "rb");
		if (f != NULL)
		{
			uint8 *ptr;
			long filesize=CFileA::getFileSize (f);
			//fseek (f, 0, SEEK_END);
			//long filesize = ftell (f);
			//sipSleep(5);
			//fseek (f, 0, SEEK_SET);
			ptr = new uint8[filesize];
			fread (ptr, filesize, 1, f);
			fclose (f);

			*_Ptrs[i] = ptr;
		}
		else
		{
			sipwarning ("AFM: Couldn't load '%s'", _FileNames[i].c_str());
			*_Ptrs[i] = (uint8*)-1;
		}
	}

}

// ***************************************************************************
void CAsyncFileManager::CMultipleFileLoad::getName (std::string &result) const
{
	result = "MultipleFileLoad (";
	uint i;
	for (i=0; i<_FileNames.size (); i++)
	{
		if (i)
			result += ", ";
		result += _FileNames[i];
	}
	result += ")";
}
// ***************************************************************************
// Signal
// ***************************************************************************

// ***************************************************************************
CAsyncFileManager::CSignal::CSignal (bool *pSgn)
{
	Sgn = pSgn;
	*Sgn = false;
}

// ***************************************************************************
void CAsyncFileManager::CSignal::run (void)
{
	*Sgn = true;
}

// ***************************************************************************
void CAsyncFileManager::CSignal::getName (std::string &result) const
{
	result = "Signal";
}

} // SIPBASE

