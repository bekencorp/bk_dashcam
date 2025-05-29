Prompt Tone Development Guide
======================================

:link_to_translation:`zh_CN:[中文]`

.. important::

    The prompt tone playback feature is developed and implemented based on the ``mp3_play`` component and the ``audio_play`` component. Users only need to call the API interface of the ``mp3_play`` component to play MP3 format prompt tones.
    Currently, the ``mp3_play`` component only supports MP3 format prompt tone playback. However, the component is designed modularly, abstracting the decoding module into a class. If customers need to play prompt tones in other formats, they can adapt based on this class.


1. API Description
-----------------------------------------

    Source code path: ``<source code>/bk_avdk/components/mp3_play/``

    The relevant API interfaces are as follows:

    - mp3_play_create          // Create an MP3 prompt tone player
    - mp3_play_destroy         // Destroy the MP3 prompt tone player
    - mp3_play_open            // Open the MP3 prompt tone player and start playback
    - mp3_play_close           // Close the MP3 prompt tone player and stop playback
    - mp3_play_write_data      // Write MP3 data to the player

    For detailed API interface usage instructions, please refer to the API interface description in ``mp3_play.h``.

.. important::

    The ``audio_play`` component supports registering a callback function for playback completion. Through this callback function, the playback completion status can be obtained. The parameters of the ``mp3_play_create`` interface include the setting of this callback function.


2. Usage Example
------------------------------------------

    Since the ``mp3_play`` component depends on the ``audio_play`` component, the macros ``CONFIG_MP3_PLAY=y`` and ``CONFIG_AUDIO_PLAY=y`` must be enabled during use.
    The prompt tone playback component can work on any CPU core. Simply enable the above macros on the same core.

    
    The ``mp3_play`` component provides a reference demo for API interface usage, supporting the playback of MP3 format prompt tones read from an SD card.

    Demo path: ``<source code>/bk_avdk/components/mp3_play/src/mp3_play_test.c``