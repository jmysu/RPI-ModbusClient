#include "tablemodel.h"
#include "mainwindow.h"
#include <QtDebug>

TableModel::TableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

TableModel::TableModel(QList<QStringList> stringLists, QStringList listHeader, QObject *parent) :
    QAbstractTableModel(parent),
    stringLists(stringLists), listHeader(listHeader)
{
}

int TableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return stringLists.size();
}

int TableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return listHeader.count(); // header
}

QVariant TableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() >= stringLists.size() || index.row() < 0)  return QVariant();

    const auto &strings = stringLists.at(index.row());
    switch (role) {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return strings.at(index.column());
            break;
        case Qt::DecorationRole:
            if (index.column()== enumModbusCSV::eActRun) {
                bool isCheck = QVariant(strings.at(index.column())).toBool();
                QPixmap Pixmap0(":/images/CrossRed_32x32.png");
                QPixmap Pixmap1(":/images/CheckGreen_32x32.png");

                QPixmap pixmap(64, 32);  //w=cell width, h=cell
                QPixmap iconPixmap;
                pixmap.fill(Qt::transparent); // draw a transparent rectangle
                if (isCheck)
                    iconPixmap = Pixmap1;
                else
                    iconPixmap = Pixmap0;

                QPainter painter(&pixmap);
                painter.drawPixmap(32, 0, 32, 32, iconPixmap);
                return pixmap;
                }
            break;
         case Qt::CheckStateRole:
            break;
         case Qt::TextAlignmentRole:
            if (index.column()>enumModbusCSV::eDescription)
                return Qt::AlignCenter;
            break;
         }
    return QVariant();
}
QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Vertical)
        return QString::number(section + 1);
    if (orientation == Qt::Horizontal)
        return listHeader.at(section);

    return QVariant();
}

bool TableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        stringLists.insert(position, { QString(), QString() });

    endInsertRows();
    return true;
}

bool TableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        stringLists.removeAt(position);

    endRemoveRows();
    return true;
}
bool TableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        int row = index.row();
        auto strings = stringLists.at(row);
        strings[index.column()]=value.toString();
        stringLists.replace(row, strings);
        emit dataChanged(index, index, {role});

        return true;
        }
    return false;
}
/*
Qt::ItemFlags TableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::ItemIsEnabled;

    //if (index.column() == enumModbusCSV::eActRun)
    //    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
*/

QList<QStringList> TableModel::getStringLists() const
{
    return stringLists;
}
