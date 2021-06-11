# Network Programming Project 1 - NPShell
## I. Introduction
In this project, you are asked to design a shell with special piping mechanisms.

## II. The structure of the working directory
```
  working_dir
  ├─ bin            # The directory contains executables
  │   ├─cat
  │   ├─ls
  │   ├─noop        # A program that does nothing
  │   ├─number      # Add a number to each line of input
  │   ├─removetag   # Remove HTML tags and output to STDOUT
  │   └─removetag0  # Same as removetag, but outputs error messages to STDERR.
  └─ test.html
```
In addition to the above executables, the following are built-in commands supported by your npshell<br>
a. setenv<br>
b. printenv<br>
c. exit<br>

## II. Scenario of using npshell
```
bash$ ./npshell   #​ execute your npshell
% printenv PATH   #​ initial PATH is bin/ and ./
bin:.
% setenv PATH bin #​ set PATH to bin/ only
% printenv PATH   
bin
% ls
bin             test.html
% ls bin
cat   ls   noop   number   removetag   removetag0
% cat test.html > test1.txt
% cat test1.txt
<!test.html>
<TITLE>Test</TITLE>
<BODY>This is a <b>test</b> program
for ras.
</BODY>
% removetag test.html

Test
This is a test program
for ras.
% removetag test.html > test2.txt
% cat test2.txt

Test
This is a test program
for ras.
% removetag0 test.html
Error: illegal tag "!test.html"

Test
This is a test program
for ras.

% removetag0 test.html > test2.txt
Error: illegal tag "!test.html"
% cat test2.txt

Test
This is a test program
for ras.

% removetag test.html | number
   1
   2 Test
   3 This is a test program
   4 for ras.
   5
% removetag test.html |1 ​# this pipe will pipe STDOUT to next command
% number ​                # the command's STDIN is from previous pipe
   1
   2 Test
   3 This is a test program
   4 for ras.
   5
% removetag test.html |2 ​# |2 will skip 1 line of commands and then
% ls                     ​# pipe STDOUT to the next next command 
bin         test1.txt
test.html   test2.txt
% number                 ​# the command's STDIN is from the previous pipe (removetag)
   1
   2 Test
   3 This is a test program
   4 for ras.
   5
% removetag test.html |2 ​# pipe STDOUT to the next next command
% removetagtest.html  |1 ​# pipe STDOUT to the next command(​merge with the previous one)
% number                 ​# STDIN is from the previous pipe
   1
   2 Test
   3 This is a test program
   4 for ras.
   5
   6
   7 Test
   8 This is a test program
   9 for ras.
  10
% removetag test.html |2
% removetag test.html |1
% number |1
% number
  1   1
  2   2 Test
  3   3 This is a test program
  4   4 for ras.
  5   5 
  6   6
  7   7 Test
  8   8 This is a test program
  9   9 for ras.
  10  10
% removetag test.html | number |1
% number
  1   1
  2   2 Test
  3   3 This is a test program
  4   4 for ras.
  5   5
% ls |2
% ls
bin             test1.txt
test.html       test2.txt
% number > test3.txt
% cat test3.txt
    1 bin
    2 test.html
    3 test1.txt
    4 test2.txt
 % removetag0 test.html |1
 Error: illegal tag "!test.html"​# output error message to STDERR
 % number
    1
    2 Test
    3 This is a test program
    4 for ras.
    5
 % removetag0 test.html !1 ​# this pipe will pipe both STDOUT and STDERR to the next command
 % number
    1 Error: illegal tag "!test.html"
    2
    3 Test
    4 This is a test program
    5 for ras.
    6
% date
 Unknown command: [date].
# TA manually move the executable `date` into ${working_dir}/bin/
% date
Mon Oct  5 15:12:35 CST 2020
% exit
bash$
```
