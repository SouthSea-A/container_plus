#pragma once
#include <vector>
#include <tuple>
#include <optional>
#include <functional>
#include <expected>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <random>

template<typename T,typename U>
struct element
{
	static_assert(std::numeric_limits<U>::is_integer, "位置索引类型无效");
	T src;
	U last;
	U next;
	template<typename...params>
	constexpr element(U last_val, U next_val, params&&...src_val) noexcept(std::is_nothrow_constructible_v<T, params...>) :
		src(std::forward<params>(src_val)...), last(last_val), next(next_val)
	{

	}
};

using page_data_type_base_16 = std::uint16_t;
using page_data_type_base_32 = std::uint32_t;
using page_data_type_base_64 = std::uint64_t;

using index_page = page_data_type_base_64;
using index_order_page = page_data_type_base_64;
using index_page_pos = page_data_type_base_16;
using index_page_order = page_data_type_base_16;

constexpr index_page INDEX_PAGE_END = std::numeric_limits<index_page>::max();
constexpr index_page_pos INDEX_PAGE_POS_END = std::numeric_limits<index_page_pos>::max();
constexpr index_page_order INDEX_PAGE_ORDER_END = std::numeric_limits<index_page_order>::max();
constexpr index_order_page INDEX_ORDER_PAGE_END = std::numeric_limits<index_order_page>::max();


template<typename T, typename U, std::size_t MAX, typename allocator_cache = std::allocator<U>>
class cache_mapping
{
	static_assert(MAX < static_cast<std::size_t>(std::numeric_limits<T>::max()), "缓存大小超出最大范围");
	static_assert(std::numeric_limits<T>::is_integer, "缓存逻辑映射类型无效");
	static_assert(std::numeric_limits<U>::is_integer, "缓存位置映射类型无效");
private:
	std::vector<U, allocator_cache> set;
public:
	cache_mapping() = default;
	cache_mapping(std::size_t size_val) :
		set()
	{
		this->set.reserve(size_val);
	}
	explicit cache_mapping(const allocator_cache& allocator_val, std::size_t size_val) :
		set(allocator_val)
	{
		this->set.reserve(size_val);
	}
	explicit cache_mapping(const allocator_cache& allocator_val) :
		set(allocator_val)
	{

	}
public:
	bool add_set(T cache_order_new, U cache_pos_new)
	{
		std::size_t pos_target = static_cast<std::size_t>(cache_order_new);

		if (this->set.size() >= MAX)
			return false;

		if (pos_target >= MAX)
			return false;

		if (pos_target >= this->set.size())
			this->set.resize(pos_target + 1, std::numeric_limits<U>::max());

		this->set[pos_target] = cache_pos_new;
		return true;
	}
	bool insert_front(T cache_order_insert, U cache_pos_new)
	{
		std::size_t pos_target = static_cast<std::size_t>(cache_order_insert);

		if (this->set.size() >= MAX)
			return false;

		if (pos_target >= MAX)
			return false;

		if (pos_target >= this->set.size())
		{
			this->set.resize(pos_target + 1, std::numeric_limits<U>::max());
			this->set[pos_target] = cache_pos_new;
		}
		else
		{
			this->set.insert(this->set.begin() + pos_target, cache_pos_new);
		}

		return true;
	}
	bool insert_back(T cache_order_insert, U cache_pos_new)
	{
		return insert_front(cache_order_insert + 1, cache_pos_new);
	}
	constexpr std::optional<U> get(T index) const noexcept
	{
		std::size_t pos_target = static_cast<std::size_t>(index);

		if (pos_target >= this->set.size())
			return std::nullopt;

		U pos_target_get = this->set[pos_target];

		if (pos_target_get == std::numeric_limits<U>::max())
			return std::nullopt;

		return std::make_optional(pos_target_get);
	}
	constexpr std::optional<U> get_next(T index) const noexcept
	{
		return get(index + 1);
	}
	constexpr std::optional<U> get_last(T index) const noexcept
	{
		if (index == 0)
			return std::nullopt;

		return get(index - 1);
	}
	constexpr std::optional<U> get_front() const noexcept
	{
		return get(0);
	}
	constexpr std::optional<U> get_back() const noexcept
	{
		return get(static_cast<T>(this->set.size() - 1));
	}
	bool del(T index) noexcept
	{
		std::size_t pos_target = static_cast<std::size_t>(index);

		if (pos_target >= this->set.size())
			return false;

		this->set.erase(this->set.begin() + pos_target);

		return true;
	}
	void clear() noexcept
	{
		this->set.clear();
		return;
	}
};

enum class page_interrupt_type
{
	page_missing,
	element_missing,
	index_invalid,
	pos_invalid,
	order_invalid,
	begin_invalid,
	end_invalid,
	free_empty,
	size_max,
};

using page_result = std::expected<void, page_interrupt_type>;
using page_result_pos = std::expected<index_page_pos, page_interrupt_type>;
using page_result_index = std::expected<index_page, page_interrupt_type>;
template<typename T>
using page_result_ref = std::expected<std::reference_wrapper<std::optional<element<T, index_page_pos>>>, page_interrupt_type>;
template<typename T>
using page_result_const_ref = std::expected<std::reference_wrapper<const std::optional<element<T, index_page_pos>>>, page_interrupt_type>;
template<typename T>
using page_result_src = std::expected<std::reference_wrapper<T>, page_interrupt_type>;
template<typename T>
using page_result_const_src = std::expected<std::reference_wrapper<const T>, page_interrupt_type>;

