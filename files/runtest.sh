#!/bin/sh

xfwm4 &
xfce4-terminal &

# Hack to disable screen blanking
xset s off
xset -dpms
xset s noblank

# Ensure pulseaudio is running *before* novena-test is started
mkdir -p /var/run/pulse
pulseaudio -D

exec /factory/novena-test
