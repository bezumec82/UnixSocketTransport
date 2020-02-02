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

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "Tools.h"

namespace UnixSocket
{
    namespace PropTree = ::boost::property_tree;

    class Server;
    class Client;

    using IoService     = ::boost::asio::io_service;
    using ErrCode       = ::boost::system::error_code;

    using EndPoint      = ::boost::asio::local::stream_protocol::endpoint;
    using EndPointUptr  = ::std::unique_ptr< EndPoint >;
    using Acceptor      = ::boost::asio::local::stream_protocol::acceptor;
    using AcceptorUptr  = ::std::unique_ptr< Acceptor >;

    using Tree = ::boost::property_tree::ptree;

    using Socket        = ::boost::asio::local::stream_protocol::socket;
    using SocketUptr    = ::std::unique_ptr< Socket >;
    using RecvCallBack  = ::std::function< void( ::std::string ) >;
    using SendCallBack  = ::std::function< void( ::std::size_t ) >;
}

namespace UnixSocket
{

    enum class Result //: int8_t
    {
        ID_FAILURE = -3,
        NO_SUCH_ADDRESS = -2,
        CFG_ERROR = -1,
        ALL_GOOD = 0,
        SEND_SUCCESS = 1,
        ID_SUCCESS = 2,
    }; //end class Result

    class Server /* Default constructable */
    {
        class Session;
        using SessionShptr = ::std::shared_ptr< Session >;
        using Sessions = ::std::list< Session >;
        using SessionHandle = Sessions::iterator;
        using ConstSessionHandle = const SessionHandle;
        using IdentifiedSessions = ::std::unordered_map< ::std::string, ConstSessionHandle >;

    public :
        struct Config
        {
            RecvCallBack    m_recv_cb;
            SendCallBack    m_send_cb;
            /*!
             * TODO : Error callback - return errors to the user.
             */
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
        public : /*--- Methods ---*/
            Session( IoService& io_service, Server * parent )
                : m_socket( io_service ),
                m_parent_ptr( parent )
            { }

            Socket& getSocket( void )
            {
                return m_socket;
            }
            void recv( void );
            Result identification( const ::std::string& );
            template< typename Data >
            void send( Data&& );
            void saveHandle( SessionHandle self )
            {
                m_self = self;
            }
            ~Session();
        private : /*--- Variables ---*/
            Socket m_socket;
            Server * m_parent_ptr;
            ::std::string m_read_buf;
            SessionHandle m_self;
                // save iterator to yourself
                // used in identification process
            /*--- Flags ---*/
            ::std::atomic< bool > m_is_identified{ false };;
        }; /* end class Session */

    public : /*--- Methods ---*/
        /* Getters and Setters */
        Result setConfig ( Config && );
        Config& getConfig( void )
        {
            return m_config;
        }
        Sessions& getSessions( void )
        {
            return m_sessions;
        }
        IdentifiedSessions& getIdentifiedSessions( void )
        {
            return m_id_sessions_map;
        }
        Result start( void );
        template< typename Data >
        Result send( const ::std::string& , Data&& );
        ~Server();
    private :
        void accept( void );

    private : /*--- Variables ---*/
        Config m_config;
        AcceptorUptr m_acceptor_uptr;
        Sessions m_sessions; /* Server should know about all opened sessions */
        IdentifiedSessions m_id_sessions_map;
        IoService m_io_service; /* Initialized with class creation */
        ::std::thread m_worker;
        /*--- Flags ---*/
        ::std::atomic< bool > m_is_configured{ false };
    }; //end class Server



    class Client /* Default constructable */
    {
    public :
        enum class ConnectType //: uint8_t
        {
            ASYNC_CONNECT = 0,
            SYNC_CONNECT = 1
        };

        struct Config
        {
            RecvCallBack    m_recv_cb;
            SendCallBack    m_send_cb;
            ::std::string   m_address; //file to connect to
            ::std::string   m_delimiter; //look 'Server::Config'

            /* For identification at server side */
            ::std::string   m_id_key; //look 'Server::Config'
            ::std::string   m_client_id;
            ConnectType     m_con_type;
        };
    public : /*--- Methods ---*/

        Result setConfig( Config&& );
        Result start( void );

        template< typename Data >
        void send( Data&& );
        ~Client();
    private :
        void connect( ConnectType );
        void recv( void );
        void identify( void );

    private : /*--- Variables ---*/
        Config m_config;
        
        SocketUptr m_socket_uptr;
        EndPointUptr m_endpoint_uptr;
        
        IoService m_io_service;
        ::std::thread m_worker;
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