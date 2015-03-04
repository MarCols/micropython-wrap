#ifndef MICROPYTHON_WRAP_DETAIL_MICROPYTHON_H
#define MICROPYTHON_WRAP_DETAIL_MICROPYTHON_H

#ifdef _MSC_VER
#pragma warning ( disable : 4200 ) //nonstandard extension used : zero-sized array in struct/union
#endif
extern "C"
{
  #include <py/mpconfig.h>
  #include <py/misc.h>
  #include <py/qstr.h>
  #include <py/nlr.h>
  #include <py/obj.h>
  #include <py/objfun.h>
  #include <py/objmodule.h>
  #include <py/runtime.h>
  #include <py/runtime0.h>
}
#ifdef _MSC_VER
#pragma warning ( default : 4200 )
#endif

#include <limits>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace upywrap
{
  inline mp_obj_t new_qstr( qstr what )
  {
    return MP_OBJ_NEW_QSTR( what );
  }

  inline mp_obj_t new_qstr( const char* what )
  {
    return new_qstr( qstr_from_str( what ) );
  }

  inline mp_obj_t mp_make_function_n( int n_args, void* fun )
  {
    auto o = m_new_obj( mp_obj_fun_builtin_t );
    o->base.type = &mp_type_fun_builtin;
    o->is_kw = false;
    o->n_args_min = n_args;
    o->n_args_max = n_args;
    o->fun = fun;
    return o;
  }

  inline mp_obj_module_t* CreateModule( const char* name, bool doRegister = false )
  {
    const qstr qname = qstr_from_str( name );
    mp_obj_module_t* mod = (mp_obj_module_t*) mp_obj_new_module( qname );
    if( doRegister )
      mp_module_register( qname, mod );
    return mod;
  }

  inline void RaiseTypeException( const char* msg )
  {
    nlr_raise( mp_obj_new_exception_msg( &mp_type_TypeError, msg ) );
  }

  inline void RaiseAttributeException( qstr name, qstr attr )
  {
    nlr_raise( mp_obj_new_exception_msg_varg( &mp_type_AttributeError, "'%s' object has no attribute '%s'", qstr_str( name ), qstr_str( attr ) ) );
  }

  inline mp_obj_t RaiseRuntimeException( const char* msg )
  {
    nlr_raise( mp_obj_new_exception_msg( &mp_type_RuntimeError, msg ) );
  }

  #define UPYWRAP_MAX_NATIVE_ARGS 3

#ifdef UPYWRAP_NOEXCEPTIONS
  #define UPYWRAP_TRY
  #define UPYWRAP_CATCH
  inline bool HasExceptions()
  {
    return false;
  }
#else
  #define UPYWRAP_TRY try {
  #define UPYWRAP_CATCH } catch( const std::exception& e ) { return RaiseRuntimeException( e.what() ); }
  inline bool HasExceptions()
  {
    return true;
  }
#endif

#if MICROPY_ENABLE_GC && MICROPY_ENABLE_FINALISER
  inline bool HasFinaliser()
  {
    return true;
  }
#else
  #define UPYWRAP_NOFINALISER
  inline bool HasFinaliser()
  {
    return false;
  }
#endif

  //Implement some casts used and check for overflow where trunctaion is needed.
  //Only implemented for signed/unsiged 64bit to corresponding 32bit, rest resolves to static_cast.
  template< class S, class T >
  struct safe_integer_caster
  {
    static T Convert( S src )
    {
      return static_cast< T >( src );
    }
  };

  template< class T, bool uns >
  struct abs_all
  {
    static T abs( T t )
    {
      return t;
    }
  };

  template< class T >
  struct abs_all< T, false >
  {
    static T abs( T t )
    {
      return std::abs( t );
    }
  };


#ifdef max
  #undef max
#endif

  template< class T, class S >
  static void IntegerBoundCheck( S src )
  {
    if( abs_all< S, std::is_unsigned< S >::value >::abs( src ) > static_cast< S >( std::numeric_limits< T >::max() ) )
      RaiseTypeException( "Source integer does not fit in target integer" );
  }

#if defined( __LP64__ ) || defined( _WIN64 )
  template<>
  struct safe_integer_caster< mp_int_t, int >
  {
    static int Convert( mp_int_t src )
    {
      IntegerBoundCheck< int >( src );
      return static_cast< int >( src );
    }
  };

  template<>
  struct safe_integer_caster< std::size_t, unsigned >
  {
    static unsigned Convert( std::size_t src )
    {
      IntegerBoundCheck< unsigned >( src );
      return static_cast< unsigned >( src );
    }
  };
#else
  template<>
  struct safe_integer_caster< std::int64_t, int >
  {
    static int Convert( std::int64_t src )
    {
      IntegerBoundCheck< int >( src );
      return static_cast< int >( src );
    }
  };

  template<>
  struct safe_integer_caster< std::uint64_t, unsigned >
  {
    static unsigned Convert( std::uint64_t src )
    {
      IntegerBoundCheck< unsigned >( src );
      return static_cast< unsigned >( src );
    }
  };
#endif

  template< class T, class S >
  T safe_integer_cast( S src )
  {
    return safe_integer_caster< S, T >::Convert( src );
  }

}

#endif //#ifndef MICROPYTHON_WRAP_DETAIL_MICROPYTHON_H
