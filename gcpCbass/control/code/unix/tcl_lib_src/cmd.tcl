#-----------------------------------------------------------------------
# This module contains a widget designed to act like a terminal.
# It provides command-line editing and command recall facilities modelled
# after tcsh (in emacs mode). In order not to pollute the global namespace
# all functions are declared within a new namespace, designated "Cmd".
# This, and a new binding tag, which is also called Cmd, are the only
# additions to the global namespace.
#-----------------------------------------------------------------------

package provide Cmd 1.0

#-----------------------------------------------------------------------
# Set up the default event bindings for all command-line widgets.
#-----------------------------------------------------------------------

namespace eval Cmd {

  #-----------------------------------------------------------------------
  # This is a wrapper around the bind command. It registers a given
  # binding script to each of a given list of events.
  #
  # Input:
  #  w       The widget to bind.
  #  events  The list of events to bind.
  #  script  The script to bind to each of the events.
  #-----------------------------------------------------------------------
  proc mbind {w events script} {
    foreach event $events {
      bind $w $event $script
    }
  }

#
# When the user presses return, call the user's callback with the
# entered command.
#
  bind Cmd <Return> "Cmd::dispatch_command %W"

# Acquire keyboad input focus and set the selection anchor when mouse
# button 1 is clicked.

  bind Cmd <1> "Cmd::select %W %x %y"

# On receiving a double-click of mouse-button 1 select the word under
# the cursor.

  bind Cmd <Double-1> "Cmd::select_word %W %x %y"

# On receiving a triple-click of mouse-button 1 select the line under
# the cursor.

  bind Cmd <Triple-1> "Cmd::select_line %W %x %y"

# Extend the selection from the last anchor point.

  mbind Cmd {<3> <B1-Motion>} "Cmd::drag_selection %W %x %y"

# By default insert all printable characters at the position of the
# insert cursor.

  bind Cmd <KeyPress> "%W insert insert %A typed; %W see insert"

# Prevent the insertion of control characters. Those control characters
# that we do want, will be bound later.

  mbind Cmd {<Control-KeyPress> <Option-KeyPress> <Meta-KeyPress> <Escape>} break

# Move one character to the left.

  mbind Cmd {<Control-b> <Left> <Shift-Left>} {Cmd::move_cursor %W insert-1c}

# Move one character to the right.

  mbind Cmd {<Control-f> <Right> <Shift-Right>} {Cmd::move_cursor %W insert+1c}

# Move one word to the left.

  mbind Cmd {<Meta-b> <Escape><b> <Control-bracketleft><b> <Control-bracketleft><b> <Control-Left> <Shift-Control-Left> <Option-Left> <Shift-Option-Left>} {Cmd::move_cursor %W [Cmd::start_of_word %W]}

# Move one word to the right.

  mbind Cmd {<Meta-f> <Escape><f> <Control-bracketleft><f> <Control-Right> <Shift-Control-Right> <Option-Right> <Shift-Option-Right>} {Cmd::move_cursor %W [Cmd::end_of_word %W]}

# Move to the start of the line.

  mbind Cmd {<Control-a> <Control-A>} "Cmd::move_cursor %W last_prompt"

# Move the to the end of the line.

  mbind Cmd {<Shift-End> <Control-e> <Control-E>} "Cmd::move_cursor %W end"

# Delete the preceding character.

  mbind Cmd {<BackSpace> <Control-h> <Control-H> <Control-question> <Delete>} "Cmd::del_prev_char %W"

# Delete the next character.

  mbind Cmd {<Control-d> <Control-D>} "Cmd::del_next_char %W"

# Delete the preceding word.

  mbind Cmd {<Meta-Delete> <Escape><Delete> <Control-bracketleft><Delete> <Meta-BackSpace> <Escape><BackSpace> <Control-bracketleft><BackSpace> <Meta-Control-h> <Escape><Control-h> <Control-bracketleft><Control-h> <Escape><Control-question> <Control-bracketleft><Control-question>} "Cmd::del_prev_word %W"

# Delete the next word.

