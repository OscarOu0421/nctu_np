# Network Programming Project 4 - SOCKS 4
## I. Introduction
In this project, you are going to implement the SOCKS 4/4A protocol in the application layer of the OSI model.<br>
SOCKS is similar to a proxy (i.e., intermediary-program) that acts as both server and client for the purpose of making requests on behalf of other clients.
Because the SOCKS protocol is independent of application protocols, it can be used for many different services: telnet, ftp, WWW, etc.
There are two types of the SOCKS operations, namely CONNECT and BIND. You have to implement both of them in this project.
We will use Boost.Asio library to accomplish this project.

## II. The structure of the working directory
```
  working_dir
  ├─ console.cpp
  └─ socks_server.cpp
```

## III. SOCKS 4 Implementation
After the SOCKS server starts listening, if a SOCKS client connects, use fork() to tackle with it.<br>
Each child process will do:<br>
  1. Receive SOCKS4 REQUEST from the SOCKS client<br>
  2. Get the destination IP and port from SOCKS4 REQUEST<br>
  3. Check the firewall (socks.conf), and send SOCKS4 REPLY to the SOCKS client if rejected<br>
  4. Check CD value and choose one of the operations<br>
    (a) CONNECT (CD=1)<br>
      i. Connect to the destination<br>
      ii. Send SOCKS4 REPLY to the SOCKS client<br>
      iii. Start relaying traffic on both directions<br> 
    (b) BIND (CD=2)<br>
      i. Bind and listen a port<br>
      ii. Send SOCKS4 REPLY to SOCKS client to tell which port to connect<br>
      iii. (SOCKS client tells destination to connect to SOCKS server)<br>
      iv. Accept connection from destination and send another SOCKS4 REPLY to SOCKS client<br>
      v. Start relaying traffic on both directions<br>

If the SOCKS server decides to reject a request from a SOCKS client, the connection will be closed immediately.<br>

## IV. Requirements
- Part I: SOCKS 4 Server Connect Operation
  - Open your browser and connect to any webpages. 
  - Turn on and set your SOCKS server, then
    - Be able to connect any webpages on Google Search. 
    - Your SOCKS server need to show messages below:
      ```
           <S_IP>: source ip
           <S_PORT>: source port
           <D_IP>: destination ip
           <D_PORT>: destination port
           <Command>: CONNECT or BIND
           <Reply>: Accept or Reject
      ```
- Part II: SOCKS 4 Server Bind Operation
  - FlashFXP settings:
    - Set your SOCKS server
    - Connection type is FTP (cannot be SFTP)
    - Data connection mode is Active Mode (PORT)
  - Connect to FTP server, and upload/download files larger than 1GB completely. e.g., Ubuntu 20.04 ISO image (download link)
    - Upload a file and download a file.
    - Check whether the SOCKS server’s output message shows that BIND operation is used.
- Part III: CGI Proxy
  - Modify console.cgi in Project 3 to implement SOCKS client mode
    - Accept SocksIP and SocksPort parameters in QUERY STRING as sh and sp, respectively
    - Use SocksIP and SocksPort to connect to your SOCKS server (by CONNECT operation)
    - Rename console.cgi into hw4.cgi in Makefile
  - Clear browser’s proxy setting
    - Open your http server, connect to panel socks.cgi
    - Key in IP, port, filename, SocksIP, SocksPort
    - Connect to 5 ras/rwg servers through SOCKS server and check the output Test Case (same as Project 3, no hidden test case) t1.txt-t5.txt
- Firewall
  - You only need to implement a simple firewall. List permitted destination IPs into socks.conf (deny all traffic by default)
  ```
    e.g.,
    permit c 140.114.*.*  # permit NTHU IP for Connect operation
    permit c 140.113.*.*  # permit NCTU IP for Connect operation
    permit b *.*.*.*      # permit all IP for Bind operation
  ```
  - Be able to change firewall rules without restarting the SOCKS server.
- Specification
  - Port number of SOCKS server is specified by the first argument: ./socks server [port]
  - You can only use C/C++ to implement this project. Except for Boost, other third-party libraries are NOT allowed.
  - Every function that touches network operations (e.g., DNS query, connect, accept, send, receive) MUST be implemented using the library Boost.Asio.
  - Both synchronous and asynchronous functions can be used. Notice that some situations only work with non-blocking operations. Be thoughtful when using       synchronous ones. 
