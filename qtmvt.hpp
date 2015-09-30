/*  Qt Model/View Templates
    Copyright (c) 2015 Luiz Romário Santana Rios <luizromario@gmail.com>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
          names of its contributors may be used to endorse or promote products
          derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LISTMODEL_HPP
#define LISTMODEL_HPP

// Qt includes
#include <QAbstractTableModel>
#include <QHash>

// STL includes
#include <array>
#include <initializer_list>
#include <functional>
#include <utility>
#include <tuple>
#include <type_traits>

using std::make_tuple;

namespace QtMVT
{

    namespace Util
    {

    template <typename T, typename... Types>
    class TypesAreDefaultConstructible
    {
    public:
        static const bool value =
            std::is_default_constructible<T>::value &&
            TypesAreDefaultConstructible<Types...>::value;
    };

    template <typename T>
    class TypesAreDefaultConstructible<T>
    {
    public:
        static const bool value = std::is_default_constructible<T>::value;
    };

    }

    namespace Model
    {

    template <int I, typename... Types>
    class ListDataAccess;

    template <bool B, typename... Types>
    class ListInsertRows;

        namespace Util
        {

        template <typename RoleType>
        struct RoleFunctions
        {
            RoleFunctions(
                QHash<int, std::function<QVariant(const RoleType &)>> &&roles = {
                    {Qt::DisplayRole, [](const RoleType &t) { return t; }}
                },
                QHash<int, std::function<bool(RoleType &, const QVariant &)>> &&editRoles = {})
            :
                roles{roles},
                editRoles{editRoles}
            {}

            QVariant data(int role, const RoleType &t) const
            {
                if (!roles.contains(role)) {
                    if (role != Qt::EditRole)
                        return {};

                    role = Qt::DisplayRole;
                }

                return roles[role](t);
            }

            bool isEditable() const
            {
                return editRoles.empty();
            }

            bool setData(int role, RoleType &t, const QVariant &value)
            {
                if (!editRoles.contains(role))
                    return false;

                return editRoles[role](t, value);
            }

            QHash<int, std::function<QVariant(const RoleType &)>> roles;
            QHash<int, std::function<bool(RoleType &, const QVariant &)>> editRoles;
        };

        }

    // A list with a fixed number of columns
    template <typename T, typename... Types>
    class List : public QAbstractTableModel
    {
        typedef std::tuple<T, Types...> _RowType;

    public:
        static const constexpr int rowSize = std::tuple_size<_RowType>::value;

        List(
            std::array<const char *, rowSize> &&headerTitles,
            std::initializer_list<std::tuple<T, Types...>> &&l,
            Util::RoleFunctions<T> &&tRoles,
            Util::RoleFunctions<Types> &&... otherRoles,
            QObject *parent = nullptr)
        :
            QAbstractTableModel{parent},
            _headerTitles(std::move(headerTitles)),
            _rows{l},
            _roleFunctions{tRoles, otherRoles...}
        {}

        List(
            std::array<const char *, rowSize> &&headerTitles,
            std::initializer_list<std::tuple<T, Types...>> &&l,
            std::function<QVariant(const T &)> &&tDisplayFunction,
            std::function<bool(T &, const QVariant &)> &&tEditFunction,
            std::function<QVariant(const Types &)> &&... otherDisplayFunctions,
            std::function<bool(Types &, const QVariant &)> &&... otherEditFunctions,
            QObject *parent = nullptr)
        :
            List{
                std::move(headerTitles),
                std::move(l),
                {
                    {{Qt::DisplayRole, tDisplayFunction}},
                    {{Qt::EditRole, tEditFunction}}
                },
                {
                    {{Qt::DisplayRole, otherDisplayFunctions}},
                    {{Qt::EditRole, otherEditFunctions}}
                }...,
                parent}
        {}

        List(
            std::array<const char *, rowSize> &&headerTitles,
            std::initializer_list<std::tuple<T, Types...>> &&l,
            std::function<QVariant(const T &)> &&tDisplayFunction,
            std::function<QVariant(const Types &)> &&... otherDisplayFunctions,
            QObject *parent = nullptr)
        :
            List{
                std::move(headerTitles),
                std::move(l),
                {
                    {{Qt::DisplayRole, tDisplayFunction}},
                    {}
                },
                {
                    {{Qt::DisplayRole, otherDisplayFunctions}},
                    {}
                }...,
                parent}
        {}

        List(
            std::array<const char *, rowSize> &&headerTitles,
            std::initializer_list<std::tuple<T, Types...>> &&l = {},
            QObject *parent = nullptr)
        :
            List{
                std::move(headerTitles),
                std::move(l),
                Util::RoleFunctions<T>(),
                Util::RoleFunctions<Types>()...,
                parent}
        {}

        List(QObject *parent = nullptr) :
            List{{}, {}, parent}
        {}

        List(const List<T, Types...> &other, QObject *parent = nullptr) :
            QAbstractTableModel{parent},
            _headerTitles(other._headerTitles),
            _rows{other._rows},
            _roleFunctions{other._roleFunctions}
        {}

        List(List<T, Types...> &&) = default;

        List<T, Types...> createNew(
            std::initializer_list<_RowType> &&l = {},
            QObject *parent = nullptr)
        {
            return {_headerTitles, std::move(l), _roleFunctions, parent};
        }

        List<T, Types...> createNew(QObject *parent)
        {
            return createNew({}, parent);
        }

        int rowCount(const QModelIndex & = {}) const
        {
            return _rows.size();
        }

        int columnCount(const QModelIndex & = {}) const
        {
            return std::tuple_size<_RowType>::value;
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
        {
            if (_indexIsInvalid(index))
                return {};

            return ListDataAccess<rowSize - 1, T, Types...>::getFromIndex(*this, index, role);
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const
        {
            if (
                section < 0 ||
                section >= rowSize ||
                orientation != Qt::Horizontal ||
                role != Qt::DisplayRole)
                return QAbstractTableModel::headerData(section, orientation, role);

            return _headerTitles[section];
        }

        Qt::ItemFlags flags(const QModelIndex &index) const
        {
            return
                QAbstractTableModel::flags(index) |
                (ListDataAccess<rowSize - 1, T, Types...>::columnIsEditable(*this, index.column())?
                     Qt::ItemIsEditable :
                     Qt::NoItemFlags);
        }

        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
        {
            if (_indexIsInvalid(index))
                return {};

            return ListDataAccess<rowSize - 1, T, Types...>::setInIndex(*this, index, value, role);
        }

        bool insertRows(int row, int count, const QModelIndex &parent = {})
        {
            return ListInsertRows<
                QtMVT::Util::TypesAreDefaultConstructible<T, Types...>::value,
                T,
                Types...>::func(*this, row, count, parent);
        }

        bool removeRows(int row, int count, const QModelIndex &parent = {})
        {
            if (count == 0)
                return true;

            if (row < 0 ||
                static_cast<size_t>(row + count) > _rows.size())
                return false;

            beginRemoveRows(parent, row, row + count - 1);

            auto rowIt = _rows.begin() + row;
            for (int i = 0; i < count; ++i)
                _rows.erase(rowIt);

            endRemoveRows();

            return true;
        }

        const std::tuple<T, Types...> &row(int rowIndex) const
        {
            Q_ASSERT(rowIndex >= 0 && rowIndex < _rows.size());
            return _rows[rowIndex];
        }

        bool insert(int row, std::initializer_list<_RowType> &&rows)
        {
            if (rows.size() == 0)
                return true;

            if (
                row < 0 ||
                static_cast<size_t>(row) > _rows.size())
                return false;

            beginInsertRows({}, row, row + rows.size() - 1);

            auto rowIt = _rows.begin() + row;
            _rows.insert(rowIt, rows);

            endInsertRows();

            return true;
        }

        bool insert(int row, _RowType &&rowElements)
        {
            return insert(row, {std::move(rowElements)});
        }

        bool append(std::initializer_list<_RowType> &&rows)
        {
            return insert(_rows.size(), std::move(rows));
        }

        bool append(_RowType &&rowElements)
        {
            return append({std::move(rowElements)});
        }

        template <std::size_t Column>
        void addRoleFunction(
            int role,
            std::function<
                QVariant(
                    const typename std::tuple_element<
                        Column, _RowType
                    >::type &)> &&function)
        {
            beginResetModel();

            auto &functions = std::get<Column>(_roleFunctions).roles;
            functions.insert(role, function);

            endResetModel();
        }

        template <std::size_t Column>
        void addRoleFunction(
            std::function<
                QVariant(
                    const typename std::tuple_element<
                        Column, _RowType
                    >::type &)> &&function)
        {
            addRoleFunction<Column>(Qt::DisplayRole, std::move(function));
        }

        template <std::size_t Column>
        void addEditRoleFunction(
            int editRole,
            std::function<
                bool(
                    typename std::tuple_element<
                        Column, _RowType
                    >::type &,
                    const QVariant &)> &&function)
        {
            beginResetModel();

            auto &functions = std::get<Column>(_roleFunctions).editRoles;
            functions.insert(editRole, function);

            endResetModel();
        }

        template <std::size_t Column>
        void addEditRoleFunction(
            std::function<
                bool(
                    typename std::tuple_element<
                        Column, _RowType
                    >::type &,
                    const QVariant &)> &&function)
        {
            addEditRoleFunction<Column>(Qt::EditRole, std::move(function));
        }

        template <std::size_t Column>
        void removeRole(int role = Qt::DisplayRole)
        {
            beginResetModel();

            auto &functions = std::get<Column>(_roleFunctions).roles;
            functions.remove(role);

            endResetModel();
        }

        template <std::size_t Column>
        void removeEditRole(int role = Qt::EditRole)
        {
            beginResetModel();

            auto &functions = std::get<Column>(_roleFunctions).editRoles;
            functions.remove(role);

            endResetModel();
        }

        void setHeaderTitle(int section, const char *title)
        {
            _headerTitles[section] = title;
        }

    private:
        inline bool _indexIsInvalid(const QModelIndex &index) const
        {
            return
                !index.isValid() ||
                index.row() < 0 ||
                index.row() >= rowCount() ||
                index.column() < 0 ||
                index .column() >= columnCount();
        }

        std::array<const char *, rowSize> _headerTitles;
        std::vector<_RowType> _rows;
        std::tuple<Util::RoleFunctions<T>, Util::RoleFunctions<Types>...> _roleFunctions;

        List(
            const decltype(_headerTitles) &headerTitles,
            std::initializer_list<_RowType> &&l,
            const decltype(_roleFunctions) &roleFunctions,
            QObject *parent)
        :
            QAbstractTableModel{parent},
            _headerTitles(std::move(headerTitles)),
            _rows{l},
            _roleFunctions{roleFunctions}
        {}

        template <int I, typename... ListTypes>
        friend class ListDataAccess;

        template <bool B, typename... ListTypes>
        friend class ListInsertRows;
    };

    template <int I, typename... Types>
    class ListDataAccess
    {
    public:
        static QVariant getFromIndex(const List<Types...> &list, const QModelIndex &i, int role)
        {
            if (i.column() != I)
                return ListDataAccess<I - 1, Types...>::getFromIndex(list, i, role);

            return std::get<I>(list._roleFunctions).data(
                role,
                std::get<I>(list._rows[i.row()]));
        }

        static bool columnIsEditable(const List<Types...> &list, const int &column)
        {
            if (column != I)
                return ListDataAccess<I - 1, Types...>::columnIsEditable(list, column);

            return !std::get<I>(list._roleFunctions).isEditable();
        }

        static bool setInIndex(List<Types...> &list, const QModelIndex &i, const QVariant &data, const int &role)
        {
            if (i.column() != I)
                return ListDataAccess<I - 1, Types...>::setInIndex(list, i, data, role);

            return std::get<I>(list._roleFunctions).setData(
                role,
                std::get<I>(list._rows[i.row()]),
                data);
        }
    };

    template <typename... Types>
    class ListDataAccess<-1, Types...>
    {
    public:
        static QVariant getFromIndex(const List<Types...> &, const QModelIndex &, int)
        {
            return {};
        }

        static bool columnIsEditable(const List<Types...> &, const int &)
        {
            return false;
        }

        static bool setInIndex(List<Types...> &, const QModelIndex &, const QVariant &, const int &)
        {
            return false;
        }
    };

    template <bool DefaultConstructible, typename... Types>
    class ListInsertRows;

    template <typename... Types>
    class ListInsertRows<true, Types...>
    {
    public:
        static bool func(List<Types...> &l, int row, int count, const QModelIndex &parent)
        {
            if (count == 0)
                return true;

            if (row < 0 ||
                static_cast<size_t>(row) > l._rows.size())
                return false;

            l.beginInsertRows(parent, row, row + count - 1);

            auto rowIt = l._rows.begin() + row;
            for (int i = 0; i < count; ++i)
                l._rows.emplace(rowIt);

            l.endInsertRows();

            return true;
        }
    };

    template <typename... Types>
    class ListInsertRows<false, Types...>
    {
    public:
        static bool func(List<Types...> &l, int row, int count, const QModelIndex &parent)
        {
            return static_cast<QAbstractTableModel &>(l).insertRows(row, count, parent);
        }
    };

    template <typename T>
    class Table : public QAbstractTableModel
    {
    public:
        Table(const std::initializer_list<std::vector<T>> &l)
        {
            for(auto &&r : l) {
                if (r.size() > _width)
                    _width = r.size();

                _table.emplace_back();
                auto &tableRow = *(_table.end() - 1);
                int i = 0;
                for (auto &&el : r)
                    tableRow[i++] = el;
            }
        }

        inline int rowCount(const QModelIndex &) const
        {
            return _table.size();
        }

        inline int columnCount(const QModelIndex &) const
        {
            return _width;
        }

        QVariant data(const QModelIndex &index, int role) const
        {
            if (role != Qt::DisplayRole ||
                !index.isValid() ||
                static_cast<size_t>(index.row()) >= _table.size())
                return {};

            auto &row = _table[index.row()];
            const auto columnInd = index.column();

            if (columnInd >= row.size())
                return {};

            return row[index.column()];
        }

    private:
        std::vector<QHash<int, T>> _table;
        size_t _width = 0;
    };

    }

}

#endif // LISTMODEL_HPP