  mbind Cmd {<Meta-d> <Escape><d> <Control-bracketleft><d> <Meta-D> <Escape><D> <Control-bracketleft><D>} "Cmd::del_next_word %W"

# Delete the whole line.

  bind Cmd <Control-u> "Cmd::cut %W last_prompt {end-1c} 1"

# Delete all text to the right of the insertion cursor.

  bind Cmd <Control-k> "Cmd::cut %W insert {end-1c} 1"

# Search the next oldest command line that starts with the text that
# precedes the insertion cursor.

  mbind Cmd {<Meta-p> <Escape><p> <Meta-P> <Escape><P> <Control-bracketleft><p> <Control-bracketleft><P>} "Cmd::search_backward %W \[%W get last_prompt insert\] insert"

# Find the next newest recent command line that starts with the text that
# precedes the insertion cursor.

  mbind Cmd {<Meta-n> <Escape><n> <Meta-N> <Escape><N> <Control-bracketleft><n> <Control-bracketleft><N>} "Cmd::search_forward %W \[%W get last_prompt insert\] insert"

# Get the next oldest history line.

  mbind Cmd {<Control-p> <Control-P> <Key-Up>} "Cmd::search_backward %W {} end"

# Get the next newest history line.

  mbind Cmd {<Control-n> <Control-N> <Key-Down>} "Cmd::search_forward %W {} end"

# Paste the selection after the insertion cursor.

  bind Cmd <ButtonRelease-2> "Cmd::paste %W"

# Scroll to the first line of the widget.

  mbind Cmd {<Home> <Escape><less> <Meta-less> <Control-bracketleft><less>} "%W see 1.0"

# Scroll to the last line of the widget.

  mbind Cmd {<End> <Escape><greater> <Meta-greater> <Control-bracketleft><greater>} "%W see end"

# Scroll back one page.

  mbind Cmd {<Prior> <Meta-v> <Meta-V> <Escape><v> <Escape><V> <Control-bracketleft><v> <Control-bracketleft><V>} "%W yview scroll -1 pages"

# Scroll forward one page.

  mbind Cmd {<Next> <Control-v> <Control-V>} "%W yview scroll 1 pages"

# Register a cleanup handler.

  bind Cmd <Destroy> "Cmd::cleanup %W"

# Register a binding to set the emacs-style cut marker at the current
# location of the insertion cursor.

  mbind Cmd {<Control-space> <Control-at>} "%W mark set mark insert"

# Register a binding that deletes the text between the emacs-style text
# mark and the insert cursor. Copy the deleted text into the clipboard.

  mbind Cmd {<Control-w> <Control-W>} "Cmd::cut %W mark insert 1"

# Register a binding that copies the text between the emacs-style text
# mark and the insert cursor to the clipboard.

  mbind Cmd {<Meta-w> <Meta-W> <Escape><w> <Escape><W> <Control-bracketleft><w> <Control-bracketleft><W>} "Cmd::cut %W mark insert 0"

# Register a binding that pastes the contents of the clipboard at the
# current location of the insertion cursor.

  mbind Cmd {<Control-y> <Control-Y>} "Cmd::paste %W CLIPBOARD"

}

