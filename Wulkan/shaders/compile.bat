@ECHO OFF

setlocal enabledelayedexpansion

for /f "tokens=2 delims=:" %%a in ('mode con ^| find "Columns"') do set "width=%%a"
set "width=%width: =%"

set "line="
for /l %%i in (1,1,%width%) do set "line=!line!-"

:: Optimized version
set GLSLO=glslc.exe %%i -O --target-env=vulkan1.3
:: With debug info
set GLSLD=glslc.exe %%i -g --target-env=vulkan1.3

:: Slang Optimized
set SLANGO=slangc %%i -target spirv -O3
:: With debug info
set SLANGD=slangc %%i -target spirv -g

echo !line!

echo Vertex shaders: 
echo.
for /r %%i in (*.vert) do (
    echo Compiling %%i into %%~ni_vert.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_vert.spv
    %GLSLD% -o %%~dpi%%~ni_dvert.spv
)

echo !line!

echo Fragment shaders:
echo.
for /r %%i in (*.frag) do (
    echo Compiling %%i into %%~ni_frag.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_frag.spv
    %GLSLD% -o %%~dpi%%~ni_dfrag.spv
)

echo !line!

echo Tesselation control shaders: 
echo.
for /r %%i in (*.tesc) do (
    echo Compiling %%i into %%~ni_tesc.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_tesc.spv
    %GLSLD% -o %%~dpi%%~ni_dtesc.spv
)

echo !line!

echo Tesselation evaluation shaders:
echo.
for /r %%i in (*.tese) do (
    echo Compiling %%i into %%~ni_tese.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_tese.spv
    %GLSLD% -o %%~dpi%%~ni_dtese.spv
)


echo !line!

echo Compute shaders: 
echo.
for /r %%i in (*.comp) do (
    echo Compiling %%i into %%~ni_comp.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_comp.spv
    %GLSLD% -o %%~dpi%%~ni_dcomp.spv
)

echo !line!

echo Slang shaders: 
echo.
for /r %%i in (*.slang) do (
    echo Compiling %%i into %%~ni_slang.spv
    echo(

    %SLANGO% -o %%~dpi%%~ni_slang.spv
    %SLANGD% -o %%~dpi%%~ni_dslang.spv
)

pause