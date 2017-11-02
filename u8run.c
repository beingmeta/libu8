/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2017 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.

*/

#include "libu8/u8source.h"
#include "libu8/libu8.h"
#include "libu8/u8elapsed.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8logging.h"
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"
#include "libu8/u8timefns.h"
#include "libu8/u8fileio.h"
#include "libu8/u8printf.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

static u8_string rundir = "/var/run/libu8/";
static uid_t runuser  = -1;
static gid_t rungroup = -1;
static int umask_value = -1;

static u8_string main_job_id = NULL;
static u8_string pid_file = NULL;
static u8_string ppid_file = NULL;
static pid_t dependent = -1;


void usage()
{
  fprintf(stderr,"fdrun [+daemon] [env=val]* jobid [env=val]* exename [args..]\n");
}

static u8_string procpath(u8_string job_id,u8_string suffix)
{
  if ( (strchr(job_id,'/')) || (strchr(job_id,'.')) )
    return u8_string_append(job_id,".",suffix,NULL);
  else {
    u8_string name = u8_string_append(job_id,".",suffix,NULL);
    u8_string path = u8_mkpath(rundir,name);
    u8_free(name);
    return path;}
}

/* Getting */

static uid_t lookup_uid(u8_string s)
{
  struct passwd _info, *info=NULL;
  char buf[2048];
  int rv = getpwnam_r(s,&_info,buf,2048,&info);
  if (rv<0) {
    errno=0;
    return -1;}
  else if (info) {
    uid_t uid = info->pw_uid;
    errno=0;
    return uid;}
  else {
    errno=0;
    return -1;}
}

static uid_t lookup_gid(u8_string s)
{
  struct group _info, *info=NULL;
  char buf[2048];
  int rv = getgrnam_r(s,&_info,buf,2048,&info);
  if (rv<0) {
    errno=0;
    return -1;}
  else if (info) {
    uid_t uid = info->gr_gid;
    errno=0;
    return uid;}
  else {
    errno=0;
    return -1;}
}

static int parse_umask(u8_string umask_init)
{
  u8_string parse_ends=NULL;
  int umask_val = strtol(umask_init,(char **)(&parse_ends),0);
  if (parse_ends == umask_init) {
    u8_log(LOGWARN,"UMASKFailed",
	   "Couldn't parse value %s (errno=%d:%s)",
	   umask_init,errno,u8_strerror(errno));
    errno=0;
    return -1;}
  else return umask_val;
}

static pid_t read_pid(u8_string file)
{
  FILE *f = u8_fopen(file,"r");
  if (f == NULL)
    return -1;
  long long intval=-1;
  int rv = fscanf(f,"%lld",&intval);
  fclose(f);
  if ( (rv < 1) || (intval<0) )
    return -1;
  else return (pid_t) intval;
}

static pid_t live_pidp(pid_t pid)
{
  if (pid>0) {
    int rv = kill(pid,0);
    if (rv<0) {
      int e = errno; errno=0;
      if (e == ESRCH)
	return 0;
      else return 1;}
    else return 1;}
  else return 0;
}

#define elapsed_since(moment) ((u8_elapsed_time())-moment)

static int kill_existing(u8_string filename,pid_t pid,double wait)
{
  if (pid>0) {
    u8_log(LOG_WARN,"ReplaceExisting",
	   "Killing process #%lld from %s",pid,filename);
    int rv = kill(pid,SIGTERM);
    if (rv<0) {
      u8_log(LOG_WARN,"SignalFailed","Couldn't send SIGTERM to %lld",pid);
      return -1;}
    double started=u8_elapsed_time();
    while ( ( elapsed_since(started) < wait) && (live_pidp(pid) ) )
      u8_sleep(0.5);
    if (live_pidp(pid)) {
      u8_log(LOG_CRIT,"SignalFailed",
	     "Process %lld still exists %f seconds after SIGTERM, using SIGKILL",
	     pid,u8_elapsed_time()-started);
      kill(pid,SIGKILL);
      return 0;}
    else return 1;}
  else return 0;
}

static int n_cycles=0, doexit=0, paused=0, restart=0;
static double last_launch = -1, fast_fail = 3, fail_start=-1;
static double exec_wait=0, exit_wait=0, error_wait=1.0, max_wait=120, backoff=10;

static int launch_loop(u8_string job_id,char **real_args,int n_args);
static void setup_signals(void);

