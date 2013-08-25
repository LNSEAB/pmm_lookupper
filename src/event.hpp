#ifndef PMM_LOOKUPPER_EVENT_HPP_
#define PMM_LOOKUPPER_EVENT_HPP_

#include "winapi.hpp"
#include <functional>

namespace pmm_lookupper { namespace event {

	struct drop_files
	{
		template <class Widget>
		using type = void (Widget&, HDROP);
	};

	struct command
	{
		template <class Widget>
		using type = void (Widget&, UINT, WPARAM, LPARAM);
	};

	struct notify
	{
		template <class Widget>
		using type = void (Widget&, UINT, NMHDR const*);
	};

	struct close
	{
		template <class Widget>
		using type = bool (Widget&);
	};

	struct destroy
	{
		template <class Widget>
		using type = void (Widget&);
	};

} } // namespace pmm_lookupper::event

#endif // PMM_LOOKUPPER_EVENT_HPP_
