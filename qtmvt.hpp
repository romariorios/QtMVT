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

// STL includes
#include <array>
#include <initializer_list>
#include <functional>
#include <map>
#include <utility>
#include <tuple>
#include <type_traits>

namespace QtMVT
{
    namespace Model
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

    template <int I, typename... Types>
    class ListTemplateFunctions;

    template <bool B, typename... Types>
    class InsertRows;

    // A list with a fixed size of columns
    template <typename T, typename... Types>
    class List : public QAbstractTableModel
    {
        typedef std::tuple<T, Types...> _RowType;

    public:
        template <typename RoleType>
        struct RoleFunctions
        {
            RoleFunctions(
                std::map<int, std::function<QVariant(const RoleType &)>> &&roles = {
                    {Qt::DisplayRole, [](const RoleType &t) { return t; }}
                },
                std::map<int, std::function<bool(RoleType &, const QVariant &)>> &&editRoles = {})
            :
                roles{roles},
                editRoles{editRoles}
            {}

            std::map<int, std::function<QVariant(const RoleType &)>> roles;
            std::map<int, std::function<bool(RoleType &, const QVariant &)>> editRoles;
        };

        static const constexpr int rowSize = std::tuple_size<_RowType>::value;

        List(
            std::initializer_list<std::tuple<T, Types...>> &&l,
            RoleFunctions<T> &&tRoles,
            RoleFunctions<Types> &&... otherRoles,
            QObject *parent = nullptr)
        :
            QAbstractTableModel{parent},
            _rows{l},
            _roleFunctions{tRoles, otherRoles...}
        {}

        List(
            std::initializer_list<std::tuple<T, Types...>> &&l,
            std::function<QVariant(const T &)> &&tDisplayFunction,
            std::function<bool(T &, const QVariant &)> &&tEditFunction,
            std::function<QVariant(const Types &)> &&... otherDisplayFunctions,
            std::function<bool(Types &, const QVariant &)> &&... otherEditFunctions,
            QObject *parent = nullptr)
        :
            List{
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
            std::initializer_list<std::tuple<T, Types...>> &&l,
            std::function<QVariant(const T &)> &&tDisplayFunction,
            std::function<QVariant(const Types &)> &&... otherDisplayFunctions,
            QObject *parent = nullptr)
        :
            List{
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

        List(std::initializer_list<std::tuple<T, Types...>> &&l, QObject *parent = nullptr) :
            List{std::move(l), RoleFunctions<T>(), RoleFunctions<Types>()..., parent}
        {}

        List(const List<T, Types...> &other, QObject *parent = nullptr) :
            QAbstractTableModel{parent},
            _rows{other._rows},
            _roleFunctions{other._roleFunctions}
        {}

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

            return ListTemplateFunctions<rowSize - 1, T, Types...>::getFromIndex(*this, index, role);
        }

        Qt::ItemFlags flags(const QModelIndex &index) const
        {
            return
                QAbstractTableModel::flags(index) |
                (ListTemplateFunctions<rowSize - 1, T, Types...>::columnIsEditable(*this, index.column())?
                     Qt::ItemIsEditable :
                     Qt::NoItemFlags);
        }

        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
        {
            if (_indexIsInvalid(index))
                return {};

            return ListTemplateFunctions<rowSize - 1, T, Types...>::setInIndex(*this, index, value, role);
        }

        bool insertRows(int row, int count, const QModelIndex &parent = {})
        {
            return InsertRows<
                TypesAreDefaultConstructible<T, Types...>::value,
                T,
                Types...>::func(*this, row, count, parent);
        }

        bool removeRows(int row, int count, const QModelIndex &parent = {})
        {
            if (count == 0)
                return true;

            if (row < 0 ||
                row + count > _rows.size())
                return false;

            beginRemoveRows(parent, row, row + count - 1);

            auto rowIt = _rows.begin() + row;
            for (int i = 0; i < count; ++i)
                _rows.erase(rowIt);

            endRemoveRows();

            return true;
        }

        bool insert(int row, std::initializer_list<_RowType> &&rows)
        {
            if (rows.size() == 0)
                return true;

            if (
                row < 0 ||
                row > _rows.size())
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

        std::vector<_RowType> _rows;
        std::tuple<RoleFunctions<T>, RoleFunctions<Types>...> _roleFunctions;

        template <int I, typename... ListTypes>
        friend class ListTemplateFunctions;

        template <bool B, typename... ListTypes>
        friend class InsertRows;
    };

    template <int I, typename... Types>
    class ListTemplateFunctions
    {
    public:
        static QVariant getFromIndex(const List<Types...> &list, const QModelIndex &i, const int &role)
        {
            if (i.column() != I)
                return ListTemplateFunctions<I - 1, Types...>::getFromIndex(list, i, role);

            auto curRoles = std::get<I>(list._roleFunctions).roles;
            auto roleFunction = curRoles.find(role);
            if (roleFunction == curRoles.end())
                return {};

            return roleFunction->second(std::get<I>(list._rows[i.row()]));
        }

        static bool columnIsEditable(const List<Types...> &list, const int &column)
        {
            if (column != I)
                return ListTemplateFunctions<I - 1, Types...>::columnIsEditable(list, column);

            return !std::get<I>(list._roleFunctions).editRoles.empty();
        }

        static bool setInIndex(List<Types...> &list, const QModelIndex &i, const QVariant &data, const int &role)
        {
            if (i.column() != I)
                return ListTemplateFunctions<I - 1, Types...>::setInIndex(list, i, data, role);

            auto curRoles = std::get<I>(list._roleFunctions).editRoles;
            auto roleFunction = curRoles.find(role);
            if (roleFunction == curRoles.end())
                return false;

            return roleFunction->second(std::get<I>(list._rows[i.row()]), data);
        }
    };

    template <typename... Types>
    class ListTemplateFunctions<-1, Types...>
    {
    public:
        static QVariant getFromIndex(const List<Types...> &, const QModelIndex &, const int &)
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
    class InsertRows;

    template <typename... Types>
    class InsertRows<true, Types...>
    {
    public:
        static bool func(List<Types...> &l, int row, int count, const QModelIndex &parent)
        {
            if (count == 0)
                return true;

            if (row < 0 ||
                row > l._rows.size())
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
    class InsertRows<false, Types...>
    {
    public:
        static bool func(List<Types...> &l, int row, int count, const QModelIndex &parent)
        {
            return static_cast<QAbstractTableModel &>(l).insertRows(row, count, parent);
        }
    };

    }

}

#endif // LISTMODEL_HPP
