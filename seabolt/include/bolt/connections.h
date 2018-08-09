/*
 * Copyright (c) 2002-2018 "Neo Technology,"
 * Network Engine for Objects in Lund AB [http://neotechnology.com]
 *
 * This file is part of Neo4j.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 */

#ifndef SEABOLT_CONNECT
#define SEABOLT_CONNECT

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "addressing.h"
#include "config.h"
#include "values.h"

typedef uint64_t bolt_request_t;

/**
 *
 */
enum BoltTransport {
    BOLT_SOCKET = 0,
    BOLT_SECURE_SOCKET = 1,
};

/**
 *
 */
enum BoltConnectionStatus {
    BOLT_DISCONNECTED = 0,          // not connected
    BOLT_CONNECTED = 1,             // connected but not authenticated
    BOLT_READY = 2,                 // connected and authenticated
    BOLT_FAILED = 3,                // recoverable failure
    BOLT_DEFUNCT = 4,               // unrecoverable failure
};

/**
 *
 */
enum BoltConnectionError {
    BOLT_SUCCESS = 0,
    BOLT_UNKNOWN_ERROR = 1,
    BOLT_UNSUPPORTED = 2,
    BOLT_INTERRUPTED = 3,
    BOLT_CONNECTION_RESET = 4,
    BOLT_NO_VALID_ADDRESS = 5,
    BOLT_TIMED_OUT = 6,
    BOLT_PERMISSION_DENIED = 7,
    BOLT_OUT_OF_FILES = 8,
    BOLT_OUT_OF_MEMORY = 9,
    BOLT_OUT_OF_PORTS = 10,
    BOLT_CONNECTION_REFUSED = 11,
    BOLT_NETWORK_UNREACHABLE = 12,
    BOLT_TLS_ERROR = 13,             // general catch-all for OpenSSL errors :/
    BOLT_END_OF_TRANSMISSION = 15,
    BOLT_SERVER_FAILURE = 16,
    BOLT_TRANSPORT_UNSUPPORTED = 0x400,
    BOLT_PROTOCOL_VIOLATION = 0x500,
    BOLT_PROTOCOL_UNSUPPORTED_TYPE = 0x501,
    BOLT_PROTOCOL_NOT_IMPLEMENTED_TYPE = 0x502,
    BOLT_PROTOCOL_UNEXPECTED_MARKER = 0x503,
    BOLT_PROTOCOL_UNSUPPORTED = 0x504,
    BOLT_STATUS_SET = 0x900,
};

/**
 * Record of connection usage statistics.
 */
struct BoltConnectionMetrics {
    struct timespec time_opened;
    struct timespec time_closed;
    unsigned long long bytes_sent;
    unsigned long long bytes_received;
};

/**
 * A Bolt client-server connection instance.
 *
 */
struct BoltConnection {
    /// The agent currently responsible for using this connection
    const void* agent;

    /// Transport type for this connection
    enum BoltTransport transport;

    char* host;
    char* port;
    char* resolvedHost;
    in_port_t resolvedPort;

    /// The security context (secure connections only)
    struct ssl_ctx_st* ssl_context;
    /// A secure socket wrapper (secure connections only)
    struct ssl_st* ssl;
    /// The raw socket that backs this connection
    int socket;

    /// The protocol version used for this connection
    int32_t protocol_version;
    /// State required by the protocol
    void* protocol_state;

    // These buffers contain data exactly as it is transmitted or
    // received. Therefore for Bolt v1, chunk headers are included
    // in these buffers

    /// Transmit buffer
    struct BoltBuffer* tx_buffer;
    /// Receive buffer
    struct BoltBuffer* rx_buffer;

    /// Connection metrics
    struct BoltConnectionMetrics metrics;
    /// Current status of the connection
    enum BoltConnectionStatus status;
    /// Current connection error code
    enum BoltConnectionError error;
};

/**
 * Create a new connection.
 *
 * @return
 */
PUBLIC struct BoltConnection* BoltConnection_create();

/**
 * Destroy a connection.
 */
PUBLIC void BoltConnection_destroy(struct BoltConnection* connection);

