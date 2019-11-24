/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
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
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

static u8_string run_dir = NULL;
static u8_string log_dir = NULL;
static u8_string log_file = NULL;
static u8_string err_file = NULL;
static u8_string job_id = NULL;
static u8_uid run_user  = -1;
static u8_gid run_group = -1;
static int umask_value = -1;

static u8_string pid_file = NULL;
static u8_string ppid_file = NULL;
static u8_string stop_file = NULL;
static pid_t dependent = -1;

static int run_as_daemon = 0;
static int restart_on_error = 0;
static int restart_on_exit = 0;

void usage()
{
  fprintf(stderr,"u8run [+opt|env=val|=jobid]* exename [args..]\n");
  fprintf(stderr,"  [opt]\t+daemon +restart\n");
  fprintf(stderr,"  [opt]\t+restart=onerr +restart=onexit\n");
  fprintf(stderr,"  [env]\tRUNDIR=dir JOBID=jobid STOPFILE=filename\n");
  fprintf(stderr,"  [env]\tLOGDIR=dir LOGFILE=file LOGLEVEL=n\n");
  fprintf(stderr,"  [env]\tRESTART=never|error|exit|always\n");
  fprintf(stderr,"  [env]\tWAIT=secs FASTFAIL=secs BACKOFF=secs\n");
  fprintf(stderr,"  [env]\tMAXWAIT=secs\n");
  fprintf(stderr,"  [env]\tRUNUSER=name|uid RUNGROUP=name|gid\n");
  fprintf(stderr,"  [env]\tUMASK=0ooo\n");
}

static u8_string procpath(u8_string job_id,u8_string suffix)
{
  if ( (strchr(job_id,'/')) || (strchr(job_id,'.')) )
    return u8_string_append(job_id,".",suffix,NULL);
  else {
    u8_string name = u8_string_append(job_id,".",suffix,NULL);
    u8_string path = u8_mkpath(run_dir,name);
    u8_free(name);
    return path;}
}

static u8_string logpath(u8_string job_id,u8_string suffix)
{
  if ( (strchr(job_id,'/')) || (strchr(job_id,'.')) )
    return u8_string_append(job_id,".",suffix,NULL);
  else {
    u8_string name = u8_string_append(job_id,".",suffix,NULL);
    u8_string path = u8_mkpath(log_dir,name);
    u8_free(name);
    return path;}
}

static void write_pid_file(u8_string);
static void write_ppid_file(u8_string);
static pid_t kill_child(u8_string job_id,pid_t pid,u8_string pid_file);

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
static double restart_wait=0, error_wait=1.0, max_wait=120, backoff=10;
static double pid_wait=0, pid_min_wait=2.0;

static void launch_loop(u8_string job_id,char **real_args,int n_args);
static void setup_signals(void);

