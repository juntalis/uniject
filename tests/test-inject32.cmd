@echo off
setlocal
if not defined INCLUDE goto error_msvc
call cl.exe /Zi /EHs-c- /nologo /W3 /WX- /Qpar /Gw /Zc:inline /Od /Gm- /GS- /fp:precise /Qfast_transcendentals /D_WINDOWS=1 /D_WIN32=1 /DNDEBUG=1 /D_NDEBUG=1 /D_UNICODE=1 /DUNICODE=1 /MT /D_CRT_SECURE_NO_DEPRECATE /Foinject-test32.obj inject-test.c /link /nologo /INCREMENTAL:NO /OPT:REF /OPT:ICF=64 /MACHINE:X86 /MANIFEST:EMBED /CGTHREADS:4 /SUBSYSTEM:WINDOWS /OUT:inject-test32.exe kernel32.lib
if errorlevel 1 exit /B %ERRORLEVEL%
call cl.exe /Zi /Zl /EHs-c- /nologo /W3 /WX- /Qpar /Gw /Zc:inline /Od /Gm- /GS- /fp:precise /Qfast_transcendentals /D_USRDLL=1 /D_WIN32=1 /DNDEBUG=1 /D_NDEBUG=1 /D_UNICODE=1 /DUNICODE=1 /MD /D_CRT_SECURE_NO_DEPRECATE /Fotest-dll32.obj test-dll.c /link /nologo /INCREMENTAL:NO /OPT:REF /OPT:ICF=64 /NODEFAULTLIB /MACHINE:X86 /MANIFEST:EMBED /CGTHREADS:4 /DLL /OUT:test32.dll kernel32.lib
if errorlevel 1 exit /B %ERRORLEVEL%
call "%~dp0inject-test32.exe"
if errorlevel 1 exit /B %ERRORLEVEL%
goto :EOF

:error_msvc
echo ERROR: Need to run the MSVC tools environment before running this.
exit /B 1

