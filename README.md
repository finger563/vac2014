This is the git repository for the Vanderbilt Aerospace Club's
CAMJET Hazard Detection System.  In this repository is all of
the necessary code for getting the hazard detection
system up and running. 

NOTE: The camjet runs on a Raspberry Pi Model A (for power and space reasons), but the setup of 
the SD card and the code is more easily done on a Model B.

TO RUN THE CAMJET HAZARD DETECTION SOFTWARE:
----------------------------------------
Having set up the computer with the WiFi adapter and the vacrpinet network, open cygwin and ping 10.1.1.1 and 10.1.1.2.  If both of those respond, the camjet is up and running and you can connect to it.  You then simply run the groundStation executable (or open the groundStation VS2012 project and run it through VS).  

In order to get the hazard detection system up and running,
the following steps must be followed:

SETUP FOR THE RASPBERRY PI:
--------------------------
* You will need a Raspberry Pi Model A (for executing the code)
* You will need a Raspberry Pi Model B (for setting up the SD card and compiling the code)
* You will need a usb keyboard, HDMI cable, Monitor, ethernet cable, and (possibly) an adapter from hdmi to dvi/vga
  * After you set up the wlan on the windows machine and ssh on the RPi, you no longer need these
  * To SSH from windows to RPI install cygwin and (in its installer) install SSH
  * ssh command : "ssh pi@10.1.1.2"
* Make an image of Raspbian for the raspberry pi onto the SD card
* boot the raspberry pi with the Raspbian SD card
* run "sudo passwd pi" and give the new password as vac2013
* run "sudo date --set="13 MAR 2014 18:57:00"" with whatever the correct date/time is
* run "sudo raspi-config" and enable
    * camera
    * US keyboard (instead of UK)
    * SSH
* create a file: "sudo nano /etc/modprobe.d/8192cu.conf"
* add the lines to turn off the power saving features of the wireless dongle:

        #Disable power saving
        options 8192cu rtw_power_mgnt=0 rtw_enusbss=1 rtw_ips_mode=1

* perform the following file copies/moves to configure wireless autoreconnect:

        #Make a backup of the original file
        sudo mv /etc/ifplugd/action.d/ifupdown /etc/ifplugd/action.d/ifupdown.original
        #Copy better version to use now
        sudo cp /etc/wpa_supplicant/ifupdown.sh /etc/ifplugd/action.d/ifupdown

* change directory to HOME
* download openframeworks
* unzip the openFrameworks archive to home folder
* rename the openFrameworks folder
* fix error in openFrameworks
* install the openframeworks dependencies
* get the vac git repository
* switch to the feature-raw-images branch
* set the openFrameworks root environment variable
* remove unnecessary ground station code
* build the openframeworks libraries and the hazard code

        cd ~
        wget http://www.openframeworks.cc/versions/v0.8.0/of_v0.8.0_linuxarmv6l_release.tar.gz
        tar -xvzf of_v0.8.0_linuxarmv6l_release.tar.gz
        mv of_v0.8.0_linuxarmv6l_release openFrameworks
        sed -i 's/VC_IMAGE_TRANSFORM_T/DISPMANX_TRANSFORM_T/g' /home/pi/openFrameworks/libs/openFrameworks/app/ofAppEGLWindow.cpp
        sudo ./openFrameworks/scripts/linux/debian_armv6l/install_dependencies.sh
        cd ~ && git clone git@github.com:finger563/vac2014
        cd vac2014 && git checkout feature-raw-images
        export OF_ROOT=/home/pi/openFrameworks
        rm -rf ground_station*
        make

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

