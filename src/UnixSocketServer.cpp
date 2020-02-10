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
        PRINT_ERR( "No RECEIVE callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_send_cb )
    {
        PRINT_ERR( "No SEND callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_error_cb )
    {
        PRINT_ERR( "No ERROR callback provided.\n" );
        return Result::CFG_ERROR;
    }

    unlink( m_config.m_address.c_str() ); //prepare address upfront
    m_is_configured.store( true );
    PRINTF( GRN, "Configuration is accepted.\n" );
    return Result::ALL_GOOD;
}

Result Server::start()
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
    auto work = [&](){
        m_io_service.run();
    }; //end []

#ifdef THREAD_IMPLEMENTATION
    m_worker = ::std::move( ::std::thread( work ) );
#else
    m_future = ::std::async( work );
#endif
    return Result::ALL_GOOD;
}

void Server::accept ()
{
    SessionHandle session_handle = m_sessions.emplace( m_sessions.end(), m_io_service, this  );
    m_acceptor_uptr->async_accept( session_handle->m_socket,
        [&, session_handle ] ( const ErrCode& error ) //mutable
        {
            if ( !error )
            {
                PRINTF( GRN, "Client accepted.\n" );
                session_handle->saveHandle( session_handle );
                session_handle->m_is_accepted.store( true );
                session_handle->recv();
                this->accept();
            }
            else
            {
                PRINT_ERR( "Error when accepting : %s\n", error.message().c_str() );
            }
        } );
}

void Server::removeSession( SessionHandle& session )
{
    PRINTF( RED, "Removing session with client '%s'\n" \
        session->m_client_id.c_str() );
    ::std::unique_lock< ::std::mutex >( m_sessions_mtx );
    if( session->m_is_identified.load() )
    {
        auto iter = m_id_sessions_map.find( session->m_client_id );
        if( iter == m_id_sessions_map.end() )
        {
            PRINT_ERR( "Can't find session with client : %s\n", \
                session->m_client_id.c_str() );
            return;
        }
        m_id_sessions_map.erase(iter);
    }
    m_sessions.erase( session );
}

Server::~Server()
{
    /* Stop accepting */
    m_acceptor_uptr->cancel();
    m_acceptor_uptr->close();
    
    {/* Destroy all sessions */
        ::std::unique_lock< ::std::mutex >( m_sessions_mtx );
        m_sessions.clear();
    }
    /* Stop handling events */
    m_io_service.stop();
#ifdef THREAD_IMPLEMENTATION
    m_worker.join();
#else
    m_future.get();
#endif
    PRINTF( YEL, "Server destroyed.\n" );
}

/* EOF */