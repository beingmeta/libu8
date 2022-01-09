/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 beingmeta, LLC
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
#include "libu8/u8status.h"
#include "libu8/libu8io.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

static u8_string run_dir = NULL;
static u8_string run_prefix = NULL;
static u8_string log_dir = NULL;
static u8_string log_file = NULL;
static u8_string err_file = NULL;
static u8_string job_id = NULL;
static u8_string job_prefix = NULL;
static u8_uid run_user  = -1;
static u8_gid run_group = -1;
static int umask_value = -1;

static u8_string pid_file = NULL;
static u8_string ppid_file = NULL;
static u8_string status_file = NULL;
static u8_string stop_file = NULL;
static u8_string done_file = NULL;
static u8_string cmd_file = NULL;
static pid_t dependent = -1;

static int just_exec = 0;
static int run_as_daemon = 0;
static int restart_on_error = 0;
static int restart_on_exit = 0;

void usage()
{
  fprintf(stderr,"u8run [+opt|env=val|@jobid]* exename [args..]\n");
  fprintf(stderr,"  [opt]\t+launch +service +chain +insane +restart\n");
  fprintf(stderr,"  [opt]\t+restart=onerr +restart=onexit\n");
  fprintf(stderr,"  [env]\tJOBID=jobid RUNDIR=dir RUNPREFIX=string\n");
  fprintf(stderr,"  [env]\tSTATUSFILE=filename STOPFILE=filename DONEFILE=filename\n");
  fprintf(stderr,"  [env]\tU8LOGFILE|LOGFILE=file U8ERRFILE|ERRFILE=file\n");
  fprintf(stderr,"  [env]\tLOGDIR=dir LOGLEVEL=n\n");
  fprintf(stderr,"  [env]\tRESTART=never|error|exit|always\n");
  fprintf(stderr,"  [env]\tWAIT=secs FASTFAIL=secs BACKOFF=secs\n");
  fprintf(stderr,"  [env]\tMAXWAIT=secs\n");
  fprintf(stderr,"  [env]\tRUNUSER=name|uid RUNGROUP=name|gid\n");
  fprintf(stderr,"  [env]\tUMASK=0ooo\n");
}

