
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


int nfd_dump_profile(nfd_options_t *opt, FILE *fh, nfd_profile_t *profp) {

	uint64_t avg_bps, avg_pps, max_bps, max_pps, time;

	histc_get_avg(&profp->hcounter, &avg_bps, &avg_pps, &time);
	histc_get_peak(&profp->hcounter, &max_bps, &max_pps, &time);

	fprintf(fh, "%-20s %5lld Mb/s %5lld p/s\n", profp->id, (LLUI)avg_bps / 1000 / 1000, (LLUI)avg_pps);

	return 1;
}

int nfd_dump_profiles(nfd_options_t *opt) {

	nfd_profile_t *profp;
	FILE *fh;

	/* open dbdump file */
	fh = fopen(opt->status_file, "w"); 

	if (fh == NULL) {
		msg(MSG_ERROR, "Can not open file %s.", opt->status_file);
		return 0;
	}

	fprintf(fh, "TYPE    RULE                                       b/s        p/s     flow/s        bytes         pkts      flows act\n");

	profp = opt->read_root_profile;

	while (profp != NULL) {

		nfd_dump_profile(opt, fh, profp);

		profp = profp->next_profile;
	}	
	
	fclose(fh);	
	return 1;
}



/* add flow into pfifiles */
int nfd_profile_add_flow(nfd_profile_t *profp, lnf_rec_t *recp) {

	lnf_brec1_t brec1;
//	nfd_counter_t c; 
	nfd_counter_t *pc; 

	if (profp->filter != NULL) {
		/* record do not match profile filter */
		if (!lnf_filter_match(profp->filter, recp)) {
			return 0;
		}
	} 

	/* get flow data */
	lnf_rec_fget(recp, LNF_FLD_BREC1, &brec1);

	/* add to profile histogram */
	histc_add(&profp->hcounter, brec1.bytes, brec1.pkts, brec1.first, brec1.last - brec1.first);


	pc = &profp->counters;
	pc->bytes += brec1.bytes;
	pc->pkts += brec1.pkts;
	pc->flows += brec1.flows;

	if (profp->mem) { 
		lnf_mem_write(profp->mem, recp);
	}

	return 1;
}

void read_data_loop(nfd_options_t *opt) {

	lnf_ring_t *ringp;
	lnf_rec_t *recp;
	nfd_profile_t *profp;
	time_t	tm;


	if (lnf_ring_init(&ringp, opt->shm, 0) != LNF_OK) {
		msg(MSG_ERROR, "Can not open shm/ringbuf %s", opt->shm);
		return;
	}

	lnf_rec_init(&recp);

	msg(MSG_INFO, "Reading data from shm/ringbuf %s", opt->shm);


	while (lnf_ring_read(ringp, recp) != LNF_EOF) {

		pthread_mutex_lock(&opt->read_lock);


		profp = opt->read_root_profile;

		while (profp != NULL) {
			nfd_profile_add_flow(profp, recp);
			profp = profp->next_profile;
		}	

		tm = time(NULL);
		if (opt->tm_display != tm) {
			nfd_dump_profiles(opt);
			opt->tm_display = tm;
		}

		pthread_mutex_unlock(&opt->read_lock);
	}

	lnf_ring_free(ringp);

}

void nfd_profile_free(nfd_profile_t *profp) { 

		if (profp->mem != NULL) {
			lnf_mem_free(profp->mem);
		}

		if (profp->filter != NULL) {
			lnf_filter_free(profp->filter);
		}

		histc_free(&profp->hcounter);

		free(profp);
}


