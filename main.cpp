#include <papiCPP.hpp>
#include <vector>
#include <list>
#include <algorithm>

#include "FreeList.hpp"

int main() {

	try {
		std::vector<int> v;

		for (int i = 9999999; i >= 0; --i) {
			v.emplace_back(i);
		}

		papi::event_set<
			PAPI_L1_TCA,
			PAPI_TLB_DM,
			PAPI_TLB_IM
		> events;

		events.start_counters();

		{
			std::list<int> l(v.begin(), v.end());
			l.sort();
		}

		events.stop_counters();

		std::cout << "std::list" << std::endl;
		std::cout << events << std::endl;

		events.reset_counters();

		events.start_counters();

		{
			FreeList<int> fl;
			fl.reserve(v.size());
			for (const auto i : v) {
				fl.emplace_back(i);
			}
			fl.sort();
		}

		events.stop_counters();

		std::cout << "FreeList" << std::endl;
		std::cout << events << std::endl;

	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
