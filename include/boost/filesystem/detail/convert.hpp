//  boost/interop/string_interop.hpp  --------------------------------------------------//

//  Copyright Beman Dawes 2011, 2012
//  Copyright (c) 2004 John Maddock

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//--------------------------------------------------------------------------------------//
//                                                                                      //
//         Conversions to enable interoperation between character strings               //
//                      of different types and encodings.                               //
//                                                                                      //
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//  John Maddock's boost/regex/pending/unicode_iterator.hpp introduced me to the idea   //
//  of performing conversion via iterator adapters. The code below for the UTF-8        //
//  to/from UTF-32 and UTF-16 to/from UTF-32 adapers was based on that header.          //
//--------------------------------------------------------------------------------------//

#if !defined(BOOST_STRING_INTEROP_HPP)
#define BOOST_STRING_INTEROP_HPP

#include <boost/interop/detail/config.hpp>

#ifdef BOOST_POSIX_API
#  error "Sorry, this proof-of-concept implementation is Windows only."
# endif

#include <boost/interop/string_types.hpp>
#include <boost/interop/detail/is_iterator.hpp>
//#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <limits.h> // CHAR_BIT

#ifndef BOOST_NO_DEFAULTED_FUNCTIONS
# define BOOST_DEFAULTED = default;
#else
# define BOOST_DEFAULTED {}
#endif

namespace boost
{
namespace interop
{

//--------------------------------------------------------------------------------------//
//                                     Synopsis                                         //
//--------------------------------------------------------------------------------------//

//  codecs
class narrow;   // native encoding for char
class wide;     // native encoding for wchar_t
class utf8;     // UTF-8 encoding for char
class utf16;    // UTF-16 encoding for char16_t
class utf32;    // UTF-32 encoding for char32_t
class default_codec;

//  conversion_iterator
template <class ToCodec, class FromCodec, class ForwardIterator>
  class conversion_iterator;

//  see select_codec trait below

//  see convert() functions below

//---------------------------------  Requirements  -------------------------------------//
//
//  DefaultCtorEndIterator:
//
//  For an iterator of type T, T() constructs the end iterator.
//
//  Codec:
//
//  from_iterator meets the DefaultCtorEndIterator requirements.
//  iterator_traits<from_iterator>::value_type is char32_t.
//
//  to_iterator meets the DefaultCtorEndIterator requirements.
//  ForwardIterator must meet the DefaultCtorEndIterator requirements.
//  iterator_traits<ForwardIterator>::value_type must be char32_t.



//--------------------------------------------------------------------------------------//
//                                  Implementation                                      //
//--------------------------------------------------------------------------------------//


//--------------------------------------------------------------------------------------//
//                                      codecs                                          //
//--------------------------------------------------------------------------------------//

//------------------------------------  helpers  ---------------------------------------//

namespace detail{

static const ::boost::uint16_t high_surrogate_base = 0xD7C0u;
static const ::boost::uint16_t low_surrogate_base = 0xDC00u;
static const ::boost::uint32_t ten_bit_mask = 0x3FFu;

inline bool is_high_surrogate(::boost::uint16_t v)
{
   return (v & 0xFFFFFC00u) == 0xd800u;
}
inline bool is_low_surrogate(::boost::uint16_t v)
{
   return (v & 0xFFFFFC00u) == 0xdc00u;
}
template <class T>
inline bool is_surrogate(T v)
{
   return (v & 0xFFFFF800u) == 0xd800;
}

inline unsigned utf8_byte_count(boost::uint8_t c)
{
   // if the most significant bit with a zero in it is in position
   // 8-N then there are N bytes in this UTF-8 sequence:
   boost::uint8_t mask = 0x80u;
   unsigned result = 0;
   while(c & mask)
   {
      ++result;
      mask >>= 1;
   }
   return (result == 0) ? 1 : ((result > 4) ? 4 : result);
}

inline unsigned utf8_trailing_byte_count(boost::uint8_t c)
{
   return utf8_byte_count(c) - 1;
}

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4100)
#endif
inline void invalid_utf32_code_point(::boost::uint32_t val)
{
   std::stringstream ss;
   ss << "Invalid UTF-32 code point U+" << std::showbase << std::hex << val << " encountered while trying to encode UTF-16 sequence";
   std::out_of_range e(ss.str());
   BOOST_INTEROP_THROW(e);
}
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

} // namespace detail

//----------------------------  select_codec type selector  ---------------------------//

