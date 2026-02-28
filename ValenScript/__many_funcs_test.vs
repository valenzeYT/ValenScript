IMPORT math
IMPORT string

lst : ["a"|"b"|"c"]

PRINT math.min 4 9
PRINT math.max 4 9
PRINT math.clamp 12 0 10
PRINT math.sin 0
PRINT math.log 10
PRINT math.pi
PRINT math.deg2rad 180
PRINT math.gcd 54 24
PRINT math.lcm 12 18

PRINT string.length "hello"
PRINT string.split "a,b,c" ","
PRINT string.join lst "-"
PRINT string.substring "abcdef" 2 3
PRINT string.reverse "abc"
PRINT string.index_of "banana" "na"
PRINT string.pad_left "7" 3 "0"
PRINT string.title "hello world"
PRINT string.to_char "A"
PRINT string.from_char 66
PRINT string.remove_suffix "hello.txt" ".txt"
