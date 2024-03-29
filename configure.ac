AC_INIT
AC_CONFIG_SRCDIR([include/libu8/libu8.h])
AC_PREFIX_PROGRAM(autoconf)
#m4_include([etc/ax_check_link_flags.m4])
m4_include([etc/ax_pthread.m4])
m4_include([etc/ax_sign.m4])
threads_default="yes"
ifmacosx="#"
shared_default="yes"
shared_suffix="so"
SHARED_LIB="libu8.so libu8stdio.so libu8fns.so libu8io.so libu8data.so"
rpath_default=$exec_prefix
CLEAN="rm -f"
U8VERSION=$(scripts/u8_gitversion etc/base_version)
U8MAJOR=$(echo ${U8VERSION} | cut -d'.' -f1)
U8MINOR=$(echo ${U8VERSION} | cut -d'.' -f2)
U8PATCHLEVEL=$(echo ${U8VERSION} | cut -d'.' -f3)
mshared_suffix=so.${U8MAJOR}
mmshared_suffix=so.${U8MAJOR}.${U8MINOR}
vshared_suffix=so.${U8VERSION}
RPMVERSION=${U8PATCHLEVEL}
DEBVERSION=${U8PATCHLEVEL}
LD="ld"
MKSTATIC="\$(LD) -r "
EXEFLAGS=
EXELIBS=
RPATHFLAGS="-Wl,'-rpath \$(RPATH)'"
LIBSOURCE=lib
EXESOURCE=exe
I18N=""
INSTALLI18N=""
SUDO=""
GITDESCRIBE=""
GITBRANCH=""
TAGS_TARGET="TAGS"
DOCTARGETS=""
DOCINSTALL=""
BUILDRPM="lsbrpmbuild"
RPMDIR=${RPMDIR:-dist}
RPMFLAGS=${RPMFLAGS:-}
GPG=`which gpg2 || which gpg || gpg`
GPGID=${GPGID:-FE1BC737F9F323D732AA26330620266BE5AFF294}
PKGCONFIG_DIR=
doi18n="no"
debugcflags=" -O0 -g3 -ggdb3 -fno-inline -fno-omit-frame-pointer "
build_shared="shared-libs"
build_static="static-libs"
shared_exe="shared-exe"
static_exe=""
test_shared="test-shared"
test_static="test-static"
install_shared="install-so"
install_static="install-static"
install_exe="install-exe"
libc_pthreads="no"
libc_dlopen="no"
suffix=""
rpath=""
have_constructor_attributes="yes"
archflags=""
definefileinfo=""
versionsh=""
doxygen=$(which doxygen)
ronn=$(which ronn || which ronn-2.7)
default_buildopts_dir="\$prefix/etc/beingmeta/opts"

if test -z "${U8PATCHLEVEL}"; then
   U8PATCHLEVEL=$(./scripts/u8_gitpatchcount ./etc/base_version)
fi;

if which lsb_release 2>&1 > /dev/null; then
   CODENAME=`lsb_release -cs || echo beingmeta`
fi

if (test "$rpath_default" = "NONE") || (test "$rpath_default" = ""); then
   rpath_default=$prefix;
fi

CFLAGS="-Wimplicit -fno-strict-aliasing -g $CFLAGS"

AC_CONFIG_AUX_DIR(etc)
AC_CONFIG_HEADERS([include/libu8/u8source.h include/libu8/config.h])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

DOCINSTALL="${DOCINSTALL} install-core-manpages install-dev-manpages";

case $target_os in
  linux*)
   MKSTATIC="${MKSTATIC} --build-id";
   AC_DEFINE(U8_RUSAGE_PROCFS_PATCH,1)
   AC_DEFINE(U8_LINUX_TARGET,1)
   AC_DEFINE(U8_UNIX_TARGET,1)
   ;;
  darwin*)
