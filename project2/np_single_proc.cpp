#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <sstream>  //istringstream
#include <unistd.h> //close(pipefd)
#include <stdio.h>  //fopen
#include <stdlib.h> //clearenv()
#include <unistd.h> //execv
#include <cstring>  //strcpy
#include <sys/wait.h>   //wait
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <iomanip>
#include <sys/types.h>  //open()
#include <sys/stat.h>   //open()
#include <fcntl.h>  //open()

#define MaxClientSize 30
#define MaxMessageSize 15000

using namespace std;

enum Sign
{
    Pipe,
    NumberPipe,
    ErrorPipe,
    Write,
    WriteUserPipe,
    None
};
struct Pipe
{
    int pipeNum;
    int senderId;
    int recverId;
    int clientId;
    int * pipefd;
    Sign sign;
};
struct Fd
{
    int in;
    int out;
    int error;
};
struct Env
{
    string name;
    string value;
};
struct ClientInfo
{
    int id;
    string name;
    string ip;
    unsigned short port;
    int sockfd;
    vector<struct Env> envV;
};


bool ids[MaxClientSize];
vector<struct ClientInfo>   clientV;
vector<struct Pipe> pipeV;
fd_set afds;
int mSock, nfds;

void SetStdInOut(Sign sign, Fd &fd, int pipeNum, int writeId, int readId, vector<struct ClientInfo>::iterator client, bool UserPipeInError, bool UserPipeOutError);
bool HasUserPipe(int senderId, int recverId);
bool HasNumberedPipe(int pipeNum, Sign sign, int clientId);
void ReducePipe(int ClientId);
void ReducePipeNum(int clientId);
void ClosePipe(int clientId, int readId);
void CreatePipe(Sign sign, int pipeNum, int clientId, int senderId, int recverId);
bool SpaceLine(string cmdLine, int &cmdSize);
void SetEnvTable(vector<struct Env> &envV, string env, string assign);
char** SetArgv(string cmd, vector<string> argV);
vector<string> SplitEnvPath(string path, char delim);
bool LegalCmd(string cmd, vector<string> argV, vector<string> pathV);
void DoCmd(string cmd, vector<string> argV, vector<string> pathV, Fd fd, int sockfd);
int DoBuildinCmd(string cmd, vector<string> argV, string &path, vector<string> &pathV, struct ClientInfo &client);
bool BuildCmd(string cmd);
void IdentifyCmd(vector<string> &splitCmdLine, vector<string>::iterator &iterLine, string &cmd, vector<string> &argV, Sign &sign, int &pipeNum, int &writeId, int &readId);
vector<string> SplitCmdWithSpace(string cmdLine);
int Shell(string cmdLine, vector<struct ClientInfo>::iterator &client);


void RemoveClient(vector<ClientInfo>::iterator iter);
vector<ClientInfo>::iterator IdentifyClientById(int id);
vector<ClientInfo>::iterator IdentifyClientByFd(int fd);
void BroadCast(vector<ClientInfo>::iterator iter,string msg, string action, int id);
void WelcomeUser(int sockfd);
int SetId();
int PassiveTcp(int port);
void InitEnv(struct ClientInfo client);
void PrintCmd(vector<struct ClientInfo>::iterator client, string cmd);
void PrintUserLogin();
void PrintUsers();
void PrintAfds();

