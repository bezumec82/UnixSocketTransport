#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "UnixSocket.h"

void serverReadCallBack( ::std::string data )
{
    ::std::cout << __func__ << " : "
                << "Server received data : " 
                << data << ::std::endl;
}

void serverSendCallBack( ::std::size_t sent_bytes )
{
    ::std::cout << __func__ << " : "
                << "Server sent " 
                << sent_bytes << "bytes." 
                << ::std::endl;
}

void clientReadCallBack( ::std::string data )
{
    ::std::cout << __func__ << " : "
                << "Client received data : " 
                << data << ::std::endl;
}

void clientSendCallBack( ::std::size_t sent_bytes )
{
    ::std::cout << __func__ << " : " 
                << "Client sent " 
                << sent_bytes << "bytes." 
                << ::std::endl;
}

#define LOOP_DELAY  ::std::chrono::milliseconds(100)

int main( int , char** )
{

    {
        ::UnixSocket::Server server;
        ::UnixSocket::Server::Config config = 
        {
            .m_recv_cb      = serverReadCallBack,
            .m_send_cb      = serverSendCallBack,
            .m_address      = "/tmp/UnixSocketServer",
            .m_delimiter    = "body",
            .m_id_key       = "auth"
        };
        server.setConfig( ::std::move( config ) ); /* No way to change config. */
        server.start();

        ::UnixSocket::Client asyncClient;
        ::UnixSocket::Client::Config asyncConfig =
        {
            .m_recv_cb      = clientReadCallBack,
            .m_send_cb      = clientSendCallBack,
            .m_address      = "/tmp/UnixSocketServer",
            .m_delimiter    = "body",
            .m_id_key       = "auth",
            .m_client_id    = "asyncClient",
            /* Client without connection seems useless */
            .m_con_type = ::UnixSocket::Client::ConnectType::ASYNC_CONNECT
        };
        asyncClient.setConfig( ::std::move( asyncConfig ) ); /* No way to change config. */
        asyncClient.start();

        ::UnixSocket::Client syncClient;
        ::UnixSocket::Client::Config syncConfig =
        {
            .m_recv_cb      = clientReadCallBack,
            .m_send_cb      = clientSendCallBack,
            .m_address      = "/tmp/UnixSocketServer",
            .m_delimiter    = "body",
            .m_id_key       = "auth",
            .m_client_id    = "syncClient",
            /* Client without connection seems useless */
            .m_con_type     = ::UnixSocket::Client::ConnectType::SYNC_CONNECT
        };
        syncClient.setConfig( ::std::move( syncConfig ) ); /* No way to change config. */
        syncClient.start();
        // while( true )

        for( int i = 0; i < 8; i++ )
        {
            PRINTF( RED, "Loop #%d.\n", i);
            ::std::this_thread::sleep_for( LOOP_DELAY );
            asyncClient.send( ::std::string{ "<body>Data from asyncClient</body>" } );
            ::std::cout << "--" << ::std::endl;

            ::std::this_thread::sleep_for( LOOP_DELAY );
            syncClient.send( ::std::string{ "<body>Data from syncClient</body>" } );
            ::std::cout << "--" << ::std::endl;

            ::std::this_thread::sleep_for( LOOP_DELAY );
            server.send< ::std::string >( 
                        ::std::string{ "syncClient" },
                        ::std::string{ "<body>Data for syncClient</body>" } );
            ::std::cout << "--" << ::std::endl;

            ::std::this_thread::sleep_for( LOOP_DELAY );
            server.send< ::std::string >( 
                        ::std::string{ "asyncClient" },  
                        ::std::string{ "<body>Data for asyncClient</body>" } );
            ::std::cout << "--" << ::std::endl;
        }
    }
    
#if( 0 )
    syncClient.~Client();
    ::std::this_thread::sleep_for( LOOP_DELAY );
    asyncClient.~Client();
    //::std::this_thread::sleep_for( LOOP_DELAY );
    server.~Server();
    ::std::this_thread::sleep_for( LOOP_DELAY );
#endif

    PRINTF( RED , "Exit main.\n" );
    return 0;
}