#   MKSTATIC="/usr/bin/libtool -static ";
#   if test "x$CC" = "xgcc"; then LIBS="$LIBS -lgcc"; fi
   shared_suffix="dylib";
   install_shared="install-dylib";
   ifmacosx="";
   RPATHFLAGS=" -rpath \$(RPATH) "
   CFLAGS="-D_DARWIN_C_SOURCE $CFLAGS"
   AC_DEFINE(DESTRUCTIVE_POLL,1)
   AC_DEFINE(CHECK_DANGLING_ERRNOS,1)
   AC_DEFINE(U8_DARWIN_TARGET,1)
   AC_DEFINE(U8_BSD_TARGET,1)
   AC_DEFINE(U8_UNIX_TARGET,1)
   ;;
esac  

AC_DEFINE_UNQUOTED(U8_HOST_CPU,"$host_cpu")
AC_DEFINE_UNQUOTED(U8_HOST_VENDOR,"$host_vendor")
AC_DEFINE_UNQUOTED(U8_HOST_OS,"$host_os")

AC_DEFINE_UNQUOTED(U8_TARGET_CPU,"$target_cpu")
AC_DEFINE_UNQUOTED(U8_TARGET_VENDOR,"$target_vendor")
AC_DEFINE_UNQUOTED(U8_TARGET_OS,"$target_os")

if test $prefix == "/usr"; then
   default_config_dir=/etc/kno;
   default_buildopts_dir=/etc/beingmeta/opts;
else
   default_config_dir=$prefix/etc/kno;
   default_buildopts_dir=$prefix/etc/beingmeta/opts;
fi

AC_SUBST(LD)
AC_SUBST(CLEAN)
AC_SUBST(TAGS_TARGET)
AC_SUBST(SHARED_LIB)
AC_SUBST(SHARED_LIBV)
AC_SUBST(U8MAJOR)
AC_SUBST(U8MINOR)
AC_SUBST(U8PATCHLEVEL)
AC_SUBST(U8VERSION)
AC_SUBST(RPMVERSION)
AC_SUBST(DEBVERSION)
AC_SUBST(PKGCONFIG_DIR)
AC_SUBST(MKSTATIC)
AC_SUBST(SUDO)
AC_SUBST(GZIP)
AC_SUBST(EXEFLAGS)
AC_SUBST(EXELIBS)
AC_SUBST(debugcflags)
AC_SUBST(shared_suffix)
AC_SUBST(vshared_suffix)
AC_SUBST(mshared_suffix)
AC_SUBST(mmshared_suffix)
AC_SUBST(suffix)
AC_SUBST(test_shared)
AC_SUBST(test_static)
AC_SUBST(build_shared)
AC_SUBST(build_static)
AC_SUBST(shared_exe)
AC_SUBST(static_exe)
AC_SUBST(install_shared)
AC_SUBST(install_static)
AC_SUBST(buildopts_dir)
AC_SUBST(archflags)
AC_SUBST(definefileinfo)
AC_SUBST(versionsh)
AC_SUBST(I18N)
AC_SUBST(INSTALLI18N)
AC_SUBST(GITDESCRIBE)
AC_SUBST(GITBRANCH)
AC_SUBST(doxygen)
AC_SUBST(ronn)
AC_SUBST(DOCTARGETS)
AC_SUBST(DOCINSTALL)

AC_SUBST(rpath)
AC_SUBST(RPATHFLAGS)

AC_SUBST(LIBSOURCE)
AC_SUBST(EXESOURCE)

AC_ARG_VAR(GITBRANCH,"Git branch for build")
AC_ARG_VAR(RPMDIR,"Location for writing built RPMs")
AC_ARG_VAR(RPMFLAGS,"Default flags for building RPMs")
AC_ARG_VAR(GPG,"GPG executable")
AC_ARG_VAR(GPGID,"Default GnuPG id for signing packages")
AC_SUBST(RPMDIR)
AC_SUBST(RPMFLAGS)
AC_SUBST(GPG)
AC_SUBST(GPGID)
AC_SUBST(GITBRANCH)
AC_SUBST(REPONAME)
AC_SUBST(CODENAME)

if test -z "${GITBRANCH}"; then
   GITBRANCH=`scripts/u8_gitbranch`;
fi;
if test -z "${GITBRANCH}" || test "${GITBRANCH}" = "main"; then
   REPONAME="kno";
else
   REPONAME="kno_${GITBRANCH}";
fi;

AC_PATH_PROG(CLANG,clang,none)

