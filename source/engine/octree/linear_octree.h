#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>
#include <bitset>
#include <assert.h>
#include "../vkapi/data_type.h"

namespace octree
{

enum LOctant
{
	DN_LEVEL_BACK_LEFT = 0,
	DN_LEVEL_BACK_RIGHT,
	DN_LEVEL_FRONT_LEFT,
	DN_LEVEL_FRONT_RIGHT,
	UP_LEVEL_BACK_LEFT,
	UP_LEVEL_BACK_RIGHT,
	UP_LEVEL_FRONT_LEFT,
	UP_LEVEL_FRONT_RIGHT,
	OCTANT_SIZE
};

template <class T>
struct LOctantNode
{
	uint32_t locCode;
	uint8_t  childrenFlags;
	std::unique_ptr<std::vector<T>> data;
};



template <class T>
class LinearOctree
{
public:

	using Point = glm::vec3;
	using Callback = std::function<bool(const Point& min, const Point& max, std::vector<T>*)>;

	enum ErrorCode
	{
		SUCCESS,
		PUSH_ERROR_DATAPOINT_OUT_OF_RANGE
	};

	LinearOctree(const Point& min, float size_length, size_t max_depth, size_t max_element_per_leaf_node = 0);
	~LinearOctree();

	LinearOctree(const LinearOctree&) = delete;
	LinearOctree(LinearOctree&&) = delete;
	void operator=(const LinearOctree&) = delete;
	void operator=(LinearOctree&&) = delete;

	// MEMBERS
	bool traverse(Callback callback);
	void clear();
	std::vector<T>& push(const Point& data_point, ErrorCode& error, Callback callback = nullptr);

	// ACCESSORS
	size_t node_depth(const LOctantNode<T>* node);
	LOctantNode<T>* parent_node(const LOctantNode<T>* node);
	bool is_leaf(const LOctantNode<T>* node);
	bool is_valid_node(LOctantNode<T>* node);
	void linearProcess(std::function<void(LOctantNode<T>&)> callback, bool only_data_node);

private:
	Point d_min;
	Point d_max;
	size_t d_max_depth;
	size_t d_max_element_per_node;
	std::unordered_map<uint32_t, LOctantNode<T>> d_nodes;
	const uint32_t ROOT_CODE = 1; // 001, 1 000, 1 001 ...

	// HELPERS
	LOctantNode<T>* LookupNode(uint32_t locCode);
	void traverseRecursive(Callback callback, const Point& curr_min, const Point& curr_max, const LOctantNode<T>* curr_node);
	LOctantNode<T>* root_node();
	LOctantNode<T>* get_node_from(LOctantNode<T>* parent, LOctant octant, bool isLeaf);
	bool is_inside(const Point& sample, const Point& min, const Point& max);

	LOctantNode<T>* split(const Point& data_point, LOctantNode<T>* currnode, const Point& min, const const Point& max, int& depth);

