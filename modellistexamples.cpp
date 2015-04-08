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

    Model::List<Person, QString> personList {
        {make_tuple(Person{"Romário", 24}, "Programador")},
        {{{Qt::DisplayRole, [](const Person &p) { return p.name + " (" + QString::number(p.age) + ")"; }}}},
        {{{Qt::DisplayRole, [](const QString &s) { return "Profession: " + s; }}}}
    };

    Model::List<Person, QString> otherPersonList {
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

    Ui::ModelListExamples ui;
    QWidget w;
    ui.setupUi(&w);

    ui.nonEditable->setModel(&personList);
    ui.editable->setModel(&otherPersonList);

    w.show();

    a.exec();
}
