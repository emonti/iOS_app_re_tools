== Synopsis:

Miscellaneous tools and bits of code written for experimenting with iOS reversing and library injection.

The library injection examples are configured for Just Light Flashlight (a free download), but are generic enough to have interesting results with other apps as well. See the plist files for hints.

== Requirements:

The included C code mostly requires a jailbroken phone. MobileSubstrate is probably good to have, at least to get started.

You'll also need either the iOS SDK or iPhone opensource development toolchain.

== Setup:

Create symbolic links in the same directory as the Makefile as follows. 

    toolchain -> /Developer/Platforms/iPhoneOS.platform/Developer
    sdk -> toolchain/SDKs/iPhoneOS3.1.3.sdk

This is what they look like on my system. You may want to change these depending on what SDK you use, and whether it is the opensource toolchain.

Run "make" on the build host.

Drop the desired dylib and plist into the following directory on the phone:

/Library/MobileSubstrate/DynamicLibraries/

That's it... launch "Just Light" on the device and see what happens. 

You may want to monitor the device log, which you can do in real time using idevicesyslog from the libimobiledevice package.

    http://www.libimobiledevice.org/

For Mac OS X, there is a formula for libimobiledevice using Homebrew which greatly simplifies installation.

    https://github.com/mxcl/homebrew

