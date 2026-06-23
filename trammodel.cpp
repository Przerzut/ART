/**
 * @file trammodel.cpp
 * @brief Implementacja modelu danych TramModel dla QML MapItemView.
 * @details Klasa stanowi pomost pomiędzy logiką biznesową C++ a interfejsem graficznym QML,
 * umożliwiając dynamiczne i wydajne renderowanie poruszających się tramwajów na mapie.
 */

#include "trammodel.h"

/**
 * @brief Konstruktor modelu tramwajów.
 * @param parent Wskaźnik na obiekt nadrzędny (zgodnie ze standardem Qt).
 */
TramModel::TramModel(QObject *parent) : QAbstractListModel(parent) {}

/**
 * @brief Zwraca liczbę elementów (tramwajów) w modelu.
 * @details Wymagana przez interfejs QAbstractListModel. Silnik QML wywołuje tę funkcję, 
 * aby dowiedzieć się, ile instancji delegata (ikon) musi narysować na mapie.
 * @param parent Indeks obiektu nadrzędnego (dla list płaskich zawsze nieprawidłowy).
 * @return Liczba obecnie śledzonych tramwajów.
 */
int TramModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_trams.count();
}

/**
 * @brief Pobiera dane dla konkretnego tramwaju i określonej roli.
 * @details Wywoływana przez QML za każdym razem, gdy potrzebuje odświeżyć właściwość 
 * (np. pozycję, prędkość, kąt obrotu) dla konkretnej ikony na mapie.
 * @param index Indeks elementu w modelu.
 * @param role Rola danych (określa, o jaką konkretną informację prosi QML).
 * @return Zmienna typu QVariant zawierająca żądaną daną.
 */
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

/**
 * @brief Mapuje identyfikatory ról C++ na nazwy właściwości widoczne w QML.
 * @details Pozwala silnikowi QML odwoływać się do danych za pomocą czytelnych zmiennych 
 * (np. `model.tramCoordinate` zamiast surowych identyfikatorów liczbowych).
 * @return Słownik mapujący wartości enum na ciągi znaków QByteArray.
 */
QHash<int, QByteArray> TramModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "tramId";
    roles[LineRole] = "tramLine";
    roles[CoordinateRole] = "tramCoordinate";
    roles[SpeedRole] = "tramSpeed";
    roles[HeadingRole] = "tramHeading";
    return roles;
}

/**
 * @brief Aktualizuje stan pojedynczego tramwaju lub dodaje nowy do modelu.
 * @details Jeśli tramwaj o danym ID już istnieje, jego pozycja, prędkość i kąt 
 * zostaną zaktualizowane, a do QML zostanie wysłany sygnał `dataChanged`. 
 * Jeśli tramwaj pojawia się po raz pierwszy, dodawany jest na koniec listy 
 * z użyciem zoptymalizowanych funkcji `beginInsertRows`/`endInsertRows`.
 * @param id Unikalny identyfikator pojazdu z API.
 * @param line Nazwa linii tramwajowej (np. "16").
 * @param lat Nowa szerokość geograficzna.
 * @param lon Nowa długość geograficzna.
 * @param speed Zaktualizowana prędkość (w km/h).
 * @param heading Kąt obrotu ikony na mapie (w stopniach).
 */
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

/**
 * @brief Trwale usuwa tramwaj z modelu danych i mapy QML.
 * @details Używane przez Garbage Collector z głównego okna do czyszczenia 
 * przestarzałych (nieaktywnych) pojazdów.
 * @param id Unikalny identyfikator usuwanego pojazdu.
 */
void TramModel::removeTram(int id) {
    int idx = findTramIndex(id);
    if (idx != -1) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_trams.removeAt(idx);
        endRemoveRows();
    }
}

/**
 * @brief Odszukuje wewnętrzny indeks tablicy dla tramwaju o konkretnym ID.
 * @details Metoda pomocnicza optymalizująca wyszukiwanie pojazdów podczas ich odświeżania.
 * @param id Unikalny identyfikator poszukiwanego pojazdu.
 * @return Wewnętrzny indeks (0 do N-1) w tablicy m_trams, lub -1 jeśli nie znaleziono.
 */
int TramModel::findTramIndex(int id) const {
    for (int i = 0; i < m_trams.count(); ++i) {
        if (m_trams[i].id == id) {
            return i;
        }
    }
    return -1;
}