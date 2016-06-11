cd /Users/hl/Github/os161-base-2.0.2

find . -name "*.c" > cscope.files

find . -name "*.h" >> cscope.files

cscope -b -q -k
