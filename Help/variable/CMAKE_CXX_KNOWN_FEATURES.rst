CMAKE_CXX_KNOWN_FEATURES
------------------------

List of C++ features known to this version of CMake.

The features listed in this variable may be known to be available to the
C++ compiler.  If the feature is available with the C++ compiler, it will
be listed in the :variable:`CMAKE_CXX_COMPILE_FEATURES` variable.

The features listed here may be used with the :command:`target_compile_features`
command.

The features known to this version of CMake are:

``cxx_delegating_constructors``
  Delegating constructors, as defined in N1986_.

.. _N1986: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1986.pdf

``cxx_variadic_templates``
  Variadic templates, as defined in N2242_.

.. _N2242: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2242.pdf

``cxx_constexpr``
  Constant expressions, as defined in N2235_.

.. _N2235: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2235.pdf

``cxx_static_assert``
  Static assert, as defined in N1720_.

.. _N1720: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2004/n1720.html

``cxx_final``
  Override control ``final`` keyword, as defined in N2928_.

.. _N2928: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2009/n2928.htm

``cxx_override``
  Override control ``override`` keyword, as defined in N2928_.

.. _N2928: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2009/n2928.htm

``cxx_auto_function``
  Automatic function return type, as defined in N2541_.

.. _N2541: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2541.htm

``cxx_auto_type``
  Automatic type deduction, as defined in N1984_.

.. _N1984: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1984.pdf

``cxx_strong_enums``
  Strongly typed enums, as defined in N2347_.

.. _N2347: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2347.pdf

``cxx_decltype``
  Decltype, as defined in N3276_.

.. _N3276: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3276.pdf

``cxx_defaulted_functions``
  Defaulted functions, as defined in N2346_.

.. _N2346: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2346.htm

``cxx_deleted_functions``
  Deleted functions, as defined in  N2346_.

.. _N2346: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2346.htm

``cxx_explicit_conversions``
  Explicit conversion operators, as defined in N2437_.

.. _N2437: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2437.pdf

``cxx_inheriting_constructors``
  Inheriting constructors, as defined in N2540_.

.. _N2540: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2540.htm

``cxx_lambdas``
  Lambda functions, as defined in N2927_.

.. _N2927: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2927.pdf

``cxx_nonstatic_member_init``
  Non-static data member initialization, as defined in N2756.

.. _N2756: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2756.htm

``cxx_noexcept``
  Exception specifications, as defined in N3050_.

.. _N3050: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3050.html

``cxx_nullptr``
  Null pointer, as defined in N2431_.

.. _N2431: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2431.pdf

``cxx_range_for``
  Range-based for, as defined in N2930_.

.. _N2930: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2930.html

``cxx_raw_string_literals``
  Raw string literals, as defined in N2442_.

.. _N2442: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2442.htm

``cxx_reference_qualified_functions``
  Reference qualified functions, as defined in N2439_.

.. _N2439: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2439.htm

``cxx_rvalue_references``
  R-value references, as defined in N2118_.

.. _N2118: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2118.html

``cxx_alias_templates``
  Template aliases, as defined in N2258_.

.. _N2258: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2258.pdf

``cxx_user_literals``
  User-defined literals, as defined in N2765_.

.. _N2765: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2765.pdf

``cxx_unicode_literals``
  Unicode string literals, as defined in N2442_.

.. _N2442: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2442.htm

``cxx_unrestricted_unions``
  Unrestricted unions, as defined in N2544_.

.. _N2544: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2544.pdf

``cxx_extern_templates``
  Extern templates, as defined in N1987_.

.. _N1987: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1987.htm

``cxx_alignas``
  Alignment control ``alignas``, as defined in N2341_.

.. _N2341: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2341.pdf

``cxx_alignof``
  Alignment control ``alignof``, as defined in N2341_.

.. _N2341: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2341.pdf

``cxx_attributes``
  Generic attributes, as defined in N2761_.

.. _N2761: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2761.pdf

``cxx_generalized_initializers``
  Initializer lists, as defined in N2672_.

.. _N2672: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2672.htm

``cxx_uniform_initialization``
  Uniform intialization, as defined in N2640_.

.. _N2640: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2640.pdf

``cxx_variadic_macros``
  Variadic macros, as defined in N1653_.

.. _N1653: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2004/n1653.htm

``cxx_inline_namespaces``
  Inline namespaces, as defined in N2535_.

.. _N2535: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2535.htm

``thread_local``
  Thread-local variables, as defined in N2659_.

.. _N2659: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2659.htm

``cxx_sizeof_member``
  Size of non-static data members, as defined in N2253_.

.. _N2253: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2253.html

``cxx_decltype_incomplete_return_types``
  Decltype on incomplete return types, as defined in N3276_.

.. _N3276 : http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3276.pdf

``cxx_right_angle_brackets``
  Right angle bracket parsing, as defined in N1757_.
.. _N1757: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1757.html
