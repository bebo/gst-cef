@ECHO ON
set errorlevel=
set FILENAME=%TEMP%\%JOB_NAME%_%ENV%_%TAG%.zip

mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" .. -DUSE_SANDBOX=0

if errorlevel 1 (
    echo "cmake -G failed with %errorlevel%"
    exit /b %errorlevel%
)

cmake --build . --config Release

if errorlevel 1 (
    echo "cmake build failed with %errorlevel%"
    exit /b %errorlevel%
)

"C:\Program Files\7-Zip\7z.exe" a -r ..\%FILENAME% -w .\dist\* -mem=AES256

@if errorlevel 1 (
    echo "zip failed with %errorlevel%"
    exit /b %errorlevel%
)

if "%LIVE%" == "true" (
    "C:\Python34\python.exe" "C:\w\jenkins_uploader.py" --project %JOB_NAME% --tag %TAG% --env %ENV%
    @if errorlevel 1 (
      echo "jenkins_upload failed with %errorlevel%"
      exit /b %errorlevel%
    )
) else (
    "C:\Python34\python.exe" "C:\w\jenkins_uploader.py" --project %JOB_NAME% --tag %TAG% --env %ENV% --no-deploy
    @if errorlevel 1 (
      echo "jenkins_upload failed with %errorlevel%"
      exit /b %errorlevel%
    )
)