int main(int argc,char *argv[])
{
  int i=1, launching = 0;
  char **launch_args = u8_alloc_n(argc,char *), *cmd=NULL;
  int n_args = 0;
  u8_string job_id=NULL;

  u8_log_show_date=1;
  u8_log_show_procinfo=1;
  u8_log_show_threadinfo=1;
  u8_log_show_elapsed=1;
  u8_log_show_appid=1;

  if (argc<3) {
    usage();
    exit(1);}
  else if (strcasecmp(argv[1],"+daemon")==0) {
    launching = 1;
    i++;}
  else {}
  while (i<argc) {
    char *arg = argv[i];
    if (strchr(arg,'=')) {
      char *eqpos = strchr(arg,'=');
      size_t name_len = eqpos-arg;
      char varname[name_len+1];
      strncpy(varname,arg,name_len);
      varname[name_len]='\0';
      setenv(varname,eqpos+1,1);
      i++;}
    else if (job_id == NULL) {
      u8_string job_arg = u8_fromlibc(arg);
      if (strchr(job_arg,'/')) {
	job_id = u8_abspath(job_arg,NULL);
	u8_free(job_arg);}
      else job_id = job_arg;
      i++;}
    else {
      cmd = arg;
      n_args=argc-i;
      memcpy(&(launch_args[0]),&(argv[i]),sizeof(char *)*n_args);
      break;}}
  launch_args[n_args]=NULL;
  main_job_id = job_id;

  if (getenv("EXECWAIT")) exec_wait=u8_getenv_float("EXECWAIT",0);
  if (getenv("LOGLEVEL")) u8_loglevel=u8_getenv_int("LOGLEVEL",5);
  if (getenv("RUNDIR"))   rundir=u8_getenv("RUNDIR");
  if (getenv("FASTFAIL")) fast_fail=u8_getenv_float("FASTFAIL",fast_fail);
  if (getenv("EXITWAIT")) exit_wait=u8_getenv_float("EXITWAIT",exit_wait);
  if (getenv("ERRWAIT"))  error_wait=u8_getenv_float("ERRORWAIT",error_wait);
  if (getenv("BACKOFF"))  backoff=u8_getenv_float("BACKOFF",backoff);
  if (getenv("MAXWAIT"))  max_wait=u8_getenv_float("BACKOFF",max_wait);

  pid_file = procpath(job_id,"pid");
  ppid_file = procpath(job_id,"ppid");
  if (u8_file_existsp(ppid_file)) {
    pid_t live = read_pid(ppid_file);
    if (live_pidp(live)) {
      if (u8_getenv("FORCE"))
	kill_existing(ppid_file,live,u8_getenv_float("KILLWAIT",15));
      else {
	u8_log(LOGCRIT,"ExistingPPID",
	       "The file %s already specifies is a live PID (%lld)",
	       ppid_file,live);
	exit(1);}}
    else u8_removefile(ppid_file);}
  if (u8_file_existsp(pid_file)) {
    pid_t live = read_pid(pid_file);
    if (live_pidp(live)) {
      if (u8_getenv("FORCE"))
	kill_existing(pid_file,live,u8_getenv_float("KILLWAIT",15));
      else {
	u8_log(LOGCRIT,"ExistingPID",
	       "The file %s already specifies is a live PID (%lld)",
	       ppid_file,live);
	exit(1);}}
    else u8_removefile(ppid_file);}
  if (launching) {
    pid_t launched = fork();
    if (launched<0) {
      u8_log(LOG_CRIT,"ForkFailed",
	     "Couldn't fork to launch %s",job_id);
      exit(1);}
    else if (launched) {
      u8_log(LOG_NOTICE,"u8run/launch",
	     "Forked (%lld) launch loop for %s (%s), waiting for %s",
	     (long long)launched,job_id,cmd,pid_file);
      while (! (u8_file_existsp(pid_file)) ) sleep(1);
      if (u8_file_existsp(pid_file))
	exit(0);
      else exit(1);}}
  setup_signals();
  return launch_loop(job_id,launch_args,n_args);
}

static pid_t kill_child(u8_string job_id,pid_t pid,u8_string pid_file)
{
  if (pid<=0) return -1;
  u8_log(LOG_INFO,"u8run/signal","Sending SIGTERM to %s:%lld",job_id,pid);
  int rv = kill(pid,SIGTERM);
  if (rv<0) return rv;
  long long term_wait = u8_getenv_int("TERMWAIT",30);
  double term_pause = u8_getenv_float("TERMPAUSE",0.4);
  double checkin = u8_elapsed_time();
  while (u8_file_existsp(pid_file)) {
    u8_log(LOG_INFO,"u8run/signal/wait",
	   "Waiting for PID file %s to be removed by %s:%lld",
	   pid_file,job_id,pid);
    double now = u8_elapsed_time();
    if ( (now-checkin) > term_wait) {
      int rv = kill(pid,SIGKILL);
      if (rv<0) return rv;
      else u8_removefile(pid_file);
      return 0;}
    else u8_sleep(term_pause);}
  u8_log(LOG_NOTICE,"u8run/signal/wait",
	 "%s:%lld finished, PID file %s gone",
	 pid_file,job_id,pid);
  return -1;
}

