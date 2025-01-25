#pragma once

#include "vk_wrap/VKW_Object.h"
#include <stack>

class DeletionQueue
{
public:
	DeletionQueue() = default;
	inline void add(VKW_Object* obj);
	inline void del_all_obj();
private:
	std::stack<VKW_Object*> to_delete;
};

inline void DeletionQueue::add(VKW_Object* obj)
{
	to_delete.push(obj);
}

inline void DeletionQueue::del_all_obj()
{
	while (!to_delete.empty()) {
		VKW_Object* obj = to_delete.top();
		obj->del();
		to_delete.pop();
	}
}