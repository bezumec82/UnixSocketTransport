#include "UnixSocket.h"

using namespace UnixSocket;

Result Server::setConfig( Config&& cfg )
{
    m_config = cfg;
    ERR_CHECK( m_config.m_address,      "file name" );
    ERR_CHECK( m_config.m_id_key,       "identification");
    ERR_CHECK( m_config.m_delimiter,    "delimiter");

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
    unlink( m_config.m_address.c_str() ); //prepare address upfront
    m_is_configured.store( true );
    PRINTF( GRN, "Configuration is accepted.\n" );
    return Result::ALL_GOOD;
}

Result Server::start( void )
{
    if( ! m_is_configured.load() )
    {
        PRINT_ERR( "Server has no configuration.\n" );
        return Result::CFG_ERROR;
    }
    ::std::cout << "Starting Unix server : " << m_config.m_address <<::std::endl;
    // PRINTF( RED, "Starting Unix server '%s'", m_config.m_address.c_str() );
    m_acceptor_uptr = ::std::make_unique< Acceptor >( m_io_service , EndPoint{ m_config.m_address } );
    accept(); /* Recursive async call inside */
    m_worker = ::std::move( 
        ::std::thread( [&](){ m_io_service.run(); } )
    );
    return Result::ALL_GOOD;
}

void Server::accept ( void )
{
    SessionHandle session_handle = m_sessions.emplace( m_sessions.end(), m_io_service, this  );
    m_acceptor_uptr->async_accept( session_handle->getSocket(),
        [&, session_handle ] ( const ErrCode& error ) //mutable
        {
            if ( !error )
            {
                PRINTF( GRN, "Client accepted.\n" );
                session_handle->saveHandle( session_handle );
                session_handle->recv();
                this->accept();
            }
            else
            {
                PRINT_ERR( "Error when accepting : %s\n", error.message().c_str() );
            }
        } );
}

Server::~Server()
{
    /* Stop accepting */
    m_acceptor_uptr->cancel();
    m_acceptor_uptr->close();
    /* Destroy all sessions */
    m_sessions.clear();
    /* Stop handling events */
    m_io_service.stop();
    m_worker.join();
    PRINTF( YEL, "Server destroyed.\n" );
}

/* EOF */