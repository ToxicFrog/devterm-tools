# DevTerm Tools

This repo holds my configuration and tooling for the [CPI DevTerm R01](https://www.clockworkpi.com/home-devterm).

The only part of this that is actually done is the [devterm port of `yaft`](./yaft/README.md), so you should probably look there.

Most of it is still a work in progress. Using it requires you have a host system with Nix installed, and also that you have ssh access to, and Nix installed on, the DevTerm itself (a process that currently requires a custom build of the Nix installer).

If you have all this set up, `nix-build profile.nix` will emit something you can install with `nix copy` and `nix profile install`. The [`deploy`](./deploy) script automates this (on my machine). If you don't have `nix` installed on the target (but do on the host), you may still be able to deploy something with `nix bundle`, but I haven't tested this.

I am working on (but do not yet have) an installer script that will automate some of this and guide you through the rest.