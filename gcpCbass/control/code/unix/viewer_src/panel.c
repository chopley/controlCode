#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <tcl.h>
#include <tk.h>

#include "panel.h"
#include "freelist.h"
#include "lprintf.h"

/**
 * Does the current version of tcl use (const char *) declarations or
 * vanilla (char *) ?
*/
#if ((TCL_MAJOR_VERSION<8) || (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION < 4)) 
#define USE_CHAR_DECL 
#endif

/*
 * List widget defaults.
 */
#define DEF_PANEL_BG       "#d9d9d9"  /* Chosen to match tkUnixDefault.h */
#define DEF_PANEL_FG       "black"
#define DEF_PANEL_BD       "2"
#define DEF_PANEL_FONT     "Helvetica -12 bold"
#define DEF_PANEL_RELIEF   "raised"
#define DEF_PANEL_WIDTH    "0"
#define DEF_PANEL_HEIGHT   "0"
#define DEF_PANEL_VARIABLE ""
#define DEF_PANEL_CURSOR   ""
#define DEF_PANEL_ARRAY    ""
#define DEF_WARN_COLOR     "red"
#define DEF_WARN_WIDTH     "2"
#define DEF_COL_WIDTH      "10"    /* Characters */
#define DEF_CELL_PADX      "2"
#define DEF_CELL_PADY      "2"
#define DEF_GRID_SIZE      "0 0"   /* sprintf(spec, "%d %d", ncol=0, nrow=0) */

/*
 * When a panel widget is first created it is created with ROW_BLK_FACTOR
 * rows and COL_BLK_FACTOR columns. Thereafter whenever memory for a new
 * column or row needs to be allocated, the corresponding parameter is
 * used to determine the number of columns or rows to actually allocate.
 * Making these parameters greater than 1 reduces the number of times
 * realloc() has to be called when the grid is being incrementally
 * constructed. Note that since the height of a row is generally much
 * less than the width of a column we should allow for more rows than
 * columns. In particular there are unlikely to be much more than 10
 * columns, or more than 40 rows.
 */
#define COL_BLK_FACTOR 4
#define ROW_BLK_FACTOR 16

/*
 * Fields nodes are allocated from a freelist, which in turn allocates
 * fields in blocks and hands them on demand. The following parameter
 * specifies the number of fields per block.
 */
#define FIELD_BLK_FACTOR 32

/*
 * Set an arbitrary max number of columns.
 */
#define PANEL_MAX_COL 100
#define PANEL_MAX_ROW 100

/*
 * The following datatype is used to record details about a single
 * column of fields.
 */
typedef struct PanelColumn PanelColumn;
struct PanelColumn {
  int x;                /* The X-coordinate of the left edge of the field */
  int width;            /* The total width of the field (pixels) */
  int nchar;            /* The width available for text (characters) */
};

/*
 * Assign single-bit values to the following field-status attributes
 */
typedef enum {
  FIELD_REDRAW_TEXT=1,     /* A redraw operation has been queued, and */
                           /*  that operation should redraw the text. */
  FIELD_REDRAW_WARN=2,     /* A redraw operation has been queued, and */
                           /*  that operation should redraw the warning */
                           /*  border. */
  FIELD_REDRAW_3D=4,       /* A redraw operation has been queued, and */
                           /*  that operation should redraw the 3d border */
  FIELD_REDRAW_ALL = FIELD_REDRAW_TEXT | FIELD_REDRAW_WARN | FIELD_REDRAW_3D,
  FIELD_WARN_ACTIVE=8      /* The warning border is active */
} FieldFlag;

/*
 * Enumerate the ways in which the text can be placed horizontally.
 */
typedef enum {
  FIELD_PACK_LEFT,         /* Place the leftmost edge of the text at the */
                           /*  leftmost edge of the field. */
  FIELD_PACK_CENTER,       /* Place the center of the text at the */
                           /*  center of the field. */
  FIELD_PACK_RIGHT,        /* Place the rightmost edge of the text at the */
                           /*  rightmost edge of the field. */
} FieldPack;

/*
 * The following datatype is used to record the details about a single
 * active cell.
 */
typedef struct PanelField PanelField;
struct PanelField {
  PanelField *next;         /* When the field is a member of the list */
                            /*  of fields that have been queued to be */
                            /*  redrawn, this member points at the next */
                            /*  member of that list. */
  char *text;               /* The text to display in the field */
                            /*  This is a pointer to the contents of a */
                            /*  TCL variable - so don't modify or free it. */
  int col, row;             /* The row and column locations of the field */
  XColor *bg;               /* The background color of the field */
  XColor *fg;               /* The foreground color of the field */
  unsigned flags;           /* A bit-mask union of FieldFlag flags */
  GC fill_gc;               /* The graphical context to use when drawing */
                            /*  the background rectangle. */
  GC ok_gc;                 /* The graphical context to use when drawing */
                            /*  the inactive warning border */
  GC text_gc;               /* The graphical context to use when drawing */
                            /*  the field text. */
  FieldPack pack;           /* How to arrange the text horizontally */
};

typedef struct {
  Tk_Window tkwin;          /* Tk's window object */
  Display *display;         /* The X display of the window */
  Tcl_Interp *interp;       /* The application's TCL interpreter */
  Tcl_Command tcl_cmd;      /* The instance command of this widget */

                      /* Public resource attributes */
  Tk_3DBorder border;       /* 3D border structure for the widget borders */
  XColor *normalFg;         /* The default foreground color of the cells. */
  Tk_Cursor cursor;         /* The active cursor of the widget */
  int borderWidth;          /* The width of the widget's 3D border */
  int relief;               /* Relief of the widget's 3D border */
  int req_width;            /* The requested widget width (pixels) */
  int req_height;           /* The requested widget height (pixels) */
  int column_width;         /* The initial width to give new columns */
  Tk_Font font;             /* The font to use in all cells */
  XColor *warn_color;       /* The color of the activated warning border */
  GC warn_color_gc;         /* The graphical context to use when drawing */
                            /*  active warning borders. */
  int warn_width;           /* The width of the warning border */
  int padx;                 /* The number of pixels to leave either side */
                            /*  of the text. */
  int pady;                 /* The number of pixels to leave above and below */
                            /*  the text. */
  char size_spec[16];       /* The current size of the grid, written using */
                            /*  sprintf(size_spec, "%d %d", ncol, nrow). */
  char *tcl_array;          /* The TCL array that contains the string values */
                            /*  of each cell, indexed by $col,$row */
                      /* Implementation parameters */
  char *tcl_array_copy;     /* A malloc'd copy of tcl_array[], used solely to */
                            /*  determine when tcl_array[] is changed */
                            /*  by Tk_ConfigureWidget() */
  XColor *black;            /* The color to use in black_dots_gc */
  GC black_dots_gc;         /* A graphical context for drawing dotted */
                            /*  black lines */
  XColor *white;            /* The color to use in white_dots_gc */
  GC white_dots_gc;         /* A graphical context for drawing dotted */
                            /*  white lines */
  FreeList *field_mem;      /* The memory allocator for fields */
  int ncol, nrow;           /* The current number of columns and rows in */
                            /*  the grid. */
  int max_ngrid;            /* The number of elements allocated to grid[] */
  PanelField **grid;        /* A one-dimensional array of max_ngrid */
                            /*  field pointers interpretted as a 2D array */
                            /*  with ncol columns and nrow rows. */
  int max_ncol;             /* The number of elements allocated to columns[] */
  PanelColumn *columns;     /* An array of max_ncol > ncol column */
                            /*  description objects. */
  PanelField *redraw_fields;/* The list of fields that have been queued to be */
                            /* redrawn. */
  int redraw_queued;        /* True if the widget has been queued to be */
                            /*  redrawn. */
  int char_width;           /* The average width of characters in the chosen */
                            /*  font (pixels). */
  int cell_height;          /* The height of a grid cell */
  unsigned long event_mask; /* The list of events being watched */
} TkPanel;

static TkPanel *new_TkPanel(Tcl_Interp *interp, char *name,
			    int argc, const char *argv[]);
static TkPanel *del_TkPanel(TkPanel *panel);

/*
 * The following functions are used to parse and print the value of the
 * -size {ncol nrow} configuration specification.
 */
#ifdef USE_CHAR_DECL
static int parse_size_spec(ClientData context, Tcl_Interp *interp,
			   Tk_Window tkwin, char *value, char *panel_ptr,
			   int offset);
#else
static int parse_size_spec(ClientData context, Tcl_Interp *interp,
			   Tk_Window tkwin, const char *value, char *panel_ptr,
			   int offset);
#endif

static int bad_size_spec(TkPanel *panel);

#ifdef USE_CHAR_DECL
static char *print_size_spec(ClientData context, Tk_Window tkwin,
			     char *panel_ptr, int offset,
			     Tcl_FreeProc **free_proc);
#else
static char *print_size_spec(ClientData context, Tk_Window tkwin,
			     char *panel_ptr, int offset,
			     Tcl_FreeProc **free_proc);
#endif

static Tk_CustomOption config_custom_size = {
  parse_size_spec,  /* The function to call to parse a size specifier string */
  print_size_spec   /* The function to call to print a size specifier string */
};

/*
 * Describe all recognized widget resources.
 */
static Tk_ConfigSpec configSpecs[] = {

  {TK_CONFIG_BORDER,
     "-background", "background", "Background",
     DEF_PANEL_BG, Tk_Offset(TkPanel, border), 0},
  {TK_CONFIG_SYNONYM,
     "-bg", "background", (char *) NULL, NULL, 0, 0},

  {TK_CONFIG_COLOR,
     "-foreground", "foreground", "Foreground",
     DEF_PANEL_FG, Tk_Offset(TkPanel, normalFg), 0},
  {TK_CONFIG_SYNONYM,
     "-fg", "foreground", (char *) NULL, NULL, 0, 0},

  {TK_CONFIG_ACTIVE_CURSOR,
     "-cursor", "cursor", "Cursor",
     DEF_PANEL_CURSOR, Tk_Offset(TkPanel, cursor), TK_CONFIG_NULL_OK},

  {TK_CONFIG_PIXELS,
     "-borderwidth", "borderWidth", "BorderWidth",
     DEF_PANEL_BD, Tk_Offset(TkPanel, borderWidth), 0},
  {TK_CONFIG_SYNONYM,
     "-bd", "borderWidth", (char *) NULL, NULL, 0, 0},
  
  {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
     DEF_PANEL_RELIEF, Tk_Offset(TkPanel, relief), 0},

  {TK_CONFIG_PIXELS,
     "-height", "height", "Height",
     DEF_PANEL_HEIGHT, Tk_Offset(TkPanel, req_height), 0},

  {TK_CONFIG_PIXELS,
     "-width", "width", "Width",
     DEF_PANEL_WIDTH, Tk_Offset(TkPanel, req_width), 0},

  {TK_CONFIG_INT,
     "-column_width", "column_width", "ColumnWidth",
     DEF_COL_WIDTH,   Tk_Offset(TkPanel, column_width), 0},

  {TK_CONFIG_FONT, "-font", "font", "Font",
     DEF_PANEL_FONT, Tk_Offset(TkPanel, font), 0},

  {TK_CONFIG_PIXELS,
     "-warn_width", "warn_width", "Width",
     DEF_WARN_WIDTH, Tk_Offset(TkPanel, warn_width), 0},

  {TK_CONFIG_PIXELS,
     "-padx", "padX", "Pad",
     DEF_CELL_PADX, Tk_Offset(TkPanel, padx), 0},

  {TK_CONFIG_PIXELS,
     "-pady", "padY", "Pad",
     DEF_CELL_PADY, Tk_Offset(TkPanel, pady), 0},

  {TK_CONFIG_COLOR,
     "-warn_color",    "warn_color", "Foreground",
     DEF_WARN_COLOR, Tk_Offset(TkPanel, warn_color), 0},

  {TK_CONFIG_STRING, "-array", "array", "Array",
     DEF_PANEL_ARRAY, Tk_Offset(TkPanel, tcl_array),
     TK_CONFIG_NULL_OK},

  {TK_CONFIG_CUSTOM, "-size",  "size",   "Size",
     DEF_GRID_SIZE,  Tk_Offset(TkPanel, size_spec),
     0, &config_custom_size},

  {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, 0}
};

#ifdef USE_CHAR_DECL
static int PanelCmd(ClientData context, Tcl_Interp *interp, int argc,
		    char *argv[]);
