dnl ---------------------------------------------------------------------------
dnl Macro: AC_CHECK_NUODB
dnl Check for custom NuoDB paths in --with-nuodb-* options.
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_CHECK_NUODB],[

# Check for custom includes path
if test [ -z "$ac_cv_nuodb_includes" ] 
then 
    AC_ARG_WITH([nuodb-includes], 
                AC_HELP_STRING([--with-nuodb-includes], [path to NuoDB header files]),
                [ac_cv_nuodb_includes=$withval])
fi
if test [ -n "$ac_cv_nuodb_includes" ]
then
    AC_CACHE_CHECK([NuoDB includes], [ac_cv_nuodb_includes], [ac_cv_nuodb_includes=""])
    NUODB_CFLAGS="-I$ac_cv_nuodb_includes"
fi

# Check for custom library path

if test [ -z "$ac_cv_nuodb_libs" ]
then
    AC_ARG_WITH([nuodb-libs], 
                AC_HELP_STRING([--with-nuodb-libs], [path to NuoSQL libraries]),
                [ac_cv_nuodb_libs=$withval])
    NUODB_LIBS="-L$ac_cv_nuodb_libs -lNuoRemote"
fi

if test [ -z "$ac_cv_nuodb_includes" -o -z "$ac_cv_nuodb_libs" ]
then
    AC_MSG_ERROR([
********************************************************************************
ERROR: cannot find NuoDB libraries. If you want to compile with NuoDB support,
       you must either specify file locations explicitly using 
       --with-nuodb-includes and --with-nuodb-libs options. If you want to 
       disable NuoDB support, use --without-nuodb option.
********************************************************************************
   ])
fi
])