int db_export_profiles(nfd_options_t *opt, nfd_profile_t **root_profile) {

	nfd_profile_t *profp, *tmp;
	lnf_rec_t *rec;
	lnf_mem_cursor_t *cursor;
	nfd_counter_t counters;
	char buf[MAX_STRING];
	char key[MAX_STRING];
	int i, field;
	int ignored = 0;

	lnf_rec_init(&rec);

	profp = *root_profile;

	msg(MSG_DEBUG, "Start exporting data");
	nfd_db_begin_transaction(&opt->db);

	while (profp != NULL) {

		/* main profile statistics */
		nfd_db_store_stats(&opt->db, profp->id, "", opt->last_window_size, &profp->counters);


		/* aggargated statistics from lnf_mem */
		if (profp->mem != NULL) {

			lnf_mem_first_c(profp->mem, &cursor);
			while (cursor != NULL) {

				lnf_mem_read_c(profp->mem, cursor, rec);

				for (i = 0; i < MAX_AGGR_FIELDS; i++) {

					field = profp->fields[i]; 

					lnf_rec_fget(rec, field, &buf);

					/* last field in list */
					if (field == LNF_FLD_ZERO_) { break; }


					switch (lnf_fld_type(field)) { 

						case LNF_ADDR:
							inet_ntop(AF_INET6, buf, key, MAX_STRING);
							break;

					}

				}

				/* get counters */
				lnf_rec_fget(rec, LNF_FLD_DOCTETS, &counters.bytes);
				lnf_rec_fget(rec, LNF_FLD_DPKTS, &counters.pkts);
				lnf_rec_fget(rec, LNF_FLD_AGGR_FLOWS, &counters.flows);

				/* update aggr mem statistics */
				if (counters.pkts > opt->window_size * opt->export_min_pps) {
					nfd_db_store_stats(&opt->db, profp->id, key, opt->last_window_size, &counters);
				} else {
					ignored++;
				}

				nfd_act_eval_profile(opt, profp, key, &counters);

				/* go to next revord */
				lnf_mem_next_c(profp->mem, &cursor);
			}
		} else {
			/* profile without mem profile */
			nfd_act_eval_profile(opt, profp, NULL, &profp->counters);
		}


		tmp = profp;
		profp = profp->next_profile;

		nfd_profile_free(tmp);
	}	

	*root_profile = NULL;

	nfd_db_end_transaction(&opt->db);

	msg(MSG_DEBUG, "Data exported (%d updates, %d inserts, %d ignores)", opt->db.updated, opt->db.inserted, ignored);

	lnf_rec_free(rec);

	return 1;	

}

void dump_data_loop(nfd_options_t *opt) {
	
	nfd_profile_t *tmp;
	int last_switched = 0;
	int remains;

	while (1) { 

		/* load new profiles configuration into dump profile taht will be read profile in while  */
		nfd_db_load_profiles(&opt->db, &opt->dump_root_profile);

		opt->last_window_size = time(NULL) - last_switched;
		msg(MSG_DEBUG, "Last windows size %d", opt->last_window_size);

		/* switch read and dump profile */
		msg(MSG_DEBUG, "Switching read and dump profiles");
		pthread_mutex_lock(&opt->read_lock);
		pthread_mutex_lock(&opt->dump_lock);

		tmp = opt->read_root_profile;
		opt->read_root_profile = opt->dump_root_profile;
		opt->dump_root_profile = tmp;

		pthread_mutex_unlock(&opt->read_lock);
		pthread_mutex_unlock(&opt->dump_lock);

		last_switched = time(NULL);

		/* drump actual dump profile (previous read profile) into DB  */
		db_export_profiles(opt, &opt->dump_root_profile);


		/* update counters */
		/*
		msg(MSG_DEBUG, "Starting counters update");

		nfd_db_update_counters(&opt->db);

		msg(MSG_DEBUG, "Finished counters update");
		*/

		/* wait rest of the time */
		remains = opt->window_size - (time(NULL) - last_switched);
		
		msg(MSG_DEBUG, "Waiting for %d seconds", remains); 

		if (remains > 0) {
			sleep(remains);
		} else {
			msg(MSG_ERROR, "Performance issue: can not process data in time window, missing %d seconds", -1 * remains);
		}

	
	} // main loop 

}
	

int main(int argc, char *argv[]) {
    extern int optind;
    char op;

	nfd_options_t opt = { 
		.debug = 0, 
//		.slot_size = 10, 
		.window_size = 60, 
		.stop_delay = 60, 
//		.num_slots = 30, 
//		.hash_buckets = 50000, 
		.db_type = NFD_DB_PGSQL, 
//		.treshold = 0.8,
		.export_interval = 10,
		.export_min_pps = 1,
		.read_root_profile = NULL,
		.dump_root_profile = NULL,
		.max_actions = 512
		};


	strcpy(opt.config_file, "./nfddos.conf");	
	strcpy(opt.pid_file, "/var/run/nfddos.pid");	
	strcpy(opt.exec_start, "./nfddos-start.sh");	
	strcpy(opt.exec_stop, "./nfddos-stop.sh");	
	strcpy(opt.status_file, "./nfddos.status");	
	strcpy(opt.flow_queue_file, "./nfddos-queue.pcap");	
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
	if (!nfd_parse_config(&opt)) {
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

	/* prepare thread locks */
	if (pthread_mutex_init(&opt.read_lock, NULL) != 0 || pthread_mutex_init(&opt.dump_lock, NULL) != 0) {
		msg(MSG_ERROR, "Canot init mutexes %s:%d", __FILE__, __LINE__);
		exit(1);
	}
	
	/* load profiles for first run */
	nfd_db_load_profiles(&opt.db, &opt.read_root_profile);

	/* execute therad with read loop */
	if (pthread_create(&opt.read_thread, NULL, (void *)&read_data_loop, (void *)&opt) != 0) {
		msg(MSG_ERROR, "Canot init read thread %s:%d", __FILE__, __LINE__);
		exit(1);
	}

	dump_data_loop(&opt);


	return 0;
}

