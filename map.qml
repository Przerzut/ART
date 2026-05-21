import QtQuick 2.15
import QtQuick.Controls 2.15
import QtLocation 5.15
import QtPositioning 5.15

Item {
    id: root
    width: 1024
    height: 768

    property int selectedTramId: mainWindow.selectedTramId

    Plugin {
        id: osmPlugin
        name: "osm"
        // Wskazujemy darmowy serwer kafelków OSM bez znaków wodnych
        PluginParameter { name: "osm.mapping.custom.host"; value: "https://tile.openstreetmap.org/" }
        PluginParameter { name: "osm.mapping.custom.mapcopyright"; value: "© OpenStreetMap contributors" }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: osmPlugin
        
        center: QtPositioning.coordinate(51.1079, 17.0385)
        zoomLevel: 14.5

        Component.onCompleted: {
            if (supportedMapTypes.length > 0) {
                var targetMap = supportedMapTypes[0]; 
                for (var i = 0; i < supportedMapTypes.length; i++) {
                    if (supportedMapTypes[i].name === "Custom URL Map") {
                        targetMap = supportedMapTypes[i];
                        break;
                    }
                }
                activeMapType = targetMap;
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            
            onWheel: wheel => {
                if (wheel.angleDelta.y > 0) map.zoomLevel += 0.5
                else map.zoomLevel -= 0.5
            }
        }

        MapPolyline {
            line.width: 5
            line.color: "#808080"
            opacity: 0.7
            
            // Zaciąga trasę prosto z MainWindow w C++
            path: mainWindow.routePath
        }

        MapItemView {
            model: tramModel

            delegate: MapQuickItem {
                id: tramDelegate
                coordinate: model.tramCoordinate
                anchorPoint.x: 20
                anchorPoint.y: 20

                // USUNIĘTO: Behavior on coordinate - C++ płynnie podaje pozycję 30 razy na sekundę!

                sourceItem: Item {
                    width: 40
                    height: 40
                    
                    property bool isSelected: model.tramId === root.selectedTramId

                    Rectangle {
                        anchors.fill: parent
                        radius: 20
                        color: parent.isSelected ? "blue" : "red"
                        border.width: parent.isSelected ? 3 : 1
                        border.color: "white"
                        
                        transform: Rotation {
                            origin.x: 20
                            origin.y: 20
                            angle: model.tramHeading 
                        }
                        
                        Rectangle {
                            width: 6; height: 15; color: "white"
                            anchors.top: parent.top; anchors.topMargin: 5
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 3
                        }
                    }
                    
                    Rectangle {
                        width: txtLinia.width + 10; height: txtLinia.height + 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.bottom; anchors.topMargin: 2
                        color: "white"
                        radius: 5
                        opacity: 0.9

                        Text {
                            id: txtLinia
                            anchors.centerIn: parent
                            text: model.tramLine
                            font.bold: true
                            font.pointSize: 12
                            color: "black"
                        }
                    }
                }
            }
        }
    }
    
    Column {
        anchors.right: parent.right; anchors.rightMargin: 15
        anchors.top: parent.top; anchors.topMargin: 15
        spacing: 10
        Button { text: "+"; font.pixelSize: 25; width: 40; height: 40; onClicked: map.zoomLevel += 0.5 }
        Button { text: "-"; font.pixelSize: 25; width: 40; height: 40; onClicked: map.zoomLevel -= 0.5 }
    }
}