static pid_t dolaunch(char **launch_args)
{
  if (u8_file_existsp(pid_file)) {
    u8_log(LOG_WARN,"LeftoverPIDfile",
	   "Removing leftover pid file %s",pid_file);
    int rv = u8_removefile(pid_file);
    if (rv<0) {
      u8_log(LOG_CRIT,"PIDCleanupFailed",
	     "Couldn't remove existing PID file %s",pid_file);
      return -1;}}

  u8_string user_spec      = u8_getenv("RUNUSER");
  u8_string group_spec     = u8_getenv("RUNGROUP");
  u8_string umask_init     = u8_getenv("UMASK");
  if (user_spec) runuser   = lookup_uid(user_spec);
  if (group_spec) rungroup = lookup_gid(group_spec);
  if (umask_init) umask_value = parse_umask(umask_init);
  double now = u8_elapsed_time();
  last_launch = u8_elapsed_time();
  pid_t pid = fork();
  if (pid == 0) {
    if (umask_value>=0) umask(umask_value);
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    if (exec_wait > 0) u8_sleep(exec_wait);
    if (runuser>0) setuid(runuser);
    if (rungroup>0) setgid(rungroup);
    return execvp(launch_args[0],launch_args);}
  else return pid;
}

static void fail_wait(u8_condition reason,u8_string job_id,char **launch_args)
{
  double fail_duration = 0;
  if (fail_start<0)
    fail_start=u8_elapsed_time();
  else fail_duration = u8_elapsed_time()-fail_start;
  double fail_pause = (fail_duration > max_wait) ? (fail_duration) :
    fail_duration+backoff;
  u8_log(LOG_NOTICE,reason,
	 "Waiting %f seconds before resuming %s (%s)",
	 fail_pause,job_id,launch_args[0]);
  double wait_start = u8_elapsed_time();
  while ( (elapsed_since(wait_start) < fail_pause) &&
	  (! doexit ) && (! paused ) )
    u8_sleep(0.5);
}

/* Signals */

struct sigaction sigaction_pause, sigaction_cont;
struct sigaction sigaction_exit, sigaction_hup;

static void log_signal(int signum,siginfo_t *info,void *stuff)
{
  u8_log(LOG_INFO,"Signal","Signal %d, errno=%d, status=%d",
	 info->si_signo,info->si_errno,info->si_status);
}

static void siginfo_cont(int signum,siginfo_t *info,void *stuff)
{
  paused = 0;
  restart = 1;
}

static void siginfo_exit(int signum,siginfo_t *info,void *stuff)
{
  paused = 0;
  doexit = 1;
}

static void siginfo_hup(int signum,siginfo_t *info,void *stuff)
{
  paused = 0;
  restart = 1;
}

static void siginfo_pause(int signum,siginfo_t *info,void *stuff)
{
  paused = 1;
}

static void setup_signals()
{
  sigaction_pause.sa_sigaction = siginfo_pause;
  sigaction_pause.sa_flags = SA_SIGINFO;
  sigemptyset(&(sigaction_pause.sa_mask));

  sigaction_cont.sa_sigaction = siginfo_cont;
  sigaction_cont.sa_flags = SA_SIGINFO;
  sigemptyset(&(sigaction_cont.sa_mask));

  sigaction_exit.sa_sigaction = siginfo_exit;
  sigaction_exit.sa_flags = SA_SIGINFO;
  sigemptyset(&(sigaction_exit.sa_mask));

  sigaction_hup.sa_sigaction = siginfo_hup;
  sigaction_hup.sa_flags = SA_SIGINFO;
  sigemptyset(&(sigaction_hup.sa_mask));

  sigaction(SIGINT,&(sigaction_exit),NULL);
  sigaction(SIGTERM,&(sigaction_exit),NULL);
  sigaction(SIGQUIT,&(sigaction_exit),NULL);
  sigaction(SIGTSTP,&(sigaction_pause),NULL);
  sigaction(SIGUSR1,&(sigaction_pause),NULL);
  sigaction(SIGCONT,&(sigaction_cont),NULL);
  sigaction(SIGHUP,&(sigaction_hup),NULL);
}

static void exit_u8run()
{
  if (dependent > 0)
    kill_child(main_job_id,dependent,pid_file);
  if (ppid_file) {
    u8_string filename = ppid_file;
    int rv = u8_removefile(filename);
    ppid_file=NULL;
    u8_free(filename);}
}

