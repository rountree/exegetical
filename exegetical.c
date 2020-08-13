/* exegetical.c
 *
 * Test with:  flux python -c 'import flux; h=flux.Flux(); print(h.rpc("exegetical.foo", { "test": 42 }).get())'
 */
#define _GNU_SOURCE 1
#include <assert.h>
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
const char default_service_name[] = MY_MOD_NAME;
static uint32_t rank, size;

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


	if( !initialized ){
		if(rank == 1){
			flux_log(h, LOG_CRIT, "QQQ %s:%d Rank %d in timer code.\n", __FILE__, __LINE__, rank );
			flux_log(h, LOG_CRIT, "QQQ %s:%d Rank %d sending message to %s\n", __FILE__, __LINE__, rank, overlay[UPSTREAM] );
			/*
			flux_future_t *future_of_msg_to_upstream = 
				flux_rpc_pack(
					h,				// flux_t *h, 
					(const char*)overlay[UPSTREAM],	// const char *topic,
					FLUX_NODEID_ANY,		// uint32_t nodeid, 
					FLUX_RPC_NORESPONSE,		// int flags,
					"{i}",				// const char *fmt, 
					42				// ...);
				);
			assert( future_of_msg_to_upstream );
			flux_future_destroy( future_of_msg_to_upstream );
			*/
		}
		initialized = 1;
	}
}

static void overlay_cb (flux_t *h, flux_msg_handler_t *mh, const flux_msg_t *msg, void *arg){
	
	// Set up to catch messages addressed to overlay[ME].
	flux_log(h, LOG_CRIT, "QQQ %s:%d The message hander overlay_cb received a message addressed to %s\n", __FILE__, __LINE__, overlay[ME]);
	
	/*
	// Use flux_request_unpack() on the flux_msg_t msg.
	int msg_int;
	const char *incoming_topic;
	int rc = flux_request_unpack (
			msg,			// const flux_msg_t *msg,
			&incoming_topic,	// const char **topic,
			"{i}",			// const char *fmt, 
			&msg_int		// ...
	);
	if( -1 == rc ){
		flux_log(h, LOG_CRIT, "QQQ %s:%d flux_request_unpack() failed.\n", __FILE__, __LINE__);
	}else{
		flux_log(h, LOG_CRIT, "QQQ %s:%d overlay_cb topic:%s msg_int:%d\n", __FILE__, __LINE__, incoming_topic, msg_int );
	}
	*/
	
}


int mod_main (flux_t *h, int argc, char **argv)
{
	int rc;

	// Get rank via flux_get_rank() and print results to stderr.
	flux_get_rank(h, &rank);
	flux_get_size(h, &size);
	flux_log(h, LOG_CRIT, "%s:%d QQQ Hello from rank %" PRIu32 " of %" PRIu32 ".\n", __FILE__, __LINE__, rank, size);

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
	overlay_msg.topic_glob = overlay[ME]; 
	flux_msg_handler_t *mh_overlay = flux_msg_handler_create (h, overlay_msg, overlay_cb, (void*)h);
	assert( mh_overlay );
	flux_msg_handler_start(mh_overlay);
	flux_log(h, LOG_CRIT, "QQQ %s:%d rank %d set up handler for %s\n", __FILE__, __LINE__, rank, overlay[ME]);


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
