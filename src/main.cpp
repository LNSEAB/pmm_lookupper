#include <stdexcept>
#include <locale>
#include <clocale>
#include "winapi.hpp"
#include "main_window.hpp"
#include "command_line.hpp"

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int cmdshow)
{
	try {
		InitCommonControls();

		auto wnd = pmm_lookupper::main_window::make();
		if( !wnd ) {
			throw std::runtime_error( "ウィンドウを生成できませんでした" );
		}

		auto const accel = pmm_lookupper::load_accelerators( hinst, IDR_ACCELERATORS );
		pmm_lookupper::parse_command_line( *wnd );

		ShowWindow( wnd->handle(), cmdshow );

		MSG msg;
		for(;;) {
			auto const result = GetMessageW( &msg, nullptr, 0, 0 );
			if( result == 0 ) {
				break;
			}
			if( result == -1 ) {
				throw std::runtime_error( "GetMessageエラー" );
			}

			if( !TranslateAccelerator( wnd->handle(), accel.get(), &msg ) ) {
				TranslateMessage( &msg );
				DispatchMessageW( &msg );
			}
		}
	}
	catch( std::exception const& e ) {
		pmm_lookupper::message_box( "エラー", e.what(), MB_OK | MB_ICONWARNING );
	}

	return 0;
}
