## `/implementation`
This directory contains the modified `linuxptp` sources. 
The [code](https://sourceforge.net/p/linuxptp/code/ci/24220e8) at commit hash `24220e8` was used.

The relevant files and directories are the following:
+ `./auth/` contains the SPD and SAD TOML definition
+ `./libs/` contains pre-built hashing libraries (check the README there for more info)
+ `authentication_db.h` bundles SAD and SPD as struct
+ `clock.{c,h}` holds references to the security databases for use in the application
+ `config.c` parses the `protect_messages` switch in a config file
+ `ddt.h` features an enhanced `PortStats` struct for security stats
+ `makefile` is adapted for our security extension 
+ `pmc.c` displays some security stats now (cf. `PortStats`)
+ `port.c` contains the calls to the SPD and SAD during message processing
+ `port_private.h` now contains a switch to toggle security processing
+ `sad.{c,h}` contains the SAD code
+ `spd.{c,h}` contains the SPD code
+ `tlv.h` contains the `AUTHENTICATION` TLV
+ `version.c` additonally displays `Security Edition`

For Blake3, the library is custom-built (cf. `./libs/`). 

For Blake2, the ready-to-use `libb2-dev` package is used. For libsodium, the `libsodium-dev` package is used. Please make sure these dependencies are met.