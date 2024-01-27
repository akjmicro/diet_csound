Csound Build Instructions
=========================

For most folks with any kind of build system, Diet Csound has simplified things.

First for your system, you'll need to make sure you have installed build dependencies.
Do all the appropriate things! It is beyond the scope of this document to tell you how to
get a build system in place for you OS. For any flavor or Debian/Ubuntu/Mint,
for instance, you'd want `build-essential` and friends. For MacOS, `XCode` tools, etc.

Just make sure you have `cmake` as part of that, too, as we've inherited the choice of `cmake`
here from the main `csound` devs.

Secondly, gather, per the install method of your OS, the secondary lib dependencies:
  * `libsndfile`
  * `jack` (Optional, you can disable in `cmake` configs)
  * `portaudio`
  * `portmidi`
  *  `etc...`

You should, on Linux w/root access anyway, be able to simply

    sudo ./build.sh

...and you'll have an installed `csound` in `/usr/local/bin`.

If I've neglected to list any deps you need beforehand, apologies, but I think you'd probably
get a clue from the output as you try to compile.

If you're trying to build on RaspberryPi, there are some relevant notes on compiling
for NEON support in the main `csound` repo's `BUILD.md`. Also, relevant notes exist there
that may give hints for compiling in situations w/o root access. As most folks (99%?) won't
have that issue, I'm not going out of my way to support that in this document, but I'm not
opposed to adding documentation here where relevant. My goal is to make builds less manual
than in the main `csound` repo, after all.

If you have any issues, you can consult the considerably more complex `BUILD.md`
in the main `csound` repository. More complex, but arguably more complete...

That doc is available here: https://github.com/csound/csound/blob/master/BUILD.md

I tried to simplify things here for most people, but do let me know if you hit a snag,
and I'll try to either the script(s) in place for you for the next release, or
add more tips into this document.

If you are a clever human, you can reach out to me: akjmicro AAATTTT gmail DOT com