template<typename T, std::size_t MAX, typename allocator_page = std::allocator<T>>
class page
{
	static_assert(MAX != 0, "页容量无效");
	static_assert(MAX < std::numeric_limits<page_data_type_base_16>::max(), "页大小超出最大范围");
private:
	using allocator_page_set = typename std::allocator_traits<allocator_page>::template rebind_alloc<std::optional<element<T, index_page_pos>>>;
	using allocator_page_free = typename std::allocator_traits<allocator_page>::template rebind_alloc<index_page_pos>;
	using allocator_page_cache = typename std::allocator_traits<allocator_page>::template rebind_alloc<index_page_pos>;
private:
	index_page_pos last;
	std::vector<std::optional<element<T, index_page_pos>>, allocator_page_set> set;
	std::vector<index_page_pos, allocator_page_free> free;
	cache_mapping<index_page_order, index_page_pos, MAX, allocator_page_cache> cache;
private:
	constexpr bool test_validity_index_page_pos(index_page_pos index, bool is_test_free) const noexcept
	{
		if (static_cast<std::size_t>(index) >= this->set.size())
			return false;

		return is_test_free == false ? true : this->set[static_cast<std::size_t>(index)].has_value() == true;
	}
	constexpr bool test_validity_index_page_order(index_page_order index) const noexcept
	{
		return static_cast<std::size_t>(index) < (this->set.size() - this->free.size());
	}
	constexpr page_result_pos get_pos_by_order(index_page_order index) const noexcept
	{
		std::optional<index_page_pos> pos_order_cache = this->cache.get(index);

		if (pos_order_cache.has_value() == false)
			return std::unexpected(page_interrupt_type::order_invalid);
			
		return pos_order_cache.value();
	}
	constexpr page_result_ref<T> get_element_ref_by_order(index_page_order index) noexcept
	{
		page_result_pos pos_target = get_pos_by_order(index);

		if (pos_target.has_value() == false)
			return std::unexpected(pos_target.error());

		return this->set[static_cast<std::size_t>(pos_target.value())];
	}
	constexpr page_result_ref<T> get_element_ref_by_pos(index_page_pos index, bool is_get_free = false) noexcept
	{
		if (test_validity_index_page_pos(index, !is_get_free) == false)
			return std::unexpected(page_interrupt_type::pos_invalid);

		return this->set[static_cast<std::size_t>(index)];
	}
	constexpr page_result_const_ref<T> get_element_const_ref_by_pos(index_page_pos index, bool is_get_free = false) const noexcept
	{
		if (test_validity_index_page_pos(index, !is_get_free) == false)
			return std::unexpected(page_interrupt_type::pos_invalid);

		return this->set[static_cast<std::size_t>(index)];
	}
	page_result add_free_by_order(index_page_order index)
	{
		page_result_pos pos_target = get_pos_by_order(index);

		if (pos_target.has_value() == false)
			return std::unexpected(pos_target.error());

		index_page_pos index_pos = pos_target.value();

		page_result_ref<T> element_target = get_element_ref_by_pos(index_pos);

		if (element_target.has_value() == false)
			return std::unexpected(element_target.error());

		element_target.value().get().reset();

		this->free.emplace_back(index_pos);
		return {};
	}
	page_result add_free_by_pos(index_page_pos index)
	{
		page_result_ref<T> element_target = get_element_ref_by_pos(index);

		if (element_target.has_value() == false)
			return std::unexpected(element_target.error());

		element_target.value().get().reset();

		this->free.emplace_back(index);
		return {};
	}
	page_result_pos get_free() noexcept
	{
		if (this->free.empty() == true)
			return std::unexpected(page_interrupt_type::free_empty);

		index_page_pos pos_free = this->free.back();

		this->free.pop_back();

		return pos_free;
	}
	void tidy_element(bool is_realloc)
	{
		if (test_validity_index_page_pos(this->last, true) == false)
			return;

		if (this->last == 0)
			return;

		if (this->free.empty() == true)
			return;

		std::optional<index_page_pos> pos_begin_res = this->cache.get_front();

		if (pos_begin_res.has_value() == false)
			return;

		index_page_pos pos_begin = pos_begin_res.value();

		std::sort(this->free.begin(), this->free.end());

		std::vector<index_page_pos>::const_iterator pos_free_it = this->free.begin();
		index_page_pos pos_current = this->last;
		index_page_pos pos_free = *pos_free_it;
		page_result_ref<T> element_free = get_element_ref_by_pos(pos_free, true);
		page_result_ref<T> element_current = get_element_ref_by_pos(pos_current);

		while (pos_free < pos_current)
		{
			if (element_free.has_value() == true && element_current.has_value() == true)
			{
				std::exchange(element_free.value().get(), element_current.value().get());
			}

			if (pos_current == pos_begin)
				pos_begin = pos_free;

			if (element_current.has_value() == true)
			{
				element<T, index_page_pos>& element_current_ref = element_current.value().get().value();

				page_result_ref<T> element_last = get_element_ref_by_pos(element_current_ref.last);
				page_result_ref<T> element_next = get_element_ref_by_pos(element_current_ref.next);

				if (element_last.has_value() == true)
					element_last.value().get().value().next = pos_free;

				if (element_next.has_value() == true)
					element_next.value().get().value().last = pos_free;

				pos_free_it++;
				element_current.value().get().reset();
			}
		
			if (pos_free_it == this->free.end()) 
				break;

			pos_free = *pos_free_it;
			element_free = get_element_ref_by_pos(pos_free, true);
			element_current = get_element_ref_by_pos(--pos_current);
		}

		if (pos_current == this->last) element_current = get_element_ref_by_pos(--pos_current);

		std::size_t try_max = pos_current;

		while (element_current.has_value() == false && try_max != 0)
		{
			element_current = get_element_ref_by_pos(--pos_current);
			try_max--;
		}

		if (element_current.has_value() == true)
		{
			this->last = pos_current;

			std::size_t size_new = static_cast<std::size_t>(pos_current + 1);

			this->set.resize(size_new);

			if (is_realloc == true) this->set.shrink_to_fit();

			this->free.clear();
			this->free.shrink_to_fit();
		}
		else
		{
			this->set.clear();
			this->free.clear();

			if (is_realloc == true) this->set.shrink_to_fit();

			this->free.shrink_to_fit();
		}

		this->cache.clear();

		pos_current = pos_begin;

		index_page_order order_current = 0;

		while (order_current < this->set.size())
		{
			this->cache.add_set(order_current, pos_current);

			element_current = get_element_ref_by_pos(pos_current);

			if (element_current.has_value() == false)
				break;

			pos_current = element_current.value().get().value().next;
			order_current++;
		}

		return;
	}
public:
	page() :
		last(0), set(), free(), cache(MAX)
	{

	}
	explicit page(const allocator_page& allocator_val) :
		last(0), set(allocator_val), free(allocator_val), cache(allocator_val, MAX)
	{

	}
public:
	template<typename...params>
	page_result_src<T> add(params&&...src_new)
	{
		std::optional<index_page_pos> pos_end_res = this->cache.get_back();
		index_page_pos pos_new = 0;
		index_page_order order_new = 0;
		page_result_ref<T> element_end =
			pos_end_res.has_value() == false ?
			std::unexpected(page_interrupt_type::end_invalid) :
			get_element_ref_by_pos(pos_end_res.value());

		page_result_src<T> result_ref = std::unexpected(page_interrupt_type::end_invalid);

		if (element_end.has_value() == true && pos_end_res.has_value() == true)
		{
			if (element_end.value().get().has_value() == false)
				return std::unexpected(page_interrupt_type::element_missing);

			page_result_pos pos_free = get_free();

			if (pos_free.has_value() == false)
			{
				if (this->set.size() >= MAX)
					return std::unexpected(page_interrupt_type::size_max);

				pos_new = static_cast<index_page_pos>(this->set.size());
				order_new = static_cast<index_page_pos>(this->set.size());
				element_end.value().get().value().next = pos_new;
				result_ref = this->set.emplace_back(std::in_place, pos_end_res.value(), INDEX_PAGE_POS_END, std::forward<params>(src_new)...).value().src;	
			}
			else
			{
				page_result_ref<T> element_free = get_element_ref_by_pos(pos_free.value(), true);

				if (element_free.has_value() == false)
				{
					(void)add_free_by_pos(pos_free.value());
					return std::unexpected(element_free.error());
				}

				pos_new = pos_free.value();
				order_new = static_cast<index_page_pos>(this->set.size() - this->free.size() - 1);
				element_end.value().get().value().next = pos_new;
				element_free.value().get().emplace(pos_end_res.value(), INDEX_PAGE_POS_END, std::forward<params>(src_new)...);
				result_ref = element_free.value().get().value().src;
				
			}
		}
		else
		{
			pos_new = 0;
			order_new = 0;
			result_ref = this->set.emplace_back(std::in_place, INDEX_PAGE_POS_END, INDEX_PAGE_POS_END, std::forward<params>(src_new)...).value().src;
			this->last = pos_new;
		}

		this->cache.add_set(order_new, pos_new);

		if (pos_new > this->last)
			this->last = pos_new;

		return result_ref;
	}
	template<typename...params>
	page_result_src<T> insert_front(index_page_order index, params&&...src_new)
	{
		page_result_pos order_insert = get_pos_by_order(index);
		page_result_src<T> result_ref = std::unexpected(page_interrupt_type::end_invalid);

		if (order_insert.has_value() == false)
		{
			result_ref = add(std::forward<params>(src_new)...);

			if (result_ref.has_value() == false)
				return std::unexpected(result_ref.error());

			return result_ref.value();
		}

		page_result_ref<T> element_insert = get_element_ref_by_pos(order_insert.value());

		if (element_insert.has_value() == false)
			return std::unexpected(element_insert.error());

		element<T, index_page_pos>& element_insert_ref = element_insert.value().get().value();

		page_result_pos pos_free = get_free();
		index_page_pos pos_new = 0;

		if (pos_free.has_value() == false)
		{
			if (this->set.size() >= MAX)
				return std::unexpected(page_interrupt_type::size_max);

			pos_new = static_cast<index_page_pos>(this->set.size());

			page_result_ref<T> element_insert_last = get_element_ref_by_pos(element_insert_ref.last);

			if (element_insert_last.has_value() == true)
				element_insert_last.value().get().value().next = pos_new;

			index_page_pos pos_new_last = element_insert_ref.last;

			element_insert_ref.last = pos_new;

			result_ref = this->set.emplace_back(std::in_place, pos_new_last, order_insert.value(), std::forward<params>(src_new)...).value().src;
		}
		else
		{
			page_result_ref<T> element_free = get_element_ref_by_pos(pos_free.value(), true);

			if (element_free.has_value() == false)
			{
				(void)add_free_by_pos(pos_free.value());
				return std::unexpected(element_free.error());
			}

			pos_new = pos_free.value();

			page_result_ref<T> element_insert_last = get_element_ref_by_pos(element_insert_ref.last);

			if (element_insert_last.has_value() == true)
				element_insert_last.value().get().value().next = pos_new;

			index_page_pos pos_new_last = element_insert_ref.last;

			element_insert_ref.last = pos_new;
			element_free.value().get().emplace(pos_new_last, order_insert.value(), std::forward<params>(src_new)...);
			result_ref = element_free.value().get().value().src;
		}

		this->cache.insert_front(index, pos_new);

		if (pos_new > this->last)
			this->last = pos_new;

		return result_ref.value();
	}
	template<typename...params>
	page_result_src<T> insert_back(index_page_order index, params&&...src_new)
	{
		page_result_pos order_insert = get_pos_by_order(index);
		page_result_src<T> result_ref = std::unexpected(page_interrupt_type::end_invalid);

		if (order_insert.has_value() == false)
		{
			result_ref = add(std::forward<params>(src_new)...);

			if (result_ref.has_value() == false)
				return std::unexpected(result_ref.error());

			return result_ref.value();
		}

		page_result_ref<T> element_insert = get_element_ref_by_pos(order_insert.value());

		if (element_insert.has_value() == false)
			return std::unexpected(element_insert.error());

		element<T, index_page_pos>& element_insert_ref = element_insert.value().get().value();

		page_result_pos pos_free = get_free();
		index_page_pos pos_new = 0;

		if (pos_free.has_value() == false)
		{
			if (this->set.size() >= MAX)
				return std::unexpected(page_interrupt_type::size_max);

			pos_new = static_cast<index_page_pos>(this->set.size());

			page_result_ref<T> element_insert_next = get_element_ref_by_pos(element_insert_ref.next);

			if (element_insert_next.has_value() == true)
				element_insert_next.value().get().value().last = pos_new;

			index_page_pos pos_new_next = element_insert_ref.next;

			element_insert_ref.next = pos_new;

			result_ref = this->set.emplace_back(std::in_place, order_insert.value(), pos_new_next, std::forward<params>(src_new)...).value().src;
		}
		else
		{
			page_result_ref<T> element_free = get_element_ref_by_pos(pos_free.value(), true);

			if (element_free.has_value() == false)
			{
				(void)add_free_by_pos(pos_free.value());
				return std::unexpected(element_free.error());
			}

			pos_new = pos_free.value();

			page_result_ref<T> element_insert_next = get_element_ref_by_pos(element_insert_ref.next);

			if (element_insert_next.has_value() == true)
				element_insert_next.value().get().value().last = pos_new;

			index_page_pos pos_new_next = element_insert_ref.next;

			element_insert_ref.next = pos_new;
			element_free.value().get().emplace(order_insert.value(), pos_new_next, std::forward<params>(src_new)...);
			result_ref = element_free.value().get().value().src;
		}

		this->cache.insert_back(index, pos_new);

		if (pos_new > this->last)
			this->last = pos_new;

		return result_ref.value();
	}
	page_result del(index_page_order index)
	{
		page_result_pos pos_del = get_pos_by_order(index);

		if (pos_del.has_value() == false)
			return std::unexpected(pos_del.error());

		page_result_ref<T> element_del = get_element_ref_by_pos(pos_del.value());

		if (element_del.has_value() == false)
			return std::unexpected(element_del.error());

		element<T, index_page_pos>& element_del_ref = element_del.value().get().value();

		page_result_ref<T> element_del_set = get_element_ref_by_pos(element_del_ref.last);
		
		if (element_del_set.has_value() == true)
			element_del_set.value().get().value().next = element_del_ref.next;

		element_del_set = get_element_ref_by_pos(element_del_ref.next);

		if (element_del_set.has_value() == true)
			element_del_set.value().get().value().last = element_del_ref.last;

		page_result add_free_res = add_free_by_pos(pos_del.value());

		if (add_free_res.has_value() == false)
		{
			this->cache.insert_front(index, pos_del.value());
			return std::unexpected(add_free_res.error());
		}

		this->cache.del(index);

		if (this->last == pos_del.value())
		{
			index_page_pos pos_current = this->last - 1;
			std::size_t try_max = static_cast<std::size_t>(this->last);

			page_result_ref<T> element_last = get_element_ref_by_pos(pos_current);

			while (element_last.has_value() == false && try_max != 0)
			{
				element_last = get_element_ref_by_pos(--pos_current);
				try_max--;
			}

			this->last = pos_current;
		}

		return {};
	}
	constexpr page_result_src<T> get(index_page_order index) noexcept
	{
		page_result_pos pos_get = get_pos_by_order(index);

		if (pos_get.has_value() == false)
			return std::unexpected(pos_get.error());

		page_result_ref<T> element_target = get_element_ref_by_pos(pos_get.value());

		if (element_target.has_value() == false)
			return std::unexpected(element_target.error());

		return element_target.value().get().value().src;
	}
	constexpr page_result_const_src<T> get_const(index_page_order index) const noexcept
	{
		page_result_pos pos_get = get_pos_by_order(index);

		if (pos_get.has_value() == false)
			return std::unexpected(pos_get.error());

		page_result_const_ref<T> element_target = get_element_const_ref_by_pos(pos_get.value());

		if (element_target.has_value() == false)
			return std::unexpected(element_target.error());

		return element_target.value().get().value().src;
	}
	constexpr page_result write(index_page_order index, const T& src_new) noexcept
	{
		page_result_pos pos_get = get_pos_by_order(index);

		if (pos_get.has_value() == false)
			return std::unexpected(pos_get.error());

		page_result_ref<T> element_target = get_element_ref_by_pos(pos_get.value());

		if (element_target.has_value() == false)
			return std::unexpected(element_target.error());

		T& src_ref = element_target.value().get().value().src;

		src_ref = src_new;
		return {};
	}
	page_result cut(page& page_other, index_page_order index_begin, index_page_order index_end, index_page_order index_save_begin)
	{
		if (index_end < index_begin)
			return std::unexpected(page_interrupt_type::index_invalid);

		if (test_validity_index_page_order(index_begin) == false)
			return std::unexpected(page_interrupt_type::index_invalid);

		if (test_validity_index_page_order(index_end) == false)
			return std::unexpected(page_interrupt_type::index_invalid);

		std::size_t loop_len = static_cast<std::size_t>(index_end - index_begin) + 1;

		if (page_other.get_size_left() < loop_len)
			return std::unexpected(page_interrupt_type::size_max);

		index_page_order order_cut_other_current = index_save_begin;
		page_result_ref<T> element_this_current = get_element_ref_by_order(index_begin);

		if (element_this_current.has_value() == false)
			return std::unexpected(page_interrupt_type::element_missing);
		
		while (loop_len > 0)
		{
			if (element_this_current.has_value() == true)
				(void)page_other.insert_front(order_cut_other_current++, element_this_current.value().get().value().src);
			
			element_this_current = get_element_ref_by_pos(element_this_current.value().get().value().next);
			(void)del(index_begin);
			loop_len--;
		}
		
		return {};
	}
	constexpr std::size_t get_size() const noexcept
	{
		return this->set.size() - this->free.size();
	}
	constexpr std::size_t get_size_left() const noexcept
	{
		return MAX - get_size();
	}
	constexpr bool empty() const noexcept
	{
		return get_size() == 0;
	}
	void rebuild()
	{
		return tidy_element(true);
	}
};

