setlocal
set lookup_deep=2

@for /L %%i in (1,1,4) do (
@mkdir spawn\%%i
cd spawn\%%i
start ..\..\f5_solve.bat
cd ..\..
)

endlocal