AC_ARG_WITH(sanitize,--with-sanitize[=address*|memory|thread],with_sanitize=$withval,with_sanitize="none")

if test "x$with_sanitize" = "xnone"; then
   echo "Configuring normal (no sanitization) build";
elif test "x$with_sanitize" = "xaddress" ||
     test "x$with_sanitize" = "x" ||
     test "x$with_sanitize" = "xyes"; then
   CFLAGS="-fsanitize=address -fno-omit-frame-pointer -O0 -g $CFLAGS"
   LDFLAGS="-g -fsanitize=address $LDFLAGS"
   build_static=""
   test_static=""
   install_static=""
   custom_malloc="asan"
   memtest_prefix=builtin_
   heaptest_prefix=builtin_
elif test "x$with_sanitize" = "xleak"; then
   CFLAGS="-fsanitize=address -fno-omit-frame-pointer -O0 -g $CFLAGS"
   LDFLAGS="-g -fsanitize=address $LDFLAGS"
   build_static=""
   test_static=""
   install_static=""
   custom_malloc="asan"
   memtest_prefix=builtin_
   heaptest_prefix=builtin_
elif test "x$CLANG" = "xnone"; then
  echo "No clang for sanitizing $with_sanitize";
  exit;
elif test "x$with_sanitize" = "xmemory"; then
   CC=$CLANG
   CFLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fPIE -g -fno-omit-frame-pointer $CFLAGS"
   LD=$CLANG
   LDFLAGS="-g -fsanitize=memory -fsanitize-memory-track-origins -pie -fno-omit-frame-pointer $LDFLAGS"
   build_static=""
   test_static=""
   install_static=""
   custom_malloc="msan-full"
elif test "x$with_sanitize" = "xthread"; then
   CC=$CLANG
   CFLAGS="-fsanitize=thread -g -fno-omit-frame-pointer $CFLAGS"
   LD=$CLANG
   LDFLAGS="-g -fsanitize=thread -fno-omit-frame-pointer $LDFLAGS"
   build_static=""
   test_static=""
   install_static=""
   custom_malloc="msan"
fi

AC_DEFINE_UNQUOTED(U8_SANITIZE,"$with_sanitize")

AC_PROG_CC([${CC} cc gcc])
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_USE_SYSTEM_EXTENSIONS

AC_DEFINE_UNQUOTED(U8_VERSION,"$U8VERSION")
AC_DEFINE_UNQUOTED(U8_MAJOR_VERSION,$U8MAJOR)
AC_DEFINE_UNQUOTED(U8_MINOR_VERSION,$U8MINOR)

if test "x$prefix" != "x/usr"; then
  CFLAGS="$CFLAGS -I$prefix/include"
  CPPFLAGS="$CPPFLAGS -I$prefix/include"
  LDFLAGS="$LDFLAGS -L$prefix/lib"
fi

AC_ARG_WITH(reponame,--with-reponame for distribution,with_reponame=$withval,with_reponame=$REPONAME)
REPONAME=$with_reponame;

AC_ARG_WITH(codename,--with-codename for distribution,with_codename=$withval,with_codename=$CODENAME)
CODENAME=$with_codename

AC_ARG_WITH(buildopts,--with-buildopts-dir location used by getbuildopt,,with_buildopts_dir=$default_buildopts_dir)
buildopts_dir=$with_buildopts_dir

AC_ARG_WITH(optlib,--with-optlib Use optional library directory,,with_optlib="none")
if test $with_optlib != "none"; then
  LDFLAGS="$LDFLAGS -L$with_optlib"
fi

AC_ARG_WITH(optdir,--with-optdir Use optional header/lib,,with_optdir="/opt/local")
if test -d $with_optdir/include; then
  CPPFLAGS="$CPPFLAGS -I$with_optdir/include"
  CFLAGS="$CFLAGS -I$with_optdir/include"
  APXSFLAGS="$APXSFLAGS -I$with_optdir/include"
fi
if test -d $with_optdir/lib; then
  LDFLAGS="$LDFLAGS -L$with_optdir/lib"
fi
if test -d $with_optdir/bin; then
  PATH="$PATH:$with_optdir/bin"
fi