using manager_data_type_base_64 = std::uint64_t;

template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
using manager_block = std::pair<std::optional<page<T, MAX / 2, allocator_manager>>, std::optional<page<T, MAX / 2, allocator_manager>>>;

using index_manager_block = manager_data_type_base_64;

constexpr index_manager_block INDEX_MANAGER_BLOCK_END = std::numeric_limits<index_manager_block>::max();
constexpr std::size_t ORDER_MANAGER_BLOCK_ORDER_END = std::numeric_limits<std::size_t>::max();
constexpr std::size_t ORDER_MANAGER_BLOCK_END = std::numeric_limits<std::size_t>::max();
constexpr std::size_t ORDER_MANAGER_END = std::numeric_limits<std::size_t>::max();

enum class manager_error
{
	index_invalid,
	page_invalid,
	order_invalid,
	block_invalid,
	block_invalid_left,
	block_invalid_right,
	block_max_size,
	block_max_size_left,
	block_max_size_right,
	free_empty,
};

using manager_result = std::expected<void, manager_error>;
using manager_result_index = std::expected<index_manager_block, manager_error>;
template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
using manager_result_ref = std::expected<std::reference_wrapper<std::optional<element<manager_block<T, MAX, allocator_manager>, index_manager_block>>>, manager_error>;
template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
using manager_result_ref_const = std::expected<std::reference_wrapper<const std::optional<element<manager_block<T, MAX, allocator_manager>, index_manager_block>>>, manager_error>;
template<typename T>
using manager_result_src_const_ref = std::expected<std::reference_wrapper<const T>, manager_error>;
template<typename T>
using manager_result_src_ref = std::expected<std::reference_wrapper<T>, manager_error>;
template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
using manager_result_const_ref = std::expected<std::reference_wrapper<const std::optional<element<manager_block<T, MAX, allocator_manager>, index_manager_block>>>, manager_error>;

constexpr std::size_t BLOCK_SIZE_DEFAULT = 32768;

template<typename T, std::size_t MAX = BLOCK_SIZE_DEFAULT, typename allocator_manager = std::allocator<T>>
class manager;

template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
class manager_iterator;

template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
class manager_iterator_const
{
	friend manager<T, MAX, allocator_manager>;
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = const T*;
	using reference = const T&;
public:
	std::reference_wrapper<const manager<T, MAX, allocator_manager>> manager_ref;
	std::size_t order_block;
	index_manager_block block_index;
	std::size_t block_order;
	std::size_t order;
private:
	constexpr bool test_validity_iterator() const noexcept
	{
		return this->block_index != INDEX_MANAGER_BLOCK_END;
	}
	constexpr void set_end() noexcept
	{
		this->order_block = ORDER_MANAGER_BLOCK_END;
		this->block_index = INDEX_MANAGER_BLOCK_END;
		this->block_order = ORDER_MANAGER_BLOCK_ORDER_END;
		this->order = ORDER_MANAGER_END;
		return;
	}
public:
    manager_iterator_const(const manager_iterator<T, MAX, allocator_manager>& other) noexcept :
		manager_ref(other.manager_ref), order_block(other.order_block), block_index(other.block_index), block_order(other.block_order), order(other.order)
	{

	}
	manager_iterator_const(const manager_iterator_const<T, MAX, allocator_manager>& other) noexcept :
		manager_ref(other.manager_ref), order_block(other.order_block), block_index(other.block_index), block_order(other.block_order), order(other.order)
	{

	}
	manager_iterator_const(const manager<T, MAX, allocator_manager>& manager_ref_val) noexcept :
		manager_ref(manager_ref_val), order_block(ORDER_MANAGER_BLOCK_END), block_index(INDEX_MANAGER_BLOCK_END), block_order(ORDER_MANAGER_BLOCK_ORDER_END), order(ORDER_MANAGER_END)
	{

	}
	manager_iterator_const(const manager<T, MAX, allocator_manager>& manager_ref_val, std::size_t order_block_val, index_manager_block block_index_val, std::size_t block_order_val, std::size_t order_val) noexcept :
		manager_ref(manager_ref_val), order_block(order_block_val), block_index(block_index_val), block_order(block_order_val), order(order_val)
	{

	}
public:
	constexpr manager_iterator_const<T, MAX, allocator_manager>& operator++() noexcept
	{
		if (test_validity_iterator() == false)
			return *this;

		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		manager_result_const_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref_const(this->block_index, false);

		if (this->block_order + 1 < manager_ref_real.block_get_size(block_target.value().get().value().src))
		{
			this->block_order++;
			this->order++;
			return *this;
		}

		this->block_order = 0;

		while (true)
		{
			index_manager_block block_index_next = block_target.value().get().value().next;

			block_target = manager_ref_real.get_block_ref_const(block_index_next, false);

			if (block_target.has_value() == false)
			{
				set_end();
				break;
			}

			if (manager_ref_real.block_get_size(block_target.value().get().value().src) > 0)
			{
				this->order_block++;
				this->block_index = block_index_next;
				this->order++;
				break;
			}
		}

		return *this;
	}
	constexpr manager_iterator_const<T, MAX, allocator_manager> operator++(int) noexcept
	{
		manager_iterator_const<T, MAX, allocator_manager> it_new = *this;
		++(*this);
		return it_new;
	}
	constexpr manager_iterator_const<T, MAX, allocator_manager>& operator--() noexcept
	{
		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();

		if (test_validity_iterator() == false)
		{
			std::optional<index_manager_block> block_index_end = manager_ref_real.cache.get_back();

			if (block_index_end.has_value() == false)
			{
				set_end();
				return *this;
			}

			manager_result_const_ref<T, MAX, allocator_manager> block_end = manager_ref_real.get_block_ref_const(block_index_end.value(), false);
			std::size_t block_size_last = manager_ref_real.block_get_size(block_end.value().get().value().src);

			if (block_size_last > 0)
			{
				this->order_block = manager_ref_real.block_size - 1;
				this->block_index = block_index_end.value();
				this->block_order = block_size_last - 1;
				this->order = manager_ref_real.element_size - 1;
			}
			else
			{
				set_end();
			}

			return *this;
		}

		manager_result_const_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref_const(this->block_index, false);

		if (this->block_order > 0)
		{
			this->block_order--;
			this->order--;
			return *this;
		}

		while (true)
		{
			index_manager_block block_index_last = block_target.value().get().value().last;

			block_target = manager_ref_real.get_block_ref(block_index_last, false);

			if (block_target.has_value() == false) break;

			std::size_t block_size_last = manager_ref_real.block_get_size(block_target.value().get().value().src);

			if (block_size_last > 0)
			{
				this->order_block--;
				this->block_index = block_index_last;
				this->block_order = block_size_last - 1;
				this->order--;
				break;
			}
		}

		return *this;
	}
	constexpr manager_iterator_const<T, MAX, allocator_manager> operator--(int) noexcept
	{
		manager_iterator_const<T, MAX, allocator_manager> it_new = *this;
		--(*this);
		return it_new;
	}
	constexpr manager_iterator_const<T, MAX, allocator_manager>& operator+=(difference_type add_val) noexcept
	{
		if (add_val < 0)
			return (*this) -= (-add_val);

		if (add_val == 0)
			return *this;

		if (test_validity_iterator() == false)
		{
			set_end();
			return *this;
		}

		std::size_t add_val_real = static_cast<std::size_t>(add_val);
		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		std::size_t index_get = this->order + add_val_real;

		if (index_get >= manager_ref_real.element_size)
		{
			set_end();
			return *this;
		}

		std::size_t order_block_get = ORDER_MANAGER_BLOCK_END;
		std::pair<index_manager_block, std::size_t> res = manager_ref_real.cache_locate.locate_by_index(index_get, order_block_get);

		this->order_block = order_block_get;
		this->block_index = res.first;
		this->block_order = index_get - res.second;
		this->order = index_get;

		return *this;
	}
	constexpr manager_iterator_const<T, MAX, allocator_manager>& operator-=(difference_type sub_val) noexcept
	{
		if (sub_val < 0)
			return (*this) += (-sub_val);

		if (sub_val == 0)
			return *this;

		if (test_validity_iterator() == false)
		{
			set_end();
			return *this;
		}

		std::size_t sub_val_real = static_cast<std::size_t>(sub_val);
		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();

		if (static_cast<std::int64_t>(this->order - sub_val_real) < 0)
		{
			std::pair<index_manager_block, std::size_t> res = manager_ref_real.cache_locate.locate_by_index(0);

			this->order_block = 0;
			this->block_index = res.first;
			this->block_order = 0;
			this->order = 0;

			return *this;
		}

		std::size_t index_get = this->order - sub_val_real;
		std::size_t order_block_get = ORDER_MANAGER_BLOCK_END;
		std::pair<index_manager_block, std::size_t> res = manager_ref_real.cache_locate.locate_by_index(index_get, order_block_get);

		this->order_block = order_block_get;
		this->block_index = res.first;
		this->block_order = index_get - res.second;
		this->order = index_get;

		return *this;
	}
	constexpr manager_iterator_const<T, MAX, allocator_manager> operator+(difference_type add_val) const noexcept
	{
		manager_iterator_const<T, MAX, allocator_manager> it_new(this->manager_ref, this->order_block, this->block_index, this->block_order, this->order);

		it_new += add_val;

		return it_new;
	}
	difference_type operator-(const manager_iterator_const& it_val) const
	{
		if (&it_val.manager_ref.get() != &this->manager_ref.get())
			throw std::logic_error("非同一附属迭代器，无法相减");

		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();

		bool is_end_this = test_validity_iterator();
		bool is_end_other = it_val.test_validity_iterator();

		if (is_end_this == false && is_end_other == false)
			return 0;

		if (is_end_this == false)
			return manager_ref_real.element_size - it_val.order;

		if (is_end_other == false)
			return manager_ref_real.element_size - this->order;

		return this->order - it_val.order;
	}
	constexpr manager_iterator_const operator-(difference_type add_val) const noexcept
	{
		manager_iterator_const<T, MAX, allocator_manager> it_new(this->manager_ref, this->order_block, this->block_index, this->block_order, this->order);

		it_new -= add_val;

		return it_new;
	}
	constexpr const reference operator[](difference_type index) const noexcept
	{
		return *((*this) + index);
	}
	constexpr reference operator[](difference_type index) noexcept
	{
		return *((*this) + index);
	}
	constexpr reference operator*() const noexcept
	{
		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		manager_result_const_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref_const(this->block_index, false);

		return manager_ref_real.block_get_const(block_target.value().get().value().src, this->block_order).value().get();
	}
	constexpr pointer operator->() const noexcept
	{
		const manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		manager_result_const_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref_const(this->block_index, false);

		return &manager_ref_real.block_get_const(block_target.value().get().value().src, this->block_order).value().get();
	}
	bool operator>=(const manager_iterator_const<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order >= it_val.order;
	}
	bool operator>(const manager_iterator_const<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order > it_val.order;
	}
	bool operator<=(const manager_iterator_const<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order <= it_val.order;
	}
	bool operator<(const manager_iterator_const<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order < it_val.order;
	}
	bool operator==(const manager_iterator_const<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		if (this->order_block != it_val.order_block)
			return false;

		if (this->block_index != it_val.block_index)
			return false;

		if (this->block_order != it_val.block_order)
			return false;

		if (this->order != it_val.order)
			return false;

		return true;
	}
	bool operator!=(const manager_iterator_const<T, MAX, allocator_manager>& it_val) const
	{
		return !(*this == it_val);
	}
};

