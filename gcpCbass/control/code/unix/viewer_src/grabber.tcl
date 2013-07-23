#=======================================================================
# Tcl code for the grabber widget of the viewer program.  This code
# communicates with C-layer code defined in tclgrabber.c
#=======================================================================

# Variables that control whether loca trace callbacks result in
# commands sent to the control program

set ::grabber(sendChannelCmd)       1 ;# True if a sendchannel command should be sent
set ::grabber(sendXImDirCmd)        1 ;# True if a ximdir command should be sent
set ::grabber(sendYImDirCmd)        1 ;# True if a yimdir command should be sent
set ::grabber(sendDkRotSenseCmd)    1 ;# True if a deck rotation sense command should be sent
set ::grabber(sendFlatfieldTypeCmd) 1 ;# True if a flatfield command should be sent
set ::grabber(redraw)               0 ;# True to redraw after certain commands

set ::grabber(chan0Text)            "Channel 0";
set ::grabber(chan1Text)            "Channel 1";
set ::grabber(chan2Text)            "Channel 2";
set ::grabber(chan3Text)            "Channel 3";

# Variables that will be written to when configuration commands are
# received from the control program

set ::grabber(rmtChannel)           0;# default grabber channel
set ::grabber(rmtPtel)              0;# The pointing telescope associated with the current channel
set ::grabber(rmtCombine)           1;# The number of images to combine
set ::grabber(rmtFlatfieldType)     0;# Type of flatfielding for images
set ::grabber(rmtFov)            12.0;# Field of view, in arcminutes of the camera
set ::grabber(rmtAspect)          0.8;# The aspect ratio of the optical camera
set ::grabber(rmtCollimation)     0.0;# The angle at which the x and y axes of the optical camera correspond to 
                                      # az and el, at dk=0

set ::grabber(rmtDk)              0.0;# Variable in which the angle of the deck will be recorded
set ::grabber(rmtXimdir)            1;# The sign of the x-correction (1 = upright or -1 = inverted).
set ::grabber(rmtYimdir)            1;# The sign of the y-correction (1 = upright or -1 = inverted).
set ::grabber(rmtDkRotSense)        1;# The sense of the deck rotation (1 = CW or -1 = CCW).

set ::grabber(rmtIxmin)             0;
set ::grabber(rmtIxmax)             0;
set ::grabber(rmtIymin)             0;
set ::grabber(rmtIymax)             0;
set ::grabber(rmtInc)               0;
set ::grabber(rmtRem)             one;

set ::grabber(rmtRedraw)            1;# True if the currently displayed image should be redrawn in response to a remote command
set ::grabber(rmtReconf)            1;# True if the currently displayed parameters should be updated in response to a remote command

# Variables that will be written to when the im_reconfigure command is
# executed by the local viewer program, in response to remote
# reconfiguration of the frame grabber

set ::grabber(lclChannel)           0;# default grabber channel
set ::grabber(lclCombine)           1;# The number of images to combine
set ::grabber(lclFlatfieldType)     0;# Type of flatfielding for images
set ::grabber(lclFov)            12.0;# Field of view, in arcminutes of the camera
set ::grabber(lclAspect)          0.8;# The aspect ratio of the optical camera
set ::grabber(lclCollimation)     0.0;# The angle at which the x and y axes of the optical camera correspond to 
                                      # az and el, at dk=0

set ::grabber(lclDk)              0.0;# Variable in which the angle of the deck will be recorded
set ::grabber(lclXimdir)            1;# The sign of the x-correction (1 = upright or -1 = inverted).
set ::grabber(lclYimdir)            1;# The sign of the y-correction (1 = upright or -1 = inverted).
set ::grabber(lclDkRotSense)        1;# The sense of the deck rotation (1 = CW or -1 = CCW).

set ::grabber(cmap)              grey;
set ::grabber(menuCmap)          grey;

set ::grabber(grid)               off;
set ::grabber(bullseye)           off;
set ::grabber(compass)             on;
set ::grabber(crosshair)           on;
set ::grabber(boxes)               on;

# Set up frame grabber variable traces for local variables

grabber im_vars channel ::grabber(lclChannel)
trace variable ::grabber(lclChannel) w grabber_lcl_channel_trace_callback

grabber im_vars combine ::grabber(lclCombine)
trace variable ::grabber(lclCombine) w grabber_lcl_combine_trace_callback

grabber im_vars chan0 ::grabber(chan0Text)
trace variable ::grabber(chan0Text) w grabber_chan0_trace_callback

grabber im_vars chan1 ::grabber(chan1Text)
trace variable ::grabber(chan1Text) w grabber_chan1_trace_callback

grabber im_vars chan2 ::grabber(chan2Text)
trace variable ::grabber(chan2Text) w grabber_chan2_trace_callback

grabber im_vars chan3 ::grabber(chan3Text)
trace variable ::grabber(chan3Text) w grabber_chan3_trace_callback

grabber im_vars flatfield ::grabber(lclFlatfieldType)
trace variable ::grabber(lclFlatfieldType) w grabber_lcl_flatfieldType_trace_callback

grabber im_vars fov ::grabber(lclFov)
trace variable ::grabber(lclFov) w grabber_lcl_fov_trace_callback

grabber im_vars aspect ::grabber(lclAspect)
trace variable ::grabber(lclAspect) w grabber_lcl_aspect_trace_callback

grabber im_vars collimation ::grabber(lclCollimation)
trace variable ::grabber(lclCollimation) w grabber_lcl_collimation_trace_callback

grabber im_vars ximdir ::grabber(lclXimdir)
trace variable ::grabber(lclXimdir) w grabber_lcl_ximdir_trace_callback

grabber im_vars yimdir ::grabber(lclYimdir)
trace variable ::grabber(lclYimdir) w grabber_lcl_yimdir_trace_callback

grabber im_vars dkRotSense ::grabber(lclDkRotSense)
trace variable ::grabber(lclDkRotSense) w grabber_lcl_dkRotSense_trace_callback

grabber im_vars xpeak ::grabber(lclXpeak)
grabber im_vars ypeak ::grabber(lclYpeak)
trace variable ::grabber(lclYpeak) w grabber_lcl_peak_trace_callback

grabber im_vars grid  ::grabber(grid)
grabber im_vars bull  ::grabber(bullseye)
grabber im_vars comp  ::grabber(compass)
grabber im_vars cross ::grabber(crosshair)
grabber im_vars box   ::grabber(boxes)