#else
static int PanelCmd(ClientData context, Tcl_Interp *interp, int argc,
		    const char *argv[]);
#endif

#ifdef USE_CHAR_DECL
static int panel_InstanceCommand(ClientData context, Tcl_Interp *interp,
				 int argc, char *argv[]);
#else
static int panel_InstanceCommand(ClientData context, Tcl_Interp *interp,
				 int argc, const char *argv[]);
#endif

static int panel_InstanceCommand_return(ClientData context, int iret);
static int panel_Configure(TkPanel *panel, Tcl_Interp *interp,
			   int argc, const char *argv[], int flags);
static int panel_cell_command(TkPanel *panel, Tcl_Interp *interp,
			      char *widget, char *cell,
			      int argc, char *argv[]);
static int field_flag_cmd(TkPanel *panel, PanelField *field,
			  int argc, char *argv[]);
static int field_bg_cmd(TkPanel *panel, PanelField *field,
			int argc, char *argv[]);
static int field_fg_cmd(TkPanel *panel, PanelField *field,
			int argc, char *argv[]);
static int field_pack_cmd(TkPanel *panel, PanelField *field,
			  int argc, char *argv[]);
static void panel_redraw_callback(ClientData context);
static void queue_panel_redraw(TkPanel *panel);
static int redraw_panel(TkPanel *panel);

#ifdef USE_CHAR_DECL
static char *panel_array_callback(ClientData context, Tcl_Interp *interp,
				  char *name1, char *name2, 
				  int flags);
#else
static char *panel_array_callback(ClientData context, Tcl_Interp *interp,
				  const char *name1, const char *name2, 
				  int flags);
#endif

static void queue_field_redraw(TkPanel *panel, PanelField *field,
			       unsigned what);
static void field_redraw_callback(ClientData context);
static int panel_update_geometry(TkPanel *panel);
static void panel_EventHandler(ClientData context, XEvent *event);
static void panel_expose_handler(TkPanel *panel, XEvent *event);
static void panel_FreeProc(char *context);
static void update_field_text_gc(TkPanel *panel, PanelField *field);
static void update_field_fill_gc(TkPanel *panel, PanelField *field);
static void update_field_ok_gc(TkPanel *panel, PanelField *field);
static PanelField *get_PanelField(TkPanel *panel, int col, int row);
static PanelField *rem_PanelField(TkPanel *panel, int col, int row);
static PanelField *del_PanelField(TkPanel *panel, PanelField *field);
static void clr_PanelFields(TkPanel *panel);
static int redraw_fields(TkPanel *panel);
static int update_field(TkPanel *panel, PanelField *field);
static int panel_column_command(TkPanel *panel, Tcl_Interp *interp,
				char *widget, int argc, char *argv[]);
static int panel_location_command(TkPanel *panel, Tcl_Interp *interp,
				  char *widget, int argc, char *argv[]);
static int column_width_cmd(TkPanel *panel, PanelColumn *column,
			    int argc, char *argv[]);
static int panel_delete_command(TkPanel *panel, Tcl_Interp *interp,
				char *widget, int argc, char *argv[]);
static int panel_shrink_command(TkPanel *panel, Tcl_Interp *interp,
				char *widget, int argc, char *argv[]);
static int panel_bbox_command(TkPanel *panel, Tcl_Interp *interp,
			      char *widget, int argc, char *argv[]);
static int field_flag_cmd(TkPanel *panel, PanelField *field,
			  int argc, char *argv[]);
static PanelColumn *get_panel_column(TkPanel *panel, int col);
static int resize_panel_grid(TkPanel *panel, int ncol, int nrow);
static void untrace_panel_array(TkPanel *panel);
static void trace_panel_array(TkPanel *panel);

/*.......................................................................
 * Provide a package initialization procedure. This creates the Tcl
 * "panel" widget creation command.
 *
 * Input:
 *  interp  Tcl_Interp *  The TCL interpreter of the application.
 * Output:
 *  return         int    TCL_OK    - Success.
 *                        TCL_ERROR - Failure.
 */
int Tkpanel_Init(Tcl_Interp *interp)
{
/*
 * Create the TCL command that is to be used for creating panel widgets.
 */
  Tcl_CreateCommand(interp, "panel", PanelCmd, NULL, 0);
  return TCL_OK;
}

/*.......................................................................
 * This function provides the TCL command that creates a panel widget.
 *
 * Input:
 *  context   ClientData    The client_data argument specified in
 *                          Tkpanel_Init() when PanelCmd was registered.
 *                          This is the main window cast to (ClientData).
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  argc             int    The number of command arguments.
 *  argv            char ** The array of 'argc' command arguments.
 *                          argv[0] = "panel"
 *                          argv[1] = the name to give the new widget.
 *                          argv[2..argc-1] = attribute settings.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
#ifdef USE_CHAR_DECL
static int PanelCmd(ClientData context, Tcl_Interp *interp, int argc,
		    char *argv[])
#else
static int PanelCmd(ClientData context, Tcl_Interp *interp, int argc,
		    const char *argv[])
#endif
{
  TkPanel *panel;                          /* The new widget instance object */
/*
 * Make sure that a name for the new widget has been provided.
 */
  if(argc < 2) {
    Tcl_AppendResult(interp, "Wrong number of arguments - should be \'",
		     argv[0], " pathName \?options\?\'", NULL);
    return TCL_ERROR;
  };
/*
 * Allocate the widget-instance object.
 */
  panel = new_TkPanel(interp, (char* )argv[1], argc-2, (const char**) (argv+2));
  if(!panel)
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Create a new widget instance object.
 *
 * Input:
 *  interp   Tcl_Interp *  The TCL interpreter object.
 *  name           char *  The name to give the new widget.
 *  argc            int    The number of argument in argv[]
 *  argv           char ** Any configuration arguments.
 * Output:
 *  return     TkPanel *  The new panel widget, or NULL on error.
 *                         If NULL is returned then the context of the
 *                         error will have been recorded in the result
 *                         field of the interpreter.
 */
