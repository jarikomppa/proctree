# ProcTree

This is a liberally licensed procedural tree generator in c++, along with an editor HappyTree.

The procedural generation itself is a port from https://github.com/supereggbert/proctree.js/
see http://snappytree.com for an online demonstration of the original proctree.js (with webgl).

## Binaries

Download win32 binaries at:
http://iki.fi/sol/zip/happytree_20150117.zip

The happytree editor requires OpenGL 3.3, but seriously, who doesn't have that?

Usage should be pretty intuitive, you can rotate the camera by dragging and
zoom in and out with the mouse wheel.

## Motivation

I wanted a tree generator I can use to generate trees at runtime in C.

## License

The generator itself is licensed under 3-clause BSD license (personally I'd have opted
for something even more liberal, but that's what the original proctree.js used). See
the source file for the actual license.

The editor's sources and support libraries are under variety of bsd and zlib liceses.
See legal_notes.txt in the happytree directory for details.

## Dependencies

The generator itself has no dependencies, and was designed to be easily plugged into whatever.

The editor (HappyTree) has a bunch of dependencies:

  - proctree (naturally)  
  - GLee opengl extension library (included)
  - stb_image image loading library (included)
  - My modified opengl framework (included)
  - SDL 1.2 cross-platform library
  - AntTweakBar easy tweak UI
  - glm gl math library
  
## Notes

The ported proctree.cpp does not generate exactly the same output as the original proctree.js.
This may be partially due to the different math accuracy used, but it's possible that I
managed to slip a bug or two into the algorithm. (Feel free to debug..)

## Anything else?

Feel free to contact me, http://iki.fi/sol/
  