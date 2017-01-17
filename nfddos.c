
#include <stdio.h>
#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include <fcntl.h>
//#include "dataflow.h"

//#include <pcap.h>
//#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include "nfddos.h"
#include "msgs.h"
#include "daemonize.h"
//#include <netinet/tcp.h>
#include <pthread.h>

#define SCAN_TIME 10*1000		/* scan loop - every 10 msecs */
#define UPDATES_IN_STEP 50		/* max number of updates (new/del) in one cycle */


// Kopie zbyvajicich argumantu do rednoho retezce
char *copy_argv(char *argv[]) {
  char **arg;
  char *buf;
  int total_length = 0;

  for (arg = argv; *arg != NULL; arg++)
    total_length += (strlen(*arg) + 1); /* length of arg plus space */

  if (total_length == 0)
    return NULL;

  total_length++; /* add room for a null */

  buf = (char *)malloc(total_length);

  *buf = 0;
  for (arg = argv; *arg != NULL; arg++) {
    strcat(buf, *arg);
    strcat(buf, " ");
  }

  return buf;
}

int mkpidfile(nfd_options_t *opt) {

	FILE * fp;
	int pid;

	fp = fopen(opt->pid_file, "w");

	if (fp == NULL) {
		msg(MSG_INFO, "Can't create PID file %s (%s)", opt->pid_file, strerror(errno));
		return 0;
	}

	pid = getpid();
	fprintf(fp,"%d\n", pid);

	fclose(fp);

	return 1;

}


void read_data_loop(nfd_options_t *opt) {

	lnf_ring_t *ringp;
	lnf_rec_t *recp;
	nfd_profile_t *profp;


	if (lnf_ring_init(&ringp, opt->shm, 0) != LNF_OK) {
		msg(MSG_ERROR, "Can not open shm/ringbuf %s", opt->shm);
		return;
	}

	lnf_rec_init(&recp);

	msg(MSG_INFO, "Reading data from shm/ringbuf %s", opt->shm);


	while (lnf_ring_read(ringp, recp) != LNF_EOF) {


		/* match general filter */
		if (opt->filter != NULL && !lnf_filter_match(opt->filter, recp)) {
			continue;
		}

		pthread_mutex_lock(&opt->read_profile_lock);

		profp = opt->root_profile;

		while (profp != NULL) {
			nfd_prof_add_flow(profp, recp);
			profp = profp->next_profile;
		}

		pthread_mutex_unlock(&opt->read_profile_lock);

		/* store flow in queue */
		pthread_mutex_lock(&opt->queue_lock);
		if (opt->queue_file != NULL) {
			if (lnf_write(opt->queue_file, recp) != LNF_OK) {
				msg(MSG_ERROR, "Can not store flow record in queuefile");
			}
		}
		pthread_mutex_unlock(&opt->queue_lock);

	}

	lnf_ring_free(ringp);

}

void dump_data_loop(nfd_options_t *opt) {
	
	//nfd_profile_t *tmp;
	int last_switched = 0;
	int remains;

	last_switched = time(NULL);

	while (1) { 

		/* wait rest of the time */
		remains = opt->window_size - (time(NULL) - last_switched);
		
		msg(MSG_DEBUG, "Waiting for %d seconds", remains); 

		if (remains > 0) {
			sleep(remains);
		} else {
			msg(MSG_ERROR, "Performance issue: can not process data in time window, missing %d seconds", -1 * remains);
		}

/*
		if (opt->db_profiles) {
			nfd_db_load_profiles(&opt->db, &opt->dump_root_profile);
		}
*/

		opt->last_window_size = time(NULL) - last_switched;
		msg(MSG_DEBUG, "Last windows size %d", opt->last_window_size);

		/* rotate queuefiles */
		pthread_mutex_lock(&opt->queue_lock);
		nfd_queue_shift(opt);
		pthread_mutex_unlock(&opt->queue_lock);

		last_switched = time(NULL);

		pthread_mutex_lock(&opt->dump_profile_lock);

		/* drump actual dump profile (previous read profile) into DB  */
		nfd_prof_dump_all(opt);

		/* evaluate and export profiles */
		nfd_prof_eval_all(opt, &opt->root_profile);

		pthread_mutex_unlock(&opt->dump_profile_lock);


		nfd_act_expire(opt, &opt->actions, opt->stop_delay);

		/* update counters */
		
//		msg(MSG_DEBUG, "Starting DB counters update");
//		nfd_db_update_counters(&opt->db);
//		msg(MSG_DEBUG, "Finished DB counters update");

	
	} // main loop 

}
	