template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
manager_iterator_const<T, MAX, allocator_manager> operator+(typename manager_iterator_const<T, MAX, allocator_manager>::difference_type add_val, const manager_iterator_const<T, MAX, allocator_manager>& it_val)
{
	return it_val + add_val;
}

template<typename T, std::size_t MAX, typename allocator_manager>
class manager_iterator
{
	friend manager<T, MAX, allocator_manager>;
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T*;
	using reference = T&;
public:
	std::reference_wrapper<manager<T, MAX, allocator_manager>> manager_ref;
	std::size_t order_block;
	index_manager_block block_index;
	std::size_t block_order;
	std::size_t order;
private:
	constexpr bool test_validity_iterator() const noexcept
	{
		return this->block_index != INDEX_MANAGER_BLOCK_END;
	}
	constexpr void set_end() noexcept
	{
		this->order_block = ORDER_MANAGER_BLOCK_END;
		this->block_index = INDEX_MANAGER_BLOCK_END;
		this->block_order = ORDER_MANAGER_BLOCK_ORDER_END;
		this->order = ORDER_MANAGER_END;
		return;
	}
public:
	manager_iterator(const manager_iterator<T, MAX, allocator_manager>& other) noexcept :
		manager_ref(other.manager_ref), order_block(other.order_block), block_index(other.block_index), block_order(other.block_order), order(other.order)
	{

	}
	manager_iterator(manager<T, MAX, allocator_manager>& manager_ref_val) noexcept :
		manager_ref(manager_ref_val), order_block(ORDER_MANAGER_BLOCK_END), block_index(INDEX_MANAGER_BLOCK_END), block_order(ORDER_MANAGER_BLOCK_ORDER_END), order(ORDER_MANAGER_END)
	{

	}
	manager_iterator(manager<T, MAX, allocator_manager>& manager_ref_val, std::size_t order_block_val, index_manager_block block_index_val, std::size_t block_order_val, std::size_t order_val) noexcept :
		manager_ref(manager_ref_val), order_block(order_block_val), block_index(block_index_val), block_order(block_order_val), order(order_val)
	{

	}
public:
	constexpr manager_iterator<T, MAX, allocator_manager>& operator++() noexcept
	{
		if (test_validity_iterator() == false)
			return *this;

		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		manager_result_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref(this->block_index, false);

		if (this->block_order + 1 < manager_ref_real.block_get_size(block_target.value().get().value().src))
		{
			this->block_order++;
			this->order++;
			return *this;
		}
	
		this->block_order = 0;

		while (true)
		{
			index_manager_block block_index_next = block_target.value().get().value().next;

			block_target = manager_ref_real.get_block_ref(block_index_next, false);

			if (block_target.has_value() == false)
			{
				set_end();
				break;
			}
				
			if (manager_ref_real.block_get_size(block_target.value().get().value().src) > 0)
			{
				this->order_block++;
				this->block_index = block_index_next;
				this->order++;
				break;
			}		
		}

		return *this;
	}
	constexpr manager_iterator<T, MAX, allocator_manager> operator++(int) noexcept
	{
		manager_iterator<T, MAX, allocator_manager> it_new = *this;
		++(*this);
		return it_new;
	}
	constexpr manager_iterator<T, MAX, allocator_manager>& operator--() noexcept
	{
		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();

		if (test_validity_iterator() == false)
		{
			std::optional<index_manager_block> block_index_end = manager_ref_real.cache.get_back();

			if (block_index_end.has_value() == false)
			{
				set_end();
				return *this;
			}

			manager_result_ref<T, MAX, allocator_manager> block_end = manager_ref_real.get_block_ref(block_index_end.value(), false);
			std::size_t block_size_last = manager_ref_real.block_get_size(block_end.value().get().value().src);

			if (block_size_last > 0)
			{
				this->order_block = manager_ref_real.block_size - 1;
				this->block_index = block_index_end.value();
				this->block_order = block_size_last - 1;
				this->order = manager_ref_real.element_size - 1;
			}
			else
			{
				set_end();
			}

			return *this;
		}

		manager_result_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref(this->block_index, false);
	
		if (this->block_order > 0)
		{
			this->block_order--;
			this->order--;
			return *this;
		}
		
		while (true)
		{
			index_manager_block block_index_last = block_target.value().get().value().last;

			block_target = manager_ref_real.get_block_ref(block_index_last, false);

			if (block_target.has_value() == false) break;
			
			std::size_t block_size_last = manager_ref_real.block_get_size(block_target.value().get().value().src);

			if (block_size_last > 0)
			{
				this->order_block--;
				this->block_index = block_index_last;
				this->block_order = block_size_last - 1;
				this->order--;
				break;
			}
		}

		return *this;
	}
	constexpr manager_iterator<T, MAX, allocator_manager> operator--(int) noexcept
	{
		manager_iterator<T, MAX, allocator_manager> it_new = *this;
		--(*this);
		return it_new;
	}
	constexpr manager_iterator<T, MAX, allocator_manager>& operator+=(difference_type add_val) noexcept
	{
		if (add_val < 0)
			return (*this) -= (-add_val);
		
		if (add_val == 0)
			return *this;

		if (test_validity_iterator() == false)
		{
			set_end();
			return *this;
		}

		std::size_t add_val_real = static_cast<std::size_t>(add_val);
		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		std::size_t index_get = this->order + add_val_real;

		if (index_get >= manager_ref_real.element_size)
		{
			set_end();
			return *this;
		}

		std::size_t order_block_get = ORDER_MANAGER_BLOCK_END;
		std::pair<index_manager_block, std::size_t> res = manager_ref_real.cache_locate.locate_by_index(index_get, order_block_get);

		this->order_block = order_block_get;
		this->block_index = res.first;
		this->block_order = index_get - res.second;
		this->order = index_get;

		return *this;
	}
	constexpr manager_iterator<T, MAX, allocator_manager>& operator-=(difference_type sub_val) noexcept
	{
		if (sub_val < 0)
			return (*this) += (-sub_val);

		if (sub_val == 0)
			return *this;

		if (test_validity_iterator() == false)
		{
			set_end();
			return *this;
		}

		std::size_t sub_val_real = static_cast<std::size_t>(sub_val);
		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		
		if (static_cast<std::int64_t>(this->order - sub_val_real) < 0)
		{
			std::pair<index_manager_block, std::size_t> res = manager_ref_real.cache_locate.locate_by_index(0);

			this->order_block = 0;
			this->block_index = res.first;
			this->block_order = 0;
			this->order = 0;

			return *this;
		}

		std::size_t index_get = this->order - sub_val_real;
		std::size_t order_block_get = ORDER_MANAGER_BLOCK_END;
		std::pair<index_manager_block, std::size_t> res = manager_ref_real.cache_locate.locate_by_index(index_get, order_block_get);

		this->order_block = order_block_get;
		this->block_index = res.first;
		this->block_order = index_get - res.second;
		this->order = index_get;

		return *this;
	}
	constexpr manager_iterator<T, MAX, allocator_manager> operator+(difference_type add_val) const noexcept
	{
		manager_iterator<T, MAX, allocator_manager> it_new(this->manager_ref, this->order_block, this->block_index, this->block_order, this->order);
	
		it_new += add_val;

		return it_new;
	}
	difference_type operator-(const manager_iterator& it_val) const
	{
		if (&it_val.manager_ref.get() != &this->manager_ref.get())
			throw std::logic_error("非同一附属迭代器，无法相减");

		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();

		bool is_end_this = test_validity_iterator();
		bool is_end_other = it_val.test_validity_iterator();

		if (is_end_this == false && is_end_other == false)
			return 0;

		if (is_end_this == false)
			return manager_ref_real.element_size - it_val.order;

		if (is_end_other == false)
			return manager_ref_real.element_size - this->order;

		return this->order - it_val.order;
	}
	constexpr manager_iterator operator-(difference_type add_val) const noexcept
	{
		manager_iterator<T, MAX, allocator_manager> it_new(this->manager_ref, this->order_block, this->block_index, this->block_order, this->order);

		it_new -= add_val;

		return it_new;
	}
	constexpr const reference operator[](difference_type index) const noexcept
	{
		return *((*this) + index);
	}
	constexpr reference operator[](difference_type index) noexcept
	{
		return *((*this) + index);
	}
	constexpr reference operator*() const noexcept
	{
		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		manager_result_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref(this->block_index, false);

		return manager_ref_real.block_get(block_target.value().get().value().src, this->block_order).value().get();
	}
	constexpr pointer operator->() const noexcept
	{
		manager<T, MAX, allocator_manager>& manager_ref_real = this->manager_ref.get();
		manager_result_ref<T, MAX, allocator_manager> block_target = manager_ref_real.get_block_ref(this->block_index, false);

		return &manager_ref_real.block_get(block_target.value().get().value().src, this->block_order).value().get();
	}
	bool operator>=(const manager_iterator<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order >= it_val.order;
	}
	bool operator>(const manager_iterator<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order > it_val.order;
	}
	bool operator<=(const manager_iterator<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order <= it_val.order;
	}
	bool operator<(const manager_iterator<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		return this->order < it_val.order;
	}
	bool operator==(const manager_iterator<T, MAX, allocator_manager>& it_val) const
	{
		if (&this->manager_ref.get() != &it_val.manager_ref.get())
			throw std::runtime_error("非同一附属迭代器，无法比较");

		if (this->order_block != it_val.order_block)
			return false;

		if (this->block_index != it_val.block_index)
			return false;

		if (this->block_order != it_val.block_order)
			return false;

		if (this->order != it_val.order)
			return false;

		return true;
	}
	bool operator!=(const manager_iterator<T, MAX, allocator_manager>& it_val) const
	{
		return !(*this == it_val);
	}
};

template<typename T, std::size_t MAX, typename allocator_manager = std::allocator<T>>
manager_iterator<T, MAX, allocator_manager> operator+(typename manager_iterator<T, MAX, allocator_manager>::difference_type add_val, const manager_iterator<T, MAX, allocator_manager>& it_val)
{
	return it_val + add_val;
}

template <typename T, typename U, typename allocator_cache = std::allocator<T>>
class cache_prefix_mapping
{
	static_assert(std::is_integral_v<T>, "索引类型无效");
	static_assert(std::is_integral_v<U>, "容量类型无效");
public:
	using sum_type = std::common_type_t<U, std::int64_t>;
	using size_type = std::uint64_t;
private:
	struct cache_prefix_mapping_node
	{
		T key;
		U value;
		sum_type sum;
		size_type size;
		std::uint32_t priority;
		cache_prefix_mapping_node* left;
		cache_prefix_mapping_node* right;
		cache_prefix_mapping_node(T k, U v, std::uint32_t p) :
			key(k),
			value(v),
			sum(static_cast<sum_type>(v)),
			size(1),
			priority(p),
			left(nullptr),
			right(nullptr)
		{

		}
	};

