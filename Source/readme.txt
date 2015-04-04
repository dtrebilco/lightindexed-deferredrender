About
====

This demo is a technical demonstration of the lighting technique 
"Light Indexed Deferred Rendering".

A full explanation of the technique used in this demo can be found at:
https://github.com/dtrebilco/lightindexed-deferredrender/blob/master/LightIndexedDeferredLighting1.1.pdf
http://lightindexed-deferredrender.googlecode.com/files/LightIndexedDeferredLighting1.1.pdf

The scene consists of 255 lights with ~40 000 polygons. This demo allows you 
to switch between different "Light Indexed Deferred Rendering" techniques 
and standard multi-pass forward rendering.

To view most of the 255 lights in a single view, select "Set static scene view" 
from the F1 options menu.

There are several places in the demo scene where artifacts can be seen when the light 
index count is saturated. These have been left in to demonstrate what happens when 
the light count saturates. To easily see this effect, switch to "1 Light per fragment"
mode.

Also note that this lighting technique was designed for a scene with lots of objects, 
polygons and materials which this demo does not demonstrate. This demo is more of a 
stress test with 255 lights.


Author
======

This demo was made by Damian Trebilco ( dtrebilco@gmail.com ) 
based on a demo by Emil Persson, aka Humus.


Controls
========

 Space - Toggles frame rate counter

 D     - Toggles deferred rendering/multi-pass forward rendering

 F1    - Opens the settings/rendering options dialog. 
 
If you want to experiment with changing light positions/sizes/colors, an editor mode is provided.

 E     - Toggles editor mode
 
 D     - Dumps editor light data to stdout (only when in editor mode)

 +/-   - Change current editing light index
 
 NUMPAD7/NUMPAD4   - Change light red color

 NUMPAD8/NUMPAD5   - Change light green color
 
 NUMPAD9/NUMPAD6   - Change light blue color 

 Mouse Left button - Position light
 
 Mouse Wheel       - Change light size


Bugs 
====
This demo has only been extensively tested on Windows XP - but there is a good chance it will work
OK on Vista or Linux. MacOS is completely untested - however the framework does support it - so it 
may work.

Currently this demo has only tested with Geforce 6/7/8 series and ATI 9550/X1800/2900 cards.
Latest drivers will be required for older cards.

There has been some reported issues with ATI mobile X1600 (lighting does not work?).
This may be solved by uncommenting the lines "lightIndex += 0.5/256.0" in the files 
lightingLIDefer.shd and lightingLIDefer_stone.shd. (Not tested however)

Newer drivers on most ATI cards (as of August 2008) seem to introduce a "halo" bug effect
around the lights. Hopefully this will be fixed in later drivers. 

ATI cards use a slower stencil volume technique to render light volumes and older cards do
not support the 4 lights per fragment option. For this reason, this demo in the default state 
is not good for benchmarking cards.

Currently the light volumes when rendered using stencil volumes use a depth-clamp "hack", when 
the view point intersects the light volume.

MSAA is not handled correctly (see the paper for ways of handling MSAA)

Fullscreen mode may not work correctly in some multi-monitor configurations.

If you are seeing incorrect rendering, trying switching on the 
"Precision Test" mode from the F1 menu. If you see any red or black 
on the screen, your video card is having trouble with the floating point 
math calculations. 
If you only see white/yellow, your video card is fine.
(This is a "hack" until all video cards support bit-logic in shaders)


Legal stuff
===========

This demo is freeware and may be used by anyone for any purpose
and may be distributed freely to anyone using any distribution
media or distribution method as long as this file is included.



Compiling code
==============

The provided code was compiled with Visual Studio 2005 and should include all needed files.
Note that the FrameWork3 files have been changed slightly from the Humus framework code.


Special thanks
==============

Spacial thanks goes to 

  Emil Persson   - For providing the frame work/demos and ideas.
  
  Timothy Farrar - For suggesting the max-blend equation light index packing technique.
  
  Alistair Doolwind - For support and proofreading of the main document. 
  
  
=====================================================================================================================
Do not conform any longer to the pattern of this world, but be transformed by the renewing of your mind.
Then you will be able to test and approve what God's will is - his good, pleasing and perfect will. (Romans 12:2 NIV)