#-----------------------------------------------------------------------
# Create a scrolled command-entry widget.
#
# Input:
#  w         The path name to give the widget.
#  args      Configuration arguments. The supported options along with
#            their default values, are as follows:
#              -width 80                  Set the width of the widget
#                                         (characters).
#              -height 10                 Set the height of the widget
#                                         (characters).
#              -bg black                  Set the background color of
#                                         the widget.
#              -fg yellow                 Set the color of the typed
#                                         text.
#              -bd 2                      Set the border width of the
#                                         widget.
#              -relief sunken             Set the border appearance.
#              -insertbackground white    Set the color of the text
#                                         cursor.
#              -font {Courier -18 bold}   Set the font used for all
#                                         text in the widget.
#              -selectbackground grey     Set the color used to
#                                         highlight text that has been
#                                         selected with the mouse.
#              -prompt "> "               Set the text that is used
#                                         for a prompt.
#              -max_hist 100              The number of lines of
#                                         history to keep.
#              -command {}                The script to execute when
#                                         the user enters a new
#                                         line. The entered line can
#                                         be retrieved by this script
#                                         via [Cmd::get $w].
#              -prompt_fg green           The foreground color of the
#                                         prompt.
#              -output_fg deepskyblue     The foreground color of the
#                                         output text.
# Output:
#  return    The value of $w (ie. the pathname of the internally created
#            frame that encapsulates the compound cmd widget).
#-----------------------------------------------------------------------
proc Cmd::create_widget {w args} {
#
# Place a frame around the text widget.
#
  frame $w -relief ridge -bd 2
#
# Create a scrollable text widget.
#
  set text [text $w.text -width 80 -height 10 -bg black -bd 2 \
            -relief sunken -insertbackground white -insertofftime 0 \
            -font {Courier -18 bold} -selectbackground grey \
            -yscrollcommand [list $w.sy set] -wrap word]
  scrollbar $w.sy -width 3m -orient vertical -command [list $text yview]
  pack $w.sy -side right -fill y
  pack $text -side left -fill both -expand true
#
# Create a namespace array for recording the attributes of the current
# widget.
#
  variable $text
#
# Create an empty history list. In this list, the most recent history
# line is always at the head of the list.
#
  set $text\(history) [list]
#
# Record the number of entries that are currently in the history list.
#
  set $text\(nhistory) 0
#
# The following variable is used to record the index in the history list
# of the last recalled line. It is reset whenever a new line is added to
# the history list.
#
  set $text\(history_index) -1
#
# On starting a history search, the original line is recorded in the following
# variable.
#
  set $text\(pre_history) {}
#
# Set defaults for the prompt, histor-buffer size and the callback script.
#
  set $text\(max_hist) 100
  set $text\(prompt) "> "
  set $text\(command) {}
#
# Set the default attributes of the prompt.
#
  $text tag configure prompt -foreground green
#
# Set the default attributes of output text.
#
  $text tag configure output -foreground deepskyblue
#
# Set the default attributes of the input text.
#
  $text tag configure typed -foreground yellow
#
# Override the defaults with user-provided configuration options.
#
  set result [eval Cmd::configure $w $args]
  if {$result != {}} {
    error $result $result
  }
#
# Start the first line.
#
  Cmd::start_new_line $text
#
# Remove the normal text widget bindings and replace them with those
# defined above for cmd widgets.
#
  bindtags $text [list $text Cmd . all]
  return $w
}

#-----------------------------------------------------------------------
# Interpret a list of configuration arguments to override the default
# attributes.
#
# Input:
#  frame     The frame that encloses the composite widget.
#  args      The list of configuration arguments (the documentation
#            provided above for create_widget lists the supported
#            options).
# Output:
#  return    An error message if an error occurred.
#-----------------------------------------------------------------------
proc Cmd::configure {frame args} {
#
# Get the text widget path.
#
  set text $frame.text
#
# Get the configuration array.
#
  upvar 0 Cmd::$text data
#
# Collect "-option value" pairs in a new list.
#
  set pairs {}
  set option {}
  foreach arg $args {
#
# Are we expecting to see a new option name?
#
    if {$option == {}} {
      if {[string first "-" $arg] >= 0} {
	set option $arg
      } else {
	return "Missing configuration attribute"
      }
#
# Append the lastest option/value pair to the list.
#
    } else {
      lappend pairs [list $option $arg]
      set option {}
    }
  }
#
# Was the last option given a value.
#
  if {$option != {}} {
    return "Missing value for configuration attribute: $option"
  }
#
# Interpret the option/value pairs.
#
  foreach opt $pairs {
    set option [lindex $opt 0]
    set value [lindex $opt 1]
    switch -regexp -- $option {
      {^(-bg)$} {
	$frame configure $option $value
	$text configure $option $value
      }
      {^(-bd|-relief)$} {
	$frame configure $option $value
      }
      {^(-insertbackground|-font|-width|-height|-selectbackground)$} {
	$text configure $option $value
      }
      {^(-prompt|-max_hist|-command)$} {
	set data([string range $option 1 [string length $option]]) $value
      }
      {^-prompt_fg$} {
	$text tag configure prompt -foreground $value
      }
      {^-output_fg$} {
	$text tag configure output -foreground $value
      }
      {^-fg} {
	$text tag configure typed -foreground $value
      }
      default {
	return "Unknown configuration attribute: $option"
      }
    }
  }
  if [winfo ismapped $text] {
    update idlestasks
    $text see insert
  }
  return {}
}

