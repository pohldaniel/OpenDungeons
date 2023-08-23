mkdir ..\binCEGUI

copy ..\lib\x86\CEGUIBase.dll ..\binCEGUI
copy ..\lib\x86\CEGUICommonDialogs.dll ..\binCEGUI
copy ..\lib\x86\CEGUICoreWindowRendererSet.dll ..\binCEGUI
copy ..\lib\x86\CEGUIDirect3D9Renderer.dll ..\binCEGUI
copy ..\lib\x86\CEGUIDirect3D10Renderer.dll ..\binCEGUI
copy ..\lib\x86\CEGUIOpenGLRenderer.dll ..\binCEGUI
copy ..\lib\x86\CEGUIOgreRenderer.dll ..\binCEGUI
copy ..\lib\x86\CEGUIExpatParser.dll ..\binCEGUI
copy ..\lib\x86\CEGUISILLYImageCodec.dll ..\binCEGUI

copy ..\OgreData\plugins.cfg ..\binCEGUI
copy ..\OgreData\resources.cfg ..\binCEGUI
copy ..\CEGUIData\samples.cfg ..\binCEGUI

copy ..\lib\x86\OgreMain.dll ..\binCEGUI
copy ..\lib\x86\OgreOverlay.dll ..\binCEGUI
copy ..\lib\x86\OgreBites.dll ..\binCEGUI
copy ..\lib\x86\OgrePaging.dll ..\binCEGUI
copy ..\lib\x86\OgreRTShaderSystem.dll ..\binCEGUI
copy ..\lib\x86\OgreTerrain.dll ..\binCEGUI
copy ..\lib\x86\OgreVolume.dll ..\binCEGUI
copy ..\lib\x86\OgreMeshLodGenerator.dll ..\binCEGUI
copy ..\lib\x86\OgreHLMS.dll ..\binCEGUI
copy ..\lib\x86\Plugin_BSPSceneManager.dll ..\binCEGUI
copy ..\lib\x86\Plugin_CgProgramManager.dll ..\binCEGUI
copy ..\lib\x86\Plugin_OctreeSceneManager.dll ..\binCEGUI
copy ..\lib\x86\Plugin_OctreeZone.dll ..\binCEGUI
copy ..\lib\x86\Plugin_ParticleFX.dll ..\binCEGUI
copy ..\lib\x86\Plugin_PCZSceneManager.dll ..\binCEGUI
copy ..\lib\x86\RenderSystem_Direct3D9.dll ..\binCEGUI
copy ..\lib\x86\RenderSystem_Direct3D11.dll ..\binCEGUI
copy ..\lib\x86\RenderSystem_GL.dll ..\binCEGUI
copy ..\libCG\x86\cg.dll ..\binCEGUI
copy ..\lib\x86\OIS.dll ..\binCEGUI

copy ..\lib\x86\CEGUICommonDialogsDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIDemo6.dll ..\binCEGUI
copy ..\lib\x86\CEGUIDragDropDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIEditboxValidationDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIEffectsDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIFontDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIGameMenuDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIHelloWorldDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIHudDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIInventoryDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUILookNFeelOverviewDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIMinesweeper.dll ..\binCEGUI
copy ..\lib\x86\CEGUIScrollablePaneDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUITabControlDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUITextDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUITreeDemo.dll ..\binCEGUI
copy ..\lib\x86\CEGUIWidgetDemo.dll ..\binCEGUI

xcopy  ..\CEGUIData\datafiles ..\binCEGUI\datafiles /e /i /h /y

copy ..\Release\CEGUISampleFramework.exe ..\binCEGUI