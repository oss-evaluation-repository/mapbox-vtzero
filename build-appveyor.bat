@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

SET CUSTOM_CMAKE=cmake-3.6.2-win64-x64
::show all available env vars
SET
ECHO cmake on AppVeyor
cmake -version

ECHO activating VS cmd prompt && CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET lodir=%CD%
SET PATH=%lodir%\%CUSTOM_CMAKE%\bin;%PATH%

ECHO our own cmake
cmake -version

CD %lodir%\..

CD %lodir%
IF EXIST build ECHO deleting build dir... && RD /Q /S build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD build
ECHO config^: %config%

::This will produce lots of LNK4099 warnings which can be ignored.
::Unfortunately they can't be disabled, see
::http://stackoverflow.com/questions/661606/visual-c-how-to-disable-specific-linker-warnings
SET CMAKE_CMD=cmake .. ^
-LA -G "Visual Studio 14 Win64" ^
-DCMAKE_BUILD_TYPE=%config%

ECHO calling^: %CMAKE_CMD%
%CMAKE_CMD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET avlogger=
IF /I "%APPVEYOR%"=="True" SET avlogger=/logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

msbuild vtzero.sln ^
/p:Configuration=%config% ^
/toolsversion:14.0 ^
/p:Platform=x64 ^
/p:PlatformToolset=v140 %avlogger%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ctest --output-on-failure ^
-C %config% ^
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


GOTO DONE

:ERROR
ECHO ~~~~~~ ERROR %~f0 ~~~~~~
SET EL=%ERRORLEVEL%

:DONE
IF %EL% NEQ 0 ECHO. && ECHO !!! ERRORLEVEL^: %EL% !!! && ECHO.
ECHO ~~~~~~ DONE %~f0 ~~~~~~

EXIT /b %EL%