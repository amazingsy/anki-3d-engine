// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Util/String.h>
#include <AnKi/Util/List.h>
#include <algorithm>

namespace anki {

/// @addtogroup util_containers
/// @{

/// A simple convenience class for string lists
class StringList : public List<String>
{
public:
	using Char = char; ///< Char type
	using Base = List<String>; ///< Base
	using Allocator = GenericMemoryPoolAllocator<Char>;

	/// Sort method for sortAll().
	enum class Sort
	{
		ASCENDING,
		DESCENDING
	};

	// Use the base constructors
	using Base::Base;

	explicit operator Bool() const
	{
		return !Base::isEmpty();
	}

	void destroy(Allocator alloc);

	/// Join all the elements into a single big string using a the seperator @a separator
	void join(Allocator alloc, const CString& separator, String& out) const;

	/// Join all the elements into a single big string using a the seperator @a separator
	void join(const CString& separator, StringAuto& out) const;

	/// Returns the index position of the last occurrence of @a value in the list.
	/// @return -1 of not found
	I getIndexOf(const CString& value) const;

	/// Sort the string list
	void sortAll(const Sort method = Sort::ASCENDING);

	/// Push at the end of the list a formated string.
	template<typename... TArgs>
	void pushBackSprintf(Allocator alloc, CString fmt, TArgs... args)
	{
		String str;
		str.sprintf(alloc, fmt, args...);

		Base::emplaceBack(alloc);
		Base::getBack() = std::move(str);
	}

	/// Push at the beginning of the list a formated string.
	template<typename... TArgs>
	void pushFrontSprintf(Allocator alloc, CString fmt, TArgs... args)
	{
		String str;
		str.sprintf(alloc, fmt, args...);

		Base::emplaceFront(alloc);
		Base::getFront() = std::move(str);
	}

	/// Push back plain CString.
	void pushBack(Allocator alloc, CString cstr)
	{
		String str;
		str.create(alloc, cstr);

		Base::emplaceBack(alloc);
		Base::getBack() = std::move(str);
	}

	/// Push front plain CString
	void pushFront(Allocator alloc, CString cstr)
	{
		String str;
		str.create(alloc, cstr);

		Base::emplaceFront(alloc);
		Base::getFront() = std::move(str);
	}

	/// Split a string using a separator (@a separator) and return these strings in a string list.
	void splitString(Allocator alloc, const CString& s, const Char separator, Bool keepEmpty = false);
};

/// String list with automatic destruction.
class StringListAuto : public StringList
{
public:
	using Base = StringList;
	using Allocator = typename Base::Allocator;

	/// Create using an allocator.
	StringListAuto(Allocator alloc)
		: Base()
		, m_alloc(alloc)
	{
	}

	/// Move.
	StringListAuto(StringListAuto&& b)
		: Base()
	{
		move(b);
	}

	~StringListAuto()
	{
		Base::destroy(m_alloc);
	}

	/// Move.
	StringListAuto& operator=(StringListAuto&& b)
	{
		move(b);
		return *this;
	}

	/// Destroy.
	void destroy()
	{
		Base::destroy(m_alloc);
	}

	/// Push at the end of the list a formated string
	template<typename... TArgs>
	void pushBackSprintf(CString fmt, TArgs... args)
	{
		Base::pushBackSprintf(m_alloc, fmt, args...);
	}

	/// Push at the beginning of the list a formated string
	template<typename... TArgs>
	void pushFrontSprintf(CString fmt, TArgs... args)
	{
		Base::pushFrontSprintf(m_alloc, fmt, args...);
	}

	/// Push back plain CString.
	void pushBack(CString cstr)
	{
		Base::pushBack(m_alloc, cstr);
	}

	/// Push front plain CString.
	void pushFront(CString cstr)
	{
		Base::pushFront(m_alloc, cstr);
	}

	/// Pop front element.
	void popFront()
	{
		getFront().destroy(m_alloc);
		Base::popFront(m_alloc);
	}

	/// Split a string using a separator (@a separator) and return these strings in a string list.
	void splitString(const CString& s, const Base::Char separator, Bool keepEmpty = false)
	{
		Base::splitString(m_alloc, s, separator, keepEmpty);
	}

private:
	Allocator m_alloc;

	void move(StringListAuto& b)
	{
		Base::operator=(std::move(b));
		m_alloc = std::move(b.m_alloc);
	}
};
/// @}

} // end namespace anki
