# socketProject
--------------------------------------------------------------

Ahmir Bijanzadeh ahmirbijan@yhaoo.com

Dominic Jacobo dominicjacob99@gmail.com

Shahmir Ahmed ashahmir12@gmail.com

Miles Chao 2miles4003@gmail.com

Jair Chavez jairchavez321@csu.fullerton.edu

--------------------------------------------------------------

Implemeted a FTP client and server in C++.
Parses common commands and prints output to the terminal.
Handles downloads and uploads betwen client and server.

--------------------------------------------------------------

How to execute program:
1. open the folder and go to the server folder
2. run the command prompt from the server folder, alternatively, you can just navigate to the correct directory using cd on command prompt
4. type the command: g++ -o server server.cpp TCPLib.cpp -pthread
6. In a seperate instance of command prompt navigate to, or open from the client folder
7. type the command: g++ -o client client.cpp TCPLib.cpp -pthread

8. from the server terminal, use the command: ./server 8080
9. from the client terminal, use the command: ./client 127.0.0.1 8080
10. with this, a connection has been established between the server and the client, the client can now input various commands that will perform specific functions
   
11. list of commands you can input into the client:
    - ls; List all files under the present folder in the directory of the server
    - quit; closes client connection to the server
    - get; download files from the server folder to the client
    - put; upload a file from the client to the server