#-----------------------------------------------------------------------
# Return the last line of input that was entered.
#
# Input:
#  frame     The command widget.
# Output:
#  return    The last input line (or {} if nothing has been entered yet).
#            This will not contain any newline characters.
#-----------------------------------------------------------------------
proc Cmd::get {frame} {
#
# Get the context array of the widget.
#
  upvar 0 Cmd::$frame.text data
#
# Return the most recent line from the history list.
#
  return [lindex $data(history) 0]
}

#======================================================================#
# ************* THE FOLLOWING METHOD FUNCTIONS ARE PRIVATE ************#
#======================================================================#

#-----------------------------------------------------------------------
# Return the position of the end of the current word in a command text
# widget.
#
# Input:
#  text     The text widget.
# Output:
#  return   The position of the end of the current word.
#-----------------------------------------------------------------------
proc Cmd::end_of_word {text} {

# Skip spaces.

  set pos 0
  set index insert
  while {[$text compare $index < end] && \
         [string match "\[ \t\]" [$text get $index]]} {
    set index "insert + [incr pos] c"
  }

# Delegate the remaining work to the text widget.

  return [$text index "$index wordend"]
}

#-----------------------------------------------------------------------
# Return the position of the start of the current word in a command text
# widget.
#
# Input:
#  text     The text widget.
# Output:
#  return   The position of the start of the current word.
#-----------------------------------------------------------------------
proc Cmd::start_of_word {text} {

# Skip spaces.

  set pos 1
  set index "insert - $pos c"
  while {[$text compare $index > last_prompt] && \
         [string match "\[ \t\]" [$text get $index]]} {
    set index "insert - [incr pos] c"
  }

# Delegate the remaining work to the text widget.

  return [$text index "$index wordstart"]
}

#-----------------------------------------------------------------------
# Move the insertion cursor to a given position in the text widget of a
# command mega-widget, but don't allow the insertion cursor to exit the
# command line that is being entered.
#
# Input:
#  text      The text widget.
#  pos       The requested position.
#-----------------------------------------------------------------------
proc Cmd::move_cursor {text pos} {
  if [$text compare $pos < last_prompt] {
    set pos last_prompt
  } elseif [$text compare $pos >= end] {
    set pos "end - 1 chars"
  }
  $text mark set insert $pos
  $text see insert
}

#-----------------------------------------------------------------------
# Delete the part of a word that immediately precedes the cursor of a
# command text widget.
#
# Input:
#  text     The text widget.
#-----------------------------------------------------------------------
proc Cmd::del_prev_word {text} {
  if [$text compare insert > last_prompt] {
    Cmd::cut $text [Cmd::start_of_word $text] insert 1
  }
  $text see insert
}

#-----------------------------------------------------------------------
# Delete the part of a word that immediately follows the cursor of a
# command text widget.
#
# Input:
#  text     The text widget.
#-----------------------------------------------------------------------
proc Cmd::del_next_word {text} {
  if [$text compare insert < end] {
    Cmd::cut $text insert [Cmd::end_of_word $text] 1
  }
  $text see insert
}

