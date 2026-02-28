IMPORT os
WHEN os.write path content
	PRINT ["event:", path, content]

WAIT 2

os.write "test.txt" "testtttt"
WHILE TRUE
    PRINT 67
    WAIT 9999999999