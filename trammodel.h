#ifndef TRAMMODEL_H
#define TRAMMODEL_H

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QList>
#include <QString>

struct TramInfo {
    int id;
    QString line;
    QGeoCoordinate coordinate; 
    double speed;
    double heading; 
};

class TramModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum TramRoles {
        IdRole = Qt::UserRole + 1,
        LineRole,
        CoordinateRole,
        SpeedRole,
        HeadingRole
    };

    explicit TramModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateTram(int id, const QString& line, double lat, double lon, double speed, double heading);
    void removeTram(int id);

private:
    QList<TramInfo> m_trams;
    int findTramIndex(int id) const;
};

#endif // TRAMMODEL_H