//  boost/interop/string_0x.hpp  -------------------------------------------------------//

//  Copyright Beman Dawes 2011

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//--------------------------------------------------------------------------------------//
//                                                                                      //
//   In namespace boost, provide typdefs for C++0x types if present, otherwise for      //
//   C++03 equivalent types. By using the boost typedefs, these C++0x features become   //
//   available for use with C++03 compilers.                                            //
//                                                                                      //
//   Also provide character type u8_t and string type u8string for UTF-8 encoded        //
//   characters and strings.                                                            //
//                                                                                      //
//   Boost              C++11            C++03                                          //
//   ----------------   --------------   --------------------------------               //
//   boost::u8_t        unsigned char    unsigned char                                  //
//   boost::u16_t       char16_t         uint16_t                                       //
//   boost::u32_t       char32_t         uint32_t                                       //
//   boost::u8string    std::basic_string<boost::u8_t>                                  //
//                                       std::basic_string<boost::u8_t>                 //
//   boost::u16string   std::u16string   std::basic_string<boost::u16_t>                //
//   boost::u32string   std::u32string   std::basic_string<boost::u32_t>                //
//                                                                                      //
//   Will use the typedefs provided by Microsoft Visual C++ 2010 if present             //
//                                                                                      //
//   Thanks to Mathias Gaunard and others for discussions leading to the final form     //
//   of these typedefs.                                                                 //
//                                                                                      //
//--------------------------------------------------------------------------------------//

#if !defined(BOOST_STRING_0X_HPP)
# define BOOST_STRING_0X_HPP

# include <boost/config.hpp>
# include <boost/cstdint.hpp>
# include <string>

//--------------------------------------------------------------------------------------//
//                                                                                      
//  Naming and namespace rationale
//
//  The purpose of this header is to emulate the new C++0x character and string types,
//  to provide an additional character and string type specified to have UTF-8
//  encoding, and to support a Boost basic_string type with associated typedefs. The
//  Boost basic_string type is derived from std::basic_string and adds interoperability
//  between instantiations with different character types.
//
//  The Boost interoperable string types are intended to be direct drop-ins for many
//  current uses of standard library string types. They have the same names as the
//  standard library types to emphasize that the interfaces are the same except for the
//  interoperability extensions.
//
//  One choice would be to put the C++0x emulation names at global scope for the new
//  character types, and put the C++0x string types in namespace std. That's the approach
//  taken by Microsoft Visual Studio 2010, but seems upsetting and controversion to at
//  least some Boost users and developers. So to avoid controversy these name go into
//  namespace boost, as is usual for Boost C++11 emulation names such as in header
//  <boost/cstdint.hpp>.
//
//  The interoperability extensions then go in their own sub-namespace to avoid clashes.
//
//--------------------------------------------------------------------------------------//

namespace boost
{
    typedef unsigned char                   u8_t;      
    typedef std::basic_string<boost::u8_t>  u8string;  // UTF-8 encoding

# if defined(BOOST_NO_CHAR16_T) && (!defined(_MSC_VER) || _MSC_VER < 1600)  // 1600 == VC++10 
    typedef boost::uint_least16_t           u16_t;
    typedef std::basic_string<boost::u16_t> u16string;
# else
    typedef char16_t                        u16_t;
    typedef std::u16string                  u16string;
# endif

# if defined(BOOST_NO_CHAR32_T) && (!defined(_MSC_VER) || _MSC_VER < 1600)  // 1600 == VC++10 
    typedef  boost::uint_least16_t          u32_t;
    typedef std::basic_string<boost::u32_t> u32string;
# else
    typedef char32_t                        u32_t;
    typedef std::u32string                  u32string;
# endif

}  // namespace boost

#endif  // !defined(BOOST_STRING_0X_HPP)
