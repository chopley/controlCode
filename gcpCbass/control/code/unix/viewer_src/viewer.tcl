#-----------------------------------------------------------------------
# This file is incomplete. To complete it add definitions for:
#
#  ::lib_dir   -    The local TCL library directory.
#  ::help_dir  -    The local HTML help directory.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Exit the program after showing information about how to start the
# program.
#
# Input:
#  status     The desired exit status.
#-----------------------------------------------------------------------
proc exit_with_command_line_help {status} {
  puts "\nUsage:\n"
  puts " $::argv0 \[options] configure <configuration_file]>"
  puts "\n  or\n"
  puts " $::argv0 \[options] monitor <hostname> \[configuration_file]"
  puts "\n  or\n"
  puts " $::argv0 \[options] read <start_time> <end_time> <configuration_file>"
  puts "\n\[Note that empty start_time and end_time arguments specify the"
  puts " corresponding time extremes of the archive. To have live monitoring"
  puts " start after reaching the end of the archive, specify the control"
  puts " program host name in the end_time field, instead of a time]."
  puts "\nOptions:"
  puts "           -help     Print this help information."
  puts "           -arcdir   The directory that contains archive files."
  puts "           -bufsize  The size of the plot buffer (registers)."
  puts "           -calfile  The name of the calibration file."
  puts "           -confdir  The directory that contains configuration files."
  puts "           -conffile The name of the default configuration file."
  puts "           -retry    If true (1), attempt to reconnect if the connection is lost"
  puts "           -gateway  The name of the gateway machine (if connecting to a host behind a firewall)."
  puts "           -timeout  The connection timeout, in seconds (if connecting to a host behind a firewall)."
  puts "\nEnvironment variables:"
  puts "     GCP_DIR        The root of the CBI directory tree."
  puts "     GCP_ARC_DIR    The main archive directory."
  exit $status
}

#
# Before doing anything else, check here to see if the user has requested
# help. Note that the remaining command line arguments are parsed in
# start_viewer.
#
if {[lsearch -exact $::argv -help] != -1} {
  exit_with_command_line_help 0
}

#
# Load external modules.
#
source $lib_dir/cmd.tcl
source $lib_dir/page.tcl
source $lib_dir/grabber.tcl
source $lib_dir/starplot.tcl

# List physical constants.

set ::PI 3.1415926535897932
set ::DTOAS  3600
set ::DTORAD $::PI/180

# Create a global variable which will control the viewer's paging.
# When this variable is true, the viewer is allowed to send pager
# requests to the control program

set ::allow_paging 1

# Create global color specifications for indicating when a specification
# is valid or invalid.

set ::error_color pink
set ::good_color lightblue

# Change the default colors of all widgets.

option clear

# Note that BackgroundWidgetColor and ActiveWidgetColor are text
# markers which are replaced by the Makefile with valid color names
# when gcpviewer is compiled (see Makefile)

option add *Background BackgroundWidgetColor widgetDefault
option add *HighlightBackground BackgroundWidgetColor widgetDefault
option add *activeBackground ActiveWidgetColor

option add *activeForeground black
option add *selectForeground black
option add *selectColor yellow

option add *Text.background $good_color widgetDefault
option add *Entry.background $good_color widgetDefault

# Make the fonts of text and entry widgets a bit bigger than
# the default font.

set font {Courier -14 bold}
option add *Entry.font $font widgetDefault
option add *Text.font $font widgetDefault
unset font

# Set the default border widget of the standard widgets.

foreach class {Button Radiobutton Checkbutton Text Scrollbar Entry} {
  option add *$class.BorderWidth 2 widgetDefault
}

# Menu entries that act as labels will be implemented using
# disabled command entries. The default color for disabled entries
# is grey. Change this to blue to make the "labels" stand out more.

option add *Menu.disabledForeground DisabledColor widgetDefault

#-----------------------------------------------------------------------
# List the available horizontal scrolling modes of monitor graphs.
#-----------------------------------------------------------------------
set ::scroll_modes {disabled maximum minimum}

#-----------------------------------------------------------------------
# ::graph_count is used to assign unique graph ids. It is incremented
# every time that a new graph is created and never decremented.
#-----------------------------------------------------------------------
set ::graph_count 0

#-----------------------------------------------------------------------
# ::plot_count is used to assign unique plot ids. It is incremented
# every time that a new plot is created and never decremented.
#-----------------------------------------------------------------------
set ::plot_count 0

set ::apod(display) 0
set ::firewall(display) 0

#-----------------------------------------------------------------------
# Add a new plot to the viewer.
#
# Input:
#  x y            The root window location of the top-left corner of the
#                 widget, or {} {} to leave this up to the window manager.
#  title          The title to write above the plot.
#  xmin xmax      The range of values along the X-axis. (The default
#                 sets a range of 5 minutes, assuming a date x-axis).
#  marker_size    The rendered size of each point.
#  join           True to draw lines between consecutive points.
#  scroll_mode    The x-axis scrolling direction:
#                  disabled - No scrolling.
#                  maximum  - Keep the maximum X-axis value visible.
#                  minimum  - Keep the minimum X-axis value visible.
#  scroll_margin  The x-axis scroll margin, expressed as a percentage
#                 of the x-axis extent. Scrolling occurs in discrete
#                 steps of this size.
#  xregister      The x-axis register.
#  xlabel         The x-axis label.
#  graphs         A list of graph definitions, one per line.
# Output:
#  return         The pathname of the plot. This is also the name of a
#                 global array that contains details about the plot.
#-----------------------------------------------------------------------
proc add_powSpecPlot {{x {}} {y {}} {title {Power Spectrum Plot}} {marker_size 1} {join 1} {xlabel {}} {type powSpec} {npt 1000} {dx 10} {axis 1}} {
    add_plot $x $y $title -2 -1 $marker_size $join disabled 20.0 {} $xlabel {} $type $npt $dx $axis
}

proc add_powSpecPlot_reveal {{revealPlot 1} {x {}} {y {}} {title {Power Spectrum Plot}} {marker_size 1} {join 1} {xlabel {}} {type powSpec} {npt 1000} {dx 10} {axis 1}} {
    add_plot_reveal $revealPlot $x $y $title -2 -1 $marker_size $join disabled 20.0 {} $xlabel {} $type $npt $dx $axis
}

proc add_plot {{x {}} {y {}} {title Plot} {xmin 0} {xmax 0.0034722} {marker_size 1} {join 0} {scroll_mode maximum} {scroll_margin 20} {xregister array.frame.utc.date} {xlabel {}} {graphs {}} {type normal} {npt 0} {dx 0.0} {axis 1}} {
  add_plot_reveal 1 $x $y $title $xmin $xmax $marker_size $join $scroll_mode $scroll_margin $xregister $xlabel $graphs $type $npt $dx $axis
}

proc add_plot_reveal {{revealPlot 1} {x {}} {y {}} {title Plot} {xmin 0} {xmax 0.0034722} {marker_size 1} {join 0} {scroll_mode maximum} {scroll_margin 20} {xregister array.frame.utc.date} {xlabel {}} {graphs {}} {type normal} {npt 0} {dx 0.0} {axis 1}} {

#
# Get a unique name for the new widget and its configuration variable.
#
  set plot .plot[incr ::plot_count]
#
# Make sure that the plot gets deleted on error.
#
  if [catch {
#
# Create a global array of plot characteristics, named after the
# plot widget.
#
    upvar #0 $plot p
    set p(tag) 0                      ;# The C layer identifier.
    set p(graphs) {}                  ;# A list of graph configuration arrays.
    set p(title) {}                   ;# The title of the plot.
    set p(marker_size) 1              ;# The size of plot markers.
    set p(join) 1                     ;# Whether to connect consecutive points.
    set p(xregister) frame.utc.date   ;# The X-axis register
    set p(xlabel) {}
    set p(xmin) 0              ;# The leftmost x-axis value.
    set p(xmax) 0.0034722      ;# The rightmost x-axis value (5 minutes in days)
    set p(ymin) 0              ;# The bottom y-axis value.
    set p(ymax) 1              ;# The top y-axis value
    set p(scroll_mode) maximum ;# The x-axis scrolling direction.
    set p(scroll_margin) 20    ;# The x-axis scroll margin.
    set p(zxmin) [list]        ;# The stack of nested x-axis left zoom limits.
    set p(zxmax) [list]        ;# The stack of nested x-axis right zoom limits.

    set p(type) normal
    set p(npt) 0
    set p(dx) 0.0

#
# Create a toplevel frame to contain the plot and its associated
# configuration and informational widgets.
#
    toplevel $plot -class MonitorPlot
    wm title $plot "Monitor plot ($plot)"
    wm iconname $plot "Plot"
    wm protocol $plot WM_DELETE_WINDOW "quit_plot $plot"
    wm withdraw $plot
#
# Tell the window manager where to map the window?
#
    if {[is_uint $x] && [is_uint $y]} {
      wm geometry $plot +${x}+${y}
    }
#
# Create the dialog that users use to request hardcopy output.
#
    set hard [create_hardcopy_dialog $plot]
#
# Create the dialog that users use to save the plot configuration.
#
    set save [create_save_plot_dialog $plot]

#
# Create the message widget first, so that we have somewhere to
# display error messages while creating other widgets.
#
    create_message_widget $plot.msg
#
# Create the main components of the widget.
#

    if {[string compare $type powSpec] != 0} {
      create_plot_menubar $plot.bar $hard $save
    } else {
      create_powSpecPlot_menubar $plot.bar $hard $save
    }

    create_plot_main_widget $plot.main
#
# Open the plot.
#
    set p(tag) [monitor add_plot $plot.main.p/xtk $type]
#
# Arrange the vertical panes of the widget.
#
    pack $plot.bar  -side top -fill x
    pack $plot.main -side top -fill both -expand true
    pack $plot.msg  -side top -fill x
#
# Set up keyboard short cuts.
#
    bind_short_cut $plot h "map_dialog $hard 0"
    bind $plot <Meta-q> "quit_plot $plot"
    bind_short_cut $plot c "clear_plot $plot"

    bind_short_cut $plot a "show_graph_dialog $plot"
    bind_short_cut $plot p "show_plot_dialog $plot"
    bind_short_cut $plot r "refresh_plot $plot"
    bind_short_cut $plot n "set ::$plot.bar.input.menu none"
    bind_short_cut $plot x "set ::$plot.bar.input.menu xzoom"
    bind_short_cut $plot y "set ::$plot.bar.input.menu yzoom"
    bind_short_cut $plot i "set ::$plot.bar.input.menu ident"
    bind_short_cut $plot s "set ::$plot.bar.input.menu stats"
    bind_short_cut $plot g "set ::$plot.bar.input.menu modwin"
    bind_short_cut $plot d "set ::$plot.bar.input.menu delwin"
    bind_short_cut_shift $plot X "plot_all_x $plot"
    bind_short_cut_shift $plot Y "unzoom_all_y $plot"
    bind_short_cut $plot b "unzoom_all $plot"

#
# Arrange for the plot to be redrawn whenever the plot widget is
# resized by the user.
#
    bind $plot.main.p <Configure> "monitor resize_plot $p(tag)"
#
# Set the initial characteristics of the plot.
#
    configure_plot $plot $title $xmin $xmax $marker_size $join $scroll_mode $scroll_margin $xregister $xlabel $type $npt $dx $axis
#
# Make the plot visible.
#
    if {$revealPlot} {
      wm deiconify $plot
    }

  } result] {
    delete_plot $plot
    error $result
  }
#
# Configure the graphs of the plot.
#
  foreach line [split $graphs "\n"] {
    if {[regexp -- {^[ \t]*graph([ \t].*)} $line unused args]} {
      eval add_graph $plot $args
    } elseif {[regexp -- {^[ \t]*$} $line]} {
      continue;
    } else {
      error "Garbled line in plot description: $line"
    }
  }
#
# Add the plot to the list of plots hosted by the viewer.
#
  lappend ::viewer(plots) $plot
  return $plot
}

#-----------------------------------------------------------------------
# Adopt general user-specified characteristics for a given plot widget.
#
# Input:
#  plot           The plot widget to be configured.
#  title          The title to write above the plot.
#  xmin xmax      The range of values along the X-axis. (The default
#                 sets a range of 5 minutes, assuming a date x-axis).
#  marker_size    The rendered size of each point.
#  join           True to draw lines between consecutive points.
#  scroll_mode    The x-axis scrolling direction:
#                  disabled - No scrolling.
#                  maximum  - Keep the maximum X-axis value visible.
#                  minimum  - Keep the minimum X-axis value visible.
#  scroll_margin  The x-axis scroll margin, expressed as a percentage
#                 of the x-axis extent. Scrolling occurs in discrete
#                 steps of this size.
#  xregister      The x-axis register.
#  xlabel         The x-axis label.
#-----------------------------------------------------------------------
proc configure_plot {plot title xmin xmax marker_size join scroll_mode scroll_margin xregister xlabel {type normal} {npt 0} {dx 0.0} {axis 1}} {
  upvar #0 $plot p       ;# The configuration array of the plot.

#
# Update the title of the plot.
#
  wm title $plot $title
  wm iconname $plot $title
#
# Get the x-axis limits in whatever form the user provided them.
#
  if {[catch {set xa [monitor date_to_mjd $xmin]}]} {
    if {[catch {set xa [monitor interval_to_hours $xmin]}]} {
      set xa $xmin
    }
  }
  if {[catch {set xb [monitor date_to_mjd $xmax]}]} {
    if {[catch {set xb [monitor interval_to_hours $xmax]}]} {
      set xb $xmax
    }
  }
#
# If scrolling is disabled then we only need a range of values, not
# two limits.
#
  if {[string compare $scroll_mode disabled] != 0} {
    if {[catch {set xb [expr {$xb - $xa}]}]} {
      error "One of the x-axis limits is not a number."
    }
    set xa 0
  }
#
# Attempt to change the monitor_viewer parameters of the plot.
#
# Is the plot currently zoomed?
#

  if {[llength $p(zxmax)] > 0} {
    monitor configure_plot $p(tag) $title [lindex $p(zxmin) end] \
	[lindex $p(zxmax) end] $marker_size $join disabled $scroll_margin \
	$xregister $xlabel $type $npt $dx $axis
  } else {
    monitor configure_plot $p(tag) $title $xa $xb $marker_size $join $scroll_mode $scroll_margin $xregister $xlabel $type $npt $dx $axis
  }
#
# Record the new parameters.
#
  set p(title) $title
  set p(marker_size) $marker_size
  set p(join) $join
  set p(scroll_margin) $scroll_margin
  set p(xregister) $xregister
  set p(xlabel) $xlabel
  set p(xmin) $xa
  set p(xmax) $xb
  set p(scroll_mode) $scroll_mode

  set p(type) $type
  set p(npt) $npt
  set p(dx) $dx
  set p(axis) $axis
}

#-----------------------------------------------------------------------
# Adopt general user-specified characteristics for all graphs of a
# given plot widget
#
# Input:
#  plot           The plot widget to be configured.
#  title          The title to write above the plot.
#  xmin xmax      The range of values along the X-axis. (The default
#                 sets a range of 5 minutes, assuming a date x-axis).
#  marker_size    The rendered size of each point.
#  join           True to draw lines between consecutive points.
#  scroll_mode    The x-axis scrolling direction:
#                  disabled - No scrolling.
#                  maximum  - Keep the maximum X-axis value visible.
#                  minimum  - Keep the minimum X-axis value visible.
#  scroll_margin  The x-axis scroll margin, expressed as a percentage
#                 of the x-axis extent. Scrolling occurs in discrete
#                 steps of this size.
#  xregister      The x-axis register.
#  xlabel         The x-axis label.
#-----------------------------------------------------------------------
proc configure_all_graphs {plot ymin ymax resetTrack} {
  upvar #0 $plot p       ;# The configuration array of the plot.

#
# Exploit the fact that unzoom_xaxis zooms out to the extent
# of the buffered data when there are no recorded zoom limits.
#
  foreach graph $p(graphs) {
    upvar #0 $graph g

# If we were told to reset the track variable when plot y-axes are
# configured, then set it to false.  Otherwise, leave it at the
# user-configured value

    if {$resetTrack} {
      configure_graph $graph $ymin $ymax $g(ylabel) $g(yregs) $g(bits) 0 $g(av) $g(axis) $g(apod)
    } else {
      configure_graph $graph $ymin $ymax $g(ylabel) $g(yregs) $g(bits) $g(track) $g(av) $g(axis) $g(apod)
    }
  }

#
# Record the new parameters.
#
  set p(ymin) $ymin
  set p(ymax) $ymax
}

#-----------------------------------------------------------------------
# Redraw a given plot without reconfiguring it.
#
# Input:
#  plot       The path name of the plot.
#-----------------------------------------------------------------------
proc refresh_plot {plot} {
  upvar #0 $plot p         ;# The configuration array of the widget.
  monitor redraw_plot $p(tag)
}

#-----------------------------------------------------------------------
# Redraw a given graph without reconfiguring it.
#
# Input:
#   graph       The path name of the  graph.
#-----------------------------------------------------------------------
proc refresh_graph {graph} {
  upvar #0 $graph g        ;# The configuration array of the graph.
  upvar #0 $g(plot) p      ;# The configuration array of the parent plot.
  monitor redraw_graph $p(tag) $g(tag)
}

#-----------------------------------------------------------------------
# Expand the X-axis limits of the specified plot and redraw it to show
# the extent of the buffered data.
#
# Input:
#  plot       The path name of the plot.
#-----------------------------------------------------------------------
proc plot_all_x {plot} {
  upvar #0 $plot p         ;# The configuration array of the widget.
#
# Exploit the fact that unzoom_xaxis zooms out to the extent
# of the buffered data when there are no recorded zoom limits.
#
  set p(zxmin) [list]
  set p(zxmax) [list]
  unzoom_xaxis $plot
}

#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify the characteristics
# of all graphs of a given plot.
#
# Input:
#  plot     The pathname of the plot to be reconfigured, or {} to create
#           a new plot.
#-----------------------------------------------------------------------
proc show_all_graph_dialog {{plot {}}} {
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the plot to be restored.
#
    upvar #0 $plot p
    set provisional 0

#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::all_graph_dialog_state apply
#
# Get the path name of the plot-configuration dialog and its configuration
# area.
#
  set dialog .all_graph_dialog
  set w $dialog.top

#
# Copy the current plot configuration into the dialog.
#
  replace_entry_contents $w.y.lim.e1 $p(ymin)
  replace_entry_contents $w.y.lim.e2 $p(ymax)

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
  tkwait variable all_graph_dialog_state ;# Wait for the apply or cancel button

#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::all_graph_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {
      configure_all_graphs $plot [$w.y.lim.e1 get] [$w.y.lim.e2 get] 0
      refresh_plot $plot
      set ::all_graph_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable all_graph_dialog_state   ;# Wait for the user again.
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
  if {[string compare $::all_graph_dialog_state cancel] == 0} {
      monitor reconfigure
  }
}

#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify the characteristics
# of all graphs of a given plot.
#
# Input:
#  plot     The pathname of the plot to be reconfigured, or {} to create
#           a new plot.
#-----------------------------------------------------------------------
proc show_all_powSpecGraph_dialog {{plot {}}} {
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the plot to be restored.
#
    upvar #0 $plot p
    set provisional 0

#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::all_graph_dialog_state apply
#
# Get the path name of the plot-configuration dialog and its configuration
# area.
#
  set dialog .all_powSpecGraph_dialog
  set w $dialog.top

#
# Copy the current plot configuration into the dialog.
#
  replace_entry_contents $w.y.lim.e1 $p(ymin)
  replace_entry_contents $w.y.lim.e2 $p(ymax)

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
  tkwait variable all_graph_dialog_state ;# Wait for the apply or cancel button

#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::all_graph_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {
      configure_all_graphs $plot [$w.y.lim.e1 get] [$w.y.lim.e2 get] 1
      refresh_plot $plot
      set ::all_graph_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable all_graph_dialog_state   ;# Wait for the user again.
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
  if {[string compare $::all_graph_dialog_state cancel] == 0} {
      monitor reconfigure
  }
}

#-----------------------------------------------------------------------
# Unzoom the X-axis of a given plot.
#
# Input:
#  plot    The Tk path name of the plot.
#-----------------------------------------------------------------------
proc unzoom_xaxis {plot} {
  upvar #0 $plot p         ;# The configuration array of the widget.
#
# Is the plot active?
#
  if {$p(tag) != 0} {
#
# If the plot isn't currently zoomed, replace the zoom-out axis limits
# with the extent of the buffered data.
#
    if {[llength $p(zxmin)] == 0} {
      monitor get_xlimits $p(tag) p(xmin) p(xmax)
#
# If it is currently zoomed, zoom out to the next pair of limits
# that are on the x-axis zoom list, or to the unzoom limits if
# we have exhausted the list.
#
    } else {
      set p(zxmin) [lreplace $p(zxmin) end end]
      set p(zxmax) [lreplace $p(zxmax) end end]
    }
#
# Redraw the plot with the new x-axis limits and scrolling mode.
#
    configure_plot $plot $p(title) $p(xmin) $p(xmax) $p(marker_size) \
	$p(join) $p(scroll_mode) $p(scroll_margin) $p(xregister) $p(xlabel) \
	$p(type) $p(npt) $p(dx) $p(axis)
    refresh_plot $plot
  }
}


