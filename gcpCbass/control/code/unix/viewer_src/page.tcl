#-----------------------------------------------------------------------
# Initialize the page facility.
#-----------------------------------------------------------------------
namespace eval ::page {
#
# page_count is used to assign unique page ids. It is incremented
# every time that a new page is created and never decremented.
#
  set page_count 0
#
# List justification specifiers.
#
  set justify_names [list left center right]
#
# List font size specifiers.
#
  set font_size_choices [list 9 11 13 15 17 19]
#
# List register output format names.
#
  set format_names [list fixed_point scientific floating sexagesimal \
      integer hex octal binary string date bit enum bool complex_fixed]
#
# Create nested namespaces.
#
  namespace eval field {}
  namespace eval label {}
  namespace eval register {}
}

#-----------------------------------------------------------------------
# Write the configuration of a single page to a given file channel as
# a TCL script that can later be used to recreate the page.
#
# Input:
#  page     The page widget who's configuration is to be recorded.
#  out      A file channel opened for write.
#-----------------------------------------------------------------------
proc save_page {page out} {
#
# Get the configuration array of the page.
#
  upvar #0 $page p
#
# Also get the grid widget.
#
  set grid $page.grid
#
# Get the current number of rows and columns in the page.
#
  ::page::grid_size $page cols rows


#
# Construct a list of column widths.
#
  set widths [list]
  for {set col 0} {$col < $cols} {incr col} {
    lappend widths [$page.grid column $col -width]
  }
#
# Output the page configuration to the file.
#

  puts $out "add_page [winfo rootx $page] [winfo rooty $page] [list [wm title $page]] [list [$grid cget -bg]] [list [$grid cget -warn_color]] [list $p(dopage_color)] [$grid cget -column_width] [string trimleft [$grid cget -font] {font}] [list $widths] [list $p(regMapGroup)] $p(regMapNumber) [list $p(boardGroup)] $p(boardNumber) {"
  for {set col 0} {$col < $cols} {incr col} {
    for {set row 0} {$row < $rows} {incr row} {
      if [info exists p($col,$row)] {
	upvar #0 $p($col,$row) f
	puts -nonewline $out "  $f(type) $col $row [list $f(bg)] [list $f(fg)] [list $f(justify)]"
	switch $f(type) {
	  label {
	    puts $out " [list $f(text)]"
	  }
	  register {
	    puts $out " [list $f(register)] [list $f(format)] [list $f(precision)] [list $f(misc)] [list $f(enum)] [list $f(signed)] [list $f(warn)] [list $f(min)] [list $f(max)] [list $f(dopage)] [list $f(nframe)]"
	  }
	}
      }
    }
  }
  puts $out "}"
}

#-----------------------------------------------------------------------
# Open a specified new file, save the configuration of the current page
# to it, then close the file.
#
# Input:
#  page      The page who's configuration is to be saved.
#  file      The name of the file to record the configuration in.
#-----------------------------------------------------------------------
proc page::save_page_to_file {page file} {
#
# Open the specified file.
#
  set out [open $file w]
#
# Save the configuration to it.
#
  if {[catch {save_page $page $out} result]} {
    report_error $result
  }
#
# Close the file.
#
  close $out
}

#-----------------------------------------------------------------------
# Update a given page to reflect changes in the register map. Currently
# this only involves updating the line of buttons that represents the
# available members of a regMapGroup of boards.
#
# Input:
#  page       The name of the global page configuration array, or
#             an empty string to request a new page.
#-----------------------------------------------------------------------
proc update_page {page} {
  upvar #0 $page p
#
# Create any buttons needed to switch between boards in the current
# board regMapGroup named by p(regMapGroup).
#
  page::update_page_regMapGroup $page
  page::update_page_boardGroup $page

  return
}

#-----------------------------------------------------------------------
# Add a new page to the viewer.
#
# Input:
#  x y            The root window location of the top-left corner of the
#                 widget, or {} {} to leave this up to the window manager.
#  title          The title to give the widget.
#  color          The background color of the widget.
#  warn_color     The color used to highlight out-of-range register values.
#  dopage_color   The color used to highlight paged register values.
#  def_width      The default width for new columns.
#  font_size      The size of the text font used by all fields.
#  widths         A list of initial column widths to give the first few
#                 columns. This can be an empty list if no columns have
#                 been created yet.
#  regMapGroup          The textual prefix of a regMapGroup of numbered boards, or
#                 {} if no regMapGrouping is wanted.
#  fields         Field description arguments arranged, one field per
#                 line. The first word on each line must be either
#                 "register" or "label" and must then be followed by the
#                 arguments expected by the add_register or add_label
#                 procedures, respectively.
# Ouput:
#  return         The pathname of the page. This is also the name of a
#                 global array that contains details about the page.
#-----------------------------------------------------------------------
proc add_page {{x {}} {y {}} {title Page} {color black} {warn_color red} {dopage_color yellow} {def_width 10} {font_size 11} {widths {}} {regMapGroup {}} {regMapNumber {0}} {boardGroup {}} {boardNumber {0}} {fields {}}} {
#
# Get a unique name for the new widget and its configuration variable.
#

  set page .page[incr ::page::page_count]

#
#
# Make sure that the page gets deleted on error.
#
 if [catch {
#
# Create a global array of page characteristics, named after the
# page widget.
#
    upvar #0 $page p
    set p(tag) [monitor add_page]   ;# The C layer identifier of the page.
    set p(count) 0                  ;# The number of fields created. Note
                                    ;#  that this does not decrease when a
                                    ;#  a field is deleted. It is used to
                                    ;#  given unique names to field widgets.
    set p(regMapGroup) $regMapGroup ;# The optional prefix of a group of
                                    ;#  numbered register maps
    set p(regMapNumber) $regMapNumber ;# The number to append to p(regMapGroup)
                                    ;#  to complete a registermap name.
    set p(boardGroup) $boardGroup   ;# The optional prefix of a group of
                                    ;#  numbered boards.
    set p(boardNumber) $boardNumber ;# The number to append to p(boardGroup)
                                    ;#  to complete a board name.
    set p(warn_color) $warn_color   ;# The default color for the warn border
    set p(dopage_color) $dopage_color;# The default color for paging
#
# The following characteristics contain the default attributes for label
# fields. Whenever a new label field is sucessfully created, the chosen
# values become the defaults for the next label field.
#
    set p(label_fg) {paleturquoise} ;# The default label-field foreground color.
    set p(label_bg) {midnightblue}  ;# The default label-field background color.
    set p(label_justify) center     ;# The default label-field justification.
    set p(label_text) {}            ;# The default label-field text.
#
# The following characteristics contain the default attributes for register
# fields. Whenever a new label field is sucessfully created, the chosen
# values become the defaults for the next register field.
#
    set p(register_fg) {green}      ;# Default register-field foreground color
    set p(register_bg) {black}      ;# Default register-field background color
    set p(register_justify) center  ;# Default register-field justification.
    set p(register_register) array.frame.record ;# Default register-field register.
    set p(register_format) floating ;# Default register-field display format.
    set p(register_precision) 15    ;# Default register-field precision.
    set p(register_signed) 0        ;# Default register-field sign allocation.
    set p(register_misc) 0          ;# Default register-field misc attribute.
    set p(register_enum) {}         ;# Default register-field enum attribute.
    set p(register_warn) 0          ;# Default register-value warn attribute.
    set p(register_min) 0.0         ;# Default register min field value.
    set p(register_max) 0.0         ;# Default register max field value.
    set p(register_dopage) 0        ;# Default register-value page attribute.
    set p(register_nframe) 1        ;# Default threshold for paging (frames)

#
# The following fields are used when selecting fields with the mouse for
# deletion, or for drag and drop operations.
#
    page::reset_selection_vars $page
#
# Create a toplevel frame to contain the page and its associated
# configuration and informational widgets.
#
    toplevel $page -class MonitorPage
    wm title $page $title
    wm iconname $page $title
    wm protocol $page WM_DELETE_WINDOW "::page::quit_page $page"
    wm withdraw $page
#
# Tell the window manager where to map the window?
#
    if {[is_uint $x] && [is_uint $y]} {
      wm geometry $page +${x}+${y}
    }
#
# Arrange for the contents of the page only to be updated when the
# page widget is mapped.
#
    bind $page <Unmap> "::page::freeze_page $page"
    bind $page <Map> "::page::unfreeze_page $page"
#
# Create the main components of the widget.
#
    page::create_page_menubar $page.bar
    page::create_page_regMapGroup $page
    page::create_page_boardGroup $page
    page::create_page_panel_widget $page.grid

#
# Create umapped configuration dialogs for later.
# 
    if {![winfo exists .page_dialog]} {
      page::create_page_dialog .page_dialog
      page::create_column_dialog .column_dialog
      page::create_field_dialog .label_dialog label
      page::create_field_dialog .register_dialog register
    }
#
# Create the page-specific dialog that is displayed when the user
# asks to save the configuration of the page to a file.
#
    page::create_save_page_dialog $page
#
# Create the menu that is posted whenever the user presses the
# right mouse button (or types space) over one of the fields of
# a page.
#
    if {![winfo exists .field_menu]} {
      set m [menu .field_menu -tearoff 0]
      $m add command -label "Modify field" -command \
	  [list set ::field_menu_var edit]
      $m add command -label "Delete field" -command \
	  [list set ::field_menu_var delete]
      $m add command -label "Save field" -command \
	  [list set ::field_menu_var save]
    }
#
# Create the menu that is posted whenever the user presses the
# right mouse button (or types space) over an unassigned field of
# a page.
#
    if {![winfo exists .field_type_menu]} {
      set m [menu .field_type_menu -tearoff 0]
      $m add command -label "Create field" -state disabled
      $m add separator
      $m add command -label "label" -command \
	  [list set ::field_type_menu_var label]
      $m add command -label "register" -command \
	  [list set ::field_type_menu_var register]
    }
#
# Create the menu that is posted whenever the user presses the
# middle mouse button over a page cell. This allows one to delete
# or insert rows and columns.
#
    if {![winfo exists .page_col_row_menu]} {
      set m [menu .page_col_row_menu -tearoff 0]
      $m add command -label "Column menu" -state disabled
      $m add separator
      $m add command -label "Insert column left" -command \
	  [list set ::page_col_row_menu_var insert_col_left]
      $m add command -label "Insert column right" -command \
	  [list set ::page_col_row_menu_var insert_col_right]
      $m add command -label "Delete Column" -command \
	  [list set ::page_col_row_menu_var delete_col]
      $m add command -label "Fit Column Width" -command \
	  [list set ::page_col_row_menu_var fit_col_width]
      $m add command -label "Set Column Width" -command \
	  [list set ::page_col_row_menu_var set_col_width]
      $m add separator
      $m add command -label "Row menu" -state disabled
      $m add separator
      $m add command -label "Insert row above" -command \
	  [list set ::page_col_row_menu_var insert_row_above]
      $m add command -label "Insert row below" -command \
	  [list set ::page_col_row_menu_var insert_row_below]
      $m add command -label "Delete Row" -command \
	  [list set ::page_col_row_menu_var delete_row]
    }
#
# Arrange the vertical panes of the widget.
#
    pack $page.bar -side top -fill x
    pack $page.grid -side top -fill both -expand true
#
# Arrange for the field_type menu to be posted and read whenever the
# the user presses the left mouse button or space-bar over a vacant field.
#
    bind $page.grid <space> "::page::read_field_menu $page %X %Y"
    bind $page.grid <3> "::page::read_field_menu $page %X %Y"
#
# Create the mouse bindings that control field selection and drag and
# drop operations.
#
    bind $page.grid <ButtonPress-1> "::page::select_fields $page %X %Y"
    bind $page.grid <B1-Motion> "::page::extend_field_selection $page %X %Y"
    bind $page.grid <ButtonRelease-1> "::page::update_selection_text $page"
#
# Create a mouse binding to post the row/column manipulation menu.
#
    bind $page.grid <2> "::page::read_page_col_row_menu $page %X %Y"
#
# Create keyboard short-cuts.
#
    bind $page <Meta-q> "::page::quit_page $page"
    bind $page <Key-Delete> "::page::delete_selected_fields $page"
    bind $page <Key-BackSpace> "::page::delete_selected_fields $page"
#
# Tell the window manager where to map the window?
#
    if {[is_uint $x] && [is_uint $y]} {
      wm geometry $page +${x}+${y}
    }
#
# Register a selection handler for the page.
#
    selection handle $page [list page::return_selection_text $page]
#
# Create and configure the widths of the first few columns.
#
    set ncol [llength $widths]
    for {set col 0} {$col < $ncol} {incr col} {
      set width [lindex $widths $col]
      if {![is_uint $width]} {
	error "Bad column width: $width"
      } else {
	$page.grid column $col -width $width
      }
    }
#
# Configure the general characteristics of the page.
#
    page::configure_page $page $title $color $warn_color $dopage_color \
	    $def_width $font_size $regMapGroup $regMapNumber $boardGroup $boardNumber
#
# Add fields to the page.
#
    foreach line [split $fields "\n"] {
      if {[regexp -- {^[ \t]*(register|label)([ \t].*)} $line unused type args]} {
	eval page::add_$type $page $args
      } elseif {[regexp -- {^[ \t]*$} $line]} {
	continue;
      } else {
	error "Unknown type of field in page description."
      }
    }
#
# Add the page to the list of pages hosted by the viewer.
#
    lappend ::viewer(pages) $page

    page::shrink_wrap $page

#
# Make the page visible.
#
    wm deiconify $page
#
# If either of the above procedures failed, delete the partially
# created page, and rethrow the error. Note that delete_field
# deletes both the panel and the monitor_viewer aspects of the field.
#
  } result] {
    delete_page $page
    error $result
  }
  return $page
}

