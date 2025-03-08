@echo off
echo Generating PJLink plugin documentation...

:: Doxygen 설치 확인
where doxygen >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: Doxygen is not installed or not in PATH
    echo Please install Doxygen from http://www.doxygen.nl/download.html
    exit /b 1
)

:: Doxyfile 생성
echo Creating Doxyfile...
(
    echo PROJECT_NAME           = "PJLink Plugin"
    echo PROJECT_BRIEF          = "Unreal Engine plugin for PJLink projector control"
    echo OUTPUT_DIRECTORY       = ./Docs
    echo INPUT                  = ./Source
    echo FILE_PATTERNS          = *.h *.cpp
    echo RECURSIVE              = YES
    echo EXCLUDE_PATTERNS       = */Intermediate/* */Binaries/*
    echo GENERATE_HTML          = YES
    echo GENERATE_LATEX         = NO
    echo EXTRACT_ALL            = YES
    echo EXTRACT_PRIVATE        = YES
    echo EXTRACT_STATIC         = YES
    echo HAVE_DOT               = NO
    echo UML_LOOK               = YES
    echo CALL_GRAPH             = YES
    echo CALLER_GRAPH           = YES
    echo CLASS_DIAGRAMS         = YES
    echo USE_MDFILE_AS_MAINPAGE = ./README.md
    echo HTML_EXTRA_STYLESHEET  = 
    echo HTML_COLORSTYLE_HUE    = 220
    echo HTML_COLORSTYLE_SAT    = 100
    echo HTML_COLORSTYLE_GAMMA  = 80
    echo HTML_TIMESTAMP         = YES
    echo GENERATE_TREEVIEW      = YES
    echo TREEVIEW_WIDTH         = 250
    echo SEARCHENGINE           = YES
    echo GENERATE_TODOLIST      = YES
) > Doxyfile

:: Doxygen 실행
echo Running Doxygen...
doxygen Doxyfile

if %ERRORLEVEL% NEQ 0 (
    echo Error: Doxygen failed to generate documentation
    exit /b 1
)

echo Documentation generated successfully in ./Docs/html/
echo Opening documentation...

:: 문서 열기
start "" ".\Docs\html\index.html"

echo.
echo Complete!