AC_ARG_WITH(rpath,--with-rpath dir,rpath=$withval,rpath=$rpath_default)

if test "$rpath" != "none" && test "$rpath" != "NONE" && test ! -z "$rpath" &&
   test "$rpath" != "/usr" && test "$rpath" != "/usr/local"; then
  EXEFLAGS=" $EXEFLAGS \$(RPATHFLAGS) "
fi

AC_ARG_WITH(pkgconfig_dir,--with-pkgconfig-dir pathname,,with_pkgconfig_dir=$prefix/share/pkgconfig)
AC_DEFINE_UNQUOTED(U8_PKGCONFIG_DIR,"$with_pkgconfig_dir")
PKGCONFIG_DIR=${with_pkgconfig_dir}

AC_ARG_WITH(encodings_dir,--with-encodings-dir pathname,,with_encodings_dir=$prefix/share/libu8/encodings)
AC_DEFINE_UNQUOTED(U8_ENCODINGS_DIR,"$with_encodings_dir")

dnl For specifying default -arch flags for OSX
AC_ARG_WITH(arch,--with[out]-arch build with particular --arch flags,,with_arch="")
if test -z $with_arch; then
  archflags=""
elif test $with_arch = "all"; then
  archflags="-arch i386 -arch x86_64 -arch ppc -arch ppc64";
elif test $with_arch = "all86"; then
  archflags="-arch i386 -arch x86_64";
elif test $with_arch = "allppc"; then
  archflags="-arch ppc -arch ppc64";
else archflags=$with_arch
fi

AC_ARG_ENABLE(maintainer-mode,--en[dis]able-maintainer-mode doesn't do anything)
AC_ARG_ENABLE(dependency-tracking,--en[dis]able-dependency-tracking doesn't do anything)

AC_ARG_ENABLE(stripping,--enable-stripping strip libraries and executables before installing,,enable_stripping="no")
if test "$enable_stripping" != "no"; then
   LIBSOURCE=lib/stripped
   EXESOURCE=exe/stripped
fi

AC_ARG_ENABLE(shared,--en[dis]able-shared Build with/without shared libraries,,enable_shared=$shared_default)
if test $target = "i686-pc-mingw32"; then
  CFLAGS="-DU8_DLL=1 $CFLAGS"
elif test $enable_shared = "yes"; then
  CFLAGS="-fPIC $CFLAGS"
else
  SHARED_LIB=
fi

AC_CHECK_PROGS(git,"git","na")
AC_CHECK_PROGS(etags,"etags","na")
AC_CHECK_PROGS(buildrpm,"lsb-rpmbuild","rpmbuild","lsb-rpm","rpm","na")
AC_CHECK_PROGS(rpm,"lsb-rpm","rpm","na")
AC_CHECK_PROG(CREATEREPO,createrepo,,echo No createrepo for )

AC_CHECK_PROGS(GZIP,"gzip","na")
AC_CHECK_PROGS(ZIP,"zip","zip")

if test $etags = "na"; then
   TAGS_TARGET="NOTAGS"
fi

if test ! -z "$doxygen"; then
   DOCTARGETS="${DOCTARGETS} docs/doxygen.done"
fi
if test ! -z "$ronn"; then
   DOCTARGETS="${DOCTARGETS} docs/ronn.done"
fi

if test $target = "i686-pc-mingw32"; then
   shared_suffix="dll"
   install_shared="install-dll"
   LIBS="$LIBS -lwsock32"
fi

AC_DEFINE_UNQUOTED(U8_SHARED_SUFFIX,"${shared_suffix}")

AC_PATH_PROG(XGETTEXT,xgettext)
AC_PATH_PROG(MSGMERGE,msgmerge)
AC_PATH_PROG(MSGFMT,msgfmt)

ALPINE_CRYPTO_LIB=libressl2.7-libcrypto
AC_SUBST(ALPINE_CRYPTO_LIB)
AC_CHECK_PROGS(APK,"apk","")
if test ! -z "${APK}"; then
   ALPINE_CRYPTO_LIB=$(${APK} search libressl | grep crypto | cut -d- -f1,2);
fi;

AC_DEFINE(FD_WORDS_ARE_ALIGNED,1)
AC_TYPE_UID_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_PID_T
AC_TYPE_MODE_T
AC_C_INLINE
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(off_t)

AC_CHECK_SIZEOF(void *)
TRY_LIBNAME=

# Check whether to use an architecture specific libdir
if test "x${libdir}" != "x\${exec_prefix}/lib"; then
  echo using explicit libdir ${libdir};
else case ${target_cpu} in
     *i386*)
	TRY_LIBNAME=lib32;
	;;
     *64*)
	TRY_LIBNAME=lib64;
	;;
     esac;	
  if test -n "${TRY_LIBNAME}"; then
    :;
  elif test ${ac_cv_sizeof_void_p} -eq 8; then
    TRY_LIBNAME=lib64;
  elif test ${ac_cv_sizeof_void_p} -eq 8; then
    TRY_LIBNAME=lib32;
  else
    TRY_LIBNAME=lib;
  fi;
  if test -z "${TRY_LIBNAME}"; then
    echo using default libdir ${libdir};
  elif test -d /usr/${TRY_LIBNAME} && test -f /usr/${TRY_LIBNAME}/libc.so; then
    libdir=\${exec_prefix}/${TRY_LIBNAME};
    echo using libdir ${libdir};
  else
    echo using default libdir ${libdir};
  fi;	 
