@for /F "usebackq tokens=1,2 delims=#" %%i IN (`findstr /I /C:"define SUPPORT_" %1`) DO @(
  @SET SUPPORT_LINE=%%j
  @SET SUPPORT_LINE=!SUPPORT_LINE:~7!
  @echo !SUPPORT_LINE!
)
@SET SUPPORT_LINE=
