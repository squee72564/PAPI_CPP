#include <papiCPP.hpp>
#include <vector>
#include <list>
#include <algorithm>

#include "FreeList.hpp"

int main() {

	try {
		std::vector<int> v;

		for (int i = 20000000; i >= 0; --i) {
			v.emplace_back(i);
		}

		papi::event_set<
			PAPI_BR_INS,
			PAPI_BR_TKN,
			PAPI_BR_MSP

		> events;


		{
			events.start_counters();

			std::list<int> l(v.begin(), v.end());
			l.sort();
				
			for (int& i : l) {
				i = i * i;
			}

			l.clear();

			events.stop_counters();
		}


		std::cout << "std::list" << std::endl;
		std::cout << events << std::endl;

		events.reset_counters();


		{
			events.start_counters();

			FreeList<int> fl(v.begin(), v.end());
			fl.sort();

			for (int& i : fl) {
				i = i * i;
			}

			fl.clear();

			events.stop_counters();
		}

		std::cout << "FreeList" << std::endl;
		std::cout << events << std::endl;

	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
