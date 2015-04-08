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

int main(int argc, char **argv)
{
    QApplication a { argc, argv };

    Model::List<const char *, int, const char *> simpleList {
        {
            make_tuple("Romário", 24, "Programador"),
            make_tuple("Mike", 30, "Plumber"),
            make_tuple("Ellie", 16, "Student"),
            make_tuple("Jesus", 33, "Carpenter")
        }
    };

    Model::List<Person, QString> personList {
        {make_tuple(Person{"Romário", 24}, "Programador")},

        [](const Person &p) { return p.name + " (" + QString::number(p.age) + ")"; },
        [](const QString &s) { return "Profession: " + s; }
    };

    Model::List<Person, QString> editablePersonList {
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
        {},
        [](const QString &s) { return "Name: " + s; },
        [](QString &s, const QVariant &v) { s = v.toString(); return true; },

        [](const QString &s) { return "Profession: " + s; },
        [](QString &s, const QVariant &v) { s = v.toString(); return true; }
    };

    Model::List<Person> removablePersonList {
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

    Ui::ModelListExamples ui;
    QWidget w;
    ui.setupUi(&w);

    ui.simple->setModel(&simpleList);
    ui.nonEditable->setModel(&personList);
    ui.editable->setModel(&editablePersonList);
    ui.insertable->setModel(&insertablePersonList);
    ui.removable->setModel(&removablePersonList);

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

    w.show();

    a.exec();
}