  template <class charT> struct select_codec;
  template <> struct select_codec<char>    { typedef narrow type; };
  template <> struct select_codec<wchar_t> { typedef wide type; };
  template <> struct select_codec<u8_t>    { typedef utf8 type; };
  template <> struct select_codec<u16_t>   { typedef utf16 type; };
  template <> struct select_codec<u32_t>   { typedef utf32 type; };

//-----------------------------  default_codec pseudo codec  ---------------------------//
//
//  provides lazy select_codec selection so that codec template parameters with defaults
//  can appear before the template parameter that determines charT.  

class default_codec
{
public:
  template <class charT>
  struct codec
  { 
    typedef typename select_codec<charT>::type type; 
  };

};

//----------------------------------  narrow codec  ------------------------------------//

namespace detail
{
  // for this proof-of-concept, simply linking in codec tables is sufficient
  extern const boost::u16_t  to_utf16[];  
  extern const unsigned char to_char[];
  extern const boost::uint8_t slice_index[];
}

class narrow
{
public:
  typedef char value_type;

  template <class charT>
  struct codec { typedef narrow type; };

  //  narrow::from_iterator  -----------------------------------------------------------//
  //
  //  meets the DefaultCtorEndIterator requirements

  template <class ForwardIterator>  
  class from_iterator
   : public boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t>
  {
    typedef boost::iterator_facade<from_iterator<ForwardIterator>,
      u32_t, std::input_iterator_tag, const u32_t> base_type;

    typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

    static_assert(boost::is_same<base_value_type, char>::value,
      "ForwardIterator value_type must be char for this from_iterator");
    BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 8);
    BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);
    
    ForwardIterator  m_begin;
    ForwardIterator  m_end;
    bool             m_default_end;

  public:

    // end iterator
    from_iterator() : m_default_end(true) {}

    // by_null
    from_iterator(ForwardIterator begin) : m_begin(begin), m_end(begin),
      m_default_end(false) 
    {
      for (;
           *m_end != typename std::iterator_traits<ForwardIterator>::value_type();
           ++m_end) {}
    }

    // by range
    template <class T>
    from_iterator(ForwardIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<ForwardIterator, T>, void >::type* x=0)
      : m_begin(begin), m_end(end), m_default_end(false) {}

    // by_size
    from_iterator(ForwardIterator begin, std::size_t sz)
      : m_begin(begin), m_end(begin), m_default_end(false) {std::advance(m_end, sz);}

    u32_t dereference() const
    {
      BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
        "Attempt to dereference end iterator");
      unsigned char c = static_cast<unsigned char>(*m_begin);
      return static_cast<u32_t>(interop::detail::to_utf16[c]);
    }

    bool equal(const from_iterator& that) const
    {
      if (m_default_end || m_begin == m_end)
        return that.m_default_end || that.m_begin == that.m_end;
      if (that.m_default_end || that.m_begin == that.m_end)
        return false;
      return m_begin == that.m_begin;
    }

    void increment()
    { 
      BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
        "Attempt to increment end iterator");
      ++m_begin;
    }
  };

  //  narrow::to_iterator  -------------------------------------------------------------//
  //
  //  meets the DefaultCtorEndIterator requirements

  template <class ForwardIterator>  
  class to_iterator
   : public boost::iterator_facade<to_iterator<ForwardIterator>,
       char, std::input_iterator_tag, const char>
  {
     typedef boost::iterator_facade<to_iterator<ForwardIterator>,
       char, std::input_iterator_tag, const char> base_type;
   
     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

     static_assert(boost::is_same<base_value_type, u32_t>::value,
       "ForwardIterator value_type must be char32_t for this iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(char)*CHAR_BIT == 8);

     ForwardIterator m_begin;

  public:
    // construct:
    to_iterator() : m_begin(ForwardIterator()) {}
    to_iterator(ForwardIterator begin) : m_begin(begin) {}

    char dereference() const
    {
      BOOST_ASSERT_MSG(m_begin != ForwardIterator(),
        "Attempt to dereference end iterator");
      u32_t c = *m_begin;
      //cout << "*** c is " << hex << c << '\n';
      //cout << "    to_slice[c >> 7] << 7 is "
      //  << unsigned int(interop::detail::slice_index[c >> 7] << 7) << '\n';
      return static_cast<char>(interop::detail::to_char
        [
          (interop::detail::slice_index[c >> 7] << 7) | (c & 0x7f)
        ]);
    }

    bool equal(const to_iterator& that) const
    {
      return m_begin == that.m_begin;
    }

    void increment()
    { 
      BOOST_ASSERT_MSG(m_begin != ForwardIterator(),
        "Attempt to increment end iterator");
      ++m_begin;  // may change m_begin to end iterator
    }

  };  // to_iterator
};  // narrow

