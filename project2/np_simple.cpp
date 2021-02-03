#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <sstream>  //istringstream
#include <unistd.h> //close(pipefd)
#include <stdio.h>  //fopen
#include <unistd.h> //execv
#include <cstring>  //strcpy
#include <sys/wait.h>   //wait
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <netdb.h>
#include <iomanip> //setfill()

#define MaxCmdSize 15000

using namespace std;

enum Sign
{
    Pipe,
    NumberPipe,
    ErrorPipe,
    Write,
    None
};
struct Pipe
{
    int num;
    int * pipefd;
    Sign sign;
};
struct Fd
{
    int in;
    int out;
    int error;
};

void SetStdInOut(vector<struct Pipe> pipeV, Sign sign, Fd &fd, int pipeNum)
{
    vector<struct Pipe>::iterator iter;
    iter = pipeV.begin();

    if(sign == Pipe)
    {
        while(iter != pipeV.end())
        {
            /*  |0  cmd or | cmd */
            if((*iter).num == 0)
            {
                close((*iter).pipefd[1]);   //close write to pipe
                fd.in = (*iter).pipefd[0];
            }

            /*  cmd |   */
            if((*iter).num == 1 && (*iter).sign == Pipe)
                fd.out = (*iter).pipefd[1];

            iter++;
        }
    }
    else if(sign == NumberPipe)
    {
        while(iter != pipeV.end())
        {
            /*  |0  cmd or | cmd */
            if((*iter).num == 0)
            {
                close((*iter).pipefd[1]);   //close write to pipe
                fd.in = (*iter).pipefd[0];
            }

            /*  |n  */
            if((*iter).num == pipeNum && (*iter).sign == NumberPipe)
                fd.out = (*iter).pipefd[1];
            
            iter++;
        }
    }
    else if(sign == ErrorPipe)
    {
        while(iter != pipeV.end())
        {
            /*  |0  cmd or | cmd */
            if((*iter).num == 0)
            {
                close((*iter).pipefd[1]);   //close write to pipe
                fd.in = (*iter).pipefd[0];
            }

            /*  !n  */
            if((*iter).num == pipeNum && (*iter).sign == ErrorPipe)
            {
                fd.out = (*iter).pipefd[1];
                fd.error = (*iter).pipefd[1];
            }
            iter++;
        }
    }
    else if(sign == Write)
    {
        while(iter != pipeV.end())
        {
            /*  |0  cmd or | cmd */
            if((*iter).num == 0)
            {
                close((*iter).pipefd[1]);   //close write to pipe
                fd.in = (*iter).pipefd[0];
            }
            iter++;
        }
    }
    else
    {
        while(iter != pipeV.end())
        {
            /*  |0  cmd or | cmd */
            if((*iter).num == 0)
            {
                close((*iter).pipefd[1]);   //close write to pipe
                fd.in = (*iter).pipefd[0];
            }
            iter++;
        }
    }
    
}
/*  check there already is a number pipe or not */
bool HasNumberedPipe(vector<struct Pipe> pipeV, int pipeNum, Sign sign)
{
    vector<struct Pipe>::iterator iter;
    iter = pipeV.begin();

    while(iter != pipeV.end())
    {
        if((*iter).num == pipeNum && (*iter).sign == sign)
            return true;

        iter++;
    }
    
    return false;
}
/*  reduce pipe's number */
void ReducePipe(vector<struct Pipe> &pipeV)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();
    while(iter != pipeV.end())
    {
        if((*iter).sign == Pipe)
            (*iter).num--;
        iter++;
    }
}
/*  reduce numbered pipe 's number  */
void ReducePipeNum(vector<struct Pipe> &PipeV)
{
    for(int i=0; i<PipeV.size();i++)
        PipeV[i].num--;
}
void ClosePipe(vector<struct Pipe> &pipeV)
{
    vector<struct Pipe>::iterator iter = pipeV.begin();
    while(iter != pipeV.end())
    {
        if((*iter).num == 0)
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
void CreatePipe(vector<struct Pipe> &pipeV, Sign sign, int pipeNum)
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
    newPipe.num = pipeNum;
    pipeV.push_back(newPipe);
}
/*  check command line is space or not  
    there are \r and \n at the last by handwrite
    but there are \n only by demo                   */
bool SpaceLine(char* cmdLine, int &cmdSize)
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
void DoCmd(string cmd, vector<string> argV, vector<string> pathV, Fd fd, int sSock)
{
    if(!LegalCmd(cmd, argV, pathV))
    {
        string msg = "Unknown command: [" + cmd + "].\n";
        write(sSock, msg.c_str(), msg.length());
        exit(1);
    }

    dup2(sSock, 1);
    dup2(sSock, 2);
    close(sSock);
    if(fd.in != 0)
        dup2(fd.in, 0);
    if(fd.out != sSock)
        dup2(fd.out, 1);
    if(fd.error != sSock)
        dup2(fd.error, 2);
    if(fd.in != sSock)
        close(fd.in);
    if(fd.out != sSock)
        close(fd.out);
    if(fd.error != sSock)
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
int DoBuildinCmd(int sockfd, string cmd, vector<string> argV, string &path, vector<string> &pathV)
{
    if(cmd == "printenv")
    {
        string env = argV[0];
        char* msg = getenv(env.c_str());
        write(sockfd, msg, strlen(msg));
        write(sockfd, "\n", strlen("\n"));
        return 0;
    }
    else if(cmd == "setenv")
    {
        string env, assign;

        env = argV[0];
        assign = argV[1];
        setenv(env.c_str(), assign.c_str(), 1);
        if(env == "PATH")
        {
            path = getenv("PATH");
            pathV.clear();
            pathV = SplitEnvPath(path, ':');
        }
        return 0;
    }
    else   // exit or EOF
        return -1;
}
bool BuildCmd(string cmd)
{
    if(cmd == "setenv" || cmd == "printenv" || cmd == "exit" || cmd == "EOF")
        return true;
    return false;
}
void IdentifyCmd(vector<string> &splitCmdLine, vector<string>::iterator &iterLine, string &cmd, vector<string> &argV, Sign &sign, int &pipeNum)
{
    string temp;
    bool isCmd = true;

    while(iterLine != splitCmdLine.end())
    {
        temp = *iterLine;

        if(temp[0] == '|')
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
        else if(temp[0] == '!')
        {
            sign = ErrorPipe;
            string num;

            for(int i = 1; i < temp.size(); i++)
                num += temp[i];

            pipeNum = stoi(num);
            
            iterLine++;
            break;
        }
        else if(temp[0] == '>')
        {
            sign = Write;
            iterLine++;
            argV.push_back(*iterLine);

            iterLine++;
            break;
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
int Shell(int sSock)
{
    vector<string> splitCmdLine;
    vector<string>::iterator iterLine;

    vector<struct Pipe> pipeV;

    vector<pid_t> pidV;

    string path = getenv("PATH");
    vector<string> pathV = SplitEnvPath(path, ':');
    
    
    write(sSock, "% ", strlen("% "));
    while(1)
    {
        char cmdLine[MaxCmdSize];
        int cmdSize;
        memset(&cmdLine, '\0', sizeof(cmdLine));
        if((cmdSize = read(sSock, cmdLine, MaxCmdSize)) <0)
        {
            if(errno == EINTR)
                continue;
            cerr<<"Fail to read cmdLine, errno: "<<errno<<endl;
            return -1;
        }

        splitCmdLine = SplitCmdWithSpace(cmdLine);
        iterLine = splitCmdLine.begin();
        bool isNumPipe = false;
        pidV.clear();

        while(iterLine != splitCmdLine.end())
        {
            Fd fd = {0, sSock, sSock};

            vector<string> argV;
            string cmd;
            Sign sign = None;
            int pipeNum = 0;

            IdentifyCmd(splitCmdLine, iterLine, cmd, argV, sign, pipeNum);
            
            /*  Do buildin command  */
            if(BuildCmd(cmd))
            {
                int status = DoBuildinCmd(sSock, cmd, argV, path, pathV);
                ClosePipe(pipeV);
                ReducePipe(pipeV);
                if(status <0)
                    return -1;
                else
                    break;
            }
            

            if(sign == Pipe)
            {
                CreatePipe(pipeV, sign, pipeNum);
            }
            else if(sign == NumberPipe || sign == ErrorPipe)
            {
                if(!HasNumberedPipe(pipeV, pipeNum, sign))
                    CreatePipe(pipeV, sign, pipeNum);
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

            SetStdInOut(pipeV, sign, fd, pipeNum);
            
            pid_t pid;
            while((pid = fork()) <0)
            {
                /*  parent process no resource to fork child process, so wait for child exit and release recourse   */
                int status = 0;
                waitpid(-1, &status, 0);
            }
            if(pid == 0)
                DoCmd(cmd, argV, pathV, fd, sSock);
            else if(pid > 0)
                pidV.push_back(pid);
            

            if(fd.in != 0)  //close read from pipe, the other entrance is closed in SetStdInOut
                close(fd.in);

            ClosePipe(pipeV);
            ReducePipe(pipeV);
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

        if(!SpaceLine(cmdLine, cmdSize))
            ReducePipeNum(pipeV);

        write(sSock, "% ", strlen("% "));
    }
    return 0;
}
int PassiveTcp(int port)
{
    const char* protocal  = "TCP";
    struct sockaddr_in servAddr;
    struct protoent *proEntry;
    int mSock, type;

    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(port);

    if((proEntry = getprotobyname(protocal) ) == NULL)
    {
        cerr<<"Fail to get protocal entry"<<endl;
        exit(1);
    }

    type = SOCK_STREAM;
    
    if((mSock = socket(PF_INET, type, proEntry->p_proto)) <0)
    {
        cerr<<"Fail to create master socket, errno: "<<errno<<endl;
        exit(1);
    }

	int optval = 1;
	if (setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) 
    {
		cout<<"Error: Set socket failed"<<endl;
		exit(1);
	}

    if(::bind(mSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) <0)
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
void InitEnv()
{
    setenv("PATH", "bin:.", 1);
}
void PrintServer(int mSock)
{
    cout<<"------Server------"<<endl;
    cout<<"fd: "<<mSock<<endl;
    cout<<"------------------"<<endl;
}
void PrintUserLogin(int sSock)
{
    cout<<"------User login------"<<endl;
    cout<<"fd: "<<sSock<<endl;
    cout<<"----------------------"<<endl;
}
int main(int argc, char* const argv[])
{
    if(argc > 2)
    {
        cerr<<"Argument set error"<<endl;
        exit(1);
    }

    InitEnv();

    struct sockaddr_in cliAddr;
    socklen_t cliLen;
    int mSock, sSock;
    int port = atoi(argv[1]);
    pid_t pid;

    /*  socket() bind() listen()    */
    mSock = PassiveTcp(port);
    PrintServer(mSock);

    /*  kill zombie */
    signal(SIGCHLD, SIG_IGN);

    while(1)
    {
        cliLen = sizeof(cliAddr);
        
        if((sSock = accept(mSock, (struct sockaddr*)&cliAddr, &cliLen)) <0)
        {
            /*  there is not client connect */
            if(errno == EINTR)
                continue;
            cerr<<"Server fail to accept, errno: "<<errno<<endl;
            exit(1);
        }

        PrintUserLogin(sSock);

        if((pid = fork()) <0)
        {
            cerr<<"Server fail to fork"<<endl;
            exit(1);
        }
        else if(pid == 0)
        {
            close(mSock);

            Shell(sSock);
            close(sSock);

            exit(0);
        }
        else
            close(sSock);
         
    }

    return 0;
}