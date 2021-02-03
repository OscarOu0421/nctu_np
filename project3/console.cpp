#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#define MAX_SERVER 5
#define MAX_ARG 3

using namespace std;
using namespace boost::asio::ip;
using namespace boost::asio;

struct HostInfo
{
    string host;
    string port;
    string file;
    string id;
    bool alive = false;
}hostInfoV[MAX_SERVER];

void PrintHeader()
{
    cout << "Content-type: text/html\r\n\r\n";
    string head = 
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
        "<meta charset=\"UTF-8\" />\n"
        "<title>Network Programming Homework3</title>\n"
        "<link\n"
        "rel=\"stylesheet\"\n"
        "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
        "integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
        "crossorigin=\"anonymous\"\b"
        "/>\n"
        "<link\n"
        "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
        "rel=\"stylesheet\"\n"
        "/>\n"
        "<link\n"
        "rel=\"icon\"\n"
        "type=\"image/png\"\n"
        "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
        "/>\n"
        "<style>\n"
        "* {\n"
            "font-family: 'Source Code Pro', monospace;\n"
            "font-size: 1rem !important;\n"
        "}\n"
        "body {\n"
            "background-color: #212529;\n"
        "}\n"
        "pre {\n"
            "color: #cccccc;\n"
        "}\n"
        "b {\n"
            "color: #01b468;\n"
        "}\n"
        "</style>\n"
    "</head>\n";
    boost::format table(
            "<table class=\"table table-dark table-bordered\">\n"
                "<thead>\n"
                    "<tr id=\"tableHead\">\n"
                        "%1%"
                    "</tr>\n"
                "</thead>\n"
                "<tbody>\n"
                    "<tr id=\"tableBody\">\n"
                        "%2%"
                    "</tr>\n"
                "</tbody>\n"
            "<\table>\n");
    string tile = 
            "</font>\n"
        "</body>\n"
    "</html>\n";
    string title;
    string body;
    for(int i = 0 ; i < MAX_SERVER ; i++)
    {
        if(hostInfoV[i].alive)
        {
            title += "<th scope=\"col\">" + hostInfoV[i].host + ":" + hostInfoV[i].port + "</th>\n";
            body += "<td><pre id=\"" + hostInfoV[i].id + "\" class=\"mb-0\"></pre></td>\n";
        }
    }
    cout<<head + (table%title%body).str() + tile;
    cout.flush();
}
void ParseQueryString()
{
    string qString = getenv("QUERY_STRING");
    vector<string> argV;
    boost::split(argV, qString, boost::is_any_of("&"), boost::token_compress_on);
    vector<string>::iterator it = argV.begin();
    int argc = 0;
    int serverc = 0;
    while(it != argV.end()-1)   // the last one of argV is empty
    {
        /* *it = { h0=..., p0=..., f0=...} */
        vector<string> tempV;
        boost::split(tempV, *it, boost::is_any_of("="), boost::token_compress_on);
        if(tempV[1].length())
        {
            hostInfoV[serverc].alive = true;
            hostInfoV[serverc].id = "c" + to_string(serverc);
            if(argc%MAX_ARG == 0)
                hostInfoV[serverc].host = tempV[1];
            else if(argc%MAX_ARG == 1)
                hostInfoV[serverc].port = tempV[1];
            else if(argc%MAX_ARG == 2)
                hostInfoV[serverc++].file = tempV[1];
        }
        argc++;
        it++;
    }
}
class client
    : public enable_shared_from_this<client>
{
public:
    client(io_context& io_context, string id, tcp::resolver::query q, string file)
        : resolver_(io_context), socket_(io_context), id_(id), q_(move(q))
        {
            file_.open("test_case/" + file, ios::in);
        }
    void Start() { DoResolve();}
private:
    
    tcp::resolver resolver_;
    tcp::socket socket_;
    string id_;
    tcp::resolver::query q_;
    fstream file_;
    enum { maxLength = 15000 };
    char data_[maxLength];

    void ReplaceCode(string &data)
    {
        boost::algorithm::replace_all(data, "&", "&amp;");
        boost::algorithm::replace_all(data, "\"", "&quot;");
        boost::algorithm::replace_all(data, "\'", "&apos;");
        boost::algorithm::replace_all(data, "<", "&lt;");
        boost::algorithm::replace_all(data, ">", "&gt;");
        boost::algorithm::replace_all(data, "\r\n", "\n");
        boost::algorithm::replace_all(data, "\n", "<br>");
    }
    void Output(string data, bool isCmd)
    {
        ReplaceCode(data);
        ;
        if(!isCmd)
        {
            boost::format output("<script>document.all('%1%').innerHTML += '%2%';</script>");
            cout<<output%id_%data;
        }
        else
        {
            boost::format output("<script>document.all('%1%').innerHTML += '<font color = \"green\">%2%</font>';</script>");
            cout<<output%id_%data;
        }
        cout.flush();
    }
    void ReceiveHandler(size_t length)
    {
        bool isCmd = false;
        string data(data_, data_ + length);
        Output(data, isCmd);
        if(data.find("% ") != string::npos)
        {
            isCmd = true;
            string cmd;
            getline(file_, cmd);
            cmd += "\n";
            Output(cmd, isCmd);
            socket_.write_some(buffer(cmd));
        }
    }
    void ConnectHandler()
    {
        auto self(shared_from_this());
        socket_.async_receive(buffer(data_, maxLength), 
            [this, self](boost::system::error_code ec, size_t length)
            {
                if(!ec)
                    ReceiveHandler(length);
                ConnectHandler();
            });
    }
    void ResolveHandler(tcp::resolver::iterator it)
    {
        auto self(shared_from_this());
        socket_.async_connect(*it, 
            [this, self](boost::system::error_code ec)
            {
                if(!ec)
                    ConnectHandler();
            });
    }
    void DoResolve()
    {
        auto self(shared_from_this());
        resolver_.async_resolve(q_, 
            [this, self](boost::system::error_code ec, tcp::resolver::iterator it)
            {
                if(!ec)
                    ResolveHandler(it);
            });
    }
};
void PrintHostInfo()
{
    string msg = "----------HostInfo------------\n";
    for(int i=0; i<MAX_SERVER; i++)
    {
        if(hostInfoV[i].alive)
        {
            msg+="Host" + to_string(i) + ":" + hostInfoV[i].host + "  port" + to_string(i) + ":" + hostInfoV[i].port + " file" + to_string(i) + ":" + hostInfoV[i].file + "\n";
        }
    }
    msg+="------------------------------\n";
    cerr<<msg;
}
int main(int argc, const char* argv[])
{
    ParseQueryString();
    PrintHostInfo();
    PrintHeader();
    io_context io_context;
    for(int i=0; i<MAX_SERVER; i++)
    {
        if(hostInfoV[i].alive)
        {
            tcp::resolver::query q(hostInfoV[i].host, hostInfoV[i].port);
            make_shared<client>(io_context, hostInfoV[i].id, move(q), hostInfoV[i].file)->Start();
        }
    }
    io_context.run();
    return 0;
}