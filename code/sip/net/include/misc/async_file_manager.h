/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __ASYNC_FILE_MANAGER_H__
#define __ASYNC_FILE_MANAGER_H__

#include "types_sip.h"
#include "task_manager.h"

namespace SIPBASE
{

/**
 * CAsyncFileManager is a class that manage file loading in a seperate thread
 * \date 2002 
 */
class CAsyncFileManager : public CTaskManager
{
	SIPBASE_SAFE_SINGLETON_DECL(CAsyncFileManager);
	CAsyncFileManager() {}
public:

	// Must be called instead of constructing the object
//	static CAsyncFileManager &getInstance ();
	// NB: release the singleton, but assert it there is any pending loading tasks. 
	// Each systems that use the async file manager should ensure it has no more pending task in it
	static void terminate ();

	// Do not use these methods with the bigfile manager
	void loadFile (const std::string &fileName, uint8 **pPtr);
	void loadFiles (const std::vector<std::string> &vFileNames, const std::vector<uint8**> &vPtrs);

	
	void signal (bool *pSgn); // Signal a end of loading for a group of "mesh or file" added
	void cancelSignal (bool *pSgn);

	/**
	 *	CCancelCallback is an interface that is used in call to CAsyncFileManager::cancelLoad.
	 *	This class contains one method that is call for each task in the async file loader.
	 *	If the method return true, then the task is removed and cancelLoad return.
	 *	\date 10/2002
	 */
	class ICancelCallback
	{
	public:
		virtual bool callback(const IRunnable *prunnable) const =0;
	};

	/// Add a load task in the CAsyncFileManager task list.
	void addLoadTask(IRunnable *ploadTask);
	/** Call this method to cancel a programmed load.
	 *	\return false if the task either already ended or running.
	 */
	bool cancelLoadTask(const ICancelCallback &cancelCallBack);
	
private:

//	CAsyncFileManager (); // Singleton mode -> access it with the getInstance function

//	static CAsyncFileManager *_Singleton;

	// All the tasks
	// -------------
	
	// Load a file
	class CFileLoad : public IRunnable
	{
		std::string _FileName;
		uint8 **_ppFile;
	public:
		CFileLoad (const std::string& sFileName, uint8 **ppFile);
		void run (void);
		void getName (std::string &result) const;
	};

	// Load multiple files
	class CMultipleFileLoad : public IRunnable
	{
		std::vector<std::string> _FileNames;
		std::vector<uint8**> _Ptrs;
	public:
		CMultipleFileLoad (const std::vector<std::string> &vFileNames, const std::vector<uint8**> &vPtrs);
		void run (void);
		void getName (std::string &result) const;
	};

	// Signal
	class CSignal  : public IRunnable
	{
	public:
		bool *Sgn;
	public:
		CSignal (bool *pSgn);
		void run (void);
		void getName (std::string &result) const;
	};

};


} // SIPBASE


#endif // __ASYNC_FILE_MANAGER_H__

/* End of async_file_manager.h */
