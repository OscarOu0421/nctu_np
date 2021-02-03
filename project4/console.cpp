#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/array.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#define MAX_SERVER 5
#define REQUEST_INDEX 7
#define REPLY_INDEX 5

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

struct SocksInfo
{
    string host;
    string port;
}socksInfo;

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
            "</table>\n");
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
    int serverNum = 0;

    while(it != argV.end())   // the last one of argV is empty
    {
        /* *it = { h0=..., p0=..., f0=..., sh=..., sp=...} */
        string key = (*it).substr(0, (*it).find("="));
        string value = (*it).substr(key.size()+1);
        
        if(!value.empty())
        {
            if(key.front() == 'h')
            {
                hostInfoV[serverNum].alive = true;
                hostInfoV[serverNum].id = "c" + to_string(serverNum);
                hostInfoV[serverNum].host = value;
            }
            else if(key.front() == 'p')
                hostInfoV[serverNum].port = value;
            else if(key.front() == 'f')
                hostInfoV[serverNum++].file = value;
            else if(key == "sh")
                socksInfo.host = value;
            else if(key == "sp")
                socksInfo.port = value;
        }
        it++;
    }
}
class reply
{
    public:
        enum statusType { Accept = 0x5a, Reject = 0x5b};
        
        reply() {}
        boost::array<mutable_buffer, REPLY_INDEX> buffers()
        {
            boost::array<mutable_buffer, REPLY_INDEX> reply = {
                buffer(&version_, 1),
                buffer(&status_, 1),
                buffer(&destPortHigh_, 1),
                buffer(&destPortLow_, 1),
                buffer(destIP_),
            };
            return reply;
        }
        void PrintReply()
        {
            cout<<"----------Reply----------"<<endl;
            cout<<"<Version>: "<<unsigned(version_)<<endl;
            cout<<"<Command>: "<<unsigned(status_)<<endl;
            cout<<"<DestPort>: "<<unsigned(destPortHigh_*256 + destPortLow_)<<endl;
            cout<<"<DestIP>: "<<address_v4(destIP_).to_string()<<endl;
            cout<<"-------------------------"<<endl<<endl;
        }
    private:
        unsigned char version_;
        unsigned char status_;
        unsigned char destPortHigh_;
        unsigned char destPortLow_;
        address_v4::bytes_type destIP_{};
};
class request
{
    public:
        enum cmdType { connect = 0x01, bind = 0x02 };
        unsigned char version_;
        unsigned char command_;
        unsigned short destPort_;
        unsigned char destPortHigh_;
        unsigned char destPortLow_;
        address_v4::bytes_type destIP_;
        string userID_;
        unsigned char Null_;

        request(cmdType command, tcp::endpoint& endpoint)
            :version_(0x04), command_(command), destPort_(endpoint.port())
            {
                /*  Check socks4 only   */
                if (endpoint.protocol() != tcp::v4()) 
                    throw boost::system::system_error(error::address_family_not_supported);

                destPortHigh_ = (destPort_ >> 8) & 0xff;
                destPortLow_ = destPort_ & 0xff;

                destIP_ = endpoint.address().to_v4().to_bytes();
                userID_ = "";
                Null_ = 0x00;
            }

        boost::array<mutable_buffer, REQUEST_INDEX> buffers()
        {
            boost::array<mutable_buffer, REQUEST_INDEX> request = {
                buffer(&version_, 1),
                buffer(&command_, 1),
                buffer(&destPortHigh_, 1),
                buffer(&destPortLow_, 1),
                buffer(destIP_),
                buffer(userID_),
                buffer(&Null_, 1)
            };
            return request;
        }
        void PrintRequest()
        {
            cout<<"----------Request----------"<<endl;
            cout<<"<Version>: "<<unsigned(version_)<<endl;
            cout<<"<Command>: "<<unsigned(command_)<<endl;
            cout<<"<DestPort>: "<<unsigned(destPort_)<<endl;
            cout<<"<DestIP>: "<<address_v4(destIP_).to_string()<<endl;
            cout<<"<High>: "<<unsigned(destPortHigh_)<<endl;
            cout<<"<Low>: "<<unsigned(destPortLow_)<<endl;
            cout<<"<UserID>: "<<userID_<<endl;
            cout<<"<NULL>: "<<unsigned(Null_)<<endl;
            cout<<"---------------------------"<<endl<<endl;
        }
};
class client
    : public enable_shared_from_this<client>
{
public:
    client(io_context& io_context, string id, tcp::resolver::query socksQuery, tcp::resolver::query httpQuery, string file)
        : resolver_(io_context), socket_(io_context), id_(id), socksQuery_(move(socksQuery)), httpQuery_(move(httpQuery))
        {
            file_.open("test_case/" + file, ios::in);
        }
    void Start() { DoResolve();}
private:
    
    tcp::resolver resolver_;
    tcp::socket socket_;
    string id_;
    tcp::resolver::query socksQuery_;
    tcp::resolver::query httpQuery_;
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
    void DoReceive()
    {
        auto self(shared_from_this());
        socket_.async_receive(buffer(data_, maxLength), 
            [this, self](boost::system::error_code ec, size_t length)
            {
                if(!ec)
                    ReceiveHandler(length);
                DoReceive();
            });
    }
    void DoResolve()
    {
        /*  Connect to socks    */
        tcp::resolver::iterator socksIter = resolver_.resolve(socksQuery_);
        socket_.connect(*socksIter);

        /*  Create http request and send to socks   */
        tcp::endpoint httpEndpoint = *resolver_.resolve(httpQuery_);
        request socksRequest(request::cmdType::connect, httpEndpoint);
        write(socket_, socksRequest.buffers(), transfer_all());
        socksRequest.PrintRequest();

        /*  Create socks reply buffer to read    */
        reply socksReply;
        read(socket_, socksReply.buffers(), transfer_all());
        socksReply.PrintReply();

        DoReceive();
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
    msg+="Socks server: " + socksInfo.host + ":" + socksInfo.port + "\n";
    msg+="------------------------------\n";
    boost::format output("<script>document.all('%1%').innerHTML += '%2%';</script>");
    string id = "c0";
    cout<<output%id%msg;
}
int main(int argc, const char* argv[])
{
    ParseQueryString();
    PrintHeader();
    // PrintHostInfo();
    io_context io_context;
    for(int i=0; i<MAX_SERVER; i++)
    {
        if(hostInfoV[i].alive)
        {
            tcp::resolver::query socksQuery(socksInfo.host, socksInfo.port);
            tcp::resolver::query httpQuery(hostInfoV[i].host, hostInfoV[i].port);
            make_shared<client>(io_context, hostInfoV[i].id, move(socksQuery), move(httpQuery), hostInfoV[i].file)->Start();
        }
    }
    io_context.run();
    return 0;
}