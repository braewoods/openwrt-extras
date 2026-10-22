# Extra OpenWRT Packages

This repository contains extra packages for my personal development and
usage of OpenWRT. It mainly contains stuff I am either in the process
of upstreaming or have determined to not be a good candidate for
upstreaming, usually for technical reasons.

## gpio-nuvoton

At the time I created this package I found the state of GPIO drivers for
Nuvoton Super-I/O chips to be pretty lackluster. I saw plenty that had one
or more of the following problems: focused on supporting a very specific
chip series, focused on a very specific situation, or support appeared as
if it was an afterthought, usually due to being written for a different
vendor's design.

I designed this kernel module to support a wide range of Nuvoton chips, but
I won't know how well that worked out until support for other chips is added.
The only drawback I can see with my design is that the only banks I expose are
the ones that were already marked as enabled when the module was loaded. This
means you can only access the GPIO pins that the BIOS or chip had enabled by
default. This seemed like a prudent choice for most systems because the GPIO
pins only have meaning if they are already connected to something.

In any case, the following chips are known to be supported:

* NCT6102D / NCT6104D / NCT6106D

## datto-dna

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
