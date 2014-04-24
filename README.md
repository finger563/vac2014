This is the git repository for the Vanderbilt Aerospace Club's
CAMJET Hazard Detection System.  In this repository is all of
the necessary code for getting the hazard detection
system up and running. 

In order to get the hazard detection system up and running,
the following steps must be followed:

FOR THE RASPBERRY PI:
---------------------
* You will need a usb keyboard, HDMI cable, Monitor, and (possibly) an adapter from hdmi to dvi/vga
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
* run the hazard detection code

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
        make run

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
The commands you need to know for composing images (putting one image next to another) and 
stitching images into a video are these two commands:

	convert <LEFT IMAGE> <RIGHT IMAGE> +append <OUTPUT IMAGE>
    ffmpeg -r <FRAMERATE> -i %05d.ppm -vcodec qtrle test.mov
	
Note: You can use +append for horizontal placement or -append for vertical placement.

* Step By Step Instructions
	* On your windows machine in Cygwin:
	
			mkdir images
			scp pi@10.1.1.2:~/share/vac2014/bin/img* images/.
			scp images/* <YOUR USERNAME>@<YOUR LINUX VM's IP ADDRESS>:~/.
	
	* On your linux VM: (`ssh <YOUR USERNAME>@<YOUR LINUX VM's IP ADDRESS>`)

			cd ~
			mkdir images
			mv img* images/.
			cd images
			mkdir odd
			mkdir even
			mkdir composite
			mv *[13579].ppm odd
			mv *[02468].ppm even
			cd odd; num=0;for file in *.ppm; do mv "$file" "$(printf "%05u" $num).ppm"; let num=num+1; done; cd ..
			cd even; num=0;for file in *.ppm; do mv "$file" "$(printf "%05u" $num).ppm"; let num=num+1; done; cd ..	
			cd odd; num=0;for file in *.ppm; do convert "$file" "../even/$file" +append "$(printf "../composite/%05u" $num).ppm"; let num=num+1; done; cd ..
			cd composite; ffmpeg -r 10 -i %05d.ppm -vcodec qtrle test.mov; cd ..

TO USE YOUR LAPTOP'S SD CARD READER TO READ THE CAMJET CARD IN A VM:
--------------------------------------------------------------------
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
