#ifndef PMM_LOOKUPPER_RESULT_VIEW_HPP_
#define PMM_LOOKUPPER_RESULT_VIEW_HPP_

#include "winapi.hpp"
#include "event_handler.hpp"
#include "window_table.hpp"
#include "procedure.hpp"

namespace pmm_lookupper {

	template <class Parent>
	class result_view
	{
	public:
		using event_handler_type = event_handler< 
			result_view, 
			event::drop_files, event::destroy
		>;

	private:
		HWND parent_;
		HWND wnd_;
		event_handler_type eh_;
		std::vector< std::string > data_;

		result_view(HWND parent, UINT id) :
			parent_( parent ),
			wnd_( GetDlgItem( parent, id ) )
		{ 
			if( wnd_ ) {
				RECT rc;
				GetWindowRect( wnd_, &rc );

				insert_column( 0, "ファイルパス", static_cast< int >( ( rc.right - rc.left ) * 0.8f ) );
				insert_column( 1, "メモ", static_cast< int >( ( rc.right - rc.left ) * 0.15f ) );

				eh_.set( event::drop_files(), &on_drop_files );

				SetWindowSubclass( wnd_, control_procedure< result_view >::address(), 1, 0 );
			}
		}

	public:
		std::size_t size() const noexcept
		{
			return ListView_GetItemCount( wnd_ );
		}
		
		void clear() noexcept
		{
			SendMessageW( wnd_, LVM_DELETEALLITEMS, 0, 0 );
		}

		void update(std::vector< std::string > const& src)
		{
			clear();
			for( auto const& p : src ) {
				push_back( p );
			}

			data_ = src;
		}

		std::vector< std::string > selected() const noexcept
		{
			std::vector< std::string > result;

			int index = next_item( -1, LVNI_SELECTED );
			while( index != -1 ) {
				result.push_back( data_[index] );
				index = next_item( index, LVNI_SELECTED );
			}

			return result;
		}

		void all_select() noexcept
		{
			LVITEMW item = { 0 };
			item.stateMask = LVIS_SELECTED;
			item.state = LVIS_SELECTED;

			SendMessageW( wnd_, LVM_SETITEMSTATE, -1, reinterpret_cast< LPARAM >( &item ) );
		}

		inline std::vector< std::string > const& data() const noexcept
		{
			return data_;
		}

		inline HWND handle() const noexcept
		{
			return wnd_;
		}

		inline event_handler_type const& event() const noexcept
		{
			return eh_;
		}

	private:
		void insert_column(int index, boost::string_ref str, int sz) 
		{
			std::wstring wstr( multibyte_to_wide( str, CP_UTF8 ) );

			LVCOLUMNW col = { 0 };
			col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
			col.fmt = LVCFMT_LEFT;	
			col.cx = sz;
			col.pszText = &wstr[0];
			col.cchTextMax = wstr.size();

			SendMessageW( wnd_, LVM_INSERTCOLUMN, index, reinterpret_cast< LPARAM >( &col ) );
		}

		int insert(int index, boost::string_ref str) noexcept
		{
			std::wstring wstr( multibyte_to_wide( str, CP_UTF8 ) );

			LVITEMW item = { 0 };
			item.mask = LVIF_TEXT;
			item.iItem = index; 
			item.pszText = &wstr[0];
			item.cchTextMax = wstr.size();

			return static_cast< int >( SendMessageW( wnd_, LVM_INSERTITEM, 0, reinterpret_cast< LPARAM >( &item ) ) );
		}

		int push_back(boost::string_ref str) noexcept
		{
			return insert( size(), str );
		}

		int next_item(int index, UINT flags) const noexcept
		{
			return static_cast< int >( SendMessageW( 
				wnd_, LVM_GETNEXTITEM, static_cast< WPARAM >( index ), static_cast< LPARAM >( flags )
			) );
		}

		static void on_drop_files(result_view& rv, HDROP p)
		{
			auto const parent = window_table().template get< Parent >( rv.parent_ );
			if( !parent ) {
				return;
			}

			auto const files = drop_files( p );
			if( files.empty() ) {
				return;
			}

			parent->refresh( files );
		}

	public:
		static boost::shared_ptr< result_view > make(HWND hwnd, UINT id)
		{
			auto p = boost::shared_ptr< result_view >( new result_view( hwnd, id ) );
			window_table().insert( p->handle(), p );

			return p;
		}
	};

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_RESULT_VIEW_HPP_
