# Extra OpenWRT Packages

This repository contains extra packages for my personal development and
usage of OpenWRT. It mainly contains stuff I am either in the process
of upstreaming or have determined to not be a good candidate for
upstreaming, usually for technical reasons.

## Datto DNA

I have created a package for the Datto DNA routers that contains a
platform driver that I wrote for the purposes of making the buttons
and LEDs functional using a kernel module instead of a userspace glue
solution like the original firmware was doing. The end result is that
they are now exposed as regular input and LED devices so that they can
be utilized by a larger variety of scripts and software.

The same package also provides a script to overwrite the board config
so that OpenWRT will have the correct enumeration for the integrated
NICs and also have the correct board id and name.

This is only needed because the Datto DNA routers lack suitable DMI
information for board matching so a custom solution integrated directly
into the image is the only viable alternative. As such the package must
be integrated into your OpenWRT image for it to be effective at all.
