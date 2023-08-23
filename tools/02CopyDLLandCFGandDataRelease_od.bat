copy ..\lib\x86\OgreMain.dll ..\Release
copy ..\lib\x86\OgreOverlay.dll ..\Release
copy ..\lib\x86\OgrePaging.dll ..\Release
copy ..\lib\x86\OgreRTShaderSystem.dll ..\Release
copy ..\lib\x86\OgreTerrain.dll ..\Release
copy ..\lib\x86\OgreVolume.dll ..\Release
copy ..\lib\x86\Plugin_BSPSceneManager.dll ..\Release
copy ..\lib\x86\Plugin_CgProgramManager.dll ..\Release
copy ..\lib\x86\Plugin_OctreeSceneManager.dll ..\Release
copy ..\lib\x86\Plugin_OctreeZone.dll ..\Release
copy ..\lib\x86\Plugin_ParticleFX.dll ..\Release
copy ..\lib\x86\Plugin_PCZSceneManager.dll ..\Release
copy ..\lib\x86\RenderSystem_Direct3D9.dll ..\Release
copy ..\lib\x86\RenderSystem_Direct3D11.dll ..\Release
copy ..\lib\x86\RenderSystem_GL.dll ..\Release
copy ..\lib\x86\OIS.dll ..\Release
copy ..\lib\x86\CEGUIBase.dll ..\Release
copy ..\lib\x86\CEGUICoreWindowRendererSet.dll ..\Release
copy ..\lib\x86\CEGUIExpatParser.dll ..\Release
copy ..\lib\x86\CEGUIOgreRenderer.dll ..\Release

copy ..\libCG\x86\cg.dll ..\Release
copy ..\SFML-2.4.2\bin\openal32.dll ..\Release
copy ..\SFML-2.4.2\bin\sfml-audio-2.dll ..\Release
copy ..\SFML-2.4.2\bin\sfml-graphics-2.dll ..\Release
copy ..\SFML-2.4.2\bin\sfml-network-2.dll ..\Release
copy ..\SFML-2.4.2\bin\sfml-system-2.dll ..\Release
copy ..\SFML-2.4.2\bin\sfml-window-2.dll ..\Release

copy ..\OpenDungeonsData\plugins.cfg ..\Release
copy ..\OpenDungeonsData\resources.cfg ..\Release

xcopy  ..\OpenDungeonsData\config ..\Release\config /e /i /h /y
xcopy  ..\OpenDungeonsData\gui ..\Release\gui  /e /i /h /y
xcopy  ..\OpenDungeonsData\levels ..\Release\levels /e /i /h /y
xcopy  ..\OpenDungeonsData\materials ..\Release\materials /e /i /h /y
xcopy  ..\OpenDungeonsData\models ..\Release\models /e /i /h /y
xcopy  ..\OpenDungeonsData\music ..\Release\music /e /i /h /y
xcopy  ..\OpenDungeonsData\particles ..\Release\particles /e /i /h /y
xcopy  ..\OpenDungeonsData\sounds ..\Release\sounds /e /i /h /y