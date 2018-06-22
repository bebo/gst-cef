ECHO ON

mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" .. -DUSE_SANDBOX=0

if errorlevel 1 (
  exit /b %errorlevel%
)

cmake --build . --config Release

if errorlevel 1 (
  exit /b %errorlevel%
)

set FILENAME=gst-cef_%TAG%.zip
set RELEASE_NAME=gst-cef_%ENV%.zip
"C:\Program Files\7-Zip\7z.exe" a -r ..\%FILENAME% -w .\dist\* -mem=AES256

if errorlevel 1 (
  exit /b %errorlevel%
)

cd ..
"C:\Program Files\Amazon\AWSCLI\aws.exe" s3api put-object --bucket bebo-app --key repo/gst-cef/%FILENAME% --body %FILENAME%
"C:\Program Files\Amazon\AWSCLI\aws.exe" s3api put-object --bucket bebo-app --key repo/gst-cef/%RELEASE_NAME% --body %FILENAME%

if errorlevel 1 (
  exit /b %errorlevel%
)