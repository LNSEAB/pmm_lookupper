#ifndef PMM_LOOKUPPER_PMM_HPP_
#define PMM_LOOKUPPER_PMM_HPP_

#include <string>
#include <vector>
#include <tuple>
#include <fstream>
#include <boost/utility/string_ref.hpp>
#include <boost/variant.hpp>

namespace pmm_lookupper {

namespace detail {

	using buffer_type = std::vector< char >;

	inline buffer_type read_file(boost::string_ref path)
	{
		std::ifstream ifs( convert_code( path, CP_UTF8, CP_OEMCP ), std::ios::binary );
		if( ifs.fail() ) {
			return {};
		}

		std::istreambuf_iterator< char > first( ifs ), last;
		return { first, last };
	}

	inline bool is_pmm_file(buffer_type const& buf) noexcept
	{
		boost::string_ref const sig( "Polygon Movie maker 0001" );

		if( buf.size() <= sig.size() ) {
			return false;
		}

		return std::equal( buf.begin(), buf.begin() + sig.size(), sig.begin() );
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

	template <class Iterator>
	inline Iterator find_path_end(Iterator itr, Iterator end)
	{
		return std::find( itr, end, '\0' );
	}

	inline std::vector< std::string > find_file_paths(buffer_type const& buf)
	{
		std::vector< std::string > result;

		for( auto itr = buf.begin(); itr != buf.end(); ++itr ) {
			if( !drive_letter_exists( itr, buf.end() ) ) {
				continue;
			}

			auto last = find_path_end( itr, buf.end() );
			result.push_back( convert_code( std::string( itr, last ), CP_OEMCP, CP_UTF8 ) );

			itr = last;
		}

		return result;
	}

} // namespace detail

	inline boost::variant< std::vector< std::string >, std::string > pmm_contain_file_paths(boost::string_ref path)
	{
		auto const buf = detail::read_file( path );
		if( buf.empty() || !detail::is_pmm_file( buf ) ) {
			return { std::string( "ファイルを読み込めませんでした" ) };
		}

		return { detail::find_file_paths( buf ) };
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_PMM_HPP_