void SetStdInOut(Sign sign, Fd &fd, int pipeNum, int writeId, int readId, vector<struct ClientInfo>::iterator client, bool UserPipeInError, bool UserPipeOutError)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();
    bool setIn =false;
    bool setOut = false;

    if(sign == Pipe)
    {
        if(UserPipeInError)
        {
            fd.in = -1;
            setIn = true;
        }
        while(iter != pipeV.end())
        {
            
            if(!setIn)
            {   /*  cmd <n  */
                if((*iter).senderId == readId && (*iter).recverId == client->id && (*iter).sign == WriteUserPipe)
                {
                    close((*iter).pipefd[1]);
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }/* |0  cmd or | cmd */
                else if((*iter).pipeNum == 0 && readId == 0 && (*iter).clientId == client->id)
                {
                    close((*iter).pipefd[1]);   //close write to pipe
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }
            }
            if(!setOut)
            {
                /*  cmd |   */
                if((*iter).pipeNum == 1 && (*iter).sign == Pipe && (*iter).clientId == client->id)
                {
                    fd.out = (*iter).pipefd[1];
                    setOut = true;
                }
            }
            iter++;
        }
    }
    else if(sign == NumberPipe)
    {
        if(UserPipeInError)
        {
            fd.in = -1;
            setIn = true;
        }
        while(iter != pipeV.end())
        {
            if(!setIn)
            {   /*  cmd <n  */
                if((*iter).senderId == readId && (*iter).recverId == client->id && (*iter).sign == WriteUserPipe)
                {
                    close((*iter).pipefd[1]);
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }/* |0  cmd or | cmd */
                else if((*iter).pipeNum == 0 && readId == 0 && (*iter).clientId == client->id)
                {
                    close((*iter).pipefd[1]);   //close write to pipe
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }
            }
            if(!setOut)
            {
                /*  cmd |n  */
                if((*iter).pipeNum == pipeNum && (*iter).sign == NumberPipe && (*iter).clientId == client->id)
                {
                    fd.out = (*iter).pipefd[1];
                    setOut = true;
                }
            }
            iter++;
        }
    }
    else if(sign == ErrorPipe)
    {
        if(UserPipeInError)
        {
            fd.in = -1;
            setIn = true;
        }
        while(iter != pipeV.end())
        {
            if(!setIn)
            {   /*  cmd <n  */
                if((*iter).senderId == readId && (*iter).recverId == client->id && (*iter).sign == WriteUserPipe)
                {
                    close((*iter).pipefd[1]);
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }/* |0  cmd or | cmd */
                else if((*iter).pipeNum == 0 && readId == 0 && (*iter).clientId == client->id)
                {
                    close((*iter).pipefd[1]);   //close write to pipe
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }
            }
            if(!setOut)
            {
                /*  cmd !n  */
                if((*iter).pipeNum == pipeNum && (*iter).sign == ErrorPipe && (*iter).clientId == client->id)
                {
                    fd.out = (*iter).pipefd[1];
                    fd.error = (*iter).pipefd[1];
                    setOut = true;
                }
            }
            iter++;
        }
    }
    else if(sign == Write)
    {
        if(UserPipeInError)
        {
            fd.in = -1;
            setIn = true;
        }
        while(iter != pipeV.end())
        {
            if(!setIn)
            {   /*  cmd <n  */
                if((*iter).senderId == readId && (*iter).recverId == client->id && (*iter).sign == WriteUserPipe)
                {
                    close((*iter).pipefd[1]);
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }/* |0  cmd or | cmd */
                else if((*iter).pipeNum == 0 && readId == 0 && (*iter).clientId == client->id)
                {
                    close((*iter).pipefd[1]);   //close write to pipe
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }
            }
            iter++;
        }
    }
    else if(sign == WriteUserPipe)
    {
        if(UserPipeInError)
        {
            fd.in = -1;
            setIn = true;
        }
        if(UserPipeOutError)
        {
            fd.out = -1;
            setOut = true;
        }
        while(iter != pipeV.end())
        {
            if(!setIn)
            {   /*  cmd <n  */
                if((*iter).senderId == readId && (*iter).recverId == client->id && (*iter).sign == WriteUserPipe)
                {
                    close((*iter).pipefd[1]);
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }/* |0  cmd or | cmd */
                else if((*iter).pipeNum == 0 && readId == 0 && (*iter).clientId == client->id)
                {
                    close((*iter).pipefd[1]);   //close write to pipe
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }
            }
            if(!setOut)
            {
                if((*iter).senderId == client->id && (*iter).recverId == writeId && (*iter).sign == WriteUserPipe)
                {
                    fd.out = (*iter).pipefd[1];
                    setOut = true;
                }
            }
            iter++;
        }
    }
    else
    {
        if(UserPipeInError)
        {
            fd.in = -1;
            setIn = true;
        }
        while(iter != pipeV.end())
        {
            if(!setIn)
            {   /*  cmd <n  */
                if((*iter).senderId == readId && (*iter).recverId == client->id && (*iter).sign == WriteUserPipe)
                {
                    close((*iter).pipefd[1]);
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }/* |0  cmd or | cmd */
                else if((*iter).pipeNum == 0 && readId == 0 && (*iter).clientId == client->id)
                {
                    close((*iter).pipefd[1]);   //close write to pipe
                    fd.in = (*iter).pipefd[0];
                    setIn = true;
                }
            }
            iter++;
        }
    }
    
}
bool HasUserPipe(int senderId, int recverId)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();

    while(iter != pipeV.end())
    {
        if((*iter).senderId == senderId && (*iter).recverId == recverId && (*iter).sign == WriteUserPipe)
            return true;

        iter++;
    }
    
    return false;
}
bool HasNumberedPipe(int pipeNum, Sign sign, int clientId)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();

    while(iter != pipeV.end())
    {
        if((*iter).pipeNum == pipeNum && ((*iter).sign == sign) && (*iter).clientId == clientId)
            return true;

        iter++;
    }
    
    return false;
}
/*  reduce pipe's number */
void ReducePipe(int clientId)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();
    while(iter != pipeV.end())
    {
        if((*iter).sign == Pipe && (*iter).clientId == clientId)
            (*iter).pipeNum--;
        iter++;
    }
}
/*  reduce numbered pipe 's number  */
void ReducePipeNum(int clientId)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();
    while(iter != pipeV.end())
    {
        if((iter->sign == NumberPipe || iter->sign == ErrorPipe) && iter->clientId == clientId)
            iter->pipeNum--;
        iter++;
    }
}
void ClosePipe(int clientId, int readId)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();
    while(iter != pipeV.end())
    {
        /*  | cmd or |0 cmd */
        if(iter->pipeNum == 0 && iter->clientId == clientId)
        {
            close(iter->pipefd[0]);
            close(iter->pipefd[1]);
            delete [] (*iter).pipefd;
            pipeV.erase(iter);
            
            break;
        }/* cmd <id */
        else if(iter->senderId == readId && iter->recverId == clientId && iter->sign == WriteUserPipe)
        {
            close((*iter).pipefd[0]);    
            close((*iter).pipefd[1]);
            delete [] (*iter).pipefd;
            pipeV.erase(iter);

            break;
        }
        iter++;
    }
}
void CreatePipe(Sign sign, int pipeNum, int clientId, int senderId, int recverId)
{
    int* pipefd = new int [2];
    struct Pipe newPipe;

    if(pipe(pipefd)<0)
    {
        cerr<<"Pipe create fail"<<" eerrno:"<<errno<<endl;
        exit(1);
    }
    newPipe.pipefd = pipefd;
    newPipe.sign = sign;
    newPipe.pipeNum = pipeNum;
    newPipe.clientId = clientId;
    newPipe.senderId = senderId;
    newPipe.recverId = recverId;
    pipeV.push_back(newPipe);
}
/*  check command line is space or not  
    there are \r and \n at the last by handwrite
    but there are \n only by demo                   */
