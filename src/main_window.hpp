#ifndef PMM_LOOKUPPER_MAIN_WINDOW_HPP_
#define PMM_LOOKUPPER_MAIN_WINDOW_HPP_

#include "winapi.hpp"
#include "event_handler.hpp"
#include "procedure.hpp"
#include "pmm.hpp"
#include "result_view.hpp"
#include "filter.hpp"
#include "emm.hpp"
#include "controls.hpp"
#include <array>
#include <boost/spirit/include/qi.hpp>

namespace pmm_lookupper {
	
	class main_window
	{
	public:
		using event_handler_type = event_handler< 
			main_window, 
			event::drop_files, event::command, event::notify, event::sizing, event::close, event::destroy 
		>;

	private:
		HWND dlg_;
		HMENU popup_;
		boost::shared_ptr< result_view< main_window > > result_;
		event_handler_type eh_;
		std::vector< std::string > data_;
		RECT rv_offset_;
		std::array< POINT, controls::size > opt_offsets_;

		main_window() :
			dlg_( CreateDialogW( 
				static_cast< HINSTANCE >( GetModuleHandle( nullptr ) ), 
				MAKEINTRESOURCEW( IDD_MAINWINDOW ), nullptr, 
				dialog_procedure< main_window >::address() 
			) ),
			result_( result_view< main_window >::make( dlg_, IDC_RESULT ) )
		{ 
			if( !dlg_ || !result_ ) {
				throw std::runtime_error( "ウィンドウを生成できませんでした" );
			}

			set_shared_icon( dlg_, ICON_BIG, IDI_LARGE );
			set_shared_icon( dlg_, ICON_SMALL, IDI_SMALL );

			eh_.set( event::command(), &on_command );
			eh_.set( event::drop_files(), &on_dragfiles );
			eh_.set( event::notify(), &on_notify );
			eh_.set( event::sizing(), &on_sizing );
			eh_.set( event::destroy(), &on_destroy );

			popup_ = LoadMenuW( nullptr, MAKEINTRESOURCEW( IDR_POPUPMENU ) );

			set_window_text( GetDlgItem( dlg_, IDC_EXTFILTER ), "pmx pmd x wav bmp fx fxsub" );
			cb_add_string( GetDlgItem( dlg_, IDC_SORT_COND ), "ファイルパス" );
			cb_add_string( GetDlgItem( dlg_, IDC_SORT_COND ), "拡張子" );
			cb_set_cursel( GetDlgItem( dlg_, IDC_SORT_COND ), 0 );

			rv_offset_ = result_view_offset();
			opt_offsets_ = option_offsets();
		}

	public:
		inline HWND handle() const noexcept
		{
			return dlg_;
		}

		inline event_handler_type const& event() const noexcept
		{
			return eh_;
		}

		inline boost::shared_ptr< result_view< main_window > > get_result_view() const noexcept
		{
			return result_;
		}

		inline std::vector< std::string > get_extensions_filter() const
		{
			namespace qi = boost::spirit::qi;
			
			std::vector< std::string > exts;

			auto const tmp_exts = get_window_text( GetDlgItem( handle(), IDC_EXTFILTER ) );
			auto const parser = boost::proto::deep_copy(
				qi::as< std::vector< std::string > >()[ +qi::alnum % qi::omit[+qi::lit( ' ' )] ]
			);
			qi::parse( tmp_exts.begin(), tmp_exts.end(), parser, exts );

			for( auto& ext : exts ) {
				ext = std::string( "." ) + ext;
			}

			return exts;
		}

		inline explicit operator bool() const noexcept
		{
			return dlg_ && IsWindow( dlg_ );
		}

		void update()
		{
			auto buf = data_;

			if( IsDlgButtonChecked( handle(), IDC_FOLDER_ONLY ) ) {
				EnableWindow( GetDlgItem( dlg_, IDC_SORT_COND ), FALSE );
				buf = remove_file_path( buf );
			}
			else {
				EnableWindow( GetDlgItem( dlg_, IDC_SORT_COND ), TRUE );
				buf = match_extension( buf, get_extensions_filter() );
			}

			auto const sort_cond_index = cb_get_cursel( GetDlgItem( dlg_, IDC_SORT_COND ) );
			if( IsWindowEnabled( GetDlgItem( dlg_, IDC_SORT_COND ) ) && sort_cond_index == 1 ) {
				boost::sort( buf );
				boost::stable_sort( buf, [](std::string const& lhs, std::string const& rhs) -> bool {
					return boost::lexicographical_compare( get_extension( lhs ), get_extension( rhs ) );
				} );
			}
			else {
				boost::sort( buf );
			}

			if( !IsDlgButtonChecked( handle(), IDC_DUPLICATION ) ) {
				buf.erase( std::unique( buf.begin(), buf.end() ), buf.end() );
			}

			auto rv = get_result_view();
			rv->update( buf );
		}

