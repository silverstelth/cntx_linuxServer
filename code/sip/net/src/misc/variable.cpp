/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/variable.h"

using namespace std;
using namespace SIPBASE;

namespace SIPBASE {


void cbVarChanged (CConfigFile::CVar &cvar)
{
	CCommandRegistry &cr = CCommandRegistry::getInstance();
	for (CCommandRegistry::TCommand::iterator comm = cr._Commands.begin(); comm != cr._Commands.end(); comm++)
	{
		if (comm->second->Type == ICommand::Variable && comm->second->getName() == cvar.Name)
		{
			IVariable *var = static_cast<IVariable*>(comm->second);
			string val = cvar.asString();
			sipinfo ("VAR: Setting variable '%s' with value '%s' from config file", cvar.Name.c_str(), val.c_str());
			var->fromString(val, true);
		}
	}
}


void IVariable::init (SIPBASE::CConfigFile &configFile)
{
	CCommandRegistry::getInstance().initVariables(configFile);
}

void CCommandRegistry::initVariables(SIPBASE::CConfigFile &configFile)
{
	for (TCommand::iterator comm = _Commands.begin(); comm != _Commands.end(); comm++)
	{
		if (comm->second->Type == ICommand::Variable)
		{
			IVariable *var = static_cast<IVariable *>(comm->second);
			if (var->_UseConfigFile)
			{
				configFile.setCallback(var->_CommandName, cbVarChanged);
				CConfigFile::CVar *cvar = configFile.getVarPtr(var->_CommandName);
				if (cvar != 0)
				{
					string val = cvar->asString();
#ifdef SIP_OS_WINDWOS
					sipinfo ("VAR: Setting variable '%s' with value '%s' from config file '%s'", var->_CommandName.c_str(), val.c_str(), configFile.getFilename().c_str());
#else
					sipinfo ("VAR: Setting variable '%s' with value '%s' from config file '%S'", var->_CommandName.c_str(), val.c_str(), configFile.getFilename().c_str());
#endif
					var->fromString(val, true);
				}
				else
				{
#ifdef SIP_OS_WINDWOS
					sipdebug ("VAR: No variable '%s' in config file '%s'", var->_CommandName.c_str(), configFile.getFilename().c_str());
#else
					sipdebug ("VAR: No variable '%s' in config file '%S'", var->_CommandName.c_str(), configFile.getFilename().c_str());
#endif
				}
			}
		}
	}
}
	
} // SIPBASE
