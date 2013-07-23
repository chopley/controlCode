#ifndef genericregs_h
#define genericregs_h

#include "arraymap.h"

/**
 * Set the max length of a source name (including '\0' terminator).
 */
enum {SRC_LEN=30};

enum {SCAN_LEN=30};

// Enumerate the available deck-axis tracking modes.

typedef enum {
  DECK_TRACK,       // Maintain the deck angle at the current
		    // deck-axis tracking offset plus the parallactic
		    // angle of the source.
  DECK_ZERO         // Set the deck angle equal to the current
		    // deck-axis tracking offset.
} DeckMode;

/*
 * Enumerate reconized scan modes.
 */
typedef enum {
  SCAN_INACTIVE = 0x0,
  SCAN_ACTIVE   = 0x1,
  SCAN_START    = 0x2,
  SCAN_BODY     = 0x4,
  SCAN_END      = 0x8,
} ScanMode;

RegMap *new_AntRegMap(void);
long net_AntRegMap_size(void);
int net_put_AntRegMap(gcp::control::NetBuf *net);
RegMap *new_ArrayRegMap(void);
long net_ArrayRegMap_size(void);
int net_put_ArrayRegMap(gcp::control::NetBuf *net);

ArrayMap *new_ArrayMap(void);
long net_ArrayMap_size(void);
int net_put_ArrayMap(gcp::control::NetBuf *net);

void documentArrayMap();

#endif