		template <class F>
		static void append_data(
			std::string const& file, F f, std::vector< std::string >& data 
		) {
			auto const paths = f( file );
			if( !paths.empty() ) {
				for( auto const& p : paths ) {
					data.push_back( p );
				}
			}
		}

		template <class F>
		static void append_data(
			std::string const& file, F f, std::vector< std::string >& data, std::vector< std::string >& errors
		) {
			auto const paths = f( file );
			if( paths.empty() ) {
				errors.push_back( file );
			}
			else {
				for( auto const& p : paths ) {
					data.push_back( p );
				}
			}
		}

		void refresh(std::vector< std::string > const& files)
		{
			std::vector< std::string > errors;

			data_.clear();

			for( auto const& f : files ) {
				auto const ext = get_extension( f );

				if( ext == ".pmm" ) {
					append_data( f, &pmm_contain_file_paths, data_, errors );
					auto const emm_path = f.substr( 0, f.find_last_of( '.' ) ) + ".emm";
					append_data( emm_path, &emm_contain_file_paths, data_ );
				}
				else if( ext == ".emm" ) {
					if( is_emm_file( f ) ) {
						append_data( f, &emm_contain_file_paths, data_ );
					}
					else {
						errors.push_back( f );
					}
				}
				else {
					errors.push_back( f );
				}
			}

			if( !errors.empty() ) {
				std::string str( "読み込めないファイルがありました。\r\n" );

				for( auto const& s : errors ) {
					str += s + "\r\n";
				}

				message_box( "エラー", str, MB_OK | MB_ICONWARNING );
			}

			update();
		}

	private:
		inline RECT result_view_offset() const noexcept
		{
			RECT rc;
			GetWindowRect( dlg_, &rc );

			RECT rv_rc;
			GetWindowRect( result_->handle(), &rv_rc );

			return {
				rv_rc.left - rc.left,
				rv_rc.top - rc.top,
				rc.right - rv_rc.right,
				rc.bottom - rv_rc.bottom
			};
		}

		inline decltype( opt_offsets_ ) option_offsets() const noexcept
		{
			RECT rc;
			GetClientRect( dlg_, &rc );

			decltype( opt_offsets_ ) dest;
			for( std::size_t i = 0; i < dest.size(); ++i ) {
				RECT opt_rc;
				GetWindowRect( GetDlgItem( dlg_, num_to_id( i ) ), &opt_rc );

				POINT pt = { opt_rc.left, opt_rc.top };
				ScreenToClient( dlg_, &pt );

				dest[i].x = rc.right - pt.x;
				dest[i].y = pt.y;
			}

			return dest;
		}

		void show_explorer() const
		{
			auto paths = get_result_view()->selected();
			paths = remove_file_path( paths );
			paths.erase( std::unique( paths.begin(), paths.end() ), paths.end() );
			
			if( paths.size() >= 3 ) {
				auto const result = message_box( 
					"確認", "ウィンドウを3つ以上同時に開こうとしていますが、開きますか？",  
					MB_OKCANCEL | MB_ICONINFORMATION, handle()
				);
				if( result != IDOK ) {
					return;
				}
			}

			for( auto const& path : paths ) {
				explorer_execute( handle(), path );
			}
		}

		void on_copy() const
		{
			auto paths = get_result_view()->selected();
			if( paths.empty() ) {
				return;
			}

			std::string str;

			for( auto itr = paths.begin(); itr != paths.begin() + ( paths.size() - 1 ); ++itr ) {
				str += *itr + "\r\n";
			}
			str += paths.back();

			copy_to_clipboard( handle(), str );
		}

		static void on_idc_open(main_window& wnd)
		{
			auto const result = get_open_file_name( 
				wnd.handle(), "PMMファイル (*.pmm)\n*.pmm\nEMMファイル (*.emm)\n*.emm\nすべてのファイル (*.*)\n*.*\n\n", "pmm",
				OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY 
			);
			if( result.which() == 0 ) {
				wnd.refresh( boost::get< std::vector< std::string > >( result ) );
			}
		}