static TkPanel *new_TkPanel(Tcl_Interp *interp, char *name,
			    int argc, const char *argv[])
{
  TkPanel *panel;       /* The new widget object */
  unsigned long mask=0; /* The bit-wise union of attributes given in 'values' */
  XGCValues values;     /* The container of the graphical attributes to be */
                        /*  associated with a new GC */
  int i;
/*
 * Get the main window.
 */
  Tk_Window main_w = Tk_MainWindow(interp);
/*
 * Allocate the container.
 */
  panel = (TkPanel *) malloc(sizeof(TkPanel));
  if(!panel) {
    Tcl_AppendResult(interp, "Insufficient memory to create ", name, NULL);
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the container
 * at least up to the point at which it can safely be passed to
 * del_TkPanel().
 */
  panel->tkwin = NULL;
  panel->display = Tk_Display(main_w);
  panel->interp = interp;
  panel->tcl_cmd = NULL;
  panel->border = NULL;
  panel->normalFg = NULL;
  panel->cursor = NULL;
  panel->borderWidth = 0;
  panel->relief = TK_RELIEF_RAISED;
  panel->req_width = 0;
  panel->req_height = 0;
  panel->column_width = 0;
  panel->font = NULL;
  panel->warn_color = NULL;
  panel->warn_color_gc = NULL;
  panel->warn_width = 0;
  panel->padx = 0;
  panel->pady = 0;
  strcpy(panel->size_spec, DEF_GRID_SIZE);
  panel->tcl_array = NULL;
  panel->tcl_array_copy = NULL;
  panel->black = NULL;
  panel->black_dots_gc = NULL;
  panel->white = NULL;
  panel->white_dots_gc = NULL;
  panel->field_mem = NULL;
  panel->ncol = 0;
  panel->nrow = 0;
  panel->max_ngrid = 0;
  panel->grid = NULL;
  panel->max_ncol = 0;
  panel->columns = NULL;
  panel->redraw_fields = NULL;
  panel->redraw_queued = 0;
  panel->char_width = 0;
  panel->cell_height = 0;
  panel->event_mask = NoEventMask;
/*
 * Create the widget window from the specified path.
 */
  panel->tkwin = Tk_CreateWindowFromPath(interp, main_w, name, NULL);
  if(!panel->tkwin)
    return del_TkPanel(panel);
/*
 * Give the widget a class name.
 */
  Tk_SetClass(panel->tkwin, "Panel");
/*
 * Create the TCL command that will allow users to configure the widget.
 */
  panel->tcl_cmd = Tcl_CreateCommand(interp, name, panel_InstanceCommand,
				     (ClientData) panel, 0);
  if(!panel->tcl_cmd)
    return del_TkPanel(panel);
/*
 * Acquire the resources necessary for drawing black dotted lines.
 */
  panel->black = Tk_GetColor(panel->interp, panel->tkwin,
			     Tk_GetUid("black"));
  if(!panel->black)
    return del_TkPanel(panel);
  mask = GCForeground | GCLineStyle | GCDashList;
  values.foreground = panel->black->pixel;
  values.line_style = LineOnOffDash;
  values.dashes = 1;
  panel->black_dots_gc = Tk_GetGC(panel->tkwin, mask, &values);
  if(!panel->black_dots_gc)
    return del_TkPanel(panel);
/*
 * Acquire the resources necessary for drawing white dotted lines.
 */
  panel->white = Tk_GetColor(panel->interp, panel->tkwin,
			     Tk_GetUid("white"));
  if(!panel->white)
    return del_TkPanel(panel);
  mask = GCForeground | GCLineStyle | GCDashList;
  values.foreground = panel->white->pixel;
  values.line_style = LineOnOffDash;
  values.dashes = 1;
  panel->white_dots_gc = Tk_GetGC(panel->tkwin, mask, &values);
  if(!panel->white_dots_gc)
    return del_TkPanel(panel);
/*
 * Create the memory allocator for PanelField objects.
 */
  panel->field_mem = new_FreeList("new_TkPanel", sizeof(PanelField),
				  FIELD_BLK_FACTOR);
  if(!panel->field_mem) {
    Tcl_AppendResult(interp, "Failed to allocate freelist.", NULL);
    return del_TkPanel(panel);
  };
/*
 * Create an empty starting grid of COL_BLK_FACTOR * ROW_BLK_FACTOR
 * cells.
 */
  panel->max_ngrid = COL_BLK_FACTOR * ROW_BLK_FACTOR;
  panel->grid = (PanelField** )malloc(panel->max_ngrid * sizeof(PanelField *));
  if(!panel->grid) {
    Tcl_AppendResult(interp, "Failed to allocate initial field grid.", NULL);
    return del_TkPanel(panel);
  };
/*
 * Clear the grid.
 */
  for(i=0; i<panel->max_ngrid; i++)
    panel->grid[i] = NULL;
/*
 * Create an initial array of COL_BLK_FACTOR column description objects.
 */
  panel->max_ncol = COL_BLK_FACTOR;
  panel->columns = (PanelColumn* )malloc(panel->max_ncol * sizeof(PanelColumn));
  if(!panel->columns) {
    Tcl_AppendResult(interp, "Failed to allocate initial column headers.",
		     NULL);
    return del_TkPanel(panel);
  };
/*
 * Mark all of the columns as having zero width.
 */
  for(i=0; i<panel->max_ncol; i++) {
    PanelColumn *pc = panel->columns + i;
    pc->x = 0;
    pc->width = 0;
    pc->nchar = 0;
  };
/*
 * Register an event handler.
 */
  panel->event_mask = ((unsigned long)(StructureNotifyMask | ExposureMask));
  Tk_CreateEventHandler(panel->tkwin, panel->event_mask, panel_EventHandler,
			(ClientData) panel);
/*
 * Parse command line and widget defaults into *panel.
 */
  if(panel_Configure(panel, interp, argc, argv, 0))
    return del_TkPanel(panel);
/*
 * Return the widget name.
 */
  Tcl_SetResult(interp, Tk_PathName(panel->tkwin), TCL_STATIC);
  return panel;
}

/*.......................................................................
 * Delete a TkPanel widget.
 *
 * Input:
 *  panel    TkPanel *   The widget to be deleted.
 * Output:
 *  return  TkPanel *   Always NULL.
 */
static TkPanel *del_TkPanel(TkPanel *panel)
{
  if(panel) {
/*
 * Delete the Tcl command attached to the widget.
 */
    if(panel->tcl_cmd)
      Tcl_DeleteCommandFromToken(panel->interp, panel->tcl_cmd);
/*
 * Delete the variable trace on panel->tcl_array.
 */
    untrace_panel_array(panel);
/*
 * Delete derived resources.
 */
    if(panel->warn_color_gc)
      Tk_FreeGC(panel->display, panel->warn_color_gc);
    if(panel->black)
      Tk_FreeColor(panel->black);
    if(panel->black_dots_gc)
      Tk_FreeGC(panel->display, panel->black_dots_gc);
    if(panel->white)
      Tk_FreeColor(panel->white);
    if(panel->white_dots_gc)
      Tk_FreeGC(panel->display, panel->white_dots_gc);
/*
 * Delete the grid and its contents.
 */
    if(panel->grid) {
      int col, row;     /* Column and row indexes within the grid */
/*
 * Locate all extant fields, remove them from the grid and return them
 * to the field free-list.
 */
      PanelField **cell = panel->grid;
      for(row=0; row < panel->nrow; row++) {
	for(col=0; col < panel->ncol; col++,cell++)
	  *cell = del_PanelField(panel, *cell);
      };
/*
 * Delete the field free-list.
 */
      panel->field_mem = del_FreeList("del_TkPanel", panel->field_mem, 1);
/*
 * Having emptied the grid of fields, delete it.
 */
      if(panel->grid) {
	free(panel->grid);
	panel->grid = NULL;
	panel->max_ngrid = 0;
      };
/*
 * The column array doesn't contain any resources to be deallocated, so
 * simply delete it.
 */
      if(panel->columns) {
	free(panel->columns);
	panel->columns = NULL;
	panel->max_ncol = 0;
      };
    };
/*
 * Delete resource values.
 */
    if(panel->display)
      Tk_FreeOptions(configSpecs, (char *) panel, panel->display, 0);
/*
 * Delete our private copy of the array name.
 */
    if(panel->tcl_array_copy)
      free(panel->tcl_array_copy);
/*
 * Remove the DestroyNotify event handler before destroying the
 * window. Otherwise this function would call itself recursively.
 */
    if(panel->event_mask != NoEventMask) {
      Tk_DeleteEventHandler(panel->tkwin, panel->event_mask,
			    panel_EventHandler, (ClientData) panel);
      panel->event_mask = NoEventMask;
    };
/*
 * Zap the window.
 */
    if(panel->tkwin) {
      Tk_DestroyWindow(panel->tkwin);
      panel->tkwin = NULL;
    };
/*
 * Delete the container.
 */
    free(panel);
  };
  return NULL;
}

/*.......................................................................
 * This function services TCL commands for a given widget.
 *
 * Input:
 *  context   ClientData    The panel widget cast to (ClientData).
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  argc             int    The number of command arguments.
 *  argv            char ** The array of 'argc' command arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
#ifdef USE_CHAR_DECL
static int panel_InstanceCommand(ClientData context, Tcl_Interp *interp,
				 int argc, char *argv[])
#else
static int panel_InstanceCommand(ClientData context, Tcl_Interp *interp,
				 int argc, const char *argv[])
#endif
{
  TkPanel *panel = (TkPanel *) context;
  char *widget;     /* The name of the widget */
  char *command;    /* The name of the command */
/*
 * Get the name of the widget.
 */
  widget = (char* )argv[0];
/*
 * Get the name of the command.
 */
  if(argc < 2) {
    Tcl_AppendResult(interp, "Missing arguments to ", widget, " command.",
		     NULL);
    return TCL_ERROR;
  };
  command = (char* )argv[1];
/*
 * Prevent untimely deletion of the widget while this function runs.
 * Note that following this statement you must return via
 * panel_InstanceCommand_return() to ensure that Tcl_Release() gets called.
 */
  Tcl_Preserve(context);
/*
 * Is the next argument potentially a field address?
 */
  if(isdigit((int) command[0])) {
    return panel_InstanceCommand_return(context,
		      panel_cell_command(panel, interp, widget, command,
		      argc-2, (char** )(argv+2)));
/*
 * Query what cell encloses a given panel-window x,y coordinate.
 */
  } else if(strcmp(command, "location") == 0) {
    return panel_InstanceCommand_return(context,
	      panel_location_command(panel, interp, widget, argc-2, 
				     (char** )(argv+2)));
/*
 * Check for recognized command names.
 */
  } else if(strcmp(command, "configure") == 0) {  /* Configure widget */
/*
 * Check the number of configure arguments.
 */
    switch(argc - 2) {
    case 0:   /* Return the values of all configuration options */
      return panel_InstanceCommand_return(context,
			Tk_ConfigureInfo(interp, panel->tkwin, configSpecs,
					 (char *) panel, NULL, 0));
      break;
    case 1:   /* Return the value of a single given configuration option */
      return panel_InstanceCommand_return(context,
		        Tk_ConfigureInfo(interp, panel->tkwin, configSpecs,
					 (char *) panel, (char* )argv[2],
					 0));
      break;
    default:  /* Change one of more of the configuration options */
      return panel_InstanceCommand_return(context,
			panel_Configure(panel, interp, argc-2, 
					(const char**)(argv+2),
					TK_CONFIG_ARGV_ONLY));
      break;
    };
  } else if(strcmp(command, "cget") == 0) {  /* Get a configuration value */
    if(argc != 3) {
      Tcl_AppendResult(interp, "Wrong number of arguments to \"", widget,
		       " cget\" command", NULL);
      return panel_InstanceCommand_return(context, TCL_ERROR);
    } else {
      return panel_InstanceCommand_return(context,
			Tk_ConfigureValue(interp, panel->tkwin, configSpecs,
					  (char *) panel, (char* )argv[2], 0));
    };
  } else if(strcmp(command, "delete") == 0) { /* Remove a field from the grid */
    return panel_InstanceCommand_return(context,
		      panel_delete_command(panel, interp, widget,
		      argc-2, (char** )(argv+2)));
  } else if(strcmp(command, "column") == 0) { /* Configure a column */
    return panel_InstanceCommand_return(context,
		      panel_column_command(panel, interp, widget,
		      argc-2, (char** )(argv+2)));
  } else if(strcmp(command, "shrink") == 0) { /* Shrink wrap the widget */
    return panel_InstanceCommand_return(context,
		      panel_shrink_command(panel, interp, widget,
		      argc-2, (char** )(argv+2)));
  } else if(strcmp(command, "bbox") == 0) {   /* Inquire bounding boxes */
    return panel_InstanceCommand_return(context,
		      panel_bbox_command(panel, interp, widget,
		      argc-2, (char** )(argv+2)));
  };
/*
 * Unknown command name.
 */
  Tcl_AppendResult(interp, "Unknown command \"", widget, " ", command, "\"",
		   NULL);
  return panel_InstanceCommand_return(context, TCL_ERROR);
}

/*.......................................................................
 * This is a private cleanup-return function of panel_InstanceCommand().
 * It should be used to return from said function after Tcl_Preserve() has
 * been called. It calls Tcl_Release() on the widget to unblock deletion
 * and returns the specified error code.
 *
 * Input:
 *  context   ClientData    The panel widget cast to (ClientData).
 *  iret             int    TCL_OK or TCL_ERROR.
 * Output:
 *  return           int    The value of iret.
 */
static int panel_InstanceCommand_return(ClientData context, int iret)
{
  Tcl_Release(context);
  return iret;
}

/*.......................................................................
 * This function services TCL configuration commands for a given widget.
 *
 * Input:
 *  panel        TkPanel *  The widget record to be configured.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  argc             int    The number of configuration arguments.
 *  argv            char ** The array of 'argc' configuration arguments.
 *  flags            int    The flags argument of Tk_ConfigureWidget():
 *                           0                - No flags.
 *                           TK_CONFIG_ARGV_ONLY - Override the X defaults
 *                                              database and the configSpecs
 *                                              defaults.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_Configure(TkPanel *panel, Tcl_Interp *interp,
			   int argc, const char *argv[], int flags)
{
  GC warn_color_gc;     /* The graphical context used when drawing active */
                        /*  warning borders. */
  unsigned long mask=0; /* The bit-wise union of attributes given in 'values' */
  XGCValues values;     /* The container of the graphical attributes to be */
                        /*  associated with the new GC */
  int error_code = TCL_OK;  /* The return code of this function */
  int old_warn_width;   /* The value of panel->warn_width before */
                        /*  the call to Tk_ConfigureWidget */
/*
 * Stop tracing the Tcl cell-value array.
 */
  untrace_panel_array(panel);
/*
 * Record the current value of the warning width resource, so that
 * we can tell if it gets changed.
 */
  old_warn_width = panel->warn_width;
/*
 * Install the new defaults in panel.
 */
#ifdef USE_CHAR_DECL
  if(Tk_ConfigureWidget(interp, panel->tkwin, configSpecs, argc, (char**)argv,
			(char *) panel, flags) == TCL_ERROR)
    return TCL_ERROR;
#else
  if(Tk_ConfigureWidget(interp, panel->tkwin, configSpecs, argc, argv,
			(char *) panel, flags) == TCL_ERROR)
    return TCL_ERROR;
#endif
/*
 * Has a Tcl array variable been provided?
 */
  if(panel->tcl_array) {
/*
 * If the variable doesn't exist the following call will return NULL.
 */
    if(Tcl_GetVar(interp, panel->tcl_array, TCL_GLOBAL_ONLY) == NULL) {
/*
 * Attempt to create the variable as an array. The only way to tell
 * Tcl that a variable is to be created as an array rather than a
 * scalar, is to create it via setting an array element, so create a
 * dummy element then delete it. 
 */
      if(Tcl_SetVar2(interp, panel->tcl_array, "tmp", "",
		     TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL ||
	 Tcl_UnsetVar2(interp, panel->tcl_array, "tmp",
		     TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == TCL_ERROR)
	return TCL_ERROR;
    };
/*
 * Ask to be told whenever any member of the array is written to or unset.
 */
    trace_panel_array(panel);
  };
/*
 * Has the user switched to a new Tcl grid array?
 */
  if(panel->tcl_array_copy == NULL || panel->tcl_array == NULL ||
     strcmp(panel->tcl_array, panel->tcl_array_copy) != 0) {
/*
 * The string contents of each existing field belong to the old array,
 * so we must discard them before they can be deleted.
 */
    clr_PanelFields(panel);
/*
 * Discard the cached name of the previous array.
 */
    if(panel->tcl_array_copy)
      free(panel->tcl_array_copy);
/*
 * Make a copy of the new array name, if there is a new array.
 */
    if(panel->tcl_array) {
      panel->tcl_array_copy = (char* )malloc(strlen(panel->tcl_array) + 1);
      if(!panel->tcl_array_copy) {
	Tcl_AppendResult(panel->interp,
			 "Unable to allocate copy of array name.", NULL);
	error_code = TCL_ERROR;
      };
      strcpy(panel->tcl_array_copy, panel->tcl_array);
    };
  };
/*
 * The default column width should not be negative.
 */
  if(panel->column_width < 0)
    panel->column_width = 0;
/*
 * Record the potentially changed background color of the widget.
 */
  Tk_SetBackgroundFromBorder(panel->tkwin, panel->border);
/*
 * If the width of the warning border was changed, update the per-field
 * graphical contexts that are used to draw the warning border when it
 * is inactive.
 */
  if(panel->warn_width != old_warn_width) {
    int col, row;
    PanelField **cell = panel->grid;
    for(row=0; row < panel->nrow; row++) {
      for(col=0; col < panel->ncol; col++,cell++) {
	if(*cell)
	  update_field_ok_gc(panel, *cell);
      };
    };
  };
/*
 * Update the graphical context used for drawing active warning
 * borders.
 */
  values.foreground = panel->warn_color->pixel;
  mask |= GCForeground;
  values.line_width = panel->warn_width;
  mask |= GCLineWidth;
  values.graphics_exposures = False;
  mask |= GCGraphicsExposures;
  warn_color_gc = Tk_GetGC(panel->tkwin, mask, &values);
  if(panel->warn_color_gc)
    Tk_FreeGC(panel->display, panel->warn_color_gc);
  panel->warn_color_gc = warn_color_gc;
/*
 * Cater for changes in the chosen font by updating the graphical contexts
 * that are used to draw text in each field.
 */
  {
    int col, row;
    PanelField **cell = panel->grid;
    for(row=0; row < panel->nrow; row++) {
      for(col=0; col < panel->ncol; col++,cell++) {
	if(*cell)
	  update_field_text_gc(panel, *cell);
      };
    };
  };
/*
 * Compute an appropriate size for the widget.
 */
  if(panel_update_geometry(panel))
    return TCL_ERROR;
/*
 * Arrange for the widget to be redrawn during idle time.
 */
  queue_panel_redraw(panel);
  return error_code;
}

/*.......................................................................
 * Configure or return information about a given field.
 *
 * Input:
 *  panel        TkPanel *  The panel widget.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  widget          char *  The name of the widget.
 *  cell            char *  The textual specification of the target
 *                          cell.
 *  argc             int    The number of arguments.
 *  argv            char ** The array of 'argc' arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_cell_command(TkPanel *panel, Tcl_Interp *interp,
			      char *widget, char *cell,
			      int argc, char *argv[])
{
  int col,row;       /* The column and row of the field to target */
  PanelField *field; /* The specified field */
/*
 * Did the user tell us which cell and what to command to apply to
 * it?
 */
  if(argc < 1) {
    Tcl_AppendResult(interp, "Missing arguments to \"", widget,
		     " ", cell, "\" command.", NULL);
    return TCL_ERROR;
  };
/*
 * Get the field number.
 */
  if(sscanf(cell, "%d,%d", &col, &row) != 2 ||
     col < 0 || col >= PANEL_MAX_COL ||
     row < 0 || row >= PANEL_MAX_ROW) {
    Tcl_AppendResult(interp, "Bad panel cell coordinate, should be $col,$row.",
		     NULL);
    return TCL_ERROR;
  };
/*
 * Get the field.
 */
  field = get_PanelField(panel, col, row);
  if(!field) {
    Tcl_AppendResult(interp, "Unable to allocate field ", cell, ".", NULL);
    return TCL_ERROR;
  };
/*
 * Process one or more paired configuration arguments.
 */
  while(argc > 0) {
    char *cmd = *argv++;
    argc--;
    if(strcmp(cmd, "-flag") == 0) {
      if(field_flag_cmd(panel, field, argc--, argv++) == TCL_ERROR)
	return TCL_ERROR;
    } else if(strcmp(cmd, "-bg") == 0) {
      if(field_bg_cmd(panel, field, argc--, argv++) == TCL_ERROR)
	return TCL_ERROR;
    } else if(strcmp(cmd, "-fg") == 0) {
      if(field_fg_cmd(panel, field, argc--, argv++) == TCL_ERROR)
	return TCL_ERROR;
    } else if(strcmp(cmd, "-pack") == 0) {
      if(field_pack_cmd(panel, field, argc--, argv++) == TCL_ERROR)
	return TCL_ERROR;
    } else {
      Tcl_AppendResult(interp, "Unknown command \"", widget, " cell ", cmd,
		       " ...\"", NULL);
      return TCL_ERROR;
    };
  };
  return TCL_OK;
}

