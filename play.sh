#! /bin/bash

# param: dir with uncompressed vgms

IFS=$'\n' 

echo $1
ls $1 > flist

for file in `cat flist`
do
echo "$file"

./vgm_p "$1$file" $2
done