		static void on_idc_save(main_window& wnd)
		{
			auto const result = get_save_file_name(
				wnd.handle(), "テキストファイル (*.txt)\n*.txt\nすべてのファイル (*.*)\n*.*\n\n", "txt",
				OFN_EXPLORER | OFN_OVERWRITEPROMPT
			);
			if( result.which() == 0 ) {
				std::ofstream ofs( convert_code( boost::get< std::string >( result ), CP_UTF8, CP_OEMCP ) );
				for( auto const& str : wnd.get_result_view()->data() ) {
					auto const s = convert_code( str, CP_UTF8, CP_OEMCP );
					ofs << s << std::endl;
				}
			}
		}

		static void on_command(main_window& wnd, UINT id, WPARAM wparam, LPARAM)
		{
			switch( id ) {
			case IDM_OPEN :
				on_idc_open( wnd );
				break;
				
			case IDM_SAVE :
				on_idc_save( wnd );
				break;

			case IDM_QUIT :
				DestroyWindow( wnd.handle() );
				break;

			case IDM_VERSION :
				DialogBoxW(
					static_cast< HINSTANCE >( GetModuleHandle( nullptr ) ), MAKEINTRESOURCEW( IDD_VERSION ),
					wnd.handle(), version_procedure::address()
				);
				break;

			case IDC_EXTFILTER :
				if( HIWORD( wparam ) == EN_CHANGE ) {
					wnd.update();
				}
				break;

			case IDC_DUPLICATION :
				wnd.update();
				break;

			case IDC_FOLDER_ONLY :
				wnd.update();
				break;

			case IDM_COPY :
			case IDM_POPUP_COPY :
				wnd.on_copy();
				break;

			case IDM_ALLSELECT :
				wnd.get_result_view()->all_select();
				break;

			case IDM_EXPLORER :
				wnd.show_explorer();
				break;

			case IDC_SORT_COND :
				if( HIWORD( wparam ) == CBN_SELENDOK ) {
					wnd.update();
				}
				break;
			}
		}

		static void on_dragfiles(main_window& wnd, HDROP p)
		{
			auto const files = drop_files( p );
			if( files.empty() ) {
				return;
			}

			wnd.refresh( files );
		}

		static void result_notify(main_window& wnd, NMHDR const* hdr)
		{
			if( hdr->code == NM_RCLICK ) {
				POINT pt;
				GetCursorPos( &pt );
				TrackPopupMenu( GetSubMenu( wnd.popup_, 0 ), TPM_LEFTALIGN, pt.x, pt.y, 0, wnd.handle(), nullptr );
			}
		}

		static void on_notify(main_window& wnd, UINT id, NMHDR const* hdr)
		{
			switch( id ) {
			case IDC_RESULT :
				result_notify( wnd, hdr );
				break;
			}
		}

		static void on_sizing(main_window& wnd, UINT, RECT const& rc)
		{
			auto const rv_x = rc.right - rc.left - wnd.rv_offset_.right - wnd.rv_offset_.left; 
			auto const rv_y = rc.bottom - rc.top - wnd.rv_offset_.bottom - wnd.rv_offset_.top;
			SetWindowPos( 
				wnd.result_->handle(), nullptr,
				0, 0, rv_x, rv_y,	
				SWP_NOZORDER | SWP_NOMOVE
			);

			for( std::size_t i = 0; i < controls::size; ++i ) {
				SetWindowPos( 
					GetDlgItem( wnd.handle(), num_to_id( i ) ), nullptr,
					rc.right - rc.left - wnd.opt_offsets_[i].x - GetSystemMetrics( SM_CXSIZEFRAME ) * 2, 
					wnd.opt_offsets_[i].y, 
					0, 0,
					SWP_NOZORDER | SWP_NOSIZE 
				);
			}

			wnd.get_result_view()->set_column_size( 0, static_cast< int >( rv_x * 0.97f ) );
		}

		static void on_destroy(main_window& wnd) noexcept
		{
			if( wnd.popup_ ) {
				DestroyMenu( wnd.popup_ );
			}
			PostQuitMessage( 0 );
		}

	public:
		static boost::shared_ptr< main_window > make()
		{
			auto p = boost::shared_ptr< main_window >( new main_window );
			window_table().insert( p->handle(), p );

			return p;
		}
	};

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_MAIN_WINDOW_HPP_
