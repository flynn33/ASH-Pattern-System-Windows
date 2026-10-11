@echo off
REM Forward all named options to the single verified SDK packaging route.
pwsh.exe -NoProfile -File "%~dp0package-sdk.ps1" %*
exit /b %errorlevel%
