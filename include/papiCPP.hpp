#ifndef PAPICPP
#define PAPICPP

extern "C"
{
#include <papi.h>
}

#include <array>
#include <string>
#include <cstddef>
#include <iostream>

namespace papi
{

        using event_code = int;
        using papi_counter = long long;

        inline std::string get_event_code_name(event_code code)
        {
                std::array<char, PAPI_MAX_STR_LEN> event_name;
                ::PAPI_event_code_to_name(code, event_name.data());

                return event_name.data();
        }

        template <event_code _Event>
        struct event
        {
                explicit event(papi_counter counter = papi_counter{0})
                        : _counter{counter}
                {

                }

                papi_counter counter() const { return _counter; }

                static constexpr event_code code() { return _Event; }
                static const std::string& name() { return s_name; }

        private:
                static const std::string s_name;
                papi_counter _counter;
        };

        template <event_code _Event>
        const std::string event<_Event>::s_name = get_event_code_name(_Event);

        template <typename _Stream, event_code _Event>
        inline _Stream& operator<<(_Stream& strm, const event<_Event>& evt)
        {
                strm << evt.name() << "=" << evt.counter();
                return strm;
        }

        template <event_code... _Events>
        struct event_set
        {
                explicit event_set()
                {
                        int ret{};
                        if ((ret = ::PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
                                throw std::runtime_error(
                                        std::string("Papi library failed to init with error: ")
                                        + ::PAPI_strerror(ret)
                                );
                        }

                        _eventset = PAPI_NULL;
                        if ((ret = ::PAPI_create_eventset(&_eventset)) != PAPI_OK) {
                                throw std::runtime_error(
                                        std::string("Papi failed to create eventset: ")
                                        + ::PAPI_strerror(ret)
                                );
                        }

                        add_events();
                }

                ~event_set()
                {
                        ::PAPI_cleanup_eventset(_eventset);
                        ::PAPI_destroy_eventset(&_eventset);
                }

                void start_counters()
                {
                        int ret{};

                        if ((ret = ::PAPI_start(_eventset)) != PAPI_OK) {
                                throw std::runtime_error(
                                        std::string("Papi failed to start counters: ")
                                        + ::PAPI_strerror(ret)
                                );
                        }
                }

                void reset_counters()
                {
                        int ret{};
                        if ((ret = ::PAPI_reset(_eventset)) != PAPI_OK) {
                                throw std::runtime_error(
                                        std::string("Papi failed to reset counters: ")
                                        + ::PAPI_strerror(ret)
                                );
                        }
                }

                void stop_counters()
                {
                        int ret{};
                        if ((ret = ::PAPI_stop(_eventset, _counters.data())) != PAPI_OK) {
                                throw std::runtime_error(
                                        std::string("Papi failed to stop counters: ")
                                        + ::PAPI_strerror(ret)
                                );
                        }
                }

                static constexpr std::size_t size() { return sizeof...(_Events); }

                template <std::size_t _EventIndex>
                auto at() const {
                        static constexpr const std::array<event_code, sizeof...(_Events)> events = {{_Events...}};
                        constexpr event_code code = events[_EventIndex];
                        return event<code>(_counters[_EventIndex]);

                }

                template <event_code _EventCode>
                auto get() const
                {
                        static constexpr const std::array<event_code, sizeof...(_Events)> events = {{_Events...}};

                        constexpr int eventIndex = find(_EventCode, events, sizeof...(_Events), 0);
                        static_assert(eventIndex != -1, "Eventcode not present in this event_set");
                        return at<eventIndex>();
                }

        private:
                template <typename ArrayT>
                static constexpr int find(event_code x, ArrayT& ar, std::size_t size, std::size_t i)
                {
                        return size == i ? -1 : (ar[i] == x ? i : find(x, ar, size, i+1));
                }

                void add_events()
                {
                        int ret{};
                        static constexpr const std::array<event_code, sizeof...(_Events)> events = {{_Events...}};
                        for (event_code event : events) {
                                if ((ret = ::PAPI_add_event(_eventset, event)) != PAPI_OK) {
                                        throw std::runtime_error(
                                                std::string("Papi failed to add event ")
						+ get_event_code_name(event)
						+ std::string(" to event set: ")
                                                + ::PAPI_strerror(ret)
                                        );
                                }
                        }
                }

                std::array<papi_counter, sizeof...(_Events)> _counters{0};
                int _eventset;
        };


namespace detail
{

        template <std::size_t N, typename _Stream, event_code... _Events>
        inline std::enable_if_t<N == event_set<_Events...>::size()>
        to_stream(_Stream&, const event_set<_Events...>&) { }

        template <std::size_t N, typename _Stream, event_code... _Events>
        inline std::enable_if_t<N < event_set<_Events...>::size()>
        to_stream(_Stream& strm, const event_set<_Events...>& set)
        {
            strm << set.template at<N>() << " ";
            detail::to_stream<N + 1>(strm, set);
        }
}

        template <typename _Stream, event_code... _Events>
        inline _Stream& operator<<(_Stream& strm, const event_set<_Events...>& set)
        {
            detail::to_stream<0>(strm, set);
            return strm;
        }

}


#endif
