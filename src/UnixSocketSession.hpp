#ifndef UNIX_SOCKET_SESSION_HPP
#define UNIX_SOCKET_SESSION_HPP

#include "UnixSocket.h"

namespace UnixSocket
{

template< typename Data >
void Server::Session::send( Data&& data )
{
    ::boost::asio::async_write( m_socket, 
        ::boost::asio::buffer(
            ::std::forward<Data>(data).data(), 
            ::std::forward<Data>(data).size() ),
        [&]( const boost::system::error_code& error, ::std::size_t bytes_transferred )
        {
            if (!error)
            {
                m_parent_ptr->getConfig().m_send_cb( bytes_transferred );
                PRINTF( GRN, "%lu bytes is sent.\n", bytes_transferred );
            }
            else
            {
                PRINT_ERR( "Error when writing : %s\n", error.message().c_str());
                if( m_socket.is_open() )
                {
                    m_socket.shutdown( Socket::shutdown_send );
                }
            }
        } );
}

}

#endif /* UNIX_SOCKET_SESSION_HPP */