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

namespace QtMVT
{
    namespace Model
    {

    template <int I, typename... Types>
    class ListTemplateFunctions;

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
                std::map<int, std::function<QVariant(const RoleType &)>> &&roles = {},
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
            std::initializer_list<std::tuple<T, Types...>> l,
            RoleFunctions<T> &&tRoles,
            RoleFunctions<Types> &&... otherRoles,
            QObject *parent = nullptr)
        :
            QAbstractTableModel{parent},
            _rows{l},
            _roleFunctions{tRoles, otherRoles...}
        {}

        List(const List<T, Types...> &other) :
            QAbstractTableModel{other.parent()},
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

            return ListTemplateFunctions<rowSize - 1, T, Types...>{}.getFromIndex(*this, index, role);
        }

        Qt::ItemFlags flags(const QModelIndex &index) const
        {
            return
                QAbstractTableModel::flags(index) |
                (ListTemplateFunctions<rowSize - 1, T, Types...>{}.columnIsEditable(*this, index.column())?
                     Qt::ItemIsEditable :
                     Qt::NoItemFlags);
        }

        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
        {
            if (_indexIsInvalid(index))
                return {};

            return ListTemplateFunctions<rowSize - 1, T, Types...>{}.setInIndex(*this, index, value, role);
        }

        bool insertRows(int row, int count, const QModelIndex &parent = {})
        {
            if (count == 0)
                return true;

            if (row < 0 ||
                row > _rows.size())
                return false;

            beginInsertRows(parent, row, row + count - 1);

            auto rowIt = _rows.begin() + row;
            for (int i = 0; i < count; ++i)
                _rows.emplace(rowIt);

            endInsertRows();

            return true;
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
    };

    template <int I, typename... Types>
    class ListTemplateFunctions
    {
    public:
        QVariant getFromIndex(const List<Types...> &list, const QModelIndex &i, const int &role)
        {
            if (i.column() != I)
                return ListTemplateFunctions<I - 1, Types...>{}.getFromIndex(list, i, role);

            auto curRoles = std::get<I>(list._roleFunctions).roles;
            auto roleFunction = curRoles.find(role);
            if (roleFunction == curRoles.end())
                return {};

            return roleFunction->second(std::get<I>(list._rows[i.row()]));
        }

        bool columnIsEditable(const List<Types...> &list, const int &column)
        {
            if (column != I)
                return ListTemplateFunctions<I - 1, Types...>{}.columnIsEditable(list, column);

            return !std::get<I>(list._roleFunctions).editRoles.empty();
        }

        bool setInIndex(List<Types...> &list, const QModelIndex &i, const QVariant &data, const int &role)
        {
            if (i.column() != I)
                return ListTemplateFunctions<I - 1, Types...>{}.setInIndex(list, i, data, role);

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
        QVariant getFromIndex(const List<Types...> &, const QModelIndex &, const int &)
        {
            return {};
        }

        bool columnIsEditable(const List<Types...> &, const int &)
        {
            return false;
        }

        bool setInIndex(List<Types...> &, const QModelIndex &, const QVariant &, const int &)
        {
            return false;
        }
    };

    }

}

#endif // LISTMODEL_HPP
