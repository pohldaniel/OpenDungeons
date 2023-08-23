copy ..\lib\x86\OgreMain.dll ..\OpenDungeons
copy ..\lib\x86\OgreOverlay.dll ..\OpenDungeons
copy ..\lib\x86\OgrePaging.dll ..\OpenDungeons
copy ..\lib\x86\OgreRTShaderSystem.dll ..\OpenDungeons
copy ..\lib\x86\OgreTerrain.dll ..\OpenDungeons
copy ..\lib\x86\OgreVolume.dll ..\OpenDungeons
copy ..\lib\x86\Plugin_BSPSceneManager.dll ..\OpenDungeons
copy ..\lib\x86\Plugin_CgProgramManager.dll ..\OpenDungeons
copy ..\lib\x86\Plugin_OctreeSceneManager.dll ..\OpenDungeons
copy ..\lib\x86\Plugin_OctreeZone.dll ..\OpenDungeons
copy ..\lib\x86\Plugin_ParticleFX.dll ..\OpenDungeons
copy ..\lib\x86\Plugin_PCZSceneManager.dll ..\OpenDungeons
copy ..\lib\x86\RenderSystem_Direct3D9.dll ..\OpenDungeons
copy ..\lib\x86\RenderSystem_Direct3D11.dll ..\OpenDungeons
copy ..\lib\x86\RenderSystem_GL.dll ..\OpenDungeons
copy ..\lib\x86\OIS.dll ..\OpenDungeons
copy ..\lib\x86\CEGUIBase.dll ..\OpenDungeons
copy ..\lib\x86\CEGUICoreWindowRendererSet.dll ..\OpenDungeons
copy ..\lib\x86\CEGUIExpatParser.dll ..\OpenDungeons
copy ..\lib\x86\CEGUIOgreRenderer.dll ..\OpenDungeons

copy ..\libCG\x86\cg.dll ..\OpenDungeons
copy ..\SFML-2.4.2\bin\openal32.dll ..\OpenDungeons
copy ..\SFML-2.4.2\bin\sfml-audio-2.dll ..\OpenDungeons
copy ..\SFML-2.4.2\bin\sfml-graphics-2.dll ..\OpenDungeons
copy ..\SFML-2.4.2\bin\sfml-network-2.dll ..\OpenDungeons
copy ..\SFML-2.4.2\bin\sfml-system-2.dll ..\OpenDungeons
copy ..\SFML-2.4.2\bin\sfml-window-2.dll ..\OpenDungeons

copy ..\OpenDungeonsData\plugins.cfg ..\OpenDungeons
copy ..\OpenDungeonsData\resources.cfg ..\OpenDungeons

xcopy  ..\OpenDungeonsData\config ..\OpenDungeons\config /e /i /h /y
xcopy  ..\OpenDungeonsData\gui ..\OpenDungeons\gui  /e /i /h /y
xcopy  ..\OpenDungeonsData\levels ..\OpenDungeons\levels /e /i /h /y
xcopy  ..\OpenDungeonsData\materials ..\OpenDungeons\materials /e /i /h /y
xcopy  ..\OpenDungeonsData\models ..\OpenDungeons\models /e /i /h /y
xcopy  ..\OpenDungeonsData\music ..\OpenDungeons\music /e /i /h /y
xcopy  ..\OpenDungeonsData\particles ..\OpenDungeons\particles /e /i /h /y
xcopy  ..\OpenDungeonsData\sounds ..\OpenDungeons\sounds /e /i /h /y