//-----------------------------------  wide codec  -------------------------------------//

  //------------------------------------------------------------------------------------//
  //  Warning: wide and utf16 duplicate each other. TODO: refactor out duplicate code.  // 
  //------------------------------------------------------------------------------------//

class wide
{
public:
  typedef wchar_t value_type;
  template <class charT> struct codec { typedef wide type; };

  //  wide::from_iterator  -------------------------------------------------------------//

  template <class ForwardIterator>
  class from_iterator
   : public boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t>
  {
     typedef boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t> base_type;
     // special values for pending iterator reads:
     BOOST_STATIC_CONSTANT(u32_t, read_pending = 0xffffffffu);

     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

     static_assert(boost::is_same<base_value_type, wchar_t>::value,
       "ForwardIterator value_type must be wchar_t for this from_iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 16);
     BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

     ForwardIterator  m_begin;   // current position
     ForwardIterator  m_end;  
     mutable u32_t    m_value;     // current value or read_pending
     bool             m_default_end;

   public:

    // end iterator
    from_iterator() : m_default_end(true) {}

    // by_null
    from_iterator(ForwardIterator begin) : m_begin(begin), m_end(begin),
      m_default_end(false) 
    {
      for (;
           *m_end != typename std::iterator_traits<ForwardIterator>::value_type();
           ++m_end) {}
      m_value = read_pending;
    }

    // by range
    template <class T>
    from_iterator(ForwardIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<ForwardIterator, T>, void >::type* x=0)
      : m_begin(begin), m_end(end), m_default_end(false) { m_value = read_pending; }

    // by_size
    from_iterator(ForwardIterator begin, std::size_t sz)
      : m_begin(begin), m_end(begin), m_default_end(false)
    {
      std::advance(m_end, sz);
      m_value = read_pending;
    }

     typename base_type::reference
        dereference() const
     {
        BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
          "Attempt to dereference end iterator");
        if (m_value == read_pending)
           extract_current();
        return m_value;
     }

     bool equal(const from_iterator& that) const 
     {
       if (m_default_end || m_begin == m_end)
         return that.m_default_end || that.m_begin == that.m_end;
       if (that.m_default_end || that.m_begin == that.m_end)
         return false;
       return m_begin == that.m_begin;
     }

     void increment()
     {
       BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
         "Attempt to increment end iterator");
       // skip high surrogate first if there is one:
       if(detail::is_high_surrogate(*m_begin))
         ++m_begin;
       ++m_begin;
       m_value = read_pending;
     }

