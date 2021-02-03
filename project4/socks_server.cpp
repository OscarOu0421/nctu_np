#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#define REQUEST_INDEX 7
#define REPLY_INDEX 5

using namespace std;
using namespace boost::asio::ip;
using namespace boost::asio;
io_context io;

class request
{
    public:
        enum cmdType { connect = 0x01, bind = 0x02 };
        unsigned char version_;
        unsigned char command_;
        unsigned char destPortHigh_;
        unsigned char destPortLow_;
        address_v4::bytes_type destIP_;
        string userID_;
        unsigned char Null_;
        unsigned char domainName_;
        // boost::array<unsigned char, 512> domainName_;
        // address_v4::bytes_type  domainName1_;
        // address_v4::bytes_type  domainName2_;
        // address_v4::bytes_type  domainName3_;
        // unsigned char domainName1_;
        // unsigned char domainName2_;
        // unsigned char domainName3_;
        // unsigned char domainName4_;
        // unsigned char domainName5_;
        unsigned char socks4aNull_;

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
        boost::array<mutable_buffer, 1> socks4aBuffers()
        {
            boost::array<mutable_buffer, 1> request = {
                buffer(&domainName_, 1)
            };
            return request;
        }
        unsigned short GetPort()
        {
            unsigned short port = destPortHigh_;
            port = (port<<8) & 0xff00;
            port |= destPortLow_;
            return port;
        }
        void PrintRequest()
        {
            cout<<"----------Request----------"<<endl;
            cout<<"<Version>: "<<unsigned(version_)<<endl;
            cout<<"<Command>: "<<unsigned(command_)<<endl;
            cout<<"<DestPort>: "<<to_string(GetPort())<<endl;
            cout<<"<DestIP>: "<<address_v4(destIP_).to_string()<<endl;
            cout<<"<UserID>: "<<userID_<<endl;
            cout<<"<NULL>: "<<unsigned(Null_)<<endl;
            cout<<"---------------------------"<<endl<<endl;
        }
        bool IsSocks4a()
        {
            string destIP = address_v4(destIP_).to_string();
            vector<string> ipV;
            boost::split(ipV, destIP, boost::is_any_of("."), boost::token_compress_on);
            vector<string>::iterator it = ipV.begin();
            int num = 0;
            bool socks4a = false;
            while(it != ipV.end())                          
            {
                string subIP = *it;  
                if(num < 3 && subIP == "0")
                    socks4a = true;
                else if(num == 3 && subIP.size() ==1 && subIP != "0")
                    socks4a = true;
                else
                    return false;
                num++;
                it++;
            }
            return socks4a;
        }
};
class reply
{
    public:
        enum statusType { Accept = 0x5a, Reject = 0x5b};
        reply(statusType status, request req)
            :version_(0), 
            status_(status),
            destPortHigh_(req.destPortHigh_),
            destPortLow_(req.destPortLow_),
            destIP_(req.destIP_) {}

