#ifndef PMM_LOOKUPPER_WINDOW_TABLE_HPP_
#define PMM_LOOKUPPER_WINDOW_TABLE_HPP_

#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>

namespace pmm_lookupper {

	class window_table_impl
	{
		boost::container::flat_map< HWND, boost::shared_ptr< void > > table_;

		window_table_impl() noexcept
		{ }

	public:
		template <class T>
		inline void insert(HWND hwnd, boost::shared_ptr< T > p)
		{
			table_.insert( std::make_pair( hwnd, boost::static_pointer_cast< void >( p ) ) );
		}

		inline void erase(HWND hwnd)
		{
			auto itr = table_.find( hwnd );
			if( itr != table_.end() ) {
				table_.erase( itr );
			}
		}

		template <class T>
		inline boost::shared_ptr< T > get(HWND hwnd) const noexcept
		{
			auto itr = table_.find( hwnd );
			if( itr == table_.end() ) {
				return {};
			}

			return boost::static_pointer_cast< T >( itr->second );
		}

		inline static window_table_impl& instance()
		{
			static std::unique_ptr< window_table_impl > obj( new window_table_impl );
			return *obj;
		}
	};

	inline window_table_impl& window_table()
	{
		return window_table_impl::instance();
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_WINDOW_TABLE_HPP_
