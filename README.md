# Pluck

A plucked-string synth for the OP-XY, built with the
[Open-XY SDK](https://github.com/etcetc000/open-xy): the example of a machine
that lives in its own repository and reaches the Open-XY builder through the
catalog.

Each note fills a delay line one period long with a burst of noise. The burst
circulates, losing a little treble and energy each pass, like a string
(Karplus-Strong synthesis). The OP-XY's own envelope and level shape the note;
the controls shape the string.

| Control | Effect |
| --- | --- |
| Bright | The pluck's brightness: a soft finger to a hard pick |
| Pick | Where the string is plucked, from near the bridge to the middle |
| Damp | Treble lost on every pass: a glassy, metallic ring to a dull, felt-muted string |
| Detune | A second string up to about 25 cents apart, for a chorused, twelve-string shimmer |

The screen shows the string between its pegs, swinging after each note from
the Pick position and settling faster the more it is damped.

| File | Owns |
| --- | --- |
| [definition.json](definition.json), [machine.json](machine.json) | Controls, presets, resources, SDK version and identity |
| [core.h](core.h), [engine.cpp](engine.cpp) | Voice state and sound |
| [display.h](display.h), [display.cpp](display.cpp), [panel.svg.fragment](panel.svg.fragment) | The screen |
| [tests/pluck.cpp](tests/pluck.cpp) | Sound checks: tuning at every Damp, ringing and fading, Damp's darkening, Detune and the declared peak |

With the SDK cloned next to this repository:

```sh
python ../open-xy/sdk.py test --machine . --cxx clang++ --host-cxx g++
python ../open-xy/sdk.py preview --machine . --out ../previews/pluck-001
python ../open-xy/tools/serve_harness.py --preview ../previews/pluck-001
```

Released versions are tags; the Open-XY catalog pins one of them.
