#include <stdexcept>
#include <locale>
#include <clocale>
#include "winapi.hpp"
#include "main_window.hpp"

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int cmdshow)
{
	try {
		InitCommonControls();

		auto wnd = pmm_lookupper::main_window::make();
		if( !wnd ) {
			throw std::runtime_error( "ウィンドウを生成できませんでした" );
		}

		HACCEL accel = LoadAccelerators( hinst, MAKEINTRESOURCEW( IDR_ACCELERATORS ) );

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

			if( !TranslateAccelerator( wnd->handle(), accel, &msg ) ) {
				TranslateMessage( &msg );
				DispatchMessageW( &msg );
			}
		}

		DestroyAcceleratorTable( accel );
	}
	catch( std::exception const& e ) {
		pmm_lookupper::message_box( "エラー", e.what(), MB_OK | MB_ICONWARNING );
	}

	return 0;
}
