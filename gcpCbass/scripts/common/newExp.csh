#/usr/bin/tcsh

if( $# != 1) then
  echo "You must specify an experiment name"
  exit
endif

foreach dir (./help ./canbus ./matlab ./modules ./control/conf ./control/code/unix/control_src ./control/code/unix/libunix_src ./directives ./util ./mediator ./antenna/control ./help ./misc/scripts)
  if ( -e $dir/expstub ) then
    if ( ! -e $dir/$1 ) then
      mkdir $dir/$1
      \cp $dir/expstub/*.cc $dir/$1
      \cp $dir/expstub/*.c $dir/$1
      \cp $dir/expstub/*.h $dir/$1
      \cp $dir/expstub/Makefile $dir/$1
      \cp $dir/expstub/Makefile.flags $dir/$1
      \cp $dir/expstub/Makefile.directives.template $dir/$1
    else
      echo "Directory: " $dir/$1 " already exists!"
    endif
  endif
end


