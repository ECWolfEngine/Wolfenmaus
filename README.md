# Wolfenmaus - Modern mouse control for Wolfenstein 3D/Spear of Destiny

Wolfenmaus enables vanilla Wolfenstein 3D or Spear of Destiny to achieve modern
style WASD controls. Unlike other solutions, Wolfenmaus does not modify the
Wolf3D code in any way (on disk or in memory), but uses the interfaces that are
built into the game.

## Game support

- Wolfenstein 3D - All 1.4 versions
- Spear of Destiny - All versions
- Many 16-bit DOS mods of the above

Other games that use the Wolfenstein 3D engine lack the required interface for
this to work. See the "How it works" section for more information.

## Limitations

- The mouse look angle is not reset between levels and can accumulate while in
  the menus, so you may often start facing an unintended direction.
- This can not be used to record demos as the angle you're facing is not part
  of the game's normal control structure.
- The `nowait` flag is implied, so the demo loop is skipped.
- The game will no longer lock your camera angle during ending sequences.
- A frame (~14ms at least) of input latency is incurred due to the look angle
  being applied prior to polling the inputs.

## Usage

Place `wolfmaus.exe` in the game directory and run it passing along the command
to run the game.

```
wolfmaus [options] wolf3d ...
```

Options:
- `-i<mask>` - Inverts the function of mouse buttons (1 = left, 2 = right, 4 =
  middle). Add values together to invert more than one button. Button inversion
  should only apply during gameplay.
- `-s<num>` - Sensitivity value matching the in game configuration (0 = slow, 9
  = fast). Default value is 5. Other values except 13 are accepted and would
  have the same effect as configuration hacking in vanilla. Values 14 and above
  invert the x axis.
- `-v` - Disables vertical mouse movement in game. Will still work in the
  menus.

Since Wolf3D binds strafe to right click by default, a typical invocation might
look like:

```
wolfmaus -i2 -s9 -v wolf3d goobers
```

For some reason, likely because they expected all movement to be done with the
mouse and in that scenario run would have no effect, id arbitrarily prevented
binding run to the mouse via the menus. It is possible to enable always run by
modifying your config file. Set the byte at `01F6` (left), `01F8` (right), or
`01FA` (middle) to `02`. The location of these values in the config file may
vary with mods. If successful the menu will show the respective button being
bound. The button can then be added to the inversion mask. Note that you can
still walk but you will have to use the mouse button and not the keyboard or
joystick.

If you consider input inversion cheating, it is worth noting that Wolf3D menu
allows multiple actions to be assigned to the same keyboard key. If you spend
most of the time in game with the run key pressed, binding strafe to the same
key would prevent the need to hold a second key.

## How it works

The problem that vanilla Wolf3D faces is that the game's input code only has
two analog axes (`controlx` and `controly`). These are populated with the sum
of inputs from the keyboard, mouse, and joystick. When you press the strafe
button the `controlx` axis is reinterpreted from being turning to being strafe.

There has, in a way, been a hidden third axis the whole time presumably thanks
to Alternate Worlds Technology. Wolfenstein 3D includes support for a VR
headset and through that we unlock the ability to offset the player's angle and
this offset is independent of the strafe control. In modern times one would
expect that this offset only applies to the camera, but for Wolf3D this is
implemented simply as adding the `helmetangle` (a value between `0` and `359`
located in memory at `0040:00F0`) at the start of the frame/tic and backing it
out at the conclusion of the frame.

Wolfmaus can effectively be seen as a modified mouse driver which more
importantly implements the interface expected for the VR helmet. The horizontal
mouse movements are simply redirected to `helmetangle`.

Since the VR headset code is included in the Wolf3D source code as released
this automatically works with many DOS mods. It does not work with all of them
as the VR code was previously seen as useless, so particularly the more
advanced mods may have removed the feature to reclaim a small amount of memory.
Additionally Wolf4GW removed most of the VR code so any 32-bit mod will not
have the required interface point.

Whether it be because they removed it, or because the code they were licensed
did not include it, none of the games that licensed the Wolf3D engine include
the VR code. There is apparently [evidence that Alternate Worlds Reality had a
version of Blake Stone for their headset][AWT], but none of the released
builds contain the code. As of this writing it is anyone's guess if copies of
it or AWT's own game using the engine, Cybertag, still exist. If they were
found presumably this would just work with them.

[AWT]: https://web.archive.org/web/20180925104927/http://www.curiousraven.com/future-vision/2013/12/13/my-first-tech-job-was-creating-virtual-reality

## Compiling

This program is written for Open Watcom v2. Ensure that the compiler is added
to your `PATH` and the Makefile should take care of the rest. If not the
`WATCOM`, `INCLUDE`, and `LIB` environment variables can be set accordingly.

```bash
# Build wolfmaus.exe
make
```

## Credits

Thanks to NY00123 for brainstorming support. This was identified while looking
for code that was still disabled after finishing Disney Sound Source support in
ReflectionHLE and pondering how the VR code would be enabled.

Thanks to Woolie Wool/Executor for naming the project.
