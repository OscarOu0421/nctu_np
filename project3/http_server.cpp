#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace boost::asio::ip;
using namespace boost::asio;

class client
    : public std::enable_shared_from_this<client>   
{
public:
    client(tcp::socket socket)
        : socket_(move(socket)){}

    void Start()  { DoRead();}

private:
    tcp::socket socket_;
    enum { maxLength = 1024 };
    char data_[maxLength];
    pid_t pid_;
    struct Request_
    {
        string requestMethod = "";
        string requestUrl = "";
        string queryString = "";
        string serverProtocol = "";
        string httpHost = "";
        string serverAddr = "";
        string serverPort = "";
        string remoteAddr = "";
        string remotePort = "";
        string cgi = "";
    };
    Request_ request_;

    void DoRead()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, maxLength),
            [this, self](boost::system::error_code ec, size_t length)
            {
                if (!ec)
                {
                    ParseRequest();
                    PrintRequest();
                    DoRequest();
                }
            });
    }
    void ParseRequest()
    {
        vector<string> requestV;
        string data = string(data_);
        boost::split(requestV, data, boost::is_any_of(" \r\n"), boost::token_compress_on);
        vector<string>::iterator it = requestV.begin();
        int count=1;
        while(it!=requestV.end())
        {
            if(count == 1)
                request_.requestMethod = *it;
            else if(count == 2)
                request_.requestUrl = *it;
            else if(count == 3)
                request_.serverProtocol = *it;
            else if(count == 5)
                request_.httpHost = *it;
            count++;
            it++;
        }
        if(strstr(request_.requestUrl.c_str(), "?") != NULL)
        {
            vector<string> queryV;
            boost::split(queryV, request_.requestUrl, boost::is_any_of("?"), boost::token_compress_on);
            request_.cgi = queryV[0];
            request_.queryString = queryV[1];
        }
        else
            request_.cgi = request_.requestUrl;

        request_.serverAddr = socket_.local_endpoint().address().to_string();
        request_.serverPort = to_string(socket_.local_endpoint().port());
        request_.remoteAddr = socket_.remote_endpoint().address().to_string();
        request_.remotePort = to_string(socket_.remote_endpoint().port());
    }
    void SetEnvVar()
    {
        setenv("REQUEST_METHOD", request_.requestMethod.c_str(), 1);
        setenv("REQUEST_URI", request_.requestUrl.c_str(), 1);
        setenv("QUERY_STRING", request_.queryString.c_str(), 1);
        setenv("SERVER_PROTOCOL", request_.serverProtocol.c_str(), 1);
        setenv("HTTP_HOST", request_.httpHost.c_str(), 1);
        setenv("SERVER_ADDR", request_.serverAddr.c_str(), 1);
        setenv("SERVER_PORT", request_.serverPort.c_str(), 1);
        setenv("REMOTE_ADDR", request_.remoteAddr.c_str(), 1);
        setenv("REMOTE_PORT", request_.remotePort.c_str(), 1);
    }
    void Dup()
    {
        dup2(socket_.native_handle(), 0);
        dup2(socket_.native_handle(), 1);
        close(socket_.native_handle());
    }
    void Exec()
    {
        cout << "HTTP/1.1 200 OK\r\n" ;
        cout << "Content-Type: text/html\r\n" ;
        fflush(stdout);

        string path = "." + request_.cgi;
        char** argv = new char*[2];
        argv[0] = new char[request_.cgi.size()+1];
        strcpy(argv[0], request_.cgi.c_str());
        argv[1] = NULL; 
        if(execv(path.c_str(), argv)<0)
        {
            cerr<<"Fail to exec cgi, errno: "<<errno<<endl;
            exit(1);
        }
    }
    void DoRequest()
    {
        if((pid_ = fork())<0)
        {
            cout<<"Fail to fork, errno: "<<errno<<endl;
            exit(1);
        }
        else if(pid_ == 0)
        {
            SetEnvVar();
            Dup();
            Exec();
            exit(0);
        }
        else
        {
            int status = 0;
            waitpid(pid_, &status, 0);
            if (status != 0)
            {
                cout<<"Fail to wait pid, errno: "<<errno<<endl;
                exit(-1);
            }
            socket_.close();
        }
    }
    void PrintRequest()
    {
        cout<<"----------Environment--------"<<endl;
        cout<<"Method method: "<<request_.requestMethod<<endl;
        cout<<"Request url: "<<request_.requestUrl<<endl;
        cout<<"Query string: "<<request_.queryString<<endl;
        cout<<"Server protocol: "<<request_.serverProtocol<<endl;
        cout<<"Http host: "<<request_.httpHost<<endl;
        cout<<"Server addr: "<<request_.serverAddr<<endl;
        cout<<"Server port: "<<request_.serverPort<<endl;
        cout<<"Remote addr: "<<request_.remoteAddr<<endl;
        cout<<"Remote port: "<<request_.remotePort<<endl;
        cout<<"-----------------------------"<<endl;
    }
};
class server
{
public:
    server(io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){ DoAccept();}

private:
    tcp::acceptor acceptor_;

    void DoAccept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if(!ec)  
                    make_shared<client>(move(socket))->Start();
                DoAccept();
            });
    }
};
int main(int argc, char* const argv[])
{
    if(argc!=2)
    {
        cout<<"Usage: ./http_server [port]"<<endl;
        exit(1);
    }
    int port = atoi(argv[1]);

    /*  kill zombie */
    signal(SIGCHLD, SIG_IGN);
    
    io_context io_context;
    server s(io_context, port);
    io_context.run();

    return 0;
}