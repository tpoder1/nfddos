

#include "nfddos.h"
#include "string.h"
#include "time.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define PROFILE_FN 		"profile"		/* profile file name */
#define NFCAP_FN 		"nfcap"			/* nfdump nfcap file name */


int nfd_act_init(nfd_actions_t *act, int max_actions) {

	act->actions = (nfd_action_t **)calloc(max_actions, sizeof(nfd_action_t *)); 

	if (act->actions == NULL) {
		msg(MSG_ERROR, "Can not alocate memory for actions, required %d itesm", max_actions);
		return 0;
	}

	act->max_actions = max_actions;

	return 1;

}

/* dump action information into file */
int nfd_act_dump(nfd_options_t *opt, FILE *fh, nfd_action_t *a) {

	char buf[MAX_STRING];
	struct tm *tmp;

	fprintf(fh, "action_name: %s\n", a->name);

	tmp = localtime(&a->tm_start);
	if (tmp == NULL) {
		msg(MSG_ERROR, "Can get time %s:%d", __FILE__, __LINE__);
		return 0;
	}
	strftime(buf, MAX_STRING, "%F %H:%M:%S", tmp);
	fprintf(fh, "action_start: %u (%s)\n", (unsigned int)a->tm_start, buf);

	tmp = localtime(&a->tm_updated);
	if (tmp == NULL) {
		msg(MSG_ERROR, "Can get time %s:%d", __FILE__, __LINE__);
		return 0;
	}
	strftime(buf, MAX_STRING, "%F %H:%M:%S", tmp);
	fprintf(fh, "action_updated: %u (%s)\n", (unsigned int)a->tm_updated, buf);

	fprintf(fh, "action_filter: %s\n", a->filter_expr);
	if (a->dynamic_field_val)  {
		fprintf(fh, "dynamic_field_value: %s\n", a->dynamic_field_val);
	}

	return 1;
}

int nfd_act_cmd(nfd_options_t *opt, nfd_profile_t *profp, nfd_action_t *a, nfd_action_cmd_t action_cmd) {

	char cmd[MAX_STRING];
	FILE *cmdh;


	/* build command */
	switch (action_cmd) {
		case NFD_ACTION_START: 
				snprintf(cmd, MAX_STRING, "%s %s %s", opt->exec_start, "new", a->action_dir); 
				break;
		case NFD_ACTION_STOP: 
				snprintf(cmd, MAX_STRING, "%s %s %s", opt->exec_stop, "del", a->action_dir); 
				break;
		default: 
				return 0; 
				break;
	}

	cmdh = popen(cmd, "w");

	if (cmdh == NULL) {
		msg(MSG_ERROR, "Can not execute external command %s.", cmd);
		return 0;
	}

	fflush(cmdh);

	if ( pclose(cmdh) != 0 ) {
		msg(MSG_ERROR, "External command %s was not executed.", cmd);
		return 0;
	}

	return 1;
}

/* prepare output dir and prepare data into it */
int nfd_act_update_dir(nfd_options_t *opt, nfd_action_t *a, nfd_profile_t *profp) {

	char buf[MAX_STRING];
	struct tm *tmp;
	FILE * profh;
	struct stat st = {0};


	a->tm_updated = a->tm_start;
	tmp = localtime(&a->tm_start);
	if (tmp == NULL) {
		msg(MSG_ERROR, "Can get time %s:%d", __FILE__, __LINE__);
		return 0;
	}

	strftime(buf, MAX_STRING, "%F-%H-%M-%S", tmp);
	snprintf(a->action_dir, MAX_STRING, "%s/%s.%03d", opt->action_dir, buf, a->id);

	/* check and try to create action dir */
	if (stat(a->action_dir, &st) == -1) {
		if (mkdir(a->action_dir, 0700) < 0) {
			msg(MSG_ERROR, "Can not create %s direcorty (%s).", a->action_dir, strerror(errno));
			return 0;
		}
	}

	/* steps performed when a new action is invoked */
	strncpy(a->filter_expr, profp->filter_expr, MAX_STRING);

	/* prepare profile filter */
	if (a->filter != NULL && ( strncmp(a->filter_expr, "", MAX_STRING) != 0 ) ) {
		if (lnf_filter_init_v2(&a->filter, a->filter_expr) != LNF_OK) {
			msg(MSG_ERROR, "Can not build filter \"%s\" %s:%d",a->filter_expr,  __FILE__, __LINE__);
		}
	}
		

	/* update data in action dir */
	if (profp != NULL) {
		snprintf(buf, MAX_STRING, "%s/%s", a->action_dir, PROFILE_FN);
		profh = fopen(buf, "w");

		if (profh != NULL) {
			nfd_prof_dump(opt, profh, profp);
			nfd_act_dump(opt, profh, a);
			fclose(profh);
		} else {
			msg(MSG_ERROR, "Can not create profile file %s (%s).", buf, strerror(errno));
		}
	}


	/* output flow file */
	if (a->file == NULL) {
		snprintf(buf, MAX_STRING, "%s/%s", a->action_dir, NFCAP_FN);
		if (lnf_open(&a->file, buf, LNF_WRITE, "nfddos") != LNF_OK) {
			msg(MSG_ERROR, "Can not open pcap file  \"%s\" %s:%d", buf,  __FILE__, __LINE__);
			a->file = NULL;
		}

		/* copy data from queue */
		if (a->file != NULL) {
			nfd_queue_add_flow(opt, a->file, a->filter, opt->queue_backtrack);
		}
	}


	return 1;

}

