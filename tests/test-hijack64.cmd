@echo off
setlocal
if not defined INCLUDE goto error_msvc
call cl.exe /Zi /EHs-c- /nologo /W3 /WX- /Qpar /Gw /Zc:inline /Od /Gm- /GS- /fp:precise /Qfast_transcendentals /D_CONSOLE=1 /D_WIN64=1 /D_WIN32=1 /DNDEBUG=1 /D_NDEBUG=1 /D_UNICODE=1 /DUNICODE=1 /MT /D_CRT_SECURE_NO_DEPRECATE /Fohijack-test64.obj hijack-test.c /link /nologo /INCREMENTAL:NO /OPT:REF /OPT:ICF=64 /MACHINE:X64 /MANIFEST:EMBED /CGTHREADS:4 /SUBSYSTEM:CONSOLE /OUT:hijack-test64.exe
if errorlevel 1 exit /B %ERRORLEVEL%
call "%~dp0hijack-test64.exe"
if errorlevel 1 exit /B %ERRORLEVEL%
goto :EOF

:error_msvc
echo ERROR: Need to run the MSVC tools environment before running this.
exit /B 1