static u8_string procpath(u8_string job_id,u8_string suffix)
{
  if (strchr(job_id,'/'))
    return u8_string_append(job_id,".",suffix,NULL);
  else {
    u8_string name = (run_prefix) ?
      (u8_string_append(run_prefix,job_id,".",suffix,NULL)) :
      (u8_string_append(job_id,".",suffix,NULL));
    u8_string path = u8_mkpath(run_dir,name);
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

static u8_string xgetenv(u8_string primary,u8_string alternate,u8_string backup)
{
  u8_string result = u8_getenv(primary);
  if (result) return result;
  result = u8_getenv(alternate);
  if (result) return result;
  return u8_getenv(backup);
}

static long long xgetenv_int(u8_string primary,u8_string alternate,u8_string backup,long long dflt)
{
  u8_string value = u8_getenv(primary), end = NULL;
  if (value == NULL) value =  u8_getenv(alternate);
  if (value == NULL) value =  u8_getenv(backup);
  if (value == NULL) return dflt;
  else if ( (value == NULL) || (*value == '\0') ) {
    if (value) u8_free(value);
    return dflt;}
  long long intval = strtoll(value,(char **)&end,10);
  if (end == value) intval = dflt;
  u8_free(value);
  return intval;
}

double xgetenv_float(u8_string var,u8_string alternate,u8_string backup,double dflt)
{
  if (var == NULL) return u8_reterr("NullEnvVar","u8_getenv_float",NULL);
  u8_string value = u8_getenv(var), end=NULL;
  if (value == NULL) value =  u8_getenv(alternate);
  if (value == NULL) value = u8_getenv(backup);

  if ( (value == NULL) || (*value == '\0') ) {
    u8_free(value);
    return dflt;}
  double floval = strtod(value,(char **)&end);
  if ( end == value )
    floval = dflt;
  u8_free(value);
  return floval;
}

static int doexit=0, paused=0, restart=0;
static double last_launch = -1, fast_fail = 3, fail_start=-1;
static double restart_wait=0, max_wait=120, backoff=10;
static double pid_wait=3.0, status_wait=-1;

static void launch_loop(u8_string job_id,char **launch_args,int n_args);
static pid_t dolaunch(char **launch_args);
static void setup_signals(void);

static int same_filep(u8_string path1,u8_string path2)
{
  u8_string rpath1 = u8_realpath(path1,NULL);
  u8_string rpath2 = u8_realpath(path2,NULL);
  int same = (strcmp(rpath1,rpath2)==0);
  u8_free(rpath1); u8_free(rpath2);
  return same;
}

static int wait_for_file(u8_string filename,double secs)
{
  if (u8_file_existsp(filename)) return 1;
  double start = u8_elapsed_time();
  while ( (u8_elapsed_time()-start) < secs) {
    if (u8_file_existsp(filename)) return 1;
    else u8_sleep(0.25);}
  return 0;
}

int main(int argc,char *argv[])
{
  int i = 1, n_args = 0;
  char **launch_args = u8_alloc_n(argc,char *), *cmd=NULL;

  u8_log_show_date=1;
  u8_log_show_procinfo=1;
  u8_log_show_threadinfo=1;
  u8_log_show_elapsed=1;
  u8_log_show_appid=1;

  run_dir = xgetenv("U8_RUNDIR","U8RUNDIR","RUNDIR");
  run_prefix = xgetenv("U8_RUNPREFIX","U8RUNPREFIX","RUNDIR");

  if (argc<2) {
    usage();
    exit(1);}
  while (i<argc) {
    char *arg = argv[i];
    if (*arg == '@') {
      if (job_id) u8_free(job_id);
      job_id = u8_fromlibc(arg+1);
      i++;}
    else if (arg[0] == '+') {
      if (just_exec) {
        u8_log(LOG_CRIT,"InconsistentRunOption",
               "The restart option '%s' cannot be used when "
               "running without forking (+exec)",
               arg);
        usage();
        exit(1);}
      else if (strcasecmp(arg,"+service")==0) {
        run_as_daemon = 1;
        restart_on_exit = 1;
        restart_on_error = 1;}
      else if ( (strcasecmp(arg,"+launch")==0) ||
                (strcasecmp(arg,"+daemon")==0) )
        run_as_daemon = 1;
      else if (strcasecmp(arg,"+exec")==0) {
        run_as_daemon = 0;
        just_exec = 1;}
      else if ( (strcasecmp(arg,"+oneshot")==0) ||
                (strcasecmp(arg,"+noloop")==0) ) {
        run_as_daemon = 1;
        just_exec = 1;}
      else if ( (strcasecmp(arg,"+insane")==0) ||
                (strcasecmp(arg,"+restart=onerr")==0) )
        restart_on_error = 1;
      else if ( (strcasecmp(arg,"+chain")==0) ||
                (strcasecmp(arg,"+restart=onexit")==0) )
        restart_on_exit = 1;
      else if ( (strcasecmp(arg,"+restart")==0) ) {
        restart_on_exit = 1;
        restart_on_error = 1;}
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
      if (name_len == 0) {
        u8_log(LOG_CRIT,"InvalidEnvSpec",
               "Invalid Envspec '%s', should specify an environment variable",
               arg);
        usage();
        exit(1);}
      char varname[name_len+5];
      /* Write the U8 version */
      strcpy(varname,"U8_");
      strncat(varname,arg,name_len);
      varname[name_len+3]='\0';
      setenv(varname,eqpos+1,1);
      /* Write the same value to the varname without the U8_ prefix */
      strncpy(varname,arg,name_len);
      varname[name_len]='\0';
      setenv(varname,eqpos+1,1);

      /* Settings which will be used immediately to generate paths */
      if (strcasecmp(varname,"rundir")==0) {
        if (run_dir) u8_free(run_dir);
        run_dir = u8_strdup(eqpos+1);}
      if (strcasecmp(varname,"runprefix")==0) {
        if (run_prefix) u8_free(run_prefix);
        run_prefix = u8_strdup(eqpos+1);}
      if (strcasecmp(varname,"jobid")==0) {
        if (job_id) u8_free(job_id);
        job_id = u8_strdup(eqpos+1);}
      i++;}
    else {
      /* This is where the actual commands start */
      cmd = arg;
      n_args=argc-i;
      memcpy(&(launch_args[0]),&(argv[i]),sizeof(char *)*n_args);
      break;}}
  /* Terminate the launch args */
  launch_args[n_args]=NULL;

 got_args:
  /* Make sure we have a jobid */
  if (job_id == NULL) {
    u8_string path = u8_fromlibc(cmd);
    job_id = u8_basename(path,"*");
    u8_free(path);}

  if (run_dir == NULL) {
    run_dir = xgetenv("U8RUNDIR","U8_RUNDIR","RUNDIR");
    if (run_dir == NULL) {
      if ( (u8_directoryp("_")) && (u8_file_writablep("_")) )
        run_dir = u8_abspath("_",NULL);
      else if ( (u8_directoryp("_runs")) && (u8_file_writablep("_runs")) )
        run_dir = u8_abspath("_runs",NULL);
      else {
        run_dir = u8_getcwd();
        /* If the command is in the current directory (or on the
           search path), default the run_prefix to '_' to simplify
           references to the run files */
        if ( (run_prefix == NULL) &&
             ( ( (cmd[0] == '.') && (cmd[1] == '/') ) ||
               (strchr(cmd,'/') == NULL) ) )
          run_prefix="_";}}}

  if ( (! (u8_directoryp(run_dir)) ) ||
       (! (u8_file_writablep(run_dir)) ) ) {
    u8_log(LOGCRIT,"BadRun_Dir",
           "The designated run_dir %s was not a writable directory",
           run_dir);
    exit(1);}
  log_dir = xgetenv("U8LOGDIR","U8_LOGDIR","LOGDIR");
  if (!(log_dir)) log_dir = u8_strdup(run_dir);

  {/* Compute runbase (run_dir/job_id) */
    u8_string local_name = (run_prefix) ?
      (u8_string_append(run_prefix,job_id,NULL)) :
      (job_id);
    job_prefix = u8_mkpath(run_dir,local_name);
    setenv("RUNBASE",job_prefix,0);
    setenv("U8_RUNBASE",job_prefix,0);
    if (run_prefix) u8_free(local_name);}

  if (log_file == NULL) log_file = xgetenv("U8_LOGFILE","U8LOGFILE","LOGFILE");
  if (err_file == NULL) err_file=xgetenv("U8_ERRFILE","U8ERRFILE","ERRFILE");

  /* Handle log file configurations which just specify the suffix */
  if ( (log_file) && (log_file[0] == '.') && (isalnum(log_file[1])) ) {
    u8_string full_logfile = (run_prefix) ?
      (u8_string_append(run_prefix,job_id,log_file,NULL)) :
      (u8_string_append(job_id,log_file,NULL));
    u8_free(log_file);
    log_file=full_logfile;}

  if ( (err_file) && (err_file[0] == '.') && (isalnum(err_file[1])) ) {
    u8_string full_errfile = (run_prefix) ?
      (u8_string_append(run_prefix,job_id,err_file,NULL)) :
      (u8_string_append(job_id,err_file,NULL));
    u8_free(err_file);
    err_file=full_errfile;}

  u8_loglevel = xgetenv_int("U8_LOGLEVEL","U8LOGLEVEL","LOGLEVEL",5);
  fast_fail = xgetenv_float("U8_FASTFAIL","U8FASTFAIL","FASTFAIL",fast_fail);
  restart_wait = xgetenv_float("U8_RESTART_WAIT","U8RESTARTWAIT","RESTARTWAIT",1);
  backoff = xgetenv_float("U8_BACKOFF","U8BACKOFF","BACKOFF",backoff);
  max_wait = xgetenv_float("U8_MAXWAIT","U8MAXWAIT","MAXWAIT",max_wait);
  pid_wait = xgetenv_float("U8_PIDWAIT","U8PIDWAIT","PIDWAIT",pid_wait);
  status_wait = xgetenv_float("U8_WAIT","U8RUN_WAIT","STATUSWAIT",status_wait);

  pid_file = xgetenv("U8_PIDFILE","U8PIDFILE","PIDFILE");
  status_file = xgetenv("U8_STATUSFILE","U8STATUSFILE","STATUSFILE");
  ppid_file = xgetenv("U8_PPIDFILE","U8PPIDFILE","PPIDFILE");
  stop_file = xgetenv("U8_STOPFILE","U8STOPFILE","STOPFILE");
  done_file = xgetenv("U8_DONEFILE","U8DONEFILE","DONEFILE");
  cmd_file = xgetenv("U8_CMDFILE","U8CMDFILE","CMDFILE");

 get_defaults:
  /* Default values */
  if (pid_file == NULL) pid_file = procpath(job_id,"pid");
  if (ppid_file == NULL) ppid_file = procpath(job_id,"ppid");
  if (cmd_file == NULL) cmd_file = procpath(job_id,"cmd");
  if (status_file == NULL) status_file = procpath(job_id,"status");
  if (stop_file == NULL) stop_file = procpath(job_id,"stop");
  if (done_file == NULL) done_file = procpath(job_id,"done");

  if (log_file == NULL) {
    if (run_prefix)
      log_file=u8_mkstring("%s%s.log",run_prefix,job_id);}
    else log_file=u8_mkstring("%s.log",job_id);

 setup_env:
  /* Setenv for this process */
  u8_setenv("U8_JOBID",job_id,1);
  u8_byte loglevel_tmp[64];
  u8_setenv("U8_LOGLEVEL",u8_bprintf(loglevel_tmp,"%d",u8_loglevel),1);
  if (stop_file) u8_setenv("U8_STOPFILE",stop_file,1);
  if (done_file) u8_setenv("U8_DONEFILE",done_file,1);
  if (status_file) u8_setenv("U8_STATUSFILE",status_file,1);
  if (run_dir)   u8_setenv("U8_RUNDIR",run_dir,1);
  if (log_dir)   u8_setenv("U8_LOGDIR",log_dir,1);
  if (log_file) {
    u8_setenv("U8_LOGFILE",log_file,1);
    u8_string log_path = u8_abspath(log_file,log_dir);
    u8_setenv("U8_LOGPATH",log_path,1);
    u8_free(log_path);}
  if (cmd_file)  u8_setenv("U8_CMDFILE",cmd_file,1);

  u8_string restart_val = xgetenv("U8_RESTART","U8RESTART","RESTART");
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

  int log_fd = -1;

 setup_stdout:
  /* Now that we're past the usage checks, redirect output if
     requested. */
  if (log_file == NULL) {
    /* Just use existing stdout and stderr */}
  else if ( (log_file[0] == '\0') ||
            ( ( (log_file[0] == '.') || (log_file[0] == '-') ) &&
              (log_file[1] == '\0') ) ) {
    /* Just use current stdout/err */
    u8_free(log_file);
    log_file = NULL;}
  else if (log_file) {
    if ( (log_file[0]) != '/' ) {
      u8_string abspath = (log_dir) ? (u8_mkpath(log_dir,log_file)) :
        (u8_abspath(log_file,NULL));
      u8_free(log_file);
      log_file = abspath;}
    log_fd = open(log_file,O_WRONLY|O_APPEND|O_CREAT,0664);
    if (log_fd<0) {
      int eno = errno; errno=0;
      fprintf(stderr,"Error opening log_file %s: %s (%d)\n",
              log_file,u8_strerror(eno),eno);
      exit(1);}
    else {
      int rv = dup2(log_fd,STDOUT_FILENO);
      if (rv<0) {
        int eno = errno; errno=0;
        fprintf(stderr,"Error redirecting stdout to %s: %s (%d)\n",
                log_file,u8_strerror(eno),eno);
        close(log_fd);
        exit(1);}}}
  if ( (err_file == NULL) && (log_fd>=0) ) {
    int rv = dup2(log_fd,STDERR_FILENO);
    if (rv<0) {
      int eno = errno; errno=0;
      fprintf(stderr,"Error redirecting stderr to %s: %s (%d)\n",
              log_file,u8_strerror(eno),eno);}}
  else if (err_file == NULL) {
    /* Leave stderr alone */ }
  else if ( (strcmp(err_file,".")==0) || (strcmp(err_file,"-")==0) ) {
    u8_free(err_file);
    err_file = NULL;}
  else {
    if ( (err_file[0]) != '/' ) {
      u8_string abspath = (log_dir) ? (u8_mkpath(log_dir,err_file)) :
        (u8_abspath(err_file,NULL));
      u8_free(err_file);
      err_file = abspath;}
    if ( (log_file) && (strcmp(err_file,log_file)==0) ) {
      int rv = dup2(log_fd,STDERR_FILENO);
      if (rv<0) {
        int eno = errno; errno=0;
        fprintf(stderr,"Error redirecting stderr to %s: %s (%d)\n",
                log_file,u8_strerror(eno),eno);
        close(log_fd);
        exit(1);}}
    else {
      int err_fd = open(err_file,O_WRONLY|O_APPEND|O_CREAT,0664);
      if (err_fd<0) {
        int eno = errno; errno=0;
        fprintf(stderr,"Error opening err_file %s: %s (%d)\n",
                err_file,u8_strerror(eno),eno);
        exit(1);}
      int rv = dup2(err_fd,STDERR_FILENO);
      if (rv<0) {
        int eno = errno; errno=0;
        fprintf(stderr,"Error redirecting stderr to %s: %s (%d)\n",
                err_file,u8_strerror(eno),eno);
        close(err_fd);
        if (log_fd>0) close(log_fd);
        exit(1);}}}

 check_files:
  if (u8_file_existsp(done_file)) {
    u8_log(LOG_WARN,"DoneFile","Exiting for done file %s",done_file);
    exit(0);}
  if (u8_file_existsp(stop_file)) {
    u8_log(LOG_WARN,"StopFile","Removing existing stop file %s",stop_file);
    int rv = u8_removefile(stop_file);
    if (rv<0) {
      u8_log(LOG_CRIT,"StopFile","Couldn't remove existing stop file %s",
             stop_file);
      exit(1);}
    exit(0);}

  if (u8_file_existsp(ppid_file)) {
    pid_t live = read_pid(ppid_file);
    if (live_pidp(live)) {
      if (u8_getenv("FORCE"))
        kill_existing(ppid_file,live,u8_getenv_float("KILLWAIT",15));
      else {
        u8_log(LOGCRIT,"ExistingPPID",
               "The file %s already specifies is a live parent PID (%lld)",
               ppid_file,live);
        exit(1);}}
    else u8_removefile(ppid_file);}

  if (u8_file_existsp(pid_file)) {
    pid_t live = read_pid(pid_file);
    if (live_pidp(live)) {
      if (u8_getenv("FORCE")) {
        kill_existing(pid_file,live,u8_getenv_float("KILLWAIT",15));
        u8_removefile(status_file);
        u8_removefile(pid_file);}
      else {
        u8_log(LOGCRIT,"ExistingPID",
               "The file %s already specifies is a live PID (%lld)",
               ppid_file,live);
        exit(1);}}
    else {
      u8_removefile(status_file);
      u8_removefile(pid_file);}}

  if (log_file) {
    u8_string run_log_file = procpath(job_id,"log");
    if ( ! ( (u8_file_existsp(run_log_file)) &&
             (same_filep(log_file,run_log_file)) ) ) {
      int rv = symlink(log_file,run_log_file);
      if (rv) {
        int saved_errno = errno; errno=0;
        u8_log(LOGWARN,"u8run",
               "Error (%s) linking log for job %s, '%s' ==>' %s'",
               u8_strerror(saved_errno),job_id,log_file,run_log_file);}}}
  if (err_file) {
    u8_string run_err_file = procpath(job_id,"err");
    if ( ! ( (u8_file_existsp(run_err_file)) &&
             (same_filep(err_file,run_err_file)) ) ) {
      int rv = symlink(err_file,run_err_file);
      if (rv) {
        int saved_errno = errno; errno=0;
        u8_log(LOGWARN,"u8run",
               "Error (%s) linking errlog for job %s, '%s' ==>' %s'",
               u8_strerror(saved_errno),job_id,err_file,run_err_file);}}}

  int restartable = (restart_on_error) || (restart_on_exit);

 start_run:
  if (run_as_daemon) {
    pid_t launched = fork();
    if (launched<0) {
      u8_log(LOG_CRIT,"ForkFailed",
             "Couldn't fork to launch %s",job_id);
      exit(1);}
    else if (launched) {
      /* When launched, we wait for a PID file to exist before we exit */
      int rv = (restartable) ? (wait_for_file(ppid_file,pid_wait)) :
        (wait_for_file(pid_file,pid_wait));
      if (rv) {
        u8_string launch_noun = (restartable) ? ("launch loop") : ("subproc");
        u8_log(LOG_NOTICE,"u8run/launch",
               "Forked %s (%lld) for %s (%s)%s%s",
               launch_noun,(long long)launched,job_id,cmd,
               ((status_wait>0) ? (", waiting for ") : ("")),
               ((status_wait>0) ? (status_file) : (U8S(""))));}
      else  {
        /* Does this need to clean anything up? */
        u8_log(LOGCRIT,"AppLaunchFailed",
               "Timed out waiting for STAT file %s for job %s",
               status_file,job_id);
        kill_child(job_id,launched,pid_file);
        exit(1);}
      if (restartable) {
        /* Wait for a grandchild */
        rv = wait_for_file(pid_file,pid_wait);
        if (rv==0) {
          /* Does this need to clean anything up? */
          u8_log(LOGCRIT,"AppLaunchFailed",
                 "Timed out waiting for granchild pid file (%s) for job %s",
                 pid_file,job_id);
          kill_child(job_id,launched,pid_file);
          exit(1);}
        else if (status_wait>0) {
          rv = wait_for_file(pid_file,status_wait);
          if (rv) {
            u8_string status = u8_filestring(status_file,NULL);
            u8_log(LOG_NOTICE,"u8run/status","For %s (%s)\n\t%s",job_id,cmd,status);}
          else {
            /* Does this need to clean anything up? */
            u8_log(LOGCRIT,"AppLaunchFailed",
                   "Timed out waiting for status file %s for job %s",
                   status_file,job_id);
            kill_child(job_id,launched,pid_file);
            exit(1);}}
        else exit(0);}
      else exit(0);}}
  setup_signals();
  if (restartable) {
    launch_loop(job_id,launch_args,n_args);
    return 0;}
  else  {
    pid_t pid = dolaunch(launch_args);
    /* Never reached */
    return (pid>0);}
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

void write_cmd_file(char **launch_args)
{
  if (cmd_file) {
    FILE *out = fopen(cmd_file,"w");
    char **scan = launch_args;
    while (*scan) {
      char *arg = *scan;
      fputs(arg,out); fputc(' ',out);
      scan++;}
    fputc('\n',out);
    fclose(out);}
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
  last_launch = u8_elapsed_time();
  write_cmd_file(launch_args);
  pid_t pid = (just_exec) ? (0) : (fork());
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
    write_pid_file(job_id);
    if (pid_file) setenv("U8RUNPIDFILE",pid_file,1);
    if (job_id) setenv("U8RUNJOBID",job_id,1);
    if (job_prefix) setenv("U8RUNJOBPREFIX",job_id,1);
    if (status_file) setenv("U8RUNSTATUSFILE",status_file,1);
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
  log_signal(signum,info,stuff);
  paused = 0;
  restart = 1;
}

static void siginfo_exit(int signum,siginfo_t *info,void *stuff)
{
  log_signal(signum,info,stuff);
  paused = 0;
  doexit = 1;
}

static void siginfo_hup(int signum,siginfo_t *info,void *stuff)
{
  log_signal(signum,info,stuff);
  paused = 0;
  restart = 1;
}

static void siginfo_pause(int signum,siginfo_t *info,void *stuff)
{
  log_signal(signum,info,stuff);
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
    if (rv<0) u8_log(LOGERR,"FileRemoveFailed","For %s",filename);
    ppid_file=NULL;
    u8_free(filename);}
  if (pid_file) {
    u8_string filename = pid_file;
    if (u8_file_existsp(filename)) {
      int rv = u8_removefile(filename);
      if (rv<0) u8_log(LOGERR,"FileRemoveFailed","For %s",filename);}
    pid_file=NULL;
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

  while (pid>0) {
    int status = 0;
    int wait_rv = waitpid(pid,&status,0);
    int exited = (wait_rv == pid) && (kill(pid,0) < 0);
    double now = u8_elapsed_time();

    u8_log(LOGWARN,"u8run/INT",
	   "Job %s:%lld signalled with rv=%d, do_exit=%d, paused=%d, "
	   "exited=%d, restart=%d, started=%f now=%f",
	   job_id,pid,wait_rv,doexit,paused,exited,restart,started,now);

    if (exited)
      u8_log(LOGWARN,"u8run","Job %s:%lld exited %s with status %d",
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
    else if (u8_file_existsp(done_file)) {
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
