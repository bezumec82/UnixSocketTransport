#include "UnixSocket.h"

using namespace UnixSocket;

/*--------------*/
/*--- Client ---*/
/*--------------*/
Result Client::setConfig( Config&& cfg )
{
    m_config = cfg;
    ERR_CHECK( m_config.m_address,       "file name" );
    ERR_CHECK( m_config.m_id_key,      "identification" );
    ERR_CHECK( m_config.m_delimiter,     "delimiter" );
    ERR_CHECK( m_config.m_client_id,   "client name" );
    if( ! m_config.m_recv_cb )
    {
        PRINT_ERR( "No read callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_send_cb )
    {
        PRINT_ERR( "No send callback provided.\n" );
        return Result::CFG_ERROR;
    }
    m_is_configured.store( true );    
    PRINTF( GRN, "Configuration is accepted.\n" );
    return Result::ALL_GOOD;
}

Result Client::start( void )
{
    if( ! m_is_configured.load() )
    {
        PRINT_ERR( "Server has no configuration.\n" );
        return Result::CFG_ERROR;
    }
    ::std::cout << "Starting Unix client. Server address : " 
                << m_config.m_address <<::std::endl;
    
    m_socket_uptr = ::std::make_unique< Socket >(m_io_service);
    m_endpoint_uptr = ::std::make_unique< EndPoint >( m_config.m_address );
    connect(m_config.m_con_type);
    m_worker = ::std::move( 
        ::std::thread( [&](){ m_io_service.run(); } )
    );
    return Result::ALL_GOOD;
}

#define CON_RETRY_DELAY ::std::chrono::milliseconds( 500 )
void Client::connect( ConnectType conType )
{
    switch( conType )
    {
        case ConnectType::ASYNC_CONNECT :
        {
            m_socket_uptr->async_connect( * m_endpoint_uptr, 
            [&] ( const ::boost::system::error_code& error )
            {
                if( !error )
                {
                    PRINTF( GRN, "Successfully connected to the server '%s'.\n", 
                            m_endpoint_uptr->path().c_str() );
                    identify();
                    m_is_connected.store( true ); //let send after identification
                    this->recv();
                } else {
                    PRINT_ERR( "Can't connect to the server '%s'.\n",
                        m_endpoint_uptr->path().c_str() );
                }
            } );
            break;
        } //end ASYNC_CONNECT
        case ConnectType::SYNC_CONNECT : //Usually before connection client is useless
        {
            try {
                m_socket_uptr->connect( * m_endpoint_uptr );
                PRINTF( GRN, "Successfully connected to the server '%s'.\n", 
                        m_endpoint_uptr->path().c_str() );
                identify();
                m_is_connected.store( true );
                this->recv();
            } catch( const ::std::exception& e ) {
                PRINT_ERR( "%s.\n", e.what() );
            }
            if( ! m_is_connected.load() ) //try again
            {
                /* Delay */
                ::std::this_thread::sleep_for( CON_RETRY_DELAY );
                this->connect( ConnectType::SYNC_CONNECT );
            }
            break;
        } //end SYNC_CONNECT
        default :
        {
            throw std::runtime_error( "Undefined connection type.\n" );
        }
    } //end switch
}

void Client::identify( void )
{
    Tree xml_tree;
    xml_tree.put( m_config.m_id_key, m_config.m_client_id );
    ::std::ostringstream xml_stream;
    PropTree::write_xml( xml_stream, xml_tree );
    PRINTF( YEL, "Sending identification : %s.\n", xml_stream.str().c_str() );
    send( xml_stream.str() );

}

void Client::recv( void )
{
    m_read_buf.reserve( 1024 );
    ::boost::asio::async_read_until( * m_socket_uptr,
    ::boost::asio::dynamic_buffer( m_read_buf ),
    ::std::string{ "</" + m_config.m_delimiter + ">" },
    [&] ( const ErrCode& error, 
    ::std::size_t bytes_transferred ) //mutable
    {
        if( error )
        {
            PRINT_ERR( "Error when reading : %s\n", error.message().c_str() );
            if( m_socket_uptr->is_open() )
            {
                m_socket_uptr->shutdown( Socket::shutdown_receive );
            }
            return;
        }
        m_config.m_recv_cb( m_read_buf );
        m_read_buf.clear();
        this->recv();
    } ); //end async_read_until
}

Client::~Client()
{
    m_io_service.stop();
    m_worker.join();
    if( m_socket_uptr->is_open() )
    {
        m_socket_uptr->shutdown( Socket::shutdown_both );
        m_socket_uptr->close();
    }
    PRINTF( YEL, "Client '%s' destroyed.\n", m_config.m_client_id.c_str() );
}

/* EOF */