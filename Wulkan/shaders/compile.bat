@ECHO OFF

setlocal enabledelayedexpansion

for /f "tokens=2 delims=:" %%a in ('mode con ^| find "Columns"') do set "width=%%a"
set "width=%width: =%"

set "line="
for /l %%i in (1,1,%width%) do set "line=!line!-"

:: Optimized version
set GLSLO=glslc.exe %%i -O --target-env=vulkan1.3

echo !line!

echo Vertex shaders: 
echo.
for /r %%i in (*.vert) do (
    echo Compiling %%i into %%~ni_vert.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_vert.spv
)

echo !line!

echo Fragment shaders: \n
echo.
for /r %%i in (*.frag) do (
    echo Compiling %%i into %%~ni_frag.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_frag.spv
)

echo !line!

echo Tesselation evaluation shaders: \n
echo.
for /r %%i in (*.tese) do (
    echo Compiling %%i into %%~ni_tese.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_tese.spv
)

echo !line!

echo Tesselation control shaders: \n
echo.
for /r %%i in (*.tesc) do (
    echo Compiling %%i into %%~ni_tesc.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_tesc.spv
)

echo !line!

echo Compute shaders: \n
echo.
for /r %%i in (*.comp) do (
    echo Compiling %%i into %%~ni_comp.spv
    echo(

    %GLSLO% -o %%~dpi%%~ni_comp.spv
)

echo !line!

pause