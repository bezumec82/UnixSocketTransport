#ifndef UNIX_SOCKET_SERVER_HPP
#define UNIX_SOCKET_SERVER_HPP

#include "UnixSocket.h"

namespace UnixSocket
{

/* Wrapper around 'Session.send' */
template< typename Data >
Result Server::send( const ::std::string& client_name, Data&& data )
{
    ::std::unique_lock< ::std::mutex >( m_sessions_mtx );
    /* Client should provide some kind recognition. */
    auto found = m_id_sessions_map.find( client_name );
    if( found != m_id_sessions_map.end() )
    {
        /* Underlaying class is Session */
        found->second->send(::std::forward<Data>(data) );
        return Result::SEND_SUCCESS;
    }
    else
    {
        PRINT_ERR( "No such client : %s.\n", client_name.c_str() );
        return Result::NO_SUCH_ADDRESS;
    }
}

template< typename Data >
Result Server::multiCast( Data&& data )
{
    ::std::unique_lock< ::std::mutex >( m_sessions_mtx );
    for( auto& it : m_id_sessions_map )
    {
        it.second->send( ::std::forward<Data>(data) );
    }
    return Result::SEND_SUCCESS;
}

template< typename Data >
Result Server::broadCast( Data&& data )
{
    ::std::unique_lock< ::std::mutex >( m_sessions_mtx );
    for( auto& it : m_sessions )
    {
        if( it.m_is_accepted.load() )
            it.send( ::std::forward<Data>(data) );
    }
    return Result::SEND_SUCCESS;
}


} //end namespace UnixSocket

#endif /* UNIX_SOCKET_SERVER_HPP */