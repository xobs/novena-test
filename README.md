Novena Test
===========

This is the testing framework for the Novena laptop.  It is originally
based on the Kovan testing framework.

Tests are arranged as a series of subclasses of the NovenaTest class.  Each
test must implement the *void runTest(void)* method, and should emit a
testStateUpdated(int level, int value, QString \*message) signal every time
the test state is updated.

Creating new tests
------------------

A minimal test would look like:

     #include "novenatest.h"
     class SimpleTest : public NovenaTest
     {
         Q_OBJECT
         public:
             SimpleTest() { }
             void runTest() {
                 emit testStateUpdated(TEST_INFO, 0, new QString("Test completed"));
             }
    };

You would then add this test to novenatestengine.cpp in the loadAllTests()
method by adding the following:

    tests.append(new SimpleTest());

When your runTest thread exits, if it hasn't emitted a TEST_ERROR state, then
the test is assumed to have completed successfully.


Current tests
-------------

The following tests have been defined.  Note that tests in italics have not yet been written:

* _Power logger_ (Logs current draw from Senoko)
* SATA prober (Ensures a SATA drive is attached)
* MMC checker (Ensures internal MMC is in place)
* Firmware loader (Loads firmware onto internal MMC, in the background)
* HDMI tester (Puts patterns on the screen)
* USB tester (Loopback?)
* PCIe tester (enumeration?  Ethernet loopback?)
* EEPROM tester (Verify EEPROM exists, update it if necessary)
* String printer (Useful for printing status messages)


USB test image for bare board
-----------------------------

While the base system software runs off the external MMC card, the actual
factory test program, along with the image to be burned, all reside on
a separate USB drive.  This drive MUST contain a valid partition table, and
MUST have the first partition be VFAT-formatted.  The disk MUST contain
the following files:

* novena-test: The program built here, with NOVENA_BAREBOARD defined in novenatestengine.cpp
* gpbb_fpga.bit: Available in the files/ directory
* novena_fpga-1.22.bit: Available in the files/ directory
* runtest.sh: Available in the files/ directory
* sounds/: A directory containing .wav files.  Suggested to be pentatonic.
* networkmanager-config: A NetworkManager keyfile configuration for the wifi access point used in the factory


USB test image for desktop configuration
----------------------------------------

* novena-test: The program built here, with NOVENA_DESKTOP defined in novenatestengine.cpp
* runtest.sh: Available in the files/ directory
* sounds/: A directory containing .wav files.  Suggested to be pentatonic.
* senoko.hex: Senoko configuration file, output from ChibiOS-3 build
* stm32flash: Executable binary output from stm32flash repo
* debian packages: The .deb files referenced in the PackageInstaller() test in novenatestengine.cpp

Optionally, you can add this file:

* networkmanager-config: A NetworkManager keyfile configuration for the wifi access point used in the factory


USB test image for laptop configuration
----------------------------------------

* novena-test: The program built here, with NOVENA_LAPTOP defined in novenatestengine.cpp
* runtest.sh: Available in the files/ directory
* sounds/: A directory containing .wav files.  Suggested to be pentatonic.
* senoko.hex: Senoko configuration file, output from ChibiOS-3 build
* stm32flash: Executable binary output from stm32flash repo
* debian packages: The .deb files referenced in the PackageInstaller() test in novenatestengine.cpp

Optionally, you can add this file:

* networkmanager-config: A NetworkManager keyfile configuration for the wifi access point used in the factory

Constructing a Factory Test image
---------------------------------

Put the factory test on its own root filesystem image, and change the disk ID
to 0x4e6f7646 ("NovF"):

    fdisk /dev/sdb
    x
    i
    0x4e6f7646
    r
    w

Then create a uEnv.txt file in the root, containing the following:

    finalhook=setenv rec factory ; setenv bootargs init=/lib/systemd/systemd rootwait rw root=PARTUUID=4e6f7646-03 console=tty0

Also, create a novena-factory-test.service file:

    [Unit]
    Description=Novena Factory Image
    DefaultDependencies=no
    After=dbus.service network-pre.target systemd-sysusers.service
    Before=systemd-network.target network.target multi-user.target shutdown.target graphical.target systemd-user-sessions.service novena-firstrun.service

    [Service]
    Type=oneshot
    RemainAfterExit=yes
    Restart=no
    ExecStart=/usr/bin/startx xfce4-terminal
    User=root

    [Install]
    WantedBy=multi-user.target

Modify /etc/fstab on the target system.  Replace all instances of
platform-2198000.usdhc with platform-2194000.usdhc and add a line to mount
the installation media, and create /factory:

    /dev/disk/by-path/platform-ci_hdrc.1-usb-0:1.4.1:1.0-scsi-0:0:0:0-part1 /factory vfat defaults 2 2

Copy the wifi configuration file to /etc/NetworkManager/system-connections/ and ensure it's chmod 0600 owned by root.

Finally, load the systemd script by running "systemctl enable novena-factory-test".

Something like this:

    #!/bin/sh
    root=/mnt
    sudo cp novena-factory-test.service "${root}/lib/systemd/system/"
    sudo chroot "${root}" systemctl enable novena-factory-test
    sudo sed -i 's/2198000/2194000/g' "${root}/etc/fstab"
    echo '/dev/disk/by-path/platform-ci_hdrc.1-usb-0:1.4.1:1.0-scsi-0:0:0:0-part1 /factory vfat defaults 2 2' >> "${root}/etc/fstab"
    sudo mkdir "${root}/factory"
    sudo cp wifi_config "${root}/etc/NetworkManager/system-connections/"
    sudo chmod 0600 "${root}/etc/NetworkManager/system-connections/wifi_config"
    sudo chown root:root "${root}/etc/NetworkManager/system-connections/wifi_config"