grabber im_vars cmap  ::grabber(cmap)
trace variable ::grabber(cmap) w grabber_cmap_trace_callback

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_image_widget {w} {

  set plot $w

#
# Create a toplevel frame to contain the plot and its associated
# configuration and informational widgets.
#

  toplevel    $plot -class ImMonitorPlot
  wm title    $plot "Frame Grabber"
  wm iconname $plot "Image"
  wm withdraw $plot
  wm protocol $plot WM_DELETE_WINDOW "quit_image $w"

# Create the main components of the widget.

  create_image_main_widget $plot.main

# Create the configuration widgets:

  create_image_control_widget    $plot.imageControl
  create_grabber_control_widget  $plot.grabberControl
  create_pointing_control_widget $plot.pointingControl

#
# Create a menubar with a file menu that contains a quit button.
#
  set mbar [frame $w.menu -relief raised -bd 2]

  menubutton $mbar.file -text {File} -menu $mbar.file.menu
  set m [menu $mbar.file.menu -tearoff 0]
  $m add command -label {Quit} -command "quit_image $w"
  pack $mbar.file -side left

#
# Create a grid-display menu.
#

  menubutton $mbar.grid -text {Grid} -menu $mbar.grid.menu
  set m [menu $mbar.grid.menu -tearoff 1]
  $m add command -label {Grid display} -state disabled
  $m add separator

#
# Select these to initialize
#

  $m invoke 1

  $m add checkbutton -label "Grid on"      -variable ::grabber(grid)      -onvalue on -offvalue off -command "toggle_grid"

  $m add checkbutton -label "Bullseye on"  -variable ::grabber(bullseye)  -onvalue on -offvalue off -command "toggle_bullseye"

  $m add checkbutton -label "Compass on"   -variable ::grabber(compass)   -onvalue on -offvalue off -command "toggle_compass"

  $m add checkbutton -label "Crosshair on" -variable ::grabber(crosshair) -onvalue on -offvalue off -command "toggle_crosshair"

  $m add checkbutton -label "Show Boxes"   -variable ::grabber(boxes)     -onvalue on -offvalue off -command "toggle_boxes"

  pack $mbar.grid -side left

#-----------------------------------------------------------------------
# Create a cursor-input-mode menu.
#-----------------------------------------------------------------------

  menubutton $mbar.input -text {Cursor} -menu $mbar.input.menu
  set m [menu $mbar.input.menu -tearoff 1]
  $m add command -label {Cursor input mode} -state disabled
  $m add separator
  $m add radiobutton -variable $m -value none   -label {None}   -underline 0
  $m add radiobutton -variable $m -value offset -label {Offset} -underline 0
  $m add radiobutton -variable $m -value zoom   -label {Zoom}   -underline 0
  $m add radiobutton -variable $m -value stat   -label {Stats}  -underline 0
  $m add radiobutton -variable $m -value fid    -label {Change Contrast} -underline 0
  $m add radiobutton -variable $m -value inc    -label {Include Region}  -underline 0
  $m add radiobutton -variable $m -value exc    -label {Exclude Region}  -underline 0

  pack $mbar.input -side left

  bind_short_cut $plot n "init_image_none_cursor   $plot $plot.main.p"
  bind_short_cut $plot o "init_image_offset_cursor $plot $plot.main.p"
  bind_short_cut $plot z "init_image_zoom_cursor   $plot $plot.main.p"
  bind_short_cut $plot s "init_image_stat_cursor   $plot $plot.main.p"
  bind_short_cut $plot c "init_image_fid_cursor    $plot $plot.main.p"
  bind_short_cut $plot i "init_image_inc_cursor    $plot $plot.main.p"
  bind_short_cut $plot e "init_image_exc_cursor    $plot $plot.main.p"

#-----------------------------------------------------------------------
# Link a callback to the value of the input-mode variable.
#-----------------------------------------------------------------------

  global $m
  trace variable $m w "change_image_cursor_mode $plot"

#-----------------------------------------------------------------------
# Create a colormap menu.
#-----------------------------------------------------------------------

  menubutton $mbar.cmap -text {Colormap} -menu $mbar.cmap.menu
  set cmap [menu $mbar.cmap.menu -tearoff 1]
  $cmap add command -label {Display Colormap} -state disabled
  $cmap add separator
  $cmap add radiobutton -variable ::grabber(menuCmap) -value grey    -label {Grey}
  $cmap add radiobutton -variable ::grabber(menuCmap) -value aips    -label {Aips}
  $cmap add radiobutton -variable ::grabber(menuCmap) -value heat    -label {Heat}
  $cmap add radiobutton -variable ::grabber(menuCmap) -value rainbow -label {Rainbow}

  pack $mbar.cmap -side left

#-----------------------------------------------------------------------
# Create a menubar for controls
#-----------------------------------------------------------------------

  menubutton $mbar.cntl -text {Controls} -menu $mbar.cntl.menu
  set m [menu $mbar.cntl.menu -tearoff 1]

  $m add command -label {Image Configuration} -command "wm deiconify $w.imageControl;    reveal $w.imageControl"
  $m add command -label {Grabber Control}     -command "wm deiconify $w.grabberControl;  reveal $w.grabberControl"
  $m add command -label {Pointing Control}    -command "wm deiconify $w.pointingControl; reveal $w.pointingControl"
  $m add separator
  $m add command -label {All Controls}        -command "reveal_controls $w"

  pack $mbar.cntl -side left

#
# Create a find menu
#
  menubutton $mbar.find -text {Find} -menu $mbar.find.menu
  set m [menu $mbar.find.menu -tearoff 0]
#
# Create its member(s)
#
  $m add command -label {Find gcpViewer} -command "reveal ."
  pack $mbar.find -side left
#
# Link a callback to the value of the colormap variable.
#
  trace variable ::grabber(menuCmap) w change_image_colormap

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

# Arrange the vertical panes of the widget.

  pack $plot.main -side top -fill both -expand true

#
# Create a global configuration array for recording the configuration of
# the target area of the image widget
#
  upvar #0 $plot.main c

#  set c(page) 0        ;# The id of the monitor page used to query registers.
#  set c(xc) 0          ;# The X coordinate of the center of the rings.
#  set c(yc) 0          ;# The Y coordinate of the center of the rings.
#  set c(dx) 1          ;# The increment in the X-axis crossing point per ring.
#  set c(dy) 1          ;# The increment in the Y-axis crossing point per ring.
#  set c(step) 1.0      ;# The current grid interval.
#  set c(update_step) 1 ;# True if c(step) is out of date.
#  set c(snap) 0        ;# True to only allow moves that lie on the grid lines.
#  set c(xoffdir) -1    ;# The sign of the x-correction (-1 or 1).
#  set c(yoffdir) -1    ;# The sign of the x-correction (-1 or 1).
#  set c(tv_angle) 0.0  ;# The deck angle at which the TV picture is upright.
#  set c(dk) 0.0        ;# The current deck angle.
#  set c(compass) 1     ;# Draw the compass lines if true.
#  set c(drawn) 0       ;# True if the compass line isn't currently drawn.
#  set c(angle) 0.0     ;# The clockwise rotation angle of the sky on the tv,
                        ;# computed by the last call to redraw_offset_compass.

#  set c(collimation) 0.0;
#  set c(aspect) 0.8;

# 
# Create a widget for messages
# 
  create_message_widget $w.msg
  pack $w.msg -side top -fill x -expand true

  $w.msg.l configure -justify center -text "Use the Controls menu to control the frame grabber"

#
# Create a register page. When the offset dialog is displayed this will
# be used to request updates of the tv angle and deck angle from the
# monitor stream.
#
#  set c(page) [monitor add_page]
#
# Arrange to request monitor data of pertinent registers when the window
# is mapped, and to unrequest them when the window is not mapped.
#
#  bind $plot.main <Map>   [list update_image_dialog_regs $w]
#  bind $plot.main <Unmap> [list update_image_dialog_regs $w]

#
# Arrange for the cursor position to be updated whenever the cursor moves
#
  bind $plot.main.p <Motion> "update_image_cursor $w %x %y"

# Arrange for the image to be redrawn anytime the user resizes the widget

  bind $plot.main <Configure> "grabber im_redraw"

# Add return the image widget.

  return $plot
}

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_image_main_widget {w} {
#
# Frame the plot area.
#
  frame $w -width 15c -height 10c
#
# Create an area for mouse-button descriptions.
#
  create_button_show_widget $w.b
#
# Frame the area that will contain the PGPLOT widget.
#
# If we don't specify the -maxcolors keyword in the pgplot command; this will
# default to 100.
#
  pgplot $w.p -width 15c -height 15c -bg black -relief sunken -bd 2

  pack   $w.b -side top -fill x
  pack   $w.p -side left -fill both -expand true

  return $w
}

