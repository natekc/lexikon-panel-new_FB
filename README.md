# lexikon-panel-new_FB
lexikon panel & backlight driver for CAF kernel

What works:
- Panel on/off

What doesn't work:
- Incremental backlight adjustment (sometimes device reboot)
This behavior can be replicated when fb overlay is updating while backlight is being adjusted.
Possibly can be fixed by porting overlay_semaphore_lock/unlock from htc mddi/msm_fb driver to prevent writing register values and setting overlay at the same time.