int main(int argc,char *argv[])
{
  int i = 1, n_args = 0;
  char **launch_args = u8_alloc_n(argc,char *), *cmd=NULL;

  u8_log_show_date=1;
  u8_log_show_procinfo=1;
  u8_log_show_threadinfo=1;
  u8_log_show_elapsed=1;
  u8_log_show_appid=1;

  run_dir = u8_getenv("RUN_DIR");

  if (argc<2) {
    usage();
    exit(1);}
  else if ( (strcmp(argv[1],"+")==0) ||
            (strcasecmp(argv[1],"+daemon")==0) ) {
    run_as_daemon = 1;
    i++;}
  else NO_ELSE;
  while (i<argc) {
    char *arg = argv[i];
    if (*arg == '=') {
      if (job_id) u8_free(job_id);
      job_id = u8_fromlibc(arg);
      i++;}
    else if (arg[0] == '+') {
      if (strcasecmp(arg,"+daemon")==0)
        run_as_daemon = 1;
      else if ( (strcasecmp(arg,"+persist")==0) ||
                (strcasecmp(arg,"+restart")==0) ){
        restart_on_exit = 1;
        restart_on_error = 1;}
      else if (strcasecmp(arg,"+restart=onerr")==0)
        restart_on_error = 1;
      else if (strcasecmp(arg,"+restart=onexit")==0)
        restart_on_exit = 1;
      else {
        u8_log(LOG_CRIT,"InvalidRunOpt",
               "Invalid runopt '%s', should be "
               "+daemon +persist +restart +restart=onerr +restart=onexit",
               arg);
        usage();
        exit(1);}
      i++;}
    else if (strchr(arg,'=')) {
      char *eqpos = strchr(arg,'=');
      size_t name_len = eqpos-arg;
      char varname[name_len+1];
      strncpy(varname,arg,name_len);
      varname[name_len]='\0';
      setenv(varname,eqpos+1,1);
      if (strcasecmp(varname,"run_dir")==0) {
        if (run_dir) u8_free(run_dir);
        run_dir = u8_strdup(eqpos+1);}
      if (strcasecmp(varname,"jobid")==0) {
        if (job_id) u8_free(job_id);
        job_id = u8_strdup(eqpos+1);}
      i++;}
    else {
      cmd = arg;
      n_args=argc-i;
      memcpy(&(launch_args[0]),&(argv[i]),sizeof(char *)*n_args);
      break;}}
  /* Terminate the launch args */
  launch_args[n_args]=NULL;


  /* Make sure we have a jobid */
  if (job_id == NULL) {
    u8_string path = u8_fromlibc(cmd);
    job_id = u8_basename(path,"*");
    u8_free(path);}

  if (run_dir == NULL) run_dir = u8_getcwd();
  if ( (! (u8_directoryp(run_dir)) ) ||
       (! (u8_file_writablep(run_dir)) ) ) {
    u8_log(LOGCRIT,"BadRun_Dir",
           "The designated run_dir %s was not a writable directory",
           run_dir);
    exit(1);}
  log_dir = u8_getenv("U8LOGDIR");
  if (!(log_dir)) log_dir = u8_getenv("LOGDIR");
  if ( (!(log_dir)) && (run_as_daemon) )
    log_dir = u8_strdup(run_dir);

  {/* Compute runbase (run_dir/job_id) */
    u8_string runbase = u8_mkpath(run_dir,job_id);
    setenv("RUNBASE",runbase,0);
    u8_free(runbase);}

  if (err_file) {}
  else if (getenv("U8ERRFILE"))
    err_file=u8_getenv("U8ERRFILE");
  else if (getenv("ERRFILE"))
    err_file=u8_getenv("ERRFILE");
  else {}

  if (log_file) {}
  else if (getenv("U8LOGFILE"))
    log_file=u8_getenv("U8LOGFILE");
  else if (getenv("LOGFILE"))
    log_file=u8_getenv("LOGFILE");
  else if ( (run_as_daemon) || (log_dir) )
    log_file = logpath(job_id,"log");
  else {}

  if (getenv("LOGLEVEL")) u8_loglevel=u8_getenv_int("LOGLEVEL",5);
  if (getenv("FASTFAIL")) fast_fail=u8_getenv_float("FASTFAIL",fast_fail);
  if (getenv("WAIT"))     restart_wait=u8_getenv_float("WAIT",1);
  if (getenv("BACKOFF"))  backoff=u8_getenv_float("BACKOFF",backoff);
  if (getenv("MAXWAIT"))  max_wait=u8_getenv_float("MAXWAIT",max_wait);
  if (getenv("PIDWAIT"))  pid_wait=u8_getenv_float("PIDWAIT",pid_wait);
  if (getenv("PIDMINWAIT"))  pid_min_wait=u8_getenv_float("PIDMINWAIT",2.0);

  if (getenv("PIDFILE"))  pid_file=u8_getenv("PIDFILE");
  if (getenv("PPIDFILE"))  ppid_file=u8_getenv("PPIDFILE");
  if (getenv("STOPFILE"))  stop_file=u8_getenv("STOPFILE");

  char *restart_val = getenv("RESTART");
  if (restart_val==NULL) {}
  else if ( (strcasecmp(restart_val,"never")==0) ||
            (strcasecmp(restart_val,"no")==0) ) {
    restart_on_error=0;
    restart_on_exit=0;}
  else if (strcasecmp(restart_val,"always")==0) {
    restart_on_error=1;
    restart_on_exit=1;}
  else if (strcasecmp(restart_val,"error")==0) {
    restart_on_error=1;}
  else if (strcasecmp(restart_val,"exit")==0) {
    restart_on_exit=1;}
  else {
    usage();
    exit(0);}

  /* Now that we're past the usage checks, redirect output if
     requested. */
  if (log_file == NULL) {
    /* Just use existing stdout and stderr */}
  else if ( ( (log_file[0] == '.') || (log_file[0] == '-') ) &&
            (log_file[1]==0) ) {}
  else if (log_file) {
    int fd = open(log_file,O_WRONLY|O_APPEND|O_CREAT,0664);
    if (fd<0) {
      int eno = errno; errno=0;
      fprintf(stderr,"Error opening log_file %s: %s (%d)\n",
              log_file,u8_strerror(eno),eno);
      exit(1);}
    else {
      int rv = dup2(fd,STDOUT_FILENO);
      if (rv<0) {
        int eno = errno; errno=0;
        fprintf(stderr,"Error redirecting stdout to %s: %s (%d)\n",
                log_file,u8_strerror(eno),eno);
        close(fd);
        exit(1);}
      else if (err_file) {
        int err_fd = open(err_file,O_WRONLY|O_APPEND|O_CREAT,0664);
        if (fd<0) {
          int eno = errno; errno=0;
          fprintf(stderr,"Error opening err_file %s: %s (%d)\n",
                  err_file,u8_strerror(eno),eno);
          exit(1);}
        int rv = dup2(err_fd,STDERR_FILENO);
        if (rv<0) {
          int eno = errno; errno=0;
          fprintf(stderr,"Error redirecting stderr to %s: %s (%d)\n",
                  err_file,u8_strerror(eno),eno);
          close(fd);
          exit(1);}}
      else {
        int rv = dup2(fd,STDERR_FILENO);
        if (rv<0) {
          int eno = errno; errno=0;
          fprintf(stderr,"Error redirecting stderr to %s: %s (%d)\n",
                  log_file,u8_strerror(eno),eno);
          close(fd);
          exit(1);}}}}
  else NO_ELSE;

  if (pid_file == NULL) pid_file = procpath(job_id,"pid");
  if (ppid_file == NULL) ppid_file = procpath(job_id,"ppid");
  if (stop_file == NULL) stop_file = procpath(job_id,"stop");
  if (u8_file_existsp(stop_file)) {
    u8_log(LOG_WARN,"StopFile","Removing existing stop file %s",stop_file);
    int rv = u8_removefile(stop_file);
    if (rv<0)
      u8_log(LOG_CRIT,"StopFile","Couldn't remove existing stop file %s",
             stop_file);
    exit(1);}

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
  if (run_as_daemon) {
    pid_t launched = fork();
    if (launched<0) {
      u8_log(LOG_CRIT,"ForkFailed",
             "Couldn't fork to launch %s",job_id);
      exit(1);}
    else if (launched) {
      /* When launched, we wait for a PID file to exist before we exit
         ourselves and leave the daemon running.
         If pid_wait is specified, we wait for the core application to
         create the PID file; otherwise, we just wait PID_MIN_WAIT and
         we'll create the PID file after we fork. */
      u8_log(LOG_NOTICE,"u8run/launch",
             "Forked (%lld) launch loop for %s (%s), waiting for %s",
             (long long)launched,job_id,cmd,pid_file);
      double launch_start = u8_elapsed_time();
      while (! (u8_file_existsp(pid_file)) ) {
        if ( (elapsed_since(launch_start)) >
             ( (pid_wait > 0) ? (pid_wait) : (pid_min_wait) ) )
          break;
        sleep(1);}
      if (u8_file_existsp(pid_file))
        exit(0);
      else {
        /* Does this need to clean anything up? */
        u8_log(LOGCRIT,"AppLaunchFailed",
               "Timed out waiting for PID file %s for job %s",
               pid_file,job_id);
        kill_child(job_id,launched,pid_file);
        exit(1);}}}
  setup_signals();
  launch_loop(job_id,launch_args,n_args);
  return 0;
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
  if (pid_wait>0) {
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
           job_id,pid,pid_file);}
  else {
    u8_removefile(pid_file);
    u8_log(LOG_NOTICE,"u8run/signal/wait",
           "%s:%lld finished, removed PID file %s",
           job_id,pid,pid_file);}
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
  if (user_spec) run_user   = u8_getuid(user_spec);
  if (group_spec) run_group = u8_getgid(group_spec);
  if (umask_init) umask_value = parse_umask(umask_init);
  double now = u8_elapsed_time();
  last_launch = u8_elapsed_time();
  pid_t pid = fork();
  if (pid == 0) {
    if (umask_value>=0) umask(umask_value);
    if (! (u8_getenv_int("NOMASK",0)) ) {
      signal(SIGINT,SIG_IGN);
      signal(SIGTSTP,SIG_IGN);}
    if ( ((int)run_group) >= 0 ) {
      int rv = setgid(run_group);
      if (rv<0)  {
	int setgid_errno = errno; errno = 0;
	u8_log(LOGCRIT,"FailedSetGID",
	       "Couldn't set GUID to %d errno=%d:%s",
	       run_group,setgid_errno,u8_strerror(setgid_errno));
	exit(13);}
      else u8_log(LOG_NOTICE,"SetGID","Set GID to %d",run_group);}
    if ( ((int)run_user) >= 0) {
      int rv = setuid(run_user);
      if (rv<0)  {
	int setuid_errno = errno; errno = 0;
	u8_log(LOGCRIT,"FailedSetUID",
	       "Couldn't set UID to %d errno=%d:%s",
	       run_user,setuid_errno,u8_strerror(setuid_errno));
	exit(13);}
      else u8_log(LOG_NOTICE,"SetUID","Set UID to %d",run_user);}
    if (pid_wait > 0) {
      /* This means we expect the main procedure to populate the PID file */}
    else write_pid_file(job_id);
    if (pid_file) setenv("PIDFILE",pid_file,1);
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
    kill_child(job_id,dependent,pid_file);
  if (ppid_file) {
    u8_string filename = ppid_file;
    int rv = u8_removefile(filename);
    ppid_file=NULL;
    u8_free(filename);}
}