#-----------------------------------------------------------------------
# Delete the character that immediately precedes the cursor in a
# command text widget.
#
# Input:
#  text     The text widget.
#-----------------------------------------------------------------------
proc Cmd::del_prev_char {text} {
  if [$text compare insert > last_prompt] {
    $text delete "insert-1c"
  }
  $text see insert
}

#-----------------------------------------------------------------------
# Delete the part of a word that immediately follows the cursor of a
# command text widget.
#
# Input:
#  text     The text widget.
#-----------------------------------------------------------------------
proc Cmd::del_next_char {text} {
  if [$text compare insert < end] {
    $text delete insert
  }
  $text see insert
}

#-----------------------------------------------------------------------
# Start a new line in the command-entry widget, along with a prompt.
#
# Input:
#  text    The pathname of the text widget.
#-----------------------------------------------------------------------
proc Cmd::start_new_line {text} {
  upvar #0 ::Cmd::$text\(prompt) prompt
#
# Specify the maximum number of text lines to maintain in the text widget.
#
  set nmax 30
#
# How many lines are currently displayed in the widget?
#
  set ntotal [$text index end]
#
# If the max number of lines has been exceeded, remove the first line to
# make room for the next.
#
  if {$ntotal > $nmax} {
    $text delete 1.0 [expr $ntotal - $nmax]
  }
#
# Start a new line unless we are already at the beginning of a
# line.
#
  if { ![regexp {^[0-9]+\.0$} [$text index insert]] } {
    $text insert end "\n" typed
  }
#
# Output the prompt and place the insertion cursor after it.
#
  $text insert end $prompt prompt
  $text mark set insert end
#
# Set a mark at the end of the prompt. The insertion cursor will be
# restricted to characters beyond this point.
#
  $text mark set last_prompt insert
  $text mark gravity last_prompt left
#
# Make sure that the prompt is visible.
#
  $text see end
  update idletasks
}

#-----------------------------------------------------------------------
# This procedure is called whenever the user presses return in the
# command area.
#
# Input:
#  text      The text widget of the command area.
#-----------------------------------------------------------------------
proc Cmd::dispatch_command {text} {
  upvar 0 Cmd::$text\(history_index) history_index
  upvar 0 Cmd::$text\(history) history
  upvar 0 Cmd::$text\(nhistory) nhistory
  upvar 0 Cmd::$text\(max_hist) max_hist
  upvar 0 Cmd::$text\(command) command
#
# Reset the history-recall index so that the next search starts with
# the most recent history line.
#
  set history_index -1
#
# Discard any emacs-style text mark from the previous line.
#
  $text mark unset mark
#
# Get the extent of the last line in the scrolled area.
#
  set end [$text index end]
  set start [expr $end - 1]
  if {$start < 1.0} {
    return
  }
#
# Get the command and remove trailing whitespace.
#
  set line [string trimright [$text get last_prompt $end]]
#
# Does the line contain any text?
#
  if ![string match "" $line] {
#
# If the new line differs from the most recent line in the history
# list, insert it as the new most recent entry.
#
    if {[string compare [lindex $history 0] $line]!=0} {
      set history [linsert $history 0 $line]
#
# Set a limit to the size of the history list.
#
      if [expr $nhistory > $max_hist] {
	set history [lreplace $history end end]
      } else {
	incr nhistory
      }
    }
#
# If provided, invoke the application's callback script.
#
    if {$command != {}} {
      if [catch "uplevel #0 $command" result] {
	puts stderr $result
      }
    }
  }
#
# Prompt for the next command.
#
  Cmd::start_new_line $text
}

