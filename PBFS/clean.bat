@echo off
setlocal

echo Cleaning...

rd /s /q build-cli 2>nul
rd /s /q build-fs 2>nul

echo Done.
