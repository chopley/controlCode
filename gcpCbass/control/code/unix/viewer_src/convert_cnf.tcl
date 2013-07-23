#!/usr/local/bin/tclsh

proc interval {t} {
  puts "configure_viewer $t 500000"
}

proc add_plot {body} {
  set ::p(graphs) [list]
  set ::ngraph 0
  eval $body
  puts "add_plot {} {} [list $::p(title)] $::p(xmin) $::p(xmax) $::p(marker_size) $::p(join_mode) $::p(scroll_mode) $::p(scroll_margin) [list $::p(xregister)] [list $::p(xlabel)] {"
  foreach graph $::p(graphs) {
    upvar #0 $graph g
    puts "  graph $g(ymin) $g(ymax) [list $g(ylabel)] [list $g(yregs)] {}"
  }
  puts "}"
}

proc xrange {xmin xmax} {
  set ::p(xmin) $xmin
  set ::p(xmax) $xmax
}
proc marker_size {s} {
  set ::p(marker_size) $s
}
proc join_mode {m} {
  set ::p(join_mode) $m
}
proc scroll_mode {m} {
  set ::p(scroll_mode) $m
}
proc scroll_margin {m} {
  set ::p(scroll_margin) $m
}
proc xregister {r} {
  set ::p(xregister) $r
}
proc xlabel {s} {
  set ::p(xlabel) $s
}
proc plot_title {t} {
  set ::p(title) $t
}
proc add_graph {body} {
  set ::graph ::graph[incr ::ngraph]
  lappend ::p(graphs) $::graph
  upvar #0 $::graph g
  set g(yregs) ""
  eval $body
}

proc yrange {ymin ymax} {
  upvar #0 $::graph g
  set g(ymin) $ymin
  set g(ymax) $ymax
}
proc ylabel {ylabel} {
  upvar #0 $::graph g
  set g(ylabel) $ylabel
}
proc yregister {yregister} {
  upvar #0 $::graph g
  if {[string length $g(yregs)] == 0} {
    set g(yregs) $yregister
  } else {
    append g(yregs) " " $yregister
  }
}

proc add_page {body} {
  set ::p(fields) [list]
  set ::nfield 0
  eval $body
  puts "add_page $::p(x) $::p(y) [list $::p(title)] [list $::p(color)] [list $::p(warn_color)] $::p(def_width) $::p(font_size) [list $::p(column_widths)] [list $::p(group)] 0 {"
  foreach field $::p(fields) {
    upvar #0 $field f
    switch -- $f(type) {
      label {
	puts "  label $f(col) $f(row) [list $f(bg)] [list $f(fg)] $f(justify) [list $f(text)]"
      }
      register {
	puts "  register $f(col) $f(row) [list $f(bg)] [list $f(fg)] $f(justify) [list $f(register)] [list $f(format)] [list $f(precision)] [list $f(misc)] [list $f(enum)] [list $f(signed)] [list $f(warn)] [list $f(min)] [list $f(max)]"
      }
    }
  }
  puts "}"
}

proc location {x y} {
  set ::p(x) $x
  set ::p(y) $y
}

proc title {t} {
  set ::p(title) $t
}

proc color {c} {
  set ::p(color) $c
}
proc warn_color {c} {
  set ::p(warn_color) $c
}
proc def_width {w} {
  set ::p(def_width) $w
}
proc font_size {s} {
  set ::p(font_size) $s
}
proc column_widths {args} {
  set ::p(column_widths) $args
}
proc group {g} {
  set ::p(group) $g
}
proc add_label {row col body} {
  set ::field ::field[incr ::nfield]
  lappend ::p(fields) $::field
  upvar #0 $::field f
  set f(type) label
  set f(row) $row
  set f(col) $col
  eval $body
}
proc add_register {row col body} {
  set ::field ::field[incr ::nfield]
  lappend ::p(fields) $::field
  upvar #0 $::field f
  set f(type) register
  set f(row) $row
  set f(col) $col
  eval $body
}

proc bg {bg} {
  upvar #0 $::field f
  set f(bg) $bg
}
proc fg {fg} {
  upvar #0 $::field f
  set f(fg) $fg
}
proc justify {justify} {
  upvar #0 $::field f
  set f(justify) $justify
}
proc text {text} {
  upvar #0 $::field f
  set f(text) $text
}
proc register {register} {
  upvar #0 $::field f
  set f(register) $register
}
proc format {format} {
  upvar #0 $::field f
  set f(format) $format
}
proc precision {precision} {
  upvar #0 $::field f
  set f(precision) $precision
}
proc misc {misc} {
  upvar #0 $::field f
  set f(misc) $misc
}
proc enum {enum} {
  upvar #0 $::field f
  set f(enum) $enum
}
proc signed {signed} {
  upvar #0 $::field f
  set f(signed) $signed
}
proc warn {warn} {
  upvar #0 $::field f
  set f(warn) $warn
}
proc min {min} {
  upvar #0 $::field f
  set f(min) $min
}
proc max {max} {
  upvar #0 $::field f
  set f(max) $max
}

if {$argc != 1} {
  puts "Usage: $argv0: <configuration_file>"
  exit 1
}
set file [lindex $argv 0]
if {![file exists $file]} {
  puts "$argv0: Couldn't find file: $file"
  exit 1
}
source $file