#-----------------------------------------------------------------------
# Search backwards through the history list of the command-input widget.
#
# Input:
#  text      The command-input text widget.
#  prefix    Search backwards for the first line that starts with this
#            string. Note that {} matches any line.
#  leave     Where the place the insertion cursor after substituting a
#            history line.
#-----------------------------------------------------------------------
proc Cmd::search_backward {text prefix leave} {
  upvar 0 Cmd::$text\(history_index) history_index
  upvar 0 Cmd::$text\(history) history
  upvar 0 Cmd::$text\(nhistory) nhistory
  upvar 0 Cmd::$text\(pre_history) pre_history
#
# Search using a temporary index into the history list.
#
  set index $history_index
#
# If this is the start of a search, keep a record of the current line.
#
  if {$index < 0} {
    set pre_history [string trimright [$text get last_prompt end]]
  }
#
# Record the position to leave the insertion cursor at when we are done.
#
  set cursor [$text index $leave]
#
# Search backwards for the first line that starts with $prefix.
#
  while {[incr index] < [llength $history]} {
#
# Get the newly indexed history line.
#
    set line [lindex $history $index]
#
# If the prefix of this line matches the one requested, replace the
# current text with this line, while keeping the insertion cursor in
# the same location so that a followup search will continue with the
# same search prefix.
#
    if {[string compare \
	[string range $line 0 [expr [string length $prefix]-1]] $prefix] == 0} {
      $text delete last_prompt end
      $text insert last_prompt $line typed
      Cmd::move_cursor $text $cursor
#
# Record the history index of the newly adopted line.
#
      set history_index $index
      return
    }
  }
}

#-----------------------------------------------------------------------
# Search forward through the history list of the command-input widget.
#
# Input:
#  text      The command-input text widget.
#  prefix    Search forwards for the first line that starts with this
#            string. Note that {} matches any line.
#  leave     Where the place the insertion cursor after substituting a
#            history line.
#-----------------------------------------------------------------------
proc Cmd::search_forward {text prefix leave} {
  upvar 0 Cmd::$text\(history_index) history_index
  upvar 0 Cmd::$text\(history) history
  upvar 0 Cmd::$text\(nhistory) nhistory
  upvar 0 Cmd::$text\(pre_history) pre_history
#
# Search using a temporary index into the history list.
#
  set index $history_index
#
# Record the position to leave the insertion cursor at when we are done.
#
  set cursor [$text index $leave]
#
# Search forwards for the first line that starts with $prefix.
#
  while {[incr index -1] >= -1} {
#
# Get the newly indexed history line.
#
    if {$index >= 0} {
      set line [lindex $history $index]
    } else {
      set line $pre_history
    }
#
# If the prefix of this line matches the one requested, replace the
# current text with this line.
#
    if {[string compare \
	[string range $line 0 [expr [string length $prefix]-1]] $prefix] == 0} {
      $text delete last_prompt end
      $text insert last_prompt $line typed
      Cmd::move_cursor $text $cursor
#
# Record the history index of the newly adopted line.
#
      set history_index $index
      return
    }
  }
}

#-----------------------------------------------------------------------
# Paste the contents of an X selection buffer after the insertion cursor.
#
# Input:
#  text       The text widget.
# Optional input:
#  selection  The selection to paste. One of:
#               PRIMARY (the default)
#               CLIPBOARD
#-----------------------------------------------------------------------
proc Cmd::paste {text {selection PRIMARY}} {
#
# Get the text to be pasted.
#
  if [catch {
    set lines [selection get -displayof $text -selection $selection]
  }] {
    return
  }
#
# We need newline characters to be treated as though a user had typed
# them in, so split the string into lines. Ignore blank lines.
#
  set last_line {}
  foreach line [split $lines "\n"] {
    if {$last_line != {}} {
      Cmd::dispatch_command $text
    }
    set last_line $line
    if {$line != {}} {
      $text insert insert $line typed
    }
  }
  if {[$text cget -state] == "normal"} {focus $text}
  $text see insert
}

#-----------------------------------------------------------------------
# Give the text widget the input focus (if not disabled) and set the
# selection anchor.
#
# Input:
#  text    The text widget.
#  x y     The coordinates at which the selecting mouse-button was pressed.
#-----------------------------------------------------------------------
proc Cmd::select {text x y} {
#
# Give the widget the keyboard input focus.
#
  if {[$text cget -state] == "normal"} {focus $text}
#
# Remove any existing selection in the widget.
#
  $text tag remove sel 0.0 end
#
# Query the character location of the mouse.
#
  set mouse [$text index @${x},${y}]
#
# Set a mark to record the new anchor position of a future selection.
#
  $text mark set anchor $mouse
}