	// bit operations
	void set_8bit_mut(uint8_t& input, size_t index, bool val);
	void set_32bit_mut(uint32_t& input, size_t index, bool val);
	uint8_t set_8bit(const uint8_t& input, size_t index, bool val);
	uint32_t set_32bit(const uint32_t& input, size_t index, bool val);
	uint32_t shift_left(const uint32_t& input, size_t count);
	uint32_t shift_right(const uint32_t& input, size_t count);
	bool is_bit_set_8bit(const uint8_t& flag, size_t index);
};


template<class T>
inline LinearOctree<T>::LinearOctree(const Point& min, float side_length, size_t max_depth, size_t max_element_per_leaf_node)
	: d_min(min)
	, d_max(min.x + side_length, min.y + side_length, min.z + side_length)
	, d_max_depth(max_depth)
	, d_max_element_per_node(max_element_per_leaf_node)
{
	assert(d_max_depth <= 10);
}

template<class T>
inline LinearOctree<T>::~LinearOctree()
{
	clear();
}

template<class T>
inline bool LinearOctree<T>::traverse(Callback callback)
{
	assert(callback);

	if (d_nodes.empty())
	{
		return false;
	}

	traverseRecursive(callback, d_min, d_max, &d_nodes[ROOT_CODE]);
	return true;
}

template<class T>
inline void LinearOctree<T>::clear()
{
	d_nodes.clear();
}

template<class T>
inline std::vector<T>& LinearOctree<T>::push(const Point& data_point, ErrorCode& error, Callback callback)
{
	static std::vector<T> dummy;

	if (!is_inside(data_point, d_min, d_max))
	{
		error = PUSH_ERROR_DATAPOINT_OUT_OF_RANGE;
		return dummy;
	}

	int depth = 0;
	return *(split(data_point, root_node(), d_min, d_max, depth)->data);
}

template<class T>
inline size_t LinearOctree<T>::node_depth(const LOctantNode<T>* node)
{
	assert(node && node->locCode); // at least flag bit must be set
	// for (uint32_t lc=node->LocCode, depth=0; lc!=1; lc>>=3, depth++);
	// return depth;

#if defined(__GNUC__)
	return (31 - __builtin_clz(node->loc_code)) / 3;
#elif defined(_MSC_VER)
	unsigned long msb;
	_BitScanReverse(&msb, node->locCode);
	return msb / 3;
#endif
}

template<class T>
inline LOctantNode<T>* LinearOctree<T>::parent_node(const LOctantNode<T>* node)
{
	assert(node);
	const uint32_t locCodeParent = (node->locCode >> 3);
	return LookupNode(locCodeParent);
}

template<class T>
inline bool LinearOctree<T>::is_leaf(const LOctantNode<T>* node)
{
	assert(node);
	return node->childrenFlags == 0 ? true : false;
}

template<class T>
inline bool LinearOctree<T>::is_valid_node(LOctantNode<T>* node)
{
	assert(node);
	return (node->childrenFlags == 0 && node->data != nullptr || node->childrenFlags != 0 && node->data == nullptr) ? true : false;
}

template<class T>
inline void LinearOctree<T>::linearProcess(std::function<void(LOctantNode<T>&)> callback, bool only_data_node)
{
	if (only_data_node)
	{
		for (auto& elem : d_nodes)
		{
			if (is_leaf(&elem.second))
			{
				callback(elem.second);
			}
		}
	}
	else
	{
		for (auto& elem : d_nodes)
		{
			callback(elem.second);
		}
	}
}

template<class T>
inline LOctantNode<T>* LinearOctree<T>::LookupNode(uint32_t locCode)
{
	return d_nodes.count(locCode) == 0 ? nullptr : &d_nodes[locCode];
}

template<class T>
inline void LinearOctree<T>::traverseRecursive(Callback callback, const Point& curr_min, const Point& curr_max, const LOctantNode<T>* curr_node)
{
	if (!curr_node)
	{
		return;
	}

	if (callback == nullptr)
	{
		return;
	}

	if (!callback(curr_min, curr_max, curr_node->data ? curr_node->data.get() : nullptr))
	{
		return;
	}


	Point delta = curr_max - curr_min;
	Point half_delta = (delta * 0.5f);

	for (int i = 0; i < LOctant::OCTANT_SIZE; ++i)
	{
		if (curr_node->childrenFlags & (1 << i))
		{
			const uint32_t locCodeChild = (curr_node->locCode << 3) | i;
			const auto* child = LookupNode(locCodeChild);

			int x = i % 2, z = (i / 2) % 2, y = i / (2 * 2);

			auto next_min = curr_min + half_delta * glm::vec3(x, y, z);
			auto next_max = next_min + half_delta;
			traverseRecursive(callback, next_min, next_max, child);
		}
	}

}

template<class T>
inline LOctantNode<T>* LinearOctree<T>::root_node()
{
	if (!d_nodes.empty())
	{
		return &d_nodes[ROOT_CODE];
	}

	d_nodes[ROOT_CODE].locCode = ROOT_CODE;
	d_nodes[ROOT_CODE].childrenFlags = 0;
	d_nodes[ROOT_CODE].data = nullptr;
	return &d_nodes[ROOT_CODE];
}

template<class T>
inline LOctantNode<T>* LinearOctree<T>::get_node_from(LOctantNode<T>* parent, LOctant octant, bool isLeaf)
{
	// TODO:
	uint32_t childCode = shift_left(parent->locCode, 3);
	childCode |= ((uint32_t)octant);

	if (is_bit_set_8bit(parent->childrenFlags, (size_t)octant))
	{
		assert(d_nodes.count(childCode));
		return &d_nodes[childCode];
	}

	assert(d_nodes.count(childCode) == 0);
	d_nodes[childCode].locCode = childCode;
	d_nodes[childCode].childrenFlags = 0;

	set_8bit_mut(parent->childrenFlags, octant, true);

	if (isLeaf)
	{
		d_nodes[childCode].data = std::make_unique<std::vector<T>>();
	}

	return &d_nodes[childCode];
}

template<class T>
inline bool LinearOctree<T>::is_inside(const Point& sample, const Point& min, const Point& max)
{
	return
		(sample.x >= min.x &&
		sample.y >= min.y &&
		sample.z >= min.z &&
		sample.x < max.x &&
		sample.y < max.y &&
		sample.z < max.z) ? true : false;

}

template<class T>
inline LOctantNode<T>* LinearOctree<T>::split(const Point& data_point, LOctantNode<T>* currnode, const Point& curr_min, const const Point& curr_max, int& depth)
{
	if (!currnode)
	{
		return nullptr;
	}

	if (node_depth(currnode) == d_max_depth)
	{
		assert(currnode->data);
		return currnode;
	}

	Point delta = curr_max - curr_min;
	Point half_delta = (delta * 0.5f);

	LOctantNode<T>* data_node = nullptr;

	for (int i = 0; i < LOctant::OCTANT_SIZE; ++i)
	{
		int x = i % 2, z = (i / 2) % 2, y = i / (2 * 2);
		auto next_min = curr_min + half_delta * Point(x, y, z);
		auto next_max = next_min + half_delta;

		if (is_inside(data_point, next_min, next_max))
		{
			++depth;

			//TODO: created

			auto next_node = get_node_from(currnode, (LOctant)i, depth == d_max_depth ? true : false);
			data_node = split(data_point, next_node, next_min, next_max, depth);
			break;
		}
	}

	assert(is_leaf(data_node));
	return data_node;
}


// bit operations
template<class T>
inline void LinearOctree<T>::set_8bit_mut(uint8_t& input, size_t index, bool val)
{
	input = *((uint8_t*)& std::bitset<8>(input).set(index, val));
}

template<class T>
inline void LinearOctree<T>::set_32bit_mut(uint32_t& input, size_t index, bool val)
{
	input = *((uint32_t*)& std::bitset<32>(input).set(index, val));
}

template<class T>
inline uint8_t LinearOctree<T>::set_8bit(const uint8_t& input, size_t index, bool val)
{
	std::bitset<8> wrapper(input);
	wrapper.set(index, val);
	return *((uint8_t*)& wrapper);
}

template<class T>
inline uint32_t LinearOctree<T>::set_32bit(const uint32_t& input, size_t index, bool val)
{
	std::bitset<32> wrapper(input);
	wrapper.set(index, val);
	return *((uint32_t*)& wrapper);
}

template<class T>
inline uint32_t LinearOctree<T>::shift_left(const uint32_t& input, size_t count)
{
	std::bitset<32> bitsample(input);
	bitsample <<= count;
	return *((uint32_t*)& bitsample);
}

template<class T>
inline uint32_t LinearOctree<T>::shift_right(const uint32_t& input, size_t count)
{
	std::bitset<32> bitsample(input);
	bitsample >>= count;
	return *((uint32_t*)& bitsample);
}

template<class T>
inline bool LinearOctree<T>::is_bit_set_8bit(const uint8_t& flag, size_t index)
{
	std::bitset<8> wrapper(flag);
	return wrapper[index];
}

// sample data node
struct DrawMeshData
{
	std::size_t vbo_offset;
	std::shared_ptr<vkapi::BufferObject> vbo;
	std::shared_ptr<vkapi::BufferObject> ibo;
	std::vector<uint32_t> ibo_host;
	std::size_t draw_index_count;
	std::size_t draw_instance_first_index = 0;
	std::size_t draw_instance_count = 1;
};

using DrawMeshOctree = LinearOctree<DrawMeshData>;
using DrawMeshNode = LOctantNode<DrawMeshData>;

}// end namespace octree