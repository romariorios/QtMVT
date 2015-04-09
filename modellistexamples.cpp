#include "qtmvt.hpp"

#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication>

#include "ui_modellistexamples.h"

using namespace QtMVT;
using namespace std;

struct Person
{
    QString name;
    int age;
};

class PersonNoDefault
{
    QString _name;
    int _age;

public:
    PersonNoDefault(QString &&name, int age) :
        _name{name},
        _age{age}
    {}

    QString name() const { return _name; }
    int age() const { return _age; }
};

int main(int argc, char **argv)
{
    QApplication a { argc, argv };

    Model::List<const char *, int, const char *> simpleList {
        {"Name", "Age", "Profession"},
        {
            make_tuple("Romário", 24, "Programador"),
            make_tuple("Mike", 30, "Plumber"),
            make_tuple("Ellie", 16, "Student"),
            make_tuple("Jesus", 33, "Carpenter")
        }
    };

    Model::List<Person, QString> personList {
        {"Person", "Profession"},
        {make_tuple(Person{"Romário", 24}, "Programador")},

        [](const Person &p) { return p.name + " (" + QString::number(p.age) + ")"; },
        [](const QString &s) { return "Profession: " + s; }
    };

    Model::List<Person, QString> editablePersonList {
        {"Person", "Profession"},
        {make_tuple(Person{"Romário", 24}, "Programador")},
        {
            {
                {Qt::DisplayRole, [](const Person &p) { return p.name; }},
                {Qt::ToolTipRole, [](const Person &p) { return "Age: " + QString::number(p.age); }}
            },
            {
                {Qt::EditRole, [](Person &p, const QVariant &value) { p.name = value.toString(); return true; }}
            }
        },
        {
            {
                {Qt::DisplayRole, [](const QString &s) { return "Profession: " + s; }}
            }
        }
    };

    Model::List<QString, QString> insertablePersonList {
        {"Name", "Profession"},
        {},
        [](const QString &s) { return "Name: " + s; },
        [](QString &s, const QVariant &v) { s = v.toString(); return true; },

        [](const QString &s) { return "Profession: " + s; },
        [](QString &s, const QVariant &v) { s = v.toString(); return true; }
    };

    Model::List<Person> removablePersonList {
        {"Person"},
        {
            make_tuple(Person{"Romário", 24}),
            make_tuple(Person{"Maria", 19}),
            make_tuple(Person{"Isabela", 33}),
            make_tuple(Person{"Antônio", 40}),
            make_tuple(Person{"Alícia", 50}),
            make_tuple(Person{"João", 26}),
            make_tuple(Person{"Ana", 45}),
            make_tuple(Person{"Francisca", 75}),
            make_tuple(Person{"Natanael", 30})
        },

        [](const Person &p)
        {
            return p.name + " (" + QString::number(p.age) + ")";
        }
    };

    Model::List<PersonNoDefault> noDefaultPersonList {
        {"Person"},
        {
            make_tuple(PersonNoDefault{"Romário", 24}),
            make_tuple(PersonNoDefault{"Marcela", 25}),
            make_tuple(PersonNoDefault{"Milton", 28})
        },

        [](const PersonNoDefault &p) { return p.name() + " (" + QString::number(p.age()) + ")"; }
    };

    auto fromPrototype = noDefaultPersonList.createNew();

    Ui::ModelListExamples ui;
    QWidget w;
    ui.setupUi(&w);

    ui.simple->setModel(&simpleList);
    ui.nonEditable->setModel(&personList);
    ui.editable->setModel(&editablePersonList);
    ui.insertable->setModel(&insertablePersonList);
    ui.removable->setModel(&removablePersonList);
    ui.noDefCtor->setModel(&noDefaultPersonList);
    ui.proto->setModel(&fromPrototype);

    QObject::connect(ui.insertableInsert, &QPushButton::clicked, [&ui, &insertablePersonList]()
    {
        int row = insertablePersonList.rowCount();
        insertablePersonList.insertRow(row);

        auto personIndex = insertablePersonList.index(row, 0);
        insertablePersonList.setData(personIndex, ui.insertableName->text());

        auto professionIndex = insertablePersonList.index(row, 1);
        insertablePersonList.setData(professionIndex, ui.insertableProfession->text());
    });

    QObject::connect(ui.removableRemove, &QPushButton::clicked, [&removablePersonList]()
    {
        removablePersonList.removeRow(0);
    });

    QObject::connect(ui.noDefCtorInsert, &QPushButton::clicked, [&ui, &noDefaultPersonList]()
    {
        noDefaultPersonList.append(
            make_tuple(
                PersonNoDefault{
                    ui.noDefCtorName->text(),
                    ui.noDefCtorAge->value()}));
    });

    QObject::connect(ui.noDefCtorAddRole, &QPushButton::clicked, [&noDefaultPersonList]()
    {
        noDefaultPersonList.addRoleFunction<0>(
            Qt::ToolTipRole,
            [](const PersonNoDefault &p)
            {
                return
                    "This person is called " + p.name() + " and is " + QString::number(p.age()) +
                    " years old.";
            });
    });

    QObject::connect(ui.protoInsert, &QPushButton::clicked, [&ui, &fromPrototype]()
    {
        fromPrototype.append(
            make_tuple(
                PersonNoDefault{
                    ui.protoName->text(),
                    ui.protoAge->value()}));
    });

    w.show();

    a.exec();
}
