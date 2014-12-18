#!/bin/sh

# Update the kernel to one where the LVDS is more stable
sum=$(md5sum /boot/zimage | awk '{print $1}')
if [ "x${sum}" = "xde494d3ed9c584df6d84e200527a4af0" ]
then
	cp -f /factory/zimage /boot/zimage
	cp -f /factory/novena.dtb /boot/novena.dtb
	sync
	sleep 1
	sync
	sleep 1
	echo b > /proc/sysrq-trigger
fi

xfwm4 &
xfce4-terminal &

# Hack to disable screen blanking
xset s off
xset -dpms
xset s noblank

# Start up a serial console
/sbin/agetty 115200 ttymxc1 vt102 &

# Ensure pulseaudio is running *before* novena-test is started
mkdir -p /var/run/pulse
pulseaudio -D
sleep 1
echo 'set-default-sink alsa_output.platform-sound.analog-stereo' | pacmd

# Let xfce4-terminal start up
sleep 3

exec /factory/novena-test
