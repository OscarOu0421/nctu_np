#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#define MAX_SERVER 5
#define MAX_ARG 3

using namespace std;
using namespace boost::asio::ip;
using namespace boost::asio;

boost::asio::io_context ioc;

class Client
    : public enable_shared_from_this<Client>
{
public:
    Client(tcp::socket socket, string id, tcp::resolver::query q, string file)
        : clientSocket_(move(socket)), id_(id), q_(move(q))
    {
        file_.open("test_case/" + file, ios::in);
    }
    void Start() { DoResolve(); }

private:
    tcp::socket clientSocket_;
    string id_;
    tcp::resolver::query q_;
    fstream file_;
    tcp::socket serverSocket_{ioc};
    tcp::resolver resolver_{ioc};
    enum
    {
        maxLength = 15000
    };
    char data_[maxLength];

    void DoResolve()
    {
        auto self(shared_from_this());
        resolver_.async_resolve(q_,
                                [this, self](boost::system::error_code ec, tcp::resolver::iterator it) {
                                    if (!ec)
                                        ResolveHandler(it);
                                });
    }
    void ResolveHandler(tcp::resolver::iterator it)
    {
        auto self(shared_from_this());
        serverSocket_.async_connect(*it,
                                    [this, self](boost::system::error_code ec) {
                                        if (!ec)
                                            ConnectHandler();
                                    });
    }
    void ConnectHandler()
    {
        auto self(shared_from_this());
        serverSocket_.async_receive(buffer(data_, maxLength),
                                    [this, self](boost::system::error_code ec, std::size_t length) {
                                        if (!ec)
                                            ReceiveHandler(length);
                                        ConnectHandler();
                                    });
    }
    void ReceiveHandler(std::size_t length)
    {
        bool isCmd = false;
        string data(data_, data_ + length);
        boost::asio::write(clientSocket_, buffer(Output(data, isCmd)));
        if (data.find("% ") != string::npos)
        {
            isCmd = true;
            string cmd;
            getline(file_, cmd);
            cmd += "\n";
            boost::asio::write(clientSocket_, buffer(Output(cmd, isCmd)));
            serverSocket_.write_some(buffer(cmd));
        }
    }
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
    string Output(string data, bool isCmd)
    {
        ReplaceCode(data);
        ;
        if (!isCmd)
        {
            boost::format output("<script>document.all('%1%').innerHTML += '%2%';</script>");
            return (output % id_ % data).str();
        }
        else
        {
            boost::format output("<script>document.all('%1%').innerHTML += '<font color = \"green\">%2%</font>';</script>");
            return (output % id_ % data).str();
        }
    }
};
class Session
    : public enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket)
        : socket_(move(socket)) {}

    void Start() { DoRead(); }