/*.......................................................................
 * Finish intepretting a cell flag-control command.
 *
 * Input:
 *  panel    TkPanel *   The widget resource object.
 *  field PanelField *   The field to be flagged or unflagged.
 *  argc         int     The number of flag-command arguments in argv[].
 *  argv        char **  The array of 'argc' arguments.
 * Output:
 *  return       int     TCL_OK    - Success. If argc was 0, then
 *                                   the current boolean state of the
 *                                   warning flag will be left in
 *                                   panel->interp->result.
 *                       TCL_ERROR - Failure (an error message will be
 *                                   left in panel->interp->result).
 */
static int field_flag_cmd(TkPanel *panel, PanelField *field,
			  int argc, char *argv[])
{
  int doflag=0;     /* True to flag the field */
  unsigned flags;   /* The new redraw flags */
/*
 * If no argument was provided, return the current state of the
 * warning flag.
 */
  if(argc == 0) {
    Tcl_AppendResult(panel->interp, (field->flags & FIELD_WARN_ACTIVE) ?
		     "on":"off", NULL);
    return TCL_OK;
  };
/*
 * There should be exactly one boolean argument specifying the
 * desired state of the flag.
 */
  if(argc < 1) {
    Tcl_AppendResult(panel->interp, "Missing boolean flag state.", NULL);
    return TCL_ERROR;
  };
  if(Tcl_GetBoolean(panel->interp, argv[0], &doflag) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Modify the drawing-attribute flags to reflect the new state of the
 * warning flag.
 */
  if(doflag)
    flags = field->flags | FIELD_WARN_ACTIVE;
  else
    flags = field->flags & ~FIELD_WARN_ACTIVE;
/*
 * If the drawing-attribute flags changed, queue a redraw.
 */
  if(flags != field->flags) {
    field->flags = flags;
    queue_field_redraw(panel, field, FIELD_REDRAW_WARN);
  };
  return TCL_OK;
}

/*.......................................................................
 * Finish intepretting a cell background-color configuration command.
 *
 * Input:
 *  panel    TkPanel *   The widget resource object.
 *  field PanelField *   The field who's background is to be modified.
 *  argc         int     The number of command arguments in argv[].
 *  argv        char **  The array of 'argc' arguments.
 * Output:
 *  return       int     TCL_OK    - Success. If argc was 0, then
 *                                   the name of the current background
 *                                   color will be left in
 *                                   panel->interp->result.
 *                       TCL_ERROR - Failure (an error message will be
 *                                   left in panel->interp->result).
 */
static int field_bg_cmd(TkPanel *panel, PanelField *field,
			int argc, char *argv[])
{
  XColor *bg;   /* The requested background color */
/*
 * If no color argument was provided, return the
 * specification of the current background color.
 */
  if(argc < 1) {
    Tcl_AppendResult(panel->interp, Tk_NameOfColor(field->bg), NULL);
    return TCL_OK;
  };
/*
 * Get the specified color.
 */
  bg = Tk_GetColor(panel->interp, panel->tkwin, Tk_GetUid(argv[0]));
  if(!bg)
    return TCL_ERROR;
/*
 * If the new color differs from the previous one, queue a redraw.
 */
  if(bg != field->bg)
    queue_field_redraw(panel, field, FIELD_REDRAW_ALL);
/*
 * Release the previous background color.
 */
  if(field->bg)
    Tk_FreeColor(field->bg);
/*
 * Record the new one.
 */
  field->bg = bg;
/*
 * Update the fill-color and field-ok graphical contexts of the field.
 */
  update_field_fill_gc(panel, field);
  update_field_ok_gc(panel, field);
  return TCL_OK;
}

/*.......................................................................
 * Finish intepretting a cell foreground-color configuration command.
 *
 * Input:
 *  panel    TkPanel *   The widget resource object.
 *  field PanelField *   The field who's foreground is to be modified.
 *  argc         int     The number of command arguments in argv[].
 *  argv        char **  The array of 'argc' arguments.
 * Output:
 *  return       int     TCL_OK    - Success. If argc was 0, then
 *                                   the name of the current foreground
 *                                   color will be left in
 *                                   panel->interp->result.
 *                       TCL_ERROR - Failure (an error message will be
 *                                   left in panel->interp->result).
 */
static int field_fg_cmd(TkPanel *panel, PanelField *field,
			int argc, char *argv[])
{
  XColor *fg;   /* The requested foreground color */
/*
 * If no color argument was provided, return the
 * specification of the current foreground color.
 */
  if(argc < 1) {
    Tcl_AppendResult(panel->interp, Tk_NameOfColor(field->fg), NULL);
    return TCL_OK;
  };
/*
 * Get the specified color.
 */
  fg = Tk_GetColor(panel->interp, panel->tkwin, Tk_GetUid(argv[0]));
  if(!fg)
    return TCL_ERROR;
/*
 * If the new color differs from the previous one, queue a redraw.
 */
  if(fg != field->fg)
    queue_field_redraw(panel, field, FIELD_REDRAW_TEXT);
/*
 * Release the previous foreground color.
 */
  if(field->fg)
    Tk_FreeColor(field->fg);
/*
 * Record the new one.
 */
  field->fg = fg;
/*
 * Update the text-rendering graphical context of the field.
 */
  update_field_text_gc(panel, field);
  return TCL_OK;
}

/*.......................................................................
 * Finish intepretting a cell text-packing configuration command.
 *
 * Input:
 *  panel    TkPanel *   The widget resource object.
 *  field PanelField *   The field to be modified.
 *  argc         int     The number of command arguments in argv[].
 *  argv        char **  The array of 'argc' arguments.
 * Output:
 *  return       int     TCL_OK    - Success. If argc was 0, then
 *                                   the name of the current packing
 *                                   attribute will be left in
 *                                   panel->interp->result.
 *                       TCL_ERROR - Failure (an error message will be
 *                                   left in panel->interp->result).
 */
static int field_pack_cmd(TkPanel *panel, PanelField *field,
			  int argc, char *argv[])
{
  FieldPack pack;   /* The translated packing attribute */
  int i;
/*
 * List the associations between FieldPack enumerators and their
 * textual equivalents.
 */
  static const struct {
    char *name;      /* The textual version */
    FieldPack pack;  /* The enumerated version */
  } spec[] = {
    {"left",   FIELD_PACK_LEFT},
    {"center", FIELD_PACK_CENTER},
    {"right",  FIELD_PACK_RIGHT},
  };
  static const int nspec = sizeof(spec)/sizeof(spec[0]);
/*
 * If no color argument was provided, return the
 * specification of the current foreground color.
 */
  if(argc < 1) {
    for(i=0; i<nspec; i++) {
      if(field->pack == spec[i].pack) {
	Tcl_AppendResult(panel->interp, spec[i].name, NULL);
	return TCL_OK;
      };
    };
    Tcl_AppendResult(panel->interp, "field_pack_cmd: Unknown pack attribute.",
		     NULL);
    return TCL_ERROR;
  };
/*
 * Translate the specified packing attribute name to a FieldPack enumerator.
 */
  pack = FIELD_PACK_LEFT;
  for(i=0; i<nspec; i++) {
    if(strcmp(spec[i].name, argv[0]) == 0) {
      pack = spec[i].pack;
      break;
    }
  };
/*
 * Not found?
 */
  if(i >= nspec) {
    Tcl_AppendResult(panel->interp, "Unknown field pack attibute: ", argv[0],
		     NULL);
    return TCL_ERROR;
  };
/*
 * If the new attribute differs from the previous one, queue a redraw.
 */
  if(pack != field->pack)
    queue_field_redraw(panel, field, FIELD_REDRAW_TEXT);
/*
 * Record the new attribute.
 */
  field->pack = pack;
  return TCL_OK;
}

/*.......................................................................
 * Return the column and row indexes of the grid cell that encloses a
 * given panel-window x,y coordinate (col row).
 *
 * Input:
 *  panel        TkPanel *  The panel widget.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  widget          char *  The name of the widget.
 *  argc             int    The number of arguments.
 *  argv            char ** The array of 'argc' arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_location_command(TkPanel *panel, Tcl_Interp *interp,
				  char *widget, int argc, char *argv[])
{
  char buffer[20]; /* A temporary buffer for formating the return string */
  int col,row;     /* The index of the located column */
  int x,y;         /* The panel X-window coordinate to locate */
/*
 * We need an x-coordinate argument.
 */
  if(argc != 2) {
    Tcl_AppendResult(interp,"Wrong number of arguments - Should be \"", widget,
		     "location $x $y\"", NULL);
    return TCL_ERROR;
  };
/*
 * Get the x,y coordinate.
 */
  if(Tcl_GetInt(panel->interp, argv[0], &x) == TCL_ERROR ||
     Tcl_GetInt(panel->interp, argv[1], &y) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Locate the column which encloses the specified X value.
 * Note that if the column is to the left of the grid we return -1,
 * whereas if it is to the right of the last existing column, we
 * return the panel->ncol.
 */
  if(x < panel->borderWidth) {
    col = -1;
  } else {
    for(col=0; col<panel->ncol; col++) {
      PanelColumn *column = panel->columns + col;
      if(x >= column->x && x < column->x + column->width)
	break;
    };
  };
/*
 * Locate the row which encloses the specified Y value.
 * Note that if the row is above the first row of the grid we
 * return -1, whereas if it is below the last existing row, we
 * return the panel->nrow.
 */
  if(y < panel->borderWidth) 
    row = -1;
  else if(y - panel->borderWidth >= panel->nrow * panel->cell_height)
    row = panel->nrow;
  else
    row = (y - panel->borderWidth) / panel->cell_height;
/*
 * Return the located row and column indexes.
 */
  sprintf(buffer, "%d %d", col, row);
  Tcl_AppendResult(interp, buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * This function is called upon via Tcl_DoWhenIdle() to redraw the entire
 * widget.
 *
 * Input:
 *  context   ClientData   The widget TkPanel pointer cast to ClientData.
 */
static void panel_redraw_callback(ClientData context)
{
  TkPanel *panel = (TkPanel *) context;
  panel->redraw_queued = 0;
  (void) redraw_panel(panel);
}

/*.......................................................................
 * Queue a panel redraw to be done when Tk is next idle.
 *
 * Input:
 *  panel    TkPanel *   The widget resource object.
 */
static void queue_panel_redraw(TkPanel *panel)
{
/*
 * If a redraw has already been queued, or the widget is not mapped
 * ignore this call. In the latter case the widget will get drawn
 * on the expose event that attends the widget being mapped.
 */
  if(Tk_IsMapped(panel->tkwin) && !panel->redraw_queued) {
    Tcl_DoWhenIdle(panel_redraw_callback, (ClientData) panel);
    panel->redraw_queued = 1;
  };
}

/*.......................................................................
 * Draw a panel widget and its contents.
 *
 * Input:
 *  panel    TkPanel *   The widget resource object.
 * Output:
 *  return       int     0 - OK.
 *                       1 - Error.
 */
static int redraw_panel(TkPanel *panel)
{
  int col, row;      /* Column and row indexes within the field grid */
  PanelField **cell; /* The grid cell at col,row */
/*
 * Draw the 3D border and the background of the window.
 */
  Tk_Fill3DRectangle(panel->tkwin, Tk_WindowId(panel->tkwin),
		     panel->border, 0, 0, Tk_Width(panel->tkwin),
		     Tk_Height(panel->tkwin), panel->borderWidth,
		     panel->relief);
/*
 * Queue all of the fields to be redrawn.
 */
  cell = panel->grid;
  for(row=0; row < panel->nrow; row++) {
    for(col=0; col < panel->ncol; col++,cell++) {
      if(*cell)
	queue_field_redraw(panel, *cell, FIELD_REDRAW_ALL);
    };
  };
  return 0;
}

/*.......................................................................
 * This function is called whenever the TCL array variable named in
 * panel->tcl_array is written to or deleted.
 *
 * Input:
 *  context   ClientData    The (TkPanel *) resource object of the panel
 *                          cast to ClientData.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  name1           char *  The name of the array (or an alias).
 *  name2           char *  The array index being set.
 *  flags            int    The operations being reported as a union of
 *                          the following values:
 *                            TCL_TRACE_WRITES  -  An element is being modified.
 *                            TCL_TRACE_UNSETS  -  An element or the array
 *                                                 is being deleted.
 */
#ifdef USE_CHAR_DECL
static char *panel_array_callback(ClientData context, Tcl_Interp *interp,
				  char *name1, char *name2, 
				  int flags)
#else
static char *panel_array_callback(ClientData context, Tcl_Interp *interp,
				  const char *name1, const char *name2, 
				  int flags)
#endif
{
  int col, row;      /* The column and row indexes taken from the array index */
/*
 * Get the widget resource container.
 */
  TkPanel *panel = (TkPanel *) context;
/*
 * If the whole array has been deleted, the text pointers in each of
 * the fields now point at free'd memory, so set them to NULL and
 * redraw the now emptied fields. Also free the recorded array name.
 */
  if((flags & TCL_TRACE_UNSETS) && !name2) {
    clr_PanelFields(panel);
    if(panel->tcl_array) {
      free(panel->tcl_array);
      panel->tcl_array = NULL;
    };
    if(panel->tcl_array_copy) {
      free(panel->tcl_array_copy);
      panel->tcl_array_copy = NULL;
    };
    return NULL;
  };
/*
 * Decode the row and column indexes from the name of the array element.
 * If the name doesn't have the right format, ignore the call.
 */
  if(sscanf(name2, "%d,%d", &col, &row) != 2 || col < 0 || row < 0)
    return NULL;
/*
 * If an element of the array has just been written, make sure that the
 * corresponding field exists, record the pointer to the new string,
 * and queue the field to be redrawn
 */
  if(flags & TCL_TRACE_WRITES || flags & TCL_TRACE_UNSETS) {
/*
 * Get the field.
 */
    PanelField *field = get_PanelField(panel, col, row);
    if(!field)
      return "Unable to allocate new panel field";
/*
 * Record a pointer to the text of the variable.
 */
    field->text = (char* )Tcl_GetVar2(interp, panel->tcl_array, (char* )name2, 
				      TCL_GLOBAL_ONLY);
/*
 * Queue the field to be redrawn.
 */
    queue_field_redraw(panel, field, FIELD_REDRAW_TEXT);
  };
  return 0;
}

/*.......................................................................
 * Queue a field to be updated when the interpretter next becomes idle.
 *
 * Input:
 *  panel      TkPanel *  The widget resource container.
 *  field   PanelField *  The field to be queued for a redraw. 
 *  what      unsigned    The union of one or more of:
 *                          FIELD_REDRAW_TEXT - redraw the text.
 *                          FIELD_REDRAW_WARN - redraw the warning border.
 */
static void queue_field_redraw(TkPanel *panel, PanelField *field,
			       unsigned what)
{
/*
 * See if a redraw has already been queued.
 */
  unsigned old_redraw_flags = field->flags & FIELD_REDRAW_ALL;
/*
 * If the specified attributes of the field have already been queued
 * to be redrawn or the panel isn't visible, there is nothing more
 * to be done.
 */
  if(old_redraw_flags == what || !Tk_IsMapped(panel->tkwin) ||
     panel->redraw_queued)
    return;
/*
 * Add the specified attributes to the union of those to be redrawn
 * by the redraw callback.
 */
  field->flags |= what;
/*
 * If not already queued, queue the field to be redrawn.
 */
  if(old_redraw_flags == 0) {
/*
 * If this is the first field to be queued for a redraw since the last
 * redraw, register an idle callback function.
 */
    if(panel->redraw_fields == NULL)
      Tcl_DoWhenIdle(field_redraw_callback, (ClientData) panel);
/*
 * Add the field to the list of fields to be updated by the above callback.
 */
    field->next = panel->redraw_fields;
    panel->redraw_fields = field;
  };
}

/*.......................................................................
 * This function is called upon via Tcl_DoWhenIdle() to redraw all fields
 * that have been modified since last being drawn.
 *
 * Input:
 *  context   ClientData   The widget TkPanel pointer cast to ClientData.
 */
static void field_redraw_callback(ClientData context)
{
  (void) redraw_fields((TkPanel *) context);
}

/*.......................................................................
 * Compute the desired dimensions of the widget.
 *
 * Input:
 *  panel   TkPanel *  The panel widget.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
static int panel_update_geometry(TkPanel *panel)
{
  Tk_FontMetrics fm;      /* The height of the current font */
  int h,w;                /* The required height and width of the window */
  int i;
/*
 * Get the vertical dimensions of the current font.
 */
  Tk_GetFontMetrics(panel->font, &fm);
/*
 * Compute the height to give each grid cell.
 */
  panel->cell_height = fm.linespace +
    (panel->warn_width + panel->pady) * 2;
/*
 * Estimate the average width of a character in the current font.
 */
  panel->char_width = Tk_TextWidth(panel->font, "0", 1);
/*
 * Recompute the widths and leftmost x-axis coordinates of each column.
 */
  for(i=0; i<panel->ncol; i++) {
    PanelColumn *column = panel->columns + i;
    column->width = panel->char_width * column->nchar +
      2 * (panel->warn_width + panel->padx);
    column->x = i==0 ? panel->borderWidth : column[-1].x + column[-1].width;
  };
/*
 * If the user has requested a specific size, honor it.
 */
  if(panel->req_width!=0 || panel->req_height!=0) {
    w = panel->req_width;
    h = panel->req_height;
  } else {
/*
 * The height needed to just encapsulate the existing rows is easy to
 * calculate because all of the rows are defined to have the same height.
 */
    h = 2 * panel->borderWidth + panel->nrow * panel->cell_height;
/*
 * The width is the sum of the widths of each column, plus the width
 * of the enclosing 3D border.
 */
    w = 2 * panel->borderWidth;
    for(i=0; i<panel->ncol; i++)
      w += panel->columns[i].width;
  };
/*
 * Make sure that the window contains at least one pixel.
 */
  if(w < 1)
    w = 1;
  if(h < 1)
    h = 1;
/*
 * Tell the geometry manager about the optimal size of the window.
 */
  Tk_GeometryRequest(panel->tkwin, w, h);
/*
 * Make sure that the widget gets redrawn.
 */
  queue_panel_redraw(panel);
  return 0;
}

/*.......................................................................
 * This is the main X event callback for panel widgets.
 * 
 * Input:
 *  context   ClientData    The panel widget cast to (ClientData).
 *  event         XEvent *  The event that triggered the callback.
 */
static void panel_EventHandler(ClientData context, XEvent *event)
{
  TkPanel *panel = (TkPanel *) context;
/*
 * Determine what type of event triggered this call.
 */
  switch(event->type) {
  case ConfigureNotify:   /* The window has been resized so redraw it */
    queue_panel_redraw(panel);
    break;
  case DestroyNotify:     /* The window has been destroyed */
/*
 * Delete the main event handler to prevent prolonged use.
 */
    if(panel->event_mask != NoEventMask) {
      Tk_DeleteEventHandler(panel->tkwin, panel->event_mask,
			    panel_EventHandler, (ClientData) panel);
      panel->event_mask = NoEventMask;
    };
/*
 * Queue deletion of the panel until all calls to Tcl_Preserve() have
 * been matched by calls to Tcl_Release().
 */
    Tcl_EventuallyFree(context, panel_FreeProc);
    break;
  case Expose:            /* Redraw the newly exposed region of the widget */
    panel_expose_handler(panel, event);
    break;
  };
  return;
}

/*.......................................................................
 * The expose-event handler for panel widgets.
 *
 * Input:
 *  panel   TkPanel * The Tk panel widget.
 *  event    XEvent   The expose event that invoked the callback.
 */
static void panel_expose_handler(TkPanel *panel, XEvent *event)
{
  XExposeEvent *expose = &event->xexpose;
  int col, row;      /* The grid column/row location of a field */
  int mincol, maxcol;/* The first and last columns that need to be redrawn */
  int minrow, maxrow;/* The first and last rows that need to be redrawn */
/*
 * Find the first column that needs to be redrawn.
 */
  for(col=0; col < panel->ncol; col++) {
    PanelColumn *column = panel->columns + col;
    if(column->x + column->width > expose->x)
      break;
  };
  mincol = col;
/*
 * Find the last column that needs to be redrawn.
 */
  for(col=mincol; col < panel->ncol; col++) {
    PanelColumn *column = panel->columns + col;
    if(column->x >= expose->x + expose->width)
      break;
  };
  maxcol = col < panel->ncol ? col : (panel->ncol - 1);
/*
 * Determine the range of rows that need to be redrawn.
 */
  minrow = (expose->y - panel->borderWidth) / panel->cell_height;
  if(minrow < 0)
    minrow = 0;
  maxrow = (expose->y - panel->borderWidth + expose->height - 1) /
    panel->cell_height;
  if(maxrow >= panel->nrow)
    maxrow = panel->nrow - 1;
/*
 * Queue redraws for the exposed fields.
 */
  for(row=minrow; row <= maxrow && row < panel->nrow; row++) {
    PanelField **cell = panel->grid + row * panel->ncol + mincol;
    for(col=mincol; col <= maxcol; col++) {
      PanelField *field = *cell++;
      if(field)
	queue_field_redraw(panel, field, FIELD_REDRAW_ALL);
    };
  };
/*
 * Does the 3D border need to be redrawn?
 */
  if(expose->x < panel->borderWidth ||
     expose->x + expose->width > Tk_Width(panel->tkwin) - panel->borderWidth ||
     expose->y < panel->borderWidth ||
     expose->y + expose->height > Tk_Height(panel->tkwin)-panel->borderWidth) {
    Tk_Draw3DRectangle(panel->tkwin, Tk_WindowId(panel->tkwin),
		       panel->border, 0, 0, Tk_Width(panel->tkwin),
		       Tk_Height(panel->tkwin), panel->borderWidth,
		       panel->relief);
  };
  return;
}

/*.......................................................................
 * This is a Tk_FreeProc() wrapper function around del_TkPanel(),
 * suitable for use with Tcl_EventuallyFree().
 *
 * Input:
 *  context   ClientData  The panel widget to be deleted, cast to
 *                        ClientData.
 */
static void panel_FreeProc(char *context)
{
  (void) del_TkPanel((TkPanel *) context);
}

/*.......................................................................
 * Whenever the foreground color or font of a field, is changed, this
 * function must be called to update the graphical context of that field.
 *
 * Input:
 *  panel     TkPanel *  The widget resource object.
 *  field  PanelField *  The field to be updated.
 */
static void update_field_text_gc(TkPanel *panel, PanelField *field)
{
  unsigned long mask=0; /* The bit-wise union of attributes given in 'values' */
  XGCValues values;     /* The container of the graphical attributes to be */
                        /*  associated with the new GC */
  GC text_gc;           /* The new graphical context */
/*
 * Specify the current foreground color of the field.
 */
  values.foreground = field->fg->pixel;
  mask |= GCForeground;
  values.font = Tk_FontId(panel->font);
  mask |= GCFont;
  values.graphics_exposures = False;
  mask |= GCGraphicsExposures;
/*
 * Get the new graphical context, before freeing the old one.
 * If the GC hasn't actually changed, and this field currently holds
 * the only reference to this GC, then allocating the new reference
 * before freeing the old reference prevents the GC from being
 * unnecessarily free()d and re-allocated.
 */
  text_gc = Tk_GetGC(panel->tkwin, mask, &values);
/*
 * Now discard the previous GC and install the new one.
 */
  if(field->text_gc)
    Tk_FreeGC(panel->display, field->text_gc);
  field->text_gc = text_gc;
  return;
}

/*.......................................................................
 * Whenever the background color of a field is changed, this function
 * must be called to update the associated graphical context of that field.
 *
 * Input:
 *  panel     TkPanel *  The widget resource object.
 *  field  PanelField *  The field to be updated.
 */
static void update_field_fill_gc(TkPanel *panel, PanelField *field)
{
  unsigned long mask=0; /* The bit-wise union of attributes given in 'values' */
  XGCValues values;     /* The container of the graphical attributes to be */
                        /*  associated with the new GC */
  GC fill_gc;           /* The new graphical context */
/*
 * Specify the current background fill-color of the field.
 */
  values.foreground = field->bg->pixel;
  mask |= GCForeground;
  values.graphics_exposures = False;
  mask |= GCGraphicsExposures;
/*
 * Get the new graphical context, before freeing the old one.
 * If the GC hasn't actually changed, and this field currently holds
 * the only reference to this GC, then allocating the new reference
 * before freeing the old reference prevents the GC from being
 * unnecessarily free()d and re-allocated.
 */
  fill_gc = Tk_GetGC(panel->tkwin, mask, &values);
/*
 * Now discard the previous GC and install the new one.
 */
  if(field->fill_gc)
    Tk_FreeGC(panel->display, field->fill_gc);
  field->fill_gc = fill_gc;
  return;
}

/*.......................................................................
 * Whenever the background color of a field is changed, or the width of
 * the warning border is changed, this function must be called to update
 * the associated graphical context of that field.
 *
 * Input:
 *  panel     TkPanel *  The widget resource object.
 *  field  PanelField *  The field to be updated.
 */
static void update_field_ok_gc(TkPanel *panel, PanelField *field)
{
  unsigned long mask=0; /* The bit-wise union of attributes given in 'values' */
  XGCValues values;     /* The container of the graphical attributes to be */
                        /*  associated with the new GC */
  GC ok_gc;             /* The new graphical context */
/*
 * Specify the color and width of the inactive warning-border.
 */
  values.foreground = field->bg->pixel;
  mask |= GCForeground;
  values.line_width = panel->warn_width;
  mask |= GCLineWidth;
  values.graphics_exposures = False;
  mask |= GCGraphicsExposures;
/*
 * Get the new graphical context, before freeing the old one.
 * If the GC hasn't actually changed, and this field currently holds
 * the only reference to this GC, then allocating the new reference
 * before freeing the old reference prevents the GC from being
 * unnecessarily free()d and re-allocated.
 */
  ok_gc = Tk_GetGC(panel->tkwin, mask, &values);
/*
 * Now discard the previous GC and install the new one.
 */
  if(field->ok_gc)
    Tk_FreeGC(panel->display, field->ok_gc);
  field->ok_gc = ok_gc;
  return;
}

/*.......................................................................
 * Get the field that resides in a given cell of the grid. If the field
 * doesn't exist yet, create it.
 *
 * Input:
 *  panel         TkPanel *  The panel to update.
 *  col, row          int    The column,row at which to create the field.
 *                           This location must lie within the current
 *                           bounds of the grid.
 * Output:
 *  return     PanelField *  The new field, or NULL on error.
 */
static PanelField *get_PanelField(TkPanel *panel, int col, int row)
{
  PanelField *field;      /* The new field */
  PanelField **grid_cell; /* The cell in which to install the field */
/*
 * Check that the requested location makes sense.
 */
  if(col < 0 || col > PANEL_MAX_COL ||
     row < 0 || row > PANEL_MAX_ROW) {
    lprintf(stderr, "get_PanelField: Column/row indexes out of range.\n");
    return NULL;
  };
/*
 * If the grid isn't big enough to contain the cell, expand it.
 */
  if(col >= panel->ncol || row >= panel->nrow) {
    int need_ncol = col < panel->ncol ? panel->ncol : col+1;
    int need_nrow = row < panel->nrow ? panel->nrow : row+1;
    if(resize_panel_grid(panel, need_ncol, need_nrow)) {
      lprintf(stderr, "Unable to expand panel grid.\n");
      return NULL;
    };
  };
/*
 * Get a pointer to the destination cell in the grid.
 */
  grid_cell = panel->grid + row * panel->ncol + col;
/*
 * If the cell is occupied, return its occupant.
 */
  if(*grid_cell)
    return *grid_cell;
/*
 * Allocate the new field.
 */
  field = (PanelField* )new_FreeListNode("get_PanelField", panel->field_mem);
  if(!field)
    return NULL;
/*
 * Initialize the container up to the point at which it can safely be
 * passed to rem_PanelField().
 */
  field->next = NULL;
  field->text = NULL;
  field->col = col;
  field->row = row;
  field->bg = NULL;
  field->fg = NULL;
  field->flags = 0;
  field->fill_gc = NULL;
  field->ok_gc = NULL;
  field->text_gc = NULL;
  field->pack = FIELD_PACK_CENTER;
/*
 * Assign the default panel background and foreground colors to the field.
 */
  field->bg = Tk_GetColor(panel->interp, panel->tkwin,
			  Tk_GetUid(Tk_NameOf3DBorder(panel->border)));
  field->fg = Tk_GetColor(panel->interp, panel->tkwin,
			  Tk_GetUid(Tk_NameOfColor(panel->normalFg)));
/*
 * Get the associated graphical contexts.
 */
  update_field_fill_gc(panel, field);
  update_field_ok_gc(panel, field);
  update_field_text_gc(panel, field);
/*
 * Queue the empty field to be drawn.
 */
  queue_field_redraw(panel, field, FIELD_REDRAW_ALL);
/*
 * Assign the new field to the grid.
 */
  *grid_cell = field;
  return field;
}

/*.......................................................................
 * Remove a field from the grid and delete its contents.
 *
 * Input:
 *  panel         TkPanel *  The resource object of the widget.
 *  col, row          int    The column,row at which to create the field.
 *                           This location must lie within the current
 *                           bounds of the grid.
 * Output:
 *  return     PanelField *  The deleted field (ie. always NULL).
 */
static PanelField *rem_PanelField(TkPanel *panel, int col, int row)
{
  PanelField **grid_cell; /* The cell that contains the redundant field */
  PanelField *field;      /* The field to be deleted */
/*
 * If the requested location doesn't exist then there is clearly nothing
 * to delete.
 */
  if(col < 0 || col >= panel->ncol || row < 0 || row >= panel->nrow)
    return NULL;
/*
 * Get a pointer to the cell that contains the redundant field.
 */
  grid_cell = panel->grid + row * panel->ncol + col;
/*
 * Get the field that is contained in the above cell.
 */
  field = *grid_cell;
/*
 * Already removed?
 */
  if(!field)
    return NULL;
/*
 * Remove the field from the grid.
 */
  *grid_cell = NULL;
/*
 * Delete the text value who's value is displayed by the field.
 */
  if(field->text) {
    char element[12];
    sprintf(element, "%d,%d", field->col, field->row);
    untrace_panel_array(panel);
    Tcl_UnsetVar2(panel->interp, panel->tcl_array, element, TCL_GLOBAL_ONLY);
    trace_panel_array(panel);
    field->text = NULL;
  };
/*
 * Erase the displayed version of the field.
 */
  if(Tk_IsMapped(panel->tkwin)) {
    XClearArea(panel->display, Tk_WindowId(panel->tkwin),
	       panel->columns[col].x,
	       panel->borderWidth + panel->cell_height * row,
	       panel->columns[col].width, panel->cell_height, False);
  };
/*
 * If the field is currently queued to be redrawn, search it out and
 * remove it from the redraw queue.
 */
  if(field->flags & FIELD_REDRAW_ALL) {
    PanelField *prev = NULL;
    PanelField *node;
    for(node=panel->redraw_fields; node && node != field; node=node->next)
      prev = node;
/*
 * If the field was found relink the redraw list around it.
 */
    if(node == field) {
      if(prev)
	prev->next = field->next;
      else
	panel->redraw_fields = field->next;
    };
  };
/*
 * Delete the field and its contents.
 */
  return del_PanelField(panel, field);
}

/*.......................................................................
 * Delete a field and its contents.
 *
 * Input:
 *  panel         TkPanel *  The resource object of the widget.
 *  field      PanelField *  The field to delete.
 * Output:
 *  return     PanelField *  The deleted field (ie. always NULL).
 */
static PanelField *del_PanelField(TkPanel *panel, PanelField *field)
{
  if(field) {
/*
 * Release the graphical contexts.
 */
    if(field->fill_gc)
      Tk_FreeGC(panel->display, field->fill_gc);
    if(field->ok_gc)
      Tk_FreeGC(panel->display, field->ok_gc);
    if(field->text_gc)
      Tk_FreeGC(panel->display, field->text_gc);
/*
 * Release the background and foreground colors of the field.
 */
    if(field->fg)
      Tk_FreeColor(field->fg);
    if(field->bg)
      Tk_FreeColor(field->bg);
/*
 * Return the field to the free-list.
 */
    field = (PanelField* )del_FreeListNode("del_PanelField", panel->field_mem, 
					   field);
  };
  return NULL;
}

/*.......................................................................
 * Redraw fields that have been marked as out of date.
 *
 * Input:
 *  panel    TkPanel *   The parent widget of the fields.
 * Output:
 *  return       int     0 - OK.
 *                       1 - Error.
 */
static int redraw_fields(TkPanel *panel)
{
/*
 * Fields that have changed since last being drawn, are listed in
 * panel->redraw_fields. As each field is drawn, remove it from
 * the list.
 */
  while(panel->redraw_fields) {
/*
 * Get the next field from the head of the list of undrawn fields.
 */
    PanelField *field = panel->redraw_fields;
/*
 * Draw the field.
 */
    if(update_field(panel, field))
      return 1;
/*
 * Remove the field from the list and move the next undrawn field to
 * the head of the list.
 */
    panel->redraw_fields = field->next;
    field->next = NULL;
  };
  return 0;
}

/*.......................................................................
 * Draw one or more components of a text field, as specified in
 * field->flags.
 *
 * Input:
 *  panel     TkPanel *  The parent panel widget.
 *  field  PanelField *  The field to be updated.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static int update_field(TkPanel *panel, PanelField *field)
{
/*
 * Compute the coordinate of the top-left corner of the field.
 */
  int x = panel->columns[field->col].x;
  int y = panel->borderWidth + panel->cell_height * field->row;
/*
 * Get the width and height of the field.
 */
  int w = panel->columns[field->col].width;
  int h = panel->cell_height;
/*
 * Get the width of the warning border.
 */
  int bd = panel->warn_width;
/*
 * Draw the warning border?
 */
  if(field->flags & FIELD_REDRAW_WARN) {
/*
 * Get the cached graphical context that matches the desired
 * state of the warning border.
 */
    GC warn_gc = field->flags & FIELD_WARN_ACTIVE ?
       panel->warn_color_gc : field->ok_gc;
/*
 * Draw the outline rectangle that constitutes the warning border.
 */
    XDrawRectangle(panel->display, Tk_WindowId(panel->tkwin),
		   warn_gc, x+bd/2, y+bd/2, w-bd, h-bd);
/*
 * Mark the warning border as up to date.
 */
    field->flags &= ~FIELD_REDRAW_WARN;
  };
/*
 * Draw the 3D border?
 *
 * We do this by drawing a 1-pixel width rectangle in the background
 * color of the field, then drawing white dots over every other pixel
 * of the left and top edges of this rectangle, then black dots over
 * every other pixel of its lower and right edges.
 */
  if(field->flags & FIELD_REDRAW_3D) {
    XPoint line[3];  /* A two-segment line */
/*
 * Compute the locations of the sides of the 3D-border rectangle.
 */
    short left = x + bd;
    short right = x + w - bd - 1;
    short top = y + bd;
    short bot = y + h - bd - 1;
/*
 * Draw 1-pixel a rectangle in the background color of the field,
 * just inside the warning border.
 */
    XDrawRectangle(panel->display, Tk_WindowId(panel->tkwin),
		   field->fill_gc, left, top, w-2*bd-1, h-2*bd-1);
/*
 * Compute the vertices of the line composes the left and top
 * sides of the 3D border rectangle.
 */
    line[0].x = left;
    line[0].y = bot;
    line[1].x = left;
    line[1].y = top;
    line[2].x = right;
    line[2].y = top;
/*
 * Draw the white dots over the above line segments.
 */
    XDrawLines(panel->display, Tk_WindowId(panel->tkwin),
	       panel->white_dots_gc, line, 3, CoordModeOrigin);
/*
 * Compute the vertices of the line composes the lower and right
 * sides of the above rectangle.
 */
    line[0].x = right;
    line[0].y = top;
    line[1].x = right;
    line[1].y = bot;
    line[2].x = left;
    line[2].y = bot;
/*
 * Draw the black dots over the above line segments.
 */
    XDrawLines(panel->display, Tk_WindowId(panel->tkwin),
	       panel->black_dots_gc, line, 3, CoordModeOrigin);
/*
 * Mark the 3D border as being up to date.
 */
    field->flags &= ~FIELD_REDRAW_3D;
  };
/*
 * Draw the text and its background?
 */
  if(field->flags & FIELD_REDRAW_TEXT) {
/*
 * Draw the background.
 */
    XFillRectangle(panel->display, Tk_WindowId(panel->tkwin),
		   field->fill_gc, x + bd+1, y + bd+1, w - 2*bd-2, h - 2*bd-2);
/*
 * Does the field contain any text?
 */
    if(field->text) {
      int xbl;    /* The X-axis coordinate of the left edge of the baseline */
      int ybl;    /* The Y-axis coordinate of the text baseline */
      Tk_FontMetrics fm;  /* The height of the current font */
      XRectangle clip;    /* The rectangle used to clip the drawn text */
/*
 * Measure the number of pixels required to display the field text.
 */
      int xlen = Tk_TextWidth(panel->font, field->text, strlen(field->text));
/*
 * Compute the X-axis starting location of the text.
 */
      switch(field->pack) {
      default:
      case FIELD_PACK_LEFT:
	xbl = x + bd + panel->padx;
	break;
      case FIELD_PACK_CENTER:
	xbl = x + w/2 - xlen/2;
	break;
      case FIELD_PACK_RIGHT:
	xbl = x + w - bd - panel->padx - xlen;
	break;
      };
/*
 * Compute the Y-axis location of the text baseline.
 */
      Tk_GetFontMetrics(panel->font, &fm);
      ybl = y + bd + panel->pady + fm.ascent;
/*
 * Compute the bounds of the region within which to contrain the drawn
 * text.
 */
      clip.x = x + bd + panel->padx;
      clip.y = y + bd + panel->pady;
      clip.width = w - 2 * (bd + panel->padx);
      clip.height = h - 2 * (bd + panel->pady);
/*
 * Establish the clip rectangle.
 */
      XSetClipRectangles(panel->display, field->text_gc, 0, 0, &clip, 1,
			 Unsorted);
/*
 * Draw the text.
 */
      Tk_DrawChars(panel->display, Tk_WindowId(panel->tkwin),
		   field->text_gc, panel->font, field->text,
		   strlen(field->text), xbl, ybl);
/*
 * The GC is potentially shared with other parts of Tk, so revert it
 * to its allocated non-clipping state.
 */
      XSetClipMask(panel->display, field->text_gc, None);
    };
/*
 * Mark the text as being up to date.
 */
    field->flags &= ~FIELD_REDRAW_TEXT;
  };
  return 0;
}

/*.......................................................................
 * Configure or return information about a given column.
 *
 * Input:
 *  panel        TkPanel *  The panel widget.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  widget          char *  The name of the widget.
 *  argc             int    The number of arguments.
 *  argv            char ** The array of 'argc' arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_column_command(TkPanel *panel, Tcl_Interp *interp,
				char *widget, int argc, char *argv[])
{
  char *spec;          /* The column specification */
  int col;             /* The specified column */
  char *endp;          /* The next unprocessed substring of argv[0] */
  PanelColumn *column; /* The column configuration container */
/*
 * We need at least the column number at least one attribute name.
 */
  if(argc < 2) {
    Tcl_AppendResult(interp, "Missing arguments to \"", widget,
		     " column\" command.", NULL);
    return TCL_ERROR;
  };
/*
 * Get the column number.
 */
  spec = *argv++;
  argc--;
  col = strtol(spec, &endp, 10);
  if(endp == spec || *endp != '\0' || col < 0 || col > PANEL_MAX_COL) {
    Tcl_AppendResult(interp, "Invalid column index (", spec, ")", NULL);
    return TCL_ERROR;
  };
/*
 * Get the specified column.
 */
  column = get_panel_column(panel, col);
  if(!column)
    return TCL_ERROR;
/*
 * Process one or more paired configuration arguments.
 */
  while(argc > 0) {
    char *cmd = *argv++;
    argc--;
    if(strcmp(cmd, "-width") == 0) {
      if(column_width_cmd(panel, column, argc--, argv++) == TCL_ERROR)
	return TCL_ERROR;
    } else {
      Tcl_AppendResult(interp, "Unknown command \"", widget, " cell ", cmd,
		       " ...\"", NULL);
      return TCL_ERROR;
    };
  };
  return TCL_OK;
}