#-----------------------------------------------------------------------
# This is a variable-trace callback attached to the image widget's colormap
# variable. Whenever a new input mode is selected, this function
# cancels the previous mode and sets up the new one.
#
# Input:
#  plot    The plot widget to which the mode refers.
#  name    The name of the cmap variable.
#  ...
#-----------------------------------------------------------------------
proc change_image_colormap {args} {
#
# Set up the new colormap
#
  grabber im_colormap $::grabber(lclChannel) $::grabber(menuCmap)

  if {$::grabber(redraw)} {
      grabber im_redraw
  }

  set ::grabber(redraw) 1;
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
proc toggle_grid {} {

#
# Toggle The Set up the new cursor mode.
#
  grabber im_toggle_grid $::grabber(lclChannel)

  if {$::grabber(redraw)} {
      grabber im_redraw
  }

  set ::grabber(redraw) 1;
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user selects a
# channel by pressing the Channel button
#-----------------------------------------------------------------------
proc select_fg_channel {plot channel args} {
  upvar #0 $channel chan
  upvar #0 $plot p

  set ::grabber(lclChannel) $chan

  grabber im_select_channel $chan

  grabber im_reconfigure $chan $chan

  grabber im_redraw
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user selects one of
# the X-image direction buttons
#-----------------------------------------------------------------------
proc select_ximdir {plot ximdir args} {
  upvar #0 $ximdir xdir

#
# Select the new channel
#

  if {$::grabber(sendXImDirCmd)} {
    if [catch {
	if {$xdir == 1} {
	    control send [concat "setOpticalCameraXImDir upright, chan=" $::grabber(lclChannel)]
	} else {
	    control send [concat "setOpticalCameraXImDir inverted, chan=" $::grabber(lclChannel)]
	}
    } result] {
      report_error "$result"
      bell
    }
  } else {

    # Tell the monitor layer that the direction has changed too -- this
    # will be needed to correctly draw the image compass

    grabber im_ximdir $::grabber(lclChannel) $xdir

    if {$::grabber(redraw)} {
	grabber im_redraw
    }

  }

  set ::grabber(sendXImDirCmd) 1
  set ::grabber(redraw) 1
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user selects one of
# the Y-image direction buttons
#-----------------------------------------------------------------------
proc select_yimdir {plot yimdir args} {
  upvar #0 $yimdir ydir

#
# Select the new channel
#

  if {$::grabber(sendYImDirCmd)} {
    if [catch {
	if {$ydir == 1} {
	    control send [concat "setOpticalCameraYImDir upright, chan=" $::grabber(lclChannel)]
	} else {
	    control send [concat "setOpticalCameraYImDir inverted, chan=" $::grabber(lclChannel)]
	}
    } result] {
      report_error "$result"
      bell
    }
  } else {

    # Tell the monitor layer that the direction has changed too -- this
    # will be needed to correctly draw the image compass

    grabber im_yimdir $::grabber(lclChannel) $ydir

    if {$::grabber(redraw)} {
	grabber im_redraw
    }
  }

  set ::grabber(sendYImDirCmd) 1
  set ::grabber(redraw) 1
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user sets the sense
# of the deck rotation
#-----------------------------------------------------------------------
proc select_dkRotSense {plot dkRotSense args} {
  upvar #0 $dkRotSense sense
  upvar #0 $plot p
#
# Select the new channel
#

  if {$::grabber(sendDkRotSenseCmd)} {
    if [catch {

	if {$sense == 1} {
	    control send "setDeckAngleRotationSense cw"
	} else {
	    control send "setDeckAngleRotationSense ccw"
	}

    } result] {
      report_error "$result"
      bell
    }
  }

  set ::grabber(sendDkRotSenseCmd) 1
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user selects a 
# flatfielding type
#-----------------------------------------------------------------------
proc select_flatfieldType {plot flatfieldType args} {

  upvar #0 $flatfieldType ffType

#
# Select the new channel
#

  if {$::grabber(sendFlatfieldTypeCmd)} {

    if [catch {

	if {$ffType == 0} {
	    control send [concat "configureFrameGrabber flatfield=none, chan=" $::grabber(lclChannel)]
	} elseif {$ffType == 1} {
	    control send [concat "configureFrameGrabber flatfield=row, chan=" $::grabber(lclChannel)]
	} else {
	    control send [concat "configureFrameGrabber flatfield=image, chan=" $::grabber(lclChannel)]
	}
	
    } result] {
	report_error "$result"
	bell
    }
  }

  # Set the value locally

  grabber im_set_flatfield $::grabber(lclChannel) $ffType

  set ::grabber(sendFlatfieldTypeCmd) 1
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user toggles the
# grid menu button
#-----------------------------------------------------------------------
proc toggle_bullseye {} {
#
# Toggle the Set up the new cursor mode.
#
  grabber im_toggle_bullseye $::grabber(lclChannel)
  grabber im_redraw
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user toggles the
# compass menu button
#-----------------------------------------------------------------------
proc toggle_compass {} {
#
# Toggle the Set up the new cursor mode.
#
  grabber im_toggle_compass $::grabber(lclChannel)
  grabber im_redraw
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user toggles the
# crosshair menu button
#-----------------------------------------------------------------------
proc toggle_crosshair {} {
#
# Toggle the Set up the new cursor mode.
#
  grabber im_toggle_crosshair $::grabber(lclChannel)
  grabber im_redraw
}

#-----------------------------------------------------------------------
# This is a variable-trace callback called when the user toggles the
# boxes menu button
#-----------------------------------------------------------------------
proc toggle_boxes {} {
#
# Toggle the Set up the new cursor mode.
#
  grabber im_toggle_boxes $::grabber(lclChannel)
  grabber im_redraw
}

#-----------------------------------------------------------------------
# This is a private callback used by the image dialog. Given a direction
# it reads the step size and camera angle to determine by how much and
# in what direction to offset the telescope.
#
# Input:
#  w       The tk pathname of the dialog.
#  x y     The position on the canvas at which the user clicked.
#-----------------------------------------------------------------------
proc send_image_offset {w x y} {
#
# Get the configuration array of the canvas.
#
  upvar #0 $w.main c
#
# Get the horizontal and vertical increments, world coordinates.
#
  set xw [$w.main.p world x $x]
  set yw [$w.main.p world y $y]

  set tx [expr {$::grabber(lclXimdir) * ($xw / $::DTOAS)}]

# Aspect ratio has already been effectively applied in the plot
# limits -- don't apply again here!
#
#  set ty [expr {$::grabber(lclYimdir) * ($yw / $::DTOAS) * $::grabber(lclAspect)}]

  set ty [expr {$::grabber(lclYimdir) * ($yw / $::DTOAS)}]

# The optical camera collimation is always angle for which a positive
# angle is CW.  The Deck angle, however, may be arbitrary.

  set coll [expr {($::grabber(lclCollimation) + $::grabber(lclDkRotSense) * $::grabber(lclDk)) * $::PI/180}]

  set dh [expr {$tx * cos($coll) - $ty * sin($coll)}]
  set dv [expr {$tx * sin($coll) + $ty * cos($coll)}]

#
# Send offset commands for the az and el axes.
#
  if [catch {
    control send "tv_offset $dh, $dv"
  } result] {
    report_error "$result"
    bell
    return
  }
}

#-----------------------------------------------------------------------
# This is a private callback used by the image dialog. Given a direction
# it reads the step size and camera angle to determine by how much and
# in what direction to offset the telescope.
#
# Input:
#  w       The tk pathname of the dialog.
#  x y     The position on the canvas at which the user clicked.
#-----------------------------------------------------------------------
proc send_centroid_offsets {w} {
#
# Get the configuration array of the canvas.
#
  upvar #0 $w.main c
#
# Get the horizontal and vertical increments, world coordinates.
#
  set xw [$w.top.left.x.sec get]
  set yw [$w.top.left.y.sec get]

# Now convert from world coordinates to sky offsets in degrees

  set skyoffsets [grabber worldToSky $::grabber(lclChannel) $xw $yw]

  set xsky [lindex $skyoffsets 0]
  set ysky [lindex $skyoffsets 1]

#
# Send offset commands for the az and el axes.
#
  if [catch {
    control send "tv_offset $xsky, $ysky"
  } result] {
    report_error "$result"
    bell
    return
  }
}

#-----------------------------------------------------------------------
# This is a private callback called when the user releases the slider 
# used to select the number of frames to combine
#-----------------------------------------------------------------------
proc setCombine {scale} {

#
# Send the frame grabber a command
#
  if [catch {
      control send [concat "configureFrameGrabber combine=[$scale get], chan=" $::grabber(lclChannel)]
  } result] {
    report_error "$result"
    bell
    return
  }

# Set the combine locally as well

  grabber im_set_combine $::grabber(lclChannel) [$scale get]
}

#-----------------------------------------------------------------------
# This is a private callback used by the image dialog. It tells the control
# program to grab the next frame from the frame grabber.
#-----------------------------------------------------------------------
proc grab_next {} {

#
# Send the frame grabber a command
#
  if [catch {
    control send "grabFrame chan=$::grabber(lclChannel)"
  } result] {
    report_error "$result"
    bell
    return
  }
}
#-----------------------------------------------------------------------
# This is a private callback used by the image dialog. It tells the control
# program to center the star
#-----------------------------------------------------------------------
proc grab_center {} {
#
# Send the frame grabber a command
#
  if [catch {
    control send "center"
  } result] {
    report_error "$result"
    bell
    return
  }
}

#-----------------------------------------------------------------------
# Change the image viewport size from the entry widget in the image
# dialog.  Called when the user hits return in the fov entry widget
#-----------------------------------------------------------------------
proc change_image_fov {w update} {
  upvar #0 $w.main c
 
#
# Get the Field of View
#

  set fov [$w.imWidth.ent get]

#
# Update the grid interval locally for the current channel
#

  grabber im_change_fov $::grabber(lclChannel) $fov 

  if {$update} {
      if [catch {control send [concat "setOpticalCameraFov fov=" $fov ", chan=" $::grabber(lclChannel)]} result] {
      report_error "$result"
      bell
    }
  } else {
  
# Redraw to update the viewport scale.  Only redraw if the command
# originated from the control system.  When the user presses Return in
# the fov box, this generates a command that gets sent to the control
# system, which causes all conneted image clients, including this one,
# to be updated from the control system.
#  
# By only updating on receipt of the command fom the control system,
# we can be sure that the image is redrawn only once,
 
    image_redraw
  }

#
# And reset the focus to the pgplot window if the call originated from
# the image dialog.  Else set the focus back to the main window
#
    if {$update} {
	focus $w
    }
}

#-----------------------------------------------------------------------
# Change the image aspect ratio.  Called when the user hits return in
# the aspect ratio entry widget
#-----------------------------------------------------------------------
proc change_image_aspect {w update} {

  upvar #0 $w c

# Get the aspect ratio

  if [catch {set ar [$w.aspect.ent get]} result] {
    puts "error occurred: $result"
  }

# Update the aspect ratio locally

  grabber im_change_aspect $::grabber(lclChannel) $ar

  if {$update} {

    if [catch {control send [concat "setOpticalCameraAspect aspect=" $ar ", chan=" $::grabber(lclChannel)]} result] {
       report_error "$result"
       bell
     }

  } else {

# Redraw to update the viewport scale.

     grabber im_redraw
  }

# And reset the focus to the pgplot window

  if {$update} {
    focus $w
  }
}

#-----------------------------------------------------------------------
# Change the collimation angle of an optical camera
#-----------------------------------------------------------------------
proc change_image_collimation {w update} {
  upvar #0 $w c

#
# Get the rotation angle into  
#

  set rot [$w.rotAngle.ent get]

    puts "Inside change_image_collimation: $::grabber(lclChannel) $rot"

#
# Update the collimation, both locally and remotely
#

  grabber im_change_rotAngle $::grabber(lclChannel) $rot

  if {$update} {
    if [catch {control send [concat "setOpticalCameraRotation angle=" $rot ", chan=" $::grabber(lclChannel)]} result] {
      report_error "$result"
      bell
    }
  } else {
    grabber im_redraw
  }
    
#
# And reset the focus to the pgplot window
#
  if {$update} {
    focus $w
  }
}

#-----------------------------------------------------------------------
# Find the centroid of the image
#-----------------------------------------------------------------------
proc find_centroid {w args} {
  upvar #0 $w c
#
# Get the centroid
#
  set centroid [grabber im_centroid $::grabber(lclChannel)]
  set xcntr [lindex $centroid 0]
  set ycntr [lindex $centroid 1]
#
# And install the centroid coordinates in the move box.
#
  set x $w.top.left.x.sec
  set y $w.top.left.y.sec

  $x delete 0 end
  $y delete 0 end

  $x insert end $xcntr
  $y insert end $ycntr

#
# And redraw
# 
  image_redraw
#
# And reset the focus to the pgplot window
#
  focus $w
}

#-----------------------------------------------------------------------
# Change the grid interval from the entry widgets in the image
# dialog.
#-----------------------------------------------------------------------
proc change_image_step {w args} {

  upvar #0 $w c
#
# Get the interval.
#
  set sec [$w.gridInt.ent get]
#
# Update the grid interval
#
  grabber im_change_step $::grabber(lclChannel) $sec
#
# And redraw
# 
  image_redraw
#
# And reset the focus to the pgplot window
#
  focus $w
}

#-----------------------------------------------------------------------
# If the image dialog is mapped and the viewer has a source of monitor
# data, ask to be sent updates of pertinent registers. Otherwise
# tell the viewer that we are no longer interested in these registers.
# This removes the overhead of monitoring these registers when we aren't
# doing offset pointing.
#
# Input:
#  w       The Tk pathname of the offset dialog.
#-----------------------------------------------------------------------
proc update_image_dialog_regs {w} {

  upvar #0 $w.main c    ;#  The plot configuration array.

#
# Discard any existing register fields.
#
 if {[catch {
   monitor delete_fields $c(page)
 } result] } {
   report_error "image_dialog: $result"
 }

  set c(compass) 0
#
# Is the offset dialog mapped?
#
  if {[winfo ismapped $w]} {
    if {[monitor have_stream]} {
      if {[catch {
#
# Arrange to have updates of the deck angle reported in the
# variable ::.offset.canvas\(dk\).
#
	set tag [monitor add_field $c(page)]
#	monitor configure_field $c(page) $tag {antenna0.tracker.actual[2]} \
#	    floating {} 0 15 0 {} 0 0 0 0 0
#	monitor field_variables $c(page) $tag $w.main\(dk\) ::unused ::unused
#	monitor field_variables $c(page) $tag ::grabber\(dk\) ::unused ::unused
	set c(compass) 1
      } result]} {
	report_error "image_dialog: $result"
      }
    }
  }
#
# Tell the monitor layer to reconfigure itself and resume monitoring.
#
  if {[monitor have_stream]} {
    monitor reconfigure
  }
}

#-----------------------------------------------------------------------
# Procedure to redraw image data and set up bindings and cursor
#-----------------------------------------------------------------------
proc image_redraw {} {

# Redraw the image

  grabber im_redraw
}

#-----------------------------------------------------------------------
# Establish network connections to the frame grabber image monitoring
# service of the control program.
#-----------------------------------------------------------------------
proc connect_imhost {} {

# Attempt to establish a monitor connection.  Note that if this fails,
# the existing stream will be left connected by the "monitor host"
# command.

    set reset 0
    if [catch {
      set reset [grabber imhost $::viewer(host)]
    } result] {
      report_error $result
    }

# And attempt to open the image widget.

    set image [open_image]
    if [catch {
      set reset [grabber open_image $image.main.p/xtk]
    } result] {
      report_error $result
    }

# Make sure bindings have their default values.

    $image.main.p setcursor norm 0.0 0.0 1

# Reveal the widget

    wm deiconify $image
    reveal_controls $image

# Redraw the image

    image_redraw
}

#-----------------------------------------------------------------------
# load_config command: Add an image to the viewer and configure it via
# a script of special commands.
#
# Input:
#  body     A script containing Tcl plot configuration commands.
# Output:
#  plot     The path name of the plot widget.
#-----------------------------------------------------------------------
proc open_image {} {
  set plot .im
  return $plot
}


#-----------------------------------------------------------------------
# Delete an image.
#-----------------------------------------------------------------------
proc quit_image {w} {

#
# Close the socket associated with this image display.
#

  grabber im_disconnect

#
# And withdraw the main image widget, and any ancillary widgets
#

  wm withdraw $w
  wm withdraw $w.grabberControl
  wm withdraw $w.imageControl
  wm withdraw $w.pointingControl
}

#=======================================================================
# Remote trace callbacks: these are invoked whenever a variable is 
# written to from the C control layer in response to an external
# configuration command
#=======================================================================

proc grabber_rmt_channel_trace_callback {args} {
}

#-----------------------------------------------------------------------
# If we have changed the configuration parameters for the channel that
# is locally selected, call the procedure to reconfigure the dialogs
# and redisplay the image
#-----------------------------------------------------------------------
proc grabber_rmt_reconf_trace_callback {args} {
    grabber im_reconfigure $::grabber(rmtChannel) $::grabber(lclChannel) 
}

#-----------------------------------------------------------------------
# If we have changed the configuration parameters for the channel that
# is locally selected, call the procedure to reconfigure the dialogs
# and redisplay the image
#-----------------------------------------------------------------------
proc grabber_rmt_redraw_trace_callback {args} {

    if {$::grabber(rmtChannel) == $::grabber(lclChannel)} {
	grabber im_redraw
    }
}

#-----------------------------------------------------------------------
# Called when a pointing telescope association is changed
#-----------------------------------------------------------------------
proc grabber_rmt_ptel_trace_callback {args} {
    grabber im_set_ptel $::grabber(rmtChannel) $::grabber(rmtPtel)
}

proc grabber_chan0_trace_callback {args} {
    .im.grabberControl.right.chan.chan0 configure -text $::grabber(chan0Text)
}

proc grabber_chan1_trace_callback {args} {
    .im.grabberControl.right.chan.chan1 configure -text $::grabber(chan1Text)
}

proc grabber_chan2_trace_callback {args} {
    .im.grabberControl.right.chan.chan2 configure -text $::grabber(chan2Text)
}

proc grabber_chan3_trace_callback {args} {
    .im.grabberControl.right.chan.chan3 configure -text $::grabber(chan3Text)
}

#-----------------------------------------------------------------------
# Called when a flatfielding type is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_flatfieldType_trace_callback {args} {
    grabber im_set_flatfield $::grabber(rmtChannel) $::grabber(rmtFlatfieldType)
}

#-----------------------------------------------------------------------
# Called when a combine value is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_combine_trace_callback {args} {
    grabber im_set_combine $::grabber(rmtChannel) $::grabber(rmtCombine)
}

#-----------------------------------------------------------------------
# Called when an aspect ratio is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_aspect_trace_callback {args} {
    grabber im_change_aspect $::grabber(rmtChannel) $::grabber(rmtAspect)
}

#-----------------------------------------------------------------------
# Called when a field-of-view is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_fov_trace_callback {args} {
    grabber im_change_fov $::grabber(rmtChannel) $::grabber(rmtFov)
}

#-----------------------------------------------------------------------
# Called when a collimation is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_collimation_trace_callback {args} {
    grabber im_change_rotAngle $::grabber(rmtChannel) $::grabber(rmtCollimation)
}

#-----------------------------------------------------------------------
# Called when an x direction is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_ximdir_trace_callback {args} {
    grabber im_ximdir $::grabber(rmtChannel) $::grabber(rmtXimdir)
}

#-----------------------------------------------------------------------
# Called when a y direction is remotely changed
#-----------------------------------------------------------------------
proc grabber_rmt_yimdir_trace_callback {args} {
    grabber im_yimdir $::grabber(rmtChannel) $::grabber(rmtYimdir)
}

#-----------------------------------------------------------------------
# Called when peak values are sent from the control system
#-----------------------------------------------------------------------
proc grabber_rmt_peak_trace_callback {args} {
    grabber im_set_centroid $::grabber(rmtChannel) $::grabber(rmtXpeak) $::grabber(rmtYpeak)
}

#-----------------------------------------------------------------------
# Called when a search box is received from the control system
#-----------------------------------------------------------------------
proc grabber_rmt_inc_trace_callback {args} {

    if {$::grabber(rmtInc) == 1} {
	grabber im_inc $::grabber(rmtChannel) $::grabber(rmtIxmin) $::grabber(rmtIymin) $::grabber(rmtIxmax) $::grabber(rmtIymax) 
    } else {																      
	grabber im_exc $::grabber(rmtChannel) $::grabber(rmtIxmin) $::grabber(rmtIymin) $::grabber(rmtIxmax) $::grabber(rmtIymax) 
    }
}

#-----------------------------------------------------------------------
# Called when a search box delete command is received from the control
# system
#-----------------------------------------------------------------------
proc grabber_rmt_rem_trace_callback {args} {

    if {$::grabber(rmtRem) == 1} {
	grabber im_del_box $::grabber(rmtChannel) $::grabber(rmtIxmin) $::grabber(rmtIymin) 
    } else {
	grabber im_del_all_box $::grabber(rmtChannel)
    }
}

#-----------------------------------------------------------------------
# Called when the deck rotation sense is set from the control system
#-----------------------------------------------------------------------

proc grabber_rmt_dkRotSense_trace_callback {args} {

    # Select the appropriate button

    set ::grabber(sendDkRotSenseCmd) 0

    if {$::grabber(dkRotSense) == 1} {
	.im.imageControl.dkRot.cw  select
    } elseif {$::grabber(dkRotSense) == -1} {
	.im.imageControl.dkRot.ccw select
    }

}

#=======================================================================
# Local trace callbacks: these are invoked whenever a variable is 
# written to from the local TCL/C layer.  
#
# These are written to by the 'grabber im_reconfigure' command, which
# is called when commands received from the control program change the
# configuration of the currently selected channel, so they should not
# re-foward commands to the control program
#=======================================================================

proc grabber_lcl_flatfieldType_trace_callback {args} {

    # Select the appropriate button.  Set the send flag to false (0),
    # so that the trace callback invoked by selecting the button
    # doesn't forward the command to the control system

    set ::grabber(sendFlatfieldTypeCmd) 0
    set ::grabber(redraw) 0

    if {$::grabber(lclFlatfieldType) == 0} {
	.im.grabberControl.left.flatField.none select
    } elseif {$::grabber(lclFlatfieldType) == 1} {
	.im.grabberControl.left.flatField.row select
    } else {
	.im.grabberControl.left.flatField.image select
    }
}

proc grabber_lcl_ptel_trace_callback {args} {

}

proc grabber_lcl_combine_trace_callback {args} {

# Set the slider to the appropriate value

    .im.grabberControl.left.nCombine.ncombine set $::grabber(lclCombine)
}

proc grabber_lcl_aspect_trace_callback {args} {

# Insert the new value into the entry box

    .im.imageControl.aspect.ent delete 0 end
    .im.imageControl.aspect.ent insert 0 $::grabber(lclAspect)
}

proc grabber_lcl_fov_trace_callback {args} {

# Insert the new value into the entry box

    .im.imageControl.imWidth.ent delete 0 end
    .im.imageControl.imWidth.ent insert 0 $::grabber(lclFov)
}

proc grabber_lcl_collimation_trace_callback {args} {

# Insert the new value into the entry box

    .im.imageControl.rotAngle.ent delete 0 end
    .im.imageControl.rotAngle.ent insert 0 $::grabber(lclCollimation)
}

proc grabber_lcl_channel_trace_callback {args} {

    # Select the appropriate button

    set ::grabber(sendChannelCmd) 0
    set ::grabber(redraw) 0

    if {      $::grabber(lclChannel) == 0} {
	.im.grabberControl.right.chan.chan0 select
    } elseif {$::grabber(lclChannel) == 1} {
	.im.grabberControl.right.chan.chan1 select
    } elseif {$::grabber(lclChannel) == 2} {
	.im.grabberControl.right.chan.chan2 select
    } elseif {$::grabber(lclChannel) == 3} {
	.im.grabberControl.right.chan.chan3 select
    }

}

proc grabber_lcl_ximdir_trace_callback {args} {

    # Select the appropriate button

    set ::grabber(sendXImDirCmd) 0
    set ::grabber(redraw) 0

    if {$::grabber(lclXimdir) == 1} {
	.im.imageControl.xMult.xdirp select
    } elseif {$::grabber(lclXimdir) == -1} {
	.im.imageControl.xMult.xdirn select
    }

}

proc grabber_lcl_yimdir_trace_callback {args} {

    # Select the appropriate button

    set ::grabber(sendYImDirCmd) 0
    set ::grabber(redraw) 0

    if {$::grabber(lclYimdir) == 1} {
	.im.imageControl.yMult.ydirp select
    } elseif {$::grabber(lclYimdir) == -1} {
	.im.imageControl.yMult.ydirn select
    }

}

proc grabber_lcl_peak_trace_callback {args} {

    # Set the centroid

    grabber im_set_centroid $::grabber(lclChannel) $::grabber(lclXpeak) $::grabber(lclYpeak)
    grabber im_redraw
}

proc grabber_cmap_trace_callback {args} {

    # Set the colormap to the value asserted by the local Tcl/C layer

    set ::grabber(redraw) 0
    set ::grabber(menuCmap) $::grabber(cmap)
}

proc grabber_lcl_dkRotSense_trace_callback {args} {

    # Select the appropriate button

    set ::grabber(sendDkRotSenseCmd) 0
    set ::grabber(redraw) 0

    if {$::grabber(lclDkRotSense) == 1} {
	.im.imageControl.dkRot.cw  select
    } elseif {$::grabber(lclDkRotSense) == -1} {
	.im.imageControl.dkRot.ccw select
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
proc change_image_cursor_mode {plot name args} {
  upvar #0 $name input
  upvar #0 $plot p

#
# Set up the new cursor mode.
#
  init_image_${input}_cursor $plot $plot.main.p
}

#-----------------------------------------------------------------------
# Disable the cursor of the image widget.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_image_offset_cursor {plot pg} {

  $pg setcursor line 0.0 0.0 5

  bind $pg <1> "send_image_offset .im %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> {}
  describe_plot_btns $plot {Offset to cursor} {N/A} {N/A}
}
#-----------------------------------------------------------------------
# Disable the cursor of the image widget.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_image_none_cursor {plot pg} {

  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> {}
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> {}
  describe_plot_btns $plot {N/A} {N/A} {N/A}
}
#-----------------------------------------------------------------------
# Disable the cursor of the image widget.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_image_zoom_cursor {plot pg} {
  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "im_start_zoom %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> "im_end_zoom_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Unzoom}
}

#-----------------------------------------------------------------------
# Prepare to define an include box
#-----------------------------------------------------------------------
proc init_image_inc_cursor {plot pg} {
  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "im_start_inc %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> "im_del_box_inc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}
}

