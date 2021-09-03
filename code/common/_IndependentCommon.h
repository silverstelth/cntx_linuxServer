#ifndef __INDEPENDENT_COMMON_H__
#define __INDEPENDENT_COMMON_H__

#include <string>

// Temorary Common File : Reason :¡¡

namespace OWN
{
	typedef	unsigned long	ulong;
	typedef unsigned short	ushort;
	typedef unsigned int	uint;
	typedef unsigned char	uchar;

	typedef unsigned int    uint32;
	typedef unsigned short  uint16;
	typedef unsigned char   uint8;

	typedef	uint16			ucchar;

	typedef	signed		__int8		sint8;
	typedef	signed		__int16		sint16;
	//typedef	signed		__int32		sint32;
	//typedef	unsigned	__int32		uint32;
	typedef	signed		long		sint32;
	typedef	signed		__int64		sint64;
	typedef	unsigned	__int64		uint64;

	typedef				int			sint;			// at least 32bits (depend of processor)

	typedef std::basic_string<ucchar> ucstringbase;

	class ucstring : public ucstringbase
	{
	public:

		ucstring () {}

		ucstring (const ucstringbase &str) : ucstringbase (str) {}

		ucstring (const std::string &str) : ucstringbase ()
		{
			// We need to convert the char into 8bits unsigned int before promotion to 16 bits
			// otherwise, as char are signed on some compiler (MSCV for ex), the sign bit is extended to 16 bits.
			resize(str.size());
			std::string::const_iterator first(str.begin()), last(str.end());
			iterator dest(begin());
			for (;first != last; ++first, ++dest)
			{
				*dest = uint8(*first);
			}
		}

		~ucstring () {}

		ucstring &operator= (ucchar c)
		{
			resize (1);
			operator[](0) = c;
			return *this;
		}

		ucstring &operator= (const char *str)
		{
			resize (strlen (str));
			for (sint i = 0; i < (sint) strlen (str); i++)
			{
				operator[](i) = uint8(str[i]);
			}
			return *this;
		}

		ucstring &operator= (const std::string &str)
		{
			resize (str.size ());
			for (sint i = 0; i < (sint) str.size (); i++)
			{
				operator[](i) = uint8(str[i]);
			}
			return *this;
		}

		ucstring &operator= (const ucstringbase &str)
		{
			ucstringbase::operator =(str);
			return *this;
		}

		ucstring& operator= (const ucchar *str)
		{
			ucstringbase::operator =(str);
			return *this;
		}
		ucstring &operator+= (ucchar c)
		{
			resize (size() + 1);
			operator[](size()-1) = c;
			return *this;
		}

		ucstring &operator+= (const char *str)
		{
			uint s = (uint)size();
			resize (s + strlen(str));
			for (uint i = 0; i < strlen(str); i++)
			{
				operator[](s+i) = uint8(str[i]);
			}
			return *this;
		}

		ucstring &operator+= (const std::string &str)
		{
			uint s = (uint)size();
			resize (s + str.size());
			for (uint i = 0; i < str.size(); i++)
			{
				operator[](s+i) = uint8(str[i]);		
			}
			return *this;
		}

		ucstring &operator+= (const ucstringbase &str)
		{
			ucstringbase::operator +=(str);
			return *this;
		}

		const ucchar *c_str() const
		{
			const ucchar *tmp = ucstringbase::c_str();
			const_cast<ucchar*>(tmp)[size()] = 0;
			return tmp;
		}

		/// Converts the controlled ucstring to a string str
		void toString (std::string &str) const
		{
			str.resize (size ());
			for (sint i = 0; i < (sint) str.size (); i++)
			{
				if (operator[](i) > 255)
					str[i] = '?';
				else
					str[i] = (char) operator[](i);
			}
		}

		/// Converts the controlled ucstring and returns the resulting string
		std::string toString () const
		{
			std::string str;
			toString(str);
			return str;
		}

