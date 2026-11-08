# hyperx-fix

Fixes the **HyperX Alloy FPS Pro** (HX-KB4RD1, USB `0951:16d2`) on macOS: backlight Fn shortcuts
(Fn+↑/↓ brightness, Fn+←/→ and Fn+1–6 effects) and media/volume keys don't work.

## Symptoms (macOS)

- The backlight flashes when you plug the keyboard in, then stays dark or stuck in one mode
- Fn+↑ just sends ↑, and Fn+1/2 just type 1/2
- Volume and media keys do nothing
- Caps Lock and the game-mode LED (Fn+F12) still work
- Everything works on Linux or Windows, and may keep working after a KVM switch back to the Mac,
  until you unplug the keyboard

## Cause

The keyboard's firmware starts up in HID **boot protocol** (the minimal BIOS mode) instead of
report protocol, which the HID spec says should be the default. In boot mode it disables its
lighting Fn layer and media keys; Fn+F12 game mode still works. Linux and Windows send
`SET_PROTOCOL(report)` when a keyboard connects. macOS doesn't, so the keyboard stays in boot mode.

It looked like the keyboard was failing because a KVM switch keeps the keyboard powered: after
the Linux host switched it to report mode, it kept working on the Mac until a full unplug.

## Fix

`hyperx-fix` watches IOKit for the keyboard connecting (plugging in, KVM switch, wake from sleep)
and sends this request to interfaces 0 and 1:

```
21 0B 01 00 0i 00 00 00   # host→interface class request, SET_PROTOCOL, value 1 = report, wIndex = i
```

It runs as a per-user LaunchAgent. No root access is needed.

## Use

```sh
make install     # build, copy to ~/Library/Application Support/hyperx-fix, load LaunchAgent
make status      # agent state + log (/tmp/hyperx-fix.log)
make uninstall
```

To use it with a different keyboard that has the same bug, change the VID/PID in `hyperx-fix.c`.
