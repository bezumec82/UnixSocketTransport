#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <atomic>
#include <sstream>
#include <unordered_map>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <Tools.h>

namespace UnixSocket
{
    namespace PropTree = ::boost::property_tree;

    class Server;
    class Client;
    enum class Result;


    using IoService     = ::boost::asio::io_service;
    using ErrCode       = ::boost::system::error_code;

    using EndPoint      = ::boost::asio::local::stream_protocol::endpoint;
    using EndPointUptr  = ::std::unique_ptr< EndPoint >;
    using Acceptor      = ::boost::asio::local::stream_protocol::acceptor;
    using AcceptorUptr  = ::std::unique_ptr< Acceptor >;

    using Socket        = ::boost::asio::local::stream_protocol::socket;
    using SocketUptr    = ::std::unique_ptr< Socket >;

    using ClientId      = ::std::string;
    using Tree          = ::boost::property_tree::ptree;

    using RecvCallBack  = void( const ClientId&, ::std::string& );
    using SendCallBack  = void( const ClientId&, ::std::size_t );
    using ErrorDescription = ::std::string;
    using ErrorCallBack = void( const ClientId&, const ErrorDescription& );

    using Buffer        = ::std::string;
    using BufferShPtr   = ::std::shared_ptr< Buffer >;
}

namespace UnixSocket
{

    enum class Result //: int8_t
    {
        SEND_ERROR      = -5,
        RECV_ERROR      = -4,
        ID_FAILURE      = -3,
        NO_SUCH_ADDRESS = -2,
        CFG_ERROR       = -1,
        ALL_GOOD        = 0,
        SEND_SUCCESS    = 1,
        ID_SUCCESS      = 2,
    }; //end class Result

    class Server /* Default constructable */
    {
        class Session;

    public :
        using Sessions = ::std::list< Session >;
        using SessionHandle = Sessions::iterator;
        using ConstSessionHandle = const SessionHandle;
        using IdentifiedSessions = ::std::unordered_map< ClientId, ConstSessionHandle >;

    public : /*--- Classes/structures/enumerators ---*/
        struct Config
        {
            ::std::function< RecvCallBack >  m_recv_cb;
            ::std::function< SendCallBack >  m_send_cb;

            /* In this callback user should handle sessions enumeration.
             * To handle it at the 'Server::session' side, mutex is necessary. */
            ::std::function< ErrorCallBack > m_error_cb;

            ::std::string   m_address; //address of server in the file system
            ::std::string   m_delimiter;
                /* Each message should have some kind of start-end sequence :
                 *  <body>
                 *      ...
                 *  </body>
                 */
            ::std::string   m_id_key;
                /* for example XML is used for communication.
                 * Server will wait for key of type <'m_id_key'>ClientName</'m_id_key'>.
                 * For example :
                 * <id>ClientName</id>, or :
                 * <name>ClientName</name>,
                 * until that no transactions will pass through Session class.
                 */

        }; //end struct Config

    private : /* No access to the Sessions from outside */
        class Session
        {
            friend class Server;
        public : /*--- Methods ---*/
            Session( IoService& io_service, Server * parent )
                : m_io_service_ref( io_service ),
                m_socket( io_service ),
                m_parent_ptr( parent )
            { }
            void recv();
            Result identification( const ::std::string& );
            template< typename Data >
            void send( Data&& );
            void saveHandle( SessionHandle self )
            {
                m_self = self;
            }
            ~Session();
        private : /*--- Variables ---*/
            IoService& m_io_service_ref;
            Socket m_socket;
            Server * m_parent_ptr;
            const int READ_BUF_SIZE = 1024;
            ::std::string m_read_buf;
            SessionHandle m_self;
                // save iterator to yourself
                // used in identification process

            ::std::string m_client_id; //Identification of remote client for this session
            /* Sessions are stored at server side by principle : 'm_client_id' -> 'm_self' */
        private : /*--- Flags ---*/
            ::std::atomic< bool > m_is_identified{ false };

            /* This flag is signalling that session is accepted, but client isn't identified yet. */
            ::std::atomic< bool > m_is_accepted{ false };

            /* This flag is signalling that session should be destroyed by 'Server' */
            ::std::atomic< bool > m_is_valid{ true };
        }; /* end class Session */

    private : /*--- Getters and Setters ---*/
        Config& getConfig()
        {
            return m_config;
        }
        Sessions& getSessions()
        {
            return m_sessions;
        }
        IdentifiedSessions& getIdentifiedSessions()
        {
            return m_id_sessions_map;
        }

    public : /*--- Methods ---*/
        Result setConfig ( Config && );
        Result start();
        template< typename Data >
        Result send( const ::std::string& , Data&& );
        template< typename Data >
        Result broadCast( Data&& ); /* Send to all clients */
        template< typename Data >
        Result multiCast( Data&& ); /* Send to all identified clients */

        ~Server();

    private :
        void removeSession( SessionHandle& );
        void accept();

    private : /*--- Variables ---*/
        
        Config m_config;
        AcceptorUptr m_acceptor_uptr;

         /* Server should know about all opened sessions */
        Sessions m_sessions;
        IdentifiedSessions m_id_sessions_map;
        ::std::mutex m_sessions_mtx; //protect access to the sessions data

        IoService m_io_service; /* Initialized with class creation */
        ::std::thread m_worker;
        ::std::future<void> m_future;

        /*--- Flags ---*/
        ::std::atomic< bool > m_is_configured{ false };
    }; //end class Server

    class Client /* Default constructable */
    {
    public :
        enum class ConnectType //: uint8_t
        {
            ASYNC_CONNECT   = 0,
            SYNC_CONNECT    = 1
        };

        struct Config
        {
            ::std::function< RecvCallBack >   m_recv_cb;
            ::std::function< SendCallBack >   m_send_cb;
            ::std::function< ErrorCallBack >  m_error_cb;

            ::std::string   m_address; //file to connect to
            ::std::string   m_delimiter; //look 'Server::Config'

            /* For identification at server side */
            ::std::string   m_id_key; //look 'Server::Config'
            ::std::string   m_client_id;
            ConnectType     m_con_type;
        };
    public : /*--- Methods ---*/

        Result setConfig( Config&& );
        Result start();

        template< typename Data >
        void send( Data&& );
        ~Client();
    private :
        void connect( ConnectType );
        void recv();
        void identify();

    private : /*--- Variables ---*/
        Config m_config;
        
        SocketUptr m_socket_uptr;
        EndPointUptr m_endpoint_uptr;
        
        IoService m_io_service;
        ::std::thread m_worker;
        ::std::future<void> m_future;

        const int READ_BUF_SIZE = 1024;
        ::std::string m_read_buf;

        /*--- Flags ---*/
        ::std::atomic< bool > m_is_configured{ false };
        ::std::atomic<bool> m_is_connected{ false };
    }; //end class client
}

/* c-style helpers */
#define ERR_CHECK(val, msg) \
    if( val == "" ) \
    { \
        PRINT_ERR( "No %s provided.\n", msg ); \
        return Result::CFG_ERROR; \
    }

#include "UnixSocketClient.hpp"
#include "UnixSocketServer.hpp"
#include "UnixSocketSession.hpp"


#endif /* UNIX_SOCKET_H*/