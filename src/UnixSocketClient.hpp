#ifndef UNIX_SOCKET_CLIENT_HPP
#define UNIX_SOCKET_CLIENT_HPP

#include "UnixSocket.h"

namespace UnixSocket
{

template< typename Data >
void Client::send( Data&& data )
{
    ::boost::asio::async_write( * m_socket_uptr,
        ::boost::asio::buffer(
            ::std::forward<Data>(data).data(), 
            ::std::forward<Data>(data).size() ),
        [&]( const boost::system::error_code& error, ::std::size_t bytes_transferred )
        {
            if ( !error ) /* All good */
            {
                m_config.m_send_cb( bytes_transferred );
                PRINTF( GRN, "%lu bytes is sent.\n", bytes_transferred );
            }
            else
            {
                PRINT_ERR( "Error when writing : %s\n", error.message().c_str() );
                if( m_socket_uptr->is_open() )
                {
                    m_socket_uptr->shutdown( Socket::shutdown_receive );
                }
            }
        } );
}

}

#endif /* _UNIX_SOCKET_H_ */