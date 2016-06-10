 (In the run window:)
  % cd ~/cs161/root
  % sys161 -w kernel

  (In the debug window:)
  % script ~/submit/asst0/gdb.script
  % cd ~/cs161/root
  % cs161-gdb kernel
  (gdb) target remote unix:.sockets/gdb
  (gdb) break menu
  (gdb) c
     [gdb will stop at menu() ...]
  (gdb) where
     [displays a nice back trace...]
  (gdb) detach
  (gdb) quit
