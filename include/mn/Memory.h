#pragma once

#include "mn/Exports.h"
#include "mn/Base.h"
#include "mn/memory/Interface.h"
#include "mn/memory/Stack.h"
#include "mn/memory/Arena.h"

#include <stdint.h>
#include <utility>

namespace mn
{
	using Allocator = memory::Interface*;

	/**
	 * Allocators are organized in a per thread stack so that you can change the allocator of any
	 * part of the code even if it doesn't support custom allocator by using
	 * allocator_push() and allocator_pop()
	 * At the base of the stack is the clib_allocator and it can't be popped
	 *
	 * @return     The current top of allocator stack of the calling thread
	 */
	API_MN Allocator
	allocator_top();

	/**
	 * @brief      Pushes the given allocator to the top of the calling thread allocator stack
	 */
	API_MN void
	allocator_push(Allocator allocator);

	/**
	 * @brief      Pops an allocator off the calling thread allocator stack
	 */
	API_MN void
	allocator_pop();


	//alloc from interface
	/**
	 * @brief      Allocates a block of memory off the given allocator
	 *
	 * @param[in]  allocator  The allocator
	 * @param[in]  size       The size
	 * @param[in]  alignment  The alignment
	 *
	 * @return     The allocated block of memory
	 */
	inline static Block
	alloc_from(Allocator self, size_t size, uint8_t alignment)
	{
		return self->alloc(size, alignment);
	}

	/**
	 * @brief      Frees a block of memory off the given allocator
	 *
	 * @param[in]  allocator  The allocator
	 * @param[in]  block      The block
	 */
	inline static void
	free_from(Allocator self, Block block)
	{
		self->free(block);
	}


	/**
	 * @brief      Allocates a single instance of type T from the given allocator
	 *
	 * @param[in]  allocator  The allocator
	 *
	 * @tparam     T          Instance type
	 *
	 * @return     A Pointer to the allocated instance
	 */
	template<typename T>
	inline static T*
	alloc_from(Allocator self)
	{
		return (T*)alloc_from(self, sizeof(T), alignof(T)).ptr;
	}

	/**
	 * @brief      Frees a single instance off the given allocator
	 *
	 * @param[in]  allocator  The allocator
	 * @param[in]  ptr        The pointer
	 *
	 * @tparam     T          Instance type
	 */
	template<typename T>
	inline static void
	free_from(Allocator self, const T* ptr)
	{
		return free_from(self, Block{ (void*)ptr, sizeof(T) });
	}


	/**
	 * @brief      Allocates a block of memory off the top of the allocator stack
	 *
	 * @param[in]  size       The size
	 * @param[in]  alignment  The alignment
	 */
	inline static Block
	alloc(size_t size, uint8_t alignment)
	{
		return alloc_from(allocator_top(), size, alignment);
	}

	/**
	 * @brief      Frees a block of memory off the top of the allocate stack
	 *
	 * @param[in]  block  The block
	 */
	inline static void
	free(Block block)
	{
		return free_from(allocator_top(), block);
	}


	/**
	 * @brief      Allocates a single instance of type T off the top of the allocator stack
	 *
	 * @tparam     T     Instance type
	 *
	 * @return     A Pointer to the allocated instance
	 */
	template<typename T>
	inline static T*
	alloc()
	{
		return (T*)alloc(sizeof(T), alignof(T)).ptr;
	}

	/**
	 * @brief      Frees a single instance off the top of the allocator stack
	 *
	 * @param[in]  ptr   The pointer
	 *
	 * @tparam     T     Instance type
	 */
	template<typename T>
	inline static void
	free(const T* ptr)
	{
		return free(Block{ (void*)ptr, sizeof(T) });
	}


	template<typename T, typename ... TArgs>
	inline static T*
	alloc_construct_from(Allocator self, TArgs&& ... args)
	{
		T* res = alloc_from<T>(self);
		::new (res) T(std::forward<TArgs>(args)...);
		return res;
	}

	template<typename T>
	inline static void
	free_destruct_from(Allocator self, T* ptr)
	{
		ptr->~T();
		free_from(self, ptr);
	}


	template<typename T, typename ... TArgs>
	inline static T*
	alloc_construct(TArgs&& ... args)
	{
		T* res = alloc<T>();
		::new (res) T(std::forward<TArgs>(args)...);
		return res;
	}

	template<typename T>
	inline static void
	free_destruct(T* ptr)
	{
		ptr->~T();
		free(ptr);
	}


	/**
	 * @brief      Allocates a single zero-initialized instance from the given allocator
	 *
	 * @param[in]  allocator  The allocator
	 *
	 * @tparam     T          Instance type
	 *
	 * @return     A Pointer to the allocated instance
	 */
	template<typename T>
	inline static T*
	alloc_zerod_from(Allocator self)
	{
		Block block = alloc_from(self, sizeof(T), alignof(T));
		block_zero(block);
		return (T*)block.ptr;
	}

	/**
	 * @brief      Allocates a single zero-initialized instance off the the top of the allocator stack
	 *
	 * @tparam     T     Instance type
	 *
	 * @return     A Pointer to the allocated instance
	 */
	template<typename T>
	inline static T*
	alloc_zerod()
	{
		Block block = alloc(sizeof(T), alignof(T));
		block_zero(block);
		return (T*)block.ptr;
	}


	inline static memory::Stack*
	allocator_stack_new(size_t stack_size, Allocator meta = memory::clib())
	{
		return alloc_construct<memory::Stack>(stack_size, meta);
	}

	inline static memory::Arena*
	allocator_arena_new(size_t block_size = 4096, Allocator meta = memory::clib())
	{
		return alloc_construct<memory::Arena>(block_size, meta);
	}

	inline static void
	allocator_free(Allocator self)
	{
		free_destruct(self);
	}

	inline static void
	destruct(Allocator self)
	{
		allocator_free(self);
	}
}
