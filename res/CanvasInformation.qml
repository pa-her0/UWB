import QtQuick 2.0
import QtQuick 2.2
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0


Item {
     property double radio: 0.8
     id: root
     width: 230*radio; height: 150*radio
     visible: canvasInfomationVisible
     signal resetSignal()

     Rectangle{
        id: contain
        anchors.fill: parent
        border.color: "black"
        anchors.margins: 5
        border.width: 0
        radius: 5
        Text {
            id: titleText
            text: language?"画布信息":"Information"
            font.pixelSize: language?22*radio:15*radio
            font.bold: true
            anchors.top: parent.top
            anchors.topMargin: 15*radio
            anchors.left: parent.left
            anchors.leftMargin: 10*radio
        }
        Rectangle{
            id: resetRec
            width: 80*radio
            height: 30*radio
            anchors.left: titleText.right
            anchors.leftMargin: language?30*radio:30*radio
            anchors.top: parent.top
            anchors.topMargin: 8*radio
            border.color: Qt.rgba(0,0,0,0.01)
            border.width: 1
            radius: 5*radio
            layer.enabled: true
            layer.effect: DropShadow {
                color: "#33000000"
                samples: 12
                radius: 4
                spread: 0.2
            }

            Image {
                id: resetImage
                width: 25*radio
                height: 25*radio
                anchors.left: parent.left
                anchors.leftMargin: 5*radio
                anchors.top: parent.top
                anchors.topMargin: 2.5*radio
                source: "qrc:/icons/reset.png"
            }

            Text{
                id: resetText
                font.pixelSize: language?20*radio:15*radio
                anchors.left: resetImage.right
                anchors.leftMargin: 5*radio
                anchors.top: parent.top
                anchors.topMargin: 5*radio
                text: language?"重置":"Reset"
            }

            MouseArea{
                anchors.fill: resetRec
                onClicked: {
                    resetSignal()
                }
            }

        }
        Rectangle{
                id:splitLine
                height: 1
                anchors.top:titleText.bottom
                anchors.topMargin: 15 * radio
                anchors.left:titleText.left
                anchors.leftMargin: 0
                anchors.right :resetRec.right
                anchors.rightMargin: 0
                color: "#CCCCCE"
        }
        Image {
            id: posImage
            width: 25*radio
            height: 25*radio
            source: "qrc:/icons/position.png"
            anchors.top: parent.top
            anchors.topMargin: 65*radio
            anchors.left: parent.left
            anchors.leftMargin: 20*radio
        }

        Text {
            id: posXText
            text: canvas_posX+","+canvas_posY
            font.pixelSize: 20*radio
            font.bold: true
            anchors.top: posImage.top
            anchors.left: posImage.right
            anchors.leftMargin: 10*radio
        }

        Image {
            id: angleImage
            width: 25*radio
            height: 25*radio
            source: "qrc:/icons/angle.png"
            anchors.top: parent.top
            anchors.topMargin: 105*radio
            anchors.left: parent.left
            anchors.leftMargin: 20*radio
        }

        Text {
            id: angleText
            text: canvas_rotate+"°"
            font.pixelSize: 20*radio
            font.bold: true
            anchors.top: angleImage.top
            anchors.topMargin: 2*radio
            anchors.left: angleImage.right
            anchors.leftMargin: 10*radio
        }

        Image {
            id: scaleImage
            width: 25*radio
            height: 25*radio
            source: "qrc:/icons/scale.png"
            anchors.top: parent.top
            anchors.topMargin: 105*radio
            anchors.left: angleImage.right
            anchors.leftMargin: 70*radio
        }

        Text {
            id: scaleText
            text: canvas_scale+"x"
            font.pixelSize: 20*radio
            font.bold: true
            anchors.top: scaleImage.top
            anchors.topMargin: 2*radio
            anchors.left: scaleImage.right
            anchors.leftMargin: 5*radio
        }


//        Text {
//            id: rotateText
//            text: qsTr("旋转:")+canvas_rotate+"°"
//            font.pixelSize: 15
//            anchors.top: posText.bottom
//            anchors.topMargin: 5
//            anchors.left: parent.left
//            anchors.leftMargin: 20
//        }

//        Text {
//            id: scaleText
//            text: qsTr("缩放:")+canvas_scale
//            font.pixelSize: 15
//            anchors.top: rotateText.bottom
//            anchors.topMargin: 5
//            anchors.left: parent.left
//            anchors.leftMargin: 20
//        }

     }
     DropShadow {
              anchors.fill: contain
              horizontalOffset:0
              verticalOffset:0
              radius:5
              spread:0.2
              samples:16
              color: "#C9C9C9C9"
              source:contain
    }

}
