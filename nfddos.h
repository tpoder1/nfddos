
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libnf.h"
#include "msgs.h"
#include "histcounter.h"
#include "hash_table.h"

#define LOG_NAME "nfddos"
#define LOG_VERSION "1.1"
#define MAX_STRING 1024
#define LLUI long long unsigned int

/* aggregation and action filter */
typedef struct nfd_track_s {

	char fields[MAX_STRING];	/* fields string from config file */
	lnf_mem_t *mem;				/* aggregation unit */
	lnf_filter_t *filter;		/* action filter */

	struct nfd_track_s  *next_track;

} nfd_track_t;

/* counter  */
typedef struct nfd_counter_s {

	uint64_t bytes;
	uint64_t pkts;
	uint64_t flows;

	uint64_t total_bytes;
	uint64_t total_pkts;
	uint64_t total_flows;

	time_t last_updated;
	int active;

} nfd_counter_t;



/* profile parameters */
typedef struct nfd_profile_s {

	char name[MAX_STRING];			/* profile name */
	lnf_filter_t *input_filter;		/* input filter */
//	nfd_track_t *root_track;		/* pointer to root aggegation */
	int slot_size; 	 			/* window size to evaluate (in seconds) */
	int num_slots; 		 			/* number of tracking slots */
	int stop_delay;					/* delay before shaping rule is removed */
	int hash_buckets;				/* number of buckets in hash table */
	int per_dst_ip;					/* apply rule as per destination IP  */
//	histc_t hcounter;				/* histogram counter */
	nfd_counter_t counters;			/* profile counters */
	hash_table_t hash_table;		/* hash table with dst address */
//	uint64_t last_updated;			/* when the statistics were updated */
//	uint64_t window_start;			/* when the current window started */
//	int time_reported;		/* when last report was done - 0 if not reported */

	struct nfd_profile_s  *next_profile;

} nfd_profile_t;


/* general options of bwd */
typedef struct nfd_options_s {

	int debug;						/* debug mode */
	int debug_fromarg;				/* debug mode set on command line */
	int foreground;					/* do not daemonize */
	double treshold;   				/* treshold (0.78 = 78%) */
	int window_size;				/* window size to evaluate */
	int slot_size; 	 				/* window size to evaluate (in seconds) */
	int num_slots; 		 			/* number of tracking slots */
	int stop_delay;					/* delay before shaping rule is removed */
	int hash_buckets;				/* number of buckets in hash table */
	int last_expire_check;			/* timestam of last expire check */
	char config_file[MAX_STRING];	/* config gile name */
	char pid_file[MAX_STRING];		/* file with PID */
	char status_file[MAX_STRING];	/* file to export status data */
	char flow_queue_file[MAX_STRING];	/* file to temporary store flow data */
	char shm[MAX_STRING];			/* shm name for ringbuf */
	int pid_file_fromarg;			/* pid file set on command line  */
	char exec_start[MAX_STRING];	/* command to exec new rule */
	char exec_stop[MAX_STRING];		/* command to exec to remove rule */
	time_t tm_display;				/* timestamp of las displayed statistics */

	nfd_profile_t  *root_profile;

} nfd_options_t;


int nfd_parse_config(nfd_options_t *opt);
nfd_profile_t * nfd_profile_new(nfd_options_t *opt, const char *name);
int nfd_profile_set_filter(nfd_profile_t *nfd_profile, char *expr);
int nf_profile_add_track(nfd_profile_t *nfd_profile, char *fields, char *filter);

void aggr_callback(char *key, char *hval, char *uval, void *p);
int sort_callback(char *key1, char *val1, char *key2, char *val2, void *p);


//int exec_node_cmd(options_t *opt, stat_node_t *stat_node, action_t action);