bool SpaceLine(string cmdLine, int &cmdSize)
{
    int originSize = cmdSize;
    for(int i = originSize-1; i>=originSize-2; i--)
    {
        if(cmdLine[i] == '\r' || cmdLine[i] == '\n')
            cmdSize--;
    }
    if(cmdSize == 0)
        return true;
    else
        return false;
}
/*  handle env table    */
void SetEnvTable(vector<struct Env> &envV, string env, string assign)
{
    vector<struct Env>::iterator iter = envV.begin();
    while(iter != envV.end())
    {
        if(iter->name == env)
        {
            iter->value = assign;
            break;
        }
        iter++;
    }
    
    if(iter == envV.end())
    {
        struct Env temp;
        temp.name = env;
        temp.value = assign;
        envV.push_back(temp);
    }
}
/*  Set argv to exec    */
char** SetArgv(string cmd, vector<string> argV)
{   
    char ** argv = new char* [argV.size()+2];
    argv[0] = new char [cmd.size()+1];
    strcpy(argv[0], cmd.c_str());
    for(int i=0; i<argV.size(); i++)
    {
        argv[i+1] = new char [argV[i].size()+1];
        strcpy(argv[i+1], argV[i].c_str());
    }
    argv[argV.size()+1] = NULL;
    return argv;
}
vector<string> SplitEnvPath(string path, char delim)
{
    vector<string> pathV;
    string temp;
    stringstream ss(path);
    
    while(getline(ss, temp, delim))
    {
        pathV.push_back(temp);
    }

    return pathV;
}
bool LegalCmd(string cmd, vector<string> argV, vector<string> pathV)
{
    string path;
    vector<string>::iterator iter;
    iter = pathV.begin();
    
    FILE* file;
    while(iter != pathV.end())
    {
        path = *iter + "/" + cmd;
        file = fopen(path.c_str(), "r");
        if(file != NULL)
        {
            fclose(file);
            return true;
        }
        iter++;
    }

    return false;
}
void DoCmd(string cmd, vector<string> argV, vector<string> pathV, Fd fd, int sockfd)
{
    /*  Test legal command  */
    if(!LegalCmd(cmd, argV, pathV))
    {
        string msg = "Unknown command: [" + cmd + "].\n";
        write(sockfd, msg.c_str(), msg.length());
        exit(1);
    }

    int devNullIn, devNullOut;
    dup2(sockfd, 1);
    dup2(sockfd, 2);
    close(sockfd);
    if(fd.in != 0)
    {
        /*  user pipe in error  */
        if(fd.in == -1)
        {
            if((devNullIn = open("/dev/null", O_RDONLY)) <0)
                cout<<"Fail to redirect /dev/null, errno: "<<errno<<endl;
            if((dup2(devNullIn, 0)) <0)
                cout<<"Fail to dup2 /dev/null, errno: "<<errno<<endl;
        }
        else
            dup2(fd.in, 0);
    }
    if(fd.out != sockfd)
    {
        /*  user pipe out error */
        if(fd.out == -1)
        {
            if((devNullOut = open("/dev/null", O_WRONLY)) <0)
                cout<<"Fail to redirect dev/null, errno: "<<errno<<endl;
            if((dup2(devNullOut, 1)) <0)
                cout<<"Fail to dup2 /dev/null, errno: "<<errno<<endl;
        }
        else
            dup2(fd.out, 1);
    }
    if(fd.error != sockfd)
        dup2(fd.error, 2);
    if(fd.in != 0)
    {
        if(fd.in == -1)
            close(devNullIn);
        else
            close(fd.in);
    }
    if(fd.out != sockfd)
    {
        if(fd.in == -1)
            close(devNullOut);
        else
            close(fd.out);
    }
    if(fd.error != sockfd)
        close(fd.error);


    char **arg = SetArgv(cmd, argV);
    vector<string>::iterator iter = pathV.begin();
    while(iter != pathV.end())
    {
        string path = (*iter) + "/" + cmd;
        if((execv(path.c_str(), arg)) == -1)
            iter++;
    }
    
    cerr<<"Fail to exec"<<endl;
    exit(1);
}
int DoBuildinCmd(string cmd, vector<string> argV, string &path, vector<string> &pathV, vector<struct ClientInfo>::iterator &client)
{
    if(cmd == "printenv")
    {
        string env = argV[0];
        char* msg = getenv(env.c_str());
        write(client->sockfd, msg, strlen(msg));
        write(client->sockfd, "\n", strlen("\n"));
        return 0;
    }
    else if(cmd == "setenv")
    {
        string env, assign;

        env = argV[0];
        assign = argV[1];
        SetEnvTable(client->envV, env, assign);
        setenv(env.c_str(), assign.c_str(), 1);
        if(env == "PATH")
        {
            path = getenv("PATH");
            pathV.clear();
            pathV = SplitEnvPath(path, ':');
        }
        return 0;
    }
    else if(cmd == "who")
    {
        string msg = "<ID> <nickname> <IP:port>  <indicate me>\n";
        write(client->sockfd, msg.c_str(), msg.length());
        for(int i=0; i<MaxClientSize; i++)
        {
            if(ids[i])
            {
                int id = i+1;
                vector<ClientInfo>::iterator iter = IdentifyClientById(id);
                msg = to_string(iter->id) + "    " + iter->name + " " + iter->ip + ":" + to_string(iter->port) + "   ";
                write(client->sockfd, msg.c_str(), msg.length());
                if(id == client->id)
                {
                    msg = "<-me\n";
                    write(client->sockfd, msg.c_str(), msg.length());
                }
                else
                    write(client->sockfd, "\n", strlen("\n"));
            }
        }

        return 0;   
    }
    else if(cmd == "tell")
    {
        int sendId = atoi(argV[0].c_str());
        string msg = "*** " + client->name + " told you ***:";
        for(int i=1; i<argV.size(); i++)
            msg += (" " + argV[i]);
        msg += "\n"; 

        vector<ClientInfo>::iterator receiver = IdentifyClientById(sendId);
        if(receiver != clientV.end())
        {
            if(write(receiver->sockfd, msg.c_str(), msg.length()) <0)
                cerr<<"Fail to send msg: " + msg + ", errno: "<<errno<<endl;
        }
        else
        {
            msg = "*** Error: user #" + to_string(sendId) + " does not exist yet. ***\n";
            write(client->sockfd, msg.c_str(), msg.length());
        }
        
        return 0;
    }
    else if(cmd == "yell")
    {
        string msg;
        for(int i=0; i<argV.size(); i++)
            msg += (" " + argV[i]);
        
        BroadCast(client, msg, "yell", 0);
        
        return 0;
    }
    else if(cmd == "name")
    {
        string name = argV[0];
        
        for(int i=0; i<MaxClientSize; i++)
        {
            if(ids[i])
            {
                int id = i+1;
                vector<ClientInfo>::iterator iter = IdentifyClientById(id);
                if(iter->name == name && id != client->id)
                {
                    string msg = "*** User '" + name + "' already exists. ***\n";
                    write(client->sockfd, msg.c_str(), msg.length());
                    return 0;
                }
            }
        }

        BroadCast(client, name, "name", 0);
        client->name = name;
        return 0;
    }
    else   // exit or EOF
        return -1;
}
bool BuildCmd(string cmd)
{
    if(cmd == "setenv" || cmd == "printenv" || cmd == "exit" || cmd == "EOF" || cmd == "who" || cmd == "tell" || cmd == "yell" || cmd == "name")
        return true;
    return false;
}
void IdentifyCmd(vector<string> &splitCmdLine, vector<string>::iterator &iterLine, string &cmd, vector<string> &argV, Sign &sign, int &pipeNum, int &writeId, int &readId)
{
    string temp;
    bool isCmd = true;

    /*  who, tell yell and name are user cmd, and set all remaining iter as argument    */
    bool isBuildinCmd = BuildCmd(splitCmdLine[0]);

    while(iterLine != splitCmdLine.end())
    {
        temp = *iterLine;

        if(temp[0] == '|' && !isBuildinCmd)
        {
            if(temp.size() == 1)
            {
                sign = Pipe;
                pipeNum = 1;
            }
            else
            {
                sign = NumberPipe;
                string num;

                for(int i = 1; i < temp.size(); i++)
                    num += temp[i];

                pipeNum = stoi(num);
            }

            iterLine++;
            break;
        }
        else if(temp[0] == '!' && !isBuildinCmd)
        {
            sign = ErrorPipe;
            string num;

            for(int i = 1; i < temp.size(); i++)
                num += temp[i];

            pipeNum = stoi(num);
            
            iterLine++;
            break;
        }
        else if(temp[0] == '>' && temp.length() == 1 && !isBuildinCmd)
        {
            sign = Write;
            iterLine++;
            argV.push_back(*iterLine);

            iterLine++;
            break;
        }
        else if(temp[0] == '>' && !isBuildinCmd)
        {
            sign = WriteUserPipe;
            string id;

            for(int i=1; i<temp.size(); i++)
                id += temp[i];

            writeId = stoi(id);
            
            /*  check cat >2 <1 case    */
            if((iterLine+1) != splitCmdLine.end())
            {
                temp = *(iterLine+1);
                if(temp[0] == '<')
                {
                    iterLine++;
                    continue;
                }
            }
            iterLine++;
            break;
        }
        else if(temp[0] == '<' && !isBuildinCmd)
        {
            string id;
            for(int i=1; i<temp.size(); i++)
                id += temp[i];

            readId = stoi(id);
        }
        else if(isCmd)
        {
            cmd = temp;
            isCmd = false;
        }
        else
        {
            argV.push_back(temp);
        }
        
        iterLine++;
    }
}
vector<string> SplitCmdWithSpace(string cmdLine)
{
    istringstream ss(cmdLine);
    vector<string> splitCmdLine;
    string temp;

    while(ss>>temp)
    {
        splitCmdLine.push_back(temp);
    }
    
    return splitCmdLine;
}
/*  Debug   */
// void printpipetable()
// {
//     vector<struct Pipe>::iterator it;
//     cout << "pipe Table" << endl;
//     for(it=pipeV.begin() ; it != pipeV.end() ; it ++)
//     {
//         cout << "Enterfd = " << (*it).pipefd[0]<< endl;
//         cout << "Outfd = " << (*it).pipefd[1]<< endl;
//         cout << "PipeNum = " << (*it).pipeNum<< endl;
//         cout<<"clientId = "<<(*it).clientId<<endl;
//         cout<<"senderId = "<<(*it).senderId<<endl;
//         cout<<"recverId = "<<(*it).recverId<<endl;
//         cout<<"Sign = "<<(*it).sign<<endl;
//     }
//     cout << "end!" <<endl;
// }
// void printargtable(vector<string> &arg, Sign sign, string cmd, int pipeNum, int writeId, int readId)
// {
//     vector<string>::iterator it_i;
//     cout << "arg table" << endl;
//     cout<<"cmd = "<<cmd<<" ";
//     for(it_i=arg.begin() ; it_i != arg.end() ; it_i ++)
//     {
//         cout << *it_i<<" ";
//     }
//         cout<<endl;
//         cout<<"Sign = "<<sign<<" pipenum ="<<pipeNum<<" writeid ="<<writeId<<" readid ="<<readId<<endl;
//         cout << "End!" << endl;
 
