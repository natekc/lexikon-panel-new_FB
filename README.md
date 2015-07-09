# lexikon-panel-new_FB
lexikon panel & backlight driver for CAF kernel

What works:
- Panel on/off

What doesn't work:
- Incremental backlight adjustment (sometimes device reboot)
cause: writing to register causes device shutdown, possible illegal address access?