#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify the characteristics
# of a given plot.
#
# Input:
#  plot     The pathname of the plot to be reconfigured, or {} to create
#           a new plot.
#-----------------------------------------------------------------------
proc show_plot_dialog {{plot {}}} {
#
# Create a new plot?
#
  if {[string length $plot]==0} {
    set provisional 1
    set plot [add_plot_reveal 0]
    set cancel_command [list delete_plot $plot]
    upvar #0 $plot p
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the plot to be restored.
#
  } else {
    upvar #0 $plot p
    set provisional 0
    set cancel_command [list configure_plot $plot $p(title) $p(xmin) $p(xmax) \
	$p(marker_size) $p(join) $p(scroll_mode) $p(scroll_margin) \
	$p(xregister) $p(xlabel) $p(type) $p(npt) $p(dx) $p(axis)]
  }
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::plot_dialog_state apply
#
# Get the path name of the plot-configuration dialog and its configuration
# area.
#
  set dialog .plot_dialog
  set w $dialog.top
#
# Copy the current plot configuration into the dialog.
#
  replace_entry_contents $w.title.e $p(title)
  replace_entry_contents $w.x.xr.e  $p(xregister)
  replace_entry_contents $w.x.lab.e $p(xlabel)
#
# As a special case, attempt to show fixed date axis limits as Gregorian
# dates instead of Modified Julian Dates.
#
  set is_date [string match *.date $p(xregister)]
  set is_scrolled [string compare $p(scroll_mode) disabled]
  if {!$is_date || $is_scrolled || [catch {
    set xmin [monitor mjd_to_date $p(xmin)]}]
  } {
    set xmin $p(xmin)
  }
  if {!$is_date || $is_scrolled || [catch {
    set xmax [monitor mjd_to_date $p(xmax)]}]
  } {
    set xmax $p(xmax)
  }
  replace_entry_contents $w.x.min.e $xmin
  replace_entry_contents $w.x.max.e $xmax
  $w.x.inc.sc set $p(scroll_margin)
  set ::$w.x.mode $p(scroll_mode)
  replace_entry_contents $w.app.point.e $p(marker_size)
  set ::$w.app.join $p(join)
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
  tkwait variable plot_dialog_state   ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::plot_dialog_state apply] == 0} {

#
# Attempt to configure the widget with the parameters that the user
# entered.
#

    if {[catch {
      configure_plot $plot [$w.title.e get] [$w.x.min.e get] \
	  [$w.x.max.e get] [$w.app.point.e get] [set ::$w.app.join] \
	  [set ::$w.x.mode] [$w.x.inc.sc get] [$w.x.xr.e get] [$w.x.lab.e get] \
	  normal
      monitor reconfigure
      set ::plot_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable plot_dialog_state   ;# Wait for the user again.
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

  if {[string compare $::plot_dialog_state cancel] == 0} {

#
# If the plot is provisional, delete it. Otherwise restore its original
# configuration.
#

    if {$provisional} {
      delete_plot $plot
      monitor reconfigure
      return {}
    } else {
      eval $cancel_command
      monitor reconfigure
    }
  } else {
#
# If the plot is new, make it visible.
#
    if {$provisional} {
      wm deiconify $plot
      tkwait visibility $plot
    }
  }
  return $plot
}
#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify the characteristics
# of a given power spectrum plot.
#
# Input:
#  plot     The pathname of the plot to be reconfigured, or {} to create
#           a new plot.
#-----------------------------------------------------------------------
proc show_powSpecPlot_dialog {{plot {}}} {
#
# Create a new plot?
#
  if {[string length $plot]==0} {
    set provisional 1
    set plot [add_powSpecPlot_reveal 0]
    set cancel_command [list delete_plot $plot]
    upvar #0 $plot p
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the plot to be restored.
#
  } else {
    upvar #0 $plot p
    set provisional 0
    set cancel_command [list configure_plot $plot $p(title) $p(xmin) $p(xmax) \
	$p(marker_size) $p(join) $p(scroll_mode) $p(scroll_margin) \
	$p(xregister) $p(xlabel) powSpec $p(npt) $p(dx) $p(axis)]
  }
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::powSpecPlot_dialog_state apply
#
# Get the path name of the plot-configuration dialog and its configuration
# area.
#
  set dialog .powSpecPlot_dialog
  set w $dialog.top
#
# Copy the current plot configuration into the dialog.
#
  replace_entry_contents $w.title.e $p(title)
  replace_entry_contents $w.x.lab.e $p(xlabel)
  replace_entry_contents $w.x.lab.e $p(xlabel)
  replace_entry_contents $w.x.npt.e $p(npt)
  replace_entry_contents $w.x.dx.e $p(dx)

  set ::$w.x.axis.type $p(axis)

  if {$p(axis)==1} {
    $w.x.axis.linear select
  } else {
    $w.x.axis.log select
  }

#
# As a special case, attempt to show fixed date axis limits as Gregorian
# dates instead of Modified Julian Dates.
#
  set is_scrolled [string compare $p(scroll_mode) disabled]

  replace_entry_contents $w.app.point.e $p(marker_size)

  set ::$w.app.join $p(join)
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
  tkwait variable powSpecPlot_dialog_state   ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::powSpecPlot_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#

    if {[catch {
      configure_plot $plot [$w.title.e get] -2 -1 [$w.app.point.e get] \
	  [set ::$w.app.join] disabled 20.0 x [$w.x.lab.e get] \
	  powSpec [$w.x.npt.e get] [$w.x.dx.e get] [set ::$w.x.axis.type]
      monitor reconfigure
      set ::powSpecPlot_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable powSpecPlot_dialog_state   ;# Wait for the user again.
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
  if {[string compare $::powSpecPlot_dialog_state cancel] == 0} {
#
# If the plot is provisional, delete it. Otherwise restore its original
# configuration.
#
    if {$provisional} {
      delete_plot $plot
      monitor reconfigure
      return {}
    } else {
      eval $cancel_command
      monitor reconfigure
    }
  } else {
#
# If the plot is new, make it visible.
#
    if {$provisional} {
      wm deiconify $plot
      tkwait visibility $plot
    }
  }
  return $plot
}

#-----------------------------------------------------------------------
# Zoom in along the x-axis of a plot.
#
# Input:
#  plot       The path name of the plot.
#  xmin xmax  The range of X-axis to display.
#-----------------------------------------------------------------------
proc zoom_xaxis {plot xmin xmax} {
  upvar #0 $plot p ;# The configuration array of the plot.
#
# Don't zoom redundantly.
#
  if {[string compare [lindex $p(zxmin) end] $xmin] == 0 && \
      [string compare [lindex $p(zxmax) end] $xmax] == 0} {
    return
  }
#
# Append the new zoom limits to the current list.
#
  lappend p(zxmin) $xmin
  lappend p(zxmax) $xmax
#
# Draw the zoomed plot.
#
  configure_plot $plot $p(title) $p(xmin) $p(xmax) $p(marker_size) $p(join) \
      $p(scroll_mode) $p(scroll_margin) $p(xregister) $p(xlabel) $p(type) \
      $p(npt) $p(dx) $p(axis)

  refresh_plot $plot
}

#-----------------------------------------------------------------------
# Clear the current plot of all historical data.
#
# Input:
#  plot     The pathname of the plot to be cleared.
#-----------------------------------------------------------------------
proc clear_plot {plot} {
  upvar #0 $plot p  ;#  The parameter array of the plot.
  monitor limit_plot $p(tag)
  refresh_plot $plot
}

#-----------------------------------------------------------------------
# Create the menu bar of a plot widget.
#
# Input:
#  w          The path name to give the widget.
#  hard       The name of the hardcopy dialog of the plot.
#  save       The name of the save-configuration dialog of the plot.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_plot_menubar {w hard save} {
#
# Create a raised frame for the menubar.
#
  frame $w -relief raised -bd 2 -width 15c
#
# Get the parent plot.
#
  set plot [winfo toplevel $w]
#
# Create the file menu.
#
  menubutton $w.file -text File -menu $w.file.menu
  set m [menu $w.file.menu -tearoff 0]
#
# Provide a dialog for saving the configuration of the plot.
#
  $m add command -label {Save Plot Configuration} \
      -command "wm deiconify $save; raise $save"
#
# Create a hardcopy dialog.
#
  $m add command -label {Hardcopy} -underline 0 -command "map_dialog $hard 0"
#
# By convention the quit entry sits at the end of the menu.
#
  $m add separator
  $m add command -label {Quit   [Meta+q]} -command "quit_plot $plot"
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
  $m add command -label {Clear Plot} -underline 0 \
      -command "clear_plot $plot"
  $m add command -label {Add Graph} -underline 0 \
      -command "show_graph_dialog $plot"
  $m add command -label {Modify Plot} -underline 7 \
      -command "show_plot_dialog $plot"
  $m add command -label {Refresh Plot} -underline 0 \
      -command "refresh_plot $plot"
  $m add command -label {Modify All Graph Y-Axes} \
      -command "show_all_graph_dialog $plot"
  $m add command -label {Auto Scale X-axis} -underline 11 \
      -command "plot_all_x $plot"
  $m add command -label {Auto Scale Y-axes} -underline 11 \
      -command "unzoom_all_y $plot"
  $m add command -label {Auto Scale Both} -underline 11 \
      -command "unzoom_all $plot"
  pack $w.config -side left
#
# Create a cursor-input-mode menu.
#
  menubutton $w.input -text {Cursor} -menu $w.input.menu
  set m [menu $w.input.menu -tearoff 1]
  $m add command -label {Cursor input mode} -state disabled
  $m add separator
  $m add radiobutton -variable $m -value none -label {None}
  $m add radiobutton -variable $m -value xzoom -label {Zoom X} -underline 5
  $m add radiobutton -variable $m -value yzoom -label {Zoom Y} -underline 5
  $m add radiobutton -variable $m -value ident -label {Identify} -underline 0
  $m add radiobutton -variable $m -value stats -label {Stats} -underline 0
  $m add radiobutton -variable $m -value modwin -label {Modify Graph} -underline 7
  $m add radiobutton -variable $m -value delwin -label {Delete Graph} -underline 0
  $m add radiobutton -variable $m -value startintwin -label {Start Integrating Graph} -underline 0
  $m add radiobutton -variable $m -value stopintwin -label {Stop Integrating Graph} -underline 0

#
# Link a callback to the value of the input-mode variable.
#
  global $m
  trace variable $m w "change_cursor_mode $plot"
  set $m none
  pack $w.input -side left
#
# Create a find menu
#
  menubutton $w.find -text {Find} -menu $w.find.menu
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
  $m add command -label {Plots in general} -command "show_help gcpViewer/plot/index"
  $m add command -label {Adding a graph} -command "show_help gcpViewer/plot/add_graph"
  $m add command -label {Reconfiguring a graph} -command "show_help gcpViewer/plot/conf_graph"
  $m add command -label {Deleting graphs} -command "show_help gcpViewer/plot/del_graph"
  $m add command -label {Reconfiguring the plot} -command "show_help gcpViewer/plot/conf_plot"
  $m add command -label {Getting the identity and value of a given point} -command "show_help gcpViewer/plot/identify"
  $m add command -label {Getting the statistics of a given trace} -command "show_help gcpViewer/plot/stats"
  $m add command -label {Zooming and autoscaling the X axis of a plot} -command "show_help gcpViewer/plot/xzoom"
  $m add command -label {Zooming and autoscaling the Y axis of a graph} -command "show_help gcpViewer/plot/index"
  $m add command -label {Clearing a plot} -command "show_help gcpViewer/plot/clear"
  $m add command -label {Refreshing a plot} -command "show_help gcpViewer/plot/refresh"
  $m add command -label {Saving the configuration of a plot to a file} -command "show_help gcpViewer/plot/save"
  $m add command -label {Getting hardcopy output of a plot} -command "show_help gcpViewer/plot/hard"
  pack $w.help -side right
  return $w
}

#-----------------------------------------------------------------------
# Create the menu bar of a powSpecPlot widget.
#
# Input:
#  w          The path name to give the widget.
#  hard       The name of the hardcopy dialog of the plot.
#  save       The name of the save-configuration dialog of the plot.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_powSpecPlot_menubar {w hard save} {
#
# Create a raised frame for the menubar.
#
  frame $w -relief raised -bd 2 -width 15c
#
# Get the parent plot.
#
  set plot [winfo toplevel $w]
#
# Create the file menu.
#
  menubutton $w.file -text File -menu $w.file.menu
  set m [menu $w.file.menu -tearoff 0]
#
# Provide a dialog for saving the configuration of the plot.
#
  $m add command -label {Save Plot Configuration} \
      -command "wm deiconify $save; raise $save"
#
# Create a hardcopy dialog.
#
  $m add command -label {Hardcopy} -underline 0 -command "map_dialog $hard 0"
#
# By convention the quit entry sits at the end of the menu.
#
  $m add separator
  $m add command -label {Quit   [Meta+q]} -command "quit_plot $plot"
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
  $m add command -label {Clear Plot} -underline 0 \
      -command "clear_plot $plot"
  $m add command -label {Add Graph} -underline 0 \
      -command "show_powSpecGraph_dialog $plot"
  $m add command -label {Modify Plot} -underline 7 \
      -command "show_powSpecPlot_dialog $plot"
  $m add command -label {Refresh Plot} -underline 0 \
      -command "refresh_plot $plot"
  $m add command -label {Modify All Graph Y-Axes} \
      -command "show_all_powSpecGraph_dialog $plot"
  $m add command -label {Auto Scale X-axis} -underline 11 \
      -command "plot_all_x $plot"
  $m add command -label {Auto Scale Y-axes} -underline 11 \
      -command "unzoom_all_y $plot"
  $m add command -label {Auto Scale Both} -underline 11 \
      -command "unzoom_all $plot"
  pack $w.config -side left
#
# Create a cursor-input-mode menu.
#
  menubutton $w.input -text {Cursor} -menu $w.input.menu
  set m [menu $w.input.menu -tearoff 1]
  $m add command -label {Cursor input mode} -state disabled
  $m add separator
  $m add radiobutton -variable $m -value none -label {None}
  $m add radiobutton -variable $m -value xzoom -label {Zoom X} -underline 5
  $m add radiobutton -variable $m -value pkident -label {Identify peaks} -underline 12
  $m add radiobutton -variable $m -value yzoom -label {Zoom Y} -underline 5
  $m add radiobutton -variable $m -value ident -label {Identify} -underline 0
  $m add radiobutton -variable $m -value stats -label {Stats} -underline 0
  $m add radiobutton -variable $m -value modwin -label {Modify Graph} -underline 7
  $m add radiobutton -variable $m -value delwin -label {Delete Graph} -underline 0
  $m add radiobutton -variable $m -value startintwin -label {Start Integrating Graph} -underline 0
  $m add radiobutton -variable $m -value stopintwin -label {Stop Integrating Graph} -underline 0

  bind_short_cut $plot k "set ::$plot.bar.input.menu pkident"

#
# Link a callback to the value of the input-mode variable.
#
  global $m
  trace variable $m w "change_cursor_mode $plot"
  set $m none
  pack $w.input -side left
#
# Create a find menu
#
  menubutton $w.find -text {Find} -menu $w.find.menu
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
  $m add command -label {Plots in general} -command "show_help gcpViewer/plot/index"
  $m add command -label {Adding a graph} -command "show_help gcpViewer/plot/add_graph"
  $m add command -label {Reconfiguring a graph} -command "show_help gcpViewer/plot/conf_graph"
  $m add command -label {Deleting graphs} -command "show_help gcpViewer/plot/del_graph"
  $m add command -label {Reconfiguring the plot} -command "show_help gcpViewer/plot/conf_plot"
  $m add command -label {Getting the identity and value of a given point} -command "show_help gcpViewer/plot/identify"
  $m add command -label {Getting the statistics of a given trace} -command "show_help gcpViewer/plot/stats"
  $m add command -label {Zooming and autoscaling the X axis of a plot} -command "show_help gcpViewer/plot/xzoom"
  $m add command -label {Zooming and autoscaling the Y axis of a graph} -command "show_help gcpViewer/plot/index"
  $m add command -label {Clearing a plot} -command "show_help gcpViewer/plot/clear"
  $m add command -label {Refreshing a plot} -command "show_help gcpViewer/plot/refresh"
  $m add command -label {Saving the configuration of a plot to a file} -command "show_help gcpViewer/plot/save"
  $m add command -label {Getting hardcopy output of a plot} -command "show_help gcpViewer/plot/hard"
  pack $w.help -side right
  return $w
}

#-----------------------------------------------------------------------
# Return the name of the save-plot-configuration dialog of a given
# plot.
#
# Input:
#  plot    The host plot.
# Output:
#  return  The tk path name of the dialog.
#-----------------------------------------------------------------------
proc save_plot_dialog {plot} {
#
# Create a string in which the path separators of $plot have been
# converted to underscores.
#
  regsub -all {\.} $plot _ suffix
#
# The dialog name is formed from the concatenation of a common prefix
# and a sanitized version of the plot widget name.
#
  return ".save$suffix"
}

#-----------------------------------------------------------------------
# Create a dialog for saving the configuration of a given plot to a
# file.
#
# Input:
#  plot     The plot to be saved.
# Output:
#  return   The value of $w.
#-----------------------------------------------------------------------
proc create_save_plot_dialog {plot} {

global ::expname

#
# Construct a name for the dialog, based on that of its host plot.
# To make the name legal, replace path separators in $plot with
# underscores.
#
  set w [create_config_dialog [save_plot_dialog $plot] {Save Plot Configuration} gcpViewer/plot/save]
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
  $w.bot.apply configure -command "save_plot_to_file $plot \[$f.e get\]; wm withdraw $w"
  return $w
}

#-----------------------------------------------------------------------
# Open a specified new file, save the configuration of the given plot
# to it, then close the file.
#
# Input:
#  plot      The plot who's configuration is to be saved.
#  file      The name of the file to record the configuration in.
#-----------------------------------------------------------------------
proc save_plot_to_file {plot file} {
#
# Open the specified file.
#
  set out [open $file w]
#
# Save the configuration to it.
#
  if {[catch {save_plot $plot $out} result]} {
    report_error $result
  }
#
# Close the file.
#
  close $out
}

#-----------------------------------------------------------------------
# Create the area of the plot widget that contains the PGPLOT widget
# and associated controls.
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_plot_main_widget {w} {
#
# Frame the plotarea.
#
  frame $w -width 15c -height 10c
#
# Create an area for mouse-button descriptions.
#
  create_button_show_widget $w.b
#
# Frame the area that will contain the PGPLOT widget.
#
  pgplot $w.p -width 15c -height 15c -bg black -relief sunken -bd 2 \
      -maxcolors 16
  pack $w.b -side top -fill x
  pack $w.p -side left -fill both -expand true
  return $w
}

#-----------------------------------------------------------------------
# Create the area of the pager widget that contains the menubar
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_pager_menubar {w} {
#
# Create a raised frame for the menubar.
#
  frame $w -relief raised -bd 2 -width 15c
#
# Create a find menu
#
  menubutton $w.find -text {Find} -menu $w.find.menu
  set m [menu $w.find.menu -tearoff 0]
#
# Create its member(s)
#
  $m add command -label {Find gcpViewer} -command "reveal ."

  pack $w.find -side left
}

#-----------------------------------------------------------------------
# Create the pager widget
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_pager_widget {w} {
  set win $w
#
# Create a toplevel frame to contain the win and its associated
# configuration and informational widgets.
#
  toplevel $win -class PagerWindow
  wm title $win "Pager Window"
  wm iconname $win "Pager"
  wm withdraw $win
  wm protocol $win WM_DELETE_WINDOW "wm withdraw $w"

# Create the main components of the widget.

  create_pager_menubar $win.bar

  frame $win.main -relief ridge -bd 2 -width 10c
  frame $win.main.pagerLabel
  frame $win.main.cmdTimeoutLabel
  frame $win.main.dataTimeoutLabel
  frame $win.main.activePageLabel
  frame $win.main.text

# Now pack the main components

  pack $win.bar -side top -fill x
  pack $win.main.pagerLabel -side top
  pack $win.main.cmdTimeoutLabel -side top -pady 4
  pack $win.main.dataTimeoutLabel -side top -pady 4
  pack $win.main.text  -side bottom -fill both -expand true

# Create the label for specifying the overall pager state

  label $win.main.pagerLabel.label -bg black -fg yellow -text "Pager State:"
  label $win.main.pagerLabel.state -bg black -fg white  -text "Unknown"

  pack $win.main.pagerLabel.label $win.main.pagerLabel.state -side left -anchor w

# Create the label for specifying the cmd timeout state

  label $win.main.cmdTimeoutLabel.label          -bg black -fg yellow -text "Command timeout watchdog state:"
  label $win.main.cmdTimeoutLabel.state          -bg black -fg white  -text "Unknown"
  label $win.main.cmdTimeoutLabel.timeoutLabel   -bg black -fg yellow -text "Timeout:"
  label $win.main.cmdTimeoutLabel.timeoutSeconds -bg black -fg white  -text "Unknown"

  pack $win.main.cmdTimeoutLabel.label $win.main.cmdTimeoutLabel.state $win.main.cmdTimeoutLabel.timeoutLabel $win.main.cmdTimeoutLabel.timeoutSeconds -side left -anchor w

# Create the label for specifying the data timeout state

  label $win.main.dataTimeoutLabel.label          -bg black -fg yellow -text "Data timeout watchdog state:"
  label $win.main.dataTimeoutLabel.state          -bg black -fg white  -text "Unknown"
  label $win.main.dataTimeoutLabel.timeoutLabel   -bg black -fg yellow -text "Timeout:"
  label $win.main.dataTimeoutLabel.timeoutSeconds -bg black -fg white  -text "Unknown"

  pack $win.main.dataTimeoutLabel.label $win.main.dataTimeoutLabel.state $win.main.dataTimeoutLabel.timeoutLabel $win.main.dataTimeoutLabel.timeoutSeconds -side left -anchor w

# Create the label for specifying a paging condition

  label $win.main.activePageLabel.label -bg yellow -fg black
  pack $win.main.activePageLabel.label

# Create the listbox for displaying register conditions

  listbox $win.main.text.text -width 110 -height 50 -font {Courier -12} -bg black -fg green -takefocus 0 -yscrollcommand [list $win.main.text.sy set] -xscrollcommand [list $win.main.text.sx set]

#  text $win.main.text.text -width 110 -height 50 -font {Courier -12} -bg black -fg green -takefocus 0 -yscrollcommand [list $win.main.text.sy set] \
# -xscrollcommand [list $win.main.text.sx set] -wrap none

  scrollbar $win.main.text.sx -width 3m -orient horizontal -command [list $win.main.text.text xview]
  scrollbar $win.main.text.sy -width 3m -orient vertical   -command [list $win.main.text.text yview]

  pack $win.main.text.sy   -side right  -fill y
  pack $win.main.text.sx   -side bottom -fill x
  pack $win.main.text.text -side left   -fill both -expand true
  pack $win.main -fill both -expand true

# Add return the pager widget.

  return $win
}

proc clear_pager {pager} {
    $pager.main.pagerLabel.state                configure -text "Unknown"
    $pager.main.cmdTimeoutLabel.state           configure -text "Unknown"
    $pager.main.cmdTimeoutLabel.timeoutSeconds  configure -text "Unknown"

    $pager.main.dataTimeoutLabel.state          configure -text "Unknown"
    $pager.main.dataTimeoutLabel.timeoutSeconds configure -text "Unknown"
    $pager.main.dataTimeoutLabel.timeoutSeconds configure -text "Unknown"

    pack forget $pager.main.activePageLabel

    $pager.main.text.text delete 0 end
}

#-----------------------------------------------------------------------
# Create an area in which the cursor button actions will be displayed.
#
# Input:
#  w          The path name to assign to the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_button_show_widget {w} {
  frame $w -height 20
  label $w.t -text "Mouse buttons: " -fg TextLabelColor
  label $w.b1 -text {N/A} -relief sunken -bg yellow
  label $w.b2 -text {N/A} -relief sunken -bg skyblue
  label $w.b3 -text {N/A} -relief sunken -bg green
  pack $w.t -side left -anchor nw
  pack $w.b1 $w.b2 $w.b3 -side left -anchor nw -fill x -expand true
  pack propagate $w false
}

#-----------------------------------------------------------------------
# Create a graph configuration dialog.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_graph_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Graph} gcpViewer/plot/conf_graph
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply  configure -command "set ::graph_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::graph_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::graph_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Specify the amount of padding to leave around the edge of the area.
#
  set xpad 2m
#
# Create an area in which the list of graph registers is selected. This
# contains a scrollable text widget in which to list the registers, and
# a menu listing recognized registers.
#
  set r [frame $w.r -relief groove -bd 2]
#
# Start with the scrollable text widget that will be used to list graph
# registers.
#
  label $r.title -text {Registers (one per line):} -fg TextLabelColor
  pack $r.title -side top -anchor nw
  frame $r.tf
  text $r.tf.t -width 20 -height 3 -wrap none \
      -yscrollcommand [list $r.tf.sy set] \
      -xscrollcommand [list $r.tf.sx set]

  bind $r.tf.t <Key-space> {break}
  bind $r.tf.t <Tab> {focus [tk_focusNext %W]; break}
  bind $r.tf.t <Shift-Tab> {focus [tk_focusPrev %W]; break}

# Bind the left mouse button to copy, the middle button to insert
 
  set $r.tf.t.text ""

  bind $r.tf.t <ButtonRelease-1> "storeSelection  $r.tf.t $r.tf.t.text"
  bind $r.tf.t <ButtonPress-2>   "insertSelection $r.tf.t $r.tf.t.text"

# Emacs-style bindings too:

  bind $r.tf.t <Control-k>        "killSelection   $r.tf.t $r.tf.t.text"
  bind $r.tf.t <Control-y>        "insertSelection $r.tf.t $r.tf.t.text"

  scrollbar $r.tf.sx -width 3m -orient horizontal \
      -command [list $r.tf.t xview]
  scrollbar $r.tf.sy -width 3m -orient vertical -command [list $r.tf.t yview]
  pack $r.tf.sy -side right -fill y
  pack $r.tf.sx -side bottom -fill x
  pack $r.tf.t -side left -fill x -expand true
  pack $r.tf -side left -anchor nw -fill x -expand true -padx 2m
#
# Now the menubutton that posts the list of known boards and their registers
# and directs any resulting user selection to the text widget.
#
  create_regmenu_button $r.mb $r.tf.t
  pack $r.mb -side top -anchor c -padx 1m
  pack $r -side top -fill x -anchor nw -expand true -pady 1m -padx 1m
#
# Encapsulate Y-axis configuration parameters.
#
  set y [frame $w.y -relief groove -bd 2]
  label $y.title -text {The configuration of the Y-axis:} -fg TextLabelColor
  pack $y.title -side top -anchor nw
#
# Next create a labelled entry widget for specifying the Y-axis label.
#
  frame $y.l
  label $y.l.l -text "Label: "
  entry $y.l.e -width 16
  pack $y.l.l -side left -anchor w
  pack $y.l.e -side left -fill x -expand true
  pack $y.l -side top -fill x -anchor nw -padx $xpad
#
# Now create the pair of Y-axis range, labelled entry widgets.
#
  frame $y.lim
  label $y.lim.l1 -text "Range: "
  entry $y.lim.e1 -width 8
  label $y.lim.l2 -text "to: "
  entry $y.lim.e2 -width 8

  pack $y.lim.l1 -side left -anchor w
  pack $y.lim.e1 -side left -fill x -expand true
  pack $y.lim.l2 -side left -anchor w
  pack $y.lim.e2 -side left -fill x -expand true
  pack $y.lim -side top -fill x -anchor nw -padx $xpad

  set track [frame $y.track]
  label $track.l -text "Track Range?"
  radiobutton $track.true  -variable $track.trackRange -value 1 -text {true}
  radiobutton $track.false -variable $track.trackRange -value 0 -text {false}
  $track.false select

  pack $track.l -side left -anchor w
  pack $track.true -side left -anchor w
  pack $track.false -side left -anchor w
  pack $track -side top -fill x -anchor nw -padx $xpad 
 
  pack $y -side top -fill x -anchor nw -pady 1m -padx 1m

#
# Create a labeled entry widget for specifying a list of bits to be
# displayed.
#
  labeled_entry $w.bits {Bit numbers (eg. 0,1..): } {} -width 12
  pack $w.bits -side top -anchor w
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

proc killSelection {w text} {
  upvar $text localVar
  if [catch {
    set localVar [$w get [$w index insert] end]
    $w delete [$w index insert] end
  } result] {
  }
}

proc storeSelection {w text} {
  upvar $text localVar
  if [catch {
    set localVar [$w get [$w index sel.first] [$w index sel.last]]
  } result] {
  }
}

proc insertSelection {w text} {
  upvar $text localVar
  if [catch {
    $w insert insert $localVar
  } result] {
   puts $result
  }
}

#-----------------------------------------------------------------------
# Create a power-spectrum graph configuration dialog.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_powSpecGraph_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Graph} gcpViewer/plot/conf_graph
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::graph_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::graph_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::graph_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Specify the amount of padding to leave around the edge of the area.
#
  set xpad 2m
#
# Create an area in which the list of graph registers is selected. This
# contains a scrollable text widget in which to list the registers, and
# a menu listing recognized registers.
#
  set r [frame $w.r -relief groove -bd 2]
#
# Start with the scrollable text widget that will be used to list graph
# registers.
#
  label $r.title -text {Registers (one per line):} -fg TextLabelColor
  pack $r.title -side top -anchor nw
  frame $r.tf
  text $r.tf.t -width 20 -height 3 -wrap none \
      -yscrollcommand [list $r.tf.sy set] \
      -xscrollcommand [list $r.tf.sx set]
  bind $r.tf.t <Key-space> {break}
  bind $r.tf.t <Tab> {focus [tk_focusNext %W]; break}
  bind $r.tf.t <Shift-Tab> {focus [tk_focusPrev %W]; break}

# Bind the middle mouse button to copy
 
  set $r.tf.t.text ""
  bind $r.tf.t <ButtonRelease-1> "storeSelection $r.tf.t $r.tf.t.text"
  bind $r.tf.t <ButtonPress-2> "insertSelection $r.tf.t $r.tf.t.text"

  scrollbar $r.tf.sx -width 3m -orient horizontal \
      -command [list $r.tf.t xview]
  scrollbar $r.tf.sy -width 3m -orient vertical -command [list $r.tf.t yview]
  pack $r.tf.sy -side right -fill y
  pack $r.tf.sx -side bottom -fill x
  pack $r.tf.t -side left -fill x -expand true
  pack $r.tf -side left -anchor nw -fill x -expand true -padx 2m
#
# Now the menubutton that posts the list of known boards and their registers
# and directs any resulting user selection to the text widget.
#
  create_regmenu_button $r.mb $r.tf.t
  pack $r.mb -side top -anchor c -padx 1m
  pack $r -side top -fill x -anchor nw -expand true -pady 1m -padx 1m
#
# Encapsulate Y-axis configuration parameters.
#
  set y [frame $w.y -relief groove -bd 2]
  label $y.title -text {The configuration of the Y-axis:} -fg TextLabelColor
  pack $y.title -side top -anchor nw
#
# Next create a labelled entry widget for specifying the Y-axis label.
#
  frame $y.l
  label $y.l.l -text "Label: "
  entry $y.l.e -width 16
  pack $y.l.l -side left -anchor w
  pack $y.l.e -side left -fill x -expand true
  pack $y.l -side top -fill x -anchor nw -padx $xpad
#
# Now create the pair of Y-axis range, labelled entry widgets.
#
  frame $y.lim
  label $y.lim.l1 -text "Range: "
  entry $y.lim.e1 -width 8
  label $y.lim.l2 -text "to: "
  entry $y.lim.e2 -width 8

  pack $y.lim.l1 -side left -anchor w
  pack $y.lim.e1 -side left -fill x -expand true
  pack $y.lim.l2 -side left -anchor w
  pack $y.lim.e2 -side left -fill x -expand true
  pack $y.lim -side top -fill x -anchor nw -padx $xpad

  set track [frame $y.track]
  label $track.l -text "Track Range?"
  radiobutton $track.true  -variable $track.trackRange -value 1 -text {true}
  radiobutton $track.false -variable $track.trackRange -value 0 -text {false}

  $track.true select

  pack $track.l -side left -anchor w
  pack $track.true -side left -anchor w
  pack $track.false -side left -anchor w
  pack $track -side top -fill x -anchor nw -padx $xpad 

# When integrating a plot, do we want vector average, or rms averaging?

  set av [frame $y.av]
  label $av.l -text "Averaging type:"
  radiobutton $av.true  -variable $av.avVector -value 1 -text {vector}
  radiobutton $av.false -variable $av.avVector -value 0 -text {rms}

  $av.true select

  pack $av.l -side left -anchor w
  pack $av.true -side left -anchor w
  pack $av.false -side left -anchor w
  pack $av -side top -fill x -anchor nw -padx $xpad 
 
  pack $y -side top -fill x -anchor nw -pady 1m -padx 1m

# When displaying a graph, do we want linear or log plotting?

  set axis [frame $y.axis]
  label $axis.l -text "Axis type:"
  radiobutton $axis.true  -variable $axis.type -value 1 -text {linear}
  radiobutton $axis.false -variable $axis.type -value 0 -text {log}

  $axis.false select

  pack $axis.l -side left -anchor w
  pack $axis.true -side left -anchor w
  pack $axis.false -side left -anchor w
  pack $axis -side top -fill x -anchor nw -padx $xpad 

# When calculating FFTs, what type of apodization should we apply?

  set apod [frame $y.apod]

  button $apod.b -text "Apodization" -padx 2 -pady 2 -relief groove -bg palegreen -activebackground palegreen -takefocus 1 -command "toggleApodDisplay $y.apod"

  create_apodization_dialog $apod.d

  pack $apod.b $apod.d -side top -anchor w
  pack forget $apod.d

  pack $apod -side top -fill x -anchor nw -padx $xpad 
 
# Now pack the whole thing

  pack $y -side top -fill x -anchor nw -pady 1m -padx 1m

#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# Create a configuration dialog for setting y-limits for all graphs
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_all_graph_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure All Graphs} gcpViewer/plot/conf_all_graph
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::all_graph_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::all_graph_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::all_graph_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Specify the amount of padding to leave around the edge of the area.
#
  set xpad 2m

#
# Encapsulate Y-axis configuration parameters.
#
  set y [frame $w.y -relief groove -bd 2]
  label $y.title -text {The configuration of the Y-axis:} -fg TextLabelColor
  pack $y.title -side top -anchor nw
#
# Now create the pair of Y-axis range, labelled entry widgets.
#
  frame $y.lim
  label $y.lim.l1 -text "Range: "
  entry $y.lim.e1 -width 8
  label $y.lim.l2 -text "to: "
  entry $y.lim.e2 -width 8
  pack $y.lim.l1 -side left -anchor w
  pack $y.lim.e1 -side left -fill x -expand true
  pack $y.lim.l2 -side left -anchor w
  pack $y.lim.e2 -side left -fill x -expand true
  pack $y.lim -side top -fill x -anchor nw -padx $xpad
  pack $y -side top -fill x -anchor nw -pady 1m -padx 1m
#
# Create a labeled entry widget for specifying a list of bits to be
# displayed.
#
  labeled_entry $w.bits {Bit numbers (eg. 0,1..): } {} -width 12
  pack $w.bits -side top -anchor w
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# Create a configuration dialog for setting y-limits for all graphs
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_all_powSpecGraph_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure All Graphs} gcpViewer/plot/conf_all_powSpecGraph
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::all_graph_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::all_graph_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::all_graph_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Specify the amount of padding to leave around the edge of the area.
#
  set xpad 2m

#
# Encapsulate Y-axis configuration parameters.
#
  set y [frame $w.y -relief groove -bd 2]
  label $y.title -text {The configuration of the Y-axis:} -fg TextLabelColor
  pack $y.title -side top -anchor nw
#
# Now create the pair of Y-axis range, labelled entry widgets.
#
  frame $y.lim
  label $y.lim.l1 -text "Range: "
  entry $y.lim.e1 -width 8
  label $y.lim.l2 -text "to: "
  entry $y.lim.e2 -width 8
  pack $y.lim.l1 -side left -anchor w
  pack $y.lim.e1 -side left -fill x -expand true
  pack $y.lim.l2 -side left -anchor w
  pack $y.lim.e2 -side left -fill x -expand true
  pack $y.lim -side top -fill x -anchor nw -padx $xpad
  pack $y -side top -fill x -anchor nw -pady 1m -padx 1m
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# Create a plot configuration dialog.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_plot_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Plot} gcpViewer/plot/conf_plot
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command "set ::plot_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::plot_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::plot_dialog_state cancel"
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Create a labelled entry widget for specifying the plot title.
#
  labeled_entry $w.title {Plot title: } {} -width 25
  $w.title.l configure -fg TextLabelColor
#
# Encapsulate an X-axis configuration area.
#
  set x [frame $w.x -relief groove -bd 2]
  label $x.title -text {The configuration of the X-axis:} -fg TextLabelColor
  pack $x.title -side top -anchor nw
#
# Create an area in which the X-axis register is specified.
# Within it place the entry widget in which the register will be
# specified, and a menu from which registers can be selected.
#
  set xr [labeled_entry $x.xr {Register: } array.frame.utc.date -width 16]
  create_regmenu_button $xr.m $xr.e
  pack $xr.m -side left -padx 2m
  pack $xr -side top -anchor nw -padx 2m
#
# Create a labelled entry widget for specifying the X-axis label.
#
  labeled_entry $x.lab {Label: } {} -width 16
  pack $x.lab -side top -anchor nw -padx 2m
#
# Create two labelled entry widgets for specifying the X-axis limits.
#
  labeled_entry $x.min {xmin: } {} -width 18
  labeled_entry $x.max {xmax: } {} -width 18
  pack $x.min $x.max -side top -anchor w -padx 2m -fill x -expand true
#
# Create a labeled scale-widget for changing the scroll increment.
#
  set inc [frame $x.inc]
  label $inc.l -text {Scroll margin %:}
  scale $inc.sc -orient horizontal -showvalue true -from 1 -to 100 \
      -resolution 0.5
  pack $inc.l -side left -anchor sw
  pack $inc.sc -side left -fill x -expand true
  pack $inc -side top -anchor nw -padx 2m
#
# Create a horizontal-scroll disposition menu.
#
  option_menu $x.mode {Scrolling mode:} $::scroll_modes {}
  pack $x.mode -side top -anchor nw -padx 2m -pady 2
#
# Encapsulate controls that affect the appearance of the plotted data.
#
  set app [frame $w.app -relief groove -bd 2]
  label $app.title -text {Plotting attributes:} -fg TextLabelColor
  pack $app.title -side top -anchor nw
#
# Create a labelled entry widget for specifying the dot sizes.
#
  labeled_entry $app.point {Marker size: } 1 -width 5
  pack $app.point -side top -anchor nw -padx 2m
#
# Create a checkbutton for specifying whether to connect
# adjacent data-points with lines.
#
  checkbutton $app.join -variable $app.join -text {Draw lines between points} \
      -anchor w
  pack $app.join -side top -anchor nw -padx 2m

  pack $w.title $w.x $w.app -side top -anchor nw -fill x -pady 1m -padx 1m
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
}