// }
int Shell(string cmdLine, vector<struct ClientInfo>::iterator &client)
{
    vector<string> splitCmdLine;
    vector<string>::iterator iterLine;
    vector<pid_t> pidV;
    
    bool isNumPipe = false;

    string path = getenv("PATH");
    vector<string> pathV = SplitEnvPath(path, ':');

    splitCmdLine = SplitCmdWithSpace(cmdLine);
    iterLine = splitCmdLine.begin();
        
    while(iterLine != splitCmdLine.end() && *iterLine != "\0")
    {
        Fd fd = {0, client->sockfd, client->sockfd};

        vector<string> argV;
        string cmd;
        Sign sign = None;
        int pipeNum = 0;
        int writeId = 0;
        int readId = 0;
        bool UserPipeInError = false;
        bool UserPipeOutError = false;

        IdentifyCmd(splitCmdLine, iterLine, cmd, argV, sign, pipeNum, writeId, readId);
            
        /*  Do buildin command  */
        if(BuildCmd(cmd))
        {
            int status;
            status = DoBuildinCmd(cmd, argV, path, pathV, client);
            ClosePipe(client->id, readId);
            ReducePipe(client->id);

            if(status == -1)
                return status;
            else
                break;
        }
        
        /*  handle cmd <n   */
        if(readId != 0)
        {
            if(!ids[readId-1])
            {
                string msg = "*** Error: user #" + to_string(readId) + " does not exist yet. ***\n";
                write(client->sockfd, msg.c_str(), msg.length());
                UserPipeInError = true;
            }
            else if(!HasUserPipe(readId, client->id))
            {
                string msg = "*** Error: the pipe #" + to_string(readId) + "->#" + to_string(client->id) + " does not exist yet. ***\n";
                write(client->sockfd, msg.c_str(), msg.length());
                UserPipeInError = true;
            }
            else
                BroadCast(client, cmdLine, "readuser", readId);
        }

        if(sign == Pipe)
        {
            CreatePipe(sign, pipeNum, client->id, 0, 0);
        }
        else if(sign == NumberPipe || sign == ErrorPipe)
        {
            if(!HasNumberedPipe(pipeNum, sign, client->id))
                CreatePipe(sign, pipeNum, client->id, 0, 0);
            isNumPipe = true;
        }
        else if(sign == Write)
        {
            string fileName = argV[argV.size()-1];
            argV.pop_back();
            FILE* file = fopen(fileName.c_str(), "w");
                
            if(file == NULL)
            {
                cerr<<"Fail to open file"<<endl;
                return -1;
            }
                
            fd.out = fileno(file);
        }
        else if(sign == WriteUserPipe)
        {
            isNumPipe = true;
            if(!ids[writeId-1])
            {
                string msg = "*** Error: user #" + to_string(writeId) + " does not exist yet. ***\n";
                write(client->sockfd, msg.c_str(), msg.length());
                UserPipeOutError = true;
            }
            else if(!HasUserPipe(client->id, writeId))
            {
                BroadCast(client, cmdLine, "writeuser", writeId);
                CreatePipe(sign, -1, client->id, client->id, writeId);
            }
            else
            {
                string msg = "*** Error: the pipe #" + to_string(client->id) + "->#" + to_string(writeId) + " already exists. ***\n";
                write(client->sockfd, msg.c_str(), msg.length());
                UserPipeOutError = true;
            }
        }

        SetStdInOut(sign, fd, pipeNum, writeId, readId, client, UserPipeInError, UserPipeOutError);
        
        pid_t pid;
        while((pid = fork()) <0)
        {
            /*  parent process no resource to fork child process, so wait for child exit and release recourse   */
            int status = 0;
            waitpid(-1, &status, 0);
        }
        if(pid == 0)
            DoCmd(cmd, argV, pathV, fd, client->sockfd);
        else if(pid > 0)
            pidV.push_back(pid);

        if(fd.in != 0)  //close read from pipe, the other entrance is closed in SetStdInOut
            close(fd.in);

        ClosePipe(client->id, readId);
        ReducePipe(client->id);
    }

    if(!isNumPipe)
    {
        vector<pid_t>::iterator iter = pidV.begin();
        while(iter != pidV.end())
        {
            int status;
            waitpid((*iter), &status, 0);
            iter++;
        }
    }
    
    int cmdSize = cmdLine.length();
    if(!SpaceLine(cmdLine, cmdSize))
        ReducePipeNum(client->id);

    string title = "% ";
    if(write(client->sockfd, title.c_str(), title.length()) <0)
        cerr<<"Fail to send %, errno: "<<errno<<endl;

    return 0;
}
void RemoveClient(vector<struct ClientInfo>::iterator client)
{
    /*  close all pipe related to client    */
    vector<struct Pipe>::iterator iter = pipeV.begin();
    while(iter != pipeV.end())
    {
        if(iter->clientId == client->id || iter->recverId == client->id)
        {
            close(iter->pipefd[0]);
            close(iter->pipefd[1]);
            delete [] iter->pipefd;
            pipeV.erase(iter);
            continue;
        }
        iter++;
    }

    ids[client->id-1] = false;
    clientV.erase(client);
}
vector<struct ClientInfo>::iterator IdentifyClientById(int id)
{
    vector<struct ClientInfo>::iterator iter = clientV.begin();
    while(iter != clientV.end())
    {
        if((*iter).id == id)
            return iter;
        iter++;
    }
    return iter;
}
vector<struct ClientInfo>::iterator IdentifyClientByFd(int fd)
{
    vector<struct ClientInfo>::iterator iter = clientV.begin();
    while(iter != clientV.end())
    {
        if(iter->sockfd == fd)
            return iter;
        iter++;
    }
    return iter;
}
void BroadCast(vector<struct ClientInfo>::iterator iter, string msg, string action, int id)
{
    string news;
    vector<struct ClientInfo>::iterator partner;
    if(id != 0)
        partner = IdentifyClientById(id);

    if(action == "login")
        news = "*** User '" + iter->name + "' entered from " + iter->ip + ":" + to_string(iter->port) + ". ***\n";
    else if(action == "logout")
        news = "*** User '" + iter->name + "' left. ***\n";
    else if(action == "yell")
        news = "*** " + iter->name + " yelled ***:" + msg + "\n";
    else if(action == "name")
        news = "*** User from " + iter->ip + ":" + to_string(iter->port) + " is named '" + msg + "'. ***\n";
    else if(action == "writeuser")
    {   
        /*  erase null at the last character    */
        while(!msg.empty() && (msg[msg.size()-1] == '\r' || msg[msg.size()-1] == '\n'))
            msg.erase(msg.size()-1);
        news = "*** " + iter->name + " (#" + to_string(iter->id) + ") just piped '" + msg + "' to " + partner->name + " (#" + to_string(partner->id) + ") ***\n";
    }
    else if(action == "readuser")
    {
        /*  erase null at the last character    */
        while(!msg.empty() && (msg[msg.size()-1] == '\r' || msg[msg.size()-1] == '\n'))
            msg.erase(msg.size()-1);
        news = "*** " + iter->name + " (#" + to_string(iter->id) + ") just received from " + partner->name + " (#" + to_string(partner->id) + ") by '" + msg + "' ***\n";
    }


    for(int fd=0; fd<nfds; fd++)
    {
        if(fd != mSock && FD_ISSET(fd, &afds))
        {
            if(write(fd, news.c_str(), news.length()) <0)
                cerr<<"Fail to send " + news + ", errno: "<<errno<<endl;
        }
        
    }
}
void WelcomeUser(int sockfd)
{
    string msg = "****************************************\n";
    write(sockfd, msg.c_str(), msg.length());
    msg = "** Welcome to the information server. **\n";
    write(sockfd, msg.c_str(), msg.length());
    msg = "****************************************\n";
    write(sockfd, msg.c_str(), msg.length());
}
int SetId()
{
    for(int i=0; i<MaxClientSize; i++)
    {
        if(!ids[i])
        {
            ids[i] = true;
            return i+1;
        }
    }
    return 0;
}
int PassiveTcp(int port)
{
    // const char* protocal  = "TCP";
    struct sockaddr_in servAddr;
    struct protoent *proEntry;
    int mSock, type;

    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(port);

    type = SOCK_STREAM;
    
    if((mSock = socket(AF_INET, type, 0)) <0)
    {
        cerr<<"Fail to create master socket, errno: "<<errno<<endl;
        exit(1);
    }

	int optval = 1;
	if (setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) 
    {
		cout<<"Error: Set socket failed, errno: "<<errno<<endl;
		exit(1);
	}

    if(bind(mSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) <0)
    {
        cerr<<"Fail to bind master socket, errno: "<<errno<<endl;
        exit(1);
    }

    if(listen(mSock, 0) <0)
    {
        cerr<<"Fail to listen master socket, errno: "<<errno<<endl;
        exit(1);
    }

    return mSock;
}
void InitEnv(vector<struct ClientInfo>::iterator client)
{
    clearenv(); //Linux only
    vector<struct Env>::iterator iter = client->envV.begin();
    while(iter != client->envV.end())
    {
        setenv(iter->name.c_str(), iter->value.c_str(), 1);
        iter++;
    }
}
void PrintCmd(vector<struct ClientInfo>::iterator client, string cmd)
{
    cout<<"------Cmd------"<<endl;
    cout<<"Id: "<<client->id<<" Cmd: "<<cmd<<endl;
    cout<<"---------------"<<endl;
}
void PrintServer(int mSock)
{
    cout<<"------Server------"<<endl;
    cout<<"fd: "<<mSock<<endl;
    cout<<"------------------"<<endl;
}
void PrintUserLogout(vector<struct ClientInfo>::iterator client)
{
    cout<<"------User logout------"<<endl;
    cout<<"Id: "<<client->id<<" fd: "<<client->sockfd<<" ip:port"<<client->ip + ":" + to_string(client->port)<<endl;
    cout<<"----------------------"<<endl;
}
void PrintUserLogin()
{
    cout<<"------User login------"<<endl;
    vector<struct ClientInfo>::iterator client = clientV.end()-1;
    cout<<"Id: "<<client->id<<" fd: "<<client->sockfd<<" ip:port "<<client->ip + ":" + to_string(client->port)<<endl;
    cout<<"----------------------"<<endl;
}
void PrintUsers()
{
    cout<<"------Client table------"<<endl;
    
    if(!clientV.size())
        cout<<"No client"<<endl;
    else
    {
        for(int i=0; i<MaxClientSize; i++)
        {
            if(ids[i])
            {
                int id = i+1;
                vector<struct ClientInfo>::iterator client = IdentifyClientById(id);
                cout<<client->id<<"    "<<client->name<<" "<<client->ip<<":"<<to_string(client->port)<<endl;
            }
        }
    }
    cout<<"------------------------"<<endl;
}
void PrintAfds()
{
    cout<<"------Afds table------"<<endl;
    for(int i=0; i<nfds; i++)
    {
        if(FD_ISSET(i, &afds))
            cout<<i<<" ";
    }
    cout<<endl;
    cout<<"----------------------"<<endl;
}
int main(int argc, char* const argv[])
{
    if(argc > 2)
    {
        cerr<<"Argument set error"<<endl;
        exit(1);
    }

    setenv("PATH", "bin:.", 1);

    struct sockaddr_in cliAddr;
    socklen_t cliLen;
    int port = atoi(argv[1]);
    fd_set rfds;

    /*  Init ids array */
    memset((void*)ids, (int)false, MaxClientSize * sizeof(bool));

    /*  socket() bind() listen()    */
    mSock = PassiveTcp(port);
    PrintServer(mSock);

    /*  kill zombie */
    signal(SIGCHLD, SIG_IGN);

    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(mSock, &afds);   //afds[mSock] = true
    
    while(1)
    {
        memcpy(&rfds, &afds, sizeof(rfds));
        PrintUsers();
        PrintAfds();

        /*  if there is a client enter, then keep going, otherwise just stop here   */
        cout<<"Wait for client entering..."<<endl;
        if(select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (timeval*)0) <0)
        {
            cerr<<"Fail to select, errno: "<<errno<<endl;
            if(errno == EINTR)
                continue;
            exit(1);
        }

        if(FD_ISSET(mSock, &rfds))
        {
            int sSock;
            cliLen = sizeof(cliAddr);
            if((sSock = accept(mSock, (struct sockaddr*)&cliAddr, &cliLen)) <0)
            {
                cerr<<"Fail to accept, errno: "<<errno<<endl;
                exit(1);
            }
            
            /*  record client info */
            struct ClientInfo client;
            client.id = SetId();
            client.name = "(no name)";
            client.ip = inet_ntoa(cliAddr.sin_addr);
            client.port = ntohs(cliAddr.sin_port);
            client.sockfd = sSock;
            struct Env env;
            env.name = "PATH";
            env.value = "bin:.";
            client.envV.push_back(env);
            clientV.push_back(client);

            FD_SET(sSock, &afds);

            PrintUserLogin();
            WelcomeUser(sSock);
            BroadCast(clientV.end()-1, "", "login", 0);
            
            string title = "% ";
            write(sSock, title.c_str(), title.length());
        }


        for(int fd=0; fd<nfds; fd++)
        {
            if(fd != mSock && FD_ISSET(fd, &rfds))
            {
                int n;
                char cmdLine[MaxMessageSize];
                memset(&cmdLine, '\0', sizeof(cmdLine));
                vector<ClientInfo>::iterator client = IdentifyClientByFd(fd);
                if(read(fd, cmdLine, MaxMessageSize) <0)
                {
                    if(errno == EINTR)
                        continue;
                    cerr<<"Fail to recv, errno: "<<errno<<endl;
                    
                    PrintUserLogout(client);
                    BroadCast(client, "", "logout", 0);
                    RemoveClient(client);
                    close(fd);
                    FD_CLR(fd, &afds);
                }
                else if(n==0)
                {
                    /*  logout  */
                    PrintUserLogout(client);
                    BroadCast(client, "", "logout", 0);
                    RemoveClient(client);
                    close(fd);
                    FD_CLR(fd, &afds);
                }
                else
                {
                    string str(cmdLine);
                    PrintCmd(client, cmdLine);

                    InitEnv(client);
                    int status;
                    if((status = Shell(cmdLine, client) <0))
                    {
                        PrintUserLogout(client);
                        BroadCast(client, "", "logout", 0);
                        RemoveClient(client);
                        close(fd);
                        FD_CLR(fd, &afds);
                    }
                }
                
            }
        }
    }

    return 0;
}