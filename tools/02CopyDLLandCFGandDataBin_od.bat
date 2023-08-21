mkdir ..\bin

copy ..\lib\x86\OgreMain.dll ..\bin
copy ..\lib\x86\OgreOverlay.dll ..\bin
copy ..\lib\x86\OgrePaging.dll ..\bin
copy ..\lib\x86\OgreRTShaderSystem.dll ..\bin
copy ..\lib\x86\OgreTerrain.dll ..\bin
copy ..\lib\x86\OgreVolume.dll ..\bin
copy ..\lib\x86\Plugin_BSPSceneManager.dll ..\bin
copy ..\lib\x86\Plugin_CgProgramManager.dll ..\bin
copy ..\lib\x86\Plugin_OctreeSceneManager.dll ..\bin
copy ..\lib\x86\Plugin_OctreeZone.dll ..\bin
copy ..\lib\x86\Plugin_ParticleFX.dll ..\bin
copy ..\lib\x86\Plugin_PCZSceneManager.dll ..\bin
copy ..\lib\x86\RenderSystem_Direct3D9.dll ..\bin
copy ..\lib\x86\RenderSystem_Direct3D11.dll ..\bin
copy ..\lib\x86\RenderSystem_GL.dll ..\bin
copy ..\lib\x86\OIS.dll ..\bin

copy ..\libCG\x86\cg.dll ..\bin
copy ..\SFML-2.4.2\bin\openal32.dll ..\bin
copy ..\SFML-2.4.2\bin\sfml-audio-2.dll ..\bin
copy ..\SFML-2.4.2\bin\sfml-graphics-2.dll ..\bin
copy ..\SFML-2.4.2\bin\sfml-network-2.dll ..\bin
copy ..\SFML-2.4.2\bin\sfml-system-2.dll ..\bin
copy ..\SFML-2.4.2\bin\sfml-window-2.dll ..\bin

copy ..\OpenDungeonsData\plugins.cfg ..\bin
copy ..\OpenDungeonsData\resources.cfg ..\bin

xcopy  ..\OpenDungeonsData\config ..\bin\config /e /i /h /y
xcopy  ..\OpenDungeonsData\gui ..\bin\gui  /e /i /h /y
xcopy  ..\OpenDungeonsData\levels ..\bin\levels /e /i /h /y
xcopy  ..\OpenDungeonsData\materials ..\bin\materials /e /i /h /y
xcopy  ..\OpenDungeonsData\models ..\bin\models /e /i /h /y
xcopy  ..\OpenDungeonsData\music ..\bin\music /e /i /h /y
xcopy  ..\OpenDungeonsData\particles ..\bin\particles /e /i /h /y
xcopy  ..\OpenDungeonsData\sounds ..\bin\sounds /e /i /h /y

copy ..\Release\OpenDungeons.exe ..\bin