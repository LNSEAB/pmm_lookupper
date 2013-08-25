#ifndef PMM_LOOKUPPER_MAIN_WINDOW_HPP_
#define PMM_LOOKUPPER_MAIN_WINDOW_HPP_

#include "winapi.hpp"
#include "event_handler.hpp"
#include "procedure.hpp"
#include "pmm.hpp"
#include "result_view.hpp"
#include "filter.hpp"
#include <boost/spirit/include/qi.hpp>

namespace pmm_lookupper {

	class main_window
	{
	public:
		using event_handler_type = event_handler< 
			main_window, 
			event::drop_files, event::command, event::notify, event::close, event::destroy 
		>;

	private:
		HWND dlg_;
		HMENU popup_;
		boost::shared_ptr< result_view< main_window > > result_;
		event_handler_type eh_;
		std::vector< std::string > data_;

		main_window() :
			dlg_( CreateDialogW( 
				static_cast< HINSTANCE >( GetModuleHandle( nullptr ) ), 
				MAKEINTRESOURCEW( IDD_MAINWINDOW ), nullptr, 
				dialog_procedure< main_window >::address() 
			) ),
			result_( result_view< main_window >::make( dlg_, IDC_RESULT ) )
		{ 
			if( dlg_ ) {
				eh_.set( event::command(), &on_command );
				eh_.set( event::drop_files(), &on_dragfiles );
				eh_.set( event::notify(), &on_notify );
				eh_.set( event::destroy(), &on_destroy );

				popup_ = LoadMenuW( nullptr, MAKEINTRESOURCEW( IDR_POPUPMENU ) );

				set_window_text( GetDlgItem( dlg_, IDC_EXTFILTER ), "pmx, pmd, x, wav, bmp" );
			}
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

		inline std::vector< std::string > get_extensions() const
		{
			namespace qi = boost::spirit::qi;

			auto const tmp_exts = get_window_text( GetDlgItem( handle(), IDC_EXTFILTER ) );
			auto const parser = qi::as_string[+qi::alnum] % ',';
			std::vector< std::string > exts;

			qi::phrase_parse( tmp_exts.begin(), tmp_exts.end(), parser, qi::space_type(), exts );
			boost::for_each( exts, [](std::string& s){ s = std::string( "." ) + s; } );

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
				buf = remove_file_path( buf );
			}
			else {
				buf = match_extension( buf, get_extensions() );
			}
			boost::sort( buf );

			if( !IsDlgButtonChecked( handle(), IDC_DUPLICATION ) ) {
				buf.erase( std::unique( buf.begin(), buf.end() ), buf.end() );
			}

			auto rv = get_result_view();
			rv->update( buf );
		}

		void refresh(std::vector< std::string > const& files)
		{
			std::vector< std::string > errors;

			for( auto const& f : files ) {
				auto const paths = find_pmm_paths( f );
				if( paths.which() == 1 ) {
					errors.push_back( f );
				}
				else {
					data_ = boost::get< std::vector< std::string > >( paths );
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

			for( auto itr = paths.begin(); itr != paths.begin() + paths.size() - 1; ++itr ) {
				str += itr->substr( 0, itr->find( '\0' ) ) + "\r\n";
			}
			str += paths.back();

			copy_to_clipboard( handle(), str );
		}

		static void on_idc_open(main_window& wnd)
		{
			auto const result = get_open_file_name( 
				wnd.handle(), "PMMファイル (*.pmm)\n*.pmm\nすべてのファイル (*.*)\n*.*\n\n", "pmm",
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
					ofs << s.substr( 0, s.find( '\0' ) ) << std::endl;
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
