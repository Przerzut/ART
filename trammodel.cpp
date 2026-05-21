#include "trammodel.h"

/*****************************************************************************
 * @file trammodel.cpp
 * @brief Implementacja modelu danych TramModel dla QML MapItemView.
 *****************************************************************************/

TramModel::TramModel(QObject *parent) : QAbstractListModel(parent) {}

int TramModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_trams.count();
}

QVariant TramModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_trams.count())
        return QVariant();

    const TramInfo &tram = m_trams[index.row()];

    switch (role) {
        case IdRole: return tram.id;
        case LineRole: return tram.line;
        case CoordinateRole: return QVariant::fromValue(tram.coordinate);
        case SpeedRole: return tram.speed;
        case HeadingRole: return tram.heading;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TramModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "tramId";
    roles[LineRole] = "tramLine";
    roles[CoordinateRole] = "tramCoordinate";
    roles[SpeedRole] = "tramSpeed";
    roles[HeadingRole] = "tramHeading";
    return roles;
}

void TramModel::updateTram(int id, const QString& line, double lat, double lon, double speed, double heading) {
    int idx = findTramIndex(id);

    if (idx != -1) {
        m_trams[idx].coordinate.setLatitude(lat);
        m_trams[idx].coordinate.setLongitude(lon);
        m_trams[idx].speed = speed;
        m_trams[idx].heading = heading;
        
        QModelIndex modelIndex = createIndex(idx, 0);
        emit dataChanged(modelIndex, modelIndex, {CoordinateRole, SpeedRole, HeadingRole});
    } else {
        beginInsertRows(QModelIndex(), m_trams.count(), m_trams.count());
        m_trams.append({id, line, QGeoCoordinate(lat, lon), speed, heading});
        endInsertRows();
    }
}

void TramModel::removeTram(int id) {
    int idx = findTramIndex(id);
    if (idx != -1) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_trams.removeAt(idx);
        endRemoveRows();
    }
}

int TramModel::findTramIndex(int id) const {
    for (int i = 0; i < m_trams.count(); ++i) {
        if (m_trams[i].id == id) {
            return i;
        }
    }
    return -1;
}