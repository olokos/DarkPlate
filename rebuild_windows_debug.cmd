@echo off

cd build

set ARCHITECTURE=x64
rem set BUILD_TYPE=Debug
set BUILD_TYPE=Debug
set VCPKG_PATH=C:\DarknetOpenCV\vcpkg
rem set TRIPLET=x64-windows-static
set TRIPLET=x64-windows


cmake -A %ARCHITECTURE% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%TRIPLET% -DCMAKE_CUDA_COMPILER="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.8/bin" ..
if ERRORLEVEL 1 goto END

msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkPlate.sln
if ERRORLEVEL 1 goto END

msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% PACKAGE.vcxproj
if ERRORLEVEL 1 goto END

:END
cd ..