  private:
     static void invalid_code_point(::boost::uint16_t val)
     {
        std::stringstream ss;
        ss << "Misplaced UTF-16 surrogate U+" << std::showbase << std::hex << val
           << " encountered while trying to encode UTF-32 sequence";
        std::out_of_range e(ss.str());
        BOOST_INTEROP_THROW(e);
     }
     static void invalid_sequence()
     {
        std::out_of_range e(
          "Invalid UTF-16 sequence encountered while trying to encode UTF-32 character");
        BOOST_INTEROP_THROW(e);
     }
     void extract_current() const
     {
        m_value = static_cast<u32_t>(static_cast< ::boost::uint16_t>(*m_begin));
        // if the last value is a high surrogate then adjust m_begin and m_value as needed:
        if(detail::is_high_surrogate(*m_begin))
        {
           // precondition; next value must have be a low-surrogate:
           ForwardIterator next(m_begin);
           u16_t t = *++next;
           if((t & 0xFC00u) != 0xDC00u)
              invalid_code_point(t);
           m_value = (m_value - detail::high_surrogate_base) << 10;
           m_value |= (static_cast<u32_t>(
             static_cast<u16_t>(t)) & detail::ten_bit_mask);
        }
        // postcondition; result must not be a surrogate:
        if(detail::is_surrogate(m_value))
           invalid_code_point(static_cast<u16_t>(m_value));
     }
  };

  //  wide::to_iterator  ---------------------------------------------------------------//

  template <class ForwardIterator>
  class to_iterator
   : public boost::iterator_facade<to_iterator<ForwardIterator>,
      wchar_t, std::input_iterator_tag, const wchar_t>
  {
     typedef boost::iterator_facade<to_iterator<ForwardIterator>,
       wchar_t, std::input_iterator_tag, const wchar_t> base_type;

     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

     static_assert(boost::is_same<base_value_type, u32_t>::value,
       "ForwardIterator value_type must be char32_t for this iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(wchar_t)*CHAR_BIT == 16);

     ForwardIterator   m_begin;
     mutable wchar_t   m_values[3];
     mutable unsigned  m_current;

  public:

     typename base_type::reference
     dereference()const
     {
        if(m_current == 2)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_iterator& that)const
     {
        if(m_begin == that.m_begin)
        {
           // Both m_currents must be equal, or both even
           // this is the same as saying their sum must be even:
           return (m_current + that.m_current) & 1u ? false : true;
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 2)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 2;
           ++m_begin;
        }
     }

     // construct:
     to_iterator() : m_begin(ForwardIterator()), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
     }
     to_iterator(ForwardIterator b) : m_begin(b), m_current(2)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
    }
  private:

     void extract_current()const
     {
        // begin by checking for a code point out of range:
        ::boost::uint32_t v = *m_begin;
        if(v >= 0x10000u)
        {
           if(v > 0x10FFFFu)
              detail::invalid_utf32_code_point(*m_begin);
           // split into two surrogates:
           m_values[0] = static_cast<wchar_t>(v >> 10) + detail::high_surrogate_base;
           m_values[1] = static_cast<wchar_t>(v & detail::ten_bit_mask)
             + detail::low_surrogate_base;
           m_current = 0;
           BOOST_ASSERT(detail::is_high_surrogate(m_values[0]));
           BOOST_ASSERT(detail::is_low_surrogate(m_values[1]));
        }
        else
        {
           // 16-bit code point:
           m_values[0] = static_cast<wchar_t>(*m_begin);
           m_values[1] = 0;
           m_current = 0;
           // value must not be a surrogate:
           if(detail::is_surrogate(m_values[0]))
              detail::invalid_utf32_code_point(*m_begin);
        }
     }
  };

};

//-----------------------------------  utf8 codec  -------------------------------------//

class utf8
{
public:
  typedef char value_type;
  template <class charT> struct codec { typedef utf8 type; };

  //  utf8::from_iterator  -------------------------------------------------------------//
  //
  //  meets the DefaultCtorEndIterator requirements

  template <class ForwardIterator>
  class from_iterator
   : public boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t>
  {
     typedef boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t> base_type;
     // special values for pending iterator reads:
     BOOST_STATIC_CONSTANT(u32_t, read_pending = 0xffffffffu);

     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

    //static_assert(boost::is_same<base_value_type, char>::value,
    //  "ForwardIterator value_type must be char for this from_iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 8);
     BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

     ForwardIterator  m_begin;  // current position
     ForwardIterator  m_end;
     mutable u32_t    m_value;    // current value or read_pending
     bool             m_default_end;

   public:

    // end iterator
    from_iterator() : m_default_end(true) {}

    // by_null
    from_iterator(ForwardIterator begin) : m_begin(begin), m_end(begin),
      m_default_end(false) 
    {
      for (;
           *m_end != typename std::iterator_traits<ForwardIterator>::value_type();
           ++m_end) {}
      m_value = read_pending;
    }

    // by range
    template <class T>
    from_iterator(ForwardIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<ForwardIterator, T>, void >::type* x=0)
      : m_begin(begin), m_end(end), m_default_end(false) { m_value = read_pending; }

    // by_size
    from_iterator(ForwardIterator begin, std::size_t sz)
      : m_begin(begin), m_end(begin), m_default_end(false)
    {
      std::advance(m_end, sz);
      m_value = read_pending;
    }

     typename base_type::reference
        dereference() const
     {
        BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
          "Attempt to dereference end iterator");
        if (m_value == read_pending)
           extract_current();
        return m_value;
     }

     bool equal(const from_iterator& that) const
     {
       if (m_default_end || m_begin == m_end)
         return that.m_default_end || that.m_begin == that.m_end;
       if (that.m_default_end || that.m_begin == that.m_end)
         return false;
       return m_begin == that.m_begin;
     }

