#include "Report.hpp"

namespace atlas {

std::map<std::string, TextReport>& textReports() {
	static std::map<std::string, TextReport> r;
	return r;
}

} // namespace atlas
