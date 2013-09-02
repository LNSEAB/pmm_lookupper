#ifndef PMM_LOOKUPPER_WINAPI_HPP_
#define PMM_LOOKUPPER_WINAPI_HPP_

#define UNICODE
#define _UNICODE

#undef WINVER
#undef _WIN32_WINNT

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#	define NOMINMAX
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlwapi.h>

#undef near
#undef far

#include <memory>
#include <vector>
#include <tuple>
#include <type_traits>
#include <boost/utility/string_ref.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/variant.hpp>
#include "resource.h"

namespace pmm_lookupper {

	extern void* enabler;

	struct locale_free_deleter
	{
		template <class T>
		inline void operator()(T p) const noexcept
		{
			LocalFree( p );
		}
	};

	template <class T>
	using local_ptr = std::unique_ptr< T, locale_free_deleter >;

	struct destroy_accelerator_deleter
	{
		inline void operator()(HACCEL p) const noexcept
		{
			DestroyAcceleratorTable( p );
		}
	};

	using accelerator_ptr = std::unique_ptr< std::remove_pointer< HACCEL >::type, destroy_accelerator_deleter >;

	inline accelerator_ptr load_accelerators(HINSTANCE hinst, UINT id) noexcept
	{
		return accelerator_ptr(
			LoadAcceleratorsW( hinst, MAKEINTRESOURCEW( id ) ),
			destroy_accelerator_deleter()
		);
	}

	inline std::wstring multibyte_to_wide(boost::string_ref src, UINT code)
	{
		int const sz = MultiByteToWideChar( code, 0, src.data(), src.size(), nullptr, 0 );
		if( sz == 0 ) {
			return {};
		}

		std::wstring dest( sz, L'\0' );
		MultiByteToWideChar( code, 0, src.data(), src.size(), &dest[0], dest.size() );

		return dest;
	}
	
	inline std::string wide_to_multibyte(boost::wstring_ref src, UINT code)
	{
		int const sz = WideCharToMultiByte( code, 0, src.data(), src.size(), nullptr, 0, nullptr, 0 );
		if( sz == 0 ) {
			return {};
		}

		std::string dest( sz, L'\0' );
		WideCharToMultiByte( code, 0, src.data(), src.size(), &dest[0], dest.size(), nullptr, 0 );

		return dest;
	}

	inline std::string convert_code(boost::string_ref str, UINT src, UINT dest)
	{
		std::wstring const tmp( multibyte_to_wide( str, src ) );
		return wide_to_multibyte( tmp, dest );
	}

	inline std::vector< std::string > get_command_line()
	{
		int sz;
		local_ptr< LPWSTR > argv( CommandLineToArgvW( GetCommandLineW(), &sz ), locale_free_deleter() );
		if( !argv ) {
			return {};
		}

		std::vector< std::string > result;
		for( int i = 0; i < sz; ++i ) {
			result.emplace_back( wide_to_multibyte( argv.get()[i], CP_UTF8 ) );
		}

		return result;
	}

	inline int message_box(boost::string_ref caption, boost::string_ref msg, UINT opts, HWND hwnd = nullptr) noexcept
	{
		return MessageBoxW( 
			hwnd, 
			multibyte_to_wide( msg, CP_UTF8 ).c_str(), 
			multibyte_to_wide( caption, CP_UTF8 ).c_str(), 
			opts 
		);
	}

	inline std::vector< std::string > drop_files(HDROP p)
	{
		UINT const cnt = DragQueryFileW( p, -1, nullptr, 0 );

		std::vector< std::string > result;
		result.reserve( cnt );

		for( UINT i = 0; i < cnt; ++i ) {
			UINT const sz = DragQueryFileW( p, i, nullptr, 0 ) + 1;

			std::wstring str( sz, L'\0' );
			DragQueryFileW( p, i, &str[0], str.size() );
			str.pop_back();

			auto const r = wide_to_multibyte( str, CP_UTF8 );
			result.emplace_back( r );
		}

		return result;
	}

	inline void set_window_text(HWND hwnd, boost::string_ref str)
	{
		SetWindowTextW( hwnd, multibyte_to_wide( str, CP_UTF8 ).c_str() );
	}

	inline std::string get_window_text(HWND hwnd)
	{
		std::wstring str( GetWindowTextLength( hwnd ) + 1, L'\0' );
		GetWindowText( hwnd, &str[0], str.size() );

		return wide_to_multibyte( str, CP_UTF8 );
	}

