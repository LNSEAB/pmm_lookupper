#ifndef PMM_LOOKUPPER_EMM_HPP_
#define PMM_LOOKUPPER_EMM_HPP_

#include <fstream>
#include <string>
#include <vector>
#include <boost/utility/string_ref.hpp>
#include "file.hpp"
#include "filter.hpp"

namespace pmm_lookupper {

	inline std::vector< std::string > emm_contain_file_paths(boost::string_ref path)
	{
		namespace qi = boost::spirit::qi;

		auto const buf = read_file( path );
		if( buf.empty() ) {
			return {};
		}

		std::vector< std::string > result;

		std::vector< std::string > const exts = {
			std::string( "fx" ),
			std::string( "fxsub" )
		};

		return { find_file_paths( buf, '\r' ) };
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_EMM_HPP_