     void increment()
     {
        BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
          "Attempt to increment end iterator");
        unsigned count = detail::utf8_byte_count(*m_begin);
        std::advance(m_begin, count);
        m_value = read_pending;
     }
  private:
     static void invalid_sequence()
     {
        std::out_of_range e(
          "Invalid UTF-8 sequence encountered while trying to encode UTF-32 character");
        BOOST_INTEROP_THROW(e);
     }
     void extract_current()const
     {
        BOOST_ASSERT_MSG(m_begin != m_end,
          "Internal logic error: extracting from end iterator");
        m_value = static_cast<u32_t>(static_cast< ::boost::uint8_t>(*m_begin));
        // we must not have a continuation character:
        if((m_value & 0xC0u) == 0x80u)
           invalid_sequence();
        // see how many extra byts we have:
        unsigned extra = detail::utf8_trailing_byte_count(*m_begin);
        // extract the extra bits, 6 from each extra byte:
        ForwardIterator next(m_begin);
        for(unsigned c = 0; c < extra; ++c)
        {
           ++next;
           m_value <<= 6;
           m_value += static_cast<boost::uint8_t>(*next) & 0x3Fu;
        }
        // we now need to remove a few of the leftmost bits, but how many depends
        // upon how many extra bytes we've extracted:
        static const boost::uint32_t masks[4] = 
        {
           0x7Fu,
           0x7FFu,
           0xFFFFu,
           0x1FFFFFu,
        };
        m_value &= masks[extra];
        // check the result:
        if(m_value > static_cast<u32_t>(0x10FFFFu))
           invalid_sequence();
     }
  };

  //  utf8::to_iterator  ---------------------------------------------------------------//
  //
  //  meets the DefaultCtorEndIterator requirements

  template <class ForwardIterator>
  class to_iterator
   : public boost::iterator_facade<to_iterator<ForwardIterator>,
       char, std::input_iterator_tag, const char>
  {
     typedef boost::iterator_facade<to_iterator<ForwardIterator>,
       char, std::input_iterator_tag, const char> base_type;
   
     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

     static_assert(boost::is_same<base_value_type, u32_t>::value,
       "ForwardIterator value_type must be char32_t for this iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(char)*CHAR_BIT == 8);

     ForwardIterator   m_begin;
     mutable u8_t      m_values[5];
     mutable unsigned  m_current;

  public:

     typename base_type::reference
     dereference()const
     {
        if(m_current == 4)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_iterator& that)const
     {
        if(m_begin == that.m_begin)
        {
           // either the m_begin's must be equal, or one must be 0 and 
           // the other 4: which means neither must have bits 1 or 2 set:
           return (m_current == that.m_current)
              || (((m_current | that.m_current) & 3) == 0);
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 4)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 4;
           ++m_begin;
        }
     }

     // construct:
     to_iterator() : m_begin(ForwardIterator()), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
     }
     to_iterator(ForwardIterator b) : m_begin(b), m_current(4)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
    }
  private:

     void extract_current()const
     {
        boost::uint32_t c = *m_begin;
        if(c > 0x10FFFFu)
           detail::invalid_utf32_code_point(c);
        if(c < 0x80u)
        {
           m_values[0] = static_cast<unsigned char>(c);
           m_values[1] = static_cast<unsigned char>(0u);
           m_values[2] = static_cast<unsigned char>(0u);
           m_values[3] = static_cast<unsigned char>(0u);
        }
        else if(c < 0x800u)
        {
           m_values[0] = static_cast<unsigned char>(0xC0u + (c >> 6));
           m_values[1] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
           m_values[2] = static_cast<unsigned char>(0u);
           m_values[3] = static_cast<unsigned char>(0u);
        }
        else if(c < 0x10000u)
        {
           m_values[0] = static_cast<unsigned char>(0xE0u + (c >> 12));
           m_values[1] = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[2] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
           m_values[3] = static_cast<unsigned char>(0u);
        }
        else
        {
           m_values[0] = static_cast<unsigned char>(0xF0u + (c >> 18));
           m_values[1] = static_cast<unsigned char>(0x80u + ((c >> 12) & 0x3Fu));
           m_values[2] = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[3] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
        }
        m_current= 0;
     }
  };

};

//----------------------------------  utf16 codec  -------------------------------------//

  //------------------------------------------------------------------------------------//
  //  Warning: wide and utf16 duplicate each other. TODO: refactor out duplicate code.  // 
  //------------------------------------------------------------------------------------//

class utf16
{
public:
  typedef u16_t value_type;
  template <class charT> struct codec { typedef utf16 type; };

  //  utf16::from_iterator  ------------------------------------------------------------//

