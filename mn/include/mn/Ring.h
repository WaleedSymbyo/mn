#pragma once

#include "mn/Memory.h"
#include "mn/Buf.h"
#include "mn/Assert.h"

#include <string.h>

namespace mn
{
	// a ring buffer which is useful for doing queues because it can push front and back
	template<typename T>
	struct Ring
	{
		Allocator allocator;
		T* ptr;
		size_t count;
		size_t cap;
		size_t head;

		inline T&
		operator[](size_t ix)
		{
			mn_assert(ix < count);
			return ptr[(head + ix) % cap];
		}

		inline const T&
		operator[](size_t ix) const
		{
			mn_assert(ix < count);
			return ptr[(head + ix) % cap];
		}
	};

	// creates a new ring instance
	template<typename T>
	inline static Ring<T>
	ring_new()
	{
		Ring<T> self{};
		self.allocator = allocator_top();
		return self;
	}

	// creates a new ring with the given allocator
	template<typename T>
	inline static Ring<T>
	ring_with_allocator(Allocator allocator)
	{
		Ring<T> self{};
		self.allocator = allocator;
		return self;
	}

	// frees the given ring
	template<typename T>
	inline static void
	ring_free(Ring<T>& self)
	{
		if(self.cap)
			free_from(self.allocator, Block { self.ptr, self.cap * sizeof(T) });
	}

	// destruct overload for ring free
	template<typename T>
	inline static void
	destruct(Ring<T>& self)
	{
		for(size_t i = 0; i < self.count; ++i)
		{
			destruct(self.ptr[(i + self.head) % self.cap]);
		}
		ring_free(self);
	}

	// ensures the ring has capacity for the given added size
	template<typename T>
	inline static void
	ring_reserve(Ring<T>& self, size_t added_size)
	{
		if(self.count + added_size <= self.cap)
			return;

		size_t next_cap = size_t(self.cap * 1.5f);
		size_t accurate_cap = self.count + added_size;
		size_t request_cap = next_cap > accurate_cap ? next_cap : accurate_cap;
		Block new_block = alloc_from(self.allocator, request_cap * sizeof(T), alignof(T));
		if(self.count)
		{
			const size_t first_copy_size = (self.cap - self.head) * sizeof(T);
			::memcpy(new_block.ptr, self.ptr + self.head, first_copy_size);
			::memcpy((char*)new_block.ptr + first_copy_size, self.ptr, self.head * sizeof(T));
		}

		if(self.cap)
			free_from(self.allocator, Block { self.ptr, self.cap * sizeof(T) });

		self.ptr = (T*)new_block.ptr;
		self.cap = request_cap;
		self.head = 0;
	}

	// pushes a value to the end of the ring buffer
	template<typename T, typename R>
	inline static void
	ring_push_back(Ring<T>& self, const R& value)
	{
		if(self.count == self.cap)
			ring_reserve(self, self.cap ? 1 : 8);

		self.ptr[(self.head + self.count) % self.cap] = value;
		++self.count;
	}

	// pushes a value to the front of the ring buffer
	template<typename T, typename R>
	inline static void
	ring_push_front(Ring<T>& self, const R& value)
	{
		if(self.count == self.cap)
			ring_reserve(self, self.cap ? 1 : 8);

		self.head = self.head ? self.head - 1 : self.cap - 1;
		self.ptr[self.head] = value;
		++self.count;
	}

	// returns the value at the back of the ring buffer
	template<typename T>
	inline static T&
	ring_back(Ring<T>& self)
	{
		mn_assert(self.count > 0);
		const size_t ix = (self.head + self.count - 1) % self.cap;
		return self.ptr[ix];
	}

	// returns the value at the back of the ring buffer
	template<typename T>
	inline static const T&
	ring_back(const Ring<T>& self)
	{
		mn_assert(self.count > 0);
		const size_t ix = (self.head + self.count - 1) % self.cap;
		return self.ptr[ix];
	}

	// returns the value of the front of the ring buffer
	template<typename T>
	inline static T&
	ring_front(Ring<T>& self)
	{
		mn_assert(self.count > 0);
		return self.ptr[self.head];
	}

	// returns the value of the front of the ring buffer
	template<typename T>
	inline static const T&
	ring_front(const Ring<T>& self)
	{
		mn_assert(self.count > 0);
		return self.ptr[self.head];
	}

	// pops a value off the back of the ring
	template<typename T>
	inline static void
	ring_pop_back(Ring<T>& self)
	{
		mn_assert(self.count > 0);
		--self.count;
	}

	// pops a value off the front of the ring
	template<typename T>
	inline static void
	ring_pop_front(Ring<T>& self)
	{
		mn_assert(self.count > 0);
		self.head = (self.head + 1) % self.cap;
		--self.count;
	}

	// returns whether the given ring is empty
	template<typename T>
	inline static bool
	ring_empty(const Ring<T>& self)
	{
		return self.count == 0;
	}
}