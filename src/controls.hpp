#ifndef PMM_LOOKUPPER_CONTROLS_HPP_
#define PMM_LOOKUPPER_CONTROLS_HPP_

#include "resource.h"

namespace pmm_lookupper {

	template <unsigned int I>
	struct control_id
	{ 
		static constexpr auto value = I;
	};

	template <class... Controls>
	struct controls_holder
	{ 
		static constexpr auto size = sizeof...( Controls );
	};

	using controls = controls_holder<
		control_id< IDC_STATIC_EXT >,
		control_id< IDC_EXTFILTER >,
		control_id< IDC_DUPLICATION >,
		control_id< IDC_FOLDER_ONLY >,
		control_id< IDC_SORT_COND >,
		control_id< IDC_STATIC_SORT_COND >
	>;

namespace detail {

	inline constexpr unsigned int id_to_num_impl(unsigned int, unsigned int, controls_holder<>) noexcept
	{
		return 0xffffffff;
	}

	template <class IDType, class... Rest>
	inline constexpr unsigned int id_to_num_impl(unsigned int id, unsigned int i, controls_holder< IDType, Rest... >) noexcept
	{
		return id == IDType::value ? i : id_to_num_impl( id, i + 1, controls_holder< Rest... >() );
	}

	inline constexpr unsigned int num_to_id_impl(unsigned int, unsigned int, controls_holder<>) noexcept
	{
		return 0xffffffff;
	}

	template <class IDType, class... Rest>
	inline constexpr unsigned int num_to_id_impl(unsigned int index, unsigned int i, controls_holder< IDType, Rest... >) noexcept
	{
		return index == i ? IDType::value : num_to_id_impl( index, i + 1, controls_holder< Rest... >() ); 
	}

} // namespace detail

	inline constexpr unsigned int id_to_num(unsigned int id) noexcept
	{
		return detail::id_to_num_impl( id, 0, controls() );
	}

	inline constexpr unsigned int num_to_id(unsigned int index) noexcept
	{
		return detail::num_to_id_impl( index, 0, controls() );
	}

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_CONTROLS_HPP_
