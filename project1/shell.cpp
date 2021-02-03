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
void DoCmd(string cmd, vector<string> argV, vector<string> pathV, Fd fd)
{
    if(fd.in != 0)
        dup2(fd.in, 0);
    if(fd.out != 1)
        dup2(fd.out, 1);
    if(fd.error != 2)
        dup2(fd.error, 2);
    if(fd.in != 0)
        close(fd.in);
    if(fd.out != 1)
        close(fd.out);
    if(fd.error != 2)
        close(fd.error);

    /*  Test legal command  */
    if(!LegalCmd(cmd, argV, pathV))
    {
        cerr<<"Unknown command: ["<<cmd<<"]."<<endl;
        exit(1);
    }


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
void DoBuildinCmd(string cmd, vector<string> argV, string &path, vector<string> &pathV)
{
    if(cmd == "printenv")
    {
        string env = argV[0];
        cout<<getenv(env.c_str())<<endl;
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
    }
    else   // exit or EOF
        exit(0);
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
void Shell()
{
    string cmdLine;
    vector<string> splitCmdLine;
    vector<string>::iterator iterLine;

    vector<struct Pipe> pipeV;

    vector<pid_t> pidV;

    string path = getenv("PATH");
    vector<string> pathV = SplitEnvPath(path, ':');
    
    cout<<"%";
    while(getline(cin, cmdLine))
    {
        splitCmdLine = SplitCmdWithSpace(cmdLine);
        iterLine = splitCmdLine.begin();
        bool isNumPipe = false;
        pidV.clear();
        
        while(iterLine != splitCmdLine.end())
        {
            Fd fd = {0, 1, 2};

            vector<string> argV;
            string cmd;
            Sign sign = None;
            int pipeNum = 0;

            IdentifyCmd(splitCmdLine, iterLine, cmd, argV, sign, pipeNum);
            
            /*  Do buildin command  */
            if(BuildCmd(cmd))
            {
                DoBuildinCmd(cmd, argV, path, pathV);
                ClosePipe(pipeV);
                ReducePipe(pipeV);
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
                    return;
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
                DoCmd(cmd, argV, pathV, fd);
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

        int cmdSize = cmdLine.length();
        if(!SpaceLine(cmdLine, cmdSize))
            ReducePipeNum(pipeV);

        cout<<"%";
    }
}
void InitEnv()
{
    setenv("PATH", "bin:.", 1);
}
int main(int argc, char* const argv[])
{
    InitEnv();
    Shell();

    return 0;
}