#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#
# Input:
#  w          The path name to assign the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_starplot_widget {w} {

  set plot $w
#
# Create a toplevel frame to contain the plot and its associated
# configuration and informational widgets.
#
  toplevel $plot -class StarPlot
  wm title $plot "Star Plot"
  wm iconname $plot "Image"
  wm withdraw $plot
  wm protocol $plot WM_DELETE_WINDOW "quit_starplot $w"

#
# Create the main components of the widget.
#
  create_starplot_main_widget $plot.main

#
# Create a menubar with a file menu that contains a quit button.
#
  create_starplot_menubar $plot

#
# Create a configuration widget
#
  create_starplot_configuration_widget $plot

#
# Create a widget for display of information about a source
#
  create_starplot_source_widget $plot.source

#
# Create a message area for subsequent display below the buttons.
# This will only be visible when there is an error message to be
# displayed.
#
  message $plot.msg -justify center -fg red -bg black -width 15c

# Arrange the vertical panes of the widget.

  pack $plot.mbar -side top -fill x
  pack $plot.main -side top -fill both -expand true
  pack $plot.config -side top -fill x

# Finally, open the image

  open_starplot $plot  

  init_starplot_identify_cursor $plot

# Add return the image widget.

  .starplot.config.site.but.ovro invoke

  starplot_command change_sunDist 3  
  starplot_command change_moonDist 3  

  return $plot
}