fi

AC_HEADER_STAT

AC_CHECK_HEADERS(strings.h netdb.h sys/types.h sys/mman.h)
AC_CHECK_HEADERS(sys/stat.h unistd.h pwd.h grp.h fcntl.h poll.h sys/poll.h)
AC_CHECK_HEADERS(sys/socket.h sys/select.h sys/un.h netdb.h)
AC_CHECK_HEADERS(netinet/in.h netinet/tcp.h)
AC_CHECK_HEADERS(dirent.h sys/ndir.h sys/dir.h)
AC_CHECK_HEADERS(sys/timeb.h utime.h dlfcn.h malloc.h sys/malloc.h malloc/malloc.h)
AC_CHECK_HEADERS(sys/resource.h resource.h sys/syscall.h)
AC_CHECK_FUNCS(getenv setenv unsetenv execvpe)
AC_CHECK_FUNCS(getrusage mallinfo syscall mstats sysconf getpagesize)
AC_CHECK_FUNCS(strchr strrchr strstr strrrstr)
AC_CHECK_FUNCS(posix_fadvise posix_madvise posix_fallocate)

AC_CHECK_HEADERS(stdatomic.h)

AC_HEADER_STAT

if test "x$with_asan" = "xyes"; then
   AC_CHECK_LIB(asan,asan_init)
fi

# Time support
AC_CHECK_HEADERS(time.h sys/time.h sys/types.h)
AC_CHECK_HEADERS_ONCE([sys/time.h])

AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_EGREP_HEADER(tm_gmtoff,time.h,AC_DEFINE(HAVE_TM_GMTOFF,1),AC_DEFINE(HAVE_TM_GMTOFF,0))
AC_EGREP_HEADER(tm_isdst,time.h,AC_DEFINE(HAVE_TM_ISDST,1),AC_DEFINE(HAVE_TM_ISDST,0))

# pwd/grp types
signed_uid_t=1
signed_gid_t=1
AX_CHECK_SIGN([uid_t],
    [ signed_uid_t=1 ],
    [ signed_uid_t=0 ], [
  #ifdef HAVE_PWD_H
  #include <pwd.h>
  #endif
  ])
AX_CHECK_SIGN([gid_t],
    [ signed_gid_t=1 ],
    [ signed_gid_t=0 ], [
  #ifdef HAVE_GRP_H
  #include <grp.h>
  #endif
  ])

if test ${signed_uid_t} = 0; then
   AC_DEFINE(UID_T_UNSIGNED,1)
fi
if test ${signed_gid_t} = 0; then
   AC_DEFINE(GID_T_UNSIGNED,1)
fi

# Particular functions

