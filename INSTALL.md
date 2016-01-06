Prerequisites:
* Fully funtional LSF installation
  * Path to C include: $LSF_BINDIR/../../include
  * Path to LSF Libs: $LSF_LIBDIR
* Execution node must be licensed to run LSF commands (otherwise LSF's API calls will fail)
* C++ compiler (tested on GCC 5)

Example compile on Linux:
```
g++ -I$LSF_BINDIR/../../include -L$LSF_LIBDIR  -lnsl -llsf -lbat -o lsf_active_reservations lsf_active_reservations.c
```

Example usage:
```
(root@login)/tmp/$ ./lsf_active_reservations 
./lsf_active_reservations {host regex} {label}
Dumps LSF Advance reservation statistics against host regex
format: type hosts slots reservations
(root@login)/tmp/$ ./lsf_active_reservations '^compute' 'batch_'
batch_system 0 0 0
batch_user 0 0 0
```
