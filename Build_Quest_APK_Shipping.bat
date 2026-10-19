@echo off
setlocal

echo =======================================================
echo   Nexrad Spatial - Meta Quest 3 Standalone APK Builder
echo   (Shipping / Distribution Optimized Build)
echo =======================================================

rem Set Android environment variables
set "ANDROID_HOME=C:\Users\Troy\AppData\Local\Android\Sdk"
set "NDKROOT=C:\Users\Troy\AppData\Local\Android\Sdk\ndk\27.2.12479018"
set "NDK_ROOT=%NDKROOT%"
set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
set "PATH=%ANDROID_HOME%\platform-tools;%PATH%"

set "UE_ROOT=O:\UE_5.7"
set "PROJECT=O:\Nexrad Spatial\NexradSpatial.uproject"
set "OUTPUT_DIR=O:\Nexrad Spatial\Packaged\Android_Shipping"

rem Verify UE Android target platform binaries exist
if not exist "%UE_ROOT%\Engine\Binaries\Android\UnrealGame-Android-Shipping.target" (
    echo.
    echo [ERROR] Unreal Engine 5.7 Android Target Platform component is not yet downloaded!
    echo.
    echo Quick 2-click fix in Epic Games Launcher:
    echo   1. Open Epic Games Launcher.
    echo   2. Go to 'Unreal Engine' -^> 'Library'.
    echo   3. Next to 'Unreal Engine 5.7', click the dropdown arrow beside 'Launch' -^> select 'Options'.
    echo   4. Check the box for 'Android' and click 'Apply'.
    echo.
    echo Once the download finishes, run this script again!
    echo.
    pause
    exit /b 1
)

echo [1/2] Building, Cooking, and Packaging Shipping APK for Meta Quest 3...
echo Target: Android_ASTC (Vulkan, ARM64, Shipping Optimized)
echo Output Directory: %OUTPUT_DIR%
echo.

call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="%PROJECT%" BuildCookRun ^
  -project="%PROJECT%" ^
  -noP4 ^
  -clientconfig=Shipping ^
  -platform=Android ^
  -cookflavor=ASTC ^
  -build ^
  -cook ^
  -pak ^
  -stage ^
  -package ^
  -archive ^
  -archivedirectory="%OUTPUT_DIR%" ^
  -utf8output

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed! Check the log output above for details.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo =======================================================
echo [SUCCESS] Shipping APK built successfully!
echo Files generated in: %OUTPUT_DIR%
echo Run Install_Quest_APK.bat with your Quest 3 connected to deploy.
echo =======================================================
pause
