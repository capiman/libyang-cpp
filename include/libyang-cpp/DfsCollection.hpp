/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <iterator>
#include <memory>
#include <set>

struct lyd_node;
struct lysc_node;
struct ly_ctx;

namespace libyang {
template <typename NodeType>
class DfsCollection;
class DataNode;
class SchemaNode;
template <typename NodeType>
struct underlying_node;
template <>
struct underlying_node<SchemaNode> {
    using type = const lysc_node;
};
template <>
struct underlying_node<DataNode> {
    using type = lyd_node;
};

template <typename NodeType>
using underlying_node_t = typename underlying_node<NodeType>::type;
struct internal_refcount;

class DataNode;

template <typename NodeType>
class DfsIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = NodeType;
    using reference = void;
    using difference_type = void;

    struct end {
    };

    ~DfsIterator();
    DfsIterator(const DfsIterator&);

    DfsIterator<NodeType>& operator++();
    DfsIterator<NodeType> operator++(int);
    NodeType operator*() const;

    struct NodeProxy {
        NodeType node;
        NodeType* operator->()
        {
            return &node;
        }
    };

    NodeProxy operator->() const;
    bool operator==(const DfsIterator& it) const;

    friend DfsCollection<NodeType>;
private:
    DfsIterator(underlying_node_t<NodeType>* start, const DfsCollection<NodeType>* coll);
    DfsIterator(const end);
    underlying_node_t<NodeType>* m_current;

    underlying_node_t<NodeType>* m_start;
    underlying_node_t<NodeType>* m_next;

    const DfsCollection<NodeType>* m_collection;

    void throwIfInvalid() const;

    void registerThis();
    void unregisterThis();
};

namespace impl {
template <typename RefType>
struct refs_type;

template <typename RefType>
using refs_type_t = typename refs_type<RefType>::type;

template <>
struct refs_type<DataNode> {
    using type = internal_refcount;
};

template <>
struct refs_type<SchemaNode> {
    using type = ly_ctx;
};
}

template <typename NodeType>
class DfsCollection {
public:
    friend DataNode;
    friend DfsIterator<NodeType>;
    friend SchemaNode;
    ~DfsCollection();
    DfsCollection(const DfsCollection<NodeType>&);
    DfsCollection& operator=(const DfsCollection<NodeType>&);

    DfsIterator<NodeType> begin() const;
    DfsIterator<NodeType> end() const;
private:

    DfsCollection(underlying_node_t<NodeType>* start, std::shared_ptr<impl::refs_type_t<NodeType>> refs);
    underlying_node_t<NodeType>* m_start;

    std::shared_ptr<impl::refs_type_t<NodeType>> m_refs;
    bool m_valid = true;

    // mutable is needed:
    // `begin` and `end` need to be const
    // because of that DfsIterator can only get a `const DataNodeCollectionDfs*`,
    // however, DfsIterator needs to register itself into m_iterators.
    mutable std::set<DfsIterator<NodeType>*> m_iterators;
    void invalidateIterators();

    void throwIfInvalid() const;
};
}