target_include_directories
--------------------------

Add include directories to a target.

::

  target_include_directories(<target> [SYSTEM] [BEFORE] <INTERFACE|PUBLIC|PRIVATE> [items1...]
    [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])

Specify include directories or targets to use when compiling a given
target.  The named <target> must have been created by a command such
as add_executable or add_library and must not be an IMPORTED target.

If BEFORE is specified, the content will be prepended to the property
instead of being appended.

The INTERFACE, PUBLIC and PRIVATE keywords are required to specify the
scope of the following arguments.  PRIVATE and PUBLIC items will
populate the INCLUDE_DIRECTORIES property of <target>.  PUBLIC and
INTERFACE items will populate the INTERFACE_INCLUDE_DIRECTORIES
property of <target>.  The following arguments specify include
directories.  Specified include directories may be absolute paths or
relative paths.  Repeated calls for the same <target> append items in
the order called.If SYSTEM is specified, the compiler will be told the
directories are meant as system include directories on some platforms
(signalling this setting might achieve effects such as the compiler
skipping warnings, or these fixed-install system files not being
considered in dependency calculations - see compiler docs).  If SYSTEM
is used together with PUBLIC or INTERFACE, the
INTERFACE_SYSTEM_INCLUDE_DIRECTORIES target property will be populated
with the specified directories.

Arguments to target_include_directories may use "generator
expressions" with the syntax "$<...>".  Generator expressions are
evaluated during build system generation to produce information
specific to each build configuration.  Valid expressions are:

::

  $<0:...>                  = empty string (ignores "...")
  $<1:...>                  = content of "..."
  $<CONFIG:cfg>             = '1' if config is "cfg", else '0'
  $<CONFIGURATION>          = configuration name
  $<BOOL:...>               = '1' if the '...' is true, else '0'
  $<STREQUAL:a,b>           = '1' if a is STREQUAL b, else '0'
  $<ANGLE-R>                = A literal '>'. Used to compare strings which contain a '>' for example.
  $<COMMA>                  = A literal ','. Used to compare strings which contain a ',' for example.
  $<SEMICOLON>              = A literal ';'. Used to prevent list expansion on an argument with ';'.
  $<JOIN:list,...>          = joins the list with the content of "..."
  $<TARGET_NAME:...>        = Marks ... as being the name of a target.  This is required if exporting targets to multiple dependent export sets.  The '...' must be a literal name of a target- it may not contain generator expressions.
  $<INSTALL_INTERFACE:...>  = content of "..." when the property is exported using install(EXPORT), and empty otherwise.
  $<BUILD_INTERFACE:...>    = content of "..." when the property is exported using export(), or when the target is used by another target in the same buildsystem. Expands to the empty string otherwise.
  $<PLATFORM_ID>            = The CMake-id of the platform   $<PLATFORM_ID:comp>       = '1' if the The CMake-id of the platform matches comp, otherwise '0'.
  $<C_COMPILER_ID>          = The CMake-id of the C compiler used.
  $<C_COMPILER_ID:comp>     = '1' if the CMake-id of the C compiler matches comp, otherwise '0'.
  $<CXX_COMPILER_ID>        = The CMake-id of the CXX compiler used.
  $<CXX_COMPILER_ID:comp>   = '1' if the CMake-id of the CXX compiler matches comp, otherwise '0'.
  $<VERSION_GREATER:v1,v2>  = '1' if v1 is a version greater than v2, else '0'.
  $<VERSION_LESS:v1,v2>     = '1' if v1 is a version less than v2, else '0'.
  $<VERSION_EQUAL:v1,v2>    = '1' if v1 is the same version as v2, else '0'.
  $<C_COMPILER_VERSION>     = The version of the C compiler used.
  $<C_COMPILER_VERSION:ver> = '1' if the version of the C compiler matches ver, otherwise '0'.
  $<CXX_COMPILER_VERSION>   = The version of the CXX compiler used.
  $<CXX_COMPILER_VERSION:ver> = '1' if the version of the CXX compiler matches ver, otherwise '0'.
  $<TARGET_FILE:tgt>        = main file (.exe, .so.1.2, .a)
  $<TARGET_LINKER_FILE:tgt> = file used to link (.a, .lib, .so)
  $<TARGET_SONAME_FILE:tgt> = file with soname (.so.3)

where "tgt" is the name of a target.  Target file expressions produce
a full path, but _DIR and _NAME versions can produce the directory and
file name components:

::

  $<TARGET_FILE_DIR:tgt>/$<TARGET_FILE_NAME:tgt>
  $<TARGET_LINKER_FILE_DIR:tgt>/$<TARGET_LINKER_FILE_NAME:tgt>
  $<TARGET_SONAME_FILE_DIR:tgt>/$<TARGET_SONAME_FILE_NAME:tgt>



::

  $<TARGET_PROPERTY:tgt,prop>   = The value of the property prop on the target tgt.

Note that tgt is not added as a dependency of the target this
expression is evaluated on.

::

  $<TARGET_POLICY:pol>          = '1' if the policy was NEW when the 'head' target was created, else '0'.  If the policy was not set, the warning message for the policy will be emitted.  This generator expression only works for a subset of policies.
  $<INSTALL_PREFIX>         = Content of the install prefix when the target is exported via INSTALL(EXPORT) and empty otherwise.

Boolean expressions:

::

  $<AND:?[,?]...>           = '1' if all '?' are '1', else '0'
  $<OR:?[,?]...>            = '0' if all '?' are '0', else '1'
  $<NOT:?>                  = '0' if '?' is '1', else '1'

where '?' is always either '0' or '1'.

Expressions with an implicit 'this' target:

::

  $<TARGET_PROPERTY:prop>   = The value of the property prop on the target on which the generator expression is evaluated.
