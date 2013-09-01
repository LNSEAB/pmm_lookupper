#ifndef PMM_LOOKUPPER_FILE_HPP_
#define PMM_LOOKUPPER_FILE_HPP_

#include <vector>
#include <fstream>
#include "winapi.hpp"

namespace pmm_lookupper {

	inline std::vector< char > read_file(boost::string_ref path)
	{
		std::ifstream ifs( convert_code( path, CP_UTF8, CP_OEMCP ), std::ios::binary );
		if( ifs.fail() ) {
			return {};
		}

		std::istreambuf_iterator< char > first( ifs ), last;
		return { first, last };
	}

	template <class Iterator>
	inline bool drive_letter_exists(Iterator itr, Iterator end)
	{
		boost::string_ref const str( ":\\" );

		if( std::distance( itr, end ) < static_cast< std::ptrdiff_t >( str.size() + 1 ) ) {
			return false;
		}

		for( int i = 'A'; i <= 'Z'; ++i ) {
			if( *itr == i ) {
				return std::equal( str.begin(), str.end(), itr + 1 );
			}
		}

		return false;
	}
	
	inline std::vector< std::string > find_file_paths(std::vector< char > const& buf, char end)
	{
		std::vector< std::string > result;

		for( auto itr = buf.begin(); itr != buf.end(); ++itr ) {
			if( !drive_letter_exists( itr, buf.end() ) ) {
				continue;
			}

			auto last = std::find( itr, buf.end(), end );
			result.push_back( convert_code( std::string( itr, last ), CP_OEMCP, CP_UTF8 ) );

			itr = last;
		}

		return result;
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_FILE_HPP_
