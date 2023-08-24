# OpenDungeons 0.7.1

This repository contains a Visual Studio 2015 setup of https://github.com/OpenDungeons/OpenDungeons and it's dependencies. After compiling keep in mind CopyDLLandCFGandDataBinOgre.bat, CopyDLLandCFGandDataBinCEGUI.bat and 02CopyDLLandCFGandDataBin.bat for generating the respective bin folder. It should be clear that the Copyright of OpenDungeons and of it's dependencies are not in my hand.

**Important:**  The build is splitted up in three solutuins opendungeons, Ogre, CEGUI and works only with x86 in release mode. You will at first build the dependencies out of the opendungeons solution and go ahead with the Ogre solutuion and after that with the CEGUI solution. Than you have to setup the libboost folder see below. The last step will be the project OpenDungeons out of the opendungeons solution. Ogre and CEGUI should work in debug and x64 mode. I have never tested it.

Used major libraries <br/>

&emsp;[https://github.com/OGRECave/ogre/releases/tag/v1.10.11](https://github.com/OGRECave/ogre/releases/tag/v1.10.11)<br/>
&emsp;FreeImag 3.18.0 [https://freeimage.sourceforge.io/download.html](https://freeimage.sourceforge.io/download.html)<br/>
&emsp;MyGui 3.4.0 [http://mygui.info](http://mygui.info)<br/>
&emsp;SDL2 2.0.22 [https://www.libsdl.org](https://www.libsdl.org)<br/>
&emsp;[https://www.boost.org/users/history/version_1_66_0.html](https://www.boost.org/users/history/version_1_66_0.html)<br/>

**Installing Boost:** Out of the "VS Native Command Prompt"

Change the Directory to the downloaddirectory of Boost with "cd boost_1_66_0", type

  &emsp;bootstrap.bat

and depending on the choosen "VS Native Command Prompt" the build will be x64 or x86

&emsp;b2 install variant=release runtime-link=shared address-model=32 --prefix=C:\OpenDungeons\libboost (Md build I used this) <br/>
&emsp;b2 install variant=release runtime-link=static address-model=32 --prefix=C:\OpenDungeons\libboost (Mt build) <br/>

&emsp;b2 install variant=debug runtime-link=shared address-model=32 --prefix=C:\OpenDungeons\libboost (Md build) <br/>
&emsp;b2 install variant=debug runtime-link=static address-model=32 --prefix=C:\OpenDungeons\libboost (Mt build) <br/>

Maybe reanaming of the .lib is also necessary, it depends on the used Visual Studio Version. With Visual Studio 2015 the needed libs looks like this.

&emsp;libboost_date_time-vc140-mt-x32-1_66<br/>
&emsp;libboost_filesystem-vc140-mt-x32-1_66<br/>
&emsp;libboost_locale-vc140-mt-x32-1_66<br/>
&emsp;libboost_program_options-vc140-mt-x32-1_66<br/>
&emsp;libboost_system-vc140-mt-x32-1_66.lib<br/>
&emsp;libboost_thread-vc140-mt-x32-1_66.lib<br/>

**Attention:** The DirectX Include path form the Project "RenderSystem_DirectD3D11" depends on the choosen Windows Kit. In my case I used Version 8.1 but I recommand to go with a newer one.<br/>

&emsp;C:\Program Files (x86)\Windows Kits\8.1\Include\um

