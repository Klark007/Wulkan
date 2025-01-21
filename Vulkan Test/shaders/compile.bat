@ECHO OFF

for %%i in (*.vert) do (
    echo "Compiling %%i into %%~ni_vert.spv"
    glslc.exe %%i -o %%~ni_vert.spv
)

for %%i in (*.frag) do (
    echo "Compiling %%i into %%~ni_frag.spv"
    glslc.exe %%i -o %%~ni_frag.spv
)

for %%i in (*.tese) do (
    echo "Compiling %%i into %%~ni_tese.spv"
    glslc.exe %%i -o %%~ni_tese.spv
)

for %%i in (*.tesc) do (
    echo "Compiling %%i into %%~ni_tesc.spv"
    glslc.exe %%i -o %%~ni_tesc.spv
)

for %%i in (*.comp) do (
    echo "Compiling %%i into %%~ni_comp.spv"
    glslc.exe %%i -o %%~ni_comp.spv
)

pause