private:
    tcp::socket socket_;
    enum
    {
        maxLength = 4096
    };
    char data_[maxLength];
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
    } request_;
    struct HostInfo
    {
        string host;
        string port;
        string file;
        string id;
        bool alive = false;
    } hostInfo_[MAX_SERVER];

    void DoRead()
    {
        auto self(shared_from_this());
        socket_.async_read_some(buffer(data_, maxLength),
                                [this, self](boost::system::error_code ec, std::size_t length) {
                                    if (!ec)
                                    {
                                        ParseRequest();
                                        PrintRequest();
                                        if (request_.cgi == "/panel.cgi")
                                            DoPanel();
                                        else
                                            DoConsole();
                                    }
                                });
    }
    void DoWrite(string html)
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, buffer(html.c_str(), html.length()),
                                 [this, self](boost::system::error_code ec, std::size_t length) {
                                     if (!ec)
                                     {
                                     }
                                 });
    }
    void ParseRequest()
    {
        vector<string> requestV;
        string data = string(data_);
        boost::split(requestV, data, boost::is_any_of(" \r\n"), boost::token_compress_on);
        vector<string>::iterator it = requestV.begin();
        int count = 1;
        while (it != requestV.end())
        {
            if (count == 1)
                request_.requestMethod = *it;
            else if (count == 2)
                request_.requestUrl = *it;
            else if (count == 3)
                request_.serverProtocol = *it;
            else if (count == 5)
                request_.httpHost = *it;
            count++;
            it++;
        }
        if (strstr(request_.requestUrl.c_str(), "?") != NULL)
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
    void PrintRequest()
    {
        cout << "----------Environment--------" << endl;
        cout << "Method method: " << request_.requestMethod << endl;
        cout << "Request url: " << request_.requestUrl << endl;
        cout << "Query string: " << request_.queryString << endl;
        cout << "Server protocol: " << request_.serverProtocol << endl;
        cout << "Http host: " << request_.httpHost << endl;
        cout << "Server addr: " << request_.serverAddr << endl;
        cout << "Server port: " << request_.serverPort << endl;
        cout << "Remote addr: " << request_.remoteAddr << endl;
        cout << "Remote port: " << request_.remotePort << endl;
        cout<<"Cgi: "<<request_.cgi<<endl;
        cout << "-----------------------------" << endl;
    }
    void DoPanel()
    {
        cout<<"Do panel"<<endl;
        string html =
        "HTTP/1.1 200 OK\r\n"
        "Content-type: text/html\r\n\r\n"
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "  <head>\n"
        "    <title>NP Project 3 Panel</title>\n"
        "    <link\n"
        "      rel=\"stylesheet\"\n"
        "       href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
        "       integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
        "       crossorigin=\"anonymous\"\n"
        "    />\n"
        "    <link\n"
        "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
        "      rel=\"stylesheet\"\n"
        "    />\n"
        "    <link\n"
        "      rel=\"icon\"\n"
        "      type=\"image/png\"\n"
        "href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\n"
        "    />\n"
        "    <style>\n"
        "      * {\n"
        "        font-family: 'Source Code Pro', monospace;\n"
        "      }\n"
        "    </style>\n"
        "  </head>\n"
        "  <body class=\"bg-secondary pt-5\">"
        "<form action=\"console.cgi\" method=\"GET\">\n"
        "      <table class=\"table mx-auto bg-light\" style=\"width: "
        "inherit\">\n"
        "        <thead class=\"thead-dark\">\n"
        "          <tr>\n"
        "            <th scope=\"col\">#</th>\n"
        "            <th scope=\"col\">Host</th>\n"
        "            <th scope=\"col\">Port</th>\n"
        "            <th scope=\"col\">Input File</th>\n"
        "          </tr>\n"
        "        </thead>\n"
        "        <tbody>";

        boost::format table(
        "          <tr>\n"
        "            <th scope=\"row\" class=\"align-middle\">Session %1%</th>\n"
        "            <td>\n"
        "              <div class=\"input-group\">\n"
        "                <select name=\"h%2%\" class=\"custom-select\">\n"
        "                   <option></option>"
        "                   <option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>"
                            "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>"
                            "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>"
                            "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>"
                            "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>"
                            "<option value=\"nplinux6.cs.nctu.edu.tw\">nplinux6</option>"
                            "<option value=\"nplinux7.cs.nctu.edu.tw\">nplinux7</option>"
                            "<option value=\"nplinux8.cs.nctu.edu.tw\">nplinux8</option>"
                            "<option value=\"nplinux9.cs.nctu.edu.tw\">nplinux9</option>"
                            "<option value=\"nplinux10.cs.nctu.edu.tw\">nplinux10</option>"
                            "<option value=\"nplinux11.cs.nctu.edu.tw\">nplinux11</option>"
                            "<option value=\"nplinux12.cs.nctu.edu.tw\">nplinux12</option>"
        "                </select>\n"
        "                <div class=\"input-group-append\">\n"
        "                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
        "                </div>\n"
        "              </div>\n"
        "            </td>\n"
        "            <td>\n"
        "              <input name=\"p%2%\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
        "            </td>\n"
        "            <td>\n"
        "              <select name=\"f%2%\" class=\"custom-select\">\n"
        "                   <option></option>"
                            "<option value=\"t1.txt\">t1.txt</option>"
                            "<option value=\"t2.txt\">t2.txt</option>"
                            "<option value=\"t3.txt\">t3.txt</option>"
                            "<option value=\"t4.txt\">t4.txt</option>"
                            "<option value=\"t5.txt\">t5.txt</option>"
                            "<option value=\"t6.txt\">t6.txt</option>"
                            "<option value=\"t7.txt\">t7.txt</option>"
                            "<option value=\"t8.txt\">t8.txt</option>"
                            "<option value=\"t9.txt\">t9.txt</option>"
                            "<option value=\"t10.txt\">t10.txt</option>"
        "              </select>\n"
        "            </td>\n"
        "          </tr>");
        for (int i = 0; i < MAX_SERVER; ++i)
            html += (table % (i + 1) % i).str();
        html +=
        "          <tr>\n"
        "            <td colspan=\"3\"></td>\n"
        "            <td>\n"
        "              <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\n"
        "            </td>\n"
        "          </tr>\n"
        "        </tbody>\n"
        "      </table>\n"
        "    </form>\n"
        "  </body>\n"
        "</html>";
        DoWrite(html);
    }
    void DoConsole()
    {
        ParseQueryString(request_.queryString);
        PrintHostInfo();
        ShowConsole();
        for (int i = 0; i < MAX_SERVER; i++)
        {
            if (hostInfo_[i].alive)
            {
                tcp::resolver::query q(hostInfo_[i].host, hostInfo_[i].port);
                make_shared<Client>(move(socket_), hostInfo_[i].id, move(q), hostInfo_[i].file)->Start();
            }
        }
    }
    void ParseQueryString(string qString)
    {
        vector<string> argV;
        boost::split(argV, qString, boost::is_any_of("&"), boost::token_compress_on);
        vector<string>::iterator it = argV.begin();
        int argc = 0;
        int serverc = 0;
        while (it != argV.end() - 1) // the last one of argV is empty
        {
            /* *it = { h0=..., p0=..., f0=...} */
            vector<string> tempV;
            boost::split(tempV, *it, boost::is_any_of("="), boost::token_compress_on);
            if (tempV[1].length())
            {
                hostInfo_[serverc].alive = true;
                hostInfo_[serverc].id = "c" + to_string(serverc);
                if (argc % MAX_ARG == 0)
                    hostInfo_[serverc].host = tempV[1];
                else if (argc % MAX_ARG == 1)
                    hostInfo_[serverc].port = tempV[1];
                else if (argc % MAX_ARG == 2)
                    hostInfo_[serverc++].file = tempV[1];
            }
            argc++;
            it++;
        }
    }
    void PrintHostInfo()
    {
        string msg = "----------HostInfo------------\n";
        for (int i = 0; i < MAX_SERVER; i++)
        {
            if (hostInfo_[i].alive)
            {
                msg += "Host" + to_string(i) + ":" + hostInfo_[i].host + "  port" + to_string(i) + ":" + hostInfo_[i].port + " file" + to_string(i) + ":" + hostInfo_[i].file + "\n";
            }
        }
        msg += "------------------------------\n";
        cout << msg;
    }
    void ShowConsole()
    {
        string head =
            "HTTP/1.1 200 OK\r\n"
            "Content-type: text/html\r\n\r\n"
        "<html>\n"
        "<head>\n"
        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n"
        "<link rel=\"icon\" href=\"data:;base64,iVBORw0KGgo=\">\n"
        "<title>Network Programming Homework 3</title>\n"
        "</head>\n"
        "<body>\n"
        "<font face=\"Courier New\" size=2 color=#FFFFFF>\n";
        boost::format table(
            "<table>\n"
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
        for (int i = 0; i < MAX_SERVER; i++)
        {
            if (hostInfo_[i].alive)
            {
                title += "<th>" + hostInfo_[i].host + ":" + hostInfo_[i].port + "</th>\n";
                body += "<td valign=\"top\" id=\"" + hostInfo_[i].id + "\"></td>\n";
            }
        }
        string html = head + (table % title % body).str() + tile;
        DoWrite(html);
    }
};
class Server
{
public:
    Server(short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
    {
        DoAccept();
    }

private:
    tcp::acceptor acceptor_;

    void DoAccept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec)
                    make_shared<Session>(move(socket))->Start();
                DoAccept();
            });
    }
};
int main(int argc, char *const argv[])
{
    if (argc != 2)
    {
        cout << "Usage: ./http_server [port]" << endl;
        exit(1);
    }
    int port = atoi(argv[1]);

    Server s(port);
    ioc.run();

    return 0;
}