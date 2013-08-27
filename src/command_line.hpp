#ifndef PMM_LOOKUPPER_COMMAND_LINE_HPP_
#define PMM_LOOKUPPER_COMMAND_LINE_HPP_

#include "winapi.hpp"
#include "main_window.hpp"

namespace pmm_lookupper {

	inline void parse_command_line(main_window& wnd)
	{
		auto const argv = get_command_line();
		std::vector< std::string > files;

		for( std::size_t i = 1; i < argv.size(); ++i ) {
			if( argv[i].find( "-d" ) != argv[i].npos ) {
				CheckDlgButton( wnd.handle(), IDC_DUPLICATION, BST_CHECKED );
			}
			else if( argv[i].find( "-f" ) != argv[i].npos ) {
				CheckDlgButton( wnd.handle(), IDC_FOLDER_ONLY, BST_CHECKED );
			}
			else if( argv[i].find( "-e" ) != argv[i].npos ) {
				if( i + 1 == argv.size() ) {
					break;
				}
				set_window_text( GetDlgItem( wnd.handle(), IDC_EXTFILTER ), argv[i + 1] );
				++i;
			}
			else {
				files.push_back( argv[i] );
			}
		}

		wnd.refresh( files );
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_COMMAND_LINE_HPP_
