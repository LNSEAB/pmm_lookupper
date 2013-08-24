#ifndef PMM_LOOKUPPER_EVENT_HANDLER_HPP_
#define PMM_LOOKUPPER_EVENT_HANDLER_HPP_

#include <utility>
#include <type_traits>
#include <boost/optional.hpp>
#include <boost/function.hpp>

namespace pmm_lookupper {
	
	extern void* enabler;

namespace detail {

	template <class Widget, class... Events>
	class event_handler_impl;

	template <class Widget, class Event, class... Rest>
	class event_handler_impl< Widget, Event, Rest... > :
		public event_handler_impl< Widget, Rest... >
	{
		using base_type = event_handler_impl< Widget, Rest... >;
		using function_type = boost::function< typename Event::template type< Widget > >;

		function_type f_;

	public:
		using base_type::set_;
		using base_type::get_;
		
		template <class E, class F>
		inline void set_(F&& f, typename std::enable_if< std::is_same< E, Event >::value >::type* = nullptr)
		{
			f_ = std::forward< F >( f );
		}

		template <class E>
		inline function_type const& get_(typename std::enable_if< std::is_same< E, Event >::value >::type* = nullptr) const
		{
			return f_;
		}
	};

	template <class Widget>
	class event_handler_impl< Widget >
	{
	public:
		void set_();
		void get_() const;
	};

} // namespace detail

	template <class Widget, class Event>
	using return_type = typename boost::function< typename Event::template type< Widget > >::result_type;

	template <class Widget, class... Events>
	class event_handler :
		detail::event_handler_impl< Widget, Events... >
	{
	public:
		template <class Event>
		inline bool empty(Event) const noexcept
		{
			return this->template get_< Event >().empty();
		}

		template <class Event>
		inline void clear(Event) 
		{
			this->template get_< Event >().clear();
		}

		template <class Event, class F>
		inline void set(Event, F&& f) 
		{
			this->template set_< Event >( std::forward< F >( f ) );
		}

		template <
			class Event, class... Args, 
			typename std::enable_if< std::is_void< return_type< Widget, Event > >::value >::type*& = enabler
		>
		inline void invoke(Event, Args&&... args) const
		{
			if( empty( Event() ) ) {
				return;
			}
			this->template get_< Event >()( std::forward< Args >( args )... );
		}

		template <
			class Event, class... Args,
			typename std::enable_if< !std::is_void< return_type< Widget, Event > >::value >::type*& = enabler
		>
		inline auto invoke(Event, Args&&... args) const
			-> boost::optional< return_type< Widget, Event > >
		{
			if( empty( Event() ) ) {
				return {};
			}
			return this->template get_< Event >()( std::forward< Args >( args )... );
		}
	};

} // namespace pmm_lookupper

#endif // PMM_LOOKUPPER_EVENT_HANDLER_HPP_
