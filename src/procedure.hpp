#ifndef PMM_LOOKUPPER_PROCEDURE_HPP_
#define PMM_LOOKUPPER_PROCEDURE_HPP_

#include "winapi.hpp"
#include "event.hpp"
#include "window_table.hpp"

namespace pmm_lookupper {

	template <class T>
	class dialog_procedure
	{
	public:
		inline static DLGPROC address() noexcept
		{ 
			return &proc_impl;
		}

	private:
		static INT_PTR CALLBACK proc_impl(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
		{
			auto obj = window_table().template get< T >( hwnd );
			if( !obj ) {
				return FALSE;
			}

			switch( msg ) {
			case WM_COMMAND :
				obj->event().invoke( event::command(), *obj, LOWORD( wparam ), wparam, lparam );
				return TRUE;

			case WM_DROPFILES :
				obj->event().invoke( event::drop_files(), *obj, reinterpret_cast< HDROP >( wparam ) );
				return TRUE;

			case WM_NOTIFY:
				obj->event().invoke( event::notify(), *obj, wparam, reinterpret_cast< NMHDR const* >( lparam ) );
				return TRUE;

			case WM_SIZING :
				obj->event().invoke( event::sizing(), *obj, wparam, *reinterpret_cast< RECT const* >( lparam ) );
				return FALSE;

			case WM_CLOSE :
				if( obj->event().empty( event::close() ) || *( obj->event().invoke( event::close(), *obj ) ) ) {
					DestroyWindow( hwnd );
				}
				return TRUE;

			case WM_DESTROY :
				obj->event().invoke( event::destroy(), *obj );
				window_table().erase( hwnd );
				return TRUE;
			}

			return FALSE;
		}
	};

	template <class T>
	class control_procedure
	{
	public:
		inline static SUBCLASSPROC address() noexcept
		{
			return &proc_impl;
		}

	private:
		static LRESULT CALLBACK proc_impl(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR)
		{
			auto obj = window_table().template get< T >( hwnd );
			if( !obj ) {
				return FALSE;
			}

			switch( msg ) {
			case WM_DROPFILES :
				obj->event().invoke( event::drop_files(), *obj, reinterpret_cast< HDROP >( wparam ) );
				break;

			case WM_DESTROY:
				obj->event().invoke( event::destroy(), *obj );
				break;
			}

			return DefSubclassProc( hwnd, msg, wparam, lparam );
		}
	};

	class version_procedure
	{
	public:
		inline static DLGPROC address() noexcept
		{
			return &proc_impl;
		}

	private:
		static INT_PTR CALLBACK proc_impl(HWND hwnd, UINT msg, WPARAM wparam, LPARAM)
		{
			switch( msg ) {
			case WM_COMMAND :
				if( LOWORD( wparam ) == IDOK ) {
					EndDialog( hwnd, 0 );
				}
				return TRUE;

			case WM_CLOSE :
				EndDialog( hwnd, 0 );
				return TRUE;
			}

			return FALSE;
		}
	};

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_PROCEDURE_HPP_