	using allocator_traits_node = typename std::allocator_traits<allocator_cache>::template rebind_alloc<cache_prefix_mapping_node>;
	using allocator_traits_mt19937 = typename std::allocator_traits<allocator_cache>::template rebind_alloc<std::mt19937>;

private:
	allocator_traits_node allocator_cache_node;
	allocator_traits_mt19937 allocator_cache_mt19937;
	cache_prefix_mapping_node* root_;
	std::mt19937* rng_;
private:
	constexpr static sum_type get_sum(const cache_prefix_mapping_node* node) noexcept
	{
		return node == nullptr ? static_cast<sum_type>(0) : node->sum;
	}
	constexpr static size_type get_size(const cache_prefix_mapping_node* node) noexcept
	{
		return node == nullptr ? static_cast<size_type>(0) : node->size;
	}
	constexpr static void pull(cache_prefix_mapping_node* node) noexcept
	{
		if (node == nullptr)
			return;

		node->size = get_size(node->left) + 1 + get_size(node->right);
		node->sum = get_sum(node->left) + static_cast<sum_type>(node->value) + get_sum(node->right);
		return;
	}
	static void check_value(const U& value)
	{
		if constexpr (std::is_signed_v<U>)
		{
			if (value < 0)
				throw std::invalid_argument("数值类型必须为非负数");
		}

		return;
	}
	static void check_index_non_negative(const sum_type& index)
	{
		if constexpr (std::is_signed_v<sum_type>)
		{
			if (index < 0)
				throw std::out_of_range("索引无效");
		}

		return;
	}
	constexpr static cache_prefix_mapping_node* merge(cache_prefix_mapping_node* left, cache_prefix_mapping_node* right) noexcept
	{
		if (left == nullptr)
			return right;

		if (right == nullptr)
			return left;


		if (left->priority > right->priority)
		{
			left->right = merge(left->right, right);
			pull(left);
			return left;
		}

		right->left = merge(left, right->left);
		pull(right);
		return right;
	}
	constexpr static void split(cache_prefix_mapping_node* node, size_type left_count, cache_prefix_mapping_node*& left, cache_prefix_mapping_node*& right) noexcept
	{
		if (node == nullptr)
		{
			left = nullptr;
			right = nullptr;
			return;
		}

		const size_type left_size = get_size(node->left);

		if (left_count <= left_size)
		{
			split(node->left, left_count, left, node->left);
			right = node;
			pull(right);
		}
		else
		{
			split(node->right, left_count - left_size - 1, node->right, right);
			left = node;
			pull(left);
		}

		return;
	}
	static void destroy(allocator_traits_node* allocator, cache_prefix_mapping_node* node) noexcept
	{
		if (node == nullptr)
			return;

		destroy(allocator, node->left);
		destroy(allocator, node->right);

		std::destroy_at(node);
		std::allocator_traits<allocator_traits_node>::deallocate(*allocator, node, 1);

		return;
	}
	constexpr static cache_prefix_mapping_node* kth_node(cache_prefix_mapping_node* node, size_type pos) noexcept
	{
		while (node != nullptr)
		{
			const size_type left_size = get_size(node->left);

			if (pos < left_size)
			{
				node = node->left;
			}
			else if (pos == left_size)
			{
				return node;
			}
			else
			{
				pos -= left_size + 1;
				node = node->right;
			}
		}

		return nullptr;
	}
	constexpr static const cache_prefix_mapping_node* kth_node(const cache_prefix_mapping_node* node, size_type pos) noexcept
	{
		while (node != nullptr)
		{
			const size_type left_size = get_size(node->left);

			if (pos < left_size)
			{
				node = node->left;
			}
			else if (pos == left_size)
			{
				return node;
			}
			else
			{
				pos -= left_size + 1;
				node = node->right;
			}
		}

		return nullptr;
	}
	constexpr static bool update_node(cache_prefix_mapping_node* node, size_type pos, const U& value) noexcept
	{
		if (node == nullptr)
			return false;

		const size_type left_size = get_size(node->left);
		bool ok = false;

		if (pos < left_size)
		{
			ok = update_node(node->left, pos, value);
		}
		else if (pos == left_size)
		{
			node->value = value;
			ok = true;
		}
		else
		{
			ok = update_node(node->right, pos - left_size - 1, value);
		}

		pull(node);
		return ok;
	}
	static cache_prefix_mapping_node* clone_tree(allocator_traits_node* allocator, const cache_prefix_mapping_node* node)
	{
		if (node == nullptr)
			return nullptr;

		cache_prefix_mapping_node* new_node = allocator->allocate(1);

		try
		{
			std::construct_at(new_node, node->key, node->value, node->priority);

			new_node->sum = node->sum;
			new_node->size = node->size;

			new_node->left = clone_tree(allocator, node->left);
			new_node->right = clone_tree(allocator, node->right);
		}
		catch (...)
		{
			destroy(allocator, new_node);
			throw;
		}

		return new_node;
	}
	void init_rng()
	{
		this->rng_ = this->allocator_cache_mt19937.allocate(1);

		try
		{
			std::construct_at(this->rng_, std::random_device{}());
		}
		catch (...)
		{
			std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, this->rng_, 1);
			this->rng_ = nullptr;
			throw;
		}

		return;
	}
public:
	cache_prefix_mapping() :
		allocator_cache_node(), allocator_cache_mt19937(), root_(nullptr), rng_(nullptr)
	{
		init_rng();
	}
	explicit cache_prefix_mapping(const allocator_cache& allocator_val) :
		allocator_cache_node(allocator_val), allocator_cache_mt19937(allocator_val), root_(nullptr), rng_(nullptr)
	{
		init_rng();
	}
	cache_prefix_mapping(const cache_prefix_mapping&) = delete;
	cache_prefix_mapping& operator=(const cache_prefix_mapping&) = delete;
	cache_prefix_mapping(cache_prefix_mapping&& other) noexcept :
		allocator_cache_node(std::move(other.allocator_cache_node)), allocator_cache_mt19937(std::move(other.allocator_cache_mt19937)), root_(other.root_), rng_(other.rng_)
	{
		other.root_ = nullptr;
		other.rng_ = nullptr;
	}
	cache_prefix_mapping& operator=(cache_prefix_mapping&& other)
	{
		if (this == &other)
			return *this;

		if constexpr (std::allocator_traits<allocator_cache>::propagate_on_container_move_assignment::value == true)
		{
			destroy(&this->allocator_cache_node, this->root_);
			this->root_ = nullptr;

			if (this->rng_ != nullptr)
			{
				std::destroy_at(this->rng_);
				std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, this->rng_, 1);

				this->rng_ = nullptr;
			}

			this->allocator_cache_node = std::move(other.allocator_cache_node);
			this->allocator_cache_mt19937 = std::move(other.allocator_cache_mt19937);

			this->root_ = std::exchange(other.root_, nullptr);
			this->rng_ = std::exchange(other.rng_, nullptr);
		}
		else
		{
			if (this->allocator_cache_node == other.allocator_cache_node && this->allocator_cache_mt19937 == other.allocator_cache_mt19937)
			{
				destroy(&this->allocator_cache_node, this->root_);
				this->root_ = nullptr;

				if (this->rng_ != nullptr)
				{
					std::destroy_at(this->rng_);

					std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, this->rng_, 1);
					this->rng_ = nullptr;
				}

				this->root_ = std::exchange(other.root_, nullptr);
				this->rng_ = std::exchange(other.rng_, nullptr);
			}
			else
			{
				cache_prefix_mapping_node* new_root = nullptr;
				std::mt19937* new_rng = nullptr;

				try
				{
					new_root = clone_tree(&this->allocator_cache_node, other.root_);

					if (other.rng_ != nullptr)
					{
						new_rng = std::allocator_traits<allocator_traits_mt19937>::allocate(this->allocator_cache_mt19937, 1);

						std::construct_at(new_rng, std::move(*other.rng_));
					}
				}
				catch (...)
				{
					destroy(&this->allocator_cache_node, new_root);

					if (new_rng != nullptr)
					{
						std::destroy_at(new_rng);

						std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, new_rng, 1);
					}

					throw;
				}

				destroy(&this->allocator_cache_node, this->root_);
				this->root_ = new_root;

				if (this->rng_ != nullptr)
				{
					std::destroy_at(this->rng_);

					std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, this->rng_, 1);
				}

				this->rng_ = new_rng;

				destroy(&other.allocator_cache_node, other.root_);
				other.root_ = nullptr;

				if (other.rng_ != nullptr)
				{
					std::destroy_at(other.rng_);
					std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, other.rng_, 1);

					other.rng_ = nullptr;
				}
			}
		}

		return *this;
	}
	~cache_prefix_mapping() noexcept
	{
		destroy(&this->allocator_cache_node, this->root_);

		std::destroy_at(this->rng_);
		std::allocator_traits<allocator_traits_mt19937>::deallocate(this->allocator_cache_mt19937, this->rng_, 1);
	}
public:
	constexpr bool empty() const noexcept
	{
		return this->root_ == nullptr;
	}
	constexpr size_type size() const noexcept
	{
		return get_size(this->root_);
	}
	constexpr sum_type total_sum() const noexcept
	{
		return get_sum(this->root_);
	}
	void clear() noexcept
	{
		destroy(&this->allocator_cache_node, this->root_);
		this->root_ = nullptr;
		return;
	}
	void push_back(const T& key, const U& value)
	{
		insert(size(), key, value);
		return;
	}
	void insert(size_type pos, const T& key, const U& value)
	{
		check_value(value);

		if (pos > size())
			throw std::out_of_range("插入位置无效");

		if (this->rng_ == nullptr)		
		{
			this->rng_ = std::allocator_traits<allocator_traits_mt19937>::allocate(this->allocator_cache_mt19937, 1);

			std::construct_at(this->rng_, std::random_device{}());
		}

		cache_prefix_mapping_node* left = nullptr;
		cache_prefix_mapping_node* right = nullptr;
		split(this->root_, pos, left, right);

		cache_prefix_mapping_node* mid = std::allocator_traits<allocator_traits_node>::allocate(this->allocator_cache_node, 1);
		std::construct_at(mid, key, value, (*this->rng_)());

		this->root_ = merge(merge(left, mid), right);

		return;
	}
	bool erase(size_type pos) noexcept
	{
		if (pos >= size())
			return false;

		cache_prefix_mapping_node* left = nullptr;
		cache_prefix_mapping_node* mid_right = nullptr;
		cache_prefix_mapping_node* mid = nullptr;
		cache_prefix_mapping_node* right = nullptr;

		split(this->root_, pos, left, mid_right);
		split(mid_right, 1, mid, right);

		destroy(&this->allocator_cache_node, mid);
		this->root_ = merge(left, right);
		return true;
	}
	bool update(size_type pos, const U& value)
	{
		check_value(value);

		if (pos >= size())
			return false;

		return update_node(this->root_, pos, value);
	}
	constexpr bool get(size_type pos, T& out_key, U& out_value) const noexcept
	{
		const cache_prefix_mapping_node* node = kth_node(this->root_, pos);

		if (node == nullptr)
			return false;

		out_key = node->key;
		out_value = node->value;
		return true;
	}
	sum_type prefix_before_pos(size_type pos) const
	{
		if (pos > size())
			throw std::out_of_range("位置无效");

		const cache_prefix_mapping_node* node = this->root_;
		sum_type result = 0;

		while (node != nullptr)
		{
			const size_type left_size = get_size(node->left);

			if (pos < left_size)
			{
				node = node->left;
			}
			else if (pos == left_size)
			{
				result += get_sum(node->left);
				return result;
			}
			else
			{
				result += get_sum(node->left) + static_cast<sum_type>(node->value);
				pos -= left_size + 1;
				node = node->right;
			}
		}

		return result;
	}
	std::pair<T, sum_type> locate_by_index(sum_type index, size_type& pos) const
	{
		check_index_non_negative(index);

		if (index >= total_sum())
			throw std::out_of_range("索引无效");

		const cache_prefix_mapping_node* node = this->root_;
		sum_type prefix = 0;
		size_type skipped_size = 0;

		while (node != nullptr)
		{
			const sum_type left_sum = get_sum(node->left);
			const size_type left_size = get_size(node->left);

			const sum_type current_begin = prefix + left_sum;
			const sum_type current_end = current_begin + static_cast<sum_type>(node->value);

			if (index < current_begin)
			{
				node = node->left;
			}
			else if (index < current_end)
			{
				pos = skipped_size + left_size;
				return { node->key, current_begin };
			}
			else
			{
				prefix = current_end;
				skipped_size += left_size + 1;
				node = node->right;
			}
		}

		throw std::logic_error("无法找到");
	}
	std::pair<T, sum_type> locate_by_index(sum_type index) const
	{
		size_type ignored_pos = 0;
		return locate_by_index(index, ignored_pos);
	}
};

