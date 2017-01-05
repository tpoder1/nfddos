#ifndef __NFDDOS_H_
#define __NFDDOS_H_


#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "libnf.h"
#include "msgs.h"
#include "histcounter.h"
#include "hash_table.h"
#include "db.h"

#define LOG_NAME "nfddos"
#define LOG_VERSION "1.2"
#define MAX_STRING 1024
#define MAX_AGGR_FIELDS 5
#define LLUI long long unsigned int

/* counter  */
typedef struct nfd_counter_s {

	uint64_t bytes;
	uint64_t pkts;
	uint64_t flows;

} nfd_counter_t;



/* profile parameters */
typedef struct nfd_profile_s {

	char id[MAX_STRING];			/* profile id */
	char name[MAX_STRING];			/* profile name */
	char filter_expr[MAX_STRING];		/* filter expr */
	char dynamic_fields_expr[MAX_STRING];	/* aggregation expr */
	lnf_filter_t *filter;			/* input filter */
//	histc_t hcounter;				/* histogram counter */
	nfd_counter_t counters;			/* profile counters */
	nfd_counter_t limits;			/* profile limits */
	lnf_mem_t *mem;					/* aggregation unit */
	int dynamic_fields[MAX_AGGR_FIELDS];	/* array of aggregation fields (last field 0 ) */

	struct nfd_profile_s  *next_profile;

} nfd_profile_t;

/* additional structutres for dynamic profile */
typedef struct nfd_dyn_profile_s {
	char key[MAX_STRING];			/* string representation of values */
	char filter_expr[MAX_STRING];	/* filter expr for filter */
} nfd_dyn_profile_t;

/* one action entry */
typedef struct nfd_action_s {

	char name[MAX_STRING];	
	int id;							/* index to action array */
	char action_dir[MAX_STRING];	/* fill directory name with action metadata */
	char filter_expr[MAX_STRING];	/* action filter */
	nfd_counter_t counters;			/* last conters value */
	int dynamic;					/* is the field dynamic ? */
	nfd_dyn_profile_t dp;			/* copy of the dynamic profile from parent profile */
	lnf_filter_t *filter;			/* action filter */
	lnf_file_t *file;				/* action output file */
	time_t tm_updated;				/* timestamp of start and updated action */
	time_t tm_start;

} nfd_action_t;

/* actions structure */
typedef struct nfd_actions_s {

	int max_actions; 
	nfd_action_t **actions;

} nfd_actions_t;

typedef enum {
	NFD_ACTION_START,
	NFD_ACTION_STOP
} nfd_action_cmd_t;


/* general options of bwd */
typedef struct nfd_options_s {

	int debug;						/* debug mode */
	int debug_fromarg;				/* debug mode set on command line */
	int foreground;					/* do not daemonize */
	int export_min_pps;   			/* minimum pps to export to DB  */
	int export_interval;   			/* period of exporting data to DB */
	int window_size;				/* window size to evaluate */
	int last_window_size;			/* real last window size */
	lnf_filter_t *filter;			/* general filter */
//	int slot_size; 	 				/* window size to evaluate (in seconds) */
//	int num_slots; 		 			/* number of tracking slots */
	int stop_delay;					/* delay before shaping rule is removed */
	int db_profiles;				/* load profiles from DB */
//	int hash_buckets;				/* number of buckets in hash table */
//	int last_expire_check;			/* timestam of last expire check */
	char config_file[MAX_STRING];	/* config gile name */
	char pid_file[MAX_STRING];		/* file with PID */
	int pid_file_fromarg;			/* pid file set on command line  */
	char status_file[MAX_STRING];	/* file to export status data */
	char queue_dir[MAX_STRING];		/* file to temporary store flow data */
	int  queue_num;					/* number of time windows stored in queue  */
	int  queue_backtrack;			/* number of time windows to backtrack when flow data for profile is created   */
	lnf_file_t  *queue_file;		/* open lnf_file_t handler  */
	char shm[MAX_STRING];			/* shm name for ringbuf */

	char exec_start[MAX_STRING];	/* command to exec new rule */
	char exec_stop[MAX_STRING];		/* command to exec to remove rule */
	char action_dir[MAX_STRING];	/* directory to put profile/action data during action */

	nfd_db_type_t db_type;			/* type of db engine  */
	nfd_db_t db;					/* db handle - see db.h  */
	char db_connstr[MAX_STRING];	/* db connection string  */
//	time_t tm_display;				/* timestamp of las displayed statistics */

	int	max_actions;				/* max number of actions */
	nfd_actions_t	actions;		/* reference to action structure */

	nfd_profile_t  *read_root_profile;	/* root profile where data are filled in */
	nfd_profile_t  *dump_root_profile;	/* root profile which is dumped into DB */

	pthread_t read_thread;			/* read loop */
	pthread_t counter_thread;		/* counter loop */

	pthread_mutex_t read_lock;		/* lock for fill thread */
	pthread_mutex_t dump_lock;		/* lock for dump thread */

} nfd_options_t;


