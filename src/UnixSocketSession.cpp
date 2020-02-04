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

    /* Usage of static buffer is abandoned in favour of multithread implementation */
    // m_read_buf.clear();
    // m_read_buf.reserve( READ_BUF_SIZE );

    BufferShPtr read_buf_shptr = ::std::make_shared< Buffer >();
    read_buf_shptr->reserve( READ_BUF_SIZE );

    ::boost::asio::async_read_until( m_socket,
    ::boost::asio::dynamic_buffer( * read_buf_shptr ),
    delimiter,
    [ &, read_buf_shptr ] ( const ErrCode& error, 
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
            identification( * read_buf_shptr );
        } else { /* Give access to data after identification. */
            m_parent_ptr->getConfig().m_recv_cb( * read_buf_shptr );
        } //end if
        this->recv();
    } ); //end async_read_until
}

Result Server::Session::identification( const ::std::string& in_data )
{
    /* Actual parsing here */
    Tree xml_tree;
    // ::std::istringstream xml_stream( in_data ); //needless copy
    /* Avoids copy : */
    boost::iostreams::stream< \
        boost::iostreams::array_source > \
            xml_stream( in_data.c_str(), in_data.size() );
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