static void write_ppid_file(u8_string job_id)
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
    fclose(f);
    return;}
  else {
    u8_log(LOGWARN,"PPIDFile","Couldn't write PPID file %s",ppid_file);
    return;}
}

static void write_pid_file(u8_string job_id)
{
  if (u8_file_existsp(pid_file))
    u8_log(LOGWARN,"OverwritePPID",
	   "Overwriting existing PPID file %s for %s",
           pid_file,job_id);
  FILE *f = u8_fopen(pid_file,"w");
  if (f) {
    pid_t pid = getpid();
    u8_byte pidstring[64]; u8_sprintf(pidstring,64,"%lld",pid);
    fputs(pidstring,f);
    fclose(f);
    return;}
  else {
    u8_log(LOGWARN,"PPIDFile","Couldn't write PPID file %s",ppid_file);
    return;}
}

/* The launch/keep alive loop */

static void launch_loop(u8_string job_id,char **launch_args,int n_args)
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

    int exit_val = WEXITSTATUS(status);

    if (exit_val == 13) {
      u8_log(LOG_WARN,"LOOPEXIT",
	     "Exiting u8run because child returned status=13");
      exit(1);}
    else if (doexit) {
      /* If someone has set doexit, just terminate the child and exit */
      if (! exited) pid = kill_child(job_id,pid,pid_file);
      exit(0);}
    else if (u8_file_existsp(stop_file)) {
      /* If someone has set doexit, just terminate the child and exit */
      if (! exited) pid = kill_child(job_id,pid,pid_file);
      u8_removefile(stop_file);
      exit(0);}
    else if ( ( (exited) && (exit_val) && (restart_on_error == 0) ) ||
	      ( (exited) && (exit_val == 0) && (restart_on_exit == 0) ) ) {
      exit(exit_val);}
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
	/* We might have been killed while paused */
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
      if (restart_wait>0) u8_sleep(restart_wait);
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
      if (restart_wait>0) u8_sleep(restart_wait);
      pid = dolaunch(launch_args);
      u8_log(LOG_NOTICE,"Restarted",
	     "Automatically restarted %s after exit, pid=%lld",
	     job_id,(long long)pid);}}
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