int main(int argc, char *argv[]) {
    extern int optind;
    char op;

	nfd_options_t opt = { 
		.debug = 0, 
//		.slot_size = 10, 
		.window_size = 60, 
		.stop_delay = 90, 
//		.num_slots = 30, 
//		.hash_buckets = 50000, 
		.db_type = NFD_DB_PGSQL, 
//		.treshold = 0.8,
		.export_interval = 10,
		.export_min_pps = 1,
		.root_profile = NULL,
		.max_actions = 512,
		.queue_num = 10,
		.queue_backtrack = 1
		};


	strcpy(opt.config_file, "./nfddos.conf");	
	strcpy(opt.pid_file, "/var/run/nfddos.pid");	
	strcpy(opt.exec_start, "./nfddos-start.sh");	
	strcpy(opt.exec_stop, "./nfddos-stop.sh");	
	strcpy(opt.exec_status, "./nfddos-status.sh");	
	strcpy(opt.action_dir, "./actions/");	
	strcpy(opt.status_file, "./nfddos.status");	
	strcpy(opt.queue_dir, "./nfddos-queue/");	
	strcpy(opt.db_connstr, "dbname=nfddos user=nfddos");	
	strcpy(opt.shm, "libnf-shm");	


    /*  process options */
	while ((op = getopt(argc, argv, "i:c:d:p:F?h")) != -1) {
		switch (op) {
			case 'c' : strncpy(opt.config_file,optarg, MAX_STRING); break;
			case 'p' : strncpy(opt.pid_file,optarg, MAX_STRING); opt.pid_file_fromarg = 1; break;
			case 'd' : opt.debug = atoi(optarg); opt.debug_fromarg = 1; break;
			case 'F' : opt.foreground = 1; if (!opt.debug_fromarg) opt.debug = 1; break;
			case 'h' :
			case '?' : opt.debug = 1; opt.foreground = 1; msg_init(opt.debug); help(); break;
			} // konec switch op 
    } // konec while op...

	msg_init(opt.debug);

	if (opt.foreground == 0) {
		if ( !daemonize() )  {
			fprintf(stderr, "Can not daemonize process\n");
			exit(1);
		}
	}


	/* process config file */
	if (!nfd_cfg_parse(&opt)) {
		exit(1);
	}

	/* create pid file */
	mkpidfile(&opt);

	/* connect to DB */
	if (!nfd_act_init(&opt.actions, opt.max_actions)) {
		exit(1);
	}

	/* connect to DB */
	if (!nfd_db_init(&opt.db, opt.db_type, opt.db_connstr)) {
		exit(1);
	}

	/* init queue file */
	if (!nfd_queue_shift(&opt)) {
		exit(1);
	}

	/* prepare thread locks */
	if (pthread_mutex_init(&opt.read_profile_lock, NULL) != 0 || pthread_mutex_init(&opt.dump_profile_lock, NULL) != 0) {
		msg(MSG_ERROR, "Canot init mutex %s:%d", __FILE__, __LINE__);
		exit(1);
	}
	
	if (pthread_mutex_init(&opt.queue_lock, NULL) != 0) {
		msg(MSG_ERROR, "Canot init mutex %s:%d", __FILE__, __LINE__);
		exit(1);
	}
	/* load profiles for first run */
//	if (opt.db_profiles) {
//		nfd_db_load_profiles(&opt.db, &opt.root_profile);
//	}

	/* execute therad with read loop */
	if (pthread_create(&opt.read_thread, NULL, (void *)&read_data_loop, (void *)&opt) != 0) {
		msg(MSG_ERROR, "Canot init read thread %s:%d", __FILE__, __LINE__);
		exit(1);
	}

	dump_data_loop(&opt);


	return 0;
}

