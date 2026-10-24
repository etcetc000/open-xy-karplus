# Pluck

A plucked-string synth for the OP-XY, built with the
[Open-XY SDK](https://github.com/etcetc000/open-xy): the example of a machine
that lives in its own repository and reaches the Open-XY builder through the
catalog.

Each note fills a delay line one period long with a burst of noise. The burst
circulates, losing a little energy and treble each pass, like a string
(Karplus-Strong synthesis).

| Control | Effect |
| --- | --- |
| Decay | How long the string rings, from muted to a long sustain |
| Bright | The pluck's brightness: a soft finger to a hard pick |
| Pick | Where the string is plucked, from near the bridge to the middle |
| Level | Output level |

The screen shows the string between its pegs, swinging after each note from
the Pick position and settling at the Decay rate.

| File | Owns |
| --- | --- |
| [definition.json](definition.json), [machine.json](machine.json) | Controls, presets, resources, SDK version and identity |
| [core.h](core.h), [engine.cpp](engine.cpp) | Voice state and sound |
| [display.h](display.h), [display.cpp](display.cpp), [panel.svg.fragment](panel.svg.fragment) | The screen |
| [tests/pluck.cpp](tests/pluck.cpp) | Sound checks: tuning, decay and the declared peak |

With the SDK cloned next to this repository:

```sh
python ../open-xy/sdk.py test --machine . --cxx clang++ --host-cxx g++
python ../open-xy/sdk.py preview --machine . --out ../previews/pluck-001
python ../open-xy/tools/serve_harness.py --preview ../previews/pluck-001
```

Released versions are tags; the Open-XY catalog pins one of them.
