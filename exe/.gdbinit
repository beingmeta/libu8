directory ..
set env LD_LIBRARY_PATH="../lib/:${LD_LIBRARY_PATH}"
set env LOGGING=8
set args PREFIX 7777
echo "Loaded libu8 .gdbinit\n"
define debug_u8run
  break dolaunch
  break launch_loop
  break main:start_run
  break main:got_args
  break main:get_defaults
  break main:setup_env
  break main:setup_stdout
  break main:check_files
end
source .gdbinit.local
