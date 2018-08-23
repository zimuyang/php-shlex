PHP_ARG_ENABLE(shlex, whether to enable shlex support,
[  --enable-shlex          Enable shlex support])

if test "$PHP_SHLEX" != "no"; then

    source_file="shlex.c"

    PHP_NEW_EXTENSION(shlex, $source_file, $ext_shared)
    PHP_ADD_INCLUDE([$ext_srcdir])
    AC_DEFINE(HAVE_SHLEX, 1, [ ])
fi