#-----------------------------------------------------------------------
# Prepare to define an exclude box
#-----------------------------------------------------------------------
proc init_image_exc_cursor {plot pg} {
  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "im_start_exc %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> "im_del_box_exc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}
}

#-----------------------------------------------------------------------
# Disable the cursor of the image widget.
#
# Input:
#  plot    The image for which we want statistics.
#  pg      The pgplot widget of the specified image.
#-----------------------------------------------------------------------
proc init_image_stat_cursor {plot pg} {
  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "im_start_stat %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> "im_end_stat_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Whole image}
}

#-----------------------------------------------------------------------
# Prepare to fiddle the contrast/brightness of the display
#
# Input:
#  plot    The plot to be fiddled.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_image_fid_cursor {plot pg} {
  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> {}
  bind $pg <B1-Motion> "im_fid_contrast %x %y"
  bind $pg <ButtonRelease-1> "grabber im_redraw"
  bind $pg <2> {}
  bind $pg <3> "im_reset_contrast"

  describe_plot_btns $plot {Hold and drag mouse} {} {Reset}
}

#-----------------------------------------------------------------------
# Draw an arrow head from the center of the rings to the cursor.
#
# Input:
#  canvas      The Tk pathname of the offset dialog.
#-----------------------------------------------------------------------
proc update_image_cursor {w x y} {
  set pg $w.main    ;# The display area of the target
#
# Display the new values.
#
    $w.cursor.x configure -text [format {%.2f} [$pg.p world x $x]]
    $w.cursor.y configure -text [format {%.2f} [$pg.p world y $y]]
}

