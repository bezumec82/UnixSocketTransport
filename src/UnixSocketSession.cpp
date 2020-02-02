#include "UnixSocket.h"

using namespace UnixSocket;

void Server::Session::recv()
{
    ::std::string delimiter = "";
    if( ! m_is_identified.load() )
    {
        delimiter = ::std::string{ "</" + m_parent_ptr->getConfig().m_id_key + ">" };
    }
    else
    {
        delimiter = ::std::string{ "</" + m_parent_ptr->getConfig().m_delimiter + ">" };
    }
    m_read_buf.reserve( 1024 );
    ::boost::asio::async_read_until( m_socket,
    ::boost::asio::dynamic_buffer( m_read_buf ),
    delimiter,
    [&] ( const ErrCode& error, 
        ::std::size_t bytes_transferred ) //mutable
    {
        if( error )
        {
            PRINT_ERR( "Error when reading : %s\n", error.message().c_str());
            if( m_socket.is_open() )
            {
               m_socket.shutdown( Socket::shutdown_receive );
            }
            /* TODO : Call for user's error handler. */
            return;
        } //end if( error )

        if( ! m_is_identified.load() )
        {
            identification(m_read_buf );
        } else { /* Give access to data after identification. */
            m_parent_ptr->getConfig().m_recv_cb( m_read_buf );
        } //end if
        m_read_buf.clear();
        this->recv();
    } ); //end async_read_until
}

Result Server::Session::identification( const ::std::string& in_data )
{
    /* Actual parsing here */
    Tree xml_tree;
    ::std::istringstream xml_stream( in_data );
    PropTree::read_xml( xml_stream, xml_tree );
    try {
        ::std::string key = xml_tree.get<std::string>( m_parent_ptr->getConfig().m_id_key );
        m_parent_ptr->getIdentifiedSessions().emplace(
            ::std::make_pair(
                key, 
                m_self
        ) );
        m_is_identified.store(true);
        PRINTF( GRN, "Client '%s' successfully identified.\n", key.c_str() );
        return Result::ID_SUCCESS;
    } catch( const ::std::exception& e )
    {
        PRINT_ERR( "%s\n", e.what() );
        return Result::ID_FAILURE;
    }
}

Server::Session::~Session()
{
    if( m_socket.is_open() )
    {
        m_socket.shutdown( Socket::shutdown_both );
        m_socket.close();
    }
    PRINTF( YEL, "Session destroyed.\n");
}

/* EOF */