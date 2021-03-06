#ifndef PMM_LOOKUPPER_FILTER_HPP_
#define PMM_LOOKUPPER_FILTER_HPP_

#include <vector>
#include <string>
#include "pmm.hpp"

namespace pmm_lookupper { 

	inline std::vector< std::string > remove_file_path(std::vector< std::string > const& paths)
	{
		std::vector< std::string > result;

		for( auto const& str : paths ) {
			if( PathIsDirectoryW( multibyte_to_wide( str, CP_UTF8 ).c_str() ) ) {
				result.emplace_back( str );
			}
			else {
				auto const p = str.find_last_of( '\\' );
				result.emplace_back( str.substr( 0, p ) );
			}
		}

		return result;
	}

	inline std::vector< std::string > match_extension(
		std::vector< std::string > const& paths, std::vector< std::string > const& exts
	) {
		std::vector< std::string > result;

		for( auto const& path : paths ) {
			auto p = path.find_last_of( '.' );
			if( p == path.npos ) {
				continue;
			}

			for( auto const& ext : exts ) {
				if( path.find( ext, p ) != path.npos ) {
					result.emplace_back( path );
					break;
				}
			}
		}

		return result;
	}

	inline std::string get_extension(std::string const& path)
	{
		auto p = path.find_last_of( '.' );
		if( p == path.npos ) {
			return {};
		}

		return path.substr( p, path.npos );
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_FILTER_HPP_