* edit /home/pi/.bashrc to add the following at the end (so that you don't have to type it every time):

	export OF_ROOT=/home/pi/openFrameworks
	
* edit /etc/rc.local (as root) to tell the camjet to run the hazard detection software when it boots:

	sudo emacs -nw /etc/rc.local
	
* add the following line (after the last line commented out with #):

	/home/pi/vac2014/bin/vac2014 &


SETUP FOR THE WINDOWS SYSTEM ON THE GROUND STATION:
--------------------------------------------------
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
* download the openframeworks for windows/VS2012 archive ( http://www.openframeworks.cc/versions/v0.8.0/of_v0.8.0_vs_release.zip )
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
To create the videos, we must do a few things:

* sort the images to separate the odd numbered images (raw) from the even numbered images (hazards)
* rename the sorted images
* create videos for each set of images, using the preserved timestamps to set their presentation time
* create a composite video of the two videos side-by-side, using the preserved image timing for presentation
* Step By Step Instructions:
	* Connect a USB SD card reader to your computer
    * Plug in the SD card from the Pi in to the SD card reader
    * Start your virtual machine
    * In VMWare or VirtualBox, pass the USB card reader through to your Virtual Machine
	* On your linux VM: 

            ls /media/

    * The ls command should show two things: (1) boot, and (2) a long string such as af599925-1134-4b6e-8883-fb6a99cd58f1
    * This string is the root folder of your SD card use it in the `<ROOT_FOLDER>` below

            cd ~            
            mkdir images
			cp -p /media/<ROOT_FOLDER>/img* images/.
			cd images
			mkdir odd
			mkdir even
			cp -p *[13579].ppm odd
			cp -p *[02468].ppm even
            rm -f img*
			cd odd; num=0;for file in *.ppm; do cp -p "$file" "$(printf "%05u" $num).ppm"; rm -f "$file"; let num=num+1; done; cd ..
			cd even; num=0;for file in *.ppm; do cp -p "$file" "$(printf "%05u" $num).ppm"; rm -f "$file"; let num=num+1; done; cd ..	
            cd odd; ffmpeg -ts_from_file 2 -t <LENGTH_OF_VIDEO_IN_SECONDS> -i %05d.ppm -vf hflip -vcodec qtrle ../raw.mov; cd ..
            cd even; ffmpeg -ts_from_file 2 -t <LENGTH_OF_VIDEO_IN_SECONDS> -i %05d.ppm -vf hflip -vcodec qtrle ../haz.mov; cd ..
            ffmpeg -i raw.mov -i haz.mov -filter_complex "[0:v:0]pad=iw*2:ih[bg]; [bg][1:v:0]overlay=w" composite_compressed.mov
            ffmpeg -i raw.mov -i haz.mov -filter_complex "[0:v:0]pad=iw*2:ih[bg]; [bg][1:v:0]overlay=w" -vcodec qtrle composite.mov

TO USE YOUR LAPTOP'S SD CARD READER TO READ THE CAMJET CARD IN A VM (VERY SLOW):
--------------------------------------------------------------------------------
* On your windows machine, plug in the SD card which has the camjet data you want to extract.
* Open a command prompt with administrator privileges and run this command:

		wmic diskdrive list brief
	
* Look for the device ID of the SDHC card, which should be something like this:

		\\.\PHYSICALDRIVE1
	
* Run this command, which will create a vmdk disk image on the desktop for you:

		"C:\Program Files\Oracle\VirtualBox\VBoxManage" internalcommands createrawvmdk -filename "%USERPROFILE%/Desktop/sdcard.vmdk" -rawdisk "\\.\PHYSICALDRIVE1"
	
* Ensure the guest VM is not running.
* Ensure VirtualBox is not running
* Start VirtualBox by right-clicking on it and choosing "Run as administrator"
* Open the settings area for the guest VM
* Click on "Storage" in the toolbar
* Next to the controller click on the icon to "Add Hard Disk"
* Select "Choose existing disk"
* Navigate to the /path/to/file.vmdk you used in step 3 and select it
* You should now be returned to the Storage tab and see your file.vmdk in the list.
* Start the VM
* Depending on whether you have a GUI or not the SD card may or may not automatically mount. If you need to mount it manually it is simply exposed as another standard block device, so on my guess this was exposed as /dev/sdb.
* If you want to copy the images from the SD card to the vm, you can use the linux copy command (cp), but you must preserve the timestamps of the images:

		cp -p /src /dest