  template <class ForwardIterator>
  class from_iterator
   : public boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t>
  {
     typedef boost::iterator_facade<from_iterator<ForwardIterator>,
       u32_t, std::input_iterator_tag, const u32_t> base_type;
     // special values for pending iterator reads:
     BOOST_STATIC_CONSTANT(u32_t, read_pending = 0xffffffffu);

     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

    static_assert(boost::is_same<base_value_type, u16_t>::value,
      "ForwardIterator value_type must be char16_t for this from_iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 16);
     BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

     ForwardIterator  m_begin;   // current position
     ForwardIterator  m_end;  
     mutable u32_t    m_value;     // current value or read_pending
     bool             m_default_end;

   public:

    // end iterator
    from_iterator() : m_default_end(true) {}

    // by_null
    from_iterator(ForwardIterator begin) : m_begin(begin), m_end(begin),
      m_default_end(false) 
    {
      for (;
           *m_end != typename std::iterator_traits<ForwardIterator>::value_type();
           ++m_end) {}
      m_value = read_pending;
    }

    // by range
    template <class T>
    from_iterator(ForwardIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<ForwardIterator, T>, void >::type* x=0)
      : m_begin(begin), m_end(end), m_default_end(false) { m_value = read_pending; }

    // by_size
    from_iterator(ForwardIterator begin, std::size_t sz)
      : m_begin(begin), m_end(begin), m_default_end(false)
    {
      std::advance(m_end, sz);
      m_value = read_pending;
    }

     typename base_type::reference
        dereference() const
     {
        BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
          "Attempt to dereference end iterator");
        if (m_value == read_pending)
           extract_current();
        return m_value;
     }

     bool equal(const from_iterator& that) const 
     {
       if (m_default_end || m_begin == m_end)
         return that.m_default_end || that.m_begin == that.m_end;
       if (that.m_default_end || that.m_begin == that.m_end)
         return false;
       return m_begin == that.m_begin;
     }

     void increment()
     {
       BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
         "Attempt to increment end iterator");
       // skip high surrogate first if there is one:
       if(detail::is_high_surrogate(*m_begin))
         ++m_begin;
       ++m_begin;
       m_value = read_pending;
     }

  private:
     static void invalid_code_point(::boost::uint16_t val)
     {
        std::stringstream ss;
        ss << "Misplaced UTF-16 surrogate U+" << std::showbase << std::hex << val
           << " encountered while trying to encode UTF-32 sequence";
        std::out_of_range e(ss.str());
        BOOST_INTEROP_THROW(e);
     }
     static void invalid_sequence()
     {
        std::out_of_range e(
          "Invalid UTF-16 sequence encountered while trying to encode UTF-32 character");
        BOOST_INTEROP_THROW(e);
     }
     void extract_current() const
     {
        m_value = static_cast<u32_t>(static_cast< ::boost::uint16_t>(*m_begin));
        // if the last value is a high surrogate then adjust m_begin and m_value as needed:
        if(detail::is_high_surrogate(*m_begin))
        {
           // precondition; next value must have be a low-surrogate:
           ForwardIterator next(m_begin);
           u16_t t = *++next;
           if((t & 0xFC00u) != 0xDC00u)
              invalid_code_point(t);
           m_value = (m_value - detail::high_surrogate_base) << 10;
           m_value |= (static_cast<u32_t>(
             static_cast<u16_t>(t)) & detail::ten_bit_mask);
        }
        // postcondition; result must not be a surrogate:
        if(detail::is_surrogate(m_value))
           invalid_code_point(static_cast<u16_t>(m_value));
     }
  };

  //  utf16::to_iterator  --------------------------------------------------------------//

  template <class ForwardIterator>
  class to_iterator
   : public boost::iterator_facade<to_iterator<ForwardIterator>,
      u16_t, std::input_iterator_tag, const u16_t>
  {
     typedef boost::iterator_facade<to_iterator<ForwardIterator>,
       u16_t, std::input_iterator_tag, const u16_t> base_type;

     typedef typename std::iterator_traits<ForwardIterator>::value_type base_value_type;

     static_assert(boost::is_same<base_value_type, u32_t>::value,
       "ForwardIterator value_type must be char32_t for this iterator");
     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(u16_t)*CHAR_BIT == 16);

     ForwardIterator   m_begin;
     mutable u16_t     m_values[3];
     mutable unsigned  m_current;

  public:

     typename base_type::reference
     dereference()const
     {
        if(m_current == 2)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_iterator& that)const
     {
        if(m_begin == that.m_begin)
        {
           // Both m_currents must be equal, or both even
           // this is the same as saying their sum must be even:
           return (m_current + that.m_current) & 1u ? false : true;
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 2)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 2;
           ++m_begin;
        }
     }

     // construct:
     to_iterator() : m_begin(ForwardIterator()), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
     }
     to_iterator(ForwardIterator b) : m_begin(b), m_current(2)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
    }
  private:

     void extract_current()const
     {
        // begin by checking for a code point out of range:
        ::boost::uint32_t v = *m_begin;
        if(v >= 0x10000u)
        {
           if(v > 0x10FFFFu)
              detail::invalid_utf32_code_point(*m_begin);
           // split into two surrogates:
           m_values[0] = static_cast<u16_t>(v >> 10) + detail::high_surrogate_base;
           m_values[1] = static_cast<u16_t>(v & detail::ten_bit_mask)
             + detail::low_surrogate_base;
           m_current = 0;
           BOOST_ASSERT(detail::is_high_surrogate(m_values[0]));
           BOOST_ASSERT(detail::is_low_surrogate(m_values[1]));
        }
        else
        {
           // 16-bit code point:
           m_values[0] = static_cast<u16_t>(*m_begin);
           m_values[1] = 0;
           m_current = 0;
           // value must not be a surrogate:
           if(detail::is_surrogate(m_values[0]))
              detail::invalid_utf32_code_point(*m_begin);
        }
     }
  };

};

