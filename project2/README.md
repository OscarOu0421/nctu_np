# Network Programming Project 2 - Remote Working Ground (rwg) Server

## I.   Introduction
In this project, you are asked to design 3 kinds of servers:<br>
1. Design a Concurrent connection-oriented server. This server allows one client connect to it.<br>
2. Design a server of the chat-like systems, called remote working systems (rwg). In this system, users can communicate with other users. You need to use the single-process concurrent paradigm to design this server.<br>
3. (Bonus) Design the rwg server using the concurrent connection-oriented paradigm with shared memory.<br>
These three servers must support all functions in project 1.<br>

## II.  Scenario of Part One
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

## III. Scenario of Part Two
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
