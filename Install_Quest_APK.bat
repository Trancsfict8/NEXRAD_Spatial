@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   Nexrad Spatial - Meta Quest 3 Installer / Launcher
echo =======================================================

set "ANDROID_HOME=C:\Users\Troy\AppData\Local\Android\Sdk"
set "PATH=%ANDROID_HOME%\platform-tools;%PATH%"

where adb.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] adb.exe could not be found!
    echo Looked in: %ANDROID_HOME%\platform-tools
    pause
    exit /b 1
)

echo [1/3] Checking for connected Meta Quest headset...
adb devices
echo.

set "APK="
rem Look for newest APK in Shipping first, then Development
if exist "O:\Nexrad Spatial\Packaged\Android_Shipping\*.apk" (
    for %%F in ("O:\Nexrad Spatial\Packaged\Android_Shipping\*.apk") do (
        set "APK=%%F"
    )
) else if exist "O:\Nexrad Spatial\Packaged\Android\*.apk" (
    for %%F in ("O:\Nexrad Spatial\Packaged\Android\*.apk") do (
        set "APK=%%F"
    )
)

if "!APK!" == "" (
    echo [ERROR] No APK found in Packaged\Android or Packaged\Android_Shipping!
    echo Please run Build_Quest_APK.bat first to generate the APK.
    echo.
    pause
    exit /b 1
)

echo Found APK: !APK!
echo.
echo [2/3] Installing onto Quest 3 (replacing existing and granting permissions)...
adb install -r -d -g "!APK!"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Installation failed!
    echo Please check:
    echo   1. Quest 3 is connected via USB-C cable.
    echo   2. Developer Mode is toggled ON in the Meta Quest mobile app.
    echo   3. Inside the Quest headset, you accepted the "Allow USB Debugging" prompt.
    echo.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Launching Nexrad Spatial on Quest 3...
adb shell am start -n com.NexradSpatial.NexradSpatial/com.epicgames.unreal.GameActivity

echo.
echo =======================================================
echo [SUCCESS] Nexrad Spatial is installed and running!
echo Put on your Quest 3 to play.
echo =======================================================
pause
