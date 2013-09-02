#ifndef PMM_LOOKUPPER_EMM_HPP_
#define PMM_LOOKUPPER_EMM_HPP_

#include <fstream>
#include <string>
#include <vector>
#include <boost/range/algorithm.hpp>
#include <boost/utility/string_ref.hpp>
#include "file.hpp"
#include "filter.hpp"

namespace pmm_lookupper {

	inline bool is_emm_file(boost::string_ref path) 
	{
		std::ifstream ifs( convert_code( path, CP_UTF8, CP_OEMCP ), std::ios::binary );
		if( ifs.fail() ) {
			return false;
		}

		std::istreambuf_iterator< char > first( ifs ), last;
		boost::string_ref const seg( "[Info]\r\nVersion = 3\r\n" );

		return std::search( first, last, seg.begin(), seg.end() ) != last;
	}

	inline std::vector< std::string > emm_contain_file_paths(boost::string_ref path)
	{
		auto const buf = read_file( path );
		if( buf.empty() ) {
			return {};
		}

		std::vector< std::string > const exts = {
			std::string( "fx" ),
			std::string( "fxsub" )
		};

		auto const result = find_file_paths( buf, '\r' );
		return match_extension( result, exts );
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_EMM_HPP_
