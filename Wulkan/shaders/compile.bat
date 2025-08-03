@ECHO OFF

for /r %%i in (*.vert) do (
    echo "Compiling %%i into %%~ni_vert.spv"
    glslc.exe %%i -O -o %%~dpi%%~ni_vert.spv
)

for /r %%i in (*.frag) do (
    echo "Compiling %%i into %%~ni_frag.spv"
    glslc.exe %%i -O -o %%~dpi%%~ni_frag.spv
)

for /r %%i in (*.tese) do (
    echo "Compiling %%i into %%~ni_tese.spv"
    glslc.exe %%i -O -o %%~dpi%%~ni_tese.spv
)

for /r %%i in (*.tesc) do (
    echo "Compiling %%i into %%~ni_tesc.spv"
    glslc.exe %%i -O -o %%~dpi%%~ni_tesc.spv
)

for /r %%i in (*.comp) do (
    echo "Compiling %%i into %%~ni_comp.spv"
    glslc.exe %%i -O -o %%~dpi%%~ni_comp.spv
)

pause