#-----------------------------------------------------------------------
# Give the text widget the input focus (if not disabled) and select the
# word that lies under the mouse cursor.
#
# Input:
#  text    The text widget.
#  x y     The coordinates at which the selecting mouse-button was pressed.
#-----------------------------------------------------------------------
proc Cmd::select_word {text x y} {
#
# Give the widget the keyboard input focus.
#
  if {[$text cget -state] == "normal"} {focus $text}
#
# Remove any existing selection in the widget.
#
  $text tag remove sel 0.0 end
#
# Query the character location of the mouse.
#
  set mouse [$text index @${x},${y}]
#
# Get the extent of the word that lies under the cursor.
#
  set first [$text index "$mouse wordstart"]
  set last [$text index "$mouse wordend"]
#
# No word?
#
  if {[$text compare $first >= $last]} {
    return
  }
#
# Get the tag of the character under the mouse.
#
  set tag [$text tag names $mouse]
  if {[llength $tag] == 0} return
#
# Determine the extent of the text that has the same tag as the tag of
# the character that the user clicked over.
#
  set tag_range [$text tag prevrange $tag "$mouse + 1 c"]
  set tag_first [lindex $tag_range 0]
  set tag_last [lindex $tag_range 1]
#
# Clip the selected word so that it only includes characters that have
# the same tag as the character over which the user clicked.
#
  if {[$text compare $first < $tag_first]} {
    set first $tag_first
  }
  if {[$text compare $last > $tag_last]} {
    set last $tag_last
  }
#
# Any previously selected region may have shrunk, so remove any selection
# markers that precede and follow the new limits.
#
  $text tag remove sel 0.0 $first
  $text tag remove sel $last end
#
# Reset the anchor and select the word.
#
  $text mark set anchor $first
  $text tag add sel $first $last
#
# Make sure that the selected position is visible.
#
  $text see $mouse
}

#-----------------------------------------------------------------------
# Give the text widget the input focus (if not disabled) and select the
# line that lies under the mouse cursor.
#
# Input:
#  text    The text widget.
#  x y     The coordinates at which the selecting mouse-button was pressed.
#-----------------------------------------------------------------------
proc Cmd::select_line {text x y} {
#
# Give the widget the keyboard input focus.
#
  if {[$text cget -state] == "normal"} {focus $text}
#
# Remove any existing selection in the widget.
#
  $text tag remove sel 0.0 end
#
# Query the character location of the mouse.
#
  set mouse [$text index @${x},${y}]
#
# Get the extent of the line under the cursor.
#
  set first "$mouse linestart"
  set last "$mouse lineend"
#
# No line?
#
  if {[$text compare $first >= $last]} {
    return
  }
#
# Get the tag of the character under the mouse.
#
  set tag [$text tag names $mouse]
  if {[llength $tag] == 0} return
#
# Determine the extent of the text that has the same tag as the tag of
# the character that the user clicked over.
#
  set tag_range [$text tag prevrange $tag "$mouse + 1 c"]
  set tag_first [lindex $tag_range 0]
  set tag_last [lindex $tag_range 1]
#
# Clip the selected line so that it only includes characters that have
# the same tag as the character over which the user clicked.
#
  if {[$text compare $first < $tag_first]} {
    set first $tag_first
  }
  if {[$text compare $last > $tag_last]} {
    set last $tag_last
  }
#
# Any previously selected region may have shrunk, so remove any selection
# markers that precede and follow the new limits.
#
  $text tag remove sel 0.0 $first
  $text tag remove sel $last end
#
# Reset the anchor and select the word.
#
  $text mark set anchor $first
  $text tag add sel $first $last
#
# Make sure that the selected position is visible.
#
  $text see $mouse
}