/* update or insert new action */
int nfd_act_upsert(nfd_options_t *opt, nfd_actions_t *act, char *dynamic_field_val, nfd_profile_t *profp) {

	nfd_action_t *a;
	int i;
	char name[MAX_STRING];

	if (dynamic_field_val == NULL || strncmp(dynamic_field_val, "", MAX_STRING) == 0) {
		snprintf(name, MAX_STRING, "%s", profp->name);
	} else {
		snprintf(name, MAX_STRING, "%s/%s", profp->name, dynamic_field_val);
	}

	msg(MSG_DEBUG, "Profile %s reached the limit", name);

	/* lookup existing action by name */
	a = NULL;
	for (i = 0; i < act->max_actions; i++) {
		if  ( act->actions[i] != NULL && act->actions[i]->name != NULL && strncmp(act->actions[i]->name, name, MAX_STRING) == 0)  {
			a = act->actions[i];
			break;
		}
	}

	/* action with this name was not found - we create new one */
	if (a == NULL) {
		a = calloc(sizeof(nfd_action_t), 1); 

		if (a == NULL) {
			msg(MSG_ERROR, "Can not alocate memory %s:%d", __FILE__, __LINE__);
			return 0;
		}

		/* find empty slot for new created actions */
		for (i = 0; i < act->max_actions; i++) {
			if  ( act->actions[i] == NULL ) {
				a->id = i;
				act->actions[i] = a;
				break;
			}
		}

		/* no free slots */
		if (i == act->max_actions - 1) {
			msg(MSG_ERROR, "Can find free action slot");
			free(a);
			return 0;
		}

		/* build action dir */
		a->tm_start = time(NULL);

		strncpy(a->name, name, MAX_STRING);
		if (dynamic_field_val != NULL) {
			strncpy(a->dynamic_field_val, dynamic_field_val, MAX_STRING);
		}

		/* prepare output dir */
		nfd_act_update_dir(opt, a, profp);

		/* all data are prepared now - exec start command */
		nfd_act_cmd(opt, profp, a, NFD_ACTION_START);

		msg(MSG_INFO, "Starting action for profile %s dir %s", name, a->action_dir);
	}  else {
		a->tm_updated = time(NULL);

		msg(MSG_DEBUG, "UPD PROFILE ACTION FOR %s", name);
	}

	/* we update information in existing profile */
	a->tm_updated = time(NULL);

	return 1;

}

/* check for expired actions */
int nfd_act_expire(nfd_options_t *opt, nfd_actions_t *act, int expire_time) {

	nfd_action_t *a;
	int i;

	/* wall via all actions */
	for (i = 0; i < act->max_actions; i++) {
		if  ( act->actions[i] != NULL && act->actions[i]->tm_updated + expire_time < time(NULL)) { 
			a = act->actions[i];

			if (a->filter != NULL) {
				lnf_filter_free(a->filter);
			}

			if (a->file != NULL) {
				lnf_close(a->file);
			}

			msg(MSG_INFO, "Stopping action for profile %s dir %s", a->name, a->action_dir);

			nfd_act_cmd(opt, NULL, a, NFD_ACTION_STOP);

			free(a);
			act->actions[i] = NULL;
		}
	}

	return 1;
}

/* evaluate limits in profile and if limits are reached take action */
int nfd_act_eval_profile(nfd_options_t *opt, nfd_profile_t *profp, char *dynamic_field_val, nfd_counter_t *c) {

	int over_limit = 0;
	int w = opt->last_window_size;

	/* check limits */
	if (profp->limits.bytes != 0 && profp->limits.bytes <= c->bytes / w) {
		over_limit = 1;
	}

	if (profp->limits.pkts != 0 && profp->limits.pkts <= c->pkts / w) {
		over_limit = 1;
	}

	if (profp->limits.flows != 0 && profp->limits.flows <= c->flows / w) {
		over_limit = 1;
	}

	if (!over_limit) {
		/* nothing to do */ 
		return 0;
	}


	nfd_act_upsert(opt, &opt->actions, dynamic_field_val, profp);

	return 1;
}

