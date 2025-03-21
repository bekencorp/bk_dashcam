Scooter Project
=======================

:link_to_translation:`zh_CN:[中文]`

1. Introduction
---------------------------------

This project is a scooter (two-wheeler) engineering effort that uses Wi-Fi navigation and simultaneously saves driving record data.

1.1 Specifications
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	* Hardware Configuration:

		* Core board, **BK7258_QFN88_9X9_V3.2**
        * Display adapter board, **BK7258_LCD_interface_V3.0**
        * MIC small board, **BK_Module_Microphone_V1.1**
        * SPEAKER small board, **BK_Module_Speaker_V1.1**
        * PSRAM 8M/16M
        * Support, UVC

1.2 Paths
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	Project Path: ``<bk_dashcam source code path>/project/scooter``

	Build Command: ``make bk7258 PROJECT=scooter``

2. Framework Diagram
---------------------------------

2.1 Software Module Architecture Diagram
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,


    The software module architecture is as shown in Figure 1.

.. figure:: ../../../_static/scooter_arch.png
    :align: center
    :alt: module architecture Overview
    :figclass: align-center

    Figure 1. software module architecture

..

    * In the scooter solution, the APP sends navigation images and navigation voice to the device for navigation.
    * In the scooter solution, the UVC camera captures data, which is then encoded in H264 and stored on an SD card for driving records.


2.2 Code Module Relationship Diagram
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

    The interfaces for the multimedia used in this solution are all defined in **media_app.h** .

.. figure:: ../../../_static/scooter_framework.png
    :align: center
    :alt: relationship diagram Overview
    :figclass: align-center

    Figure 3. module relationship diagram


3. Demonstration Description
---------------------------------

The navigation function and driving record function can be used independently in this solution.

3.1 Navigation Function
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

To enable the navigation function, follow these three steps. The board can use either AP mode or STA mode based on your needs.

AP Mode::

    Step 1: Enter the command to turn on the board's Wi-Fi. The command is "test ap".
    Step 2: Connect your phone to the board's Wi-Fi. The SSID is "bicycle", and the password is "12345678".
    Step 3: Open the corresponding scooter APP on your phone and cast the navigation image to the device.

STA Mode::

    Step 1: Turn on your phone's hotspot. Set the hotspot name to "bicycle" and the password to "12345678".
    Step 2: Enter the command to turn on the board's Wi-Fi, which will automatically connect to "bicycle". The command is "test sta", and it will automatically connect to Wi-Fi "bicycle".
    Step 3: Open the corresponding scooter APP on your phone and cast the navigation image to the device.

3.2 Driving Record Function
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

Before enabling the driving record function, ensure the SD card is readable.

Use the following two commands to test::

    Step 1: Enter the command to mount the SD card. The command is "vfs mount fatfs /".
    Step 2: Enter the command to scan all files on the SD card. The command is "vfs scan /".
    Step 3: Enter the command to unmount the file directory on the SD card. The command is "vfs umount /".


If no "fail" logs appear and the file names on the SD card are displayed, it indicates the SD card is readable.


Enabling the Driving Record Function::

    Step 1: Enter the command to open the UVC camera. The command is "media uvc open 1280X720".
    Step 2: Enter the command to start H264 encoding. The command is "media pipeline h264_open".
    Step 3: Enter the command to start automatic recording. The command is "media save_auto 15 10000".
        10000 represents that data is stored in a file every 10,000 milliseconds.
        15 indicates that the files cycle every 15 files.
        Files are saved in the root directory by default.
        File names default to auto_0.h264, auto_1.h264, …, auto_14.h264.

Disabling the Driving Record Function::

    Enter the command to stop automatic recording. The command is "media save_stop 1".

Closing the UVC Camera and H264 Video Encoding::

    Step 1: Enter the command to close the H264 module. The command is "media pipeline h264_close".
    Step 2: Enter the command to close the UVC camera. The command is "media uvc close".
