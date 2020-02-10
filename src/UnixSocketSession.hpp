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
            if ( ! error )
            {
                m_parent_ptr->getConfig().m_send_cb( m_client_id, bytes_transferred );
                PRINTF( GRN, "%lu bytes is sent.\n", bytes_transferred );
            }
            else
            {
                PRINT_ERR( "Error when writing : %s\n", error.message().c_str() );
                if( m_socket.is_open() )
                {
                    m_socket.shutdown( Socket::shutdown_send );
                }
                m_parent_ptr->getConfig().m_error_cb( m_client_id, error.message().c_str() );
                m_is_valid.store( false );
                // m_io_service_ref.post( \
                //     ::std::bind( &Server::serveSessions , m_parent_ptr ) );
                m_io_service_ref.post( \
                    ::std::bind( &Server::removeSession, m_parent_ptr, m_self) );
                return;
            }
        } );
}

}

#endif /* UNIX_SOCKET_SESSION_HPP */