

#include "nfddos.h"
#include "string.h"
#include "time.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


int nfd_act_cmd(nfd_options_t *opt, nfd_action_t *a, nfd_action_cmd_t action_cmd) {

	char cmd[MAX_STRING];
	FILE *fh;
	struct stat st = {0};


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

	/* check and try to create action dir */
	if (stat(a->action_dir, &st) == -1) {
		if (mkdir(a->action_dir, 0700) < 0) {
			msg(MSG_ERROR, "Can not create %s direcorty (%s(.", a->action_dir, strerror(errno));
			return 0;
		}
	}	


	fh = popen(cmd, "w");

	if (fh == NULL) {
		msg(MSG_ERROR, "Can not execute external command %s.", cmd);
		return 0;
	}

	fflush(fh);

	if ( pclose(fh) != 0 ) {
		msg(MSG_ERROR, "External command %s was not executed.", cmd);
		return 0;
	}

	return 1;
}

int nfd_act_init(nfd_actions_t *act, int max_actions) {

	act->actions = (nfd_action_t **)calloc(max_actions, sizeof(nfd_action_t *)); 

	if (act->actions == NULL) {
		msg(MSG_ERROR, "Can not alocate memory for actions, required %d itesm", max_actions);
		return 0;
	}

	act->max_actions = max_actions;

	return 1;

}

/* update or insert new action */
int nfd_act_upsert(nfd_options_t *opt, nfd_actions_t *act, char *name) {

	nfd_action_t *a;
	int i;
	char buf[MAX_STRING];
	struct tm *tmp;
	time_t t;

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
		t = time(NULL);
		tmp = localtime(&t);
		if (tmp == NULL) {
			msg(MSG_ERROR, "Can get time %s:%d", __FILE__, __LINE__);
			return 0;
		}
		strftime(buf, MAX_STRING, "%F-%H-%M-%S", tmp);
		snprintf(a->action_dir, MAX_STRING, "%s/%s.%03d", opt->action_dir, buf, a->id);

		/* steps performed when a new action is invoked */
		/* XXX TODO */	
		strncpy(a->name, name, MAX_STRING);

		nfd_act_cmd(opt, a, NFD_ACTION_START);

		msg(MSG_INFO, "Starting action for profile %s dir %s", name, a->action_dir);
	}  else {

		msg(MSG_DEBUG, "UPD PROFILE ACTION FOR %s", name);
	}

	/* we update information in existing profile */
	a->updated = time(NULL);

	return 1;

}

/* check for expired actions */
int nfd_act_expire(nfd_actions_t *act, int expire_time) {

	nfd_action_t *a;
	int i;

	/* wall via all actions */
	for (i = 0; i < act->max_actions; i++) {
		if  ( act->actions[i] != NULL && act->actions[i]->updated + expire_time < time(NULL)) { 
			a = act->actions[i];

			msg(MSG_INFO, "Stopping action for profile %s dir %s", a->name, a->action_dir);


			free(a);
			act->actions[i] = NULL;
		}
	}

	return 1;
}

/* evaluate limits in profile and if limits are reached take action */
int nfd_act_eval_profile(nfd_options_t *opt, nfd_profile_t *profp, char *key, nfd_counter_t *c) {

	int over_limit = 0;
	int w = opt->last_window_size;
	char name[MAX_STRING];
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

	if (key == NULL || strncmp(key, "", MAX_STRING) == 0) {
		snprintf(name, MAX_STRING, "%s", profp->name);
	} else {
		snprintf(name, MAX_STRING, "%s/%s", profp->name, key);
	}

	msg(MSG_DEBUG, "Profile %s reached the limit", name);

	nfd_act_upsert(opt, &opt->actions, name);

	return 1;
}

