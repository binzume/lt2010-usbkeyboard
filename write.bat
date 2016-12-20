@echo off
setlocal enabledelayedexpansion

cd /D %~dp1
echo writing %~n1.hex ...

hidspx -ph %~n1.hex
set ERR=%ERRORLEVEL%
IF %ERR% NEQ 0 goto ERROR

exit /B 0

:ERROR
pause
exit /B %ERR%