AC_CHECK_FUNCS(isatty fstat)
AC_CHECK_FUNCS(clock_gettime)
AC_CHECK_FUNCS(strdup strerror_r)
AC_CHECK_FUNCS(access flock fcntl fseeko)
AC_CHECK_FUNCS(chown chmod setuid getuid geteuid utime)
AC_CHECK_FUNCS(getpwnam getpwnam_r getpwgid getpwgid_r)
AC_CHECK_FUNCS(setgid getgid getegid getgroups)
AC_CHECK_FUNCS(getgrnam getgrnam_r getgrgid getgrgid_r)
AC_CHECK_FUNCS(symlink realpath mkdtemp mkstemp)
AC_CHECK_FUNCS(ftime gettimeofday localtime_r gmtime_r getpid)
AC_CHECK_FUNCS(gethostbyname2_r)
AC_CHECK_FUNCS(getservbyname)
AC_CHECK_FUNCS(nanosleep)
AC_CHECK_FUNCS(mmap)
AC_FUNC_STRERROR_R

# Syslog
AC_CHECK_FUNCS(syslog,syslog_target="syslog_libs",syslog_target="")

# Internationalization
AC_CHECK_HEADERS(libintl.h)
AC_CHECK_LIB(intl,gettext)
AC_CHECK_FUNCS(gettext,[libc_gettext="yes"])
if test "x$libc_gettext" != "xyes"; then
   AC_CHECK_LIB(intl,gettext)
fi
AC_CHECK_FUNCS(gettext,[do_i18n="yes"])
if test "x$do_i18n" = "xyes"; then
   AC_CHECK_FUNCS(textdomain bindtextdomain bindtextdomain_codeset)
fi

AC_ARG_WITH(i18n,--with[out]-i18n built and install message catalogs,,with_i18n="yes")

if test "x$doi18n" = "xyes" && test "x$with_i18n" = "xyes"; then
  I18N="i18n"
  INSTALLI18N="install-i18n"
fi

AC_ARG_WITH(fileinfo,--with[out]-fileinfo record fileinfo,,with_fileinfo="yes")
if test "x$with_fileinfo" = "xyes" ; then
   definefileinfo="-D_FILEINFO=\"\\\"\`exe/u8_fileinfo ./$< .\`\\\"\""
   versionsh="include/framerd/versions.h"
fi

dnl Use sudo for installations in makefile
AC_ARG_WITH(sudo,--with[out]-sudo use sudo with install,,with_sudo="yes")
if test "x$with_sudo" == "xyes"; then
  AC_CHECK_PROGS(sudo_program,sudo,"na")
  if test "x$sudoprogram" == "xna"; then
    SUDO=""
  else 
    SUDO="unset LD_LIBRARY_PATH; unset DYLD_LIBRARY_PATH; $sudo_program "
  fi
elif test "x$with_sudo" != "xno"; then
  SUDO="$with_sudo "
fi

AC_ARG_WITH(tcmalloc,--with[out]-tcmalloc Use the Google malloc library,,with_tcmalloc="no")
if test $with_tcmalloc = "yes"; then
  AC_CHECK_LIB(tcmalloc,malloc)
fi

AC_CHECK_LIB(dl,dlopen)

AC_ARG_ENABLE(threads,--en/disable-threads Enable multi-threading,,enable_threads=$threads_default)
if test $enable_threads = "yes"; then
  AX_PTHREAD()
  LIBS="$PTHREAD_LIBS $LIBS"
  CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
  AC_DEFINE(U8_THREADS_ENABLED,1)
else
  AC_DEFINE(U8_THREADS_ENABLED,0)
fi

dnl ***********************************
dnl *** Checks for working __thread ***
dnl ***********************************
AC_MSG_CHECKING(for working __thread)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
            #include <pthread.h>
            __thread int i;
	    static int res1, res2;
	]], [[
	     void thread_main (void *arg)
	     {
		i = arg;
		sleep (1);
		if (arg == 1) res1 = (i == arg);
		else res2 = (i == arg);
	     }

	     int main () {
		pthread_t t1, t2;
 		i = 5;
		pthread_create (&t1, NULL, thread_main, 1);
		pthread_create (&t2, NULL, thread_main, 2);

		pthread_join (t1, NULL);
		pthread_join (t2, NULL);
		return !(res1 + res2 == 2);
		}
	    ]])],[
		tls_default=no
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_THREAD_STORAGE_CLASS)],[tls_default=yes
		AC_MSG_RESULT(no)])