#-----------------------------------------------------------------------
# Adopt general user-specified characteristics for a given page widget.
#
# Input:
#  page           The page widget to be configured.
#  title          The title of the widget.
#  color          The background color of the widget.
#  warn_color     The color used to highlight out-of-range register values.
#  dopage_color   The color used to highlight paged register values.
#  def_width      The default width for new columns.
#  font_size      The size of the text font used by all fields.
#  regMapGroup          The textual prefix of a regMapGroup of numbered boards, or
#                 {} if no regMapGrouping is wanted.
#  boardGroup          The textual prefix of a regMapGroup of numbered boards, or
#                 {} if no regMapGrouping is wanted.
#-----------------------------------------------------------------------
proc page::configure_page {page title color warn_color dopage_color def_width font_size regMapGroup regMapNumber boardGroup boardNumber} {
  upvar #0 $page p       ;# The configuration array of the page.
#
# Validate the parameters.
#

  if {[catch {winfo rgb . $color}] } {
    error "Bad page color: $color"
  } elseif {[catch {winfo rgb . $warn_color}] } {
    error "Bad warning color: $warn_color"
  } elseif {[catch {winfo rgb . $dopage_color}] } {
    error "Bad paging color: $dopage_color"
  } elseif {![is_uint $def_width]} {
    error "Bad default column width: $def_width"
  } elseif {[lsearch -exact $::page::font_size_choices $font_size] < 0} {
    error "Bad page font size: $font_size"
  } elseif {![regexp -- {^[a-zA-Z_]*$} $regMapGroup]} {
    error "Bad regMapGroup prefix"
  } elseif {![is_uint $regMapNumber]} {
    error "Bad regMapGroup number"
  } elseif {![regexp -- {^[a-zA-Z_]*$} $boardGroup]} {
    error "Bad boardGroup prefix"
  } elseif {![is_uint $boardNumber]} {
    error "Bad board number"
  }

#
# Update the title of the page.
#
  wm title $page $title
  wm iconname $page $title
#
# If a font with the requested font size doesn't exist yet, create it
# now.
#
  if {[lsearch -exact [font names] font$font_size] < 0} {
    font create font$font_size -family Helvetica -size $font_size -weight bold
  }
#
# Configure the panel widget.
#
  $page.grid configure -bg $color -warn_color $warn_color \
	  -column_width $def_width -font font$font_size
#
# Record the paging color
#
    set p(dopage_color) $dopage_color
#
# Did the register map group specification change?
#

  if {[string compare $p(regMapGroup) $regMapGroup] != 0 || \
      [string compare $p(regMapNumber) $regMapNumber] != 0 || \
     ![info exists p(regmap_pattern)] || \
      [string compare $p(boardGroup) $boardGroup] != 0 || \
      [string compare $p(boardNumber) $boardNumber] != 0 || \
      ![info exists p(board_pattern)]} {

#
# Record the details of the board regMapGroup.
#
    set p(regMapGroup)  $regMapGroup
    set p(boardGroup)   $boardGroup
    set p(regMapNumber) $regMapNumber
    set p(boardNumber)  $boardNumber
#
# Record the regular expression patterns used to detect and decompose
# the boards and registers that belong to the regMapGroup. Keeping this
# in a variable allows Tcl to cache the compiled regular expressions
# after their first use.
#
    set p(regmap_pattern) "^${regMapGroup}(\[0-9\]+)\$"
    set p(board_pattern)  "^${boardGroup}(\[0-9\]+)\$"
    set p(reg_pattern)    "^(\\..+)"      

    if {[string length $regMapGroup] != 0} {
      set p(reg_pattern)    "^${regMapGroup}\[0-9\]+(\\..+)\$"      
      if {[string length $boardGroup] != 0} {
        set p(reg_pattern)   "^${regMapGroup}.${boardGroup}\[0-9\]+(\\..+)\$"      
      }
    } 

#
# Reconfigure the register fields to accomodate potential changes of
# their registers. 
#
    foreach field [list_fields $page] {
      upvar #0 $field f
      if {[string compare $f(type) register] == 0} {

	configure_register $field $f(bg) $f(fg) $f(justify) $f(register) \
		$f(format) $f(signed) $f(precision) $f(misc) $f(enum) $f(warn) \
		$f(min) $f(max) $f(dopage) $f(nframe)
      }
    }
#
# Configure the box of buttons that is used to switch between different
# boards which have the same prefix name but different numeric suffixes.
#
    page::update_page_regMapGroup $page
    page::update_page_boardGroup $page
}
}

#-----------------------------------------------------------------------
# Display a configuration dialog to allow the user to configure the
# general characteristics of a new or existing page.
#
# Input:
#  page       The page to be reconfigured, or {} to request that a new
#             page be created.
# Output:
#  return     The pathname of the page, or {} if a new page couldn't be
#             created.
#-----------------------------------------------------------------------
proc page::show_page_dialog {page} {
#
# Create a new page?
#
  if {[string length $page]==0} {
    set provisional 1
    set page [add_page]
    set cancel_command [list delete_page $page]
    upvar #0 $page p
    set grid $page.grid
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the page to be restored.
#
  } else {
    upvar #0 $page p
    set provisional 0
    set grid $page.grid
    set cancel_command [list configure_page $page [wm title $page] \
	[$grid cget -bg] [$grid cget -warn_color] [list $p(dopage_color)] \
	[$grid cget -column_width] [string trimleft [$grid cget -font] {font}] \
	$p(regMapGroup) $p(regMapNumber) $p(boardGroup) $p(boardNumber)]
  }
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::page_dialog_state apply
#
# Get the path name of the page-configuration dialog and its configuration
# area.
#
  set dialog .page_dialog
  set w $dialog.top
#
# Copy the current page configuration into the dialog.
#
  replace_entry_contents $w.title.e [wm title $page]
  replace_entry_contents $w.bg.e [$grid cget -bg]
  set ::$w.font_size [string trimleft [$grid cget -font] {font}]
  replace_entry_contents $w.dopage_color.e $p(dopage_color)
  replace_entry_contents $w.warn_color.e [$grid cget -warn_color]
  replace_entry_contents $w.def_width.e [$grid cget -column_width]
  replace_entry_contents $w.regMapGroup.e $p(regMapGroup)
  replace_entry_contents $w.boardGroup.e $p(boardGroup)
#
# Display the dialog.
#
  map_dialog $dialog 1
#
# Prevent other interactions with the program until the dialog has
# been finished with.
#
  set old_focus [focus]               ;# Record the keyboard input focus.
  focus $dialog                       ;# Move keyboard focus to the dialog
  grab set $dialog                    ;# Prevent conflicting user input
  tkwait variable page_dialog_state   ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::page_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {

      configure_page $page [$w.title.e get] [$w.bg.e get] \
                     [$w.warn_color.e get] [$w.dopage_color.e get] \
		     [$w.def_width.e get] \
                     [set ::$w.font_size] [$w.regMapGroup.e get] $p(regMapNumber) [$w.boardGroup.e get] $p(boardNumber)
      set ::page_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable page_dialog_state   ;# Wait for the user again.
    }
  }
#
# Withdraw the dialog.
#
  grab release $dialog          ;# Re-enable interactions with other windows
  focus $old_focus              ;# Restore the original keyboard focus.
  wm withdraw $dialog           ;# Hide the dialog for later use.
  dialog_error $dialog {}       ;# Remove the error message dialog.
#
# Did the user hit the cancel button?
#
  if {[string compare $::page_dialog_state cancel] == 0} {
#
# If the page is provisional, delete it. Otherwise restore its original
# configuration.
#
    if {$provisional} {
      delete_page $page
      return {}
    } else {
      eval $cancel_command
    }
  } else {
#
# If the page is new, make it visible.
#
    if {$provisional} {
      wm deiconify $page
      tkwait visibility $page
    }
  }
  return $page
}

#-----------------------------------------------------------------------
# Delete a page widget and its C counterpart.
#-----------------------------------------------------------------------
proc delete_page {page} {
  upvar #0 $page p
#
# Remove the page from monitor_viewer.
#
  if {$p(tag) != 0} {
    monitor remove_page $p(tag)
  }
#
# Delete the field configuration arrays.
#
  foreach field [::page::list_fields $page] {
    global $field
    unset $field
  }
#
# Delete the save-page dialog of the widget.
#
  destroy [::page::save_config_dialog $page]
#
# Delete the page widget.
#
  destroy $page
#
# Delete the page configuration array.
#
  unset p
#
# Delete the grid text-variable array.
#
  unset ::$page.grid
#
# Remove the page from the list of pages managed by the viewer.
#
  set index [lsearch -exact $::viewer(pages) $page]
  if {$index >= 0} {
    set ::viewer(pages) [lreplace $::viewer(pages) $index $index]
  }
}

