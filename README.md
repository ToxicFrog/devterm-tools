# devterm setup

This repo holds my configuration for the ClockworkPi DevTerm-R01. It's a low
power RISC-V machine, so I want to build as much stuff on ancilla as possible
and push it to the devterm.

`profile.nix` includes a set of binaries to be installed with `nix profile install`.
`deploy` will build it and push it to the devterm, removing the previous profile
if one exists.

Other tools are in the works.
