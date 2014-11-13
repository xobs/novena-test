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
sleep 1
echo 'set-default-sink alsa_output.platform-sound.analog-stereo' | pacmd

exec /factory/novena-test