	inline boost::variant< std::vector< std::string >, DWORD > get_open_file_name(
		HWND hwnd, boost::string_ref filter, boost::string_ref ext, UINT flags, std::size_t buf_size = 4096
	) {
		std::vector< wchar_t > buf( buf_size );
		std::wstring const filter_w = multibyte_to_wide( filter, CP_UTF8 );
		std::wstring const ext_w = multibyte_to_wide( ext, CP_UTF8 );

		std::vector< wchar_t > filter_buf( filter_w.size() );
		boost::transform( filter_w, filter_buf.begin(), [](wchar_t const& c) -> wchar_t { return c == L'\n' ? L'\0' : c; } );

		OPENFILENAMEW ofn = { 0 };
		ofn.lStructSize = sizeof( OPENFILENAMEW );
		ofn.hwndOwner = hwnd;
		ofn.hInstance = static_cast< HINSTANCE >( GetModuleHandle( nullptr ) );
		ofn.lpstrFilter = &filter_buf[0];
		ofn.lpstrFile = &buf[0];
		ofn.nMaxFile = buf.size();
		ofn.Flags = flags;
		ofn.lpstrDefExt = ext_w.c_str();

		std::vector< std::string > result;

		if( !GetOpenFileNameW( &ofn ) ) {
			return { CommDlgExtendedError() };
		}

		std::wstring first_str( buf.begin(), boost::find( buf, L'\0' ) );
		if( PathIsDirectoryW( first_str.c_str() ) ) {
			for( auto itr = boost::find( buf, L'\0' ) + 1; itr != buf.end() && *itr != L'\0'; ++itr ) {
				auto i = std::find( itr, buf.end(), L'\0' );
				result.emplace_back( wide_to_multibyte( first_str + L'\\' + std::wstring( itr, i ), CP_UTF8 ) );
				itr = i;
			}
		}
		else {
			result.emplace_back( wide_to_multibyte( first_str, CP_UTF8 ) );
		}

		return { result };
	}

	inline boost::variant< std::string, DWORD > get_save_file_name(
		HWND hwnd, boost::string_ref filter, boost::string_ref ext, UINT flags, std::size_t buf_size = 1024
	) {
		std::vector< wchar_t > buf( buf_size );
		std::wstring const filter_w = multibyte_to_wide( filter, CP_UTF8 );
		std::wstring const ext_w = multibyte_to_wide( ext, CP_UTF8 );

		std::vector< wchar_t > filter_buf( filter_w.size() );
		boost::transform( filter_w, filter_buf.begin(), [](wchar_t const& c) -> wchar_t { return c == L'\n' ? L'\0' : c; } );

		OPENFILENAMEW ofn = { 0 };
		ofn.lStructSize = sizeof( OPENFILENAMEW );
		ofn.hwndOwner = hwnd;
		ofn.hInstance = static_cast< HINSTANCE >( GetModuleHandle( nullptr ) );
		ofn.lpstrFilter = &filter_buf[0];
		ofn.lpstrFile = &buf[0];
		ofn.nMaxFile = buf.size();
		ofn.Flags = flags;
		ofn.lpstrDefExt = ext_w.c_str();

		if( !GetSaveFileNameW( &ofn ) ) {
			return { CommDlgExtendedError() };
		}

		return { wide_to_multibyte( std::wstring( buf.begin(), boost::find( buf, L'\0' ) ), CP_UTF8 ) };
	}

	inline void explorer_execute(HWND hwnd, boost::string_ref path)
	{
		std::wstring path_w( multibyte_to_wide( path, CP_UTF8 ) );
		ShellExecuteW( hwnd, L"explore", path_w.c_str(), nullptr, nullptr, SW_SHOWNORMAL );
	}

	inline void copy_to_clipboard(HWND hwnd, boost::string_ref str)
	{
		std::string s( convert_code( str, CP_UTF8, CP_OEMCP ) );
		s += '\0';

		HGLOBAL buf = GlobalAlloc( GHND | GMEM_SHARE, s.size() );
		if( !buf ) {
			return;
		}

		auto p = static_cast< char* >( GlobalLock( buf ) );
		if( !p ) {
			GlobalFree( buf );
			return;
		}
		boost::copy( s, p );
		GlobalUnlock( buf );

		if( !OpenClipboard( hwnd ) ) {
			GlobalFree( buf );
			return;
		}

		EmptyClipboard();
		SetClipboardData( CF_TEXT, buf );
		CloseClipboard();
	}

	template <class WP, class LP, typename std::enable_if< std::is_integral< LP >::value >::type*& = enabler>
	inline LRESULT send_message(HWND hwnd, UINT msg, WP wparam, LP lparam) noexcept
	{
		return SendMessageW( hwnd, msg, wparam, lparam );
	}

	template <class WP, class LP, typename std::enable_if< std::is_pointer< LP >::value >::type*& = enabler>
	inline LRESULT send_message(HWND hwnd, UINT msg, WP wparam, LP lparam) noexcept
	{
		return SendMessageW( hwnd, msg, wparam, reinterpret_cast< LPARAM >( lparam ) );
	}

	inline void cb_add_string(HWND hwnd, boost::string_ref str) noexcept
	{
		auto const tmp = multibyte_to_wide( str, CP_UTF8 );
		send_message( hwnd, CB_ADDSTRING, 0, tmp.c_str() );
	}

	inline int cb_get_cursel(HWND hwnd) noexcept
	{
		return send_message( hwnd, CB_GETCURSEL, 0, 0 );
	}

	inline void cb_set_cursel(HWND hwnd, int index) noexcept
	{
		send_message( hwnd, CB_SETCURSEL, index, 0 ); 
	}

	inline void set_shared_icon(HWND hwnd, UINT icon_type, UINT id) noexcept
	{
		send_message( 
			hwnd, WM_SETICON, icon_type, 
			LoadImageW( GetModuleHandle( nullptr ), MAKEINTRESOURCEW( id ), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED ) 
		);
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_WINAPI_HPP_