#-----------------------------------------------------------------------
# Add a label field to a given page.
#
# Input:
#  page        The parent page.
#  col row     The location of the field in the grid (column,row).
#  bg fg       The bacground and foreground colors of the field.
#  justify     The justification of the text in the field (left center right).
#  text        The text of the label.
# Output:
#  field       The new field.
#-----------------------------------------------------------------------
proc page::add_label {page col row {bg {}} {fg {}} {justify {}} {text {}}} {
  upvar #0 $page p
#
# Check the requested location.
#
  if { ![is_uint $row] || ![is_uint $col] } {
    error "Bad row,col field location: $row,$col"
  }
#
# If the cell is already in use, delete its owner first.
#
  if {[info exists p($col,$row)]} {
    delete_field $page $col $row
  }
#
# Determine a unique name to give to the field.
#
  set field $page.grid.[incr p(count)]
#
# Create a global configuration with the above name,
# then append this name to the list of fields in the host page.
#
  upvar #0 $field f
  set f(type) label         ;# The field type (label or register)
  set f(page) $page         ;# The parent page.
  set f(col) 0              ;# The column in which the field is displayed.
  set f(row) 0              ;# The row in which the field is displayed.
#
# Add the field to the grid.
#
  grid_field $page $field $col $row
#
# Configure the field.
#
  if {[catch {
    configure_label $field $bg $fg $justify $text
  } result]} {
    delete_field $page $col $row
    error $result
  }
  return $field
}

#-----------------------------------------------------------------------
# Validate and change the user-settable parameters of an existing label
# field.
#
# Input:
#  field      The field to be configured.
#  bg fg       The bacground and foreground colors of the field.
#  justify     The justification of the text in the field (left center right).
#  text        The text of the label.
#-----------------------------------------------------------------------
proc page::configure_label {field {bg {}} {fg {}} {justify {}} {text {}}} {
#
# Get the configuration arrays of the field and its parent page.
#
  upvar #0 $field f
  set page $f(page)
  upvar #0 $page p
#
# Substitute defaults for omitted arguments.
#
  foreach arg {bg fg justify text} {
    if {[string length [set $arg]]==0} {
      set $arg $p(label_$arg)
    }
  }
#
# Reconfigure the panel entry of the field.
#
  $page.grid $f(col),$f(row) -bg $bg -fg $fg -pack $justify -flag 0
  set ::$page.grid($f(col),$f(row)) $text
#
# Record the new configuration.
#
  foreach item {bg fg justify text} {
    set f($item) [set $item]
  }
}
 
#-----------------------------------------------------------------------
# Add a register field to a given page.
#
# Input:
#  page        The parent page.
#  col row     The location of the field in the grid (column,row).
#  bg fg       The foreground and background colors of the field.
#  justify     The justification of the text in the field (left center right).
#  register    The specification of the register to be displayed.
#  format      The display format for the register.
#  precision   The floating point precision to display.
#  misc        The miscellaneous formatting number associated with $format.
#  enum        The list of names to use with enumeration and boolean formats.
#  signed      A boolean value specifying whether to allocate space for a
#              sign character.
#  warn        If true, warn if the register value is outside the range
#              of the following min,max arguments.
#  min max     If warn is true, these set the min and max floating point
#              values of the valid range of values.
# Output:
#  field       The new field.
#-----------------------------------------------------------------------
proc page::add_register {page col row {bg {}} {fg {}} {justify {}} \
    {register {}} {format {}} {precision {}} {misc {}} \
    {enum {}} {signed {}} {warn {}} {min {}} {max {}} {dopage {}} {nframe {}}} {
  upvar #0 $page p
#
# Check the requested location.
#
  if { ![is_uint $row] || ![is_uint $col] } {
    error "Bad row,col field location: $row,$col"
  }
#
# If the cell is already in use, delete its owner first.
#
  if {[info exists p($col,$row)]} {
    delete_field $page $col $row
  }
#
# Register the field with monitor_viewer.
#
  set tag [monitor add_field $p(tag)]
#
# Determine a unique name to give to the field.
#
  set field $page.grid.[incr p(count)]
#
# Create a global configuration with the above name,
# then append this name to the list of fields in the host page.
#
  upvar #0 $field f
  set f(type) register      ;# The field type (label or register)
  set f(tag) $tag           ;# The monitor-viewer id of the field (see below).
  set f(page) $page         ;# The parent page.
  set f(col) 0              ;# The column in which the field is displayed.
  set f(row) 0              ;# The row in which the field is displayed.
  set f(flag) off           ;# The state of the warning border (on or off).
  set f(pflag) off          ;# The state of the pager (on or off).
  set f(warn) 0             ;# True to enable warnings.
  set f(dopage) 0           ;# True to enable paging.
#
# Monitor_viewer writes to the following variable whenever the warning
# border needs to be updated.
#
  trace variable ::$field\(flag\) w [list update_warn_border $page $field]
#
# Monitor_viewer writes to the following variable whenever the pager should
# be activated
#
  trace variable ::$field\(pflag\) w [list update_pager_state $page $field]

#
# Add the field to the grid.
#
  grid_field $page $field $col $row
#
# Configure the field.
#
  if {[catch {
    configure_register $field $bg $fg $justify $register $format $signed \
	$precision $misc $enum $warn $min $max $dopage $nframe
  } result]} {
    delete_field $page $col $row
    error $result
  }
  return $field
}

#-----------------------------------------------------------------------
# Validate and change the user-settable parameters of an existing register
# field.
#
# Input:
#  field      The field to be configured.
#  bg fg       The background and foreground colors of the field.
#  justify     The justification of the text in the field (left center right).
#  register    The specification of the register to be displayed.
#  format      The display format for the register.
#  signed      A boolean value specifying whether to allocate space for a
#              sign character.
#  precision   The floating point precision to display.
#  misc        The miscellaneous formatting number associated with $format.
#  enum        The list of names to use with enumeration and boolean formats.
#-----------------------------------------------------------------------
proc page::configure_register {field bg fg justify register format signed \
	precision misc enum warn min max dopage nframe} {
    #
    # Get the configuration arrays of the field and its parent page.
#
    upvar #0 $field f
    set page $f(page)
    upvar #0 $page p
    #
    # Substitute defaults for omitted arguments.
    #
    foreach arg {bg fg justify register format signed precision misc enum \
	    warn min max dopage nframe} {
	if {[string length [set $arg]]==0} {
	    set $arg $p(register_$arg)
	}
    }
#
# Should the warning border be dislayed?
#
    if {$warn && $f(flag)} {
	set flag on
    } else {
	set flag off
    }
  #
# Reconfigure the panel entry of the field.
#
  $page.grid $f(col),$f(row) -bg $bg -fg $fg -pack $justify -flag $flag 
  set ::$page.grid($f(col),$f(row)) -
  #
# If the register is marked as being signed, arrange for space to be
# left for a sign character.
#
  if {$signed} {
      set flags {0 }
  } else {
      set flags {0}
  }
#
# If the parent page is showing one of a group of register maps, filter
# the register name to switch to the currently selected register map.
#

  if {[string length $p(regMapGroup)] > 0} {
    if {[string length $p(boardGroup)] > 0} {
      if {[regexp -- $p(reg_pattern) $register dummy reg]} {
	  set register $p(regMapGroup).$p(boardGroup)$p(boardNumber)$reg
      }
    } else {
      if {[regexp -- $p(reg_pattern) $register dummy reg]} {
	  set register $p(regMapGroup)$p(regMapNumber)$reg
      } 
    }
  }

#
# Reconfigure the monitor-viewer attributes of the field.
#
  monitor configure_field $p(tag) $f(tag) $register $format $flags 0 \
	  $precision $misc $enum $warn $min $max $dopage $nframe
#
# Record the new configuration.
#
  foreach item {bg fg justify register format signed precision misc enum warn min max dopage nframe} {
      set f($item) [set $item]
  }
}

#-----------------------------------------------------------------------
# Display a configuration dialog to allow the user to configure an
# existing field.
#
# Input:
#  page         The parent page of the field.
#  col row      The column,row location of the field.
#  provisional  If true, delete the field if the user presses the cancel
#               button.
# Output:
#  return     The name of the field, or {} if a new field couldn't be
#             created.
#-----------------------------------------------------------------------
proc page::show_field_dialog {page col row provisional} {
  upvar #0 $page p       ;# The configuration array of the page.
  set grid $page.grid    ;# The pathname of the panel widget.
  upvar #0 $grid g       ;# The text-variable array of the grid.
#
# Does the register field exist yet?
#
  if {![info exists p($col,$row)]} {
    error "Can't configure nonexistent field."
  }
#
# Get the configuration array of the field.
#
  set field $p($col,$row)
  upvar #0 $field f
#
# In case the user cancel's an erroneously entered configuration,
# construct a command to use to restore the initial configuration.
#
  switch $f(type) {
    label {
      set cancel_command [list configure_label $field $f(bg) $f(fg) $f(justify) $f(text)]
    }
    register {
      set cancel_command [list configure_register $field $f(bg) $f(fg) $f(justify) $f(register) $f(format) $f(signed) $f(precision) $f(misc) $f(enum) $f(warn) $f(min) $f(max) $f(dopage) $f(nframe) ]
    }
  }
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::field_dialog_state apply
#
# Get the path name of the type-specific field-configuration dialog and
# its configuration area.
#
  set dialog .$f(type)_dialog
  set w $dialog.top
#
# Copy the current field configuration into the dialog.
#
  replace_entry_contents $w.fg.e $f(fg)
  replace_entry_contents $w.bg.e $f(bg)
  set ::$w.justify $f(justify)
  switch $f(type) {
    label {
      replace_entry_contents $w.text.e $f(text)
    }
    register {
      replace_entry_contents $w.reg.e $f(register)
      set ::$w.format $f(format)
      replace_entry_contents $w.prec.e $f(precision)
      set ::$w.signed $f(signed)
      replace_entry_contents $w.misc.e $f(misc)
      replace_entry_contents $w.enum.e $f(enum)
      set ::$w.warn $f(warn)
      replace_entry_contents $w.range.min $f(min)
      replace_entry_contents $w.range.max $f(max)
      set ::$w.dopage $f(dopage)
      replace_entry_contents $w.fpage.nframe $f(nframe)
    }
  }
#
# Display the dialog.
#
  map_dialog $dialog 1
#
# Prevent other interactions with the program until the dialog has
# been finished with.
#
  set old_focus [focus]               ;# Record the keyboard input focus.
  focus $dialog                       ;# Move keyboard focus to the dialog
  grab set $dialog                    ;# Prevent conflicting user input
  tkwait variable field_dialog_state   ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::field_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {
      switch $f(type) {
	label {
	  configure_label $field [$w.bg.e get] [$w.fg.e get] \
	      [set ::$w.justify] [$w.text.e get]
	}
	register {
	  configure_register $field [$w.bg.e get] [$w.fg.e get] \
	      [set ::$w.justify] [$w.reg.e get] [set ::$w.format] \
	      [set ::$w.signed] [$w.prec.e get] [$w.misc.e get] \
	      [$w.enum.e get] [set ::$w.warn] \
              [$w.range.min get] [$w.range.max get] \
	      [set ::$w.dopage] [$w.fpage.nframe get]
	}
      }
#
# See if there are any errors while attempting to reconfigure.
#
      monitor reconfigure
      set ::field_dialog_state "done"
    } result]} {
      dialog_error $dialog $result         ;# Exhibit the error message.
      tkwait variable field_dialog_state   ;# Wait for the user again.
    }
  }
#
# Withdraw the dialog.
#
  grab release $dialog          ;# Re-enable interactions with other windows
  focus $old_focus              ;# Restore the original keyboard focus.
  wm withdraw $dialog           ;# Hide the dialog for later use.
  dialog_error $dialog {}       ;# Remove the error message dialog.
