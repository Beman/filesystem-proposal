//  boost/cxx11_chars.hpp  --------------------------------------------------------//

//  Copyright Beman Dawes 2011

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//--------------------------------------------------------------------------------------//
//                                                                                      //
//   In namespace boost, provide typedefs for C++11 types if present, otherwise for     //
//   C++03 equivalent types. By using the boost typedefs, these C++11 features become   //
//   available for use with C++03 compilers.                                            //
//                                                                                      //
//                                                                                      //
//   Boost               C++11            C++03                                         //
//   ----------------    --------------   --------------------------------              //
//   boost::char16  char16_t         uint16_t                                      //
//   boost::char32  char32_t         uint32_t                                      //
//   boost::u16string    std::u16string   std::basic_string<boost::char16>         //
//   boost::u32string    std::u32string   std::basic_string<boost::char32>         //
//                                                                                      //
//   Uses the typedefs provided by Microsoft Visual C++ 2010 if present                 //
//                                                                                      //
//   Thanks to Mathias Gaunard and others for discussions leading to the final form     //
//   of these typedefs.                                                                 //
//                                                                                      //
//--------------------------------------------------------------------------------------//

#if !defined(BOOST_CXX11_CHAR_TYPES_HPP)
# define BOOST_CXX11_CHAR_TYPES_HPP

# include <boost/config.hpp>
# include <boost/cstdint.hpp>
# include <string>

//--------------------------------------------------------------------------------------//
//                                                                                      
//  Naming and namespace rationale
//
//  The purpose of this header is to emulate the new C++11 char16_t and char32_t
//  character and string types so that they can be used in C++03 programs.
//
//  The emulation names use char16/char32 rather than char16_t/char32_t to avoid use of
//  names that are keywords in C++11.
//
//  The emulation names are placed in namespace boost, as is usual for Boost C++11
//  emulation names such as those in header <boost/cstdint.hpp>.
//
//  An alternative would would have been to place the C++11 emulation names at global
//  scope, and put the C++11 string types in namespace std. That is the approach taken
//  by Microsoft Visual Studio 2010, but is controversion with some Boost users and
//  developers, and runs counter to usual Boost practice.  
//
//  Thanks to Mathias Gaunard and others for discussions leading to the final form
//  of these typedefs.
//
//--------------------------------------------------------------------------------------//

namespace boost
{

# if defined(BOOST_NO_CHAR16_T) && (!defined(_MSC_VER) || _MSC_VER < 1600)  // 1600 == VC++10 
    typedef boost::uint_least16_t             char16;
    typedef std::basic_string<boost::char16>  u16string;
# else
    typedef char16_t                          char16;
    typedef std::u16string                    u16string;
# endif

# if defined(BOOST_NO_CHAR32_T) && (!defined(_MSC_VER) || _MSC_VER < 1600)  // 1600 == VC++10 
    typedef  boost::uint_least16_t            char32;
    typedef std::basic_string<boost::char32>  u32string;
# else
    typedef char32_t                          char32;
    typedef std::u32string                    u32string;
# endif

}  // namespace boost

#endif  // !defined(BOOST_CXX11_CHAR_TYPES_HPP)