int nfd_cfg_parse(nfd_options_t *opt);
int nfd_cfg_set_filter(nfd_options_t *opt, char *filter);



int nfd_prof_dump(nfd_options_t *opt, FILE *fh, nfd_profile_t *profp);
int nfd_prof_dump_all(nfd_options_t *opt);
nfd_profile_t * nfd_prof_new(char *name);
int nfd_prof_add_flow(nfd_profile_t *profp, lnf_rec_t *recp);
int db_export_profiles(nfd_options_t *opt, nfd_profile_t **root_profile);
int nfd_prof_set_filter(nfd_profile_t *profp, char *filter); 
int nfd_prof_set_dynamic(nfd_profile_t *profp, char *fields);
int nfd_prof_add(nfd_profile_t **root_profile, nfd_profile_t *profp);
void nfd_prof_free(nfd_profile_t *profp);


int nfd_act_init(nfd_actions_t *act, int max_actions);
int nfd_act_dump(nfd_options_t *opt, FILE *fh, nfd_action_t *a);
int nfd_act_cmd(nfd_options_t *opt, nfd_profile_t *profp, nfd_action_t *a, nfd_action_cmd_t action_cmd);
int nfd_act_upsert(nfd_options_t *opt, nfd_actions_t *act, nfd_dyn_profile_t *dp, nfd_profile_t *profp, nfd_counter_t *counters);
int nfd_act_expire(nfd_options_t *opt, nfd_actions_t *act, int expire_time);
int nfd_act_eval_profile(nfd_options_t *opt, nfd_profile_t *profp, nfd_dyn_profile_t *dp, nfd_counter_t *c);


int nfd_db_init(nfd_db_t *db, nfd_db_type_t db_type, const char *connstr);

int nfd_db_load_profiles(nfd_db_t *db, nfd_profile_t **root_profile);
int nfd_db_mk_profile(nfd_profile_t **nfd_profile, char *id, char *filter, char *fields, nfd_counter_t *limits, char *errbuf);
int nfd_db_store_stats(nfd_db_t *db, char *id, char *key, int window, nfd_counter_t *c);

void nfd_db_free(nfd_db_t *db);

void aggr_callback(char *key, char *hval, char *uval, void *p);
int sort_callback(char *key1, char *val1, char *key2, char *val2, void *p);


int nfd_queue_shift(nfd_options_t *opt);
int nfd_queue_add_flow(nfd_options_t *opt, lnf_file_t *output, lnf_filter_t *filter, int backtrack);

typedef void* yyscan_t;

int yyget_lineno (yyscan_t yyscanner );
int yyparse (yyscan_t scanner, nfd_options_t *opt);
int yylex_init (yyscan_t* scanner);
void yyset_in  (FILE * in_str ,yyscan_t yyscanner );
int yyparse (yyscan_t scanner, nfd_options_t *opt);
int yylex_destroy (yyscan_t yyscanner );



#endif // __NFDDOS_H_