#
# Did the user cancel the configuration?
#
  if {[string compare $::field_dialog_state cancel] == 0} {
#
# If the field was provisional, delete it. Otherwise restore its
# original configuration.
#
    if {$provisional} {
      delete_field $page $col $row
      monitor reconfigure
      return {}
    } else {
      eval $cancel_command
      monitor reconfigure
    }
#
# On successful reconfiguration, record the new values as the default values
# for the next field of this type.
#
  } else {
    adopt_field_pars $field
  }
  return $field
}

#-----------------------------------------------------------------------
# Update the frame that contains one button per page of a selected regMapGroup
# of pages.
#
# Input:
#  page      The name of the parent page widget.
#-----------------------------------------------------------------------
proc page::update_page_regMapGroup {page} {
  upvar #0 $page p

#
# Before doing anything destroy the button callback to prevent it firing
# while we are making changes.
#
  trace vdelete ::$page.regMapGroup w [list page::regMapGroup_button_callback $page]
#
# Discard the current contents of the frame, and remove any variable
# trace from the button-value variable.
#
  foreach slave [pack slaves $page.regMapGroup] {
    destroy $slave
  }
#
# If a list of register maps is available and a register map-regMapGroup prefix 
# has provided, create one button per register map that belongs to the regMapGroup.
#
  set first {}
  if {[monitor have_stream] && [string compare $p(regMapGroup) {}] != 0} {
    pack [label $page.regMapGroup.name -text "$p(regMapGroup): " -fg TextLabelColor] -side left

    foreach name [monitor list_regmaps] {
      if {[regexp -- $p(regmap_pattern) $name dummy n] == 1} {
	radiobutton $page.regMapGroup.$n -text $n -variable $page.regMapGroup -value $n \
	    -indicatoron 0 -width 2
	pack $page.regMapGroup.$n -side left

	if {$first == {}} {
	  set first $n
	}
      }
    }
  }
#
# Display the frame if it now contains any buttons, and set up a variable
# trace to respond to the user pressing any of the buttons.
#

  if {$first != {}} {
    pack $page.regMapGroup -after $page.bar -side top -fill x -expand true
    set ::$page.regMapGroup $p(regMapNumber)
    trace variable ::$page.regMapGroup w [list page::regMapGroup_button_callback $page]
  } else {
    pack forget $page.regMapGroup
  }
  return
}

#-----------------------------------------------------------------------
# Update the frame that contains one button per page of a selected group
# of pages.
#
# Input:
#  page      The name of the parent page widget.
#-----------------------------------------------------------------------
proc page::update_page_boardGroup {page} {
  upvar #0 $page p
#
# Before doing anything destroy the button callback to prevent it firing
# while we are making changes.
#
  trace vdelete ::$page.boardGroup w [list page::boardGroup_button_callback $page]
#
# Discard the current contents of the frame, and remove any variable
# trace from the button-value variable.
#
  foreach slave [pack slaves $page.boardGroup] {
    destroy $slave
  }
#
# If a list of register maps is available and a register map-boardGroup prefix 
# has provided, create one button per register map that belongs to the boardGroup.
#

  set first {}
  if {[monitor have_stream] && [string compare $p(boardGroup) {}] != 0} {
    pack [label $page.boardGroup.name -text "$p(regMapGroup).$p(boardGroup): " -fg TextLabelColor] -side left

    foreach name [monitor list_boards $p(regMapGroup)] {
      if {[regexp -- $p(board_pattern) $name dummy n] == 1} {
	radiobutton $page.boardGroup.$n -text $n -variable $page.boardGroup -value $n \
	    -indicatoron 0 -width 2
	pack $page.boardGroup.$n -side left
	if {$first == {}} {
	  set first $n
	}
      }
    }
  }
#
# Display the frame if it now contains any buttons, and set up a variable
# trace to respond to the user pressing any of the buttons.
#
  if {$first != {}} {
    pack $page.boardGroup -after $page.bar -side top -fill x -expand true
    set ::$page.boardGroup $p(boardNumber)
    trace variable ::$page.boardGroup w [list page::boardGroup_button_callback $page]
  } else {
    pack forget $page.boardGroup
  }
  return
}

#-----------------------------------------------------------------------
#  ***************  PAGE MANAGEMENT PROCEDURES  **********************  |
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# When a page widget is unmapped (eg. iconized) tell the viewer to stop
# updating its contents.
#
# Input:
#  page        The page widget.
#-----------------------------------------------------------------------
proc page::freeze_page {page} {
  upvar #0 $page p
  if {$p(tag) != 0} {
    monitor freeze_page $p(tag)
  }
}

#-----------------------------------------------------------------------
# When a page widget is newly mapped (eg. deiconized) tell the viewer to
# resume updating its contents.
#
# Input:
#  page        The page widget.
#-----------------------------------------------------------------------
proc page::unfreeze_page {page} {
  upvar #0 $page p
  if {$p(tag) != 0} {
    monitor unfreeze_page $p(tag)
  }
}

#-----------------------------------------------------------------------
# The following function is called to post a menu of options for modifying
# or adding a field
#
# Input:
#  page        The parent page.
#  rx ry       The root-window location under which the field is located.
#-----------------------------------------------------------------------
proc page::read_field_menu {page rx ry} {
  upvar #0 $page p    ;# The configuration array of the page.
#
# Get the row and column index under the cursor.
#
  root_to_page $page $rx $ry col row
#
# Is their a field under the cursor?
#
  if {[info exists p($col,$row)]} {
#
# Get the occupying field widget and its configuration array.
#
    set field $p($col,$row)
    upvar #0 $field f
#
# Post and read the field-modification menu.
#
    read_menu .field_menu $rx $ry field_menu_var
    switch $::field_menu_var {
      delete {
	delete_field $page $col $row
	if {[monitor have_stream]} {monitor reconfigure}
      }
      edit {
	show_field_dialog $f(page) $col $row 0
      }
      save {
	::page::adopt_field_pars $field
      }
    }
#
# If no field currently exists, post the field-addition menu.
#
  } else {
#
# If col < 0 it means that rx was left of column 0, so insert a
# new column 0 there.
#
    if {$col < 0} {
      insert_col_after $page -1
      set col 0
    }
#
# If row < 0 it means that ry was above row 0, so insert a new row
# 0 there.
#
    if {$row < 0} {
      insert_row_after $page -1
      set row 0
    }

#
# Post the field-type selection menu and wait for the users response.
#
    read_menu .field_type_menu $rx $ry field_type_menu_var
    if {[string length $::field_type_menu_var] > 0} {
      add_$::field_type_menu_var $page $col $row
      show_field_dialog $page $col $row 1
    }
  }
}

#-----------------------------------------------------------------------
# The following function is called to post a menu of options for inserting
# and deleting rows and columns.
#
# Input:
#  page        The parent page.
#  rx ry       The root-window location under which the field is located.
#-----------------------------------------------------------------------
proc page::read_page_col_row_menu {page rx ry} {
#
# Get the row and column index under the cursor.
#
  root_to_page $page $rx $ry col row
#
# Post and read the menu.
#
  read_menu .page_col_row_menu $rx $ry page_col_row_menu_var
  switch $::page_col_row_menu_var {
    delete_row {
      delete_row $page $row
      if {[monitor have_stream]} {monitor reconfigure}
    }
    delete_col {
      delete_col $page $col
      if {[monitor have_stream]} {monitor reconfigure}
    }
    fit_col_width {
      $page.grid column $col -width ?
    }
    set_col_width {
      query_column_width $page $col
    }
    insert_row_above {
      insert_row_after $page [expr {$row - 1}]
    }
    insert_row_below {
      insert_row_after $page $row
    }
    insert_col_left {
      insert_col_after $page [expr {$col - 1}]
    }
    insert_col_right {
      insert_col_after $page $col
    }
  }
}

#-----------------------------------------------------------------------
# Create the menu bar of a page widget.
#
# Input:
#  w          The path name to give the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc page::create_page_menubar {w} {
#
# Create a raised frame for the menubar.
#
  frame $w -relief raised -bd 2 -width 15c
#
# Get the parent page.
#
  set page [winfo toplevel $w]
#
# Create the file menu.
#
  menubutton $w.file -text File -menu $w.file.menu
  set m [menu $w.file.menu -tearoff 0]
#
# By convention the quit entry sits at the end of the menu.
#
  set save_dialog [save_config_dialog $page]
  $m add command -label {Save Page Configuration} \
      -command "wm deiconify $save_dialog; raise $save_dialog"
  $m add separator
  $m add command -label {Quit   [Meta+q]} -command "::page::quit_page $page"
#
# Pack to the left of the menu bar.
#
  pack $w.file -side left
#
# Create the configuration menu.
#
  menubutton $w.config -text Configure -menu $w.config.menu
  set m [menu $w.config.menu -tearoff 0]
#
# Create its members.
#
  $m add command -label {Modify page} -command "page::show_page_dialog $page"
  $m add command -label {Shrink-wrap page} -command "::page::shrink_wrap $page"
  pack $w.config -side left

#
# Create the find menu.
#
  menubutton $w.find -text Find -menu $w.find.menu
  set m [menu $w.find.menu -tearoff 0]
#
# Create its member(s)
#
  $m add command -label {Find gcpViewer} -command "reveal ."
  pack $w.find -side left

#
# Create the help menu.
#
  menubutton $w.help -text Help -menu $w.help.menu
  set m [menu $w.help.menu -tearoff 0]
  $m add command -label {Pages in general} -command "show_help gcpViewer/page/index"
  $m add command -label {Adding labels} -command "show_help gcpViewer/page/label"
  $m add command -label {Adding register display fields} -command "show_help gcpViewer/page/reg"
  $m add command -label {Moving fields with the mouse} -command "show_help gcpViewer/page/drag"
  $m add command -label {Inserting, resizing and deleting columns} -command "show_help gcpViewer/page/col"
  $m add command -label {Inserting and deleting rows} -command "show_help gcpViewer/page/row"
  $m add command -label {Creating a single page for a regMapGroup of similar boards} -command "show_help gcpViewer/page/regMapGroup"
  $m add command -label {Changing the characteristics of a page} -command "show_help gcpViewer/page/gen"
  $m add command -label {Shrink wrapping a page} -command "show_help gcpViewer/page/shrink"
  $m add command -label {Saving the configuration of a page} -command "show_help gcpViewer/page/save"
  pack $w.help -side right
  return $w
}

#-----------------------------------------------------------------------
# Create a frame for a row of buttons. If the user specifies the prefix
# of a regMapGroup of numbered boards in the page configuration dialog, the
# one button per board that matches the prefix will be placed in this
# frame to enable the user to switch between boards. When no prefix has
# been specified the frame will be left unmapped.
#
# Input:
#  page       The path name of the page widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc page::create_page_regMapGroup {page} {
#
# Construct the new widget name.
#
  set w $page.regMapGroup
  return [frame $w -bd 2 -relief ridge]
}