/**
 * Open a connection to a Bolt server.
 *
 * This function attempts to connect a BoltConnection to _address_ over
 * _transport_. The `address` should be a pointer to a `BoltAddress` struct
 * that has been successfully resolved.
 *
 * This function blocks until the connection attempt succeeds or fails.
 * On returning, the connection status will be set to either `BOLT_CONNECTED`
 * (if successful) or `BOLT_DEFUNCT` (if not). If defunct, the error code for
 * the connection will be set to one of the following:
 *
 * @verbatim embed:rst:leading-asterisk
 * ========================  ====================================================================
 * Error code                Description
 * ========================  ====================================================================
 * BOLT_UNRESOLVED_ADDRESS   The supplied address has not been resolved.
 * BOLT_CONNECTION_REFUSED   The remote server refused to accept the connection.
 * BOLT_INTERRUPTED          The connection attempt was interrupted.
 * BOLT_NETWORK_UNREACHABLE  The server address is on an unreachable network.
 * BOLT_OUT_OF_FILES         The system limit on the total number of open files has been reached.
 * BOLT_OUT_OF_MEMORY        Insufficient memory is available.
 * BOLT_OUT_OF_PORTS         No more local ports are available.
 * BOLT_PERMISSION_DENIED    The current process does not have permission to create a connection.
 * BOLT_TIMED_OUT            The connection attempt timed out.
 * BOLT_TLS_ERROR            An error occurred while attempting to secure the connection.
 * BOLT_UNKNOWN_ERROR        An error occurred for which no further detail can be determined.
 * BOLT_UNSUPPORTED          One or more connection parameters are unsupported.
 * ========================  ====================================================================
 * @endverbatim
 *
 * @param connection the connection to open
 * @param transport the type of transport over which to connect
 * @param address descriptor of the remote Bolt server address
 * @return 0 if the connection was opened successfully, -1 otherwise
 */
PUBLIC int BoltConnection_open(struct BoltConnection* connection, enum BoltTransport transport,
        struct BoltAddress* address);

/**
 * Close a connection.
 *
 * @param connection
 */
PUBLIC void BoltConnection_close(struct BoltConnection* connection);

/**
 * Initialise the connection and authenticate using the basic
 * authentication scheme.
 *
 * @param connection the connection to initialise
 * @param user_agent the user-agent string
 * @param auth_token dictionary that contains user credentials
 * @return
 */
PUBLIC int
BoltConnection_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token);

/**
 * Send all queued requests.
 *
 * @param connection
 * @return the latest request ID
 */
PUBLIC int BoltConnection_send(struct BoltConnection* connection);

/**
 * Take an exact amount of data from the receive buffer, deferring to
 * the socket if not enough data is available.
 *
 * @param connection
 * @param buffer
 * @param size
 * @return
 */
int BoltConnection_receive(struct BoltConnection* connection, char* buffer, int size);

/**
 * Fetch the next value from the result stream for a given request.
 * This will discard the responses of earlier requests that have not
 * already been fully consumed. This function will always consume at
 * least one record from the result stream and is not able to check
 * whether the given request has already been fully consumed; doing
 * so is the responsibility of the calling application.
 *
 * After calling this function, the value returned by
 * `BoltConnection_data` should contain either record data
 * (stored in a `BOLT_LIST`) or summary metadata (in a `BOLT_SUMMARY`).
 *
 * This function will block until an appropriate value has been fetched.
 *
 * @param connection the connection to fetch from
 * @param request the request for which to fetch a response
 * @return 1 if record data is received,
 *         0 if summary metadata is received,
 *         -1 if an error occurs
 *
 */
PUBLIC int BoltConnection_fetch(struct BoltConnection* connection, bolt_request_t request);

/**
 * Fetch values from the result stream for a given request, up to and
 * including the next summary. This will discard any unconsumed result
 * data for this request as well as the responses of earlier requests
 * that have not already been fully consumed. This function will always
 * consume at least one record from the result stream and is not able
 * to check whether the given request has already been fully consumed;
 * doing so is the responsibility of the calling application.
 *
 * After calling this function, the value returned by
 * `BoltConnection_data` should contain the summary metadata of the
 * received result (in a `BOLT_SUMMARY`).
 *
 * @param connection the connection to fetch from
 * @param request the request for which to fetch a response
 * @return >=0 the number of records discarded from this result
 *         -1 if an error occurs
 */
