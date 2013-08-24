#ifndef PMM_LOOKUPPER_MAIN_WINDOW_HPP_
#define PMM_LOOKUPPER_MAIN_WINDOW_HPP_

#include "winapi.hpp"
#include "event_handler.hpp"
#include "procedure.hpp"
#include "pmm.hpp"
#include "result_view.hpp"
#include <boost/spirit/include/qi.hpp>

namespace pmm_lookupper {

	class main_window
	{
	public:
		using event_handler_type = event_handler< 
			main_window, 
			event::drop_files, event::command, event::close, event::destroy 
		>;

	private:
		HWND dlg_;
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
				eh_.set( event::destroy(), &on_destroy );
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

		inline explicit operator bool() const noexcept
		{
			return dlg_ && IsWindow( dlg_ );
		}

		void update()
		{
			auto buf = get_data();

			if( IsDlgButtonChecked( handle(), IDC_FOLDER_ONLY ) ) {
				buf = filter_folder( buf );
			}
			else {
				buf = filter_exts( buf );
			}
			boost::sort( buf );

			if( IsDlgButtonChecked( handle(), IDC_DUPLICATION ) ) {
				buf.erase( std::unique( buf.begin(), buf.end() ), buf.end() );
			}

			auto rv = get_result_view();
			rv->update( buf );
		}

		void refresh(std::vector< std::string > const& files)
		{
			for( auto const& f : files ) {
				auto const paths = find_pmm_paths( f );
				if( paths.which() == 1 ) {
					message_box( "エラー", boost::get< std::string >( paths ), MB_OK | MB_ICONWARNING );
				}
				else {
					stock_data( boost::get< std::vector< std::string > >( paths ) );
				}
			}

			update();
		}

	private:
		inline void stock_data(std::vector< std::string > const& v)
		{
			data_ = v;
		}

		inline std::vector< std::string > const& get_data() const noexcept
		{
			return data_;
		}

		std::vector< std::string > filter_folder(std::vector< std::string > const& buf) const
		{
			std::vector< std::string > result;

			for( auto const& str : buf ) {
				auto const p = str.find_last_of( "\\" );
				result.emplace_back( str.substr( 0, p ) );
			}

			return result;
		}

		std::vector< std::string > filter_exts(std::vector< std::string > const& buf) const
		{
			namespace qi = boost::spirit::qi;

			auto const tmp_exts = get_window_text( GetDlgItem( handle(), IDC_EXTFILTER ) );
			auto const parser = qi::as_string[+qi::alnum] % ',';

			std::vector< std::string > exts;
			qi::phrase_parse( tmp_exts.begin(), tmp_exts.end(), parser, qi::space_type(), exts );
			boost::for_each( exts, [](std::string& s){ s = std::string( "." ) + s; } );

			std::vector< std::string > result;
			for( auto const& path : buf ) {
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

		static void on_idc_explorer(main_window& wnd)
		{
			auto paths = wnd.get_result_view()->selected();
			paths = wnd.filter_folder( paths );
			paths.erase( std::unique( paths.begin(), paths.end() ), paths.end() );
			
			if( paths.size() >= 3 ) {
				auto const result = message_box( 
					"確認", "ウィンドウを3つ以上同時に開こうとしていますが、開きますか？",  
					MB_OKCANCEL | MB_ICONINFORMATION, wnd.handle()
				);
				if( result != IDOK ) {
					return;
				}
			}

			for( auto const& path : paths ) {
				explorer_execute( wnd.handle(), path );
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

			case IDC_EXPLORER :
				on_idc_explorer( wnd );
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

		static void on_destroy(main_window&) noexcept
		{
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