#-----------------------------------------------------------------------
# This is the trace-variable callback for the board-selection radiobuttons.
#
# Input:
#  page   The parent page widget.
#  args   Irrelevant arguments added by the trace command.
#-----------------------------------------------------------------------
proc page::regMapGroup_button_callback {page args} {
  upvar #0 $page p

  set grid $page.grid  ;# The panel widget.

    # NB: Using list [wm ...] below puts {} around the page name,
    # which gets sequentially tacked on to the name each time the
    # user presses a button!

# configure_page $page [list [wm title $page]]

    set boardNumber 0

    if [info exists ::$page.boardGroup] {
	set boardNumber [set ::$page.boardGroup]
     }

 configure_page $page [wm title $page] \
      [list [$grid cget -bg]] [list [$grid cget -warn_color]] \
      [list $p(dopage_color)] \
      [$grid cget -column_width] [string trimleft [$grid cget -font] {font}] \
      [list $p(regMapGroup)] [set ::$page.regMapGroup] \
      $p(boardGroup) $boardNumber
  if {[catch {
    monitor reconfigure
  } result]} {
    report_error $result
  }
}

#-----------------------------------------------------------------------
# Create a frame for a row of buttons. If the user specifies the prefix
# of a regMapGroup of numbered boards in the page configuration dialog, the
# one button per board that matches the prefix will be placed in this
# frame to enable the user to switch between boards. When no prefix has
# been specified the frame will be left unmapped.
#
# Input:
#  page       The path name of the page widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc page::create_page_boardGroup {page} {
#
# Construct the new widget name.
#
  set w $page.boardGroup
  return [frame $w -bd 2 -relief ridge]
}

#-----------------------------------------------------------------------
# This is the trace-variable callback for the board-selection radiobuttons.
#
# Input:
#  page   The parent page widget.
#  args   Irrelevant arguments added by the trace command.
#-----------------------------------------------------------------------
proc page::boardGroup_button_callback {page args} {
  upvar #0 $page p

  set grid $page.grid  ;# The panel widget.

    # NB: Using list [wm ...] below puts {} around the page name,
    # which gets sequentially tacked on to the name each time the
    # user presses a button!

# configure_page $page [list [wm title $page]]

    set regMapNumber 0

    if [info exists ::$page.regMapGroup] {
	set regMapNumber [set ::$page.regMapGroup]
     }

 configure_page $page [wm title $page] \
      [list [$grid cget -bg]] [list [$grid cget -warn_color]] \
      [list $p(dopage_color)] \
      [$grid cget -column_width] [string trimleft [$grid cget -font] {font}] \
      $p(regMapGroup) $regMapNumber \
      [list $p(boardGroup)] [set ::$page.boardGroup]
  if {[catch {
    monitor reconfigure
  } result]} {
    report_error $result
  }
}

#-----------------------------------------------------------------------
# Create the area of the page widget that contains the page grid
# and associated controls.
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc page::create_page_panel_widget {w} {
  panel $w -array $w -bd 0 -relief flat
  return $w
}

#-----------------------------------------------------------------------
# Delete a page and reconfigure the C viewer to match.
#-----------------------------------------------------------------------
proc page::quit_page {page} {
  delete_page $page
  if [monitor have_stream] {
    monitor reconfigure
  }
}

#-----------------------------------------------------------------------
# Create a page configuration dialog.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc page::create_page_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Page} gcpViewer/page/gen
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::page_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::page_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::page_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Create a labelled entry widget for specifying the page title.
#
  labeled_entry $w.title {Page title: } {} -width 16
  $w.title.l configure -fg TextLabelColor
#
# Create a labelled entry widget for specifying the page color.
#
  labeled_entry $w.bg {Page Color: } {} -width 16
  $w.bg.l configure -fg TextLabelColor
#
# Create a labelled option menu for specifying the font size.
#
  option_menu $w.font_size {Font size: } $::page::font_size_choices {}
  $w.font_size.l configure -fg TextLabelColor
#
# Allow specification of the color used to highlight out-of-range values.
#
  labeled_entry $w.warn_color {Warning Color: } {} -width 16
  $w.warn_color.l configure -fg TextLabelColor
#
# Allow specification of the color used to highlight paged values
#
  labeled_entry $w.dopage_color {Paging Color: } {} -width 16
  $w.dopage_color.l configure -fg TextLabelColor
#
# Allow specification of the default column width.
#
  labeled_entry $w.def_width {Default Column Width: } {} -width 16
  $w.def_width.l configure -fg TextLabelColor
#
# Allow specification of the textual prefix of a set of related
# regmaps, such that a row of buttons used to choose which register map
# to display, can be created.
#
  labeled_entry $w.regMapGroup "Enable switching between\n numbered register maps whose\n names all start with: " {} -width 16
  $w.regMapGroup.l configure -fg TextLabelColor

#
# Allow specification of the textual prefix of a set of related
# boards, such that a row of buttons used to choose which register map
# to display, can be created.
#
  labeled_entry $w.boardGroup "Enable switching between\n numbered boards whose\n names all start with: " {} -width 16
  $w.boardGroup.l configure -fg TextLabelColor

#
# Arrange the widgets one below the other.
#
  pack $w.title $w.bg $w.font_size $w.warn_color $w.dopage_color $w.def_width \
	  $w.regMapGroup $w.boardGroup \
      -side top -anchor nw -fill x -pady 1m -padx 1m
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# Create a column configuration dialog.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc page::create_column_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Column} gcpViewer/page/col
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::column_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::column_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::column_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Create a labelled entry widget for specifying the column width.
#
  labeled_entry $w.width {Column width: } {} -width 16
  $w.width.l configure -fg TextLabelColor
#
# Arrange the widgets one below the other.
#
  pack $w.width -side top -anchor nw -fill x -pady 1m -padx 1m
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# Shrink-wrap a page widget to discard unoccupied rows and columns on
# the periphery of the page, and resize the widget to just enclose the
# remaining rows and columns.
#
# Input:
#  page      The page widget to shrink-wrap.
#-----------------------------------------------------------------------
proc page::shrink_wrap {page} {
  $page.grid shrink
  wm geometry $page {}
}

#-----------------------------------------------------------------------
# Create a field configuration dialog.
#
# Input:
#  dialog    The path name to give to the dialog.
#  type      The type of field that the dialog configures, from:
#              label    -   A text label.
#              register -   A register display field.
# Output:
#  return    The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc page::create_field_dialog {dialog type} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Field} gcpViewer/page/$type
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::field_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::field_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::field_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Create labelled entry widgets for specifying the foreground and
# background colors of the field.
#
  labeled_entry $w.bg {Background Color: } {} -width 16
  $w.bg.l configure -fg TextLabelColor
  labeled_entry $w.fg {Foreground Color: } {} -width 16
  $w.fg.l configure -fg TextLabelColor
#
# Create a labelled option menu for specifying the justification.
#
  option_menu $w.justify {Justify: } $::page::justify_names {}
  $w.justify.l configure -fg TextLabelColor
#
# Position the shared items.
#
  pack $w.bg $w.fg $w.justify -side top -anchor nw -fill x -pady 1m -padx 1m
#
# Add type-specific configuration items.
#
  switch $type {
    label {
      labeled_entry $w.text {The text of the label: } {} -width 16
      $w.text.l configure -fg TextLabelColor
      pack $w.text -side top -anchor nw -fill x -pady 1m -padx 1m
    }
    register {
#
# Create a labelled entry widget and a register menu for selecting
# the register element to be displayed.
#
      set reg [labeled_entry $w.reg {Register: } {} -width 16]
      $reg.l configure -fg TextLabelColor
      create_regmenu_button $reg.m $reg.e
      pack $reg.m -side left -padx 2m
      bind $reg.e <Return> "$dialog.bot.apply invoke"

#
# Create a menu of display formats.
#
      option_menu $w.format {Display format: } $::page::format_names {}
      $w.format.l configure -fg TextLabelColor
#
# Allow the user to specify whether to allocate space for a sign
# character.
#
      checkbutton $w.signed -variable $w.signed -anchor w \
	  -text {Allocate space for sign character}
#
# Allow entry of the precision attribute. The label will be set
# dynamically according to its interpretation by the selected format.
#
      labeled_entry $w.prec {} {} -width 5
      $w.prec.l configure -fg TextLabelColor
#
# Allow entry of the miscellaneous width attribute. The label will be
# dynamically set to accord with its interpretation by the selected format.
#
      labeled_entry $w.misc {} {} -width 5
      $w.misc.l configure -fg TextLabelColor
#
# Allow the entry of enumeration names.
#
      labeled_entry $w.enum {} {} -width 5
      $w.enum.l configure -fg TextLabelColor
#
# Allow the user to request field-value range checking.
#
      checkbutton $w.warn -variable $w.warn -anchor w \
	  -text {Highlight field values that are out of range}
#
# Allow entry of the optional field-value range.
#
      set rng [frame $w.range]
      label $rng.lmin -text {Range from: } -fg TextLabelColor
      entry $rng.min -width 10
      label $rng.lmax -text {to}
      entry $rng.max -width 10
      pack $rng.lmin $rng.min $rng.lmax $rng.max -side left

#
# Allow the user to request paging
#
      checkbutton $w.dopage -variable $w.dopage -anchor w \
	  -text {Activate pager if value goes out of range}

#
# Allow entry of the optional field-value range.
#
      set fpg [frame $w.fpage]
      label $fpg.lnframe -text {Threshold (frames): } -fg TextLabelColor
      entry $fpg.nframe -width 10
      pack $fpg.lnframe $fpg.nframe -side left

#
# Arrange the widgets top to bottom.
#
      pack $w.reg $w.format $w.signed $w.prec $w.misc $w.enum $w.warn \
	  $rng $w.dopage $fpg -side top -anchor nw -fill x -pady 1m -padx 1m
#
# Set a variable trace on the format option menu. This will allow us to
# set the labels and sensitivity state of attribute entry fields with
# format-specific semantics.
#
      trace variable ::$w.format w [list page::relabel_register_attributes $w]
    }
  }
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# This is a private trace variable callback for the format-specification
# option menu of the field configuration dialog. It changes the labels
# associated with format-specific entry fields according to the chosen
# format.
#
# Input:
#  w      The work area frame of the register field configuration dialog.
#  args   Irrelevant arguments added by the trace command.
#-----------------------------------------------------------------------
proc page::relabel_register_attributes {w args} {
  upvar #0 $w.format format
#
# The precision attribute means different things under different formats.
# Get a description to use as its label.
#
  switch -regexp -- $format {
    (fixed_point|scientific|sexagesimal) {set text {Decimal places:}}
    floating {set text {Significant figures:}}
    (integer|hex|octal|binary) {set text {Zero padding width:}}
    string {set text {Maximum width:}}
    (date|bit|enum) {set text {}}
    default {set text {The precision: }}
  }
#
# Reconfigure the label of the precision entry field.
#
  if {$text == {}} {
    $w.prec.l configure -fg steelblue -text {(Unused)}
    $w.prec.e configure -state disabled -fg grey
    replace_entry_contents $w.prec.e {0}
  } else {
    $w.prec.l configure -fg TextLabelColor -text $text
    $w.prec.e configure -state normal -fg black
  }
#
# The misc attribute is only used with a couple of formats.
# Get the description of its function, or an empty string if
# it isn't used.
#
  switch -regexp -- $format {
    sexagesimal {set text {The hours field width:}}
    bit {set text {A bit number 0..31:}}
    default {set text {}}
  }
#
# Configure the entry field and its label according to its function.
#
  if {$text == {}} {
    replace_entry_contents $w.misc.e {0}
    $w.misc.e configure -state disabled -fg grey
    $w.misc.l configure -fg steelblue -text {(Unused)}
  } else {
    $w.misc.e configure -state normal -fg black
    $w.misc.l configure -fg TextLabelColor -text $text
  }
#
# The enum attribute is only used with a couple of formats.
# Get the description of its function, or an empty string if
# it isn't used.
#
  switch -regexp -- $format {
    enum {set text {Names for 0,1,2.. :}}
    bit {set text {Names for values 0,1:}}
    default {set text {}}
  }
#
# Configure the entry field and its label according to its function.
#
  if {$text == {}} {
    replace_entry_contents $w.enum.e {}
    $w.enum.e configure -state disabled -fg grey
    $w.enum.l configure -fg steelblue -text {(Unused)}
  } else {
    $w.enum.e configure -state normal -fg black
    $w.enum.l configure -fg TextLabelColor -text $text
  }
}