template<typename T, std::size_t MAX, typename allocator_manager>
class manager
{
	static_assert(MAX % 2 == 0, "MAX 必须为偶数");
public:
	friend manager_iterator<T, MAX, allocator_manager>;
	friend manager_iterator_const<T, MAX, allocator_manager>;
	using iterator = manager_iterator<T, MAX, allocator_manager>;
	using const_iterator = manager_iterator_const<T, MAX, allocator_manager>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
	using allocator_manager_set = typename std::allocator_traits<allocator_manager>::template rebind_alloc<std::optional<element<manager_block<T, MAX, allocator_manager>, index_manager_block>>>;
	using allocator_manager_free = typename std::allocator_traits<allocator_manager>::template rebind_alloc<index_manager_block>;
	using allocator_manager_cache = typename std::allocator_traits<allocator_manager>::template rebind_alloc<index_manager_block>;

private:
	allocator_manager allocator_;
	std::vector<std::optional<element<manager_block<T, MAX, allocator_manager>, index_manager_block>>, allocator_manager_set> set;
	std::vector<index_manager_block, allocator_manager_free> free;
	cache_mapping<std::size_t, index_manager_block, std::numeric_limits<index_manager_block>::max() - 1, allocator_manager_cache> cache;
	cache_prefix_mapping<index_manager_block, std::size_t, allocator_manager> cache_locate;
	std::size_t block_size = 0;
	std::size_t element_size = 0;
private:
	constexpr bool test_validity_order(std::size_t order) const noexcept
	{
		return this->set.size() == 0 ? false : order < this->element_size;
	}
	constexpr bool test_validity_index_block(index_manager_block index, bool is_test_free) const noexcept
	{
		std::size_t index_test = static_cast<std::size_t>(index);

		if (index_test >= this->set.size())
			return false;

		return is_test_free == false ? true : this->set[index_test].has_value() == true;
	}
	constexpr manager_result_ref<T, MAX, allocator_manager> get_block_ref(index_manager_block index, bool is_get_free) noexcept
	{
		if (test_validity_index_block(index, !is_get_free) == false)
			return std::unexpected(manager_error::index_invalid);

		return this->set[static_cast<std::size_t>(index)];
	}
	constexpr manager_result_ref_const<T, MAX, allocator_manager> get_block_ref_const(index_manager_block index, bool is_get_free) const noexcept
	{
		if (test_validity_index_block(index, !is_get_free) == false)
			return std::unexpected(manager_error::index_invalid);

		return this->set[static_cast<std::size_t>(index)];
	}
	manager_result_index add_block()
	{
		manager_result_index index_free = get_free();
		std::optional<index_manager_block> block_index_last = this->cache.get_back();
		index_manager_block index_new = INDEX_MANAGER_BLOCK_END;
		index_manager_block index_last = block_index_last.has_value() == false ? INDEX_MANAGER_BLOCK_END : block_index_last.value();

		if (index_free.has_value() == false)
		{
			index_new = static_cast<index_manager_block>(this->set.size());

			this->set.emplace_back(std::in_place, index_last, INDEX_MANAGER_BLOCK_END, page<T, MAX / 2, allocator_manager>(this->allocator_), std::nullopt);
		}
		else
		{
			index_new = index_free.value();

			manager_result_ref<T, MAX, allocator_manager> block_free = get_block_ref(index_new, true);

			if (block_free.has_value() == false)
			{
				(void)add_free(index_new);
				return std::unexpected(block_free.error());
			}

			block_free.value().get().emplace(index_last, INDEX_MANAGER_BLOCK_END, page<T, MAX / 2, allocator_manager>(this->allocator_), std::nullopt);
		}

		if (block_index_last.has_value() == true)
		{
			manager_result_ref<T, MAX, allocator_manager> block_last = get_block_ref(block_index_last.value(), false);

			if (block_last.has_value() == true) block_last.value().get().value().next = index_new;
		}

		this->cache.add_set(this->block_size, index_new);
		this->cache_locate.push_back(index_new, 0);
		this->block_size++;
		return index_new;
	}
	manager_result_index insert_front_block(std::size_t index)
	{
		std::optional<index_manager_block> index_insert = this->cache.get(index);

		if (index_insert.has_value() == false)
			return add_block();
		
		manager_result_ref<T, MAX, allocator_manager> block_insert = get_block_ref(index_insert.value(), false);

		if (block_insert.has_value() == false)
			return std::unexpected(block_insert.error());

		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_ref = block_insert.value().get().value();
		index_manager_block index_last = block_insert_ref.last;
		index_manager_block index_next = index_insert.value();
		manager_result_index index_free = get_free();
		index_manager_block index_new = INDEX_MANAGER_BLOCK_END;

		if (index_free.has_value() == false)
		{
			index_new = static_cast<index_manager_block>(this->set.size());
			block_insert_ref.last = index_new;

			this->set.emplace_back(std::in_place, index_last, index_next, page<T, MAX / 2, allocator_manager>(this->allocator_), std::nullopt);
		}
		else
		{
			index_new = index_free.value();
			
			manager_result_ref<T, MAX, allocator_manager> block_free = get_block_ref(index_new, true);

			if (block_free.has_value() == false)
			{
				add_free(index_new);
				return std::unexpected(block_free.error());
			}

			block_insert_ref.last = index_new;

			block_free.value().get().emplace(index_last, index_next, page<T, MAX / 2, allocator_manager>(this->allocator_), std::nullopt);
		}

		manager_result_ref<T, MAX, allocator_manager> block_insert_last = get_block_ref(index_last, false);

		if (block_insert_last.has_value() == true)
			block_insert_last.value().get().value().next = index_new;

		this->cache.insert_front(index, index_new);
		this->cache_locate.insert(index, index_new, 0);
		this->block_size++;
		return index_new;
	}
	manager_result_index insert_back_block(std::size_t index)
	{
		std::optional<index_manager_block> index_insert = this->cache.get(index);

		if (index_insert.has_value() == false)
			return add_block();

		manager_result_ref<T, MAX, allocator_manager> block_insert = get_block_ref(index_insert.value(), false);

		if (block_insert.has_value() == false)
			return std::unexpected(block_insert.error());

		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_ref = block_insert.value().get().value();
		index_manager_block index_last = index_insert.value();
		index_manager_block index_next = block_insert_ref.next;
		manager_result_index index_free = get_free();
		index_manager_block index_new = INDEX_MANAGER_BLOCK_END;

		if (index_free.has_value() == false)
		{
			index_new = static_cast<index_manager_block>(this->set.size());
			block_insert_ref.next = index_new;

			this->set.emplace_back(std::in_place, index_last, index_next, page<T, MAX / 2, allocator_manager>(this->allocator_), std::nullopt);
		}
		else
		{
			index_new = index_free.value();

			manager_result_ref<T, MAX, allocator_manager> block_free = get_block_ref(index_new, true);

			if (block_free.has_value() == false)
			{
				(void)add_free(index_new);
				return std::unexpected(block_free.error());
			}

			block_insert_ref.next = index_new;

			block_free.value().get().emplace(index_last, index_next, page<T, MAX / 2, allocator_manager>(this->allocator_), std::nullopt);
		}

		manager_result_ref<T, MAX, allocator_manager> block_insert_next = get_block_ref(index_next, false);

		if (block_insert_next.has_value() == true)
			block_insert_next.value().get().value().last = index_new;

		this->cache.insert_back(index, index_new);
		this->cache_locate.insert(index + 1, index_new, 0);
		this->block_size++;
		return index_new;
	}
	manager_result add_free(index_manager_block index)
	{
		manager_result_ref<T, MAX, allocator_manager> block_del_target = get_block_ref(index, false);

		if (block_del_target.has_value() == false)
			return std::unexpected(block_del_target.error());

		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_del_ref = block_del_target.value().get().value();
		manager_result_ref<T, MAX, allocator_manager> block_del_last = get_block_ref(block_del_ref.last, false);

		if (block_del_last.has_value() == true)
			block_del_last.value().get().value().next = block_del_ref.next;

		manager_result_ref<T, MAX, allocator_manager> block_del_next = get_block_ref(block_del_ref.next, false);

		if (block_del_next.has_value() == true)
			block_del_next.value().get().value().last = block_del_ref.last;

		block_del_target.value().get().reset();

		this->free.emplace_back(index);
		return {};	
	}
	manager_result_index get_free() noexcept
	{
		if (this->free.empty() == true)
			return std::unexpected(manager_error::free_empty);

		index_manager_block index_free = this->free.back();

		this->free.pop_back();

		return index_free;
	}
	template<typename...params>
    manager_result_src_ref<T> block_add(std::size_t index, manager_block<T, MAX, allocator_manager>& block, params&&...src_new)
	{
		if (block.second.has_value() == false)
		{
			if (block.first.value().get_size_left() > 0)
			{
				page_result_src<T> res_add = block.first.value().add(std::forward<params>(src_new)...);

				if (res_add.has_value() == false)
				{
					if(res_add.error() != page_interrupt_type::size_max)
						return std::unexpected(manager_error::block_invalid_left);
				}

				this->cache_locate.update(index, block_get_size(block));
				return res_add.value();
			}

			block.second.emplace(page<T, MAX / 2, allocator_manager>(this->allocator_));

			page_result_src<T> res_add = block.second.value().add(std::forward<params>(src_new)...);

			if (res_add.has_value() == false)
				return res_add.error() == page_interrupt_type::size_max ? std::unexpected(manager_error::block_max_size) : std::unexpected(manager_error::block_invalid_right);

			this->cache_locate.update(index, block_get_size(block));
			return res_add.value();
		}

		page_result_src<T> res_add = block.second.value().add(std::forward<params>(src_new)...);

		if (res_add.has_value() == false)
			return res_add.error() == page_interrupt_type::size_max ? std::unexpected(manager_error::block_max_size) : std::unexpected(manager_error::block_invalid_right);

		this->cache_locate.update(index, block_get_size(block));
		return res_add.value();
	}
	manager_result_src_ref<T> block_insert_front(std::size_t index, manager_block<T, MAX, allocator_manager>& block, std::size_t order, const T& src_new)
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			index_page_order page_order_insert = static_cast<index_page_order>(order);
			page_result_src<T> res_insert = page_left.insert_front(page_order_insert, src_new);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_left);

				if (block.second.has_value() == false)
					block.second.emplace(page<T, MAX / 2, allocator_manager>(this->allocator_));

				page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
				index_page_order order_cut = static_cast<index_page_order>(page_left.get_size() - 1);
				page_result res_cut = page_left.cut(page_right, order_cut, order_cut, 0);

				if (res_cut.has_value() == false)
				{
					if (res_cut.error() != page_interrupt_type::size_max)
						return std::unexpected(manager_error::block_invalid_right);

					return std::unexpected(manager_error::block_max_size_right);				
				}
					
				res_insert = page_left.insert_front(page_order_insert, src_new);

				if (res_insert.has_value() == false)
					return std::unexpected(manager_error::block_invalid_left);
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_insert = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_insert < page_right.get_size())
		{
			page_result_src<T> res_insert = page_right.insert_front(page_order_insert, src_new);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_right);

				return std::unexpected(manager_error::block_max_size_right);
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		return std::unexpected(manager_error::order_invalid);
	}
	template<typename...params>
	manager_result_src_ref<T> block_insert_front(std::size_t index, manager_block<T, MAX, allocator_manager>& block, std::size_t order, params&&...src_new)
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			auto param_store = std::make_tuple(std::forward<params>(src_new)...);
			index_page_order page_order_insert = static_cast<index_page_order>(order);
			page_result_src<T> res_insert = std::apply
			(
				[&](auto&&...param_old)
				{
					return page_left.insert_front(page_order_insert, std::forward<decltype(param_old)>(param_old)...);
				},
				param_store
			);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_left);

				if (block.second.has_value() == false)
					block.second.emplace(page<T, MAX / 2, allocator_manager>(this->allocator_));

				page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
				index_page_order order_cut = static_cast<index_page_order>(page_left.get_size() - 1);
				page_result res_cut = page_left.cut(page_right, order_cut, order_cut, 0);

				if (res_cut.has_value() == false)
				{
					if (res_cut.error() != page_interrupt_type::size_max)
						return std::unexpected(manager_error::block_invalid_right);

					return std::unexpected(manager_error::block_max_size_right);
				}

				res_insert = std::apply
				(
					[&](auto&&...param_old)
					{
						return page_left.insert_front(page_order_insert, std::forward<decltype(param_old)>(param_old)...);
					},
					std::move(param_store)
				);


				if (res_insert.has_value() == false)
					return std::unexpected(manager_error::block_invalid_left);
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_insert = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_insert < page_right.get_size())
		{
			page_result_src<T> res_insert = page_right.insert_front(page_order_insert, std::forward<params>(src_new)...);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_right);

				return std::unexpected(manager_error::block_max_size_right);
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		return std::unexpected(manager_error::order_invalid);
	}
	manager_result_src_ref<T> block_insert_back(std::size_t index, manager_block<T, MAX, allocator_manager>& block, std::size_t order, const T& src_new)
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			index_page_order page_order_insert = static_cast<index_page_order>(order);
			page_result_src<T> res_insert = page_left.insert_back(page_order_insert, src_new);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_left);

				if (block.second.has_value() == false)
					block.second.emplace(page<T, MAX / 2, allocator_manager>(this->allocator_));

				page<T, MAX / 2, allocator_manager>& page_right = block.second.value();

				if (page_order_insert == static_cast<index_page_order>(page_left.get_size() - 1))
				{
					res_insert = page_right.insert_front(0, src_new);

					if (res_insert.has_value() == false)
					{
						if (res_insert.error() != page_interrupt_type::size_max)
							return std::unexpected(manager_error::block_invalid_right);

						return std::unexpected(manager_error::block_max_size_right);
					}
				}
				else
				{
					index_page_order order_cut = static_cast<index_page_order>(page_left.get_size() - 1);
					page_result res_cut = page_left.cut(page_right, order_cut, order_cut, 0);

					if (res_cut.has_value() == false)
					{
						if (res_cut.error() != page_interrupt_type::size_max)
							return std::unexpected(manager_error::block_invalid_right);

						return std::unexpected(manager_error::block_max_size_right);
					}

					res_insert = page_left.insert_back(page_order_insert, src_new);

					if (res_insert.has_value() == false)
						return std::unexpected(manager_error::block_invalid_left);
				}
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_insert = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_insert < page_right.get_size())
		{
			page_result_src<T> res_insert = page_right.insert_back(page_order_insert, src_new);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_right);

				return std::unexpected(manager_error::block_max_size_right);
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		return std::unexpected(manager_error::order_invalid);
	}
	template<typename...params>
	manager_result_src_ref<T> block_insert_back(std::size_t index, manager_block<T, MAX, allocator_manager>& block, std::size_t order, params&&...src_new)
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			auto param_store = std::make_tuple(std::forward<params>(src_new)...);
			index_page_order page_order_insert = static_cast<index_page_order>(order);
			page_result_src<T> res_insert = std::apply
			(
				[&](auto&&...param_old)
				{
					return page_left.insert_back(page_order_insert, std::forward<decltype(param_old)>(param_old)...);
				},
				param_store
			);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_left);

				if (block.second.has_value() == false)
					block.second.emplace(page<T, MAX / 2, allocator_manager>(this->allocator_));

				page<T, MAX / 2, allocator_manager>& page_right = block.second.value();

				if (page_order_insert == static_cast<index_page_order>(page_left.get_size() - 1))
				{
					res_insert = std::apply
					(
						[&](auto&&...param_old)
						{
							return page_right.insert_front(0, std::forward<decltype(param_old)>(param_old)...);
						},
						std::move(param_store)
					);

					if (res_insert.has_value() == false)
					{
						if (res_insert.error() != page_interrupt_type::size_max)
							return std::unexpected(manager_error::block_invalid_right);

						return std::unexpected(manager_error::block_max_size_right);
					}
				}
				else
				{
					index_page_order order_cut = static_cast<index_page_order>(page_left.get_size() - 1);
					page_result res_cut = page_left.cut(page_right, order_cut, order_cut, 0);

					if (res_cut.has_value() == false)
					{
						if (res_cut.error() != page_interrupt_type::size_max)
							return std::unexpected(manager_error::block_invalid_right);

						return std::unexpected(manager_error::block_max_size_right);
					}

					res_insert = std::apply
					(
						[&](auto&&...param_old) 
						{
							return page_left.insert_front(0, std::forward<decltype(param_old)>(param_old)...);
						}, 
						std::move(param_store)
					);

					if (res_insert.has_value() == false)
						return std::unexpected(manager_error::block_invalid_left);
				}
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_insert = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_insert < page_right.get_size())
		{
			page_result_src<T> res_insert = page_right.insert_back(page_order_insert, std::forward<params>(src_new)...);

			if (res_insert.has_value() == false)
			{
				if (res_insert.error() != page_interrupt_type::size_max)
					return std::unexpected(manager_error::block_invalid_right);

				return std::unexpected(manager_error::block_max_size_right);
			}

			this->cache_locate.update(index, block_get_size(block));
			return res_insert.value();
		}

		return std::unexpected(manager_error::order_invalid);
	}
	constexpr manager_result_src_const_ref<T> block_get_const(const manager_block<T, MAX, allocator_manager>& block, std::size_t order) const noexcept
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		const page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			index_page_order page_order_get = static_cast<index_page_order>(order);
			page_result_const_src<T> src_target = page_left.get_const(page_order_get);

			if (src_target.has_value() == true)
				return src_target.value().get();
				
			return std::unexpected(manager_error::order_invalid);
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		const page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_get = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_get < page_right.get_size())
		{
			page_result_const_src<T> src_target = page_right.get_const(page_order_get);

			if (src_target.has_value() == true)
				return src_target.value().get();
		}

		return std::unexpected(manager_error::order_invalid);
	}
	constexpr manager_result_src_ref<T> block_get(manager_block<T, MAX, allocator_manager>& block, std::size_t order) noexcept
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			index_page_order page_order_get = static_cast<index_page_order>(order);
			page_result_src<T> src_target = page_left.get(page_order_get);

			if (src_target.has_value() == true)
				return src_target.value().get();

			return std::unexpected(manager_error::order_invalid);
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_get = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_get < page_right.get_size())
		{
			page_result_src<T> src_target = page_right.get(page_order_get);

			if (src_target.has_value() == true)
				return src_target.value().get();
		}

		return std::unexpected(manager_error::order_invalid);
	}
	constexpr std::size_t block_get_size(const manager_block<T, MAX, allocator_manager>& block) const noexcept
	{
		std::size_t size_totoal = 0;

		if (block.first.has_value() == true) size_totoal += block.first.value().get_size();
		if (block.second.has_value() == true) size_totoal += block.second.value().get_size();

		return size_totoal;
	}
	constexpr manager_result block_write(manager_block<T, MAX, allocator_manager>& block, std::size_t order, const T& src_new) noexcept
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			index_page_order page_order_write = static_cast<index_page_order>(order);

			if (page_left.write(page_order_write, src_new).has_value() == false)
				return std::unexpected(manager_error::order_invalid);

			return {};
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_write = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_write < page_right.get_size())
		{
			if (page_right.write(page_order_write, src_new).has_value() == false)
				return std::unexpected(manager_error::order_invalid);

			return {};
		}

		return std::unexpected(manager_error::order_invalid);
	}
	manager_result block_del(std::size_t index, manager_block<T, MAX, allocator_manager>& block, std::size_t order)
	{
		if (block.first.has_value() == false)
			return std::unexpected(manager_error::block_invalid_left);

		page<T, MAX / 2, allocator_manager>& page_left = block.first.value();

		if (order < page_left.get_size())
		{
			index_page_order page_order_del = static_cast<index_page_order>(order);

			if (page_left.del(page_order_del).has_value() == true)
			{
				this->cache_locate.update(index, block_get_size(block));
				return {};
			}
			
			return std::unexpected(manager_error::order_invalid);
		}

		if (block.second.has_value() == false)
			return std::unexpected(manager_error::order_invalid);

		page<T, MAX / 2, allocator_manager>& page_right = block.second.value();
		index_page_order page_order_del = static_cast<index_page_order>(order - page_left.get_size());

		if (page_order_del < page_right.get_size())
		{
			if (page_right.del(page_order_del).has_value() == true)
			{
				this->cache_locate.update(index, block_get_size(block));
				return {};
			}
		}

		return std::unexpected(manager_error::order_invalid);
	}
	constexpr manager_result_index get_index_block_by_oder(std::size_t order, std::size_t& order_left_save) const
	{
		if (test_validity_order(order) == false)
			return std::unexpected(manager_error::order_invalid);

		std::pair<index_manager_block, std::size_t> res = this->cache_locate.locate_by_index(order);

		order_left_save = order - res.second;

		return res.first;
	}