/*.......................................................................
 * Delete the contents of a given cell.
 *
 * Input:
 *  panel        TkPanel *  The panel widget.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  widget          char *  The name of the widget.
 *  argc             int    The number of arguments.
 *  argv            char ** The array of 'argc' arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_delete_command(TkPanel *panel, Tcl_Interp *interp,
				char *widget, int argc, char *argv[])
{
  int col, row;    /* The location of the field to be deleted */
/*
 * There should be one argument to specify the field to be deleted.
 */
  if(argc != 1) {
    Tcl_AppendResult(interp, "Usage: \"", widget, " delete $col,$row\"", NULL);
    return TCL_ERROR;
  };
/*
 * Get the field number.
 */
  if(sscanf(argv[0], "%d,%d", &col, &row) != 2) {
    Tcl_AppendResult(interp, "Usage: \"", widget, " delete $col,$row\"", NULL);
    return TCL_ERROR;
  };
/*
 * Delete the specified field.
 */
  (void) rem_PanelField(panel, col, row);
  return TCL_OK;
}

/*.......................................................................
 * Shrink wrap the widget by deleting unoccupied trailing columns and rows.
 *
 * Input:
 *  panel        TkPanel *  The panel widget.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  widget          char *  The name of the widget.
 *  argc             int    The number of arguments.
 *  argv            char ** The array of 'argc' arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_shrink_command(TkPanel *panel, Tcl_Interp *interp,
				char *widget, int argc, char *argv[])
{
  int col, row;       /* Column and row indexes */
  int maxcol, maxrow; /* The maximum occupied column and row indexes */
  PanelField **cell;  /* The cell being processed */