#-----------------------------------------------------------------------
# Delete a starplot widget
#-----------------------------------------------------------------------
proc quit_starplot {w} {

# Withdraw the image widgets

  wm withdraw $w
  wm withdraw $w.nedObj
  wm withdraw $w.nedCat
  wm withdraw $w.source
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
proc create_starplot_main_widget {w} {
#
# Frame the plotarea.
#
  frame $w -width 15c -height 10c
#
# Create an area for mouse-button descriptions.
#
  create_starplot_button_show_widget $w.b
#
# Frame the area that will contain the PGPLOT widget.
#
# If we don't specify the -maxcolors keyword in the pgplot command; this will
# default to 100.
#
  pgplot $w.p -width 15c -height 15c -bg black -relief sunken -bd 2

  bind $w.p <1> "mark_star $w.p %x %y"

  pack $w.b -side top  -fill x
  pack $w.p -side left -fill both -expand true

  return $w
}

proc mark_star {w wx wy} {

    set wx [$w.main.p world x $wx]
    set wy [$w.main.p world y $wy]

    global ::sourceInfo
    starplot_command mark ::sourceInfo $wx $wy

# Finally, deiconify the source widget

    wm deiconify $w.source
    focus $w.source
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
proc create_starplot_configuration_widget {w} {

# Frame the configuration area

  frame $w.config

# Create the frame for configuring the catalog

  create_starplot_cat_config_widget $w.config.cat

# Create the frame for configuring the calibrator list

  create_starplot_cal_config_widget $w.config.cal

# Create the frame for configuring the catalog

  create_starplot_survey_config_widget $w.config.survey

# Create a labeled entry widget for changing the elevation limit

  create_starplot_limit_config_widget $w.config.lim

# Create the frame for configuring the site

  create_starplot_site_config_widget $w.config.site

# Create the frame for configuring the time

  create_starplot_time_config_widget $w.config.time

  pack $w.config.cat $w.config.cal $w.config.survey $w.config.lim \
      $w.config.site $w.config.time -side top -fill x

# Create the top-level frame for configuring a NED search

  create_ned_object_widget  $w.nedObj
  create_ned_catalog_widget $w.nedCat

  return $w
}

#------------------------------------------------------------
# Create the NED object configuration dialog
#------------------------------------------------------------

proc create_starplot_source_widget {w} {
    toplevel $w -class {Source Information}
    wm title $w "Source Information"
    wm withdraw $w
    wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"

    text $w.t -fg TextLabelColor -bg BackgroundWidgetColor -height 12 -width 50
    pack $w.t

    global ::sourceInfo
    trace variable ::sourceInfo w "insertSourceInfo $w.t ::sourceInfo"
}

proc insertSourceInfo {w srcInfo args} {
    upvar #0 $srcInfo info
    $w delete 1.0 end
    $w insert 1.0 $info
}

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#-----------------------------------------------------------------------

proc create_starplot_cat_config_widget {w} {
#
# Frame the configuration area
#
  frame $w -relief sunken -bd 2 -width 15c 

#
# Create a labeled entry widget $w.f.e.
#
  set file [starplot_labeled_entry $w.file {Load Catalog} {} -width 40]

  $w.file.e insert 0 $::env(GCP_DIR)/control/ephem/
  $w.file.e xview end
  $w.file.e icursor end

#
# Create a button to clear the catalog
#

  button $w.add     -text {Add}     -fg TextLabelColor -command "starplot_command add_catalog \[$w.file.e get\]" -width 5
  button $w.replace -text {Replace} -fg TextLabelColor -command "starplot_command replace_catalog \[$w.file.e get\]" -width 5

  bind $w.file.e <Return> "starplot_command add_catalog \[$w.file.e get\]"

  pack $w.file $w.add $w.replace -side left

  return $w
}

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#-----------------------------------------------------------------------

proc create_starplot_cal_config_widget {w} {
#
# Frame the configuration area
#
  frame $w -relief sunken -bd 2 -width 15c 

#
# Create a labeled entry widget $w.f.e.
#
  set cal [starplot_labeled_entry $w.file {Load Cal List} {} -width 40]

  $w.file.e insert 0 ""
  $w.file.e xview end
  $w.file.e icursor end

#
# Create a button to clear the catalog
#

  button $w.add     -text {Add}     -fg TextLabelColor -command "starplot_command add_cal \[$w.file.e get\]" -width 5
  button $w.replace -text {Replace} -fg TextLabelColor -command "starplot_command replace_cal \[$w.file.e get\]" -width 5

  bind $w.file.e <Return> "starplot_command add_cal \[$w.file.e get\]"

  pack $w.file $w.add $w.replace -side left

  return $w
}

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#-----------------------------------------------------------------------

proc create_starplot_survey_config_widget {w} {
#
# Frame the configuration area
#
  frame $w -relief sunken -bd 2 -width 15c 

  radiobutton $w.nvss  -variable $w.surveyChoice -value nvss  -text {NVSS} -fg TextLabelColor
  radiobutton $w.first -variable $w.surveyChoice -value first -text {FIRST} -fg TextLabelColor

  global $w.surveyChoice
  trace variable $w.surveyChoice w "print_choice $w.surveyChoice"

  $w.nvss select

  label $w.fminlab -text {Min Flux (Jy)} -width 15 -fg TextLabelColor
  entry $w.fminent -width 5
  label $w.fmaxlab -text {Max Flux (Jy)} -width 15 -fg TextLabelColor
  entry $w.fmaxent -width 5

  $w.fminent insert 0 1
  $w.fmaxent insert 0 1000

  button $w.add     -text {Add}     -fg TextLabelColor -command "add_survey $w"     -width 5
  button $w.replace -text {Replace} -fg TextLabelColor -command "replace_survey $w" -width 5

  pack $w.nvss $w.first $w.fminlab $w.fminent $w.fmaxlab $w.fmaxent $w.add $w.replace -side left

  return $w
}

proc print_choice {config choice args} {
    upvar #0 $choice ch
}

proc add_survey {w} {
  upvar #0 $w.surveyChoice ch

  if {[catch {
      starplot_command add_survey $ch [$w.fminent get] [$w.fmaxent get]
  } result]} {
      puts stderr $result
      starplot_error $result
  }

}

proc replace_survey {w} {
  upvar #0 $w.surveyChoice ch
  starplot_command replace_survey $ch [$w.fminent get] [$w.fmaxent get]
}

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#-----------------------------------------------------------------------

proc create_starplot_site_config_widget {w} {
#
# Frame the configuration area
#
  frame $w -relief sunken -bd 2 -width 15c

  frame $w.lab -width 5c
  frame $w.ent -width 5c
  frame $w.but -width 5c

#
# Create a labeled entry widget for changing the latitude
#
  label $w.lab.lat -text {Latitude}  -width 10 -justify left -fg TextLabelColor
  label $w.lab.lng -text {Longitude} -width 10 -justify left -fg TextLabelColor

  pack $w.lab.lat $w.lab.lng -side top

#
# Create a frame of entries
#
  entry $w.ent.lat -width 12 -justify right
  entry $w.ent.lng -width 12 -justify right

  bind $w.ent.lat <Return> "starplot_command change_latitude  \[$w.ent.lat get\]"
  bind $w.ent.lng <Return> "starplot_command change_longitude \[$w.ent.lng get\]"

  pack $w.ent.lat $w.ent.lng -side top

#
# Create a frame of buttons
#

  button $w.but.ovro  -text {OVRO}  -fg TextLabelColor -command "starplot_change_site ovro $w"  -width 5
  button $w.but.pole  -text {Pole}  -fg TextLabelColor -command "starplot_change_site pole $w"  -width 5
  button $w.but.sa    -text {SA} -fg TextLabelColor -command "starplot_change_site sa $w" -width 5

  pack $w.but.ovro $w.but.pole $w.but.sa -side left

#
# Pack all three frames
#

  pack $w.lab $w.ent $w.but -side left -fill x
  
  return $w
}

#-----------------------------------------------------------------------
# Create the area of the image widget that contains the PGPLOT widget
# and associated controls.
#-----------------------------------------------------------------------

proc create_starplot_time_config_widget {w} {
#
# Frame the configuration area
#
  frame $w -relief sunken -bd 2 -width 15c

  frame $w.lab -width 5c
  frame $w.ent -width 5c
  frame $w.but -width 5c

# Create a labeled entry widget for changing the LST

  label $w.lab.lst -text {LST}       -width 10 -justify left -fg TextLabelColor
  pack $w.lab.lst -side top

# Create a labeled entry widget for changing the MJD

  label $w.lab.mjd -text {MJD}       -width 10 -justify left -fg TextLabelColor
  pack $w.lab.mjd -side top

# Create a frame of entries

  entry $w.ent.lst -width 10
  bind $w.ent.lst <Return> "starplot_command change_lst \[$w.ent.lst get\];starplot_command redraw"
  pack $w.ent.lst -side top

  entry $w.ent.mjd -width 10
  bind $w.ent.mjd <Return> "starplot_command change_mjd \[$w.ent.mjd get\];starplot_command redraw"
  pack $w.ent.mjd -side top

# Create a frame of buttons

  button $w.but.lst   -text {Now}   -fg TextLabelColor -command "starplot_clear_lst $w" -width 5
  button $w.but.track -text {Track} -fg TextLabelColor -command "starplot_track     $w" -width 5
  pack $w.but.lst $w.but.track -side left

# Pack all three frames

  pack $w.lab $w.ent $w.but -side left -fill x
  
  return $w
}

proc starplot_clear_lst {w} {
  global ::lst
  starplot_command clear_lst
  starplot_command redraw
  $w.ent.lst delete 0 end
  $w.ent.mjd delete 0 end
}

proc starplot_track {w} {
  global ::lst
  starplot_command clear_lst
  starplot_command track
  $w.ent.lst delete 0 end
  $w.ent.mjd delete 0 end
}

proc starplot_clear_mjd {w} {
  global ::lst
  starplot_command clear_lst
  starplot_command redraw
  $w.ent.lst delete 0 end
  $w.ent.mjd delete 0 end
}

#-----------------------------------------------------------------------
# Create the area of the starplot widget that specifies limits
#-----------------------------------------------------------------------

proc create_starplot_limit_config_widget {w} {
#
# Frame the configuration area
#
  frame $w -relief sunken -bd 2 -width 30c

  frame $w.lab1 -width 5c
  frame $w.ent1 -width 5c

#
# Create a labeled entry widget for changing the display limits
#
  label $w.lab1.mag  -text {Magnitude limit} -width 15 -justify left -fg TextLabelColor
  label $w.lab1.el   -text {Elevation limit} -width 15 -justify left -fg TextLabelColor
  label $w.lab1.flux -text {Flux limit (Jy)} -width 15 -justify left -fg TextLabelColor

  entry $w.ent1.mag  -width 10
  entry $w.ent1.el   -width 10
  entry $w.ent1.flux -width 10

  $w.ent1.el insert 0 40

  bind $w.ent1.mag  <Return> "starplot_command change_maglim  \[$w.ent1.mag get\]"
  bind $w.ent1.el   <Return> "starplot_command change_el      \[$w.ent1.el  get\]"
  bind $w.ent1.flux <Return> "starplot_command change_fluxlim \[$w.ent1.flux  get\]"

  pack $w.lab1.mag  $w.lab1.el $w.lab1.flux -side top
  pack $w.ent1.mag  $w.ent1.el $w.ent1.flux -side top

  frame $w.lab2 -width 10c
  frame $w.ent2 -width 5c

#
# Create a labeled entry widget for changing the display limits
#
  label $w.lab2.sun  -text {Display radius from Sun (deg)} -width 30 -justify left -fg TextLabelColor
  label $w.lab2.moon -text {Display radius from Moon (deg)} -width 30 -justify left -fg TextLabelColor

  entry $w.ent2.sun  -width 10
  entry $w.ent2.moon -width 10

  bind $w.ent2.sun  <Return> "starplot_command change_sunDist \[$w.ent2.sun get\]"
  bind $w.ent2.moon <Return> "starplot_command change_moonDist \[$w.ent2.moon get\]"

  pack $w.lab2.sun  $w.lab2.moon -side top
  pack $w.ent2.sun  $w.ent2.moon -side top

  pack $w.lab1 $w.ent1 $w.lab2 $w.ent2 -side left -fill x

  # And set the limits now

  $w.ent2.sun  insert 0 3
  $w.ent2.moon insert 0 3

  return $w
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
proc create_starplot_menubar {w} {
#
# Create a raised frame for the menubar.
#
  frame $w.mbar -relief raised -bd 2 -width 15c

#............................................................
# Create a Find menu
#
  menubutton $w.mbar.find -text {Find} -menu $w.mbar.find.menu -fg TextLabelColor
  set m [menu $w.mbar.find.menu -tearoff 0]
#
# Create its member(s)
#
  $m add command -label {Find szaViewer} -command "reveal ." -foreground TextLabelColor
  $m add command -label {Find Source Info} -command "reveal $w.source" -foreground TextLabelColor

#-----------------------------------------------------------------------
# Create a File menu
#-----------------------------------------------------------------------

  menubutton $w.mbar.file -text {File} -menu $w.mbar.file.menu -fg TextLabelColor
  set m [menu $w.mbar.file.menu -tearoff 0]

  $m add command -label {Quit} -command "quit_starplot $w" -foreground TextLabelColor

#-----------------------------------------------------------------------
# Create a Cursor menu
#-----------------------------------------------------------------------

  menubutton $w.mbar.cursor -text {Cursor} -menu $w.mbar.cursor.menu -fg TextLabelColor
  set m [menu $w.mbar.cursor.menu -tearoff 1]

  $m add command -label {Identify} -underline 0 -command "init_starplot_identify_cursor $w" -foreground TextLabelColor
  $m add command -label {Zoom}     -underline 0 -command "init_starplot_zoom_cursor $w"     -foreground TextLabelColor
  $m add command -label {NED}      -underline 0 -command "init_starplot_ned_cursor $w"      -foreground TextLabelColor

  bind_short_cut $w i "init_starplot_identify_cursor $w"
  bind_short_cut $w z "init_starplot_zoom_cursor $w"
  bind_short_cut $w n "init_starplot_ned_cursor $w"

#-----------------------------------------------------------------------
# Create a Configure menu
#-----------------------------------------------------------------------

  menubutton $w.mbar.conf -text {Configure} -menu $w.mbar.conf.menu -fg TextLabelColor
  set m [menu $w.mbar.conf.menu -tearoff 0]

  image create photo nedlogo -file  "$::env(GCP_DIR)/control/code/unix/viewer_src/nedlogo_g3.jpg"

  $m add command -label {NED} -command "reveal_nedControls $w" -foreground TextLabelColor -image nedlogo
#  $m add command -label {NED} -command "wm deiconify $w.nedObj; reveal $w.nedObj" -foreground TextLabelColor -image nedlogo

  pack $w.mbar.file   -side left
  pack $w.mbar.cursor -side left
  pack $w.mbar.conf   -side left
  pack $w.mbar.find   -side left
}

#------------------------------------------------------------
# Create the NED object configuration dialog
#------------------------------------------------------------

proc create_ned_object_widget {w} {
    toplevel $w -class NED -width 12c
    wm title $w "NED Objects"
    wm withdraw $w
    wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"

# Create a frame for the any/all checkbuttons

    global $w.select

    label $w.l1 -text "Include objects with" -fg TextLabelColor
    radiobutton $w.selAny -variable $w.select -text {ANY of the selected types} -value any
    radiobutton $w.selAll -variable $w.select -text {ALL of the selected types} -value all

    grid configure $w.l1     -row 0 -column 0 -sticky w
    grid configure $w.selAny -row 0 -column 1 -sticky w
    grid configure $w.selAll -row 1 -column 1 -sticky w

    $w.selAny select

# Create separate frames for each type of object

    global ::objRow
    set ::objRow 2

    addNedObjects  $w "Objects"                            none                      true
    addNedObjects  $w "Unclassified Extragalactic Objects" unclassifiedExtragalactic true
    addNedObjects  $w "Classified Extragalactic Objects"   classifiedExtragalactic   true
    addNedObjects  $w "Galaxy Components"                  galactic                  true

# Reconfigure the ALL widget, as it is special:

    set all "all objects"
    global $w.$all
    trace variable $w.$all w "selectAllNedSources $w {$w.$all}"

# Pack all frames together

#    pack $w.obj -side top
}

#------------------------------------------------------------
# Create the NED catalog configuration dialog
#------------------------------------------------------------

proc create_ned_catalog_widget {w} {

    toplevel $w -class NED -width 12c
    wm title $w "NED Catalogs"
    wm withdraw $w
    wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"

# Create separate frames for each type of object

    global $w.select

    label $w.l1 -text "Include objects associated with" -fg TextLabelColor
    radiobutton $w.selAny -variable $w.select -text {ANY of the selected catalogs} -value any
    radiobutton $w.selAll -variable $w.select -text {ALL of the selected catalogs} -value all

    grid configure $w.l1     -row 0 -column 0 -sticky w
    grid configure $w.selAny -row 0 -column 1 -sticky w
    grid configure $w.selAll -row 1 -column 1 -sticky w

    label $w.l2 -text "Exclude objects associated with" -fg TextLabelColor
    radiobutton $w.selNot -variable $w.select -text {ANY of the selected catalogs} -value not

    grid configure $w.l2     -row 2 -column 0 -sticky w
    grid configure $w.selNot -row 2 -column 1 -sticky w

    label $w.l3 -text "Exclude objects with" -fg TextLabelColor
    radiobutton $w.selNon -variable $w.select -text {only one name among the selected catalogs} -value non

    grid configure $w.l3     -row 3 -column 0 -sticky w
    grid configure $w.selNon -row 3 -column 1 -sticky w

    $w.selAny select

# Now add catalog entries starting on row 4

    global ::catRow
    set ::catRow 4

    addNedCatalogs $w "Catalogs" true

# Pack all frames together

#    pack $w -side top
}

#-----------------------------------------------------------------------
# Add object buttons for all objects of the requested type
#-----------------------------------------------------------------------

proc addNedObjects {w objName objType doTrace} {

    global ::objs

    starplot_command listNedObjects $objType ::objs

# First add a button for ALL sources

    addNedObjectSpacer $w [concat "Spacer1 " $objName]

    set len [string length $objName]

    if {[expr $len > 0]} {
	set uname [concat "All " $objName]
    } else {
	set uname [concat "All" $objName]
    }

    set lname [string tolower $uname]

    addNedObject       $w $uname

    addNedObjectSpacer $w [concat "Spacer2 " $objName]

# Reconfigure this particular widget, as it is special:

    if {$doTrace} {
	global $w.$lname
	trace variable $w.$lname w "selectNedSources $w {$w.$lname} $objType"
    }
    
# Now add each object in turn

    foreach obj $::objs {
	addNedObject $w $obj
    }
}

#-----------------------------------------------------------------------
# Add catalog entries for all catalogs
#-----------------------------------------------------------------------

proc addNedCatalogs {w catName doTrace} {

    global ::cats
    starplot_command listNedCatalogs ::cats

# First add a button for ALL sources

    addNedCatalogSpacer $w [concat "Spacer1 " $catName]

    set len [string length $catName]

    if {[expr $len > 0]} {
	set uname [concat "All " $catName]
    } else {
	set uname [concat "All" $catName]
    }

    set lname [string tolower $uname]

    addNedCatalog       $w $uname
    addNedCatalogSpacer $w [concat "Spacer2 " $catName]

# Reconfigure this particular widget, as it is special:

    if {$doTrace} {
	global $w.$lname
	trace variable $w.$lname w "selectNedCatalogs $w {$w.$lname}"
    }
    
# Now add each catalog in turn

    foreach cat $::cats {
	addNedCatalog $w $cat
    }
}

#------------------------------------------------------------
# Respond to the used pressing the All buttons in the source-type
# select menu
#------------------------------------------------------------

proc selectNedCatalogs {w state args} {

    upvar #0 $state st

    starplot_command listNedCatalogs ::cats

    foreach cat $::cats {

	set lname     [string tolower $cat]

	if {$st == "selected"} {
	    $w.$lname select
	} else {
	    $w.$lname deselect
	}

    }
}

#------------------------------------------------------------
# Respond to the used pressing the All buttons in the source-type
# select menu
#------------------------------------------------------------

proc selectNedSources {w state objType args} {

    upvar #0 $state st

    starplot_command listNedObjects $objType ::objs

    foreach obj $::objs {

	set lname     [string tolower $obj]

	set allowName [concat "allow " $lname]
	set incName   [concat "include " $lname]
	set excName   [concat "exclude " $lname]

	if {$st == "allow"} {
	    $w.$allowName select
	} elseif {$st == "exclude"} {
	    $w.$excName select
	} else {
	    $w.$incName select
	}
    }
}

#------------------------------------------------------------
# Respond to the used pressing the All buttons in the source-type
# select menu
#------------------------------------------------------------

proc selectAllNedSources {w state args} {

    upvar #0 $state st

    set ceg "all classified extragalactic objects"
    set ueg "all unclassified extragalactic objects"
    set g   "all galaxy components"

    if {$st == "allow"} {

	set ceg [concat "allow " $ceg]
	set ueg [concat "allow " $ueg]
	set g   [concat "allow " $g]

    } elseif {$st == "exclude"} {

	set ceg [concat "exclude " $ceg]
	set ueg [concat "exclude " $ueg]
	set g   [concat "exclude " $g]

    } else {

	set ceg [concat "include " $ceg]
	set ueg [concat "include " $ueg]
	set g   [concat "include " $g]

    }

    $w.obj.$ceg select
    $w.obj.$ueg select
    $w.obj.$g select
}

#------------------------------------------------------------
# Return a list of all known NED object types
#------------------------------------------------------------

proc listNedObjects {} {
  global ::objs
  starplot_command listNedObjects classifiedExtragalactic ::objs

  foreach obj $objs {
   puts $obj
  }
}

#-----------------------------------------------------------------------
# Associate the passed pgplot widget with the starplot pgplot device
#-----------------------------------------------------------------------
proc open_starplot {w} {

  if [catch {
    set reset [starplot open_image $w.main.p/xtk]
  } result] {
    report_error $result
  }

#
# Arrange for the plot to be redrawn whenever the plot widget is
# resized by the user.
#

  bind $w.main.p <Configure> "starplot_command redraw"
}

#-----------------------------------------------------------------------
# Display an error message the error message area of a given dialog.
#
# Input:
#  message       The error message to be displayed, or {} to clear and
#                remove the error-message area.
#-----------------------------------------------------------------------
proc starplot_error {message} {

    puts $message

  if { [string length $message] > 0 } {
    .starplot.msg configure -text $message
    pack .starplot.msg -after .starplot.config -expand true -fill x
  } else {
    pack forget .starplot.msg
    .starplot.msg configure -text {}
  }
}

#-----------------------------------------------------------------------
# Wrapper around any command we will send to the Starplot interface
#-----------------------------------------------------------------------
proc starplot_command {x args} {

# Clear any legacy error messages from the last command we issued

    pack forget .starplot.msg

    if {[catch {
	
	switch [llength $args] {
	    0 {
		starplot $x
	    }
	    1 {
		starplot $x [lindex $args 0]
	    }
	    2 {
		starplot $x [lindex $args 0] [lindex $args 1]
	    }
	    3 {
		starplot $x [lindex $args 0] [lindex $args 1] [lindex $args 2]
	    }
	}

# If this command resulted in an error, print it in our message window

    } result]} {
	starplot_error $result
    }
}

#-----------------------------------------------------------------------
# Create an area in which the cursor button actions will be displayed.
#
# Input:
#  w          The path name to assign to the widget.
# Output:
#  return     The path name of the widget.
#-----------------------------------------------------------------------
proc create_starplot_button_show_widget {w} {
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
proc starplot_labeled_entry {w label value args} {
  frame $w
  label $w.l -text $label -fg TextLabelColor
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
# This is a variable-trace callback attached to each plot's input-mode
# variable. Whenever a new input mode is selected, this function
# cancels the previous mode and sets up the new one.
#
# Input:
#  plot    The plot widget to which the mode refers.
#  name    The name of the input variable.
#  ...
#-----------------------------------------------------------------------
proc change_starplot_cursor_mode {plot name args} {

  upvar #0 $name mode
  upvar #0 $plot p

#
# Set up the new cursor mode.
#
  init_starplot_${mode}_cursor $plot
}

#-----------------------------------------------------------------------
# Disable the cursor of the image widget.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_starplot_identify_cursor {plot} {
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "mark_star $plot %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> {}

  describe_starplot_btns $plot {Select a source} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Disable the cursor of the image widget.
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_starplot_zoom_cursor {plot} {
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "starplot_start_zoom $plot %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> "starplot_end_zoom_full $plot"

  describe_starplot_btns $plot {Select an anchor point} {N/A} {Unzoom}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback cancels a zoom registered by 
# im_start_zoom.  It resets the cursor to normal and sets up ket bindings
# for the next zoom.
#
#-----------------------------------------------------------------------
proc starplot_cancel_zoom {plot} {
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next zoom.
#
  bind $pg <1> "starplot_start_zoom $plot %x %y"
  bind $pg <2> {}
  bind $pg <3> "starplot_end_zoom_full $plot"

  describe_starplot_btns $plot {Select an anchor point} {N/A} {Unzoom}
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
proc starplot_start_zoom {plot wx wy} {

  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#
  set x [$pg world x $wx]
  set y [$pg world y $wy]

  $pg setcursor line $x $y 5
#
# If the left mouse button is pressed, start a zoom.
#
  bind $pg <1> "starplot_end_zoom $plot $x $y %x %y"
  bind $pg <2> "starplot_cancel_zoom $plot"
  bind $pg <3> "starplot_end_zoom_full $plot"

  describe_starplot_btns $plot {Select a radius} {Cancel} {Unzoom}
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
proc starplot_end_zoom {plot x1 y1 wx2 wy2} {
  
  set pg $plot.main.p

  set wx2 [$pg world x $wx2]
  set wy2 [$pg world y $wy2]

  $pg setcursor norm 0.0 0.0 1

  starplot setrange $x1 $y1 $wx2 $wy2
  starplot_command redraw

#
# Reset the key bindings to start the next zoom.
#
  bind $pg <1> "starplot_start_zoom $plot %x %y"
  bind $pg <2> {}
  bind $pg <3> "starplot_end_zoom_full $plot"

  describe_starplot_btns $plot {Select an anchor point} {N/A} {Unzoom}
}
#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback is registered by starplotstart_zoom
# It receives the start coordinates of a zoom from starplotstart_zoom and
# the coordinate of the end of the zoom from the callback arguments
# provided by the pgplot widget.
#
# Input:
#  x1 y1          The coordinate of the start of the slice in the image
#                 window. These values were supplied when the callback
#                 was registered by start_slice.
#  wx2 wy2        The X-window coordinate of the end of the slice.
#-----------------------------------------------------------------------
proc starplot_end_zoom_full {plot} {
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

  starplot setrange_full
  starplot_command redraw

#
# Reset the key bindings to start the next zoom.
#
  bind $pg <1> "starplot_start_zoom $plot %x %y"
  bind $pg <2> {}
  bind $pg <3> "starplot_end_zoom_full $plot"

 describe_starplot_btns $plot {Select a radius} {N/A} {Unzoom}
}

#-----------------------------------------------------------------------
# Initialize the NED cursor
#
# Input:
#  plot    The plot to be zoomed.
#  pg      The pgplot widget of the specified plot.
#-----------------------------------------------------------------------
proc init_starplot_ned_cursor {plot} {
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

  bind $pg <1> "starplot_start_ned $plot %x %y"
  bind $pg <B1-Motion> {}
  bind $pg <2> {}
  bind $pg <3> {}

  describe_starplot_btns $plot {Select a position} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# This image-window pgplot-cursor callback cancels a zoom registered by 
# im_start_zoom.  It resets the cursor to normal and sets up ket bindings
# for the next zoom.
#-----------------------------------------------------------------------
proc starplot_cancel_ned {plot} {
  set pg $plot.main.p

  $pg setcursor norm 0.0 0.0 1

#
# Reset the key bindings to start the next NED selection
#
  bind $pg <1> "starplot_start_ned $plot %x %y"
  bind $pg <2> {}
  bind $pg <3> {}

  describe_starplot_btns $plot {Select a position} {N/A} {N/A}
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
proc starplot_start_ned {plot wx wy} {

  set pg $plot.main.p

#
# Convert from X coordinates to world coordinates.
#
  set x [$pg world x $wx]
  set y [$pg world y $wy]

  $pg setcursor line $x $y 5
#
# If the left mouse button is pressed, start a zoom.
#
  bind $pg <1> "starplot_end_ned $plot $x $y %x %y"
  bind $pg <2> "starplot_cancel_ned $plot"
  bind $pg <3> {}

  describe_starplot_btns $plot {Select a search radius} {Cancel} {N/A}
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
proc starplot_end_ned {w x1 y1 wx2 wy2} {
  
  set pg $w.main.p

  set wx2 [$pg world x $wx2]
  set wy2 [$pg world y $wy2]

  $pg setcursor norm 0.0 0.0 1

# Now do the search

  searchNed $w $x1 $y1 $wx2 $wy2

  starplot_command redraw

#
# Reset the key bindings to start the next NED selection
#
  bind $pg <1> "starplot_start_ned $w %x %y"
  bind $pg <2> {}
  bind $pg <3> {}

  describe_starplot_btns $w {Select a position} {N/A} {N/A}
}

#-----------------------------------------------------------------------
# Initiate a NED search
#-----------------------------------------------------------------------
proc searchNed {w x1 y1 wx2 wy2 args} {

    # Clear any masks that are currently set

    starplot clearNedMasks

    # mask the sources the user has currently selected

    starplot maskNedObjects $w.nedObj classifiedExtragalactic
    starplot maskNedObjects $w.nedObj unclassifiedExtragalactic
    starplot maskNedObjects $w.nedObj galactic

    # mask the catalogs

    starplot maskNedCatalogs $w.nedCat
    
    # Set the type of source inclusion selection

    starplot setNedObjectSelection $w.nedObj.select

    # Set the type of catalog selection

    starplot setNedCatalogSelection $w.nedCat.select

    # Finally, do the search

    starplot search_ned $x1 $y1 $wx2 $wy2
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
proc describe_starplot_btns {plot b1 b2 b3} {
  set b $plot.main.b
  $b.b1 configure -text $b1
  $b.b2 configure -text $b2
  $b.b3 configure -text $b3
}

proc starplot_change_site {site w} {

  global ::latVar
  global ::lngVar

  starplot_command change_site $site ::latVar ::lngVar

  $w.ent.lat delete 0 end
  $w.ent.lng delete 0 end

  $w.ent.lat insert 0 $::latVar
  $w.ent.lng insert 0 $::lngVar
}

#-----------------------------------------------------------------------
# Add a NED object to the grid on which we are managing objects
#-----------------------------------------------------------------------

proc addNedObject {w name} {

    global ::objRow

    set nb $w

    set lname     [string tolower $name]
    set allowName [concat "allow " $lname]
    set incName   [concat "include " $lname]
    set excName   [concat "exclude " $lname]

# Create separate frames for the label and the actual buttons

    set name     [label $w.$lname -text $name -fg TextLabelColor -justify left -padx 30  ]

    global $w.$lname

    set allow   [radiobutton $w.$allowName -variable $w.$lname -text {Allow}   -value allow  ]
    set include [radiobutton $w.$incName   -variable $w.$lname -text {Include} -value include  ]
    set exclude [radiobutton $w.$excName   -variable $w.$lname -text {Exclude} -value exclude  ]

    $allow select

    grid configure $name    -row $::objRow -column 0
    grid configure $allow   -row $::objRow -column 1
    grid configure $include -row $::objRow -column 2
    grid configure $exclude -row $::objRow -column 3

    set ::objRow [expr $::objRow + 1]
}

#-----------------------------------------------------------------------
# Add a spacer into the grid on which we are managing our objects
#-----------------------------------------------------------------------

proc addNedObjectSpacer {w name} {

    global ::objRow

    set lname     [string tolower $name]

# Create separate frames for the label and the actual buttons

    set spacer  [frame $w.$lname -relief sunken -bd 2 -bg TextLabelColor]

    grid configure $spacer -row $::objRow -column 0 -columnspan 4 -sticky ew

# And increment the row

    set ::objRow [expr $::objRow + 1]
}

#-----------------------------------------------------------------------
# Add a NED catalog to the grid on which we are managing catalogs
#-----------------------------------------------------------------------

proc addNedCatalog {w name} {

    set lname     [string tolower $name]

# Create separate frames for the label and the actual buttons

    set button [checkbutton $w.$lname -text $name -variable $w.$lname -onvalue selected -offvalue unselected -fg TextLabelColor]

    global $w.lname
    grid configure $button  -row $::catRow -column 0 -columnspan 2 -sticky w

    global ::catRow
    set ::catRow [expr $::catRow + 1]
}

#-----------------------------------------------------------------------
# Add a spacer into the grid on which we are managing our catalogs
#-----------------------------------------------------------------------

proc addNedCatalogSpacer {w name} {

    set lname     [string tolower $name]

# Create separate frames for the label and the actual buttons

    set spacer  [frame $w.$lname -relief sunken -bd 2 -bg TextLabelColor]

    grid configure $spacer -row $::catRow -column 0 -columnspan 2 -sticky ew

# And increment the row

    global ::catRow
    set ::catRow [expr $::catRow + 1]
}

#-----------------------------------------------------------------------
# Reveal the controls menus of the frame grabber widget
#-----------------------------------------------------------------------
proc reveal_nedControls {w} {

# First get the location of the main widget

    getGeometry $w                 xw   yw   x   y

# Set the x location of the ned object window to be to the right of
# the main widget

    set x_obj [expr $x + $xw + 10]
    set y_obj $y

# Set the x location of the ned catalog window to be to the right of
# the main widget

    set x_cat [expr $x + $xw/2 + 10]

    wm deiconify $w.nedObj
    set g_obj +$x_obj+$y_obj
    wm geometry $w.nedObj $g_obj

    wm deiconify $w.nedCat
    getGeometry $w.nedCat xwCat ywCat xCat yCat

    set y_cat [expr $y + ($yw - $ywCat)]

    set g_cat +$x_cat+$y_cat

    wm geometry $w.nedCat $g_cat
}

proc getGeometry {w xwName ywName xName yName} {

    upvar 1 $xwName xw
    upvar 1 $ywName yw
    upvar 1 $xName  x
    upvar 1 $yName  y

    set g [wm geometry $w]
    scan $g "%dx%d+%d+%d" xw yw x y
}
