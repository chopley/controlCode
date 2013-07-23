#ifndef controlscript_h
#define controlscript_h

#include "gcp/control/code/unix/libscript_src/script.h"
#include "gcp/control/code/unix/control_src/common/genericcontrol.h"

/*
 * An object of the following type is allocated by each schedule script
 * to record non-script objects that need to be garbage collected when 
 * the script is deleted or discarded. It can be found in Script::data.
 */
typedef struct {
  int ref_count;    /* The reference count of the parent Script */
  List *schedules;  /* The list of schedules in use by the script */
} ScheduleData;

Script* new_ControlScript(ControlProg *cp, int batch, HashTable *signals);

int rtc_offline(Script *sc, char *cmd);

void generateAutoDocumentation(Script* sc, std::string dir);

#endif