/*
 * There shouldn't be any arguments.
 */
  if(argc > 0) {
    Tcl_AppendResult(interp, "The shrink command doesn't take any arguments.",
		     NULL);
    return TCL_ERROR;
  };
/*
 * Find the indexes of the last occupied column and row.
 */
  maxrow = maxcol = 0;
  cell = panel->grid;
  for(row=0; row < panel->nrow; row++) {
    for(col=0; col < panel->ncol; col++) {
      if(*cell++) {
	if(col > maxcol)
	  maxcol = col;
	if(row > maxrow)
	  maxrow = row;
      };
    };
  };
/*
 * Resize the grid to be just big enough to contain all current fields.
 */
  return resize_panel_grid(panel, maxcol+1, maxrow+1);
}

/*.......................................................................
 * Return the bounding box of a given area of rows and columns.
 *
 * Input:
 *  panel        TkPanel *  The panel widget.
 *  interp    Tcl_Interp *  The TCL intrepreter.
 *  widget          char *  The name of the widget.
 *  argc             int    The number of arguments.
 *  argv            char ** The array of 'argc' arguments.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Failure.
 */
static int panel_bbox_command(TkPanel *panel, Tcl_Interp *interp,
			      char *widget, int argc, char *argv[])
{
  int scol, ecol;     /* The first and last columns of the region */
  int srow, erow;     /* The first and last rows of the region */
  int x,y;            /* The coordinate of the top left corner of cell */
                      /*  srow,scol */
  int w,h;            /* The width and height of the region */
  char buffer[80];    /* The buffer in which to construct the return string */
/*
 * Get column,row ranges of the query.
 */
  switch(argc) {
  case 4:
    if(Tcl_GetInt(panel->interp, argv[0], &scol) == TCL_ERROR ||
       Tcl_GetInt(panel->interp, argv[1], &srow) == TCL_ERROR ||
       Tcl_GetInt(panel->interp, argv[2], &ecol) == TCL_ERROR ||
       Tcl_GetInt(panel->interp, argv[3], &erow) == TCL_ERROR)
      return TCL_ERROR;
    break;
  case 2:
    if(Tcl_GetInt(panel->interp, argv[0], &scol) == TCL_ERROR ||
       Tcl_GetInt(panel->interp, argv[1], &srow) == TCL_ERROR)
      return TCL_ERROR;
    ecol = scol;
    erow = srow;
    break;
  case 0:
    srow = 0;
    erow = panel->nrow - 1;
    scol = 0;
    ecol = panel->ncol - 1;
    break;
  default:
    Tcl_AppendResult(interp,
	     "Wrong argument count, should be \"panel bbox ?col row ?col row\"",
	     NULL);
    return TCL_ERROR;
  };
/*
 * Sort the row and column ranges into ascending order.
 */
  if(srow > erow) {
    int tmp = srow;
    srow = erow;
    erow = tmp;
  };
  if(scol > ecol) {
    int tmp = scol;
    scol = ecol;
    ecol = tmp;
  };
/*
 * Check the validity of the specified ranges.
 */
  if(srow < 0 || erow >= panel->nrow || scol < 0 || ecol >= panel->ncol) {
    Tcl_AppendResult(panel->interp, "Column/row indexes out of range.", NULL);
    return TCL_ERROR;
  };
/*
 * Determine the x,y coordinates of the top left corner of cell srow,scol.
 */
  x = panel->columns[scol].x;
  y = panel->borderWidth + srow * panel->cell_height;
/*
 * Compute the width and height of the region.
 */
  w = panel->columns[ecol].x - x + panel->columns[ecol].width;
  h = (erow - srow + 1) * panel->cell_height;
/*
 * Compose the return string, containing the above parameters.
 */
  sprintf(buffer, "%d %d %d %d", x, y, w, h);
  Tcl_AppendResult(panel->interp, buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Finish intepretting a column-width command.
 *
 * Input:
 *  panel      TkPanel *   The widget resource object.
 *  column PanelColumn *   The target column.
 *  argc           int     The number of flag-command arguments in argv[].
 *  argv          char **  The array of 'argc' arguments.
 * Output:
 *  return         int     TCL_OK    - Success. If argc was 0, then
 *                                     the current width will be left in
 *                                     panel->interp->result.
 *                         TCL_ERROR - Failure (an error message will be
 *                                     left in panel->interp->result).
 */
static int column_width_cmd(TkPanel *panel, PanelColumn *column,
			    int argc, char *argv[])
{
  int width;    /* The new width of the column */
  char *endp;   /* The next unprocessed substring of argv[0] */
  char *spec;   /* The width specification */
/*
 * If no argument was provided, return the current width of the column.
 */
  if(argc == 0) {
    char buff[20];
    sprintf(buff, "%d", column->nchar);
    Tcl_AppendResult(panel->interp, buff, NULL);
    return TCL_OK;
  };
/*
 * Get the width specification.
 */
  spec = argv[0];
/*
 * Have we been asked to fit for the width?
 */
  if(spec[0] == '?') {
/*
 * Determine the maximum number of pixels required by any field in the column.
 */
    int col = column - panel->columns;
    int row;
    int maxpix = 0;
    for(row=0; row<panel->nrow; row++) {
      PanelField *field = panel->grid[row * panel->ncol + col];
      if(field && field->text) {
	int npix = Tk_TextWidth(panel->font, field->text, strlen(field->text));
	if(npix > maxpix)
	  maxpix = npix;
      };
    };
/*
 * Convert maxpix from pixels to characters, rounding up to an
 * integral number of average-width characters.
 */
    width = (maxpix + panel->char_width - 1) / panel->char_width;
/*
 * Read the desired width of the field.
 */
  } else {
    width = strtol(spec, &endp, 10);
    if(endp == spec || *endp != '\0' || width < 0) {
      Tcl_AppendResult(panel->interp, "Invalid column width (", spec, ")",
		       NULL);
      return TCL_ERROR;
    };
  };
/*
 * If the column width changed, recompute the geometry of the widget.
 */
  if(width != column->nchar) {
    column->nchar = width;
    panel_update_geometry(panel);
  };
  return TCL_OK;
}

/*.......................................................................
 * Return the configuration container of a given column. If that column
 * doesn't exist, create both it and any intervening columns.
 *
 * Input:
 *  panel       TkPanel *  The parent panel widget.
 *  col             int    The index of the required column.
 * Output:
 *  return  PanelColumn *  The requested container. On error
 *                         NULL will be returned and an error
 *                         message will be left in panel->interp->result.
 */
static PanelColumn *get_panel_column(TkPanel *panel, int col)
{
/*
 * Check the column number.
 */
  if(col < 0 || col > PANEL_MAX_COL) {
    Tcl_AppendResult(panel->interp, "Illegal column index.", NULL);
    return NULL;
  };
/*
 * If needed, increase the size of the grid and column arrays.
 */
  if(col >= panel->ncol &&
     resize_panel_grid(panel, col+1, panel->nrow) == TCL_ERROR)
    return NULL;
/*
 * Return the requested column.
 */
  return panel->columns + col;
}

/*.......................................................................
 * Update the grid and column arrays to accomodate a given number of
 * columns and rows. This is a no-op if nrow and ncol match the
 * current grid size.
 *
 * Input:
 *  panel     TkPanel *   The panel widget.
 *  ncol,nrow     int     The desired number of columns and rows.
 * Output:
 *  return        int     TCL_OK.
 *                        TCL_ERROR (an error message will be left
 *                                   in panel->interp->result).
 */
static int resize_panel_grid(TkPanel *panel, int ncol, int nrow)
{
  int col,row;    /* Loop variable column and row indexes */
/*
 * No change in size required?
 */
  if(ncol == panel->ncol && nrow == panel->nrow)
    return TCL_OK;
/*
 * Increase the number of columns?
 */
  if(ncol > panel->ncol) {
/*
 * Increase the size of the column array?
 */
    if(ncol > (int)panel->max_ncol) {
      int try_ncol = ncol + COL_BLK_FACTOR;
      PanelColumn *tmp = (PanelColumn* )realloc(panel->columns, 
						try_ncol*sizeof(PanelColumn));
      if(tmp) {
	panel->columns = tmp;
	panel->max_ncol = try_ncol;
      } else {
	Tcl_AppendResult(panel->interp,
			 "Insufficient memory to enlarge column array.", NULL);
	return TCL_ERROR;
      };
    };
/*
 * Initialize all newly created columns.
 */
    for(col=panel->ncol; col<ncol; col++) {
      PanelColumn *column = panel->columns + col;
      column->x = col>0 ? column[-1].x + column[-1].width : panel->borderWidth;
      column->nchar = panel->column_width;
      column->width = 0;   /* To be set by panel_update_geometry() below */
    };
  };
/*
 * If the number of rows has decreased, evict any occupants of the
 * redundant rows.
 */
  if(nrow < panel->nrow) {
    for(row=nrow; row<panel->nrow; row++) {
      for(col=0; col<panel->ncol; col++) {
	if(panel->grid[row * panel->ncol + col])
	  rem_PanelField(panel, col, row);
      };
    };
  };
/*
 * If the number of columns has decreased, evict any occupants of the
 * redundant columns.
 */
  if(ncol < panel->ncol) {
    for(row=0; row<panel->nrow; row++) {
      for(col=ncol; col<panel->ncol; col++) {
	if(panel->grid[row * panel->ncol + col])
	  rem_PanelField(panel, col, row);
      };
    };
  };
/*
 * Allocate a bigger grid array?
 */
  if(ncol * nrow > panel->max_ngrid) {
    int try_ngrid = (ncol + COL_BLK_FACTOR) * (nrow + ROW_BLK_FACTOR);
    PanelField **tmp = (PanelField** )realloc(panel->grid, 
					      try_ngrid * sizeof(PanelField *));
    if(tmp) {
      panel->grid = tmp;
      panel->max_ngrid = try_ngrid;
    } else {
      Tcl_AppendResult(panel->interp,
		       "Insufficient memory to enlarge grid array.", NULL);
      return TCL_ERROR;
    };
  };
/*
 * Move existing cells to their new positions in the re-dimensioned
 * grid array.
 */
  if(ncol < panel->ncol) {
    for(row=1; row < panel->nrow; row++) {
      PanelField **src = panel->grid + row * panel->ncol;
      PanelField **dst = panel->grid + row * ncol;
      for(col=0; col < panel->ncol; col++)
	dst[col] = src[col];
    };
  } else if(ncol > panel->ncol) {
    for(row=panel->nrow-1; row>0; row--) {
      PanelField **src = panel->grid + row * panel->ncol;
      PanelField **dst = panel->grid + row * ncol;
      for(col=panel->ncol-1; col>=0; col--)
	dst[col] = src[col];
    };
  };
/*
 * Initialize newly created cells.
 */
  for(row=0; row < nrow; row++) {
    PanelField **cell = panel->grid + row * ncol + panel->ncol;
    for(col=panel->ncol; col < ncol; col++)
      *cell++ = NULL;
  };
  for(row=panel->nrow; row < nrow; row++) {
    PanelField **cell = panel->grid + row * ncol;
    for(col=0; col<ncol; col++)
      *cell++ = NULL;
  };
/*
 * Record the new grid size.
 */
  panel->ncol = ncol;
  panel->nrow = nrow;
  sprintf(panel->size_spec, "%d %d", ncol, nrow);
/*
 * Update the widget geometry to account for the modified grid.
 */
  panel_update_geometry(panel);
  return TCL_OK;
}

/*.......................................................................
 * Clear the text of all fields in the grid.
 *
 * Input:
 *  panel   TkPanel *  The widget resource container.
 */
static void clr_PanelFields(TkPanel *panel)
{
  int col, row;    /* Column and row indexes of cells in the grid */
  PanelField **cell = panel->grid;
  for(row=0; row < panel->nrow; row++) {
    for(col=0; col < panel->ncol; col++) {
      PanelField *field = *cell++;
      if(field) {
	field->text = NULL;
	queue_field_redraw(panel, field, FIELD_REDRAW_TEXT);
      };
    };
  };
  return;
}

/*.......................................................................
 * Register panel_array_callback() as a write and unset trace callback
 * for panel->tcl_array.
 *
 * Input:
 *  panel   TkPanel *  The widget resouce container.
 */
static void trace_panel_array(TkPanel *panel)
{
  if(panel->tcl_array) {
    Tcl_TraceVar(panel->interp, panel->tcl_array,
		 TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 panel_array_callback, (ClientData) panel);
  };
}

/*.......................................................................
 * Remove the trace callback from panel->tcl_array.
 *
 * Input:
 *  panel   TkPanel *  The widget resource container.
 */
static void untrace_panel_array(TkPanel *panel)
{
  if(panel->tcl_array) {
    Tcl_UntraceVar(panel->interp, panel->tcl_array,
		   TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		   panel_array_callback, (ClientData) panel);
  };
}

/*.......................................................................
 * This is the Tk_OptionParseProc() used to parse "-size {$ncol $nrow}"
 * configuration strings, and resize the grid to the requested size,
 * deleting any fields that lie outside the final grid.
 *
 * Input:
 *  context  ClientData    Unused (0).
 *  interp   Tcl_Interp *  The Tcl interpretter that owns the widget.
 *  tkwin     Tk_Window *  The target window.
 *  value          char *  The string to parse. This must contain
 *                         two integers, the number of columns followed
 *                         by the number of rows.
 *  panel_ptr      char *  The widget resource container cast to (char *).
 *  offset          int    The offset of the size_spec member wrt panel_ptr.
 * Output:
 *  return          int    TCL_OK
 *                         TCL_ERROR (A message will be left in
 *                         interp->result).
 */
#ifdef USE_CHAR_DECL
static int parse_size_spec(ClientData context, Tcl_Interp *interp,
			   Tk_Window tkwin, char *value, char *panel_ptr,
			   int offset)
#else
static int parse_size_spec(ClientData context, Tcl_Interp *interp,
			   Tk_Window tkwin, const char *value, char *panel_ptr,
			   int offset)
#endif
{
  int size[2];      /* The requested size for the grid {ncol,nrow} */
  int ncol, nrow;   /* The number of columns and rows, taken from size[] */
  char *endp;       /* The pointer to the next unprocessed character after */
                    /*  a call to strtol(). */
  char *cptr;       /* The next character to process from value[] */
  int i;
/*
 * Get the resource container of the widget.
 */
  TkPanel *panel = (TkPanel *) panel_ptr;
/*
 * Read the requested size of the grid.
 */
  cptr = (char* )value;
  for(i=0; i<2; i++) {
/*
 * Skip leading white-space.
 */
    while(isspace((int) *cptr))
      cptr++;
/*
 * Attempt to read the number of columns (i=0) or rows (i=1).
 */
    size[i] = strtol(cptr, &endp, 10);
    if(endp == cptr)
      return bad_size_spec(panel);
    cptr = endp;
  };
/*
 * Skip trailing white-space.
 */
  while(isspace((int) *cptr))
    cptr++;
/*
 * We should be at the end of the string.
 */
  if(*cptr != '\0')
    return bad_size_spec(panel);
/*
 * Extract the numbers that were requested.
 */
  ncol = size[0];
  nrow = size[1];
/*
 * Check the requested size.
 */
  if(ncol < 0 || ncol > PANEL_MAX_COL ||
     nrow < 0 || nrow > PANEL_MAX_ROW)
    return bad_size_spec(panel);
/*
 * Resize the grid.
 */
  return resize_panel_grid(panel, ncol, nrow);
}

/*.......................................................................
 * This is the error-return function of parse_size_spec(). It resets the
 * value of panel->size_spec to the actual size of the grid, writes an
 * error message to panel->interp->result, and returns TCL_ERROR.
 */
static int bad_size_spec(TkPanel *panel)
{
  sprintf(panel->size_spec, "%d %d", panel->ncol, panel->nrow);
  Tcl_AppendResult(panel->interp, "Usage: -size {$ncol $nrow}", NULL);
  return TCL_ERROR;
}

/*.......................................................................
 * This is the Tk_OptionPrintProc() used to print "-size {$ncol $nrow}"
 * configuration strings.
 *
 * Input:
 *  context     ClientData    Unused (0).
 *  tkwin        Tk_Window *  The target window.
 *  panel_ptr         char *  The widget resource container cast to (char *).
 *  offset             int    The offset of the size_spec member wrt panel_ptr.
 * Input/Output:
 *  free_proc Tcl_FreeProc ** The destructor to apply to 'context'.
 * Output:
 *  return          int    TCL_OK
 *                         TCL_ERROR (A message will be left in
 *                         interp->result).
 */
#ifdef USE_CHAR_DECL
static char *print_size_spec(ClientData context, Tk_Window tkwin,
			     char *panel_ptr, int offset,
			     Tcl_FreeProc **free_proc)
#else
static char *print_size_spec(ClientData context, Tk_Window tkwin,
			     char *panel_ptr, int offset,
			     Tcl_FreeProc **free_proc)
#endif
{
  TkPanel *panel = (TkPanel *) panel_ptr;
  return panel->size_spec;
}