#-----------------------------------------------------------------------
# Fiddle the contrast and brightness of the image widget
#-----------------------------------------------------------------------
proc im_fid_contrast {x1 y1} {
  
  set plot .im
  set pg $plot.main.p

  set x1 [$pg world x $x1]
  set y1 [$pg world y $y1]

  grabber im_contrast $::grabber(lclChannel) $x1 $y1
}

#-----------------------------------------------------------------------
# Fiddle the contrast and brightness of the image widget
#-----------------------------------------------------------------------
proc im_reset_contrast {} {
  grabber im_reset_contrast $::grabber(lclChannel)
  grabber im_redraw
}

#-----------------------------------------------------------------------
# This is used as a pgplot image-widget cursor callback. It augments the
# cursor in the image window with a line rubber-band anchored at the
# selected cursor position and registers a new callback to receive both
# the current coordinates and coordinates of the end of the slice when
# selected.
#
# Input:
#  wx wy   The X-window coordinates of the position that the user selected
#          with the cursor.
#-----------------------------------------------------------------------
proc im_start_zoom {wx wy} {

  set plot .im
  set pg $plot.main.p
#
# Convert from X coordinates to world coordinates.
#
  set x [$pg world x $wx]
  set y [$pg world y $wy]

  $pg setcursor rect $x $y 5
#
# If the left mouse button is pressed, start a zoom.
#
  bind $pg <1> "im_end_zoom $x $y %x %y"
  bind $pg <2> "im_cancel_zoom"
  bind $pg <3> "im_end_zoom_full"

  describe_plot_btns $plot {Select the other corner of the range} {Cancel} {Unzoom}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback is registered by im_start_zoom
# It receives the start coordinates of a zoom from im_start_zoom and
# the coordinate of the end of the zoom from the callback arguments
# provided by the pgplot widget.
#
# Input:
#  x1 y1          The coordinate of the start of the slice in the image
#                 window. These values were supplied when the callback
#                 was registered by start_slice.
#  wx2 wy2        The X-window coordinate of the end of the slice.
#-----------------------------------------------------------------------
proc im_end_zoom {x1 y1 wx2 wy2} {
  set plot .im
  set pg $plot.main.p
  
  set wx2 [$pg world x $wx2]
  set wy2 [$pg world y $wy2]

  $pg setcursor norm 0.0 0.0 1

  grabber im_setrange $::grabber(lclChannel) $x1 $y1 $wx2 $wy2
  image_redraw
#
# Reset the key bindings to start the next zoom.
#
  bind $pg <1> "im_start_zoom %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_end_zoom_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Unzoom}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback is registered by im_start_zoom
# It receives the start coordinates of a zoom from im_start_zoom and
# the coordinate of the end of the zoom from the callback arguments
# provided by the pgplot widget.
#
# Input:
#  x1 y1          The coordinate of the start of the slice in the image
#                 window. These values were supplied when the callback
#                 was registered by start_slice.
#  wx2 wy2        The X-window coordinate of the end of the slice.
#-----------------------------------------------------------------------
proc im_end_zoom_full {} {
  set plot .im
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

  grabber im_setrange_full $::grabber(lclChannel)
  image_redraw
#
# Reset the key bindings to start the next zoom.
#
  bind $pg <1> "im_start_zoom %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_end_zoom_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Unzoom}
}
#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback cancels a zoom registered by 
# im_start_zoom.  It resets the cursor to normal and sets up ket bindings
# for the next zoom.
#
#-----------------------------------------------------------------------
proc im_cancel_zoom {} {
  set plot .im
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next zoom.
#

  bind $pg <1> "im_start_zoom %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_end_zoom_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Unzoom}
}

#-----------------------------------------------------------------------
# This is used as a pgplot image-widget cursor callback. It augments the
# cursor in the image window with a line rubber-band anchored at the
# selected cursor position and registers a new callback to receive both
# the current coordinates and coordinates of the end of the slice when
# selected.
#
# Input:
#  wx wy   The X-window coordinates of the position that the user selected
#          with the cursor.
#-----------------------------------------------------------------------
proc im_start_inc {wx wy} {

  set plot .im
  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#

  set x [$pg world x $wx]
  set y [$pg world y $wy]

  $pg setcursor rect $x $y 10
#
# If the left mouse button is pressed, start a stat.
#
  bind $pg <1> "im_end_inc $x $y %x %y"
  bind $pg <2> "im_cancel_inc"
  bind $pg <3> {}

  describe_plot_btns $plot {Select the other corner of the range} {Cancel} {N/A}
}

#-----------------------------------------------------------------------
# This is used as a pgplot image-widget cursor callback. It augments the
# cursor in the image window with a line rubber-band anchored at the
# selected cursor position and registers a new callback to receive both
# the current coordinates and coordinates of the end of the slice when
# selected.
#
# Input:
#  wx wy   The X-window coordinates of the position that the user selected
#          with the cursor.
#-----------------------------------------------------------------------
proc im_start_exc {wx wy} {

  set plot .im
  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#

  set x [$pg world x $wx]
  set y [$pg world y $wy]

  $pg setcursor rect $x $y 2
#
# If the left mouse button is pressed, start a stat.
#
  bind $pg <1> "im_end_exc $x $y %x %y"
  bind $pg <2> "im_cancel_exc"
  bind $pg <3> {}

  describe_plot_btns $plot {Select the other corner of the range} {Cancel} {N/A}
}

#-----------------------------------------------------------------------
# Called when the user has selected the second vertex of a exclude box
#-----------------------------------------------------------------------
proc im_end_inc {x1 y1 x2 y2} {
  set plot .im
  set pg $plot.main.p
  
  set x2 [$pg world x $x2]
  set y2 [$pg world y $y2]

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_inc %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_del_box_inc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}

# 
# And send the search box to the control system
#
  set minCoord [grabber worldToPixel $::grabber(lclChannel) $x1 $y1]
  set maxCoord [grabber worldToPixel $::grabber(lclChannel) $x2 $y2]

  set ixmn [lindex $minCoord 0]
  set iymn [lindex $minCoord 1]

  set ixmx [lindex $maxCoord 0]
  set iymx [lindex $maxCoord 1]

# Just send this command to the control system.  On receipt, the
# control system will send it back to us, and we will implement it
# then, in rmt_inc_trace_callback

  control send [concat "addSearchBox $ixmn, $iymn, $ixmx, $iymx, true, chan=" $::grabber(lclChannel)]
}

#-----------------------------------------------------------------------
# Called when the user has selected the second vertex of a exclude box
#-----------------------------------------------------------------------
proc im_end_exc {x1 y1 x2 y2} {
  set plot .im
  set pg $plot.main.p
  
  set x2 [$pg world x $x2]
  set y2 [$pg world y $y2]

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_exc %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_del_box_exc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}

# 
# And send the search box to the control system
#
  set minCoord [grabber worldToPixel $::grabber(lclChannel) $x1 $y1]
  set maxCoord [grabber worldToPixel $::grabber(lclChannel) $x2 $y2]

  set ixmn [lindex $minCoord 0]
  set iymn [lindex $minCoord 1]

  set ixmx [lindex $maxCoord 0]
  set iymx [lindex $maxCoord 1]

# Just send this command to the control system.  On receipt, the
# control system will send it back to us, and we will implement it
# then, in rmt_inc_trace_callback

  control send [concat "addSearchBox $ixmn, $iymn, $ixmx, $iymx, false, chan=" $::grabber(lclChannel)]
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback cancels a box definition begun by 
# im_start_inc
#-----------------------------------------------------------------------
proc im_cancel_inc {} {
  set plot .im
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_inc %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_del_box_inc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback cancels a box definition begun by 
# im_start_exc
#-----------------------------------------------------------------------
proc im_cancel_exc {} {
  set plot .im
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_exc %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_del_box_exc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}
}

#-----------------------------------------------------------------------
# This is used as a pgplot image-widget cursor callback. It augments the
# cursor in the image window with a line rubber-band anchored at the
# selected cursor position and registers a new callback to receive both
# the current coordinates and coordinates of the end of the slice when
# selected.
#
# Input:
#  wx wy   The X-window coordinates of the position that the user selected
#          with the cursor.
#-----------------------------------------------------------------------
proc im_del_box_inc {wx wy} {

  set plot .im
  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#

  set x [$pg world x $wx]
  set y [$pg world y $wy]

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_inc %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_del_box_inc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}

  set coord [grabber worldToPixel $::grabber(lclChannel) $x $y]

  set ix [lindex $coord 0]
  set iy [lindex $coord 1]

  control send [concat "remSearchBox x=$ix, y=$iy, chan=" $::grabber(lclChannel)]
}