public:
	manager() :
		manager(allocator_manager())
	{

	}
	explicit manager(const allocator_manager& allocator_val) :
		allocator_(allocator_val), set(allocator_val), free(allocator_val), cache(allocator_val), cache_locate(allocator_val), block_size(0), element_size(0)
	{

	}
public:
	void erase(const const_iterator& it_erase)
	{
		manager_result_ref<T, MAX, allocator_manager> block_del_target = get_block_ref(it_erase.block_index, false);

		if (block_del_target.has_value() == false)
			return;

		if (block_del(it_erase.order_block, block_del_target.value().get().value().src, it_erase.block_order).has_value() == false)
			return;

		block_del_target = get_block_ref(it_erase.block_index, false);

		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_del_ref = block_del_target.value().get().value();
		page<T, MAX / 2, allocator_manager>& block_left = block_del_ref.src.first.value();

		if (block_del_ref.src.second.has_value() == true)
		{
			page<T, MAX / 2, allocator_manager>& block_right = block_del_ref.src.second.value();

			if (block_right.get_size() == 0)
				block_del_ref.src.second.reset();
		}

		if (block_left.get_size() == 0)
			block_del_ref.src.first = std::exchange(block_del_ref.src.second, std::nullopt);

		if (block_del_ref.src.first.has_value() == false)
		{
			(void)add_free(it_erase.block_index);
			this->cache_locate.erase(it_erase.order_block);
			this->cache.del(it_erase.order_block);
			this->block_size--;
		}

		this->element_size--;
		return;
	}
	void erase(const iterator& it_erase)
	{
		return erase(const_iterator(it_erase));
	}
	void erase(const const_reverse_iterator& it_erase)
	{
		return erase(it_erase.base());
	}
	void erase(const reverse_iterator& it_erase)
	{
		return erase(const_iterator(it_erase.base()));
	}
	const_iterator::reference at(std::size_t order) const
	{
		std::size_t order_left = 0;
		manager_result_index index_get = get_index_block_by_oder(order, order_left);

		if (index_get.has_value() == false)
			throw std::runtime_error("索引无效");

		manager_result_ref_const<T, MAX, allocator_manager> block_get_target = get_block_ref_const(index_get.value(), false);

		if (block_get_target.has_value() == false)
			throw std::runtime_error("索引无效");

		const element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_get_ref = block_get_target.value().get().value();
		manager_result_src_const_ref<T> src_target = block_get_const(block_get_ref.src, order_left);

		if (src_target.has_value() == false)
			throw std::runtime_error("索引无效");

		return src_target.value().get();
	}
	iterator::reference at(std::size_t order)
	{
		std::size_t order_left = 0;
		manager_result_index index_get = get_index_block_by_oder(order, order_left);

		if (index_get.has_value() == false)
			throw std::runtime_error("索引无效");

		manager_result_ref<T, MAX, allocator_manager> block_get_target = get_block_ref(index_get.value(), false);

		if (block_get_target.has_value() == false)
			throw std::runtime_error("索引无效");

		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_get_ref = block_get_target.value().get().value();
		manager_result_src_ref<T> src_target = block_get(block_get_ref.src, order_left);

		if (src_target.has_value() == false)
			throw std::runtime_error("索引无效");

		return src_target.value().get();
	}
	constexpr iterator begin() noexcept
	{
		std::optional<index_manager_block> block_index_begin = this->cache.get_front();

		if (block_index_begin.has_value() == false) return iterator(*this);

		return iterator(*this, 0, block_index_begin.value(), 0, 0);
	}
	constexpr iterator end() noexcept
	{
		return iterator(*this);
	}
	constexpr const_iterator begin() const noexcept
	{
		std::optional<index_manager_block> block_index_begin = this->cache.get_front();

		if (block_index_begin.has_value() == false) return const_iterator(*this);

		return const_iterator(*this, 0, block_index_begin.value(), 0, 0);
	}
	constexpr const_iterator end() const noexcept
	{
		return const_iterator(*this);
	}
	constexpr const_iterator cbegin() const noexcept
	{
		std::optional<index_manager_block> block_index_begin = this->cache.get_front();

		if (block_index_begin.has_value() == false) return const_iterator(*this);

		return const_iterator(*this, 0, block_index_begin.value(), 0, 0);
	}
	constexpr const_iterator cend() const noexcept
	{
		return const_iterator(*this);
	}
	constexpr reverse_iterator rbegin() noexcept
	{
		return reverse_iterator(end());
	}
	constexpr reverse_iterator rend() noexcept
	{
		return reverse_iterator(begin());
	}
	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}
	constexpr const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}
	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}
	constexpr const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(begin());
	}
	constexpr std::size_t size() const noexcept
	{
		return this->element_size;
	}
	iterator::reference push_back(const T& src_new)
	{
		return emplace_back(src_new);
	}
	void pop_back()
	{
		return erase(cend() - 1);
	}
	template<typename...params>
	iterator::reference emplace(const const_iterator& it_emplace, params&&...src_new)
	{
		manager_result_ref<T, MAX, allocator_manager> block_insert = get_block_ref(it_emplace.block_index, false);

		if (block_insert.has_value() == false)
			return emplace_back(std::forward<params>(src_new)...);

		auto param_store = std::make_tuple(std::forward<params>(src_new)...);
		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_ref = block_insert.value().get().value();
		manager_result_src_ref<T> res_insert = std::apply
		(
			[&](auto&&...param_old)
			{
				return block_insert_front(it_emplace.order_block, block_insert_ref.src, it_emplace.block_order, std::forward<decltype(param_old)>(param_old)...);
			},
			param_store
		);

		if (res_insert.has_value() == false)
		{
			if (res_insert.error() != manager_error::block_max_size_right)
				throw std::runtime_error("插入失败");

			manager_result_index index_new = insert_back_block(it_emplace.order_block);

			if (index_new.has_value() == false)
				throw std::runtime_error("插入失败");

			manager_result_ref<T, MAX, allocator_manager> block_insert_new = get_block_ref(index_new.value(), false);

			if (block_insert_new.has_value() == false)
				throw std::runtime_error("插入失败");

			element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_new_ref = block_insert_new.value().get().value();

			block_insert = get_block_ref(it_emplace.block_index, false);
			element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_ref_old = block_insert.value().get().value();

			block_insert_new_ref.src.first = std::exchange(block_insert_ref_old.src.second, std::nullopt);

			this->cache_locate.update(it_emplace.order_block, block_get_size(block_insert_ref_old.src));
			this->cache_locate.update(it_emplace.order_block + 1, block_get_size(block_insert_new_ref.src));

			if (it_emplace.block_order < MAX / 2)
			{
				res_insert = std::apply
				(
					[&](auto&&...param_old)
					{
						return block_insert_front(it_emplace.order_block, block_insert_ref_old.src, it_emplace.block_order, std::forward<decltype(param_old)>(param_old)...);
					},
					std::move(param_store)
				);

				if (res_insert.has_value() == false)
					throw std::runtime_error("插入失败");
			}
			else
			{
				res_insert = std::apply
				(
					[&](auto&&...param_old)
					{
						return block_insert_front(it_emplace.order_block, block_insert_ref_old.src, it_emplace.block_order - (MAX / 2), std::forward<decltype(param_old)>(param_old)...);
					},
					std::move(param_store)
				);

				if (res_insert.has_value() == false)
					throw std::runtime_error("插入失败");
			}
		}

		this->element_size++;
		return res_insert.value();
	}
	template<typename...params>
	iterator::reference emplace_back(params&&...src_new)
	{
		std::optional<index_manager_block> block_last = this->cache.get_back();
		manager_result_src_ref<T> result_ref = std::unexpected(manager_error::block_invalid);

		if (block_last.has_value() == false)
		{
			manager_result_index index_new = add_block();

			if (index_new.has_value() == false)
				throw std::runtime_error("添加失败");

			manager_result_ref<T, MAX, allocator_manager> block_new = get_block_ref(index_new.value(), false);

			if (block_new.has_value() == false)
				throw std::runtime_error("添加失败");

			element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_new_ref = block_new.value().get().value();

			result_ref = block_add(this->block_size - 1, block_new_ref.src, std::forward<params>(src_new)...);

			if (result_ref.has_value() == false)
				throw std::runtime_error("添加失败");
		}
		else
		{
			manager_result_ref<T, MAX, allocator_manager> block_back = get_block_ref(block_last.value(), false);

			if (block_back.has_value() == false)
				throw std::runtime_error("添加失败");

			element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_back_ref = block_back.value().get().value();

			if (block_get_size(block_back_ref.src) == MAX)
			{
				manager_result_index index_new = add_block();

				if (index_new.has_value() == false)
					throw std::runtime_error("添加失败");

				manager_result_ref<T, MAX, allocator_manager> block_new = get_block_ref(index_new.value(), false);

				if (block_new.has_value() == false)
					throw std::runtime_error("添加失败");

				element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_new_ref = block_new.value().get().value();

				result_ref = block_add(this->block_size - 1, block_new_ref.src, std::forward<params>(src_new)...);

				if (result_ref.has_value() == false)
					throw std::runtime_error("添加失败");
			}
			else
			{
				result_ref = block_add(this->block_size - 1, block_back_ref.src, std::forward<params>(src_new)...);

				if (result_ref.has_value() == false)
				{
					throw std::runtime_error("添加失败");
				}
			}
		}

		this->element_size++;
		return result_ref.value().get();
	}
	template<typename...params>
	iterator insert(const const_iterator& it_insert, const T& src_new)
	{
		iterator it_res(*this, it_insert.order_block, it_insert.block_index, it_insert.block_order, it_insert.order);
		manager_result_ref<T, MAX, allocator_manager> block_insert = get_block_ref(it_insert.block_index, false);

		if (block_insert.has_value() == false)
		{
			emplace_back(src_new);
			return it_res;
		}
			
		element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_ref = block_insert.value().get().value();
		manager_result_src_ref<T> res_insert = block_insert_front(it_insert.order_block, block_insert_ref.src, it_insert.block_order, src_new);

		if (res_insert.has_value() == false)
		{
			if (res_insert.error() != manager_error::block_max_size_right)
				throw std::runtime_error("插入失败");

			manager_result_index index_new = insert_back_block(it_insert.order_block);

			if (index_new.has_value() == false)
				throw std::runtime_error("插入失败");

			manager_result_ref<T, MAX, allocator_manager> block_insert_new = get_block_ref(index_new.value(), false);

			if (block_insert_new.has_value() == false)
				throw std::runtime_error("插入失败");

			element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_new_ref = block_insert_new.value().get().value();

			block_insert = get_block_ref(it_insert.block_index, false);
			element<manager_block<T, MAX, allocator_manager>, index_manager_block>& block_insert_ref_old = block_insert.value().get().value();

			block_insert_new_ref.src.first = std::exchange(block_insert_ref_old.src.second, std::nullopt);

			this->cache_locate.update(it_insert.order_block, block_get_size(block_insert_ref_old.src));
			this->cache_locate.update(it_insert.order_block + 1, block_get_size(block_insert_new_ref.src));

			if (it_insert.block_order < MAX / 2)
			{
				res_insert = block_insert_front(it_insert.order_block, block_insert_ref_old.src, it_insert.block_order, src_new);

				if (res_insert.has_value() == false)
					throw std::runtime_error("插入失败");
			}
			else
			{
				res_insert = block_insert_front(it_insert.order_block + 1, block_insert_new_ref.src, it_insert.block_order - (MAX / 2), src_new);

				if (res_insert.has_value() == false)
					throw std::runtime_error("插入失败");

				it_res.order_block++;
				it_res.block_index = index_new.value();
				it_res.block_order = it_insert.block_order - (MAX / 2);
			}
		}

		this->element_size++;
		return it_res;
	}
	iterator insert(const iterator& it_insert, const T& src_new)
	{
		return insert(const_iterator(it_insert), src_new);
	}
	reverse_iterator insert(const reverse_iterator& it_insert, const T& src_new)
	{
		return reverse_iterator(insert(it_insert.base(), src_new));
	}
	reverse_iterator insert(const const_reverse_iterator& it_insert, const T& src_new)
	{
		return reverse_iterator(insert(it_insert.base(), src_new));
	}
	iterator::reference back()
	{
		if (this->element_size == 0)
			throw std::runtime_error("元素不存在");

		return at(this->element_size - 1);
	}
	const_iterator::reference back() const
	{
		if (this->element_size == 0)
			throw std::runtime_error("元素不存在");

		return at(this->element_size - 1);
	}
	iterator::reference front()
	{
		if (this->element_size == 0)
			throw std::runtime_error("元素不存在");

		return at(0);
	}
	const_iterator::reference front() const
	{
		if (this->element_size == 0)
			throw std::runtime_error("元素不存在");

		return at(0);
	}
	void shrink_to_fit()
	{
		std::optional<index_manager_block> block_index_current = this->cache.get_front();
		std::size_t index_current = 0;

		while (block_index_current.has_value() == true)
		{
			manager_result_ref<T, MAX, allocator_manager> block_target = get_block_ref(block_index_current.value(), false);

			if (block_target.has_value() == true)
			{
				manager_block<T, MAX, allocator_manager>& block_ref = block_target.value().get().value().src;

				if (block_ref.first.has_value() == true) block_ref.first.value().rebuild();
				if (block_ref.second.has_value() == true) block_ref.second.value().rebuild();
			}
		
			block_index_current = this->cache.get(++index_current);
		}

		return;
	}
	constexpr bool empty() const noexcept
	{
		return this->element_size == 0;
	}
	constexpr void clear() noexcept
	{
		this->set.clear();
		this->free.clear();
		this->cache.clear();
		this->cache_locate.clear();
		this->block_size = 0;
		this->element_size = 0;
		return;
	}
public:
	const_iterator::reference operator[](std::size_t order) const
	{
		return at(order);
	}
	iterator::reference operator[](std::size_t order)
	{
		return at(order);
	}
};

template<typename T, typename allocator_container_plus = std::allocator<T>>
using container_plus = manager<T, BLOCK_SIZE_DEFAULT, allocator_container_plus>;

