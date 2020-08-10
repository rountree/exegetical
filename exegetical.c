/* exegetical.c
 *
 * Test with:  flux python -c 'import flux; h=flux.Flux(); print(h.rpc("exegetical.foo", { "test": 42 }).get())'
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <jansson.h>
#include <flux/core.h>

#define MY_MOD_NAME "exegetical"
static const int NO_FLAGS = 0;
//static const char const * DEFAULT_NAMESPACE = NULL;
const char default_service_name[] = MY_MOD_NAME;
static uint32_t rank, size;
#define HELLO flux_log(h, LOG_CRIT, "%s:%d rank %" PRIu32 " size %" PRIu32 ".\n", __FILE__, __LINE__, rank, size)

enum{
	UPSTREAM	=0,
	ME,
	DOWNSTREAM1,
	DOWNSTREAM2,
	NUM_MSGS
};
char * overlay[NUM_MSGS];


void timer_handler( flux_reactor_t *r, flux_watcher_t *w, int revents, void* arg ){

	static int initialized = 0; 
	flux_t *h = (flux_t*)arg;
	flux_get_rank(h, &rank);
	flux_get_size(h, &size);
/*
	static int count = 0;
	int rc;

	if( !initialized ){
		flux_log(h, LOG_CRIT, "%s:%d rank %" PRIu32 " size %" PRIu32 ".  Timer!\n", __FILE__, __LINE__, rank, size);
		initialized = 1;
	}
	count++;

        // Allocate the kvs transaction
        flux_kvs_txn_t *kvs_txn = flux_kvs_txn_create();
        assert( NULL != kvs_txn );

        // Create an entry from the kvs
        rc = flux_kvs_txn_put( kvs_txn, NO_FLAGS, "mykey", "myvalue" );
        assert( -1 != rc );

        // Commit the key+value.
        flux_future_t *kvs_future = flux_kvs_commit( h, DEFAULT_NAMESPACE, NO_FLAGS, kvs_txn );
        assert( NULL != kvs_future );

        // Wait for confirmation
        rc = flux_future_wait_for( kvs_future, 10.0 );
        assert( rc != -1 );

        // Destroy our future.
        flux_future_destroy( kvs_future );
*/

	// flux_log( (flux_t *)arg, LOG_CRIT, "%s:%d rank %" PRIu32 " size %" PRIu32 ".  Me=%s.  Timer!\n", __FILE__, __LINE__, rank, size, overlay[ME]);
	if( !initialized ){
		if(rank == 1){
			flux_future_t *future_msg_to_upstream = flux_rpc (
					h, 				// flux_t *h, 
					(const char*)overlay[UPSTREAM],	// const char *topic,
					"Well howdy!",			// const cahr *s,
					FLUX_NODEID_ANY,		// uint32_t nodeid, 
					FLUX_RPC_NORESPONSE);		// int flags);
			assert( future_msg_to_upstream );
			flux_future_destroy( future_msg_to_upstream );	// Not needed due to no response.
		}
		initialized = 1;
	}
}

static void overlay_cb (flux_t *h, flux_msg_handler_t *mh, const flux_msg_t *msg, void *arg){
	// Set up to catch messages addressed to overlay[ME].
	flux_log(h, LOG_CRIT, "%s:%d The message hander overlay_cb received a message addressed to %s\n", __FILE__, __LINE__, overlay[ME]);

}

static void foo_cb (flux_t *h, flux_msg_handler_t *mh, const flux_msg_t *msg, void *arg){

	const void *data;
	int size, rc;

	rc = flux_msg_get_payload (msg, &data, &size);
	if (-1 == rc){
		flux_log_error (h, "echo_cb: flux_msg_get_payload");
		rc = flux_respond_error (h, msg, errno, "flux_msg_get_payload failed");
		if( -1 == rc ){
			flux_log_error (h, "flux_respond_error");
		}
	}
	flux_log(h, LOG_CRIT, "%s:%d data=%s \n", __FILE__, __LINE__, (char*)data);

	//rc = flux_respond_raw (h, msg, data, size);	// original
	rc = flux_respond( h, msg, "{\"test_response\":24}" ); // this also works
	if (-1 == rc){
		flux_log_error (h, "echo_cb: flux_respond_raw");
		rc = flux_respond_error (h, msg, errno, "flux_respond_raw failed");
		if( -1  == rc ){
			flux_log_error (h, "flux_respond_error");
		}
	}
	return;
}


