IMPORT os
IMPORT json
IMPORT zip

obj : json.parse os.read "__sample.json"
PRINT TYPE obj
PRINT obj<"a">
PRINT json.valid os.read "__sample.json"
PRINT json.stringify obj
PRINT json.pretty obj 2

os.write "zip_src.txt" "hello zip"
PRINT zip.compress "zip_src.txt" "zip_test.zip"
PRINT zip.exists "zip_test.zip"
PRINT zip.list "zip_test.zip"
PRINT zip.extract "zip_test.zip" "zip_out"
PRINT os.exists "zip_out\\zip_src.txt"