//----------------------------------  utf32 codec  -------------------------------------//

class utf32
{
public:
  typedef u32_t value_type;
  template <class charT> struct codec { typedef utf32 type; };

  //  utf32::from_iterator  ------------------------------------------------------------//

  template <class ForwardIterator>
  class from_iterator
    : public boost::iterator_facade<from_iterator<ForwardIterator>,
        u32_t, std::input_iterator_tag, const u32_t> 
  {
    static_assert(boost::is_same<typename std::iterator_traits<ForwardIterator>::value_type,
      u32_t>::value,
      "ForwardIterator value_type must be char32_t for this from_iterator");
    ForwardIterator  m_begin;
    ForwardIterator  m_end;
    bool             m_default_end;

  public:

    // end iterator
    from_iterator() : m_default_end(true) {}

    // by_null
    from_iterator(ForwardIterator begin) : m_begin(begin), m_end(begin),
      m_default_end(false) 
    {
      for (;
           *m_end != typename std::iterator_traits<ForwardIterator>::value_type();
           ++m_end) {}
    }

    // by range
    template <class T>
    from_iterator(ForwardIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<ForwardIterator, T>, void >::type* x=0)
      : m_begin(begin), m_end(end), m_default_end(false) {}

    // by_size
    from_iterator(ForwardIterator begin, std::size_t sz)
      : m_begin(begin), m_end(begin), m_default_end(false)
    {
      std::advance(m_end, sz);
    }

    u32_t dereference() const
    {
      BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
        "Attempt to dereference end iterator");
      return *m_begin;
    }

    bool equal(const from_iterator& that) const
    {
      if (m_default_end || m_begin == m_end)
        return that.m_default_end || that.m_begin == that.m_end;
      if (that.m_default_end || that.m_begin == that.m_end)
        return false;
      return m_begin == that.m_begin;
    }

    void increment()
    {
      BOOST_ASSERT_MSG(!m_default_end && m_begin != m_end,
        "Attempt to increment end iterator");
      ++m_begin;
    }
  };

  //  utf32::to_iterator  --------------------------------------------------------------//

  template <class ForwardIterator>
  class to_iterator
   : public boost::iterator_facade<to_iterator<ForwardIterator>,
      u32_t, std::input_iterator_tag, const u32_t>
  {
    ForwardIterator m_itr;
  public:
    to_iterator() : m_itr(ForwardIterator()) {}
    to_iterator(ForwardIterator itr) : m_itr(itr) {}
    u32_t dereference() const { return *m_itr; }
    bool equal(const to_iterator& that) const {return m_itr == that.m_itr;}
    void increment() { ++m_itr; }
  };

};

//--------------------------------------------------------------------------------------//
//                                 conversion_iterator                                  //
//--------------------------------------------------------------------------------------//

//  A conversion_iterator composes a ToCodec's to_iterator and a FromCodec's from_iterator
//  into a single iterator that adapts an ForwardIterator to FromCodec's value_type to 
//  behave as an iterator to the ToCodec's value_type.