#-----------------------------------------------------------------------
# Create a power spectrum plot configuration dialog.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_powSpecPlot_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Power Spectrum Plot} gcpViewer/plot/conf_powSpecPlot
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply  configure -command "set ::powSpecPlot_dialog_state apply"
  $dialog.bot.cancel configure -command "set ::powSpecPlot_dialog_state cancel"
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW "set ::powSpecPlot_dialog_state cancel"

#
# Get the path of the configuration area.
#
  set w $dialog.top

#
# Create a labelled entry widget for specifying the plot title.
#
  labeled_entry $w.title {Plot title: } {Power Spectrum Plot} -width 25
  $w.title.l configure -fg TextLabelColor

#
# Encapsulate an X-axis configuration area.
#
  set x [frame $w.x -relief groove -bd 2]
  label $x.title -text {The configuration of the transform:} -fg TextLabelColor
  pack $x.title -side top -anchor nw

#
# Create a labelled entry widget for specifying the X-axis label.
#
  labeled_entry $x.lab {Label: } {} -width 16
  pack $x.lab -side top -anchor nw -padx 2m

#
# Create two labelled entry widgets for specifying the X-axis limits.
#
  labeled_entry $x.npt {Npt: } {} -width 10
  labeled_entry $x.dx {dx (ms): } {} -width 10
  pack $x.npt $x.dx -side top -anchor w -padx 2m -fill x -expand true

#
# Create radio buttons for selecting the axis type (linear or logarithmic)
#
  set axis [frame $w.x.axis]
  label $axis.label -text "Axis type:"
  radiobutton $axis.linear -variable $axis.type -value 1 -text {linear}
  radiobutton $axis.log    -variable $axis.type -value 0 -text {log}

  $axis.linear select

  pack $axis.label  -side left -anchor w
  pack $axis.linear -side left -anchor w
  pack $axis.log    -side left -anchor w
  pack $axis -side top -fill x -anchor nw -pady 1m -padx 1m

#
# Encapsulate controls that affect the appearance of the plotted data.
#
  set app [frame $w.app -relief groove -bd 2]
  label $app.title -text {Plotting attributes:} -fg TextLabelColor
  pack $app.title -side top -anchor nw

#
# Create a labelled entry widget for specifying the dot sizes.
#
  labeled_entry $app.point {Marker size: } 1 -width 5
  pack $app.point -side top -anchor nw -padx 2m

#
# Create a checkbutton for specifying whether to connect
# adjacent data-points with lines.
#
  checkbutton $app.join -variable $app.join -text {Draw lines between points} \
      -anchor w
  pack $app.join -side top -anchor nw -padx 2m

  pack $w.title $w.x $w.app -side top -anchor nw -fill x -pady 1m -padx 1m

#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
}

#-----------------------------------------------------------------------
# Given an entry widget and a text value as its arguments this command
# replaces the current contents of the widget with the new value.
#
# Input:
#  w         The path name of the entry widget.
#  string    The new text to be placed in the entry field.
#-----------------------------------------------------------------------
proc replace_entry_contents {w string} {
#
# If the widget is marked as being in a normal sensitivity state then
# we can go ahead directly to delete and replace the contents. Otherwise
# we must change its state before replacing its contents, then reset
# its state to disabled.
#
  if {[string compare [$w cget -state] normal] == 0} {
    $w delete 0 end
    $w insert 0 $string
  } else {
    $w configure -state normal
    $w delete 0 end
    $w insert 0 $string
    $w configure -state disabled
  }
}

#-----------------------------------------------------------------------
# Create a message area widget.
#
# Input:
#  w        The path name to assign the widget.
# Output:
#  return   The value of w.
#-----------------------------------------------------------------------
proc create_message_widget {w} {
#
# Frame the area.
#
  frame $w -relief groove -bd 2
#
# A label will be used to display messages.
#
  label $w.l -relief sunken -bd 2 -anchor w
  pack $w.l -side left -anchor w -fill x -expand true
  return $w
}

#-----------------------------------------------------------------------
# Return the name of the plot widget that has a given C-layer tag.
#
# Input:
#  p_tag     The tag of the plot to be located.
# Output:
#  return    The name of the plot widget, or {} if not found.
#-----------------------------------------------------------------------
proc tag_to_plot {p_tag} {
  foreach plot [list_plots] {
    upvar #0 $plot p
    if {$p_tag == $p(tag)} {
      return $plot
    }
  }
  report_error {tag_to_plot: Invalid tag}
  return {}
}

#-----------------------------------------------------------------------
# Locate the graph that has a given C-layer tag.
#
# Input:
#  p_tag     The tag of the parent plot if known, otherwise send 0.
#  g_tag     The tag of the graph to be located.
# Output:
#  return    The name of the global graph configuration array, if found,
#            or {} otherwise.
#-----------------------------------------------------------------------
proc tag_to_graph {p_tag g_tag} {
#
# Get the list of plots to be searched.
#
  if {!$p_tag} {
    set plot_widgets [list_plots]
  } else {
    set plot_widgets [tag_to_plot $p_tag]
    if {[string compare $plot_widgets {}] == 0} {
      report_error {tag_to_graph: Invalid plot tag.}
      return {}
    }
  }
#
# Search for the graph tag in the specified list of plots.
#
  foreach plot $plot_widgets {
    upvar #0 $plot p
    foreach graph $p(graphs) {
      upvar #0 $graph g
      if {$g(tag) == $g_tag} {
	return $graph
      }
    }
  }
  report_error {tag_to_graph: Invalid graph tag}
}

#-----------------------------------------------------------------------
# Return a list of top-level plot widgets.
#-----------------------------------------------------------------------
proc list_plots {} {
  return $::viewer(plots)
}

#-----------------------------------------------------------------------
# Display a message in the message area of a given plot widget.
#
# Input:
#  plot     The path name of the plot.
#  msg      The message to be displayed.
#-----------------------------------------------------------------------
proc show_plot_message {plot msg} {
  $plot.msg.l configure -text $msg
}

#-----------------------------------------------------------------------
# Return 1 if the argument is a floating point number - possibly -ve.
#-----------------------------------------------------------------------
proc is_float {arg} {
  regexp "^\[ \t]*\[+-]?(\[0-9]+\[.]?\[0-9]*|\[.]\[0-9]+)(\[eE]\[+-]?\[0-9]+)?\[ \t]*\$" $arg
}
#-----------------------------------------------------------------------
# Return 1 if the argument is a floating point number >= 0.0.
#-----------------------------------------------------------------------
proc is_ufloat {arg} {
  regexp "^\[ \t]*(\[0-9]+\[.]?\[0-9]*|\[.]\[0-9]+)(\[eE]\[+-]?\[0-9]+)?\[ \t]*\$" $arg
}
#-----------------------------------------------------------------------
# Return 1 if the argument is a (signed) integer.
#-----------------------------------------------------------------------
proc is_int {arg} {
  regexp "^\[ \t]*\[+-]?\[0-9]+\[ \t]*\$" $arg
}
#-----------------------------------------------------------------------
# Return 1 if the argument is an unsigned integer.
#-----------------------------------------------------------------------
proc is_uint {arg} {
  regexp "^\[ \t]*\[0-9]+\[ \t]*\$" $arg
}
#-----------------------------------------------------------------------
# Return 1 if the argument is boolean.
#-----------------------------------------------------------------------
proc is_bool {arg} {
  regexp "^\[ \t]*\[01]\[ \t]*\$" $arg
}
#-----------------------------------------------------------------------
# Return 1 if the argument is a date and time.
#-----------------------------------------------------------------------
proc is_date_and_time {arg} {
  expr {![catch {monitor date_to_mjd $arg}]}
}

#-----------------------------------------------------------------------
# Delete a plot widget and its C counterpart.
#-----------------------------------------------------------------------
proc delete_plot {plot} {
  upvar #0 $plot p
#
# Get the C identification tag of the plot.
#
  if {$p(tag) != 0} {
    monitor remove_plot $p(tag)
  }
#
# Delete the plot widget.
#
  destroy $plot
#
# Delete the plot configuration array and its contents.
#
  foreach graph $p(graphs) {
    global $graph
    unset $graph
  }
  unset p
#
# Remove the plot from the list of plots managed by the viewer.
#
  set index [lsearch -exact $::viewer(plots) $plot]
  if {$index >= 0} {
    set ::viewer(plots) [lreplace $::viewer(plots) $index $index]
  }
}

#-----------------------------------------------------------------------
# Delete a graph configuration array and its C counterpart.
#
# Input:
#  graph   The global name of the graph configuration array to be
#          deleted.
#-----------------------------------------------------------------------
proc delete_graph {graph} {
  upvar #0 $graph g
  upvar #0 $g(plot) p
#
# Delete the C version of the graph.
#
  if {$g(tag) != 0} {
    monitor remove_graph $p(tag) $g(tag)
  }
#
# Locate the array in the list of configuration arrays of its parent
# plot and remove it.
#
  set index [lsearch -exact $p(graphs) $graph]
  if {$index >= 0} {
    set p(graphs) [lreplace $p(graphs) $index $index]
  }
#
# Delete the array.
#
  unset g
}

#-----------------------------------------------------------------------
# Integrate a graph 
#
# Input:
#  graph   The global name of the graph configuration array to be
#          deleted.
#-----------------------------------------------------------------------
proc integrate_graph {graph doint} {
  upvar #0 $graph g
  upvar #0 $g(plot) p
#
# Integrate the graph.
#
  if {$g(tag) != 0} {
    monitor integrate_graph $p(tag) $g(tag) $doint
  }
}

#-----------------------------------------------------------------------
# Delete a plot and reconfigure the C viewer to match.
#-----------------------------------------------------------------------
proc quit_plot {plot} {
  delete_plot $plot
  monitor reconfigure
}