#-----------------------------------------------------------------------
# This is a private trace variable callback used to control highlighting
# of a register field.
#
# Input:
#  page     The parent page widget.
#  field    The register field to update.
#  args     Irrelevant arguments added by the trace command.
#-----------------------------------------------------------------------
proc update_pager_state {page field args} {
  upvar #0 $field f
  upvar #0 $page p

# If the global pager flag has been set and we are currently allowing pages

# puts "$f(register) was out of range"

 if {$f(pflag)} {
   if {$::allow_paging} {
       control send "pager on, register=$f(register)"
   }

# Re-display the cell with the pager color

   $page.grid $f(col),$f(row) -bg $p(dopage_color) -flag $f(pflag)

 } else {

# Else redisplay with the ok color

     $page.grid $f(col),$f(row) -bg $f(bg) -flag $f(flag)
  }
}

#-----------------------------------------------------------------------
# This is a private trace variable callback used to control highlighting
# of a register field.
#
# Input:
#  page     The parent page widget.
#  field    The register field to update.
#  args     Irrelevant arguments added by the trace command.
#-----------------------------------------------------------------------
proc update_warn_border {page field args} {
  upvar #0 $field f

  if {$f(warn)} {
     $page.grid $f(col),$f(row) -flag $f(flag)
  }
}
#-----------------------------------------------------------------------
# This is a private trace variable callback used to control highlighting
# of a register field.
#
# Input:
#  page     The parent page widget.
#  field    The register field to update.
#  args     Irrelevant arguments added by the trace command.
#-----------------------------------------------------------------------
#proc update_warn_border {page field args} {
#  upvar #0 $field f
#
#  $page.grid $f(col),$f(row) -flag 1
#
#  if {$f(warn)} {
#      $page.grid configure -warn_color red 
#  }
#  else {
#      $page.grid configure -warn_color yellow
#  }
#}
#
#-----------------------------------------------------------------------
# Delete a given field from a page.
#
# Input:
#  page      The parent page.
#  col row   The field location in the grid (column,row).
#-----------------------------------------------------------------------
proc page::delete_field {page col row} {
  upvar #0 $page p         ;# The page configuration array.
#
# Already deleted?
#
  if {![info exists p($col,$row)]} {
    return
  }
#
# Get the field widget and its configuration array.
#
  set field $p($col,$row)
  upvar #0 $field f
#
# Delete the C version of the field.
#
  if {[info exists f(tag)]} {
    if {$f(tag)} {
      monitor remove_field $p(tag) $f(tag)
    }
  }
#
# Remove the configuration array of the field from the array of fields
# in its parent page.
#
  unset p($col,$row)
#
# Remove the field from the grid.
#
  $page.grid delete $col,$row
#
# Delete the field configuration array.
#
  if [info exists ::$field] {
    unset ::$field
  }
}

#-----------------------------------------------------------------------
# Make the attributes of a given field the initial defaults of the
# next field to be created.
#-----------------------------------------------------------------------
proc page::adopt_field_pars {field} {
  upvar #0 $field f
  upvar #0 $f(page) p
  switch $f(type) {
    label {
      foreach item {fg bg justify text} {
	set p(label_$item) $f($item)
      }
    }
    register {
      foreach item {fg bg justify register format precision signed misc enum warn min max dopage nframe} {
	set p(register_$item) $f($item)
      }
    }
  }
}

#-----------------------------------------------------------------------
# Check the compatibility between a given misc field attribute
# and the corresponding format attribute. An error will be thrown if
# there is a problem.
#
# Input:
#  misc     The misc attribute to be checked.
#  format   A valid format to check against.
#-----------------------------------------------------------------------
proc page::check_misc_attribute {misc format} {
#
# The misc attribute has different meanings to different formats.
#
  switch -- $format {
    sexagesimal {
      if {![is_uint $misc]} {
	error "The misc field attribute must be an integer >= 0"
      }
    }
    bit {
      set is_bad 1
      if {[is_uint $misc]} {
	if {$misc >= 0 && $misc <= 31} {
	  set is_bad 0
	}
      }
      if {$is_bad} {
	error "The bit number field attribute must be in the range 0..31"
      }
    }
  }
}

#-----------------------------------------------------------------------
# Return the grid column and row indexes that correspond to a given
# root-window position.
#
# Input:
#  page        The gridded page widget.
#  rx ry       The root window coordinates.
# Input/Ouput:
#  col row     The names of variables in the callers stack frame, into
#              which the row and column indexes will be placed.
#-----------------------------------------------------------------------
proc page::root_to_page {page rx ry col row} {
  upvar 1 $col c
  upvar 1 $row r
#
# Convert the root-window coordinates to page coordinates.
#
  set px [expr {$rx - [winfo rootx $page.grid]}]
  set py [expr {$ry - [winfo rooty $page.grid]}]
#
# Convert the x,y location to row and column indexes in the current grid.
#
  set cr [$page.grid location $px $py]
  set c [lindex $cr 0]
  set r [lindex $cr 1]
}

#-----------------------------------------------------------------------
# Place a field at a new position in the grid. If the field has previously
# been on the grid, then it is the callers responsibility to remove it
# from the grid and from the 2-d field array before making this call.
#
# If the target cell is already occupied, the occupant will be removed
# and destroyed.
#
# Input:
#  page     The gridded page widget.
#  field    The field widget to be placed on the grid.
#  col row  The position in the grid at which to place the field.
#-----------------------------------------------------------------------
proc page::grid_field {page field col row} {
  upvar #0 $field f
  upvar #0 $page p
  upvar #0 $page.grid g
#
# Redundant operation?
#
  if {[info exists p($col,$row)] && [string compare $p($col,$row) $field]==0} {
    return
  }
#
# If the destination cell is already occupied, delete its occupant.
#
  delete_field $page $col $row
#
# Place the field on the grid.
#
#  $page.grid $col,$row -bg $f(bg) -fg $f(fg) -pack $f(justify) -flag $f(flag)
  set g($col,$row) {}
#
# Change the variable into which the viewer writes new field values,
# such that the new values end up in the correct cell of the grid.
#
  if {[info exists f(tag)]} {
    if {$f(tag)} {
      monitor field_variables $p(tag) $f(tag) $page.grid\($col,$row\) \
	  $field\(flag\) $field\(pflag\)
    }
  }
  set p($col,$row) $field
  set f(col) $col
  set f(row) $row
#
# If this is a register field, make sure that the warning border reflects
# the current status of the register. 
#
  if {[string compare $f(type) register] == 0} {
    if {$f(flag) && $f(warn)} {
      $page.grid $f(col),$f(row) -flag on
    } else {
      $page.grid $f(col),$f(row) -flag off
    }
  } else {
    $page.grid $f(col),$f(row) -flag off
  }
}

#-----------------------------------------------------------------------
# Move a field from one location in the grid to another.
#
# Input:
#  page              The gridded page widget.
#  src_col src_row   The column and row indexes of the cell to be moved.
#  dst_col dst_row   The column and row indexes of the destination cell.
#-----------------------------------------------------------------------
proc page::move_field {page src_col src_row dst_col dst_row} {
  upvar #0 $page p
  upvar #0 $page.grid g
#
# Is there a field to be moved?
#
  if {[info exists p($src_col,$src_row)]} {
#
# Get the field configuration variable.
#
    set field $p($src_col,$src_row)
    upvar #0 $field f
#
# Get the current contents of the field.
#
    set value $g($src_col,$src_row)
#
# Remove the field from its original position in the grid.
#
    unset p($src_col,$src_row)
    $page.grid delete $src_col,$src_row
#
# Copy the field to its new location.
#
    grid_field $page $field $dst_col $dst_row
#
# Reconfigure the field.
#
    switch $f(type) {
      label {
	configure_label $field $f(bg) $f(fg) $f(justify) $f(text)
      }
      register {
	configure_register $field $f(bg) $f(fg) $f(justify) $f(register) \
	    $f(format) $f(signed) $f(precision) $f(misc) $f(enum) \
	    $f(warn) $f(min) $f(max) $f(dopage) $f(nframe)
	set g($dst_col,$dst_row) $value
      }
    }
  } else {
    delete_field $page $dst_col $dst_row
  }
}

#-----------------------------------------------------------------------
# Discard any previous field selection then place the anchor of a new
# selection in the cell that lies under a given root-window position.
# Subsequent calls to drag_field will be needed to extend the selection.
#
# Input:
#  page      The gridded page widget who's selection is to be started.
#  rx ry     The root window position at which to anchor the new
#            selection.
#-----------------------------------------------------------------------
proc page::select_fields {page rx ry} {
  upvar #0 $page p
#
# Ignore calls to this function while start_drag is waiting
# for the drag window to become visible.
#
  if {[winfo exists .drag]} {
    return
  }
#
# Get the row and column index under the cursor.
#
  root_to_page $page $rx $ry col row
#
# If there is a selection active and the cursor is over the first cell
# of that selection, initiate a drag.
#
  if {($col == $p(scol) && $row == $p(srow)) && \
      ($p(scol) != $p(ecol) || $p(srow) != $p(erow))} {
    start_drag $page $rx $ry
    return
  }
#
# Discard any previous selection.
#
  unselect_fields $page
#
# For the moment the selection has zero extent so the start and end cells
# are the same.
#
  set p(scol) $col
  set p(srow) $row
  set p(ecol) $col
  set p(erow) $row
#
# Acquire the primary selection and arrange for the current selection
# to be discarded if another selection is started in any window.
#
  selection own -command [list page::unselect_fields $page] $page
}