#-----------------------------------------------------------------------
# This is used as a pgplot image-widget cursor callback. It augments the
# cursor in the image window with a line rubber-band anchored at the
# selected cursor position and registers a new callback to receive both
# the current coordinates and coordinates of the end of the slice when
# selected.
#
# Input:
#  wx wy   The X-window coordinates of the position that the user selected
#          with the cursor.
#-----------------------------------------------------------------------
proc im_del_box_exc {wx wy} {

  set plot .im
  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#

  set x [$pg world x $wx]
  set y [$pg world y $wy]

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_exc %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_del_box_exc %x %y"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Delete the nearest box}

  set coord [grabber worldToPixel $::grabber(lclChannel) $x $y]

  set ix [lindex $coord 0]
  set iy [lindex $coord 1]

  control send [concat "remSearchBox x=$ix, y=$iy, chan=" $::grabber(lclChannel)]
}

#-----------------------------------------------------------------------
# This is used as a pgplot image-widget cursor callback. It augments the
# cursor in the image window with a line rubber-band anchored at the
# selected cursor position and registers a new callback to receive both
# the current coordinates and coordinates of the end of the slice when
# selected.
#
# Input:
#  wx wy   The X-window coordinates of the position that the user selected
#          with the cursor.
#-----------------------------------------------------------------------
proc im_start_stat {wx wy} {

  set plot .im
  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#

  set x [$pg world x $wx]
  set y [$pg world y $wy]

  $pg setcursor rect $x $y 5
#
# If the left mouse button is pressed, start a stat.
#
  bind $pg <1> "im_end_stat $x $y %x %y"
  bind $pg <2> "im_cancel_stat"
  bind $pg <3> "im_end_stat_full"

  describe_plot_btns $plot {Select the other corner of the range} {Cancel} {Whole image}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback is registered by im_start_stat
# It receives the start coordinates of a stat from im_start_stat and
# the coordinate of the end of the stat from the callback arguments
# provided by the pgplot widget.
#
# Input:
#  x1 y1          The coordinate of the start of the slice in the image
#                 window. These values were supplied when the callback
#                 was registered by start_slice.
#  wx2 wy2        The X-window coordinate of the end of the slice.
#-----------------------------------------------------------------------
proc im_end_stat {x1 y1 wx2 wy2} {
  set plot .im
  set pg $plot.main.p
  
  set wx2 [$pg world x $wx2]
  set wy2 [$pg world y $wy2]

  $pg setcursor norm 0.0 0.0 1

  set stats [grabber im_stat $::grabber(lclChannel) $x1 $y1 $wx2 $wy2]

#
# Decompose the result.
#

  set min [format %.6g [lindex $stats 0]]
  set max [format %.6g [lindex $stats 1]]
  set mean [format %.6g [lindex $stats 2]]
  set rms [format %.6g [lindex $stats 3]]
  set npoint [lindex $stats 4]

  if {$npoint == 1} {
    show_plot_message $plot "$npoint pixel has: min=$min  max=$max  mean=$mean  rms=$rms"
  } else {
    show_plot_message $plot "$npoint pixels have: min=$min  max=$max  mean=$mean  rms=$rms"
  }

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_stat %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_end_stat_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Whole image}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback is registered by im_start_stat
# It receives the start coordinates of a stat from im_start_stat and
# the coordinate of the end of the stat from the callback arguments
# provided by the pgplot widget.
#
# Input:
#  x1 y1          The coordinate of the start of the slice in the image
#                 window. These values were supplied when the callback
#                 was registered by start_slice.
#  wx2 wy2        The X-window coordinate of the end of the slice.
#-----------------------------------------------------------------------
proc im_end_stat_full {} {
  set plot .im
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

  set stats [grabber im_stat $::grabber(lclChannel) 0 0 0 0]
#
# Decompose the result.
#
  set min [format %.6g [lindex $stats 0]]
  set max [format %.6g [lindex $stats 1]]
  set mean [format %.6g [lindex $stats 2]]
  set rms [format %.6g [lindex $stats 3]]
  set npoint [lindex $stats 4]
  show_plot_message $plot "$npoint pixels have  min=$min  max=$max  mean=$mean  rms=$rms"

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_stat %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_end_stat_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Whole image}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback cancels a stat begun by 
# im_start_stat.
#-----------------------------------------------------------------------
proc im_cancel_stat {} {
  set plot .im
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next stat.
#

  bind $pg <1> "im_start_stat %x %y"
  bind $pg <2> {}
  bind $pg <3> "im_end_stat_full"

  describe_plot_btns $plot {Select one corner of the range} {N/A} {Whole image}
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
proc create_grabber_dialog {w} {

  create_config_dialog $w {Connect Host}

#
# Create an entry widget for specifying the name of the control computer.
# Have the host name reflected in the $::viewer(host) variable.
#

  labeled_entry $w.host {Control Computer} $::viewer(host) -width 20 \
      -textvariable ::viewer(host)

#
# Layout the widgets from top to bottom.
#

  pack $w.host -side top -anchor nw -expand true -fill x

#
# Bind the ok button to load the specified file.
#

  $w.bot.apply configure -command "connect_imhost; wm withdraw $w"
}

#------------------------------------------------------------
# Create a top-level widget for pointing control
#------------------------------------------------------------
proc create_pointing_control_widget {w} {

  toplevel    $w -class PointingControl
  wm title    $w "Pointing Controls"
  wm withdraw $w
  wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"

  set top [frame $w.top]
  set bot [frame $w.bot]

  pack $top -side top -anchor n -fill both -expand true
  pack $bot -side top -anchor s -fill both -expand true

  set xy  [frame $top.left]
  set but [frame $top.right]

  pack $xy  -side left -anchor w -fill both -expand true
  pack $but -side left -anchor e -fill both -expand true

#------------------------------------------------------------ 
# Create an entry area for specifying the x coordinate to 
# move to.
#------------------------------------------------------------

  set x [frame $xy.x]
  label $x.title -anchor w -text {Move to x (arcsec): }
  entry $x.sec -width 8 -takefocus 0 -textvariable ::$x.sec
  $x.sec insert end 0.0
  pack $x.title $x.sec -side left
  pack $x -side top -anchor w

#------------------------------------------------------------
# Create an entry area for specifying the y coordinate 
# to move to.
#------------------------------------------------------------

  set y [frame $xy.y]
  label $y.title -anchor w -text {Move to y (arcsec): }
  entry $y.sec -width 8 -takefocus 0 -textvariable ::$y.sec
  $y.sec insert end 0.0
  pack $y.title $y.sec -side left
  pack $y -side top -anchor w

#
# Create a button for the user to press to find the peak brightness of the 
# image 
#

  button $but.cent -bg [$w cget -bg] -width 10 -text "Find Peak" -command "find_centroid $w"
  pack $but.cent -side top -anchor w

#
# Create a button for the user to press to send the entry widget offsets to 
# control program.
#

  button $but.move -bg [$w cget -bg] -width 10 -text "Move" -command "send_centroid_offsets $w"
  pack $but.move -side top -anchor w

#
# Create a button for the user to press to skip this star.
#

  button $but.skip -bg [$w cget -bg] -width 10 -text "Skip this star" -command skip_offset_star  
  pack $but.skip -side top -anchor w

#
# Create a button for the user to press to center the star
#

  button $but.center -bg red -width 10 -text "Center" -command grab_center
  pack $but.center -side top -anchor w

#
# Create a button for the user to press to get the next frame from the grabber
#

  button $but.grab -bg green -width 10 -text "Next Frame" -command "grab_next"
  pack $but.grab -side top -anchor w

# Create a button for the user to press when the star is centered in

  button $bot.mark -bg [$w cget -bg] -text "Press here when the star is centered in the image" -command "send_offset_mark $bot"

  pack $bot.mark -anchor w -fill x -side top
}

#------------------------------------------------------------
# Create a top-level widget for control of the grabber
#------------------------------------------------------------
proc create_grabber_control_widget {w} {

  toplevel $w -class GrabberControl
  wm title $w "Frame Grabber Control"
  wm withdraw $w
  wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"

#------------------------------------------------------------
# Create an entry area for specifying the flatfielding type
#------------------------------------------------------------

  set left  [frame $w.left]
  set right [frame $w.right]

  pack $left $right -side left -fill both -expand true

  set flatField [frame $left.flatField -relief ridge -borderwidth 2]
  label $flatField.lab -width 26 -anchor w -text {Type of flatfielding: }

  #------------------------------------------------------------
  # Create radio buttons for toggling flatfield type
  #------------------------------------------------------------

  radiobutton $flatField.none  -variable $flatField.type -value 0 -text {none}
  radiobutton $flatField.row   -variable $flatField.type -value 1 -text {row}
  radiobutton $flatField.image -variable $flatField.type -value 2 -text {image}

  # Create a button for taking flatfield images

  button $flatField.take -width 10 -bg green -text "Take Flatfield" -command takeFlatfield

  pack $flatField.lab $flatField.none $flatField.row $flatField.image $flatField.take -side left -anchor w -fill x -expand true

  global $flatField.type
  trace variable $flatField.type w "select_flatfieldType $flatField.type"

# 
# Create a scale for specifying the number of images to combine
#

  set nCombine [frame $left.nCombine -bg skyblue -relief ridge -borderwidth 2]

  scale $nCombine.ncombine -from 1 -to 30 -length 300\
	  -orient horizontal -label "Number of images to integrate:" \
	  -troughcolor skyblue \
	  -tickinterval 5 -showvalue true -takefocus 1 

  bind $nCombine.ncombine <ButtonRelease-1> "setCombine $nCombine.ncombine"
  
  $nCombine.ncombine set 1

  pack $nCombine.ncombine   -side left -anchor w -fill x    -expand true

  pack $flatField $nCombine -side top  -anchor w -fill both -expand true

#
# Create radio buttons for toggling channels
#

  set chan [frame $right.chan -relief ridge -borderwidth 2]

  radiobutton $chan.chan0 -variable $chan.channel -value 0 -text $::grabber(chan0Text) -justify left
  radiobutton $chan.chan1 -variable $chan.channel -value 1 -text {Channel 1} -justify left
  radiobutton $chan.chan2 -variable $chan.channel -value 2 -text {Channel 2} -justify left
  radiobutton $chan.chan3 -variable $chan.channel -value 3 -text {Channel 3} -justify left

  global $chan.channel
  trace variable $chan.channel w "select_fg_channel $chan.channel"

  $chan.chan0 select

  pack $chan.chan0 $chan.chan1 $chan.chan2 $chan.chan3 -side top 

  pack $chan -anchor w -fill both -expand true

  return $w
}

#------------------------------------------------------------
# Create a top-level widget for control of the image 
# characteristics
#
#------------------------------------------------------------
proc create_image_control_widget {w} {

  toplevel    $w -class ImageControl
  wm title    $w "Image Control"
  wm withdraw $w
  wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"

#------------------------------------------------------------
# Create an entry area for specifying the grid interval,
# in arcseconds.
#------------------------------------------------------------

  set gridInt [frame $w.gridInt]

  label $gridInt.lab -width 26 -anchor w -text {Grid interval (arcsec): }
  entry $gridInt.ent -width 7 -takefocus 0 -textvariable ::$gridInt.ent

  pack $gridInt.lab $gridInt.ent -side left -anchor w

  $gridInt.ent insert end 30

# Whenever the user presses return in this entry widget, update the
# grid interval

  bind $gridInt.ent  <Return> "change_image_step $w"

#------------------------------------------------------------
# Create an entry area for specifying the image width, 
# in arcminutes.
#------------------------------------------------------------

  set imWidth [frame $w.imWidth]

  label $imWidth.lab -bg skyblue -width 26 -anchor w -text {Image width (arcmin): }
  entry $imWidth.ent -width 7  -takefocus 0 -textvariable ::$imWidth.ent

  pack $imWidth.lab $imWidth.ent -side left -anchor w

  $imWidth.ent insert end $::grabber(lclFov)

# Whenever the user presses return in this entry widget, update the
# image width

  bind $imWidth.ent  <Return> "change_image_fov $w 1"

#------------------------------------------------------------
# Create an entry area for specifying the aspect ratio (y/x)
#------------------------------------------------------------

  set aspect [frame $w.aspect]

  label $aspect.lab -width 26 -anchor w -text {Aspect ratio (y/x): }
  entry $aspect.ent -width 7  -takefocus 0 -textvariable ::$aspect.ent

  pack $aspect.lab $aspect.ent -side left -anchor w

  $aspect.ent insert end 0.8

# Whenever the user presses return in this entry widget, update the
# aspect ratio

  bind $aspect.ent   <Return> "change_image_aspect $w 1"

#------------------------------------------------------------
# Create an entry area for specifying the rotation angle
#------------------------------------------------------------

  set rotAngle [frame $w.rotAngle]

  label $rotAngle.lab -bg skyblue -width 26 -anchor w -text {Rotation angle (degrees, cw): }
  entry $rotAngle.ent -width 7  -takefocus 0 -textvariable ::$rotAngle.ent

  pack $rotAngle.lab $rotAngle.ent -side left -anchor w

  $rotAngle.ent insert end 0.0

# Whenever the user presses return in this entry widget, update the
# rotation angle

  bind $rotAngle.ent <Return> "change_image_collimation $w 1"

#------------------------------------------------------------
# Create an label for specifying the x direction
#------------------------------------------------------------

  set xMult [frame $w.xMult]
  label $xMult.lab -width 26 -anchor w -text {Sense of the x-axis: }

  # Create radio buttons for toggling x-direction

  radiobutton $xMult.xdirp -variable $xMult.xdir -value  1 -text {upright}
  radiobutton $xMult.xdirn -variable $xMult.xdir -value -1 -text {inverted}

  pack $xMult.lab $xMult.xdirp $xMult.xdirn -side left -anchor w

  global $xMult.xdir
  trace variable $xMult.xdir w "select_ximdir $xMult.xdir"

#------------------------------------------------------------
# Create an label for specifying the y direction
#------------------------------------------------------------

  set yMult [frame $w.yMult]

  label $yMult.lab -bg skyblue -width 26 -anchor w -text {Sense of the y-axis: }

  # Create radio buttons for toggling y-direction

  radiobutton $yMult.ydirp -variable $yMult.ydir -value  1 -text {upright}
  radiobutton $yMult.ydirn -variable $yMult.ydir -value -1 -text {inverted}

  pack $yMult.lab $yMult.ydirp $yMult.ydirn -side left -anchor w

  global $yMult.ydir
  trace variable $yMult.ydir w "select_yimdir $yMult.ydir"

#------------------------------------------------------------
# Create an entry area for specifying the deck angle sense
#------------------------------------------------------------

  set dkRot [frame $w.dkRot]

  label $dkRot.lab -width 26 -anchor w -text {Sense of the deck rotation: }

  # Create radio buttons for toggling deck rotation sense

  radiobutton $dkRot.cw  -variable $dkRot.sense -value  1 -text {CW      }
  radiobutton $dkRot.ccw -variable $dkRot.sense -value -1 -text {CCW}

  pack $dkRot.lab $dkRot.cw $dkRot.ccw -side left -anchor w

  global $dkRot.sense
  trace variable $dkRot.sense w "select_dkRotSense $dkRot.sense"

  pack $gridInt $imWidth $aspect $rotAngle $xMult $yMult -side top -anchor w

  return $w
}

#-----------------------------------------------------------------------
# Reveal the controls menus of the frame grabber widget
#-----------------------------------------------------------------------
proc reveal_controls {w} {

# First get the location of the frame grabber window

    getGeometry $w                 xw   yw   x   y

# Set the x location of all windows to be to the right of the main widget

    set x_all [expr $x + $xw + 10]

# Now deiconify the imageControl widget, get its width, and set it to
# be at the level of the main widget

    wm deiconify $w.imageControl;  reveal $w.imageControl    
    getGeometry $w.imageControl    xwIm ywIm xIm yIm
    set y_im $y
    set g_im +$x_all+$y_im
    wm geometry $w.imageControl    $g_im

# Now deiconify the grabberControl widget, get its width, and set it to
# be at just below the imageControl widget

    wm deiconify $w.grabberControl;  reveal $w.grabberControl;  
    getGeometry $w.grabberControl  xwGb ywGb xGb yGb
    set y_gb [expr $y    + $ywIm + 30]
    set g_gb +$x_all+$y_gb
    wm geometry $w.grabberControl  $g_gb

# Now deiconify the pointingControl widget, get its width, and set it to
# be at just below the grabberControl widget

    wm deiconify $w.pointingControl; reveal $w.pointingControl; 
    getGeometry $w.pointingControl xwPt ywPt xPt yPt
    set y_pt [expr $y_gb  + $ywGb + 30]
    set g_pt +$x_all+$y_pt
    wm geometry $w.pointingControl $g_pt
}

#-----------------------------------------------------------------------
# Procedure to get the current geometry of a window
#-----------------------------------------------------------------------
proc getGeometry {w xwName ywName xName yName} {

    upvar 1 $xwName xw
    upvar 1 $ywName yw
    upvar 1 $xName  x
    upvar 1 $yName  y

    set g [wm geometry $w]
    scan $g "%dx%d+%d+%d" xw yw x y
}

#-----------------------------------------------------------------------
# Take a flatfield image 
#-----------------------------------------------------------------------
proc takeFlatfield {} {
    control send "takeFlatfield chan=$::grabber(lclChannel)"
}