#-----------------------------------------------------------------------
# Populate the main toplevel widget of the application.
#-----------------------------------------------------------------------
proc start_viewer {} {

#
# Get the viewer name
#

  control viewername_var ::viewername
  control expname_var ::expname

#
# Set the window-manager attributes of the window.
#
  wm title . "$::viewername"
  wm iconname . "$::viewername"
  wm protocol . WM_DELETE_WINDOW quit_application
#
# Create a global array of viewer configuration parameters.
#
  set ::viewer(plots) {}    ;# The list of plot widgets hosted by the viewer.
  set ::viewer(pages) {}    ;# The list of page widgets hosted by the viewer.
  set ::viewer(interval) 1           ;# The sub-sampling interval of the viewer.
  set ::viewer(bufsize) 500000       ;# The size of the monitoring buffer.
  set ::viewer(host) [info hostname] ;# The name of the control computer.
  set ::viewer(gateway) {}           ;# The name of the gateway computer (if any)
  set ::viewer(timeout) {}           ;# The name of the gateway computer (if any)
  set ::viewer(service) all          ;# The network service(s) to request from
                                     ;#  the control computer.
  set ::viewer(retry) 0              ;# If true, retry getting a connection if lost
  set ::viewer(cal_file) {}          ;# The calibration file.
  set ::viewer(arc_dir)  {}          ;# The directory containing archive files.
  set ::viewer(conf_dir) {}          ;# The directory containing configuration files.
  set ::viewer(conf_file) {cbass_south.stat}         ;# The configuration file.
  set ::viewer(conf_path) {}         ;# The full configuration file.
  set viewer(image) {}               ;# The image plot widget hosted by the viewer
  set ::viewer(belloff) true         ;# If false, sound the bell when printing a message to the message box

#
# Create dialogs.
#

  create_load_config_dialog      .load_config
  create_save_config_dialog      .save_config
  create_load_cal_dialog         .load_cal
  create_host_dialog             .host_dialog
  create_archive_dialog          .archive_dialog
  create_graph_dialog            .graph_dialog
  create_plot_dialog             .plot_dialog
  create_all_graph_dialog        .all_graph_dialog

  create_powSpecGraph_dialog     .powSpecGraph_dialog
  create_powSpecPlot_dialog      .powSpecPlot_dialog
  create_all_powSpecGraph_dialog .all_powSpecGraph_dialog

  create_viewer_dialog           .viewer_dialog
  create_offset_dialog           .offset
  create_grabber_dialog          .grabber

#
# Create an iconified widget for later display of pager conditions
#
  create_pager_widget .pagerWindow

#
# Create an iconified widget for later display of frame grabber images.
#
  create_image_widget .im

#
# Create an iconified widget for later display of the star plotter
#
  create_starplot_widget .starplot

#
# Create a menubar at the top of the window.
#
  create_viewer_menubar .bar

#
# Create a scrolled text widget for logging error messages.
#
  create_viewer_logger .log

#
# Create a status pane.
#
  create_status_pane .status

#
# Create a scrolled command-entry area.
#
  Cmd::create_widget .command -command "dispatch_command .command" \
      -prompt "$::viewername> "
  pack .bar -side top -anchor nw -fill x
  pack .status -side top -fill both -expand true
  pack .log -side top -fill both -expand true
  pack .command -side top -fill both -expand true
#
# Create an empty register menu. This will be used until a data source
# is next opened.
#
  create_regmenu
#
# Register a script to be called whenever the register map changes.
#

  monitor regmap_callback regmap_changed_callback

#
# Display a greeting message.
#
  show_message "Welcome to $::viewername"

#
# Attach callbacks to events related to the control connection to the
# control program.
#
  control log_array ::log_array
  trace variable ::log_array(text) w log_trace_callback

  control reply_array ::reply_array
  trace variable ::reply_array(text) w reply_trace_callback

  control sched_var ::sched_var
  trace variable ::sched_var w sched_trace_callback

  control archiver_var ::archiver_var
  trace variable ::archiver_var w archiver_trace_callback

  control ant_var ::ant_var
  trace variable ::ant_var w ant_trace_callback

  control pager_var ::allow_paging
  trace variable ::allow_paging w pager_trace_callback

  control pagerReg_var ::pagerRegister
  trace variable ::pagerRegister w pager_register_trace_callback

  control pageCond_array ::pageCond_array
  trace variable ::pageCond_array(text) w pageCond_trace_callback

  control cmdTimeout_array ::cmdTimeout_array
  trace variable ::cmdTimeout_array(active) w cmd_timeout_active_trace_callback
  trace variable ::cmdTimeout_array(seconds) w cmd_timeout_timeout_trace_callback

  control dataTimeout_array ::dataTimeout_array
  trace variable ::dataTimeout_array(active) w data_timeout_active_trace_callback
  trace variable ::dataTimeout_array(seconds) w data_timeout_timeout_trace_callback

# Set up frame grabber variable traces for remote variables

  control grabber_var channel ::grabber(rmtChannel)
  trace variable ::grabber(rmtChannel) w grabber_rmt_channel_trace_callback

  control grabber_var redraw ::grabber(rmtRedraw)
  trace variable ::grabber(rmtRedraw) w grabber_rmt_redraw_trace_callback

  control grabber_var reconf ::grabber(rmtReconf)
  trace variable ::grabber(rmtReconf) w grabber_rmt_reconf_trace_callback

  control grabber_var combine ::grabber(rmtCombine)
  trace variable ::grabber(rmtCombine) w grabber_rmt_combine_trace_callback

  control grabber_var ptel ::grabber(rmtPtel)
  trace variable ::grabber(rmtPtel) w grabber_rmt_ptel_trace_callback

  control grabber_var flatfield ::grabber(rmtFlatfieldType)
  trace variable ::grabber(rmtFlatfieldType) w grabber_rmt_flatfieldType_trace_callback

  control grabber_var fov ::grabber(rmtFov)
  trace variable ::grabber(rmtFov) w grabber_rmt_fov_trace_callback

  control grabber_var aspect ::grabber(rmtAspect)
  trace variable ::grabber(rmtAspect) w grabber_rmt_aspect_trace_callback

  control grabber_var collimation ::grabber(rmtCollimation)
  trace variable ::grabber(rmtCollimation) w grabber_rmt_collimation_trace_callback

  control grabber_var ximdir ::grabber(rmtXimdir)
  trace variable ::grabber(rmtXimdir) w grabber_rmt_ximdir_trace_callback

  control grabber_var yimdir ::grabber(rmtYimdir)
  trace variable ::grabber(rmtYimdir) w grabber_rmt_yimdir_trace_callback

  control grabber_var dkRotSense ::grabber(rmtDkRotSense)
  trace variable ::grabber(rmtDkRotSense) w grabber_rmt_dkRotSense_trace_callback

  control grabber_var xpeak ::grabber(rmtXpeak)
  control grabber_var ypeak ::grabber(rmtYpeak)
  trace variable ::grabber(rmtYpeak) w grabber_rmt_peak_trace_callback

  control grabber_var ixmin ::grabber(rmtIxmin)
  control grabber_var ixmax ::grabber(rmtIxmax)
  control grabber_var iymin ::grabber(rmtIymin)
  control grabber_var iymax ::grabber(rmtIymax)
  control grabber_var inc   ::grabber(rmtInc)
  trace variable ::grabber(rmtInc) w grabber_rmt_inc_trace_callback

  control grabber_var rem   ::grabber(rmtRem)
  trace variable ::grabber(rmtRem) w grabber_rmt_rem_trace_callback

  control host_callback control_host_callback

#
# If the environment variable which specifies the root of the
# CBI directory tree doesn't exist, emit a warning.
#
  if {![info exists ::env(GCP_DIR)]} {
    show_message "Warning: The GCP_DIR environment variable isn't set."
  }
#
# Look for command line switches, placing non-switch values in $args.
#
  set args [list]
  for {set iarg 0} {$iarg < $::argc} {incr iarg} {
    set arg [lindex $::argv $iarg]
    switch -glob -- $arg {
      -help {
	exit_with_command_line_help 0
      }
      -arcdir {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(arc_dir) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The $arg switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -bufsize {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(bufsize) [lindex $::argv [incr iarg]]
	  if {![is_uint $::viewer(bufsize)]} {
	    puts stderr "$::argv0: The buffer size must be a positive integer."
	    exit_with_command_line_help 1
	  }
	} else {
	  puts stderr "$::argv0: The $arg switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -calfile {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(cal_file) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The -calfile switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -confdir {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(conf_dir) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The -confdir switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -conffile {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(conf_file) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The -conffile switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -gateway {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(gateway) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The -gateway switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -timeout {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(timeout) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The -timeout switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -retry {
	if {$::argc >= $iarg + 2} {
	  set ::viewer(retry) [lindex $::argv [incr iarg]]
	} else {
	  puts stderr "$::argv0: The -retry switch needs an argument."
	  exit_with_command_line_help 1
	}
      }
      -* {
	puts stderr "$::argv0: Unknown command-line switch: $arg"
	exit_with_command_line_help 1
      }
      default {
	lappend args $arg
      }
    }
  }
#
# If no calibration file was explicitly specified, get the default cal file.
#
  if {[string length $::viewer(cal_file)] == 0} {
    if {[info exists ::env(GCP_DIR)]} {
      set ::viewer(cal_file) $::env(GCP_DIR)/control/conf/$::expname/cal
    }
  }
#
# If no archive directory was specified on the command line, see if
# one has been specified via the GCP_ARC_DIR environment variable.
#
  if {[string length $::viewer(arc_dir)]==0} {
    if {[info exists ::env(GCP_ARC_DIR)]} {
      set ::viewer(arc_dir) $::env(GCP_ARC_DIR)
    } else {
      set ::viewer(arc_dir) [pwd]
    }
  }
#
# If no configuration file directory was specified on the command line, 
# construct a default conf directory
#
  if {[string length $::viewer(conf_dir)] == 0} {
    if {[info exists ::env(GCP_DIR)]} {
      set ::viewer(conf_dir) $::env(GCP_DIR)/control/conf/$::expname/
    }
  }
#
# Construct the full pathname of the default configuration file from the conf directory and
# the default file name
#

  set ::viewer(conf_path) $::viewer(conf_dir)/$::viewer(conf_file)

#
# Write the default path into the save_config entry.  This is already done in 
# create_save_config proc
#
#  .save_config.top.f.e insert 0 $::viewer(conf_dir)

# And arrange that the ends of the default strings are displayed in both the load and save
# dialog boxes
#
  .save_config.top.f.e xview end
  .load_config.top.f.e xview end
#
# How many arguments are left?
#
  set nargs [llength $args]
#
# Does the user want to connect to a source of data?
#
  if {!$nargs == 0} {
#
# How should we get monitoring data?
#
    switch -- [lindex $args 0] {
      monitor {
	if {$nargs < 2 || $nargs > 3} {
	  puts stderr "$::argv0: Wrong number of arguments."
	  exit_with_command_line_help 1
	}
	set ::viewer(host) [lindex $args 1]
	puts "Attempting to connect to the control program on host: $::viewer(host)"
	connect_host

	if {$nargs > 2} {
	  load_config [lindex $args 2] 1
	}
      }
      read {
	if {$nargs != 4} {
	  puts stderr "$::argv0: Wrong number of arguments."
	  exit_with_command_line_help 1
	}
	replace_entry_contents .archive_dialog.ta.e [lindex $args 1]
	replace_entry_contents .archive_dialog.tb.e [lindex $args 2]
	open_archive .archive_dialog
	load_config [lindex $args 3] 1
      }
      configure {
	if {$nargs != 2} {
	  puts stderr "$::argv0: Wrong number of arguments."
	  exit_with_command_line_help 1
	}
	load_config [lindex $args 1] 1
      }
      default {
	puts stderr "$::argv0: Unrecognized first argument."
	exit_with_command_line_help 1
      }
    }
  }
}

#-----------------------------------------------------------------------
# Create the main menu bar of the application.
#
# Input:
#  w       The path name to give the menu bar.
#-----------------------------------------------------------------------
proc create_viewer_menubar {w} {
#
# Create a raised frame for the menubar.
#
  frame $w -relief raised -bd 2 -width 10c
#
# Create the file menu.
#
  menubutton $w.file -text File -menu $w.file.menu
  set m [menu $w.file.menu -tearoff 0]
  $m add command -label {Load Configuration File} \
      -command "map_dialog .load_config 0"
  $m add command -label {Save Configuration File} \
      -command "map_dialog .save_config 0"
  $m add separator
  $m add command -label {Connect to the control program} \
      -command "map_dialog .host_dialog 0"
  $m add command -label {Load archive data from disk} \
      -command "map_dialog .archive_dialog 0"
  $m add separator
  $m add command -label {Load Calibration File} \
      -command "map_dialog .load_cal 0"
  $m add separator
  $m add command -label {Quit} -command quit_application
#
# The Motif style guide specifies that the File menu must be the
# at the left edge of the menubar.
#
  pack $w.file -side left
#
# Create the configuration menu.
#
  menubutton $w.config -text Configure -menu $w.config.menu
  set m [menu $w.config.menu -tearoff 0]
  $m add command -label {Viewer}                  -command "show_viewer_dialog"
  $m add command -label {Add plot}                -command "show_plot_dialog"
  $m add command -label {Add power spectrum plot} -command "show_powSpecPlot_dialog"
  $m add command -label {Add page}                -command "add_page"
  pack $w.config -side left

#
# Create the utilities menu.
#
  menubutton $w.utils -text Utilities -menu $w.utils.menu
  set m [menu $w.utils.menu -tearoff 0]
#  $m add command -label {Optical Pointing} -command "map_dialog .offset 0"
  $m add command -label {Frame Grabber}     -command "map_dialog .grabber 0"
  $m add command -label {Pager Window}      -command "reveal .pagerWindow"  
  $m add command -label {Star Plot}         -command "reveal .starplot"

  pack $w.utils -side left
#
# Create the window-finder menu.
#
  menubutton $w.find -text {Find} -menu $w.find.menu
  set m [menu $w.find.menu -tearoff 0 -postcommand [list populate_window_menu $w.find.menu]]
  $m add command -label {Find Window} -state disabled
  $m add separator
  pack $w.find -side left
#
# Create the help menu.
#
  menubutton $w.help -text Help -menu $w.help.menu
  set m [menu $w.help.menu -tearoff 0]
  $m add command -label {The gcpViewer program} -command "show_help viewer/index"
  $m add command -label {Page windows} -command "show_help viewer/page/index"
  $m add command -label {Plot windows} -command "show_help viewer/plot/index"
  $m add command -label {Optical pointing} -command "show_help viewer/offset"
  $m add separator
  $m add command -label {Saving and restoring the configuration of the viewer} -command "show_help viewer/save_config"
  $m add command -label {Connecting to the control program} -command "show_help viewer/connect"
  $m add command -label {Displaying archive data} -command "show_help viewer/archive"
  $m add command -label {Configuring general properties of the viewer} -command "show_help viewer/conf_gen"
  $m add separator
  $m add command -label {The schedule queue window} -command "show_help viewer/schedule_window"
  $m add command -label {The archiving status window} -command "show_help viewer/archive_window"
  $m add command -label {The log window} -command "show_help viewer/log_window"
  $m add command -label {The command window} -command "show_help viewer/command_window"
  $m add command -label {The main menu} -command "show_help viewer/main_menu"
  $m add separator
  $m add command -label {Schedule Documentation} -command "show_help scheduleDocumentation/ScheduleIndex"
  $m add separator
  $m add command -label {The command list} -command "show_help CommandIndex"
  $m add command -label {The register list} -command "show_help registers"
  $m add command -label {The scheduling language} -command "show_help LanguageIndex"
  pack $w.help -side right

  return $w
}

#-----------------------------------------------------------------------
# Create a scrolled text widget for displaying messages from the
# the control program and script error messages.
#
# Input:
#  w        The path name to give the widget.
#-----------------------------------------------------------------------
proc create_viewer_logger {w} {
  frame $w -relief ridge -bd 2 -width 10c
  text $w.text -width 80 -height 10 -bg black \
      -fg green -takefocus 0 -yscrollcommand [list $w.sy set] \
      -xscrollcommand [list $w.sx set]
  scrollbar $w.sx -width 3m -orient horizontal -command [list $w.text xview]
  scrollbar $w.sy -width 3m -orient vertical -command [list $w.text yview]
  pack $w.sy -side right -fill y
  pack $w.sx -side bottom -fill x
  pack $w.text -side left -fill both -expand true
#
# Create a tag to be used when displaying error messages.
#
  $w.text tag configure bad -foreground red
  $w.text tag configure log -foreground yellow

  return $w
}

#-----------------------------------------------------------------------
# Create a window for displaying the archiver and scheduler statuses.
#
# Input:
#  w       The pathname to give the widget.
#-----------------------------------------------------------------------
proc create_status_pane {w} {
#
# Enclose the ensemble in a frame.
#
  frame $w
#
# Create a horizontally tiled region containing two listboxes and
# a scroll bar. The two listboxes will be scrolled by the same
# scroll bar. The leftmost listbox will display schedule queue indexes
# while the rightmost listbox will display the corresponding names of
# queued schedules.
#
  set lframe [frame $w.lframe -bd 2 -relief sunken -bg grey]

  set queue [frame $w.lframe.queue -bd 2 -relief sunken -bg grey]
  label   $queue.title -text {The schedule queue} -bg grey
  listbox $queue.indexes -width 2 -bg black -fg yellow -height 4 -bd 0 -takefocus 0
  listbox $queue.scripts -width 30 -height 4 -bd 0 -bg black -fg white -yscrollcommand [list $queue.y set] -takefocus 0
  scrollbar $queue.y -width 3m -orient vertical -command [list scroll_sched_queue $queue]
  pack $queue.title -side top
  pack $queue.indexes -side left -fill y
  pack $queue.scripts -side left -fill both -expand true
  pack $queue.y -side left -fill y
#
# Allocate the first entry in the listboxes for the currently running
# schedule.
#
  $queue.scripts insert 0 {*idle*}
  $queue.indexes insert 0 {->}

# Set up the display area for default antennas

  set ant [frame $w.lframe.ant -bg black]
  label $ant.label -bg black -fg yellow -text "Default antennas:"
  label $ant.antennas -bg black -fg white -text N/A
  pack $ant.label $ant.antennas -side left -anchor w

# Layout the schedule queue and default antenna widgets within the left frame

pack $queue -side top -fill both -expand true

# Comment out the default antenna area
#pack $ant -side bottom -fill both -expand true

#
# Layout the above widgets within the outer frame.
#
  pack $lframe -side left -fill both -expand true
#
# Create the menu that is displayed when one clicks the right mouse
# button over a list entry.
#
  set m [menu ${queue}_menu -tearoff 0]
  $m add command -label "Discard schedule" -command \
      [list set ::sched_menu_var remove]
  $m add command -label "Advance by one entry" -command \
      [list set ::sched_menu_var advance]
  $m add command -label "Retard by one entry" -command \
      [list set ::sched_menu_var retard]
#
# Create the menu that is displayed when one clicks the right mouse
# button over the running schedule.
#
  set m [menu ${queue}_run_menu -tearoff 0]
  $m add command -label "Abort schedule" -command \
      [list set ::sched_menu_var abort]
  $m add command -label "Suspend schedule" -command \
      [list set ::sched_menu_var suspend]
  $m add command -label "Resume schedule" -command \
      [list set ::sched_menu_var resume]
#
# Arrange for the appropriate one of the above menus to be posted and
# read whenever the user presses the right mouse button or space-bar
# over the list of schedules.
#
  bind $queue.scripts <space> "read_sched_menu $queue %X %Y %x %y"
  bind $queue.scripts <3> "read_sched_menu $queue %X %Y %x %y"
#
# Encapsulate the archiver status area.
#
  set arc [frame $w.arc -bg grey -relief sunken -bd 2]
  label $arc.title -bg grey -text {Archiving status}
#
# Create two frames side by side, one for parameter names, and
# another for the associated values.
#
  set left  [frame $arc.left  -bg black]
  set right [frame $arc.right -bg black]
#
# Create the parameter name labels.
#
  label $left.file -bg black -fg yellow -text "Archive file:"
  label $left.dir -bg black -fg yellow -text "Archive directory:"
  label $left.comb -bg black -fg yellow -text "Combine:"
  label $left.size -bg black -fg yellow -text "Max file size:"
  label $left.filter -bg black -fg yellow -text "Archive filtering:"
  pack $left.file $left.dir $left.comb $left.size $left.filter -side top -anchor w
#
# Create the corresponding value labels.
#
  label $right.file -bg black -fg white -text N/A
  label $right.dir -bg black -fg white -text N/A
  label $right.comb -bg black -fg white -text N/A
  label $right.size -bg black -fg white -text N/A
  label $right.filter -bg black -fg white -text N/A
  pack $right.file $right.dir $right.comb $right.size $right.filter -side top -anchor w

  pack $arc.title -side top -fill x
  pack $left $right -side left -expand true -fill both

  pack $arc -side top -fill both -expand true
}

#-----------------------------------------------------------------------
# The following function is called to post a menu of scheduling options
# over an entry in the schedule listbox.
#
# Input:
#  queue       The parent window.
#  rx ry       The root window coordinates of the cursor.
#  wx wy       The listbox window coordinates of the cursor.
#-----------------------------------------------------------------------
proc read_sched_menu {queue rx ry x y} {
#
# Determine which listbox menu is below the cursor.
#
  set index [$queue.scripts index @${x},${y}]
#
# Distinguish between the queued schedules and the currently running
# schedule.
#
  if {$index > 0} {
#
# Get the schedule-queue index.
#
    set entry [expr {$index - 1}]
#
# Post and read the schedule-queue menu.
#
    read_menu ${queue}_menu $rx $ry sched_menu_var
    catch {
      switch $::sched_menu_var {
	remove {
	  control send "remove_schedule $entry"
	}
	advance {
	  if {$entry > 0} {
	    control send "advance_schedule ${entry}, 1"
	  }
	}
	retard {
	  if {$index < [$queue.scripts index end]} {
	    control send "retard_schedule ${entry}, 1"
	  }
	}
      }
    }
  } else {
#
# Post and read the running-schedule menu.
#
    read_menu ${queue}_run_menu $rx $ry sched_menu_var
    catch {
      switch $::sched_menu_var {
	abort {
	  control send "abort_schedule"
	}
	suspend {
	  control send "suspend_schedule"
	}
	resume {
	  control send "resume_schedule"
	}
      }
    }
  }
}

#-----------------------------------------------------------------------
# Clear the schedule queue
#-----------------------------------------------------------------------                                                                                               
proc clear_schedules {queue} {
  $queue.scripts delete 0 end
  $queue.indexes delete 0 end

  $queue.scripts insert 0 {*idle*}
  $queue.indexes insert 0 {->}
}

#-----------------------------------------------------------------------
# This is a private scroll-callback function used to scroll the two
# schedule-queue listboxes created by create_sched_window. It simply
# passes on its arguments to those two windows.
#-----------------------------------------------------------------------
proc scroll_sched_queue {queue args} {
  eval $queue.indexes yview $args
  eval $queue.scripts yview $args
}

#-----------------------------------------------------------------------
# Append a schedule to the displayed queue of schedules.
#
# Input:
#  queue    The path name of the schedule display area.
#  name     The name of the schedule to add.
#-----------------------------------------------------------------------
proc append_schedule {queue name} {
  set index [$queue.scripts index end]
  if {$index < 0} {
    set index 1
  }
  $queue.scripts insert $index $name
  $queue.indexes insert $index [format {%2d} [expr $index - 1]]
}

#-----------------------------------------------------------------------
# Remove a given entry from the displayed queue of schedules.
#
# Input:
#  queue    The pathname of the schedule display area.
#  index    The queue index of the entry to be removed.
#-----------------------------------------------------------------------
proc remove_schedule {queue index} {
  if {$index < 0 || [expr $index + 1] >= [$queue.scripts index end]} {
    error {remove_schedule: Non-existent schedule queue entry}
  } else {
    $queue.scripts delete [expr {$index + 1}]
    $queue.indexes delete [expr [$queue.indexes index end] - 1]
  }
}

#-----------------------------------------------------------------------
# Move a given schedule within the displayed queue of schedules.
#
# Input:
#  queue    The pathname of the schedule display area.
#  src      The queue index of the entry to be moved.
#  dst      The queue index to which to move the above entry.
#-----------------------------------------------------------------------
proc move_schedule {queue src dst} {
#
# Get the number of entries currently in the list.
#
  set size [$queue.scripts index end]
#
# Increment the source and destination indexes to match the equivalent
# indexes in the list box.
#
  incr src
  incr dst
#
# Check that the source and destination indexes are in range.
#
  if {$src < 1 || $dst < 1 || $src >= $size || $dst >= $size} {
    error {move_schedule: Non-existent schedule queue entry}
  }
#
# Move the schedule entry on the display.
#
  set name [$queue.scripts get $src]
  $queue.scripts delete $src
  $queue.scripts insert $dst $name
}

#-----------------------------------------------------------------------
# Report the currently running schedule in the displayed queue of
# schedules.
#
# Input:
#  queue    The pathname of the schedule display area.
#  name     The name of the schedule.
#-----------------------------------------------------------------------
proc show_running_schedule {queue name} {
  $queue.scripts delete 0
  $queue.scripts insert 0 $name
}

#-----------------------------------------------------------------------
# Display the currently running schedule as being suspended.
#
# Input:
#  queue    The pathname of the schedule display area.
#-----------------------------------------------------------------------
proc show_schedule_suspended {queue} {
  $queue.indexes delete 0
  $queue.indexes insert 0 { S}
}

#-----------------------------------------------------------------------
# Display the currently running schedule as being resumed.
#
# Input:
#  queue    The pathname of the schedule display area.
#-----------------------------------------------------------------------
proc show_schedule_resumed {queue} {
  $queue.indexes delete 0
  $queue.indexes insert 0 {->}
}

#-----------------------------------------------------------------------
# This procedure is called whenever the user presses return in the
# command-input area.
#
# Input:
#  w      The command-line widget to read the command from.
#-----------------------------------------------------------------------
proc dispatch_command {w} {
#
# Get the new command line and remove leading and trailing spaces.
#
  set cmd [string trimleft [string trimright [Cmd::get $w]]]
#
# Filter commands to the appropriate sources.
#
  switch -regexp -- [string tolower $cmd] {
    {^-} {
      catch {eval Cmd::configure $w $cmd}
    }
    {^exit$} {
      quit_application
    }
    {^connect$} {
      connect_host
    }
    {^help ?.*$} {
      show_help [string range $cmd 5 end]
    }
    {^ *$} {
      # Ignore empty lines.
    }
    default {
      if [catch {control send $cmd} result] {
	report_error $result
      }
    }
  }
}

#-----------------------------------------------------------------------
# Display help on the specified subject.
#
# Input:
#  subject    The subject to be described, or {} to list all subjects.
#-----------------------------------------------------------------------
proc show_help {subject} {
#
# Get the name of the help index file.
#
  set help_index $::help_dir/index.html
#
# Check that the html help directory is accessible and that it contains
# at least the help-index file.
#
  if ![file exists $help_index] {
    report_error "The help index ($help_index) is not accessible."
    return
  }
#
# Find the path of the appropriate help file.
#
  if {$subject == {}} {
    set help_file $help_index
  } else {
    set help_file "$::help_dir/${subject}.html"
  }
#
# Does the help file exist?
#  
  if ![file exists $help_file] {
    report_error "No help is available on: $subject"
    return
  }
#
# If netscape is running, the following will tell it to display the
# requested page.
#
  if [catch {send-netscape openUrl(file:$help_file)}] {
#
# The request failed. Assume that this is because netscape isn't running
# yet, so try to start it.
#
    show_message "Starting browser."
    update idletasks
    if [catch {exec netscape file:$help_file &} result] {
      report_error $result
      return
    }
  }
}

#-----------------------------------------------------------------------
# This command is called to quit the application.
#-----------------------------------------------------------------------
proc quit_application {} {
  exit
}

#-----------------------------------------------------------------------
# This command is a wrapper around the bind command that binds both
# the specified key and its upper-case equivalent.
#
# Input:
#  w      The window to bind the event to.
#  key    The key to bind.
#  cmd    The callback script to assign to the event.
#-----------------------------------------------------------------------
proc bind_short_cut {w key cmd} {
  bind $w <[string tolower $key]> $cmd
  bind $w <[string toupper $key]> $cmd
}

#-----------------------------------------------------------------------
# This command is a wrapper around the bind command that binds both
# the specified key and its upper-case equivalent.
#
# Input:
#  w      The window to bind the event to.
#  key    The key to bind.
#  cmd    The callback script to assign to the event.
#-----------------------------------------------------------------------
proc bind_short_cut_shift {w key cmd} {
    bind $w <Shift-[string toupper $key]> $cmd
}

#-----------------------------------------------------------------------
# Display a message in the log widget of the main widget.
#
# Input:
#  bad      True if the message is to be displayed as an error message.
#  msg      The message to be displayed.
#-----------------------------------------------------------------------
proc show_message {msg {bad 0}} {
#
# Separate the message into lines.
#
  set lines [split $msg \n]
#
# Get the "line.char" index of the line just after the last displayed line
# and remove the ".char" part.
#
  regsub -- {\..*$} [.log.text index end] {} endline
#
# Determine the number of lines currently displayed, noting that line
# numbers start at 1.
#
  set nline [expr {$endline - 2}]
#
# Temporarily allow insertion and deletion of characters.
#
  .log.text configure -state normal
#
# If adding in the new set of lines will result in the max number of
# log lines being exceeded, delete the current contents of the log
#
  set limit 100
  set ntotal [expr [llength $lines] + $nline]
  if {$ntotal > $limit} {
    .log.text delete 1.0 [expr $ntotal - $limit + 1].0
  }
#
# Append each line to the text widget.
#
  if {$bad == 1} { # Error messages
    foreach line $lines {
      .log.text insert end "$line\n" bad
	if {$::viewer(belloff) == "false"} {
          bell
        }
    }
  } elseif {$bad == 2} { # Interactive log messages
    foreach line $lines {
      .log.text insert end "$line\n" log
    }

  } else {
    foreach line $lines { # Normal log messages
      .log.text insert end "$line\n"
    }
  }
#
# Disable insertion and deletion of characters so that the user
# can't edit the log text.
#
  .log.text configure -state disabled
#
# Make sure that the last line is visible.
#
  .log.text see end
}

#-----------------------------------------------------------------------
# Report an error.
#
# Input:
#  msg     The error message.
#-----------------------------------------------------------------------
proc report_error {msg} {
  show_message $msg 1
}

#-----------------------------------------------------------------------
# Display a single-line message in the message area of a plot widget.
#
# Input:
#  plot     The reporting plot.
#  msg      The message to be displayed.
#-----------------------------------------------------------------------
proc show_plot_message {plot msg} {
#
# Separate the message into lines.
#
  set lines [split $msg \n]
#
# Show the final line.
#
  $plot.msg.l configure -text [lindex $lines end]
}

#-----------------------------------------------------------------------
# Catch Tcl error messages and display them in the message window.
#-----------------------------------------------------------------------
proc bgerror {msg} {
  report_error "$msg"
}

#-----------------------------------------------------------------------
# Delete all plots.
#-----------------------------------------------------------------------
proc delete_plots {} {
#
# Delete plots that are attached to widgets.
#
  foreach plot [list_plots] {
    delete_plot $plot
  }
#
# Delete any remaining C plots.
#
  monitor delete_plots
  return
}

#-----------------------------------------------------------------------
# Reconfigure the viewer from a given configuration file. Configuration
# files are actually scripts that set up the various fields via Tcl
# commands provided below.
#
# Input:
#  file     The full name of the configuration file.
#  replace  If true, replace the existing configuration. If false add
#           to the existing configuration.
#-----------------------------------------------------------------------
proc load_config {file replace} {

# Don't do anything if the configuration file is just the default config directory

  set testStr $::viewer(conf_dir)/
  if {[string compare $file $testStr] == 0} {
    return
  }
    
#
# Discard the current plot and page hierarchies.
#

  if {$replace} {
    delete_plots
    delete_pages
  }

#
# Execute the configuration script.
#
  if [catch {source $file} result] {
    report_error "Error reading configuration file: $file"
    report_error "$result"
  }

  update idletasks
  monitor reconfigure
}

#-----------------------------------------------------------------------
# Load a new calibration file.
#
# Input:
#  file    The full name of the calibration file.
#-----------------------------------------------------------------------
proc load_cal {file} {
#
# If a calibration file was specified, and there is something to
# be calibrated, attempt to load it.
#
  if [monitor have_stream] {
    if {![regexp "^\[ \t]*\$" $file] &&
        [catch {monitor load_cal $file} result]} {
      report_error $result
    }
  }
}

#-----------------------------------------------------------------------
# Add a new graph to a given plot.
#
# Input:
#  plot      The pathname of the parent plot.
# Output:
#  return    The name of the global array that contains details about
#            the graph.
#-----------------------------------------------------------------------
proc add_powSpecGraph {plot {ymin 0} {ymax 1} {ylabel {}} {yregs {}} {bits {}} {track 1} {av 1} {axis 0} {apod 0}} {
    add_graph $plot $ymin $ymax $ylabel $yregs $bits $track $av $axis $apod
}

proc add_graph {plot {ymin 0} {ymax 1} {ylabel {}} {yregs {}} {bits {}} {track 0} {av 1} {axis 1} {apod 0}} {

  upvar #0 $plot p  ;# The configuration array of the parent plot.

#
# Get a unique name for the configuration array of the graph.
#

  set graph graph[incr ::graph_count]

#
# Make sure that the plot gets deleted on error.
#

  if [catch {
    upvar #0 $graph g
    set g(tag) [monitor add_graph $p(tag)] ;# The C-layer id of the graph.
    set g(plot)   $plot  ;# The parent plot of the graph.
    set g(yregs)  {}     ;# The list of newline-separated Y-axis registers.
    set g(bits)   {}     ;# The list of bits to be plotted.
    set g(ylabel) {}     ;# The y-axis label string.
    set g(ymin)   0      ;# The value at the lower edge of the Y-axis.
    set g(ymax)   1      ;# The value at the upper edge of the Y-axis.
    set g(zymin)  [list] ;# The y-axis list of nested lower zoom limits.
    set g(zymax)  [list] ;# The y-axis list of nested upper zoom limits.
    set g(track)  0      ;# True if tracking the user-specified range
    set g(av)     0      ;# True if doing rms averaging of power spectra
    set g(axis)   1      ;# True if plotting a linear axis for this graph
    set g(apod)   0      ;# Type of apodization to apply

#
# Establish the initial configuration of the graph.
#

    configure_graph $graph $ymin $ymax $ylabel $yregs $bits $track $av $axis $apod

  } result] {
    delete_graph $graph
    error $result
  }
#
# Add the name of the graph configuration array to the list of graph
# configurations of the parent plot.
#
  lappend p(graphs) $graph

  return $graph
}

#-----------------------------------------------------------------------
# Reconfigure a given graph.
#
# Input:
#  graph      The graph to be reconfigured.
#  ymin ymax  The lowest and highest values visible on the y-axis.
#  ylabel     The label to draw parallel to the y-axis.
#  yregs      The list of registers to display on the graph.
#  bits       A list of bit numbers to extract from each register and
#             display on the graph. Send an empty list if not relevant.
#-----------------------------------------------------------------------
proc configure_graph {graph ymin ymax ylabel yregs bits track av axis apod} {
  upvar #0 $graph g    ;# The configuration array of the graph.
  upvar #0 $g(plot) p  ;# The configuration array of the parent plot.

#
# Attempt to install the revised parameters.
#
# Is the graph currently zoomed?
#
  if {[llength $g(zymax)] > 0} {
    monitor configure_graph $p(tag) $g(tag) [lindex $g(zymin) end] \
	[lindex $g(zymax) end] $ylabel [join $yregs "\n"] $bits $track $av $axis $apod
  } else {
    monitor configure_graph $p(tag) $g(tag) $ymin $ymax $ylabel \
	[join $yregs "\n"] $bits $track $av $axis $apod
  }
#
# Record the parameters.
#
  set g(ymin)    $ymin
  set g(ymax)    $ymax
  set g(ylabel)  $ylabel
  set g(yregs)   $yregs
  set g(bits)    $bits
  set g(track)   $track
  set g(av)      $av
  set g(axis)    $axis
  set g(apod)    $apod
}

#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify the characteristics
# of a given graph.
#
# Input:
#  plot      The parent plot of the graph.
#  graph     The configuration array of the graph to be reconfigured,
#            or {} to create a new graph.
#-----------------------------------------------------------------------
proc show_graph_dialog {plot {graph {}}} {
#
# Create a new graph?
#
  if {[string length $graph]==0} {
    set provisional 1
    set graph [add_graph $plot]
    set cancel_command [list delete_graph $graph]
    upvar #0 $graph g
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the graph to be restored.
#
  } else {
    upvar #0 $graph g
    set provisional 0
    set cancel_command [list configure_graph $graph $g(ymin) $g(ymax) $g(ylabel) $g(yregs) $g(bits) $g(track) $g(av) $g(axis) $g(apod)]
  }
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::graph_dialog_state apply
#
# Get the path name of the graph-configuration dialog and its configuration
# area.
#
  set dialog .graph_dialog
  set w $dialog.top
#
# Copy the current graph configuration into the dialog.
#
  $w.r.tf.t delete 1.0 end
  $w.r.tf.t insert 1.0 [join $g(yregs) "\n"]
  replace_entry_contents $w.y.l.e $g(ylabel)
  replace_entry_contents $w.y.lim.e1 $g(ymin)
  replace_entry_contents $w.y.lim.e2 $g(ymax)
  replace_entry_contents $w.bits.e $g(bits)

  set ::$w.y.track.trackRange $g(track)

  if {$g(track)==1} {
    $w.y.track.true select
  } else {
    $w.y.track.false select
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
  tkwait variable graph_dialog_state   ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::graph_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {
      configure_graph $graph [$w.y.lim.e1 get] [$w.y.lim.e2 get] \
	  [$w.y.l.e get] [split [$w.r.tf.t get 1.0 end] "\n"] [$w.bits.e get] [set ::$w.y.track.trackRange] $g(av) $g(axis) $g(apod)
      monitor reconfigure
      set ::graph_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable graph_dialog_state   ;# Wait for the user again.
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
  if {[string compare $::graph_dialog_state cancel] == 0} {
#
# If the graph is provisional, delete it. Otherwise restore its original
# configuration.
#
    if {$provisional} {
      delete_graph $graph
      monitor reconfigure
      return {}
    } else {
      eval $cancel_command
      monitor reconfigure
    }
  }
  return $graph
}

#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify the characteristics
# of a given power spectrum graph
#
# Input:
#  plot      The parent plot of the graph.
#  graph     The configuration array of the graph to be reconfigured,
#            or {} to create a new graph.
#-----------------------------------------------------------------------
proc show_powSpecGraph_dialog {plot {graph {}}} {
#
# Create a new graph?
#
  if {[string length $graph]==0} {
    set provisional 1
    set graph [add_powSpecGraph $plot]
    set cancel_command [list delete_graph $graph]
    upvar #0 $graph g
#
# If the user subsequently presses the cancel button, arrange for the
# current configuration of the graph to be restored.
#
  } else {
    upvar #0 $graph g
    set provisional 0
    set cancel_command [list configure_graph $graph $g(ymin) $g(ymax) $g(ylabel) $g(yregs) $g(bits) $g(track) $g(av) $g(axis) $g(apod)]
  }
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::graph_dialog_state apply
#
# Get the path name of the graph-configuration dialog and its configuration
# area.
#
  set dialog .powSpecGraph_dialog
  set w $dialog.top
#
# Copy the current graph configuration into the dialog.
#
  $w.r.tf.t delete 1.0 end
  $w.r.tf.t insert 1.0 [join $g(yregs) "\n"]

  replace_entry_contents $w.y.l.e $g(ylabel)
  replace_entry_contents $w.y.lim.e1 $g(ymin)
  replace_entry_contents $w.y.lim.e2 $g(ymax)

  set ::$w.y.track.trackRange $g(track)

  if {$g(track)==1} {
    $w.y.track.true select
  } else {
    $w.y.track.false select
  }

  set ::$w.y.axis.type $g(axis)

  if {$g(axis)==1} {
    $w.y.axis.true select
  } else {
    $w.y.axis.false select
  }

  set ::$w.y.apod.d.type $g(axis)

  if {$g(apod)==0} {
    $w.y.apod.d.rectangular.rad select
  } elseif {$g(apod)==1} {
    $w.y.apod.d.triangular.rad select
  } elseif {$g(apod)==2} {
    $w.y.apod.d.hamming.rad select
  } elseif {$g(apod)==3} {
    $w.y.apod.d.hann.rad select
  } elseif {$g(apod)==4} {
    $w.y.apod.d.sine.rad select
  } elseif {$g(apod)==5} {
    $w.y.apod.d.sinc.rad select
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
  tkwait variable graph_dialog_state  ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::graph_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {
      configure_graph $graph [$w.y.lim.e1 get] [$w.y.lim.e2 get] \
	  [$w.y.l.e get] [split [$w.r.tf.t get 1.0 end] "\n"] $g(bits) [set ::$w.y.track.trackRange] [set ::$w.y.av.avVector] \
	  [set ::$w.y.axis.type] [set ::$w.y.apod.d.type]
      monitor reconfigure
      set ::graph_dialog_state "done"
    } result]} {
      dialog_error $dialog $result       ;# Exhibit the error message.
      tkwait variable graph_dialog_state ;# Wait for the user again.
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
  if {[string compare $::graph_dialog_state cancel] == 0} {
#
# If the graph is provisional, delete it. Otherwise restore its original
# configuration.
#
    if {$provisional} {
      delete_graph $graph
      monitor reconfigure
      return {}
    } else {
      eval $cancel_command
      monitor reconfigure
    }
  }
  return $graph
}

#-----------------------------------------------------------------------
# Expand the Y-axis limits of the specified plot and redraw it to show
# the extent of the buffered data.
#
# Input:
#  plot       The path name of the plot.
#-----------------------------------------------------------------------
proc unzoom_all {plot} {
    plot_all_x $plot
    unzoom_all_y $plot
}

#-----------------------------------------------------------------------
# Expand the Y-axis limits of the specified plot and redraw it to show
# the extent of the buffered data.
#
# Input:
#  plot       The path name of the plot.
#-----------------------------------------------------------------------
proc unzoom_all_y {plot} {
  upvar #0 $plot p         ;# The configuration array of the widget.
#
# Exploit the fact that unzoom_xaxis zooms out to the extent
# of the buffered data when there are no recorded zoom limits.
#
  foreach graph $p(graphs) {
    upvar #0 $graph g
    set g(zymin) [list]
    set g(zymax) [list]
    unzoom_yaxis $graph
  }
}

#-----------------------------------------------------------------------
# Zoom in along the y-axis of a graph.
#
# Input:
#  graph      The name of the configuration array of the graph.
#  ymin ymax  The range of Y-axis to display.
#-----------------------------------------------------------------------
proc zoom_yaxis {graph ymin ymax} {
  upvar #0 $graph g     ;# The conguration array of the graph.
#
# Don't zoom redundantly.
#
  if {[string compare [lindex $g(zymin) end] $ymin] == 0 && \
      [string compare [lindex $g(zymax) end] $ymax] == 0} {
    return
  }
#
# Append the new zoom limits to the current list.
#
  lappend g(zymin) $ymin
  lappend g(zymax) $ymax
#
# Draw the zoomed graph.
#
  configure_graph $graph $g(ymin) $g(ymax) $g(ylabel) $g(yregs) $g(bits) $g(track) $g(av) $g(axis) $g(apod)
  refresh_graph $graph
}

#-----------------------------------------------------------------------
# Unzoom the Y-axis of a given graph.
#
# Input:
#  graph    The graph to unzoom.
#-----------------------------------------------------------------------
proc unzoom_yaxis {graph} {
  upvar #0 $graph g     ;# The configuration array of the graph.
  upvar #0 $g(plot) p   ;# The configuration array of the parent plot.
#
# Is the graph active?
#
  if {$g(tag) != 0} {
#
# If the graph isn't currently zoomed, replace the zoom-out axis limits
# with extent of the buffered data.
#
    if {[llength $g(zymin)] == 0} {
      monitor get_y_autoscale $p(tag) $g(tag) g(ymin) g(ymax)
#
# If it is currently zoomed, zoom out to the next pair of limits
# that are on the x-axis zoom list, or to the unzoom limits if
# we have exhausted the list.
#
    } else {
      set g(zymin) [lreplace $g(zymin) end end]
      set g(zymax) [lreplace $g(zymax) end end]
    }
#
# Redraw the graph with the new y-axis limits.
#
    configure_graph $graph $g(ymin) $g(ymax) $g(ylabel) $g(yregs) $g(bits) $g(track) $g(av) $g(axis) $g(apod)
    refresh_graph $graph
  }
}

#-----------------------------------------------------------------------
# The following functions create post, manage and destroy the single
# menu of register names that is shared between all entry and text
# widgets that take register specifications as arguments.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Create a "button" that will cause the shared register menu to be posted
# and associated with a given target entry or text widget.
#
# Input:
#  mb       The name to give the button.
#  target   The entry or text widget to which selections from the
#           register menu should be dispatched when the menu is posted
#           by this button.
#-----------------------------------------------------------------------
proc create_regmenu_button {mb target} {
  button $mb -text "List" -padx 2 -pady 2 -relief groove -bg palegreen -activebackground palegreen -takefocus 1
  bind $mb <ButtonPress> [list read_regmenu %X %Y $target]
  bind $mb <Key-space> [list read_regmenu %X %Y $target]
  return $mb
}

#-----------------------------------------------------------------------
# Create the shared register menu (.regmenu). This should be called
# whenever the register map changes. If no register map is available
# a dummy register menu will be created with a single disabled entry
# that reports to the user that the register map is unavailable. All
# terminals of the register menu are directed to call the dispatch_regmenu
# procedure when selected.
#-----------------------------------------------------------------------
proc create_regmenu {} {
  set m .regmenu
#
# Delete the existing register menu and its descendants.
#
  if {[winfo exists $m]} {
    destroy $m
  }
#
# Create the register menu.
#
  menu $m -tearoff 0
#
# If no register map current exists, give the menu a disabled entry that
# reports the lack of registers.
#
  if ![monitor have_stream] {
    $m add command -label "(unavailable)" -state disabled
  } else {
    $m add command -label "Register Maps" -state disabled
    $m add separator
#
# Get the list of CBI boards.
#
    set regmaps [monitor list_regmaps]
    foreach regmap $regmaps {
#
# Add a cascade entry that cascades a submenu of the regmaps's boards
#
      $m add cascade -label $regmap -menu $m.$regmap
#
# Create the menu of board registers.
#
      set r [menu $m.$regmap -tearoff 0]
      $r add command -label "Boards" -state disabled
      $r add separator

      set boards [monitor list_boards $regmap]

      foreach board $boards {
#
# Add a cascade entry that cascades a submenu of the board's registers.
#
        $r add cascade -label $board -menu $r.$board
#
# Create the menu of board registers.
#
        set b [menu $r.$board -tearoff 0]
        $b add command -label "Registers" -state disabled
        $b add separator

	set ireg 0
	set iseg 0

        foreach reg [monitor list_regs $regmap $board] {
          set b [add_register $b $regmap $board $reg ireg iseg]

#	  $b add command -label $reg -command [list set ::regmenu_variable $regmap.$board.$reg]
        }

      }
    }
  }
}

proc add_register {blist regmap board reg ireg iseg} {

  upvar 1 $ireg iReg
  upvar 1 $iseg iSeg

  set iReg [expr $iReg + 1]

  if {$iReg > 30} {
    set iReg 0
    set iSeg [expr $iSeg + 1]

    $blist add cascade -label "more" -menu $blist.$iSeg

    set b [menu $blist.$iSeg -tearoff 0]
    $b add command -label "Registers" -state disabled
    $b add separator

    $b add command -label $reg -command [list set ::regmenu_variable $regmap.$board.$reg]

    return $b

  } else {
    $blist add command -label $reg -command [list set ::regmenu_variable $regmap.$board.$reg]
    return $blist
  }
}

#-----------------------------------------------------------------------
# Post a menu at a given position and wait for the user to select an
# entry from it, or for the menu to unpost itself. This routine relies
# on each terminal menu item writing to a global variable whenever an
# entry is selected. read_menu will set this variable to "" before the
# menu is posted, arrange for it to be touched if the menu unposts
# itself, and then wait for it to be written to before cleaning up and
# returning. The caller should use the final value of the variable to
# assertain what menu entry was selected. The special value "" means
# that no entry was selected.
#
# The recommended usage is as follows:
#
#  menu .m -tearoff 0
#  .m add command -label say_hello -command [list set ::myvar say_hello]
#  .m add command -label exit -command [list set ::myvar exit]
#  bind .some_widget <1> {post_example_menu .m %x %y}
#
#    ...other code...
#
#  proc post_example_menu {menu x y} {
#    read_menu $menu $x $y myvar
#    if {$::myvar != ""} {
#      switch $::myvar {
#      say_hello {puts Hello}
#      exit {exit}
#    }
#  }
#
# Input:
#  menu        The path name of the menu widget.
#  x y         The root-window coordinates at which to post the menu.
#  var         The name of the global variable (without :: prefix) that
#              is written to by each of the menu commands.
#-----------------------------------------------------------------------
proc read_menu {menu x y var} {
#
# Record any existing grab.
#
  set old_grab_window [grab current]
  if {$old_grab_window == ""} {
    set old_grab_status ""
  } else {
    set old_grab_status [grab status $old_grab_window]
    grab release $old_grab_window
  }
#
# Record any existing keyboard focus.
#
  set old_focus [focus]
#
# Initialize the menu variable to its unselected state.
#
  set ::$var ""
#
# Post the menu and wait for it to become visible.
#
  $menu post $x $y
  tkwait visibility $menu
#
# Give the menu keyboard focus and exclusive use to the mouse.
#
  focus $menu
  grab -global $menu
#
# Wait for a menu entry to be selected, or for the menu to unpost itself.
# In order to detect the menu being unposted, touch the menu variable, being
# careful not to overwrite the value written by a menu entry command.
#
  bind $menu <Unmap> "set ::$var \[set ::$var\]"
  tkwait variable ::$var
  bind $menu <Unmap> {}
#
# Restore the original input focus and grab window.
#
  catch {grab release $menu}
  if {$old_grab_window != ""} {
    catch {
      if {[string compare $old_grab_status global] == 0} {
	grab -global $old_grab_window
      } else {
	grab $old_grab_window
      }
    }
  }
  catch {focus $old_focus}
#
# Unpost the menu.
#
  $menu unpost
}

#-----------------------------------------------------------------------
# Post a register-selection menu, wait for the user to select an entry
# (or unpost the menu), and copy the selected register name into the
# given target text or entry widget.
#
# Input:
#  x y      The root window location at which to post the menu.
#  target   The name of the text of entry widget into which to
#           copy the selected register.
#-----------------------------------------------------------------------
proc read_regmenu {x y target} {
  read_menu .regmenu $x $y regmenu_variable
  if {$::regmenu_variable == ""} {
    return
  }
  switch [winfo class $target] {
    Entry {
#
# Replace the current contents of the entry widget with the new text.
#
      $target delete 0 end
      $target insert end $::regmenu_variable
    }

    Text {
#
# Place the new text on a new line at the end of the contents of
# a text widget.
#
      if {[regexp "^\[ \t]*\$" [$target get "end -1 lines linestart" "end -1 lines lineend"]]} {
	$target delete "end linestart" "end lineend"
      } else {
	$target insert end "\n"
      }
      $target insert end $::regmenu_variable
      $target see end
    }
  }
}

#-----------------------------------------------------------------------
# Establish network connections to the control and/or monitoring
# services of the control program.
#-----------------------------------------------------------------------
proc connect_host {{host {}} {discard {}} {service {}} {retry {}} {gateway {}} {timeout {}}} {

#
# Substitute for unspecified procedure arguments.
#
  if {[string length $host] == 0} {
    set host $::viewer(host)
  }

  if {[string length $gateway] == 0} {
    set gateway $::viewer(gateway)
  }

  if {[string length $timeout] == 0} {
    set timeout $::viewer(timeout)
  }

  if {[string length $discard] == 0} {
    set discard [set ::.host_dialog.discard]
  }

  if {[string length $service] == 0} {
    set service $::viewer(service)
  }

  if {[string length $retry] == 0} {
    set retry $::viewer(retry)
  }

#
# Which services are desired?
#
  set control 0
  set monitor 0
  switch -- $service {
    control {set control 1}
    monitor {set monitor 1}
    all {set monitor 1; set control 1}
  }

#
# If requested, attempt to establish a control connection.
#
  if $control {
    if [catch {control host $host $retry $gateway $timeout} result] {
      report_error $result
      return
    }

    # Clear the schedule queue

    clear_schedules .status.lframe.queue
  }
#
# Attempt to establish a monitor connection.
# Note that if this fails, the existing stream will be left connected by
# the "monitor host" command.
#
  if $monitor {
    monitor eos_callback {}
    if [catch {monitor host $host $retry $gateway $timeout} result] {
      report_error $result
#
# The error shouldn't be treated as fatal unless it prevented the
# stream from being opened.
#
      if {![monitor have_stream]} return
    }
#
# Reselect the current registers.
#
    if {[catch {monitor reconfigure} result]} {
      report_error $result
    }
#
# If a calibration file was specified, attempt to load it.
#
    if [catch {load_cal $::viewer(cal_file)} result] {
      report_error $result
    }
#
# If a configuration file was specified, attempt to load it.
#
    if [catch {load_config $::viewer(conf_path) 0} result] {
      report_error $result
    }
#
# Discard buffered data from the last monitor stream?
#
    if {$discard} {
      monitor clr_buffer
    }
#
# Specify the sub-sampling interval.
#
    monitor set_interval $::viewer(interval)
  }
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::log_array(text) variable, such that it is called whenever a new log
# message is received from the control program.
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc log_trace_callback {args} {
  show_message "$::log_array(text)" $::log_array(is_error_msg)
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::reply_array(text) variable, such that it is called whenever a new reply
# message is received from the control program.
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc reply_trace_callback {args} {
  Cmd::output .command $::reply_array(text)
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::sched_var variable, such that it is called whenever a new status
# message is received from the scheduler.
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc pageCond_trace_callback {args} {

  if { $::pageCond_array(add) } {
    insertPagerMessage $::pageCond_array(text)
  } else {
    clearPagerMessages
  }

}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::allow_paging variable, such that it is called whenever a new status
# message is received about the paging status.  In particular, if paging has
# been re-enabled, this tells the monitor to reset the counters for all
# page-enabled variables
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc cmd_timeout_active_trace_callback {args} {

  if { $::cmdTimeout_array(active) == "1" } {
      .pagerWindow.main.cmdTimeoutLabel.state configure -text "Enabled" -bg black
  } else {
      .pagerWindow.main.cmdTimeoutLabel.state configure -text "Disabled" -bg red
  }
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::allow_paging variable, such that it is called whenever a new status
# message is received about the paging status.  In particular, if paging has
# been re-enabled, this tells the monitor to reset the counters for all
# page-enabled variables
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc cmd_timeout_timeout_trace_callback {args} {
    set timeout [concat $::cmdTimeout_array(seconds) " seconds"]
    .pagerWindow.main.cmdTimeoutLabel.timeoutSeconds configure -text $timeout -bg black
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::allow_paging variable, such that it is called whenever a new status
# message is received about the paging status.  In particular, if paging has
# been re-enabled, this tells the monitor to reset the counters for all
# page-enabled variables
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc data_timeout_active_trace_callback {args} {

  if { $::dataTimeout_array(active) == "1" } {
      .pagerWindow.main.dataTimeoutLabel.state configure -text "Enabled" -bg black
  } else {
      .pagerWindow.main.dataTimeoutLabel.state configure -text "Disabled" -bg red
  }
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::allow_paging variable, such that it is called whenever a new status
# message is received about the paging status.  In particular, if paging has
# been re-enabled, this tells the monitor to reset the counters for all
# page-enabled variables
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc data_timeout_timeout_trace_callback {args} {
    set timeout [concat $::dataTimeout_array(seconds) " seconds"]
    .pagerWindow.main.dataTimeoutLabel.timeoutSeconds configure -text $timeout -bg black
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::sched_var variable, such that it is called whenever a new status
# message is received from the scheduler.
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc sched_trace_callback {args} {
  switch -regexp -- $::sched_var {
    {^ADD } {
      append_schedule .status.lframe.queue [string range $::sched_var 4 end]
    }
    {^RM } {
      remove_schedule .status.lframe.queue [string range $::sched_var 3 end]
    }
    {^MV } {
      scan $::sched_var {MV %d %d} src dst
      move_schedule .status.lframe.queue $src $dst
    }
    {^RUN } {
      show_running_schedule .status.lframe.queue [string range $::sched_var 4 end]
    }
    {^IDLE} {
      show_running_schedule .status.lframe.queue {*idle*}
    }
    {^SUSPEND} {
      show_schedule_suspended .status.lframe.queue
    }
    {^RESUME} {
      show_schedule_resumed .status.lframe.queue
    }
  }
}
#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::archiver_var variable, such that it is called whenever a new status
# message is received from the archiver.
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc archiver_trace_callback {args} {
  switch -regexp -- $::archiver_var {
    {^OPEN } {
      .status.arc.right.file configure -bg black -text [string range $::archiver_var 5 end]
    }
    {^CLOSE} {
      .status.arc.right.file configure -bg red -text (none)
    }
    {^SAMPLE } {
      .status.arc.right.comb configure -text [string range $::archiver_var 7 end]
    }
    {^SIZE } {
      .status.arc.right.size configure -text [string range $::archiver_var 5 end]
    }
    {^DIR } {
      .status.arc.right.dir configure -text [string range $::archiver_var 4 end]
    }
    {^FILTER } {
      .status.arc.right.filter configure -text [string range $::archiver_var 7 end]
    }
  }
}

proc clear_archiver {arc} {
  $arc.right.file   configure -text N/A
  $arc.right.dir    configure -text N/A
  $arc.right.comb   configure -text N/A
  $arc.right.size   configure -text N/A
  $arc.right.filter configure -text N/A
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::allow_paging variable, such that it is called whenever a new status
# message is received about the paging status.  In particular, if paging has
# been re-enabled, this tells the monitor to reset the counters for all
# page-enabled variables
#
#  args    Ignored arguments.
#-----------------------------------------------------------------------
proc pager_trace_callback {args} {

    if {$::allow_paging} {
        monitor reset_counters
       .pagerWindow.main.pagerLabel.state configure -text "Enabled" -bg black
        restorePagerConfiguration
    } else {
       .pagerWindow.main.pagerLabel.state configure -text "Disabled" -bg red
    }
}

proc restorePagerConfiguration {args} {

    set last [.pagerWindow.main.text.text index end]

    if {[catch {

        for {set i 0} {$i < $last} {incr i} {
            .pagerWindow.main.text.text itemconfigure $i -background black
            .pagerWindow.main.text.text itemconfigure $i -foreground green
        }

# And hide the active page message

        pack forget .pagerWindow.main.activePageLabel

    } result]} {
        puts $result
    }

}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::pagerRegister variable, such that it is called whenever a new
# status message is received about a register that activated the pager
#-----------------------------------------------------------------------
proc pager_register_trace_callback {args} {
    set last [.pagerWindow.main.text.text index end]

    set reg $::pagerRegister
    convertRegString reg

    # Configure the active page label to reflect the condition that
    # activated the pager
    
    .pagerWindow.main.activePageLabel.label configure -text $reg
    
    # And reveal the window
    
    pack .pagerWindow.main.activePageLabel -after .pagerWindow.main.dataTimeoutLabel
    
    if {[catch {

        for {set i 0} {$i < $last} {incr i} {

            set cond [.pagerWindow.main.text.text get $i]
            set len [string length $cond]

            set firstNonSpace 0
            for {set iChar 0} {$iChar < $len} {incr iChar} {
                if {![string is space [string index $cond $iChar]]} {
                    set firstNonSpace $iChar
                    break;
                }
            }

            set lastNonSpace $firstNonSpace
            for {set iChar $firstNonSpace} {$iChar < $len} {incr iChar} {
                if {[string is space [string index $cond $iChar]]} {
                    set lastNonSpace [expr {$iChar - 1}]
                    break;
                }
            }

            set regMatch [string range $cond $firstNonSpace $lastNonSpace]
            if {[string compare $regMatch $reg]==0} {
                .pagerWindow.main.text.text itemconfigure $i -background yellow
                .pagerWindow.main.text.text itemconfigure $i -foreground black
            }
        }

    } result]} {
        puts $result
    }
}

#-----------------------------------------------------------------------
# The following procedure is registered as a trace callback for the
# ::ant_var variable, such that it is called whenever a new status
# message is received about the default antenna selection.  
#-----------------------------------------------------------------------
proc ant_trace_callback {args} {
 .status.lframe.ant.antennas configure -text [string range $::ant_var 0 end]
}

#-----------------------------------------------------------------------
# The following procedure is registered as a callback that is called
# whenever the connection to the control program is successfully opened
# or broken.
#-----------------------------------------------------------------------
proc control_host_callback {} {
  set host [control host];
  if {[string length $host] == 0} {
      set mess1 "Control connection lost to $::expname"
      set mess2 "Control"
      report_error "$mess1$mess2"

# If the grabber window is open, kill it now

      wm withdraw .grabber

# Clear the schedule queue

      clear_schedules .status.lframe.queue
      clear_archiver  .status.arc
      clear_pager     .pagerWindow

# And notify the control layer that we've lost connection

      control status 0

  } else {
      set mess1 "Connection established to $::expname"
      set mess2 "Control at host: $host"
      show_message "$mess1$mess2"

      control status 1
  }
}

#-----------------------------------------------------------------------
# The following procedure is called by the monitor interface whenever
# the register map changes.
#-----------------------------------------------------------------------
proc regmap_changed_callback {} {
  create_regmenu
#
# Reconfigure the child pages to accomodate any changes
# in the register map.
#
  foreach page $::viewer(pages) {
    update_page $page
  }
#
# Reconfigure the offset dialog if mapped.
#
  update_offset_dialog_regs .offset
#
# Reselect the current registers from the new register map.
#
  if {[catch {monitor reconfigure} result]} {
    report_error $result
  }
}

#-----------------------------------------------------------------------
# Create the frame-work of a configuration dialog.
#
# This is split into a top section and a bottom section.
# The top section $w.top will contain nothing until the caller puts
# something there. The bottom section contains two buttons,
# $w.bot.apply and $w.bot.cancel. The "cancel" button
# is assigned the command {wm withdraw $w}. The apply button should be
# set by the caller.
#
# Note that the dialog is not initially mapped. To display it temporarily
# use the command {wm deiconify $w} and then when it is no longer required
# call {wm withdraw $w}.
#
# Input:
#  w        The name to give the widget.
#  title    The title to give the dialog.
# Optional input:
#  help     The help page to display when the help button is pressed.
#           This should have the .html suffix omitted.
# Output:
#  return   The value of $w.
#-----------------------------------------------------------------------
proc create_config_dialog {w title {help {}}} {
#
# Create the toplevel dialog window but don't map it.
#
  toplevel $w -class dialog
  wm withdraw $w
  wm title $w $title
  wm iconname $w Dialog
  wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"
#
# Create the top and bottom frames.
#
  frame $w.top -relief raised -bd 1
  pack $w.top -side top -fill both -expand 1
  frame $w.bot -relief raised -bd 1
  pack $w.bot -side bottom -fill both -expand 1
#
# Create three buttons in the bottom frame.
#
  button $w.bot.apply -text Apply
  button $w.bot.cancel -text Cancel -command "wm withdraw $w"
  pack $w.bot.apply $w.bot.cancel -side left -expand 1 -pady 2m
  if {[string compare $help {}] != 0} {
    button $w.bot.help -text Help -command "show_help $help"
    pack $w.bot.help -side left -expand 1 -pady 2m
  }
  return $w
}

#-----------------------------------------------------------------------
# Create a dialog for loading plot configuration files.
#
# Input:
#  w      The Tk path name to give the dialog. This must be a name
#         suitable for a toplevel dialog (ie "^\.[a-zA-Z_]*$").
# Output:
#  return The value of $w.
#-----------------------------------------------------------------------
proc create_load_config_dialog {w} {
#  global ::viewer(conf_path)
  create_config_dialog $w {Load Configuration} gcpViewer/save_config
#
# Create a labeled entry widget $w.f.e.
#
  set f [labeled_entry $w.top.f {File name} {} -width 20 -textvariable ::viewer(conf_path)]
  pack $f

# Insert a default path

  if {[info exists ::env(GCP_DIR)]} {
      $f.e insert 0 $::env(GCP_DIR)/control/conf/$::expname/
      $f.e xview end
  }

#
# Since we want to be able to either add to or replace the existing
# configuration, replace the "Apply" button of the dialog with two buttons.
#
  destroy $w.bot.apply
  button $w.bot.replace -text Replace
  button $w.bot.merge -text Merge
  pack $w.bot.replace $w.bot.merge -side left -before $w.bot.cancel -expand 1 -pady 2m
# By default, hitting Return in the load_configuration dialog box will merge the
# new configuration file, NOT replace it.
  bind $f.e <Return> "$w.bot.merge invoke"
#
# Bind the replace and merge buttons to load the specified file.
#
  $w.bot.replace configure -command {load_config $::viewer(conf_path) 1; wm withdraw .load_config}
  $w.bot.merge   configure -command {load_config $::viewer(conf_path) 0; wm withdraw .load_config}
}

#-----------------------------------------------------------------------
# Create a dialog for saving plot configuration files.
#
# Input:
#  w      The Tk path name to give the dialog. This must be a name
#         suitable for a toplevel dialog (ie "^\.[a-zA-Z_]*$").
# Output:
#  return The value of $w.
#-----------------------------------------------------------------------
proc create_save_config_dialog {w} {
  create_config_dialog $w {Save Configuration} gcpViewer/save_config
#
# Create a labeled entry widget $w.f.e.
#
  set f [labeled_entry $w.top.f {File name} {} -width 20]
  bind $f.e <Return> "$w.bot.apply invoke"
  pack $f

# Insert a default path

  if {[info exists ::env(GCP_DIR)]} {
      $f.e insert 0 $::env(GCP_DIR)/control/conf/$::expname/
      $f.e xview end
  }

#
# Bind the ok button to load the specified file.
#
  $w.bot.apply configure -command "save_config \[$f.e get\]; wm withdraw .save_config"
}

#-----------------------------------------------------------------------
# Create a dialog for loading register calibration files.
#
# Input:
#  w      The Tk path name to give the dialog. This must be a name
#         suitable for a toplevel dialog (ie "^\.[a-zA-Z_]*$").
# Output:
#  return The value of $w.
#-----------------------------------------------------------------------
proc create_load_cal_dialog {w} {
  create_config_dialog $w {Load Calibration}
#
# Create a labeled entry widget $w.f.e.
#
  set f [labeled_entry $w.top.f {File name} {} -width 30 -textvariable ::viewer(cal_file)]
  bind $f.e <Return> "$w.bot.apply invoke"
  pack $f
#
# Bind the ok button to load the specified file.
#
  $w.bot.apply configure -command {load_cal $::viewer(cal_file); wm withdraw .load_cal}
}

#-----------------------------------------------------------------------
# Create a dialog for connecting to the real-time data stream of the
# control program.
#
# Input:
#  w      The Tk path name to give the dialog. This must be a name
#         suitable for a toplevel dialog (ie "^\.[a-zA-Z_]*$").
# Output:
#  return The value of $w.
#-----------------------------------------------------------------------
proc create_host_dialog {w} {
  create_config_dialog $w {Connect Host} gcpViewer/connect
#
# Create an entry widget for specifying the name of the control computer.
# Have the host name reflected in the $::viewer(host) variable.
#
  labeled_entry $w.host {Control Computer} $::viewer(host) -width 20 \
      -textvariable ::viewer(host)

  bind $w.host.e <Return> "$w.bot.apply invoke"

#
# Create widget for firewall setting
#
  button $w.fwb -text "Firewall settings" -padx 2 -pady 2 -relief groove -bg palegreen -activebackground palegreen -takefocus 1 -command "toggleFirewall $w"

  set f [frame $w.firewall -bd 2 -relief groove]

  labeled_entry $f.gateway {Gateway Computer} $::viewer(gateway) -width 20 \
      -textvariable ::viewer(gateway)

  labeled_entry $f.timeout {Timeout (seconds)} $::viewer(timeout) -width 5 \
      -textvariable ::viewer(timeout)

  $f.timeout.e insert 0 {10}
  pack $f.gateway $f.timeout -side top -anchor nw -expand true -fill x

#
# Create a framed area of radio buttons, used to select the services
# to connect to. Link the selected value to the ::viewer(service)
# variable.
#
  set f [frame $w.serve -bd 2 -relief groove]
  label $f.title -fg AlternateLabelColor -text {The desired service:}
  radiobutton $f.control -text {Telescope control} -value control \
      -variable ::viewer(service)
  radiobutton $f.monitor -text {Real-time monitoring} -value monitor \
      -variable ::viewer(service)
  radiobutton $f.all -text {Both of the above} -value all \
      -variable ::viewer(service)
  set ::viewer(service) all

#
# Give the user the choice of whether to automatically try to
# reconnect if the connection is lost
#
  checkbutton $f.retry -text {Automatically reconnect if the connection is lost?} \
      -variable ::viewer(retry)
  set ::viewer(retry) 0

  pack $f.title -side top -anchor w
  pack $f.control $f.monitor $f.all $f.retry -side top -anchor w -padx 5

#
# Create an entry widget for specifying the calibration file to
# use when connecting for real-time monitoring. Link the value of
# the entry widget to the ::viewer(cal_file) variable.
#
  labeled_entry $w.cal {Calibration file} {} -width 30 \
      -textvariable ::viewer(cal_file)

#
# Give the user the choice of whether to keep previously plotted
# data.
#
  checkbutton $w.discard -text {Discard previously plotted data?} \
      -variable ::$w.discard
  set ::$w.discard 1
#
# Layout the widgets from top to bottom.
#
  $w.cal.e xview end

  pack $w.host -side top -anchor nw -expand true -fill x
  pack $w.fwb -side top -anchor w 
  pack $w.firewall $w.serve $w.cal $w.discard -side top -anchor nw -expand true -fill x
  pack forget $w.firewall
  set ::firewall(display) 0

#
# Bind the ok button to load the specified file.
#
  $w.bot.apply configure -command "connect_host; wm withdraw $w"
}

#-----------------------------------------------------------------------
# Procedure for toggling display of firewall settings
#-----------------------------------------------------------------------
proc toggleFirewall {w} {

  if { $::firewall(display) } {
    set ::firewall(display) 0
    pack forget $w.firewall
  } else {
    set ::firewall(display) 1
    pack forget $w.host $w.fwb $w.firewall $w.serve $w.cal $w.discard

    pack $w.host -side top -anchor nw -expand true -fill x
    pack $w.fwb -side top -anchor w 
    pack $w.firewall $w.serve $w.cal $w.discard -side top -anchor nw -expand true -fill x
  }

}

#-----------------------------------------------------------------------
# Create a dialog for loading archive data from a given directory.
#
# Input:
#  w      The Tk path name to give the dialog. This must be a name
#         suitable for a toplevel dialog (ie "^\.[a-zA-Z_]*$").
# Output:
#  return The value of $w.
#-----------------------------------------------------------------------
proc create_archive_dialog {w} {
  create_config_dialog $w {Load archived data} gcpViewer/archive
#
# Create a string containing the current UTC in the format expected by
# the "monitor disk" command.
#
  set utc [clock format [clock seconds] -format {%d-%b-%Y %H:%M} -gmt true]
#
# Give a bit of explanation.
#
  frame $w.exp -relief raised -bd 1
  label $w.exp.t -text \
"Note that times are specified like DD-MMM-YYYY:HH:MM:SS. A blank
start or end time requests the corresponding time extreme of the archive.
To have live monitoring start after reaching the end of the archive,
specify the control program host name in the end UTC field, instead of a time."
  pack $w.exp.t -fill both
  pack $w.exp -side top -fill x
  labeled_entry $w.dir {The archive directory} {} -width 35 -textvariable ::viewer(arc_dir)
  frame $w.t -relief groove -bd 2
  labeled_entry $w.ta {The start UTC} $utc -width 35
  labeled_entry $w.tb {The end UTC} $utc -width 35

  labeled_entry $w.cal {Calibration file} {} -textvariable ::viewer(cal_file) -width 35

  labeled_entry $w.interval {Sub-sampling interval} {1} -width 35
  checkbutton $w.live -text {Plot data while loading?} -variable ::$w.live
  checkbutton $w.discard -text {Discard previously plotted data?} \
      -variable ::$w.discard
  set ::$w.discard 1
  pack $w.dir $w.ta $w.tb $w.cal $w.interval -side top -anchor ne -expand true
  pack $w.live $w.discard -side top -anchor center
#
# Bind the ok button to load the specified file.
#
  $w.bot.apply configure -command "open_archive $w; wm withdraw $w"
}

#-----------------------------------------------------------------------
# This is the callback function attached to the archive_dialog "apply"
# button. It reads the parameters listed in the archive dialog and
# passes them to connect_archive to open a file-input stream.
#
# Input:
#  w      The path of the connect_archive dialog.
#-----------------------------------------------------------------------
proc open_archive {w} {
#
# The end time can be a date and time, empty to make it open ended,
# or the name of the computer that is running gcpControl. In the
# latter case, after reading the archive, monitoring will continue
# with live data from the control program.
#
  set tb [$w.tb.e get]
  set host {}
  set gateway {}
  if {![string match {^[ \t]*$} $tb] && ![is_date_and_time $tb]} {
    set host $tb
    set tb {}
  }
#
# Attempt to load the archive. If this fails, the existing stream
# will be left connected.
#
  if [catch {
    monitor disk [$w.dir.e get] [$w.ta.e get] $tb [set ::$w.live]
#
# The error shouldn't be treated as fatal unless it prevented the
# stream from being opened.
#
  } result] {
    report_error $result
    if {![monitor have_stream]} return
  }
#
# Reselect the current registers.
#
  if {[catch {monitor reconfigure} result]} {
    report_error $result
  }
#
# If a calibration file was specified, attempt to load it.
#
  if [catch {load_cal $::viewer(cal_file)} result] {
    report_error $result
  }
#
# Discard buffered data from the last monitor stream?
#
  if {[set ::$w.discard]} {
    monitor clr_buffer
  }
#
# Reconfigure for the new stream.
#
  monitor reconfigure
#
# Specify the sub-sampling interval.
#
  configure_viewer [$w.interval.e get] $::viewer(bufsize)
#
# If a continuation hostname was given arrange to attempt to connect
# to the control program on that hostname after reading the
# tail of the archive.
#
  if {[string length $host] > 0} {
    monitor eos_callback [list connect_host $host 0 all gateway]
  } else {
      monitor eos_callback {}
  }
}

#-----------------------------------------------------------------------
# Create a dialog for configuring the viewer.
#
# Input:
#  dialog  The path name to give to the dialog.
# Output:
#  return  The toplevel widget of the dialog.
#-----------------------------------------------------------------------
proc create_viewer_dialog {dialog} {
#
# Create the dialog.
#
  create_config_dialog $dialog {Configure Viewer} gcpViewer/conf_gen
#
# Set up the callbacks of the apply and cancel buttons.
#
  $dialog.bot.apply configure -command {set ::viewer_dialog_state apply}
  $dialog.bot.cancel configure -command {set ::viewer_dialog_state cancel}
#
# If the user asks the window manager to kill the window, react as
# though the cancel button had been pressed.
#
  wm protocol $dialog WM_DELETE_WINDOW {set ::viewer_dialog_state cancel}
#
# Get the path of the configuration area.
#
  set w $dialog.top
#
# Create a labeled entry widget for specification of the sampling
# interval.
#
  set f [labeled_entry $w.interval {Sub-sampling Interval} 1 -width 10]
  pack $f -side top -anchor e
#
# Create a labeled entry widget for specification of the buffer size.
#
  set f [labeled_entry $w.bufsize {Buffer size} {5000000} -width 10]
  pack $f -side top -anchor e
#
# Create a checkbutton to turn off the message bell
#
  set f [checkbutton $w.bell -text {Turn off infernal beeping?} -variable ::viewer(belloff) -onvalue true -offvalue false]
  $f select
  pack $f -side top -anchor w
#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $dialog.msg -justify center -fg red -width 200
  return $dialog
}

#-----------------------------------------------------------------------
# Create a labelled entry widget. This consists of a frame $w, which
# contains a label $w.l and an entry widget $w.e.
#
# Input:
#  w        The Tk path name of the frame.
#  label    A label to place to the left of the entry widget.
#  default  The initial value to show in the entry widget.
#  args     Configuration arguments to append to the entry command.
# Output:
#  return   The value of $w.
#-----------------------------------------------------------------------
proc labeled_entry {w label value args} {
  frame $w
  label $w.l -text $label
  eval entry $w.e $args
  if {[string compare $value {}] != 0} {
    $w.e delete 0 end
    $w.e insert end $value
  }
  pack $w.l -side left
  pack $w.e -side left -expand true -fill x
  return $w
}

#-----------------------------------------------------------------------
# Create a ridged frame containing, from left to right, a label $w.l,
# an entry widget $w.e and a button $w.b. Pressing <Return> in the entry
# widget will invoke the button. The caller should bind a command to the
# button and set the size of the entry widget after calling this function.
#
# Input:
#  w        The Tk path name of the frame.
#  label    A label to place to the left of the entry widget.
#  value    The initial value to display in the entry widget.
# Output:
#  return   The value of $w.
#-----------------------------------------------------------------------
proc actuated_entry {w label value} {
  frame $w -relief ridge -bd 2
  label $w.l -text $label
  entry $w.e
  $w.e insert end $value
  button $w.b -text {Apply}
  bind $w.e <Return> "$w.b invoke"
  pack $w.l -side left
  pack $w.e -side left -expand true -fill x
  pack $w.b -side left
  return $w
}

#-----------------------------------------------------------------------
# This procedure is called to set the status of a configuration widget
# based on the validity of its value. It changes the background color of
# the widget to signal its status.
#
# Input:
#  w      The path name of the widget.
#  bad    The new status: 0 - Good, 1 - Bad.
# Output:
#  return The value of $bad.
#-----------------------------------------------------------------------
proc set_widget_status {w bad} {
  if {$bad} {
    $w configure -bg $::error_color
  } else {
    $w configure -bg $::good_color
  }
  return $bad
}

#-----------------------------------------------------------------------
# Save the current viewer configuration as a Tcl script.
#
# Input:
#  file     The name of the new configuration file.
#-----------------------------------------------------------------------
proc save_config {file} {
#
# Open the specified file.
#
  set out [open $file w]
#
# Save the states of global controls.
#
  puts $out "configure_viewer $::viewer(interval) $::viewer(bufsize)"
#
# Write plot configurations.
#
  foreach plot $::viewer(plots) {
    save_plot $plot $out
  }
#
# Write page configurations.
#
  foreach page $::viewer(pages) {
    save_page $page $out
  }
  close $out
}

#-----------------------------------------------------------------------
# Write the configuration of a single plot to a given file channel as
# a TCL script that can later be used to recreate the plot.
#
# Input:
#  plot     The plot widget who's configuration is to be recorded.
#  out      A file channel opened for write.
#-----------------------------------------------------------------------
proc save_plot {plot out} {
#
# Get the configuration array of the page.
#
  upvar #0 $plot p
#
# Write the configuration of the plot and its graphs.
#
  puts $out "add_plot [winfo rootx $plot] [winfo rooty $plot] [list $p(title)] $p(xmin) $p(xmax) $p(marker_size) $p(join) $p(scroll_mode) $p(scroll_margin) [list $p(xregister)] [list $p(xlabel)] {"
  foreach graph $p(graphs) {
    upvar #0 $graph g
      puts $out "  graph $g(ymin) $g(ymax) [list $g(ylabel)] [list $g(yregs)] [list $g(bits)] [list $g(track)] $g(av) $g(axis) $g(apod)"
  }
  puts $out "} $p(type) $p(npt) $p(dx) $p(axis)"
}

#-----------------------------------------------------------------------
# Create a labelled option menu.
#
# The value of the option menu can be changed or read via a local alias
# to the global $w variable that is created by this command.
# The label will be named $w.l and the menubutton $w.opt.
#
# Input:
#  w       The path name to give the frame that surrounds the label
#          and menu button.
#  label   A label to be placed to the left of the option menu button.
#  values  A list of option names. The initial value of the option menu
#          will be the first element of this list.
#  cmd     A command to be executed whenever the selected option menu
#          entry changes. The newly selected value will be inserted as
#          a single list element wherever %v occurs in $cmd. If no
#          command is required, pass {} here.
#  args    Extra configuration arguments for the menu button.
# Output:
#  return  The name of the widget ($w).
#-----------------------------------------------------------------------
proc option_menu {w label values cmd args} {
#
# Encapsulate the label and menubutton within a frame.
#
  frame $w
  label $w.l -text $label
#
# Create an option menubutton and the menu that it will post.
#
  eval {menubutton $w.opt -menu $w.opt.m -indicator on -relief raised -bd 2} $args
  set m [menu $w.opt.m -tearoff no]
#
# Insert the list of options as radio buttons linked to a global variable
# that has the same name as the menubutton widget.
#
  global $w
  foreach value $values {
    $m add radiobutton -label $value -value $value -variable $w
  }
#
# Set the initial value of the menu and install this as the initial
# menubutton label.
#
  set $w [lindex $values 0]
  option_menu_handler {} $w
  pack $w.l $w.opt -side left
#
# Link the command that will both keep the menubutton label up to date
# and evaluate the user's callback, to updates of the radiobutton variable.
#
  trace variable $w w [list option_menu_handler $cmd $w]
  return $w
}

#-----------------------------------------------------------------------
# This is a private callback of the option_menu procedure. Whenever the
# selected option menu value changes, it updates the label of the
# associated option menu button to match and invokes the user's callback.
#
# Input:
#  cmd       The user's callback command.
#  w         The path name of the frame that surrounds the menubutton
#            widget. This is also the name of the global
#            variable that contains the value of the option menu.
#  args      Ignored extra arguments. This allows the procedure to
#            be used as a trace callback.
#-----------------------------------------------------------------------
proc option_menu_handler {cmd w args} {
  upvar #0 $w var
  $w.opt configure -text $var
  if {[string compare $cmd {}] != 0} {
    regsub -all {%v} $cmd [list $var] newcmd
    eval $newcmd
  }
}

#-----------------------------------------------------------------------
# This is a variable-trace callback attached to each plot's input-mode
# variable. Whenever a new input mode is selected, this function
# cancels the previous mode and sets up the new one.
#
# Input:
#  plot    The plot widget to which the mode refers.
#  name    The name of the input variable.
#  ...
#-----------------------------------------------------------------------
proc change_cursor_mode {plot name args} {
  upvar #0 $name input
  upvar #0 $plot p
  if {$p(tag) == 0} {
    return
  }
#
# Set up the new cursor mode.
#
  init_${input}_cursor $plot $plot.main.p $p(tag)
}

#-----------------------------------------------------------------------
# Display a cursor that has a reference position as well as the cursor
# position. Arrange for its reference position to be kept up to date
# with X-axis scrolling and for a given command to be called to report
# the graph and graph coordinates of the reference and cursor
# position when mouse button 1 is pressed.
#
# Input:
#  event   The event to bind to <1>, <2> or <3>.
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  mode    The type of cursor required.
#  cmd     The command to call when the selection has been made.
#  x y     The X-window coordinates of the first location.
#-----------------------------------------------------------------------
proc bind_binary_cursor {event plot pg p_tag mode cmd x y} {
#
# Get the graph and its coordinate that corresponds to
# the given X window coordinate.
#
  if {[catch {
    monitor cursor_to_graph $p_tag 0 [$pg world x $x] [$pg world y $y] \
	g_tag xa ya
  }] } {
    return
  }
#
# Compose a command to be called to redraw the cursor.
#
  set cursor_cmd [concat redraw_binary_cursor $pg $mode $p_tag $g_tag $xa $ya]
#
# Display the cursor.
#
  eval $cursor_cmd
#
# Arrange for the cursor to be redisplayed whenever the plot scrolls.
#
  monitor scroll_callback $p_tag $cursor_cmd
#
# Arrange for the specified reporting command to be invoked with the
# start and end graph coordinates of the cursor.
#
  bind $pg $event [list eval_binary_cursor $cmd $plot $p_tag $g_tag $xa $ya %x %y]
}

#-----------------------------------------------------------------------
# This is a private callback function of bind_binary_cursor, used to
# evaluate the user's command. If no graphs currently exist in the plot,
# this function quietly ignores the call.
#
# Input:
#  cmd      The command to be run. The following arguments will be
#           appended to this command when it is executed.
#              plot pg p_tag g_tag xa ya xb yb
#           where plot is the plot under the cursor and p_tag is its C-level
#           identifier. g_tag is the C-level identifier of the graph
#           that was under the first cursor selection and xa,ya and xb,yb
#           are the two coordinates within this graph of the two selections.
#  plot     The plot under the cursor.
#  pg      The pgplot widget of the plot.
#  p_tag   The C-level identifier of the plot.
#  g_tag   Either 0, or the tag id of the graph that the reported
#          coordinates should represent.
#  xa,ya   The graph coordinates of the first cursor selection, within $pg.
#  x,y     The X-window coordinates of the second cursor selection, within $pg.
#-----------------------------------------------------------------------
proc eval_binary_cursor {cmd plot p_tag g_tag xa ya x y} {
  set pg $plot.main.p
  catch {
    monitor cursor_to_graph $p_tag $g_tag [$pg world x $x] [$pg world y $y] \
	g_tag xb yb
    eval $cmd $plot $pg $p_tag $g_tag $xa $ya $xb $yb
  }
}

#-----------------------------------------------------------------------
# This function is used to display any of the PGPLOT rubber-band cursors
# that require a reference position.
#
# Input:
#  pg       The name of the pgplot widget.
#  mode     The cursor type.
#  p_tag    The id tag of the parent plot.
#  g_tag    The idtag of the graph that is to contain the cursor.
#  x,y      The graph coordinates at which to display the cursor.
#  ...
#-----------------------------------------------------------------------
proc redraw_binary_cursor {pg mode p_tag g_tag x y args} {
  monitor graph_to_cursor $p_tag $g_tag $x $y cx cy
  eval $pg setcursor $mode $cx $cy 4
}

#-----------------------------------------------------------------------
# Bind a command to a given mouse-button, such that when that button
# is pressed over the specified plot, the command will be called with
# the graph and graph coordinates that correspond to the mouse location.
#
# Input:
#  event   The event to bind to <1>, <2> or <3>.
#  plot    The plot to bind the event to.
#  p_tag   The id tag of $plot.
#  g_tag   Either 0, or the tag id of the graph that the reported
#          coordinates should represent.
#  cmd     The command to call when the button is pressed. The following
#          arguments will be appended to the command:
#            plot g_tag x y
#-----------------------------------------------------------------------
proc bind_cursor_btn {event plot p_tag g_tag cmd} {
  bind $plot.main.p $event [list eval_cursor_btn $cmd $plot $p_tag $g_tag %x %y]
}

#-----------------------------------------------------------------------
# This is the private callback function of bind_cursor_btn used to
# respond to a press of a given mouse button event within a plot.
# If the plot contains no graphs, this function quietly ignores the
# button press.
#
# Input:
#  cmd     The user-specified command to be run when the button is pressed,
#          The following arguments will be appended to the command:
#            $plot $p_tag $g_tag $x $y, where x and y are the graph
#          coordinates under the cursor.
#  plot    The plot that the user pressed the button over.
#  pg      The pgplot widget of the plot.
#  p_tag   The C-level identifier of the plot.
#  g_tag   Either 0, or the tag id of the graph that the reported
#          coordinates should represent.
#-----------------------------------------------------------------------
proc eval_cursor_btn {cmd plot p_tag g_tag x y} {
  set pg $plot.main.p
  catch {
    monitor cursor_to_graph $p_tag $g_tag [$pg world x $x] [$pg world y $y] \
	g_tag gx gy
    eval $cmd $plot $pg $p_tag $g_tag $gx $gy
  }
}

#-----------------------------------------------------------------------
# Disable the cursor of a given plot.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_none_cursor {plot pg p_tag} {
  $pg setcursor norm 0 0 4
  bind $pg <1> {}
  bind $pg <2> {}
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {N/A} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Prepare to receive an X-axis zoom range via the cursor of a given plot.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_pkident_cursor {plot pg p_tag} {
  $pg setcursor norm 0 0 4

  global ::npk
    set ::npk {}

  bind $plot <Key-0>  "getNpk %K"
  bind $plot <Key-1>  "getNpk %K"
  bind $plot <Key-2>  "getNpk %K"
  bind $plot <Key-3>  "getNpk %K"
  bind $plot <Key-4>  "getNpk %K"
  bind $plot <Key-5>  "getNpk %K"
  bind $plot <Key-6>  "getNpk %K"
  bind $plot <Key-7>  "getNpk %K"
  bind $plot <Key-8>  "getNpk %K"
  bind $plot <Key-9>  "getNpk %K"
  bind $plot <Return> "stage2_pkident_cursor $plot $pg $p_tag %x %y"

  bind $pg <ButtonPress-1> {}
  bind $pg <ButtonPress-2> {}
  bind $pg <ButtonPress-3> {}

  show_plot_message $plot "With the cursor in the window, type a number, followed by Return"
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {N/A} {N/A} {N/A}
}

proc getNpk {key} {
  global ::npk
  set ::npk [concat $::npk$key]
}

#-----------------------------------------------------------------------
# Prepare to receive an X-axis zoom range via the cursor of a given plot.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc stage2_pkident_cursor {plot pg p_tag x y} {

  if {[catch {
      monitor cursor_to_graph $p_tag 0 [$pg world x $x] [$pg world y $y] \
	  g_tag xa ya
  }] } {
      return
  }

  if {$::npk > 0} {

    $pg setcursor vline 0 0 4
    bind $pg <1> "stage3_pkident_cursor $plot $pg $p_tag %x %y"
    bind $pg <2> "init_pkident_cursor $plot $pg $p_tag"
    bind $pg <3> "cursor_pkident $plot $pg $p_tag $g_tag $x $y $x $y 1"
    monitor scroll_callback $p_tag {}
    describe_plot_btns $plot {Select one edge of the range} {Cancel} {Full Range}
  } else {
    cursor_pkident $plot $pg $p_tag $g_tag $x $y $x $y 0
  }
}

#-----------------------------------------------------------------------
# Having received the first end of a X-axis zoom range, prepare to
# receive the second end.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#  x y     The X window coordinates of the user's first selection.
#-----------------------------------------------------------------------
proc stage3_pkident_cursor {plot pg p_tag x y} {

  if {[catch {
      monitor cursor_to_graph $p_tag 0 [$pg world x $x] [$pg world y $y] \
	  g_tag xa ya
  }] } {
      return
  }
 
  $pg setcursor vline 0 0 4
  bind_binary_cursor <1> $plot $pg $p_tag xrng cursor_pkident $x $y
  bind $pg <2> "init_pkident_cursor $plot $pg $p_tag %x %y"
  bind $pg <3> "cursor_pkident $plot $pg $p_tag $g_tag $x $y $x $y 1"
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Select the other edge} {Cancel} {Full Range}
}

#-----------------------------------------------------------------------
# Respond to the completion of a cursor X-axis zoom selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  xa ya   The graph coordinates of the first cursor selection.
#  xb yb   The graph coordinates of the second cursor selection.
#-----------------------------------------------------------------------
proc cursor_pkident {plot pg p_tag g_tag xa ya xb yb {full 0}} {
  monitor pkident $p_tag $g_tag $::npk $xa $xb $full
  init_pkident_cursor $plot $pg $p_tag
  refresh_plot $plot
}

#-----------------------------------------------------------------------
# Prepare to receive an X-axis zoom range via the cursor of a given plot.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_xzoom_cursor {plot pg p_tag} {
  $pg setcursor vline 0 0 4
  bind $pg <1> "stage2_xzoom_cursor $plot $pg $p_tag %x %y"
  bind $pg <2> "cursor_adopt_xzoom $plot $pg $p_tag"
  bind_cursor_btn <3> $plot $p_tag 0 cursor_xunzoom
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Select one edge of the range} {Adopt} {Unzoom the X-axis}
}

#-----------------------------------------------------------------------
# Having received the first end of a X-axis zoom range, prepare to
# receive the second end.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#  x y     The X window coordinates of the user's first selection.
#-----------------------------------------------------------------------
proc stage2_xzoom_cursor {plot pg p_tag x y} {
  bind_binary_cursor <1> $plot $pg $p_tag xrng cursor_xzoom $x $y
  bind $pg <2> "init_xzoom_cursor $plot $pg $p_tag"
  describe_plot_btns $plot {Select the other edge} {Cancel} {Unzoom the X-axis}
}

#-----------------------------------------------------------------------
# Respond to the completion of a cursor X-axis zoom selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  xa ya   The graph coordinates of the first cursor selection.
#  xb yb   The graph coordinates of the second cursor selection.
#-----------------------------------------------------------------------
proc cursor_xzoom {plot pg p_tag g_tag xa ya xb yb} {
  zoom_xaxis $plot $xa $xb
  init_xzoom_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Respond to a cursor button press to unzoom the associated plot.
#
# Input:
#  plot    The plot that the button was pressed in.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  ...     Ignored arguments.
#-----------------------------------------------------------------------
proc cursor_xunzoom {plot pg p_tag args} {
  unzoom_xaxis $plot
  init_xzoom_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Adopt the current X-axis zoom range as the default X-axis range.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#-----------------------------------------------------------------------
proc cursor_adopt_xzoom {plot pg p_tag} {
  upvar #0 $plot p
#
# Do nothing if the plot isn't currently zoomed.
#
  if {[llength $p(zxmin)] == 0} {
    return
  }
#
# Install the X-axis zoom limits as the new fixed limits.
#
  set p(xmin) [lindex $p(zxmin) end]
  set p(xmax) [lindex $p(zxmax) end]
  set p(zxmin) [list]
  set p(zxmax) [list]
#
# Re-enable scrolling if needed.
#
  if {[string compare $p(scroll_mode) disabled] != 0} {
    configure_plot $plot $p(title) $p(xmin) $p(xmax) $p(marker_size) $p(join) $p(scroll_mode) $p(scroll_margin) $p(xregister) $p(xlabel) $p(type) $p(npt) $p(dx) $p(axis)
    refresh_plot $plot
  }
#
# Prepare the cursor for another zoom.
#
  init_xzoom_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Prepare to receive a Y-axis zoom range via the cursor of a given plot.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_yzoom_cursor {plot pg p_tag} {
  $pg setcursor hline 0 0 4
  bind $pg <1> "stage2_yzoom_cursor $plot $pg $p_tag %x %y"
  bind_cursor_btn <2> $plot $p_tag 0 cursor_adopt_yzoom
  bind_cursor_btn <3> $plot $p_tag 0 cursor_yunzoom
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Select one edge of the range} {Adopt} {Unzoom the nearest Y-axis}
}

#-----------------------------------------------------------------------
# Having received the first end of a Y-axis zoom range, prepare to
# receive the second end.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#  x y     The X window coordinates of the user's first selection.
#-----------------------------------------------------------------------
proc stage2_yzoom_cursor {plot pg p_tag x y} {
  bind_binary_cursor <1> $plot $pg $p_tag yrng cursor_yzoom $x $y
  bind $pg <2> "init_yzoom_cursor $plot $pg $p_tag"
  describe_plot_btns $plot {Select the other edge} {Cancel} {Unzoom the nearest Y-axis}
}

#-----------------------------------------------------------------------
# Respond to the completion of a cursor Y-axis zoom selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  xa ya   The graph coordinates of the first cursor selection.
#  xb yb   The graph coordinates of the second cursor selection.
#-----------------------------------------------------------------------
proc cursor_yzoom {plot pg p_tag g_tag xa ya xb yb} {
  set graph [tag_to_graph $p_tag $g_tag]
  zoom_yaxis $graph $ya $yb
  init_yzoom_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Respond to a cursor button press to unzoom the nearest graph.
#
# Input:
#  plot    The plot that the button was pressed in.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  ...     Ignored arguments.
#-----------------------------------------------------------------------
proc cursor_yunzoom {plot pg p_tag g_tag args} {
  set graph [tag_to_graph $p_tag $g_tag]
  unzoom_yaxis $graph
  init_yzoom_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Adopt the current Y-axis zoom range of a graph as its default
# Y-axis range.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  ...     Ignored arguments.
#-----------------------------------------------------------------------
proc cursor_adopt_yzoom {plot pg p_tag g_tag args} {
#
# Get the selected graph.
#
  set graph [tag_to_graph $p_tag $g_tag]
  upvar #0 $graph g
#
# Do nothing if the graph isn't currently zoomed.
#
  if {[llength $g(zymin)] == 0} {
    return
  }
#
# Install the Y-axis zoom limits as the new fixed limits.
#
  set g(ymin) [lindex $g(zymin) end]
  set g(ymax) [lindex $g(zymax) end]
  set g(zymin) [list]
  set g(zymax) [list]
#
# Prepare the cursor for another zoom.
#
  init_yzoom_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Initialize data-point identification cursor input mode.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_ident_cursor {plot pg p_tag} {
  $pg setcursor cross 0 0 4
  bind_cursor_btn <1> $plot $p_tag 0 cursor_ident
  bind $pg <2> "init_ident_cursor $plot $pg $p_tag"
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Click near the point that you want to identify} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Respond to the completion of a data-point cursor selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  x y     The graph coordinates of the cursor selection.
#-----------------------------------------------------------------------
proc cursor_ident {plot pg p_tag g_tag x y} {
  upvar #0 $plot p

  if [catch {
    monitor find_point $p_tag $g_tag $x $y reg altreg x y
#
# If the x-axis register is a date and time, convert its value from
# MJD days to a calendar date.
#
    if {![string match *.date $p(xregister)] || [catch {
      set xval [monitor mjd_to_date $x]}]
    } {
      set xval [format %.15g $x]
    }
    show_plot_message $plot "The cursor was nearest to  $p(xregister)=$xval  $reg=[format %.15g $y]"
  } result] {
    show_plot_message $plot $result
  }
  init_ident_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Initialize trace-statistics cursor input mode.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_stats_cursor {plot pg p_tag} {
  $pg setcursor cross 0 0 4
  bind_cursor_btn <1> $plot $p_tag 0 cursor_stats
  bind $pg <2> "init_stats_cursor $plot $pg $p_tag"
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Click near the trace whose statistics you want} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Respond to the completion of a trace-statistics cursor selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  x y     The graph coordinates of the cursor selection.
#-----------------------------------------------------------------------
proc cursor_stats {plot pg p_tag g_tag x y} {
  upvar #0 $plot p

  if [catch {
    monitor find_point $p_tag $g_tag $x $y yreg statreg gx gy
    monitor get_x_axis_limits $p(tag) xmin xmax
    monitor subset_stats $p_tag $statreg $xmin $xmax min max mean rms npoint nsig
    show_plot_message $plot "$npoint samples of $yreg have  min=[format %.3g $min]  max=[format %.3g $max]  mean=[format %.3g $mean]  rms=[format %.3g $rms] nsig=[format %.3g $nsig]"
  } result] {
    show_plot_message $plot $result
  };
  init_stats_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Initialize select-a-graph-to-modify input mode.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_modwin_cursor {plot pg p_tag} {
  $pg setcursor cross 0 0 4
  bind_cursor_btn <1> $plot $p_tag 0 cursor_modwin
  bind $pg <2> "init_modwin_cursor $plot $pg $p_tag"
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Click in the graph that you want to reconfigure} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Respond to the completion of a modify-graph cursor selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  x y     The graph coordinates of the cursor selection.
#-----------------------------------------------------------------------
proc cursor_modwin {plot pg p_tag g_tag x y} {
  upvar #0 $plot p

  if [catch {
      if {[string compare $p(type) powSpec] == 0} {
        show_powSpecGraph_dialog $plot [tag_to_graph $p_tag $g_tag]
      } else {
        show_graph_dialog $plot [tag_to_graph $p_tag $g_tag]
      }
  } result] {
    show_plot_message $plot $result
  }
  init_modwin_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Initialize select-a-graph-to-delete input mode.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_delwin_cursor {plot pg p_tag} {
  $pg setcursor cross 0 0 4
  bind_cursor_btn <1> $plot $p_tag 0 cursor_delwin
  bind $pg <2> "init_delwin_cursor $plot $pg $p_tag"
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Click in the graph that you want to delete} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Respond to the completion of a delete-graph cursor selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  x y     The graph coordinates of the cursor selection.
#-----------------------------------------------------------------------
proc cursor_delwin {plot pg p_tag g_tag x y} {
  if [catch {
    delete_graph [tag_to_graph $p_tag $g_tag]
    monitor reconfigure
  } result] {
    show_plot_message $plot $result
  }
  init_delwin_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Initialize select-a-graph-to-integerate input mode.
#
# Input:
#  plot    The plot to be integrated
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_startintwin_cursor {plot pg p_tag} {
  $pg setcursor cross 0 0 4
  bind_cursor_btn <1> $plot $p_tag 0 cursor_startintwin
  bind $pg <2> "init_startintwin_cursor $plot $pg $p_tag"
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Click in the graph that you want to integrate} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Respond to the completion of an integrate-graph cursor selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  x y     The graph coordinates of the cursor selection.
#-----------------------------------------------------------------------
proc cursor_startintwin {plot pg p_tag g_tag x y} {
  if [catch {
    integrate_graph [tag_to_graph $p_tag $g_tag] 1
    monitor reconfigure
  } result] {
    show_plot_message $plot $result
  }
  init_startintwin_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Initialize select-a-graph-to-integerate input mode.
#
# Input:
#  plot    The plot to be integrated
#  pg      The pgplot widget of the specified plot.
#  p_tag   The tag of the specified plot.
#-----------------------------------------------------------------------
proc init_stopintwin_cursor {plot pg p_tag} {
  $pg setcursor cross 0 0 4
  bind_cursor_btn <1> $plot $p_tag 0 cursor_stopintwin
  bind $pg <2> "init_stopintwin_cursor $plot $pg $p_tag"
  bind $pg <3> {}
  monitor scroll_callback $p_tag {}
  describe_plot_btns $plot {Click in the graph that you want to stop integrating} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Respond to the completion of an integrate-graph cursor selection.
#
# Input:
#  plot    The plot widget to prepare.
#  pg      The pgplot widget of $plot.
#  p_tag   The id tag of $plot.
#  g_tag   The graph of the selection.
#  x y     The graph coordinates of the cursor selection.
#-----------------------------------------------------------------------
proc cursor_stopintwin {plot pg p_tag g_tag x y} {
  if [catch {
    integrate_graph [tag_to_graph $p_tag $g_tag] 0
    monitor reconfigure
  } result] {
    show_plot_message $plot $result
  }
  init_stopintwin_cursor $plot $pg $p_tag
}

#-----------------------------------------------------------------------
# Display descriptions of the actions of each mouse button wrt the
# pgplot window.
#
# Input:
#  plot      The plot to document.
#  b1        The label for button 1.
#  b2        The label for button 2.
#  b3        The label for button 3.
#-----------------------------------------------------------------------
proc describe_plot_btns {plot b1 b2 b3} {
  set b $plot.main.b
  $b.b1 configure -text $b1
  $b.b2 configure -text $b2
  $b.b3 configure -text $b3
}

#-----------------------------------------------------------------------
# Create a dialog for saving plots to hardcopy devices.
#
# Input:
#  plot   The Tk plot widget to associate the dialog with.
# Output:
#  return The value of $w.
#-----------------------------------------------------------------------
proc create_hardcopy_dialog {plot} {
#
# Extract the plot number suffix.
#
  regsub {^\.plot} $plot {} suffix
  set w [create_config_dialog .hard$suffix {Hardcopy} gcpViewer/plot/hard]
#
# Create a labeled entry widget $w.f.e.
#
  set f [labeled_entry $w.top.f {Device name:} {snap.ps/cps} -width 20]
  bind $f.e <Return> "$w.bot.apply invoke"
  pack $f
#
# Bind the ok button to load the specified file.
#
  $w.bot.apply configure -command "plot_hardcopy $plot \[$f.e get\]; wm withdraw $w"
  return $w
}

#-----------------------------------------------------------------------
# Make a hardcopy version of a given plot.
#
# Input:
#  plot     The plot to be rendered.
#  dev      The name of the PGPLOT device to draw to.
#-----------------------------------------------------------------------
proc plot_hardcopy {plot dev} {
  upvar #0 $plot p
  if {$p(tag) == 0} {
    show_plot_message $plot {Plot not configured - press the UPDATE button}
  } else {
    monitor plot_hardcopy $p(tag) $dev
  }
}

#-----------------------------------------------------------------------
# Reconfigure the viewer.
#
# Input:
#  interval    The register sub-sampling interval. A value of 1 means
#              that the control program will attempt to send all of the
#              frames that it receives from the control system. In reality
#              depending on the speed of the network etc, some frames
#              may be dropped.
#  bufsize     The size of the monitoring buffer.
#-----------------------------------------------------------------------
proc configure_viewer {interval bufsize} {
#
# Establish the new monitor interval.
#
  monitor set_interval $interval
#
# Reconfigure the monitor buffer size.
#
  monitor resize_buffer $bufsize
#
# Record the new values.
#
  set ::viewer(interval) $interval
  set ::viewer(bufsize) $bufsize
}

#-----------------------------------------------------------------------
# Invoke the dialog that allows the user to modify general characteristics
# of the viewer.
#-----------------------------------------------------------------------
proc show_viewer_dialog {} {
#
# In case the user presses the cancel button, construct a command to
# use to restore the current configuration of the viewer.
#
  set cancel_command [list configure_viewer $::viewer(interval) $::viewer(bufsize)]
#
# The following variable is used by callbacks to signal when the apply
# or cancel buttons are pressed. Its value is either "apply" or "cancel".
#
  set ::viewer_dialog_state apply
#
# Get the path name of the viewer-configuration dialog and its configuration
# area.
#
  set dialog .viewer_dialog
  set w $dialog.top
#
# Copy the current viewer configuration into the dialog.
#
  replace_entry_contents $w.interval.e $::viewer(interval)
  replace_entry_contents $w.bufsize.e $::viewer(bufsize)
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
  tkwait variable viewer_dialog_state   ;# Wait for the apply or cancel button
#
# Unless the user presses the cancel button don't withdraw the dialog until
# an error free configuration has been specified.
#
  while {[string compare $::viewer_dialog_state apply] == 0} {
#
# Attempt to configure the widget with the parameters that the user
# entered.
#
    if {[catch {
      configure_viewer [$w.interval.e get] [$w.bufsize.e get]
      set ::viewer_dialog_state "done"
    } result]} {
      dialog_error $dialog $result        ;# Exhibit the error message.
      tkwait variable viewer_dialog_state   ;# Wait for the user again.
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
# If the user hit the cancel button, restore the original configuration
# of the viewer.
#
  if {[string compare $::viewer_dialog_state cancel] == 0} {
    eval $cancel_command
  } else {
#
# Tell monitor_viewer to change the sampling interval.
#
    monitor reconfigure
  }
}

#-----------------------------------------------------------------------
# Delete all pages.
#-----------------------------------------------------------------------
proc delete_pages {} {
#
# Delete pages that are attached to widgets.
#
  foreach page $::viewer(pages) {
    delete_page $page
  }
  return
}

#-----------------------------------------------------------------------
# Display a dialog for users to use to interactively add incremental
# offsets in azimuth and elevation. This acts as a front end to the
# control language "offset" command.
#
# Input:
#  w         The tk pathname to give the dialog. This must be a name
#            that is valid for a toplevel window.
# Output:
#  return    The same as $w.
#-----------------------------------------------------------------------
proc create_offset_dialog {w} {
#
# Create a top-level widget for the dialog.
#
  toplevel $w
  wm withdraw $w
  wm title $w {Add offsets}
  wm iconname $w Dialog
  wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"
  bind $w <Meta-q> "wm withdraw $w"
#
# Create a menubar with a file menu that contains a quit button.
#
  set mbar [frame $w.menu -relief raised -bd 2]
  menubutton $mbar.file -text File -menu $mbar.file.menu
  set m [menu $mbar.file.menu -tearoff 0]
  $m add command -label {Quit   [Meta+q]} -command "wm withdraw $w"
  pack $mbar.file -side left
#
# Create the help menu.
#
  menubutton $mbar.help -text Help -menu $mbar.help.menu
  set m [menu $mbar.help.menu -tearoff 0]
  $m add command -label {The offset dialog} -command "show_help gcpViewer/offset"
  pack $mbar.help -side right
  pack $mbar -side top -fill x
#
# Create an area in which to display offsets under the cursor.
#
  set cursor [frame $w.cursor -relief ridge]
  label $cursor.xtitle -text {X: } -fg TextLabelColor
  label $cursor.x -text 0.0 -width 10
  label $cursor.ytitle -text {  Y: } -fg TextLabelColor
  label $cursor.y -text 0.0 -width 10
  label $cursor.units -text "arcsec"
  pack $cursor.xtitle $cursor.x $cursor.ytitle $cursor.y $cursor.units -side left
  pack $cursor -side top -fill x
#
# Create a canvas for displaying a target area.
#
  set canvas [canvas $w.canvas -width 400 -height 300 -bg skyblue -relief groove -bd 2]
  pack $canvas -expand true -fill both
#
# Create a global configuration array for recording the configuration of
# the target area of the canvas.
#
  upvar #0 $canvas c
  set c(page) 0        ;# The id of the monitor page used to query registers.
  set c(rings) 5       ;# The number of rings to draw.
  set c(xc) 50         ;# The X coordinate of the center of the rings.
  set c(yc) 50         ;# The Y coordinate of the center of the rings.
  set c(dx) 5          ;# The increment in the X-axis crossing point per ring.
  set c(dy) 5          ;# The increment in the Y-axis crossing point per ring.
  set c(step) 1.0      ;# The current grid interval.
  set c(update_step) 1 ;# True if c(step) is out of date.
  set c(snap) 0        ;# True to only allow moves that lie on the grid lines.
  set c(dir) 1         ;# The sign of the correction (-1 or 1).
  set c(tv_angle) 0.0  ;# The deck angle at which the TV picture is upright.
  set c(dk) 0.0        ;# The current deck angle.
  set c(compass) 0     ;# Draw the compass lines if true.
  set c(drawn) 0       ;# True if the compass line isn't currently drawn.
  set c(angle) 0.0     ;# The clockwise rotation angle of the sky on the tv,
                       ;#  computed by the last call to redraw_offset_compass.
#
# Arrange for the rings to be redrawn whenever the window is resized
# and when it is first displayed.
#
  bind $canvas <Configure> "redraw_offset_target $w"
  bind $canvas <Map> "redraw_offset_target $w"
#
# Arrange for an arrow to be drawn from the center to the cursor.
#
  bind $canvas <Motion> "redraw_offset_arrow $w %x %y"
  bind $canvas <1> "send_offset $w %x %y"
#
# Create a button for the user to press when the star is centered in the
# cross hairs.
#
  button $w.mark -bg [$w cget -bg] -text "Press here when the star\nis centered on the TV" -command "send_offset_mark $w"
  pack $w.mark -fill both -expand true -side top
#
# Create an entry area for specifying the grid interval in arcseconds.
#
  set step [frame $w.step]
  label $step.title -anchor w -text {Grid interval: }
  entry $step.sec -width 4 -textvariable ::$step.sec
  $step.sec insert end 10
  label $step.unit -anchor w -text arcsec
  button $step.skip -bg [$w cget -bg] -text "Skip this star" -command skip_offset_star
  pack $step.title $step.sec $step.unit -side left
  pack $step.skip -side right
  pack $step -side top -anchor w -fill x
#
# Whenever the user changes the value in the above entry widget,
# recompute the grid interval.
#
  trace variable ::$step.sec w "compute_offset_step $w"
#
# Get the initial grid interval.
#
  compute_offset_step $w
#
# Create a row of check buttons.
#
  set option [frame $w.option -relief ridge -bd 1]
  checkbutton $option.snap -text {Snap to grid   } -variable ::$canvas\(snap\) -anchor c
  checkbutton $option.reverse -text {Reverse arrow} -offvalue 1 -onvalue -1 -variable ::$canvas\(dir\) -anchor c
  pack $option.snap $option.reverse -side left -expand true -fill x
  pack $option -side top -fill x
#
# Create a register page. When the offset dialog is displayed this will
# be used to request updates of the tv angle and deck angle from the
# monitor stream.
#
  set c(page) [monitor add_page]
#
# Arrange to request monitor data of pertinent registers when the window
# is mapped, and to unrequest them when the window is not mapped.
#
  bind $canvas <Map> "update_offset_dialog_regs $w; monitor reconfigure"
  bind $canvas <Unmap> "update_offset_dialog_regs $w; monitor reconfigure"
#
# Whenever the orientation of the image wrt the sky changes, redraw the
# compass lines.
#
  trace variable ::$canvas\(dk\) w "redraw_offset_compass $w"
#
# Return the pathname of the dialog.
#
  return $w
}

#-----------------------------------------------------------------------
# Erase and redraw the offset target to account for possible changes in
# the size of the canvas or changes in the requested number of divisions.
#
# Input:
#  offset        The Tk pathname of the offset widget.
#-----------------------------------------------------------------------
proc redraw_offset_target {offset} {
  set canvas $offset.canvas  ;# The canvas widget of the dialog.
  upvar #0 $canvas c         ;# The configuration array of the target area.
#
# Delete any currently drawn grid and arrow.
#
  $canvas delete rings
  $canvas delete arrow
#
# Determine the current dimensions of the canvas.
#
  if {[winfo ismapped $canvas]} {
    set w [winfo width $canvas]
    set h [winfo height $canvas]
  } else {
    set w [$canvas cget -width]
    set h [$canvas cget -height]
  }
#
# Get the smallest dimension of the canvas area.
#
  set s [expr {$w < $h ? $w : $h}]
#
# Get the maximum radius that can be accomodated by this size.
#
  set r [expr {int($s / 2.0)}]
#
# Work out a new center for the target area, based on the size of
# the canvas.
#
  set c(xc) [expr {round($r + ($w - $s)/2.0)}]
  set c(yc) [expr {round($r + ($h - $s)/2.0)}]
#
# Work out the distance between rings along the x and y axes.
#
  set c(dx) [expr {floor($r / $c(rings))}]
  set c(dy) [expr {floor($r / $c(rings))}]
#
# Draw the vertical grid lines.
#
  set nhalf [expr {int($w / $c(dx) / 2.0)}]
  for {set i -$nhalf} {$i <= $nhalf} {incr i} {
    set x [expr {$c(xc) + $i * $c(dx)}]
    $canvas create line $x 0 $x $h -tags rings -width 1 -fill green
  }
#
# Draw the horizontal grid lines.
#
  set nhalf [expr {int($h / $c(dy) / 2.0)}]
  for {set i -$nhalf} {$i <= $nhalf} {incr i} {
    set y [expr {$c(yc) + $i * $c(dy)}]
    $canvas create line 0 $y $w $y -tags rings -width 1 -fill green
  }
#
# Draw the rings.
#
  for {set i 0} {$i <= $c(rings)} {incr i} {
    set xmin [expr {$c(xc) - $i * $c(dx)}]
    set xmax [expr {$c(xc) + $i * $c(dx)}]
    set ymin [expr {$c(yc) - $i * $c(dy)}]
    set ymax [expr {$c(yc) + $i * $c(dy)}]
    $canvas create oval $xmin $ymin $xmax $ymax -tags rings -width 1 -outline blue
  }
#
# Redraw the elevation and azimuth lines.
#
  redraw_offset_compass $offset
}

#-----------------------------------------------------------------------
# Draw an arrow head from the center of the rings to the cursor.
#
# Input:
#  canvas      The Tk pathname of the offset dialog.
#-----------------------------------------------------------------------
proc redraw_offset_arrow {w x y} {
  set canvas $w.canvas  ;# The display canvas of the target
  upvar #0 $canvas c    ;# The configuration array of the target area.
#
# Delete the current arrow.
#
  $canvas delete arrow
#
# If the user only wants to move by integral numbers of arcseconds in each
# direction, modify x and y to lie on the nearest grid point.
#
  if {$c(snap)} {
    set x [expr {$c(xc) + round(($x-$c(xc))/$c(dx)) * $c(dx)}]
    set y [expr {$c(yc) + round(($y-$c(yc))/$c(dy)) * $c(dy)}]
  }
#
# Compute the offset that corresponds to the cursor position(arcseconds).
#
  set dh [expr {($c(step) * ($x - $c(xc)) / $c(dx)) * 3600}]
  set dv [expr {($c(step) * ($c(yc) - $y) / $c(dy)) * 3600}]
#
# Display the new values.
#
  $w.cursor.x configure -text [format {%.2f} $dh]
  $w.cursor.y configure -text [format {%.2f} $dv]
#
# Draw the pointer with the arrow-head at the selected end of the line.
#
  if {$c(xoffdir) < 0} {
    set where last
  } else {
    set where first
  }
  $canvas create line $c(xc) $c(yc) $x $y -arrow $where -tags arrow
}

#-----------------------------------------------------------------------
# This is a private callback used by the offset dialog. Given a direction
# it reads the step size and camera angle to determine by how much and
# in what direction to offset the telescope.
#
# Input:
#  w       The tk pathname of the dialog.
#  x y     The position on the canvas at which the user clicked.
#-----------------------------------------------------------------------
proc send_offset {w x y} {
#
# Get the configuration array of the canvas.
#
  upvar #0 $w.canvas c

#
# Get the horizontal and vertical increments.
#
  set dh [expr {$c(xoffdir) * $c(step) * ($c(xc) - $x) / $c(dx)}]
  set dv [expr {$c(yoffdir) * $c(step) * ($y - $c(yc)) / $c(dy)}]
#
# Send offset commands for the az and el axes.
#
  if [catch {control send "tv_offset $dh, $dv"} result] {
    report_error "$result"
    bell
    return
  }
}

#-----------------------------------------------------------------------
# Change the color of the mark-offset-frame button, tell the control
# system to mark the next archive frame as a pointing reference frame,
# then wait for a couple of seconds before turning the button color
# back to normal. This gives a visible indication that one must wait
# for the marked archive frame integration to complete before entering
# new offsets.
#
# Input:
#  w       The tk pathname of the frame that contains the $w.mark
#          button that invoked this callback.
#-----------------------------------------------------------------------
proc send_offset_mark {w} {
#
# Tell the control system to mark the next frame as a pointing reference
# frame.
#
  if {[catch {control send "mark one, f0"} result]} {
    report_error $result
    return
  }
#
# Determine the normal color of the button, and the color to make it
# to give feedback.
#
  set normal [$w cget -bg]
  set feedback red
#
# Change the button color to red to indicate that the command is in
# progress.
#
  $w.mark configure -bg $feedback -activebackground $feedback
#
# Arrange for the button color to be reverted to normal in a few seconds.
#
  after 3500 [list end_offset_marker_delay $w $normal]
}

#-----------------------------------------------------------------------
# This procedure is called as an "after" callback at the end of the delay
# that allows an offset marker to propagate to the archiver in the control
# program.
#
# Input:
#  w      The Tk pathname of the offset dialog.
#  color  The normal background color of the mark-offset button.
#-----------------------------------------------------------------------
proc end_offset_marker_delay {w color} {
#
# Reset the color of the mark-offset button to normal.
#
  $w.mark configure -bg $color -activebackground $color
#
# Provide a hook whereby scheduling scripts can see when it is safe
# to go to the next star (by using the signaled() function).
#
  if {[catch {control send "signal done"} result]} {
    report_error $result
    return
  }
}

#-----------------------------------------------------------------------
# Send a "done" signal to the control program to have it skip to the
# next star.
#-----------------------------------------------------------------------
proc skip_offset_star {} {
  if {[catch {control send "signal done"} result]} {
    report_error $result
    return
  }
}

#-----------------------------------------------------------------------
# Recompute the offset step size from the entry widgets in the offset
# dialog.
#
# Input:
#  w      The Tk pathname of the dialog.
#  args   Unused arguments. The inclusion of this parameter allows this
#         procedure to be used as a trace callback.
#-----------------------------------------------------------------------
proc compute_offset_step {w args} {
  upvar #0 $w.canvas c    ;#  The canvas configuration array.
#
# Get the interval.
#
  set sec [$w.step.sec get]
#
# The value must be a valid floating point number greater than zero.
# Note that this has to be tested in two steps because the Tcl &&
# operator doesn't support short-circuiting.
#
  set bad 1
  if {[is_ufloat $sec]} {
    if {$sec > 0.0} {
      set bad 0
    }
  }
#
# Set the color of the entry widget to indicate the status of its
# contents, then return if the number is invalid.
#
  if {[set_widget_status $w.step.sec $bad]} {
    return
  }
#
# Record the step size in decimal degrees.
#
  set c(step) [expr {$sec / 3600.0}]
}


#-----------------------------------------------------------------------
# If the offset dialog is mapped and the viewer has a source of monitor
# data, ask to be sent updates of pertinent registers. Otherwise
# tell the viewer that we are no longer interested in these registers.
# This removes the overhead of monitoring these registers when we aren't
# doing offset pointing.
#
# Input:
#  w       The Tk pathname of the offset dialog.
#-----------------------------------------------------------------------
proc update_offset_dialog_regs {w} {
  upvar #0 $w.canvas c    ;#  The canvas configuration array.
#
# Discard any existing register fields.
#
  monitor delete_fields $c(page)
  set c(compass) 0
#
# Is the offset dialog mapped?
#
  if {[winfo ismapped $w]} {
    if {[monitor have_stream]} {
      if {[catch {
#
# Arrange to have updates of the camera angle reported in
# the variable ::.offset.canvas(tv_angle).
#
	set tag [monitor add_field $c(page)]
#	monitor configure_field $c(page) $tag camera.angle floating {} \
#	    0 15 0 {} 0 0 0 0 0
	monitor field_variables $c(page) $tag ::$w.canvas\(tv_angle\) ::unused ::unused
#
# Arrange to have updates of the deck angle reported in the
# variable ::.offset.canvas\(dk\).
#
	set tag [monitor add_field $c(page)]
#	monitor configure_field $c(page) $tag {tracker.actual[2]} \
#	    floating {} 0 15 0 {} 0 0 0 0 0
#	puts "About to configure antenna0.tracker.actual[2] (1)"
#	monitor configure_field $c(page) $tag {antenna0.tracker.actual[2]} \
#	    floating {} 0 15 0 {} 0 0 0 0 0
#	puts "Done to configure antenna0.tracker.actual[2] (1)"

	monitor field_variables $c(page) $tag ::$w.canvas\(dk\) ::unused ::unused
	set c(compass) 1
      } result]} {
	report_error "offset_dialog: $result"
      }
    }
  }
}

#-----------------------------------------------------------------------
# Redraw the elevation and azimuth pointers to reflect a new tv
# orientation.
#
# Input:
#  w       The Tk pathname of the offset dialog.
#  args    Consume extra arguments sent when this function is used as
#          as trace callback.
#-----------------------------------------------------------------------
proc redraw_offset_compass {w args} {
  upvar #0 $w.canvas c    ;#  The canvas configuration array.
#
# Do we have sufficient information to draw the compass lines?
#
  if {!$c(compass)} {
    $w.canvas delete az_el ;# Delete the compass lines.
    set c(drawn) 0
    return
  }
#
# Get the current angle of elevation clockwise of the upwards direction
# of the TV.
#
  set angle [expr {$c(tv_angle) - $c(dk)}]
#
# Don't redraw the compass redundantly.
#
  if {$c(drawn) && $angle == $c(angle)} {
    return
  }
#
# Delete the current az and el pointers.
#
  $w.canvas delete az_el
  set c(drawn) 0
#
# Keep a record of the new sky angle and precompute its trig terms.
#
  set $c(angle) $angle
  set sin_angle [expr {sin($angle * $::PI/180.0)}]
  set cos_angle [expr {cos($angle * $::PI/180.0)}]
#
# Choose the ring on which the pointers will end, such that the
# end of the pointers will always be visible.
#
  set rings [expr $c(rings) - 1]
#
# Draw a pointer along the direction of increasing elevation to the edge
# of the second to last ring.
#
  set dx [expr {$rings * $sin_angle * $c(dx)}]
  set dy [expr {$rings * $cos_angle * $c(dy)}]
  $w.canvas create line $c(xc) $c(yc) [expr {$c(xc)+$dx}] [expr {$c(yc)-$dy}] \
      -arrow last -fill white -tags az_el
  $w.canvas create text [expr {$c(xc)+1.05*$dx}] [expr {$c(yc)-1.05*$dy}] \
      -text El -tags az_el
#
# Draw a pointer along the direction of increasing azimuth to the edge
# of the last ring.
#
  set dx [expr {$rings * $cos_angle * $c(dx)}]
  set dy [expr {-$rings * $sin_angle * $c(dy)}]
  $w.canvas create line $c(xc) $c(yc) [expr {$c(xc)+$dx}] [expr {$c(yc)-$dy}] \
      -arrow last -fill yellow -tags az_el
  $w.canvas create text [expr {$c(xc)+1.05*$dx}] [expr {$c(yc)-1.05*$dy}] \
      -text Az -tags az_el
  set c(drawn) 1
}

#-----------------------------------------------------------------------
# Deiconify and raise an unmapped dialog, such that the dialog window
# appears under the cursor.
#
# Input:
#  dialog   The dialog to be mapped.
#  modal    If the caller is going to perform a grab on the dialog, then
#           it should set this to non-zero to tell map_dialog to keep
#           the dialog visible.
#-----------------------------------------------------------------------
proc map_dialog {dialog modal} {
#
# Treat the dialog as modal?
#
  if {$modal} {
    wm transient $dialog .
    bind $dialog <Visibility> [list keep_dialog_visible $dialog %W %s]
  }
#
# Find out where the cursor is currently located.
#
  set xy [winfo pointerxy $dialog]
  set x [lindex $xy 0]
  set y [lindex $xy 1]
#
# Find out the dimensions of the dialog.
# If the dialog hasn't been mapped before, the width and height will both
# be reported as 1, so in this case substitute minimal guesses.
#
  set w [winfo width $dialog]
  set h [winfo height $dialog]
  if {$w == 1 && $h == 1} {
    set w 150
    set h 80
  }
#
# Compute the x and y positions at which to map the dialog.
#
  set x [expr {$x - $w / 3}]
  set y [expr {$y - $h / 3}]
#
# Make sure that the dialog won't appear off the edge of the display.
#
  if {$x < 0} {set x 0}
  if {$y < 0} {set y 0}
#
# Request the computed position.
#
  wm geometry $dialog +${x}+${y}
#
# Display the dialog.
#
  wm deiconify $dialog
  raise $dialog
}

#-----------------------------------------------------------------------
# This is the Visibility-event callback procedure used by the map_dialog
# function to keep a modal dialog visible above all other windows (except
# for sub-menus). To prevent the dialog from being raised above a
# sub-menu the function won't raise the dialog unless it currently has
# the keyboard grabbed.
#
# Input:
#  dialog     The pathname of the dialog.
#  target     The target window of the event given by the %W attribute of
#             the Visibility event.
#  state      The %s state attribute of the Visibility event.
#-----------------------------------------------------------------------
proc keep_dialog_visible {dialog target state} {
  if {[string compare $target $dialog]==0 && \
      [string compare [grab status $dialog] none]!=0} {
    if {[string compare $state VisibilityUnobscured] != 0} {
      raise $dialog
      update idletasks
    }
  }
}

#-----------------------------------------------------------------------
# Display an error message the error message area of a given dialog.
#
# Input:
#  message       The error message to be displayed, or {} to clear and
#                remove the error-message area.
#-----------------------------------------------------------------------
proc dialog_error {dialog message} {
  if { [string length $message] > 0 } {
    $dialog.msg configure -text $message
    pack $dialog.msg -after $dialog.bot -expand true -fill x
  } else {
    pack forget $dialog.msg
    $dialog.msg configure -text {}
  }
}

#-----------------------------------------------------------------------
# Populate the given menu with the names of windows that are currently
# associated with gcpViewer, and arrange for selecting one of these
# entries to make corresponding visible.
#
# Input:
#  m       The menu to populate.
#-----------------------------------------------------------------------
proc populate_window_menu {m} {
#
# First clear the menu.
#
  $m delete 0 end

#
# Display the list of fixed windows
#
  $m add command -label {Utilities windows} -state disabled
  $m add command -label "Frame Grabber" -command "reveal .im; reveal_controls .im"
  $m add command -label "Pager Window"  -command "reveal .pagerWindow"
  $m add command -label "Star Plot"     -command "reveal .starplot"

  $m add separator

#
# Display the list of plot windows.
#
  $m add command -label {Plot windows} -state disabled
  $m add separator
  foreach plot $::viewer(plots) {
    upvar #0 $plot p
    $m add command -label $p(title) -command "reveal $plot"
  }
#
# Display the list of page windows.
#
  $m add command -label {Page windows} -state disabled
  $m add separator
  foreach page $::viewer(pages) {
    upvar #0 $page p
    $m add command -label [wm title $page] -command "reveal $page"
  }
}

#-----------------------------------------------------------------------
# Reveal a given window by deiconizing it and raising it in the center
# of the screen.
#
# Input:
#  w       The window to be revealed.
#-----------------------------------------------------------------------
proc reveal {w} {
  wm deiconify $w
  raise $w
#
# Get the dimensions of the screen.
#
  set sh [winfo screenheight .]
  set sw [winfo screenwidth .]
#
# Get the dimensions of the window.
#
  set ww [winfo width $w]
  set wh [winfo height $w]
#
# Ask the window manager to place the window in the middle of the screen.
# The addition of the small random number is to work around a bug in
# tk, where if the geometry doesn't change, Tk hangs for
# a couple of seconds, while waiting for a ConfigureNotify event that
# never arrives. Without the random number, this hang will occur if
# one uses this function twice on the same window, without moving
# it in the mean time.
#
  set x [expr {int(($sw - $ww)/2.0 + rand()*10)}]
  set y [expr {int(($sh - $wh)/2.0 + rand()*10)}]
  wm geometry $w +${x}+${y}
}

proc convertRegString {msg} {
    upvar 1 $msg message

    # If the pager string begins with 'delta', replace it with the
    # unicode equivalent of the Greek Delta symbol

    set start [string first "delta " $message]
    set end [expr {$start + 5}]

    if { $start != -1} {
      set message [string replace $message $start $end "    \u0394"]
    }
}

#-----------------------------------------------------------------------
# Insert a pager condition into the pager window
#-----------------------------------------------------------------------
proc insertPagerMessage {message} {

    # If the pager string begins with 'delta', replace it with the
    # unicode equivalent of the Greek Delta symbol

    set start [string first "delta " $message]
    set end [expr {$start + 5}]

    if { $start != -1} {
      set ss [string replace $message $start $end "    \u0394"]
    } else {
      set ss $message
    }

  .pagerWindow.main.text.text insert end $ss
}

#-----------------------------------------------------------------------
# Clear the pager ager window
#-----------------------------------------------------------------------
proc clearPagerMessages {} {

    if [catch {
        .pagerWindow.main.text.text delete 0 end

    } result] {
        puts "Error was $result"
    }
}

#-----------------------------------------------------------------------
# Apodization widget 
#-----------------------------------------------------------------------

proc create_pair {w name value text} {

  image create photo $name -file  "$::env(GCP_DIR)/control/code/unix/viewer_src/$name.gif" -width 180 -height 73
  set f [frame $w.$name]

  radiobutton $f.rad -variable $w.type -value $value -text $text
#  canvas      $f.can -width 180 -height 73
#  $f.can create image 0 0 -image $name -anchor nw
  button      $f.can -width 180 -height 73 -image $name -command "$f.rad select"

  pack $f.rad -side left
  pack $f.can -side right
  pack $f -fill x

  return $f
}

proc create_apodization_dialog {w} {

  frame $w
  set none     [create_pair $w "rectangular" 0 "None (rectangular)"]
  set triangle [create_pair $w "triangular"  1 "Triangle"]
  set hamming  [create_pair $w "hamming"     2 "Hamming"]
  set hann     [create_pair $w "hann"        3 "Hann"]
  set cosine   [create_pair $w "sine"        4 "Cosine/Sine"]
  set sinc     [create_pair $w "sinc"        5 "Lanczos (Sinc)"]

  $w.hamming.rad select

  pack $none $triangle $hamming $hann $cosine $sinc -side top

  return $w
}

proc toggleApodDisplay {w} {

  if { $::apod(display) } {
    set ::apod(display) 0
    pack forget $w.d
  } else {
    set ::apod(display) 1
    pack $w.d
  }
}

#-----------------------------------------------------------------------
# Start the viewer
#-----------------------------------------------------------------------

start_viewer