		/// Convert this ucstring (16bits char) into a utf8 string
		std::string toUtf8() const
		{
			std::string	res;
			ucstring::const_iterator first(begin()), last(end());
			for (; first != last; ++first)
			{
				//ucchar	c = *first;
				uint nbLoop = 0;
				if (*first < 0x80)
					res += char(*first);
				else if (*first < 0x800)
				{
					ucchar c = *first;
					c = c >> 6;
					c = c & 0x1F;
					res += c | 0xC0;
					nbLoop = 1;
				}
				else /*if (*first < 0x10000)*/
				{
					ucchar c = *first;
					c = c >> 12;
					c = c & 0x0F;
					res += c | 0xE0;
					nbLoop = 2;
				}

				for (uint i=0; i<nbLoop; ++i)
				{
					ucchar	c = *first;
					c = c >> ((nbLoop - i - 1) * 6);
					c = c & 0x3F;
					res += char(c) | 0x80; 
				}
			}
			return res;
		}

		// for luabind (can't bind to 'substr' else ...)
		ucstring luabind_substr(size_type pos = 0, size_type n = npos) const
		{
			return ucstringbase::substr(pos, n);
		}

		/// Convert the utf8 string into this ucstring (16 bits char)
		void fromUtf8(const std::string &stringUtf8)
		{
			// clear the string
			erase();

			uint8 c;
			ucchar code;
			sint iterations = 0;

			std::string::const_iterator first(stringUtf8.begin()), last(stringUtf8.end());
			for (; first != last; )
			{
				c = *first++;
				code = c;

				if ((code & 0xFE) == 0xFC)
				{
					code &= 0x01;
					iterations = 5;
				}
				else if ((code & 0xFC) == 0xF8)
				{
					code &= 0x03;
					iterations = 4;
				}
				else if ((code & 0xF8) == 0xF0)
				{
					code &= 0x07;
					iterations = 3;
				}
				else if ((code & 0xF0) == 0xE0)
				{
					code &= 0x0F;
					iterations = 2;
				}
				else if ((code & 0xE0) == 0xC0)
				{
					code &= 0x1F;
					iterations = 1;
				}
				else if ((code & 0x80) == 0x80)
				{
				}
				else
				{
					push_back(code);
					iterations = 0;
				}

				if (iterations)
				{
					for (sint i = 0; i < iterations; i++)
					{
						if (first == last)
						{
							return;
						}

						uint8 ch;
						ch = *first ++;

						if ((ch & 0xC0) != 0x80)
						{
							code = '?';
							break;
						}

						code <<= 6;
						code |= (ucchar)(ch & 0x3F);
					}
					push_back(code);
				}
			}
		}

		static ucstring makeFromUtf8(const std::string &stringUtf8)
		{
			ucstring ret;
			ret.fromUtf8(stringUtf8);

			return ret;
		}
	};

	inline ucstring operator+(const ucstringbase &ucstr, ucchar c)
	{
		ucstring	ret;
		ret= ucstr;
		ret+= c;
		return ret;
	}

	inline ucstring operator+(const ucstringbase &ucstr, const char *c)
	{
		ucstring	ret;
		ret= ucstr;
		ret+= c;
		return ret;
	}

	inline ucstring operator+(const ucstringbase &ucstr, const std::string &c)
	{
		ucstring	ret;
		ret= ucstr;
		ret+= c;
		return ret;
	}

	inline ucstring operator+(ucchar c, const ucstringbase &ucstr)
	{
		ucstring	ret;
		ret= c;
		ret += ucstr;
		return ret;
	}

	inline ucstring operator+(const char *c, const ucstringbase &ucstr)
	{
		ucstring	ret;
		ret= c;
		ret += ucstr;
		return ret;
	}

	inline ucstring operator+(const std::string &c, const ucstringbase &ucstr)
	{
		ucstring	ret;
		ret= c;
		ret += ucstr;
		return ret;
	}
}

#endif // __INDEPENDENT_COMMON_H__