#-----------------------------------------------------------------------
# Initiate a drag and drop operation.
#
# Input:
#  page      The gridded page widget.
#  rx ry     The root window position at which the drag was initiated.
#-----------------------------------------------------------------------
proc page::start_drag {page rx ry} {
  upvar #0 $page p     ;# The configuration array of the page.
  set grid $page.grid  ;# The panel widget.
  upvar #0 $grid g     ;# The grid text-variable array.
  set drag .drag.grid  ;# The name of the panel widget in the drag window.
  upvar #0 $drag d     ;# The drag-grid text-variable array.
#
# Create the undecorated toplevel widget that will display the fields
# that are being dragged.
#
  toplevel .drag
  wm overrideredirect .drag 1
#
# Place a temporary panel widget in the toplevel.
#
  panel $drag -array $drag -bd 0 -relief flat -font [$grid cget -font]
  pack $drag -fill both
#
# Compute the number of selected rows and columns.
#
  switch -- $p(select_dir) {
    h {
      set ncol [expr {$p(ecol) - $p(scol)}]
      set nrow [expr {$p(erow) - $p(srow) + 1}]
    }
    v {
      set ncol [expr {$p(ecol) - $p(scol) + 1}]
      set nrow [expr {$p(erow) - $p(srow)}]
    }
  }
#
# Configure the drag grid to look the same as the fields that are being
# dragged.
#
  $drag configure -bg [$grid cget -bg] -font [$grid cget -font]
  for {set col 0} {$col < $ncol} {incr col} {
    set src_col [expr {$p(scol) + $col}]
    $drag column $col -width [$grid column $src_col -width]
    for {set row 0} {$row < $nrow} {incr row} {
      set src_row [expr {$p(srow) + $row}]
      set src_cell $src_col,$src_row
      if {[info exists g($src_cell)]} {
	set d($col,$row) $g($src_cell)
	$drag $col,$row -fg [$grid $src_cell -fg] -bg [$grid $src_cell -bg] \
	    -flag [$grid $src_cell -flag] -pack [$grid $src_cell -pack]
      }
    }
  }
#
# Work out the offset into the toplevel widget of the center of the
# first field.
#
  set xywh [$drag bbox 0 0]
  set wx [expr {[lindex $xywh 2] / 2}]
  set wy [expr {[lindex $xywh 3] / 2}]
#
# Place the center of the first field over the cursor.
#
  wm geometry .drag +[expr $rx - $wx]+[expr $ry - $wy]
  tkwait visibility $drag
#
# Demand exclusive access to mouse events until the operation is completed.
#
  if [catch {grab -global .drag} result] {
    report_error "Unable to grab X server: $result"
    destroy .drag
    return
  }
#
# While the cursor is ours slave the position of the toplevel window to
# the position of the cursor.
#
  bind .drag <Motion> "wm geometry .drag +\[expr %X - $wx\]+\[expr %Y - $wy\]"
#
# Bind button events to drop the widget.
#
  bind .drag <ButtonRelease> "::page::drop_fields $page %X %Y"
}

#-----------------------------------------------------------------------
# If a previously made selection is being dragged, and the cursor
# is currently over a valid grid cell, paste the selected fields there
# and deactivate the drag.
#
# Input:
#  page     The gridded page widget.
#  rx ry    The world-coordinate position at which to drop the fields.
#-----------------------------------------------------------------------
proc page::drop_fields {page rx ry} {
  upvar #0 $page p
#
# Release the grab on the X-server and delete the drag window.
#
  grab release .drag
  destroy .drag
#
# Get the row and column index under the cursor.
#
  root_to_page $page $rx $ry col row
#
# If the row and column indexes are on the grid, paste the selection there.
#
  grid_size $page cols rows
  if {$col >= 0 && $row >= 0 && $row < $rows && $col < $cols} {
    paste_fields $page $rx $ry
  }
#
# In pasting over existing cells we may have deleted some register fields,
# so appraise the C layer of this.
#
  if {[monitor have_stream]} {
    monitor reconfigure
  }
}

#-----------------------------------------------------------------------
# Deactivate any current field selection.
#
# Input:
#  page     The gridded page widget.
#-----------------------------------------------------------------------
proc page::unselect_fields {page} {
  upvar #0 $page p
#
# If a previous selection had been made clear it by making the end of
# the selected range the same as its start.
#
  switch -- $p(select_dir) {
    h {extend_row_selection $page $p(scol)}
    v {extend_col_selection $page $p(srow)}
  }
  reset_selection_vars $page
}

#-----------------------------------------------------------------------
# Reset the selection-status variables to mark the selection as inactive.
# You should call unselect_fields instead if there are any visibly
# selected fields that need to be returned to their normal colors.
#
# Input:
#  page     the gridded page widget.
#-----------------------------------------------------------------------
proc page::reset_selection_vars {page} {
  upvar #0 $page p
  set p(select_dir) none
  set p(selection_text) {}
  set p(ecol) 0
  set p(scol) 0
  set p(srow) 0
  set p(erow) 0
}

#-----------------------------------------------------------------------
# Extend the currently selected row or column of fields to the cell that
# lies under a given root-window position.
#
# Input:
#  page     The gridded page widget.
#  rx ry    The root window position of the new end point of the
#           selection.
#-----------------------------------------------------------------------
proc page::extend_field_selection {page rx ry} {
  upvar #0 $page p
#
# Ignore calls to this function while start_drag is waiting
# for the drag window to become visible.
#
  if {[winfo exists .drag]} {
    return
  }
#
# Get the row and column index under the cursor.
#
  root_to_page $page $rx $ry col row
#
# If nothing has been selected, use the direction moved to determine the
# direction of the selection (vertical or horizontal).
#
  if {$p(ecol) == $p(scol) && $p(erow) == $p(srow)} {
    if {$col != $p(scol)} {
      set p(select_dir) h
    } elseif {$row != $p(srow)} {
      set p(select_dir) v
    } else {
      return
    }
  }
#
# Update the hightlighted row or column to reflect the new position of
# the cursor.
#
  switch -- $p(select_dir) {
    h {extend_row_selection $page $col}
    v {extend_col_selection $page $row}
  }
}

#-----------------------------------------------------------------------
# Extend a previously started selection along the fields of a row.
#
# Input:
#  page     The gridded page widget.
#  ecol     The new end column.
#-----------------------------------------------------------------------
proc page::extend_row_selection {page ecol} {
  upvar #0 $page p     ;# The configuration array of the page.
#
# Don't allow the end position to go left of the start column.
#
  if {$ecol < $p(scol)} {
    set ecol $p(scol)
  }
#
# Toggle the selection status of the fields that have been newly selected
# or unselected.
#
  if {$ecol > $p(ecol)} {
    for {set col $p(ecol)} {$col < $ecol} {incr col} {
      toggle_field_selection $page $col $p(srow)
    }
  } elseif {$ecol < $p(ecol)} {
    for {set col $ecol} {$col < $p(ecol)} {incr col} {
      toggle_field_selection $page $col $p(srow)
    }
  }
#
# Record the new end column of the selection.
#
  set p(ecol) $ecol
}

#-----------------------------------------------------------------------
# Extend a previously started selection down the fields of a column.
#
# Input:
#  page     The gridded page widget.
#  erow     The new end row.
#-----------------------------------------------------------------------
proc page::extend_col_selection {page erow} {
  upvar #0 $page p     ;# The configuration array of the page.
#
# Don't allow the end position to go above the start column.
#
  if {$erow < $p(srow)} {
    set erow $p(srow)
  }
#
# Toggle the selection status of the fields that have been newly selected
# or unselected.
#
  if {$erow > $p(erow)} {
    for {set row $p(erow)} {$row < $erow} {incr row} {
      toggle_field_selection $page $p(scol) $row
    }
  } elseif {$erow < $p(erow)} {
    for {set row $erow} {$row < $p(erow)} {incr row} {
      toggle_field_selection $page $p(scol) $row
    }
  }
#
# Record the new end rowumn of the selection.
#
  set p(erow) $erow
}

#-----------------------------------------------------------------------
# Change the highlight appearance of a widget by reversing it foreground
# and background colors.
#
# Input:
#  page      The gridded page widget that contains the field.
#  col row   The row and column indexes of the field to be toggled.
#-----------------------------------------------------------------------
proc page::toggle_field_selection {page col row} {
  set grid $page.grid  ;# The panel widget.
  upvar #0 $grid g     ;# The panel-widget text-variable array.
  set cell $col,$row   ;# The grid coordinate of the field.
#
# If the grid cell is occupied, swap its background and foreground
# colors.
#
  if {[info exists g($cell)]} {
    $grid $cell -bg [$grid $cell -fg] -fg [$grid $cell -bg]
  }
}

#-----------------------------------------------------------------------
# Paste the currently selected fields, placing the first at the grid
# cell that lies under root-window coordinates rx,ry.
#
# Input:
#  page     The gridded page widget to paste from/to.
#  rx ry    The root-window coordinates at which to start pasting.
#-----------------------------------------------------------------------
proc page::paste_fields {page rx ry} {
  upvar #0 $page p    ;# The page configuration array.
#
# Nothing to paste?
#
  if {[string compare $p(select_dir) none] == 0} return
#
# Get the row and column index under the cursor.
#
  root_to_page $page $rx $ry dst_col dst_row
#
# Paste horizontally or vertically.
#
  switch -- $p(select_dir) {
    h {paste_row $page $dst_col $dst_row}
    v {paste_col $page $dst_col $dst_row}
  }
}

#-----------------------------------------------------------------------
# Move the selected fields of the currently selected row to a new position
# in the grid.
#
# Input:
#  page      The gridded page widget.
#  dst_col   The destination column at which to start pasting.
#  dst_row   The destination row into which to paste.
#-----------------------------------------------------------------------
proc page::paste_row {page dst_col dst_row} {
  upvar #0 $page p    ;# The page configuration array.
#
# Get the start and end column indexes (assumed to be in ascending order),
# and the row of the selected range.
#
  set src_scol $p(scol)
  set src_ecol $p(ecol)
  set src_row $p(srow)
#
# No movement?
#
  if {$src_scol == $dst_col && $src_row == $dst_row} {
    return
  }
#
# Compute the number of columns to be copied.
# Note that src_ecol points to the field that follows the last selected field.
#
  set ncol [expr {$src_ecol - $src_scol}]
#
# Move the selected fields to the specified destination, being aware
# of the possibility that the source and destination regions could overlap.
#
  if {$src_scol < $dst_col} {
    for {set col [expr {$ncol - 1}]} {$col >= 0} {incr col -1} {
      move_field $page [expr {$src_scol + $col}] $src_row \
	               [expr {$dst_col + $col}] $dst_row
    }
  } else {
    for {set col 0} {$col < $ncol} {incr col} {
      move_field $page [expr {$src_scol + $col}] $src_row \
	               [expr {$dst_col + $col}] $dst_row
    }
  }
#
# The fields that were highlighted as selected no longer exist, so
# mark the selection as inactive.
#
  reset_selection_vars $page
}

#-----------------------------------------------------------------------
# Move the selected fields of the currently selected column to a new
# position in the grid.
#
# Input:
#  page      The gridded page widget.
#  dst_col   The destination column into which to paste.
#  dst_row   The destination row at which to start pasting.
#-----------------------------------------------------------------------
proc page::paste_col {page dst_col dst_row} {
  upvar #0 $page p    ;# The page configuration array.
#
# Get the start and end row indexes (assumed to be in ascending order),
# and the row of the selected range.
#
  set src_srow $p(srow)
  set src_erow $p(erow)
  set src_col $p(scol)
#
# Compute the index of the end row, relative to the start row.
# Note that src_erow points to the field that follows the last selected field.
#
  set nrow [expr {$src_erow - $src_srow}]
#
# Move the selected fields to the specified destination, being aware
# of the possibility that the source and destination regions could overlap.
#
  if {$src_srow < $dst_row} {
    for {set row [expr {$nrow - 1}]} {$row >= 0} {incr row -1} {
      move_field $page $src_col [expr {$src_srow + $row}] \
	               $dst_col [expr {$dst_row + $row}]
    }
  } else {
    for {set row 0} {$row < $nrow} {incr row} {
      move_field $page $src_col [expr {$src_srow + $row}] \
	               $dst_col [expr {$dst_row + $row}]
    }
  }
#
# The fields that were highlighted as selected no longer exist, so
# mark the selection as inactive.
#
  reset_selection_vars $page
}

