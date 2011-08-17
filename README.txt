== Synopsis:

Miscellaneous reversing and hacking tools for iOS reversing and library injection.

The library injection examples are configured for Just Light Flashlight (a free download), but are generic enough to have interesting results with other apps as well. See the plist files for hints.

== Requirements:

The included code mostly requires a jailbroken phone. MobileSubstrate is also good to have, at least to get started testing library injection.

You'll also need either the iOS SDK or iPhone opensource development toolchain.

== Compiling:

Create symbolic links in the same directory as the Makefile as follows. 

    toolchain -> /Developer/Platforms/iPhoneOS.platform/Developer
    sdk -> toolchain/SDKs/iPhoneOS4.3.sdk

This is what they look like on my system. You may want to change these depending on what SDK you use, and whether it is the opensource toolchain.

Run "make" on the build host.

== Usage:

Most of the commandline utilities are self explanatory or have usage information.

To test the injectable dylibs with MobileSubstrate drop the desired dylib and plists into the following directory on the phone:

/Library/MobileSubstrate/DynamicLibraries/

That's it... launch "Just Light" on the device and see what happens. 

The phoshizzle examples use NSLog() so you may want to monitor the device log. You can do this from Xcode organizer or by using idevicesyslog from the libimobiledevice package.

    http://www.libimobiledevice.org/

For Mac OS X, there is a formula for libimobiledevice using Homebrew which greatly simplifies installation.

    https://github.com/mxcl/homebrew