template <class ToCodec, class FromCodec, class ForwardIterator>
class conversion_iterator
  : public ToCodec::template to_iterator<
      typename FromCodec::template from_iterator<ForwardIterator> >
{
public:
  typedef typename FromCodec::template from_iterator<ForwardIterator>  from_iterator_type;
  typedef typename ToCodec::template to_iterator<from_iterator_type>   to_iterator_type;

  conversion_iterator() BOOST_DEFAULTED

  conversion_iterator(ForwardIterator begin)
    : to_iterator_type(from_iterator_type(begin)) {}

  template <class U>
  conversion_iterator(ForwardIterator begin, U end,
    // enable_if ensures 2nd argument of 0 is treated as size, not range end
    typename boost::enable_if<boost::is_same<ForwardIterator, U>, void >::type* x=0)
    : to_iterator_type(from_iterator_type(begin, end)) {}

  conversion_iterator(ForwardIterator begin, std::size_t sz)
    : to_iterator_type(from_iterator_type(begin, sz)) {}
};

//--------------------------------------------------------------------------------------//
//                                 convert function                                     //
//--------------------------------------------------------------------------------------//

//  container
template <class ToCodec,
# ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
          class FromCodec = default_codec,
          class ToString = std::basic_string<typename ToCodec::value_type>,
# else
          class FromCodec,
          class ToString,
# endif
          class FromString>
  // enable_if resolves ambiguity with single iterator overload
typename boost::disable_if<boost::is_iterator<FromString>,
ToString>::type convert(const FromString& s)
{
  typedef conversion_iterator<ToCodec,
    typename FromCodec::template codec<typename FromString::value_type>::type,
    typename FromString::const_iterator>
      iter_type;

  ToString tmp;

  iter_type iter(s.cbegin(), s.cend());
  std::copy(iter, iter_type(), std::back_insert_iterator<ToString>(tmp));
  return tmp;
}

//  null terminated iterator
template <class ToCodec,
# ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
          class FromCodec = default_codec,
          class ToString = std::basic_string<typename ToCodec::value_type>,
# else
          class FromCodec,
          class ToString,
# endif
          class ForwardIterator>
  // enable_if resolves ambiguity with FromContainer overload
typename boost::enable_if<boost::is_iterator<ForwardIterator>,
ToString>::type convert(ForwardIterator begin)
{
  typedef conversion_iterator<ToCodec,
    typename FromCodec::template
      codec<typename std::iterator_traits<ForwardIterator>::value_type>::type,
    ForwardIterator>
      iter_type;

  ToString tmp;
  iter_type itr(begin);
  for (; itr != iter_type(); ++itr)
    tmp.push_back(*itr);
  return tmp;
}

//  iterator, size
template <class ToCodec,
# ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
          class FromCodec = default_codec,
          class ToString = std::basic_string<typename ToCodec::value_type>,
# else
          class FromCodec,
          class ToString,
# endif
          class ForwardIterator>
ToString convert(ForwardIterator begin, std::size_t sz)
{
  typedef conversion_iterator<ToCodec,
    typename FromCodec::template
      codec<typename std::iterator_traits<ForwardIterator>::value_type>::type,
    ForwardIterator>
      iter_type;

  ToString tmp;
  iter_type itr(begin, sz);
  for (; itr != iter_type(); ++itr)
    tmp.push_back(*itr);
  return tmp;
}

//  iterator range
template <class ToCodec,
# ifndef BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
          class FromCodec = default_codec,
          class ToString = std::basic_string<typename ToCodec::value_type>,
# else
          class FromCodec,
          class ToString,
# endif
          class ForwardIterator, class ForwardIterator2>
  // enable_if ensures 2nd argument of 0 is treated as size, not range end
typename boost::enable_if<boost::is_iterator<ForwardIterator2>,
ToString>::type convert(ForwardIterator begin, ForwardIterator2 end)
{
  typedef conversion_iterator<ToCodec,
    typename FromCodec::template
      codec<typename std::iterator_traits<ForwardIterator>::value_type>::type,
    ForwardIterator>
      iter_type;

  ToString tmp;
  iter_type itr(begin, end);
  for (; itr != iter_type(); ++itr)
    tmp.push_back(*itr);
  return tmp;
}

}  // namespace interop
}  // namespace boost

#endif  // BOOST_STRING_INTEROP_HPP
