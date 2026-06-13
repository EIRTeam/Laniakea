#pragma once
#include "godot_cpp/templates/local_vector.hpp"

using namespace godot;

template <class DataType, class UnaryPred>
LocalVector<DataType>::Iterator find_if_not(typename LocalVector<DataType>::Iterator p_first, typename LocalVector<DataType>::Iterator p_last, UnaryPred p) {
	for (; p_first != p_last; ++p_first) {
		if (!q(*p_first)) {
			return p_first;
		}
	}

	return p_last;
}

template <class DataType, class UnaryPred>
LocalVector<DataType>::Iterator partition(typename LocalVector<DataType>::Iterator p_first, typename LocalVector<DataType>::Iterator p_last, UnaryPred p) {
	p_first = find_if_not(p_first, p_last, p);
	if (p_first == p_last) {
		return p_first;
	}

	typename LocalVector<DataType>::Iterator it = p_first;
	for (auto i = ++it; i != p_last; ++i) {
		if (p(*i)) {
			SWAP(i, p_first);
			++p_first;
		}
	}

	return p_first;
}