static u8_string write_ppid_file(u8_string job_id)
{
  if (u8_file_existsp(ppid_file))
    u8_log(LOGWARN,"OverwritePPID",
	   "Overwriting existing PPID file %s for %s",
	   ppid_file,job_id);
  FILE *f = u8_fopen(ppid_file,"w");
  if (f) {
    pid_t pid = getpid();
    u8_byte pidstring[64]; u8_sprintf(pidstring,64,"%lld",pid);
    fputs(pidstring,f);
    fclose(f);}
  else {
    u8_log(LOGWARN,"PPIDFile","Couldn't write PPID file %s",ppid_file);
    return NULL;}
}

/* The launch/keep alive loop */

static int launch_loop(u8_string job_id,char **launch_args,int n_args)
{
  write_ppid_file(job_id);

  atexit(exit_u8run);

  u8_log(LOGNOTICE,"u8run","Sustaining job %s %s",launch_args[0],job_id);
  {int i=1; while (i < n_args) {
      u8_log(LOG_INFO,"u8run/arg","    %s",launch_args[i++]);}}

  double started = u8_elapsed_time();
  pid_t pid = dolaunch(launch_args);
  int n_cycles = 1;

  while (pid>0) {
    int status = 0;
    int wait_rv = waitpid(pid,&status,0);
    int exited = (wait_rv == pid) && (kill(pid,0) < 0);
    double now = u8_elapsed_time();

    u8_log(LOGNOTICE,"u8run/INT",
	   "Job %s:%lld signalled with rv=%d, do_exit=%d, paused=%d, "
	   "exited=%d, restart=%d, started=%f now=%f",
	   job_id,pid,wait_rv,doexit,paused,exited,restart,started,now);

    if (exited)
      u8_log(LOGNOTICE,"u8run","Job %s:%lld exited %s with status %d",
	     job_id,pid,
	     (WIFSIGNALED(status)) ? ("on signal") : ("normally"),
	     WEXITSTATUS(status));

    if (doexit) {
      /* If someone has set doexit, just terminate the child and exit */
      if (! exited) pid = kill_child(job_id,pid,pid_file);
      exit(0);}
    else if ( (exited) && ((now-started) < fast_fail) )
      /* If the child exited very quickly, count it as a failure
	 and pause before restarting. */
      fail_wait("FastFail",job_id,launch_args);
    else if ( (exited) && (WEXITSTATUS(status)) )
      /* If the child exited with a non-zero status, also wait a bit */
      fail_wait("ExitError",job_id,launch_args);
    else fail_start = 0;
    /* If you've been paused, kill your job (if any) and loop until
       you're not, calling wait() in the loop */
    if (paused) {
      int pause_status = 0; int pause_rv = 0;
      u8_log(LOG_WARN,"Paused","Pausing %s, terminating %lld",
	     job_id,(long long)pid);
      if ( (pid>0) && (!(exited)) )
	pid = kill_child(job_id,pid,pid_file);
      while ( (paused) && (pause_rv=wait(&pause_status)) ) {
	u8_log(LOG_INFO,"WakeUp","Woke up for rv=%d",pause_rv);
	/* Handle signals while paused */
	if (doexit) {
	  kill_child(job_id,pid,pid_file);
	  exit(0);}
	else if (restart) {
	  paused=0; break;}
	else if (paused)
	  /* Ignore signal, keep going */
	  continue;
	else break;}}
    if (doexit) {
      if (! exited )
	pid = kill_child(job_id,pid,pid_file);
      exit(0);}
    else if (restart) {
      u8_log(LOG_NOTICE,"RESTART","Handling requested restart of %s",job_id);
      if ( (! exited ) && (pid>0) ) {
	u8_log(LOG_WARN,"Restart/cleanup",
	       "Terminating existing %s:%lld",job_id,pid);
	pid = kill_child(job_id,pid,pid_file);}
      else u8_log(LOG_INFO,"Restarting","Restarting job %s",job_id);
      started = u8_elapsed_time();
      pid = dolaunch(launch_args);
      u8_log(LOG_NOTICE,"Restarted","Restarted job %s:%lld",job_id,pid);
      restart = 0;}
    else if (! exited)
      /* Some other signal */
      continue;
    else {
      u8_log(LOG_INFO,"Restarting",
	     "Automatically restarting %s after exit",job_id);
      started = u8_elapsed_time();
      pid = dolaunch(launch_args);
      u8_log(LOG_NOTICE,"Restarted",
	     "Automatically restarted %s after exit, pid=%lld",
	     job_id,(long long)pid);}}
}