#-----------------------------------------------------------------------
# Extend the selection from the current anchor marker to the given x,y
# position.
#
# Input:
#  text    The text widget.
#  x y     The coordinates at which the selecting mouse-button was pressed.
#-----------------------------------------------------------------------
proc Cmd::drag_selection {text x y} {
#
# Get the character index under the specified x,y position.
#
  set cursor [$text index @${x},${y}]
#
# Get the two end points of the selection in ascending order.
#
  if [$text compare anchor < $cursor] {
    set first anchor
    set last $cursor
  } else {
    set first $cursor
    set last anchor
  }
#
# Deselect all characters.
#
  $text tag remove sel 0.0 end
#
# Get the tag that lies under the anchor point.
#
  set tag [$text tag names anchor]
  if {[llength $tag] == 0} return
#
# If the character under $first is the same as under the anchor then
# figure out how many characters follow it before $last that have the
# same tag. These should be selected.
#
  if {[string compare [$text tag names $first] $tag] == 0} {
    set range [$text tag prevrange $tag "$first + 1 c"]
    set last_tag [lindex $range 1]
    if {[$text compare $last_tag > $last]} {
      $text tag add sel $first $last
      set first [$text index "$last + 1 c"]
    } else {
      $text tag add sel $first $last_tag
      set first [$text index "$last_tag + 1 c"]
    }
  }
#
# Pick out subsequent parts of the new select range that
# have the same tag character as the anchor and select them.
#
  while {[$text compare $first <= $last]} {
    set range [$text tag nextrange $tag $first $last]
    if {[llength $range] == 2} {
      set first_tag [lindex $range 0]
      set last_tag [lindex $range 1]
      if {[$text compare $last_tag > $last]} {
	$text tag add sel $first_tag $last
	set first [$text index "$last + 1 c"]
      } else {
	$text tag add sel $first_tag $last_tag
	set first [$text index "$last_tag + 1 c"]
      }
    } else {
      set first [$text index "$last + 1 c"]
    }
  }
#
# Make sure that the end of the selection is visible.
#
  $text see $last
}

#-----------------------------------------------------------------------
# Delete the private data associated with a command widget.
#
# Input:
#  text    The text widget of the command widget.
#-----------------------------------------------------------------------
proc Cmd::cleanup {text} {
  variable $text
  unset $text
}

#-----------------------------------------------------------------------
# If requested, delete the text between the two specified positions, then
# copy the deleted text into the clipboard.
#
# Input:
#  text     The text widget.
#  first    The start of the range to be cut.
#  last     The end of the range to be cut.
#  delete   If true delete the text. Otherwise just copy it to the
#           clipboard.
#-----------------------------------------------------------------------
proc Cmd::cut {text first last delete} {
#
# If either position is invalid (eg. the text mark isn't always set),
# or the start and end positions are the same, ignore the call.
#
  if {[catch {$text index $first}] || [catch {$text index $last}]} return
  if [$text compare $first == $last] return
#
# Sort the two positions into ascending order.
#
  if [$text compare $first > $last] {
    set tmp $first
    set first $last
    set last $tmp
  }
#
# Place a copy of the text in the clipboard.
#
  catch {
    clipboard clear -displayof $text
    clipboard append -displayof $text [$text get $first $last]
  }
#
# Delete the text.
#
  if $delete {
    $text delete $first $last
  }
#
# Make sure that the user sees the result.
#
  $text see insert
}

#-----------------------------------------------------------------------
# Insert output text before the current line.
# 
# Input:
#  frame    The command widget.
#  output   The text to be inserted.
#-----------------------------------------------------------------------
proc Cmd::output {frame output} {
#
# Get the embedded text widget.
#
  set text $frame.text
#
# The new text will be inserted just before the current prompt.
#
  set where {last_prompt linestart}
#
# Operate on the individual lines of the output text one by one.
#
  foreach line [split $output "\n"] {
    if {[string length $line] > 0} {
      $text insert $where [append line \n] output
    }
  }
#
# Make sure that the last line is visible.
#
  $text see end
}