PUBLIC int BoltConnection_fetch_summary(struct BoltConnection* connection, bolt_request_t request);

/**
 * Set the next Cypher statement template to be run on this connection.
 *
 * @param connection
 * @param cypher
 * @param cypher_size
 * @param n_parameters
 * @return
 */
PUBLIC int BoltConnection_cypher(struct BoltConnection* connection, const char* cypher, size_t cypher_size,
        int32_t n_parameters);

/**
 * Return a pointer to a Cypher parameter.
 *
 * @param connection
 * @param index
 * @param key
 * @param key_size
 * @return
 */
PUBLIC struct BoltValue* BoltConnection_cypher_parameter(struct BoltConnection* connection, int32_t index,
        const char* key, size_t key_size);

/**
 * Load a bookmark to be used when beginning the next transaction.
 *
 * @param connection
 * @param bookmark
 * @return
 */
PUBLIC int BoltConnection_load_bookmark(struct BoltConnection* connection, const char* bookmark);

/**
 * Load a transaction BEGIN request into the request queue.
 *
 * @param connection
 * @return
 */
PUBLIC int BoltConnection_load_begin_request(struct BoltConnection* connection);

/**
 * Load a transaction COMMIT request into the request queue.
 *
 * @param connection
 * @return
 */
PUBLIC int BoltConnection_load_commit_request(struct BoltConnection* connection);

/**
 * Load a transaction ROLLBACK request into the request queue.
 *
 * @param connection
 * @return
 */
PUBLIC int BoltConnection_load_rollback_request(struct BoltConnection* connection);

/**
 * Load a RUN request into the request queue.
 *
 * @param connection
 * @return
 */
PUBLIC int BoltConnection_load_run_request(struct BoltConnection* connection);

/**
 * Load a DISCARD_ALL request into the request queue.
 *
 * @param connection
 * @param n should always be -1
 * @return
 */
PUBLIC int BoltConnection_load_discard_request(struct BoltConnection* connection, int32_t n);

/**
 * Load a PULL_ALL request into the request queue.
 *
 * @param connection
 * @param n should always be -1
 * @return
 */
PUBLIC int BoltConnection_load_pull_request(struct BoltConnection* connection, int32_t n);

/**
 * Load a RESET request into the request queue.
 *
 * RESET message resets the connection to discard any outstanding results,
 * rollback the current transaction and clear any unacknowledged
 * failures.
 *
 * @param connection
 * @return
 */
PUBLIC int BoltConnection_load_reset_request(struct BoltConnection* connection);

/**
 * Obtain a handle to the last request sent to the server. This handle
 * can be used to fetch response data for a particular request.
 *
 * @param connection
 * @return
 */
PUBLIC bolt_request_t BoltConnection_last_request(struct BoltConnection* connection);

/**
 * Obtain the latest bookmark sent by the server. This may return null if
 * server did not return any bookmark data for this connection. This pointer is
 * alive, which means the underlying bookmark data may be changed over time with
 * updated data on this same connection. Do not change underlying data and clone
 * it if you want to have a fixed bookmark in-hand.
 *
 * @param connection
 * @return
 */
PUBLIC char* BoltConnection_last_bookmark(struct BoltConnection* connection);


/**
* Obtain a value from the current record.
*
* @param connection
* @param field
* @return pointer to a `BoltValue` data structure formatted as a BOLT_LIST
*/
PUBLIC struct BoltValue* BoltConnection_record_fields(struct BoltConnection* connection);

/**
*
* @param connection
* @return
*/
PUBLIC int BoltConnection_summary_success(struct BoltConnection* connection);

/**
 * Obtain the details of the latest server generated FAILURE message
 *
 * @param connection
 * @return
 */
PUBLIC struct BoltValue* BoltConnection_failure(struct BoltConnection* connection);

/**
 * Return the fields available in the current result.
 *
 * @param connection
 * @return
 */
PUBLIC struct BoltValue* BoltConnection_fields(struct BoltConnection* connection);

/**
 * Returns the metadata sent by the server.
 *
 * @param connection
 * @return
 */
PUBLIC struct BoltValue* BoltConnection_metadata(struct BoltConnection* connection);

PUBLIC char* BoltConnection_server(struct BoltConnection* connection);

#endif // SEABOLT_CONNECT
