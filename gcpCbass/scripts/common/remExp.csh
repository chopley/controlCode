#/usr/bin/tcsh

if( $# != 1) then
  echo "You must specify an experiment name"
  exit
endif

foreach dir (./help ./canbus ./matlab ./modules ./control/conf ./control/code/unix/control_src ./control/code/unix/libunix_src ./directives ./util ./mediator ./antenna/control ./antenna/canbus ./help ./scripts)
  if ( -e $dir/$1 ) then
      \rm -rf $dir/$1
  endif
end