#-----------------------------------------------------------------------
# Insert an empty column to the left of a given column (or -1 to insert
# a column to the left of the first column). If there are any non-empty
# columns after the new column, they are shifted right by one column.
#
# Input:
#  page      The gridded page widget.
#  col       The grid index of the column beyond which to insert the
#            new column.
#-----------------------------------------------------------------------
proc page::insert_col_after {page col} {
  upvar #0 $page p
#
# Get the current number of rows and columns in the page.
#
  grid_size $page ncol nrow
#
# The new column might encroach on the current selection, so discard
# the current selection.
#
  unselect_fields $page
#
# Make room for the new column if there are any columns at and/or beyond $col.
#
  if {$col < $ncol - 1} {
    for {set src [expr {$ncol - 1}]} {$src > $col} {incr src -1} {
      for {set row 0} {$row < $nrow} {incr row} {
	move_field $page $src $row [expr {$src + 1}] $row
      }
    }
  } else {
    $page.grid configure -size "[expr {$col + 2}] $nrow"
  }
}

#-----------------------------------------------------------------------
# Insert an empty row above a given row (or -1 to insert a row above the
# first row). If there are any non-empty rows after the new row, they
# are shifted down by one row.
#
# Input:
#  page      The gridded page widget.
#  row       The grid index of the row beyond which to insert the new row.
#-----------------------------------------------------------------------
proc page::insert_row_after {page row} {
  upvar #0 $page p
#
# Get the current number of rows and columns in the page.
#
  grid_size $page ncol nrow
#
# The new row might encroach on the current selection, so discard
# the current selection.
#
  unselect_fields $page
#
# Make room for the new row if there are any rows at and/or beyond $row.
#
  if {$row < $nrow - 1} {
    for {set src [expr {$nrow - 1}]} {$src > $row} {incr src -1} {
      for {set col 0} {$col < $ncol} {incr col} {
	move_field $page $col $src $col [expr {$src + 1}]
      }
    }
  } else {
    $page.grid configure -size "$ncol [expr {$row + 2}]"
  }
}

#-----------------------------------------------------------------------
# Delete a given row and its contents from the grid. If there are any
# rows below the deleted row, they are moved up by one row to fill the
# vacated space.
#
# Input:
#  page     The gridded page widget.
#  row      The grid index of the row to be deleted.
#-----------------------------------------------------------------------
proc page::delete_row {page row} {
  upvar #0 $page p
#
# Get the current number of rows and columns in the page.
#
  grid_size $page cols rows
#
# The deleted row might be part of the current selection, so discard
# the current selection.
#
  unselect_fields $page
#
# Locate and remove the requested fields.
#
  for {set col 0} {$col < $cols} {incr col} {
    delete_field $page $col $row
  }
#
# If there are any rows below the deleted row, shuffle them back over the
# removed row.
#
  if {$rows > $row + 1} {
    for {set src [expr {$row + 1}]} {$src < $rows} {incr src} {
      for {set col 0} {$col < $cols} {incr col} {
	move_field $page $col $src $col [expr {$src - 1}]
      }
    }
  }
#
# Delete the emptied row at the bottom of the grid.
#
  $page.grid shrink
}

#-----------------------------------------------------------------------
# Delete a given column and its contents from the grid. If there are any
# columns to the right of the deleted column, they are moved left by one
# column to fill the vacated space.
#
# Input:
#  page     The gridded page widget.
#  col      The grid index of the column to be deleted.
#-----------------------------------------------------------------------
proc page::delete_col {page col} {
  upvar #0 $page p
#
# Get the current number of rows and columns in the page.
#
  grid_size $page cols rows
#
# The deleted column might be part of the current selection, so discard
# the current selection.
#
  unselect_fields $page
#
# Locate and remove the requested fields.
#
  for {set row 0} {$row < $rows} {incr row} {
    delete_field $page $col $row
  }
#
# If there are any columns to the right of the deleted column, shuffle
# them back over the removed column.
#
  if {$cols > $col + 1} {
    for {set src [expr {$col + 1}]} {$src < $cols} {incr src} {
      for {set row 0} {$row < $rows} {incr row} {
	move_field $page $src $row [expr {$src - 1}] $row
      }
    }
  }
#
# Delete the emptied column from the right edge of the grid.
#
  $page.grid shrink
}

#-----------------------------------------------------------------------
# Present the user with a dialog to allow specification of a new column
# width.
#
# Input:
#  page       The parent page of the column.
#  col        The index of the column to be modified.
#-----------------------------------------------------------------------
proc page::query_column_width {page col} {
#
# Get the path name of the column configuration dialog.
#
  set dialog .column_dialog
#
# Get the configuration area of the dialog.
#
  set w $dialog.top
#
# Copy the current configuration into the dialog.
#
  replace_entry_contents $w.width.e [$page.grid column $col -width]
#
# Display the dialog.
#
  map_dialog $dialog 1
#
# Prevent other interactions with the program until the dialog has
# been finished with.
#
  set old_focus [focus]               ;# Record the keyboard input focus.
  focus $dialog                       ;# Move keyboard focus to the dialog
  grab set $dialog                    ;# Prevent conflicting user input
  tkwait variable column_dialog_state ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::column_dialog_state apply] == 0} {
#
# Validate the contents of the dialog.
#
    if { ![is_uint [$w.width.e get]] } {
#
# Display an error message and wait for the user to press the apply
# or cancel buttons again.
#
      dialog_error $dialog "The width must be a positive integer, or zero."
      tkwait variable page_dialog_state
#
# Configuration ok?
#
    } else {
#
# Install the new configuration.
#
      $page.grid column $col -width [$w.width.e get]
      break
    }
  }
#
# Withdraw the dialog.
#
  grab release $dialog          ;# Re-enable interactions with other windows
  focus $old_focus              ;# Restore the original keyboard focus.
  wm withdraw $dialog           ;# Hide the dialog for later use.
  return
}

#-----------------------------------------------------------------------
# Return the number of rows and columns that exist in a given page.
#
# Input:
#  page       The parent page.
# Input/Output:
#  cols rows  The number of columns and rows will be assigned to the
#             variables in the caller's stack frame called $cols and
#             $rows.
#-----------------------------------------------------------------------
proc page::grid_size {page cols rows} {
  upvar $cols c
  upvar $rows r
  set cr [$page.grid cget -size]
  set c [lindex $cr 0]
  set r [lindex $cr 1]
}

#-----------------------------------------------------------------------
# List the fields of a given page using the names of their individual
# configuration arrays.
#
# Input:
#  page     The parent page.
#-----------------------------------------------------------------------
proc page::list_fields {page} {
  upvar #0 $page p
#
# Get the current number of rows and columns in the page.
#
  grid_size $page cols rows
#
# Scan the cells of the grid for occupied fields. Place the names of
# the located fields in the return list.
#
  list fields
  for {set col 0} {$col < $cols} {incr col} {
    for {set row 0} {$row < $rows} {incr row} {
      if [info exists p($col,$row)] {
	lappend fields $p($col,$row)
      }
    }
  }
#
# Return the list.
#
  if {[info exists fields]} {
    return $fields
  } else {
    return {}
  }
}

#-----------------------------------------------------------------------
# If any fields have been selected with the mouse, delete them.
#
# Input:
#  page     The parent page of the fields.
#-----------------------------------------------------------------------
proc page::delete_selected_fields {page} {
  upvar #0 $page p   ;#  The page configuration array.
#
# Delete part of a row or part of a column.
#
  switch -- $p(select_dir) {
    h {
      set row $p(srow)
      for {set col $p(scol)} {$col < $p(ecol)} {incr col} {
	delete_field $page $col $row
      }
    }
    v {
      set col $p(scol)
      for {set row $p(srow)} {$row < $p(erow)} {incr row} {
	delete_field $page $col $row
      }
    }
  }
}

#-----------------------------------------------------------------------
# Return the name of the save-page-configuration dialog of a given
# page.
#
# Input:
#  page    The host page.
# Output:
#  return  The tk path name of the dialog.
#-----------------------------------------------------------------------
proc page::save_config_dialog {page} {
#
# Create a string in which the path separators of $page have been
# converted to underscores.
#
  regsub -all {\.} $page _ suffix
#
# The dialog name is formed from the concatenation of a common prefix
# and a sanitized version of the page widget name.
#
  return ".save$suffix"
}

#-----------------------------------------------------------------------
# Create a dialog for saving the configuration of a given page to a
# file.
#
# Input:
#  page     The page to be saved.
# Output:
#  return   The value of $w.
#-----------------------------------------------------------------------
proc page::create_save_page_dialog {page} {
    global ::expname
#
# Construct a name for the dialog, based on that of its host page.
# To make the name legal, replace path separators in $page with
# underscores.
#
  set w [create_config_dialog [save_config_dialog $page] {Save Page Configuration} gcpViewer/page/save]
#
# Create a labeled entry widget $w.f.e.
#
  set f [labeled_entry $w.top.f {File name} {} -width 20]

# Insert a default path

  if {[info exists ::env(GCP_DIR)]} {
      $f.e insert 0 $::env(GCP_DIR)/control/conf/$::expname/
      $f.e xview end
  }

  bind $f.e <Return> "$w.bot.apply invoke"
  pack $f
#
# Bind the ok button to load the specified file.
#
  $w.bot.apply configure -command "::page::save_page_to_file $page \[$f.e get\]; wm withdraw $w"
}

#-----------------------------------------------------------------------
# If any fields have currently been selected, copy the text of each of
# of the selected fields into $p(selection_text) to be returned
# on subsequent paste requests.
#
# Input:
#  page     The gridded page widget.
#-----------------------------------------------------------------------
proc page::update_selection_text {page} {
  upvar #0 $page p
  upvar #0 $page.grid g
#
# Ignore calls to this function while start_drag is waiting
# for the drag window to become visible.
#
  if {[winfo exists .drag]} {
    return
  }
#
# Assemble the text to be returned later if a paste request is received.
#
  set p(selection_text) {}
  switch -- $p(select_dir) {
    h {
      for {set col $p(scol)} {$col < $p(ecol)} {incr col} {
	set cell $col,$p(srow)
	if {[info exists g($cell)]} {
	  if {![info exists scol]} {set scol $col} ;# The first occupied field?
	  set ecol $col                            ;# The last occupied field.
	  append p(selection_text) $g($cell) { }
	}
      }
#
# Update the recorded start and end columns to reflect the actual extent
# of occupied fields. If there aren't any selected fields, discard the
# selection.
#
      if {![info exists scol]} {
	reset_selection_vars $page
      } else {
	set p(scol) $scol
	set p(ecol) [incr ecol]
      }
    }
    v {
      for {set row $p(srow)} {$row < $p(erow)} {incr row} {
	set cell $p(scol),$row
	if {[info exists g($cell)]} {
	  if {![info exists srow]} {set srow $row} ;# The first occupied field?
	  set erow $row                            ;# The last occupied field.
	  append p(selection_text) $g($cell) { }
	}
      }
#
# Update the recorded start and end columns to reflect the actual extent
# of occupied fields. If there aren't any selected fields, discard the
# selection.
#
      if {![info exists srow]} {
	reset_selection_vars $page
      } else {
	set p(srow) $srow
	set p(erow) [incr erow]
      }
    }
  }
}

#-----------------------------------------------------------------------
# This is a selection callback handler. It returns up to maxbytes bytes
# of the text in $p(selection_text), starting 'offset' bytes into the
# string.
#
# Input:
#  page      The page widget.
#-----------------------------------------------------------------------
proc page::return_selection_text {page offset maxbytes} {
  upvar #0 $page p
#
# On the first call update the selection string.
#
  if {$offset == 0} {
    update_selection_text $page
  }
  return [string range $p(selection_text) $offset [expr {$offset + $maxbytes}]]
}

