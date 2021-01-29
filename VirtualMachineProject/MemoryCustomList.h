#pragma once
#include "MemoryAllocatorLight.h"
#include <cassert>
#include <exception>

template <typename ValueType>
struct MemoryCustomListNode
{
public:
	MemoryCustomListNode< ValueType>* prev = nullptr, * next = nullptr;
	ValueType value;
};

template <typename ValueType>
class MemoryCustomList
{
	using NodeType = MemoryCustomListNode< ValueType>;
	MemoryAllocatorLight allocator;
	NodeType *first, *last;


	[[nodiscard]] NodeType* createNode(const ValueType& value) {
		auto* voidptr = allocator.allocate(sizeof(MemoryCustomListNode<ValueType>));
		if (voidptr == nullptr)
			throw std::bad_alloc();
		auto* typeptr = new (voidptr) MemoryCustomListNode<ValueType>();
		//auto* typeptr = static_cast<MemoryCustomListNode<ValueType>*>(voidptr);
		typeptr->prev = nullptr;
		typeptr->next = nullptr;
		typeptr->value = value;
		return typeptr;
	} 
	[[nodiscard]] NodeType* createNode(ValueType&& value) {
		auto* voidptr = allocator.allocate(sizeof(MemoryCustomListNode<ValueType>));
		if (voidptr == nullptr)
			throw std::bad_alloc();
		auto* typeptr = static_cast<MemoryCustomListNode<ValueType>*>(voidptr);
		typeptr->prev = nullptr;
		typeptr->next = nullptr;
		typeptr->value = std::move(value);
		return typeptr;
	}

public:
	//class iterator {
	//	NodeType* ptr;
	//public:
	//	iterator() :ptr(nullptr) {}
	//	iterator(NodeType* ptr) :ptr(ptr) {}
	//
	//};

	//NodeType* begin() {
	//	return first;
	//}

	void push_front(NodeType* node)
	{
		assert(node != nullptr);
		assert(node->prev == nullptr);
		assert(node->next == nullptr);

		if (first) {
			node->next = first;
			first->prev = node;
			first = node;
		}
		else
		{
			first = last = node;
		}
	}

	void push_back(NodeType* node)
	{
		assert(node != nullptr);
		assert(node->prev == nullptr);
		assert(node->next == nullptr);

		if (last) {
			node->prev = last;
			last->next = node;
			last = node;
		}
		else
		{
			first = last = node;
		}
	}

	void push_front(const ValueType& value) {
		auto* typeptr = createNode(value);
		push_front(typeptr);
	}

	void push_back(const ValueType& value) {
		auto* typeptr = createNode(value);
		push_back(typeptr);
	}

	void insert(NodeType* nodeafter, NodeType* node)
	{
		assert(node != nullptr);
		assert(node->prev == nullptr);
		assert(node->next == nullptr);

		if (nodeafter != nullptr)
		{
			node->next = nodeafter;
			node->prev = nodeafter->prev;
			nodeafter->prev = node;
			if (node->prev) {
				node->prev->next = node;
			}
			else
			{
				first = node;
			}
		}
		else
		{
			if (last != nullptr)
			{
				node->prev = last;
				last->next = node;
				last = node;
			}
			else
			{
				first = last = node;
			}	
		}
	}

	void erase(NodeType* node) {
		release_node(node);
		node->~NodeType();
		allocator.deallocate(node);
	}

	void release_node(NodeType* node) {
		assert(node != nullptr); 
		
		NodeType* p = node->prev;
		NodeType* n = node->next;

		if (p != nullptr)
		{
			p->next = n;
		}
		else
		{
			assert(first == node);
			first = n;
		}

		if (n != nullptr)
		{
			n->prev = p;
		}
		else 
		{
			assert(last == node);
			last = p;
		}

		node->prev = node->next = nullptr;
	}

	void splice(NodeType* nodeafter, MemoryCustomList<ValueType> &otherlist, NodeType* node) {
		otherlist.release_node(node);
		insert(nodeafter, node);
	}

	~MemoryCustomList() {

		while (first)
		{
			erase(first);
		}
	}
};

