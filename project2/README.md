# Network Programming Project 2 - Remote Working Ground (rwg) Server

## I.   Introduction
In this project, you are asked to design 3 kinds of servers:<br>
1. Design a Concurrent connection-oriented server. This server allows one client connect to it.<br>
2. Design a server of the chat-like systems, called remote working systems (rwg). In this system, users can communicate with other users. You need to use the single-process concurrent paradigm to design this server.<br>
3. (Bonus) Design the rwg server using the concurrent connection-oriented paradigm with shared memory.<br>
These three servers must support all functions in project 1.<br>
## II.  The structure of the working directory
```
  working_dir
  ├─ bin            # The directory contains executables
  │   ├─cat
  │   ├─ls
  │   ├─noop        # A program that does nothing
  │   ├─number      # Add a number to each line of input
  │   ├─removetag   # Remove HTML tags and output to STDOUT
  │   └─removetag0  # Same as removetag, but outputs error messages to STDERR.
  ├─ (your user pipe files)
  └─ test.html

```
## III.  Scenario of Part One
You can use telnet to connect to your server.<br>
Assume your server is running on nplinux1 and listening at port 7001.<br>
```
bash$ telnet nplinux1.cs.nctu.edu.tw 7001
% ls | cat
bin test.html
% ls |1
% cat
bin test.html
% exit
bash$
```

## IV. Scenario of Part Two
### A.  Introduction of Requirements
You are asked to design the following features in your server.<br>
1. Pipe between different users. Broadcast message whenever a user pipe is used.<br>
2. Broadcast message of login/logout information.<br>
3. New built-in commands:<br>
  • who: show information of all users.<br>
  • tell: send a message to another user.<br>
  • yell: send a message to all users.<br>
  • name: change your name.<br>
4. All commands in project 1
### B.  Scenario
The following is a scenario of using the rwg system.<br>
Assume your server is running on nplinux1 and listening at port 7001.<br>
```

bash$ telnet nplinux1.nctu.edu.tw 7001
***************************************
** Welcome to the information server **
***************************************                     # welcome message
*** User ’(no name)’ entered from 140.113.215.62:1201. ***  # Broadcast message of user login
% who
<ID>    <nickname>  <IP:port>           <indicate me>
1       (no name)   140.113.215.62:1201 <- me
% name Jamie
*** User from 140.113.215.62:1201 is named ’Jamie’. ***
% ls
bin test.html
% *** User ’(no name)’ entered from 140.113.215.63:1013. *** # User 2 logins
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Jamie       140.113.215.62:1201 <- me
2       (no name)   140.113.215.63:1013
% *** User from 140.113.215.63:1013 is named ’Roger’. ***    # User 2 inputs ’name Roger’
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Jamie       140.113.215.62:1201 <- me
2       Roger       140.113.215.63:1013
% *** User ’(no name)’ entered from 140.113.215.64:1302. *** # User 3 logins
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Jamie       140.113.215.62:1201 <- me
2       Roger       140.113.215.63:1013
3       (no name)   140.113.215.64:1302
% yell Who knows how to make egg fried rice? help me plz!
*** Jamie yelled ***: Who knows how to make egg fried rice? help me plz!
% *** (no name) yelled ***: Sorry, I don’t know. :-(          # User 3 yells
*** Roger yelled ***: HAIYAAAAAAA !!!                         # User 2 yells
% tell 2 Plz help me, my friends!
% *** Roger told you ***: Yeah! Let me show you the recipe    #User2 tells to User1
*** Roger (#2) just piped ’cat EggFriedRice.txt >1’ to Jamie (#1) *** # Broadcast message of user pipe 
*** Roger told you ***: You can use ’cat <2’ to show it!
cat <5                                                        #mistyping
*** Error: user #5 does not exist yet. ***
% cat <2                                                      # receive from user pipe
*** Jamie (#1) just received from Roger (#2) by ’cat <2’ ***
Ask Uncle Gordon
Season with MSG !!
% tell 2 It’s works! Great!
% *** Roger (#2) just piped ’number EggFriedRice.txt >1’ to Jamie (#1) ***
*** Roger told you ***: You can receive by your program! Try ’number <2’!
number <2
*** Jamie (#1) just received from Roger (#2) by ’number <2’ ***
1 1 Ask Uncle Gorgon
2 2 Season with MSG !!
% tell 2 Cool! You’re genius! Thank you!
% *** Roger told you ***: You’re welcome!
*** User ’Roger’ left. ***
exit
bash$
```


Now, let’s see what happened to the second user:
```
bash$ telnet nplinux1.nctu.edu.tw 7001                        # The server port number
***************************************
** Welcome to the information server **
***************************************
*** User ’(no name)’ entered from 140.113.215.63:1013. ***
% name Roger
*** User from 140.113.215.63:1013 is named ’Roger’. ***
% *** User ’(no name)’ entered from 140.113.215.64:1302. ***
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Jamie       140.113.215.62:1201
2       Roger       140.113.215.63:1013 <- me
3       (no name)   140.113.215.64:1302
% *** Jamie yelled ***: Who knows how to make egg fried rice? help me plz!
*** (no name) yelled ***: Sorry, I don’t know. :-(
yell HAIYAAAAAAA !!!
*** Roger yelled ***: HAIYAAAAAAA !!!
% *** Jamie told you ***: Plz help me, my friends!
tell 1 Yeah! Let me show you the recipe
% cat EggFriedRice.txt >1 # write to the user pipe
*** Roger (#2) just piped ’cat EggFriedRice.txt >1’ to Jamie (#1) ***
% tell 1 You can use ’cat <2’ to show it!
% *** Jamie (#1) just received from Roger (#2) by ’cat <2’ ***
*** Jamie told you ***: It’s works! Great!
number EggFriedRice.txt >1
*** Roger (#2) just piped ’number EggFriedRice.txt >1’ to Jamie (#1) ***
% tell 1 You can receive by your program! Try ’number <2’!
% *** Jamie (#1) just received from Roger (#2) by ’number <2’ ***
*** Jamie told you ***: Cool! You’re genius! Thank you!
tell 1 You’re welcome!
% exit
bash$
```
