#ifndef genericscript_h
#define genericscript_h

#include "controlscript.h"

/*
 * All commands that send messages to the real-time controller, should
 * call rtc_offline() before attempting to send the command.  This
 * function reports an error and returns 1 if the command should be
 * aborted. Some commands will of course get queued before we know
 * that the connection has been lost. These will be silently discarded
 * by the communications thread.
 */

Script* add_GenericScriptCommands(ControlProg *cp, Script* sc, int batch, 
				  HashTable *signals);

// This must be defined by specific experiments 

Script* add_SpecificScriptCommands(ControlProg *cp, Script* sc, int batch, 
					  HashTable *signals);

#endif