        boost::array<const_buffer, REPLY_INDEX> buffers()
        {
            boost::array<const_buffer, REPLY_INDEX> reply = {
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
class server
{
    public:
        server(unsigned short port)
            :acceptor_(io, tcp::endpoint(tcp::v4(), port))
            {
                SignalWait();
                Accept();
            }
    private:
        tcp::acceptor acceptor_;
        tcp::resolver resolver_{io};
        tcp::socket sourSocket_{io};
        tcp::socket destSocket_{io};
        signal_set signal_{io};
        request req_;
        array<unsigned char, 65536> destBuf_{};
        array<unsigned char, 65536> sourBuf_{};
        string hostName_ = "";
        
        void SignalWait()
        {
            signal_.async_wait(boost::bind(&server::WaitHandle, this));
        }
        void WaitHandle()
        {
            if(acceptor_.is_open())
            {
                int status = 0;
                while(waitpid(-1, &status, WNOHANG)){}
                SignalWait();
            }
        }
        void Accept()
        {
            acceptor_.async_accept(sourSocket_, boost::bind(&server::AcceptHandle, this, _1));
        }
        void AcceptHandle(const boost::system::error_code& ec)
        {
            if(!ec)
            {
                io.notify_fork(boost::asio::io_context::fork_prepare);
                pid_t pid;
                while((pid = fork()) <0)
                {
                    /*  parent process no resource to fork child process, so wait for child exit and release recourse   */
                    int status = 0;
                    waitpid(-1, &status, 0);
                }
                if(pid == 0)
                {
                    io.notify_fork(boost::asio::io_context::fork_child);
                    signal_.cancel();
                    acceptor_.close();

                    string cmd, destPort, destIP, status;

                    RecvSock4Request();
                    GetDestInfo(cmd, destPort, destIP);
                    if(PassFirewall(cmd, destIP))
                    {
                        status = "Accept";
                        PrintMsg(destIP, destPort, cmd, status);
                        if(cmd == "Connect")
                        {
                            if(req_.IsSocks4a())
                            {
                                RecvSock4aRequest();
                                Resolve(tcp::resolver::query(hostName_, destPort));
                            }   
                            else
                                Resolve(tcp::resolver::query(destIP, destPort));
                        }
                        else
                        {
                            tcp::acceptor bindAcceptor{io};
                            tcp::endpoint endpoint(tcp::v4(), 0);
                            
                            Bind(bindAcceptor, endpoint);

                            req_.destPortHigh_ = bindAcceptor.local_endpoint().port() >>8;
                            req_.destPortLow_ = bindAcceptor.local_endpoint().port();
                            req_.destIP_ = make_address_v4("0.0.0.0").to_bytes();
                            Reply();

                            bindAcceptor.accept(destSocket_);
                            
                            Reply();

                            CommFromDest();
                            CommFromSour();
                        }
                    }
                    else
                    {
                        status = "Reject";
                        PrintMsg(destIP, destPort, cmd, status);
                        reply rep(reply::Reject, req_);
                        // rep.PrintReply();
                        write(sourSocket_, rep.buffers());
                    }
                    
                }
                else
                {
                    io.notify_fork(boost::asio::io_context::fork_parent);
                    sourSocket_.close();
                    destSocket_.close();
                    
                    Accept();
                }
            }
            else
            {
                cerr<<"Fail to accept, error: "<<ec.message()<<endl;
                Accept();
            }
        }
        void RecvSock4Request()
        {
            boost::system::error_code error;
            read(sourSocket_, req_.buffers(), error);
            // req_.PrintRequest();

            if(error == error::eof)
                return; // Connection closed cleanly by peer.
            else if(error)
                throw boost::system::system_error(error); // Some other error.
        }
        void RecvSock4aRequest()
        {
            boost::system::error_code error;
            while(1)
            {
                read(sourSocket_, buffer(&req_.domainName_, 1), error);
                cout<<unsigned(req_.domainName_)<<" ";
                if(req_.domainName_ == 0x00)
                    break;
                hostName_ += char(req_.domainName_);
            }

            if(error == error::eof)
                return; // Connection closed cleanly by peer.
            else if(error)
                throw boost::system::system_error(error); // Some other error.
        }
        void GetDestInfo(string &cmd, string &destPort, string &destIP)
        {
            cmd = (req_.command_ == req_.connect) ? "Connect" : "Bind";
            destPort = to_string(req_.GetPort());
            destIP = address_v4(req_.destIP_).to_string();
        }
        bool PassFirewall(string cmd, string destIP)
        {
            ifstream firewallStream("./socks.conf");
            string line;
            string permitType = (cmd == "Connect") ? "permit c " : "permit b ";
            vector<string> permitV;

            while(getline(firewallStream, line))
            {
                if(line.substr(0, permitType.size()) == permitType)
                    permitV.push_back(line.substr(permitType.size()));
            }

            vector<string>::iterator it = permitV.begin();
            while(it != permitV.end())
            {
                string acceptIP = (*it).substr(0, (*it).find('*'));
                if(destIP.substr(0, acceptIP.size()) == acceptIP)
                    return true;
                it++;
            }
            return false;
        }
        void PrintMsg(string destIP, string destPort, string cmd, string status)
        {
            // cout<<"----------SocksServerMsg----------"<<endl;
            cout<<"<S_IP>: "<<sourSocket_.remote_endpoint().address().to_string()<<endl;
            cout<<"<S_PORT>: "<<to_string(sourSocket_.remote_endpoint().port())<<endl;
            cout<<"<D_IP>: "<<destIP<<endl;
            cout<<"<D_PORT>: "<<destPort<<endl;
            cout<<"<Command>: "<<cmd<<endl;
            cout<<"<Reply>: "<<status<<endl<<endl;
            // cout<<"----------------------------------"<<endl<<endl;
        }
        void Resolve(tcp::resolver::query q)
        {
            resolver_.async_resolve(q,
                [this](const boost::system::error_code& ec, tcp::resolver::iterator it) 
                {
                    if (!ec)
                        Connect(it);
                });
        }
        void Connect(tcp::resolver::iterator it)
        {
            destSocket_.async_connect(*it, 
                [this](const boost::system::error_code& ec) 
                {
                    if(!ec) 
                    {
                        Reply();
                        CommFromDest();
                        CommFromSour();
                    }
                });
        }
        void Bind(tcp::acceptor &bindAcceptor, tcp::endpoint &endpoint)
        {
            bindAcceptor.open(endpoint.protocol());
            bindAcceptor.bind(endpoint);
            bindAcceptor.listen();
            bindAcceptor.local_endpoint().port();
        }
        void Reply()
        {
            reply rep(reply::Accept, req_);
            // rep.PrintReply();
            write(sourSocket_, rep.buffers());
        }
        void CommFromDest()
        {
            destSocket_.async_receive(buffer(destBuf_),
                [this](boost::system::error_code ec, size_t length) 
                {
                    if(!ec || ec == boost::asio::error::eof) 
                        CommToSour(length);
                    else
                        throw system_error{ec};
                });
        }
        void CommToSour(size_t length)
        {
            async_write(sourSocket_, buffer(destBuf_, length),
                [this](boost::system::error_code ec, size_t length) 
                {
                    if(!ec)
                        CommFromDest();
                    else
                        throw system_error{ec};
                });
        }
        void CommFromSour()
        {
            sourSocket_.async_receive(buffer(sourBuf_),
                [this](boost::system::error_code ec, size_t length) 
                {
                    if(!ec || ec == boost::asio::error::eof) 
                        CommToDest(length);
                    else
                        throw system_error{ec};
                });
        }
        void CommToDest(size_t length)
        {
            async_write(destSocket_, buffer(sourBuf_, length),
                [this](boost::system::error_code ec, size_t length) 
                {
                    if(!ec)
                        CommFromSour();
                    else
                        throw system_error{ec};
                });
        }
};
int main(int argc, char* const argv[])
{
    if(argc!=2)
    {
        cout<<"Usage: ./socks_server [port]"<<endl;
        exit(1);
    }
    int port = atoi(argv[1]);
    
    server s(port);
    io.run();

    return 0;
}