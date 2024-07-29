#!/bin/bash

# Compile all the C programs
gcc read100.c -o read100
gcc read510.c -o read510
gcc write10end.c -o write10end
gcc copy.c -o copy
gcc rename.c -o rename

# Test read100
./read100
echo -n "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" > /tmp/test.txt
diff /tmp/read100.txt /tmp/test.txt
if [ $? -eq 0 ]
then
    echo "read100 pass"
else
    echo "read100 fail"
fi

# Test read510
echo "1234567890" > /tmp/read510.txt
output=$(./read510)
if [ "$output" == "567890" ]
then
    echo "read510 pass"
else
    echo "read510 fail"
fi

# Test write10end
echo -n "1234567890" > /tmp/write10end.txt
./write10end
echo -n "12345678901234567890" > /tmp/test.txt
diff /tmp/write10end.txt /tmp/test.txt
if [ $? -eq 0 ]
then
    echo "write10end pass"
else
    echo "write10end fail"
fi

# Test copy
echo "1234567890" > /tmp/original.txt
./copy
diff /tmp/original.txt /tmp/copy.txt
if [ $? -eq 0 ]
then
    echo "copy pass"
else
    echo "copy fail"
fi

# Test rename
echo "1234567890" > /tmp/oldname.txt
./rename
if [ -f /tmp/newname.txt ]
then
    echo "rename pass"
else
    echo "rename fail"
fi