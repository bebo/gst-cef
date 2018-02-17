mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" .. -DUSE_SANDBOX=0
cmake --build . --config Release

set FILENAME=gst-cef_%TAG%.zip
"C:\Program Files\7-Zip\7z.exe" a -r ..\%FILENAME% -w .\dist\* -mem=AES256

cd ..
"C:\Program Files\Amazon\AWSCLI\aws.exe" s3api put-object --bucket bebo-app --key repo/gst-cef/%FILENAME% --body %FILENAME%
