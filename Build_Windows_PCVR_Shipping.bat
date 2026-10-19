@echo off
setlocal

echo =======================================================
echo   Nexrad Spatial - Windows PCVR Packaging (Shipping)
echo =======================================================

set "UE_ROOT=O:\UE_5.7"
set "PROJECT=O:\Nexrad Spatial\NexradSpatial.uproject"
set "OUTPUT_DIR=O:\Nexrad Spatial\Packaged\Windows"

echo Building, Cooking, and Packaging Windows PCVR (Shipping)...
echo Output Directory: %OUTPUT_DIR%
echo.

call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="%PROJECT%" BuildCookRun ^
  -project="%PROJECT%" ^
  -noP4 ^
  -clientconfig=Shipping ^
  -platform=Win64 ^
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
echo [SUCCESS] Windows PCVR Package built successfully!
echo Files generated in: %OUTPUT_DIR%
echo =======================================================
