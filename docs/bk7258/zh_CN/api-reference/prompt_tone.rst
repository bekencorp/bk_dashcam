提示音开发指南
=============================

:link_to_translation:`en:[English]`

.. important::

    提示音播放功能是基于 ``mp3_play`` 组件和 ``audio_play`` 组件开发实现的，用户只需要调用 ``mp3_play`` 组件的API接口即可实现mp3格式提示音的播放。
    目前 ``mp3_play`` 组件仅支持mp3格式提示音播放，但是组件是基于模块化设计，将解码模块抽象出一个类，客户如需播放其他格式提示音，可基于此类进行适配。


1. API说明
-----------------------------------------

    源码路径： ``<source code>/bk_avdk/components/mp3_play/``

    相关API接口如下：

    - mp3_play_create          //创建mp3提示音播放器
    - mp3_play_destroy         //注销mp3提示音播放器
    - mp3_play_open            //打开mp3提示音播放器，开始播放
    - mp3_play_close           //关闭mp3提示音播放器，停止播放
    - mp3_play_write_data      //写mp3数据到播放器

    详细的API接口使用说明请参考 ``mp3_play.h`` 中的API接口说明。

.. important::

    ``audio_play`` 组件支持注册播放完成的回调函数，通过此回调函数即可拿到播放完成的状态。 ``mp3_play_create`` 接口的参数包含此回调函数的设置。


2. 使用示例
------------------------------------------

    由于 ``mp3_play`` 组件依赖 ``audio_play`` 组件，所以使用时需要打开宏 ``CONFIG_MP3_PLAY=y`` 和 ``CONFIG_AUDIO_PLAY=y``。
    提示音播放组件可工作在任意cpu核上，在同一个核上打开上述宏即可。

    
    ``mp3_play`` 组件提供了API接口使用的参考demo，支持从sdcard中读取mp3格式的提示音播放。

    demo路径：``<source code>/bk_avdk/components/mp3_play/src/mp3_play_test.c``