int mod_main (flux_t *h, int argc, char **argv)
{
	int rc;

	// Get rank via flux_get_rank() and print results to stderr.
	flux_get_rank(h, &rank);
	flux_get_size(h, &size);
	flux_log(h, LOG_CRIT, "%s:%d QQQ Hello from rank %" PRIu32 " of %" PRIu32 ".\n", __FILE__, __LINE__, rank, size);

	// Set up a message handler.
	struct flux_match match = FLUX_MATCH_REQUEST;
	match.topic_glob = "exegetical.foo";
	flux_msg_handler_t *mh = flux_msg_handler_create (h, match, foo_cb, NULL);
	assert( mh );
	flux_msg_handler_start(mh);

	// Set up the overlay network.
	// Upstream rank:  if( rank ){ upstream = rank/2 }
	// Downstream rank:  if( (rank*2) < size ){ downstream1 = rank*2 }; if( (rank*2)+1 < size ){ downstream2 = rank*2+1 }

	// UPSTREAM
	if( rank ){
		assert( -1 != asprintf( &overlay[UPSTREAM], "exegetical.%" PRIu32 "", rank/2 ));
	}else{
		overlay[UPSTREAM] = NULL;	// rank 0 doesn't have an upstream node.
	}

	// DOWNSTREAM 1 and 2
	if( rank ){
		if( rank*2 < size ){
			assert( -1 != asprintf( &overlay[DOWNSTREAM1], "exegetical.%" PRIu32 "", rank*2 ));
		}else{
			overlay[DOWNSTREAM1] = NULL;
		}
		if( rank*2+1 < size ){
			assert( -1 != asprintf( &overlay[DOWNSTREAM2], "exegetical.%" PRIu32 "", rank*2+1 ));
		}else{
			overlay[DOWNSTREAM2] = NULL;
		}
	}else{
		assert( -1 != asprintf( &overlay[DOWNSTREAM1], "exegetical.%" PRIu32 "", 1 ));
		overlay[DOWNSTREAM2] = NULL;
	}

	// ME
	assert( -1 != asprintf( &overlay[ME], "exegetical.%" PRIu32 "", rank ));

	// Set up the overlay_cb message handler.	
	struct flux_match overlay_msg = FLUX_MATCH_REQUEST;
	match.topic_glob = overlay[ME];
	flux_msg_handler_t *mh_overlay = flux_msg_handler_create (h, overlay_msg, overlay_cb, (void*)h);
	assert( mh_overlay );
	flux_msg_handler_start(mh_overlay);

	// Set up a timer.
	flux_watcher_t* timer_watch_p = flux_timer_watcher_create( flux_get_reactor(h), 1.0, 1.0, timer_handler, h);
	assert( timer_watch_p );
	flux_watcher_start( timer_watch_p );


	// Start the reactor.  Does not return from flux_reactor_run() until the module is unloaded.
	flux_reactor_t *exegetical_reactor = flux_get_reactor(h);
	assert( exegetical_reactor );
	rc = flux_reactor_run ( exegetical_reactor, NO_FLAGS );
	assert( -1 != rc );

	// Unregister the service.
	flux_future_t *kvs_unregister_future;
	kvs_unregister_future = flux_service_unregister (h, default_service_name);    
	assert( NULL != kvs_unregister_future );
	rc = flux_future_wait_for( kvs_unregister_future, 10.0 );
	assert( rc != -1 );
	flux_future_destroy ( kvs_unregister_future );

	return 0;
}

MOD_NAME (MY_MOD_NAME);
