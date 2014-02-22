This is the git repository for the Vanderbilt Aerospace Club's
CAMJET Hazard Detection System.  In this repository is all of
the necessary code for getting the hazard detection
system up and running. 

In order to get the hazard detection system up and running,
the following steps must be followed:

FOR THE RASPBERRY PI:
---------------------
* Make an image of Raspbian for the raspberry pi onto the SD card
* boot the raspberry pi with the Raspbian SD card
* run "sudo passwd pi" and give the new password as vac2013
* run "sudo raspi-config" and enable
  ** camera
  ** US keyboard (instead of UK)
  ** SSH
* edit /etc/network/interfaces to add the following:

    allow-hotplug wlan0
    auto wlan0
    iface wlan0 inet static
    address 10.1.1.2
    netmask 255.255.255.0
    network 10.1.1.0
    broadcast 10.1.1.255
    wpa-ssid vacrpinet
    wpa-psk vacrpi2014

* create a file: "sudo nano /etc/modprobe.d/8192cu.conf"
* add the lines to turn off the power saving features of the wireless dongle:

#Disable power saving
options 8192cu rtw_power_mgnt=0 rtw_enusbss=1 rtw_ips_mode=1

* change directory to HOME: "cd ~"
* download openframeworks: "wget http://www.openframeworks.cc/versions/v0.8.0/of_v0.8.0_linuxarmv6l_release.tar.gz"
* unzip the openFrameworks archive to home folder: "tar -xvzf of_v0.8.0_linuxarmv6l_release.tar.gz"
* rename the openFrameworks folder: "mv of_v0.8.0_linuxarmv6l_release openFrameworks"
* fix error in openFrameworks: "sed -i 's/VC_IMAGE_TRANSFORM_T/DISPMANX_TRANSFORM_T/g' /home/pi/openFrameworks/libs/openFrameworks/app/ofAppEGLWindow.cpp"
* install the openframeworks dependencies: "sudo ./openFrameworks/scripts/linux/debian_armv6l/install_dependencies.sh"
* get the vac git repository: "cd ~ && git clone git@github.com:finger563/vac2014"
* switch to the feature-raw-images branch: "cd vac2014 && git checkout feature-raw-images"
* set the openFrameworks root environment variable: "export OF_ROOT=/home/pi/openFrameworks"
* remove unnecessary ground station code: "rm -rf ground_station*"
* build the openframeworks libraries and the hazard code: "make"
* run the hazard detection code: "make run"

FOR THE WINDOWS SYSTEM ON THE GROUND STATION:
---------------------------------------------
To configure the wireless interface in windows, first plug in the wireless adapter.
You must open a command prompt as an administrator and run the following commands:

    netsh wlan set hostednetwork mode=allow ssid=vacrpinet key=vacrpi2014
    netsh wlan start hostednetwork

You must then configure the new network to have a static ip and subnet mask:

    IP: 10.1.1.1
    MASK: 255.255.255.0

* install git for windows, tortoise git, and MS VS2012
* get the vac git repository (same url as above)
* switch to the feature-raw-images branch
* uncompress the openframeworks archive (of_v0.8.0_vs_release.zip)
* copy vac2014/ground_station/groundStation to openFrameworks/examples/groundStation/. (there should be groundStation inside a groundStation folder)
* open the .sln file in the folder to which you copied
* build it (F6); this builds the openframeworks libraries and the application
* run the ground station (F5)


TO SET UP A CROSS-COMPILER AND USE DISTCC:
------------------------------------------
On a linux machine, if you want to set up a cross compiler you must:

* sudo apt-get install ia32-libs
* download https://github.com/raspberrypi/tools into $RPI_TOOLS
* copy /usr from the pi (or the compressed usr.tar.gz) into $RPI_ROOT
* build example on linux with make -j{N} RPI_TOOLS=$RPI_TOOLS RPI_ROOT=$RPI_ROOT GST_VERSION=0.10 PLATFORM_OS=Linux PLATFORM_ARCH=armv6l

If you want the pi (on same network) to use linux machine to compiile its code, you must use distcc:

LINUX:
* sudo apt-get install mercurial bison flex texinfo automake curl
* sudo apt-get install build-essential libncurses-dev libtool gawk gperf
* sudo apt-get install ia32-libs
* download https://github.com/raspberrypi/tools into $RPI_TOOLS
* sudo apt-get install distcc

* Edit /etc/default/distcc (e.g. sudo vi /etc/default/distcc)

    Change STARTDISTCC="false" to STARTDISTCC="true"
    Change ALLOWEDNETS="127.0.0.1" to include the network IP addresses of your Raspberry Pis
    Note: Addresses use CIDR notation. To allow your localhost AND IP addresses in the range 192.168.1.0-192.168.1.255 use this ALLOWEDNETS="127.0.0.1 192.168.1.0/24.
    Note: If you want help with CIDR notation, you can use the calculator here http://www.subnet-calculator.com/cidr.php.
    Change ZEROCONF="false" to ZEROCONF="true"
    Change LISTENER="127.0.0.1" to LISTENER="" in order to listen for incoming connections all any network interface (not just the localhost/127.0.0.1).
   
* Edit /etc/init.d/distcc (e.g. sudo vi /etc/init.d/distcc)

    Change PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin to PATH=$RPI_TOOLS/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
    Note: $RPI_TOOLS should be the path were you installed the raspberry pi compiler before.

* to start distcc : sudo service distcc {re}start

PI:
* sudo apt-get install distcc
* mkdir $HOME/.distcc/
* Edit $HOME/.distcc/hosts (e.g. vi $HOME/.distcc/hosts) and add the ip(s) of the computers that have the rpi tools installed

* To compile (finally!)

    make -j 6 CXX=/usr/lib/distcc/arm-linux-gnueabihf-g++ CC=/usr/lib/distcc/arm-linux-gnueabihf-gcc

   If you would like to simplify your command, you can set MAKEFLAGS using the following:
   export MAKEFLAGS="-s -j 6 CXX=/usr/lib/distcc/arm-linux-gnueabihf-g++ CC=/usr/lib/distcc/arm-linux-gnueabihf-gcc"
   make


TO ENCODE A SEQUENCE OF .PPM IMAGES (PACKED PIXEL, LOSSLESS) INTO A LOSSLESS AVI:
---------------------------------------------------------------------------------

    ffmpeg -r 25 -b 180000 -i img%04d.ppm test.avi
    ffmpeg -r N -i img%04d.ppm -vcodec qtrle test.mov