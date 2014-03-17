#ifndef PMM_LOOKUPPER_PMM_HPP_
#define PMM_LOOKUPPER_PMM_HPP_

#include <string>
#include <vector>
#include <tuple>
#include <fstream>
#include <boost/utility/string_ref.hpp>
#include <boost/variant.hpp>
#include "file.hpp"

namespace pmm_lookupper {

namespace {

	inline bool is_pmm_file(std::vector< char > const& buf) noexcept
	{
		boost::string_ref const sig( "Polygon Movie maker 000" );

		if( buf.size() <= sig.size() ) {
			return false;
		}

		return std::equal( buf.begin(), buf.begin() + sig.size(), sig.begin() );
	}

} // namespace 

	inline std::vector< std::string > pmm_contain_file_paths(boost::string_ref path)
	{
		auto const buf = read_file( path );
		if( buf.empty() || !is_pmm_file( buf ) ) {
			return {};
		}

		return { find_file_paths( buf, '\0' ) };
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_PMM_HPP_