dnl This forces the use of the threadlocal storage functions from the pthreads
dnl  library.  Otherwise, the executable will use __thread storage declarations
dnl  when available
AC_ARG_WITH(tls,--with[out]-tls Force use of threadlocal storage,,with_tls=$tls_default)
if test $with_tls = "yes"; then
  AC_DEFINE(U8_FORCE_TLS,1)
else
  AC_DEFINE(U8_FORCE_TLS,0)
fi

AC_CHECK_HEADERS(pthread.h)
AC_CHECK_FUNCS(pthread_rwlock_init pthread_self pthread_create)
AC_CHECK_FUNCS(pthread_mutexattr_init pthread_mutexattr_settype)
AC_CHECK_FUNCS(pthread_threadid_np)
AC_CHECK_FUNCS(pthread_get_stacksize_np pthread_get_stackaddr_np)
AC_CHECK_FUNCS(pthread_getattr_np pthread_attr_get_np)
AC_CHECK_FUNCS(pthread_attr_getstack pthread_attr_getstackaddr)
AC_CHECK_FUNCS(pthread_attr_getstacksize pthread_attr_setstacksize)

AC_ARG_ENABLE(malloc-debugging,--en[dis]able-malloc-debugging en[dis]ables libu8 malloc debugging support)
if test "malloc_debugging_enabled" = "yes"; then
  AC_DEFINE(U8_DEBUG_MALLOC,1)
fi



AC_ARG_ENABLE(largefiles,--en/disable-largefiles Enable use of large (> 2GB) file support,,largefiles_enabled=yes)
if test $largefiles_enabled = "yes"; then
  AC_DEFINE(U8_LARGEFILES_ENABLED,1)
else
  AC_DEFINE(U8_LARGEFILES_ENABLED,0)
fi

dnl ***********************************
dnl *** Checks for working constructor attributes ***
dnl ***********************************
AC_MSG_CHECKING(for working __constructor_attributes)
dnl ***********************************
dnl *** Checks for working constructor attributes ***
dnl ***********************************
AC_MSG_CHECKING(for working __constructor_attributes)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
             int tmpinit(void) __attribute__ ((constructor));

	    ]])],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_CONSTRUCTOR_ATTRIBUTES)],[AC_MSG_RESULT(no)])

dnl ***********************************
dnl *** Checks for working __builtin_expect ***
dnl ***********************************
AC_MSG_CHECKING(for working __builtin_expect)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
	     int main (int argc,char *argv[]) {
                if (__builtin_expect((argc>2),1)) return 0;
                else return 1;
		}
	    ]])],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_BUILTIN_EXPECT)],[AC_MSG_RESULT(no)])

# OS X crypto
AC_CHECK_HEADERS(CommonCrypto/CommonDigest.h CommonCrypto/CommonHMAC.h)
AC_CHECK_HEADERS(CommonCrypto/CommonCryptor.h)
AC_CHECK_FUNCS(CCHmacInit CCCryptorCreate)

# OpenSSL crypto
AC_CHECK_HEADERS(openssl/hmac.h)
AC_CHECK_HEADERS(openssl/md5.h)
AC_CHECK_HEADERS(openssl/sha.h)
AC_CHECK_HEADERS(openssl/err.h)
AC_CHECK_HEADERS(openssl/evp.h)
AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new)
AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init)
AC_CHECK_FUNCS(EVP_get_cipherbyname EVP_CIPHER_CTX_new EVP_CIPHER_CTX_init SHA1_Init)

# UUID functions
AC_CHECK_HEADERS(uuid/uuid.h)
AC_CHECK_LIB(uuid,uuid_generate_time)
AC_CHECK_FUNCS(uuid_generate_time)

AC_CONFIG_FILES([makefile docs/makefile etc/libu8.pc etc/getbuildopt
           dist/libu8-2.7.spec dist/libu8.spec dist/alpine/APKBUILD
	   scripts/u8_install_shared scripts/u8_gzinstall])
AC_CONFIG_COMMANDS([default],[chmod a+x etc/getbuildopt scripts/u8_install_shared scripts/u8_gzinstall],[])
AC_OUTPUT


