#include "TreeIO.hpp"

#include <iostream>

namespace sc
{
namespace tio
{
Vector<bool> _tabs;
void applyTab(bool has_next)
{
	for(size_t i = 0; i < _tabs.size(); ++i) {
		if(i == _tabs.size() - 1) {
			std::cout << (has_next ? " ├─" : " └─");
		} else {
			std::cout << (_tabs[i] ? " │" : "  ");
		}
	}
}
} // namespace tio
} // namespace sc
