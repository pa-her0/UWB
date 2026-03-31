import QtQuick 2.0
import QtQuick 2.2
import QtQuick.Controls 2.0
Item {
    id: root
    width: 250; height: 20
    visible: calibrationTextVisible
    property int imageIndex: 1

    Image {
        id: image
        width: 15
        height: 15
        anchors.top: parent.top
        anchors.left: parent.left
        source: "qrc:/icons/frame-"+imageIndex+".gif"
    }

    Text {
        id: text
        anchors.left: image.right
        anchors.leftMargin: 5
        anchors.verticalCenter: root.verticalCenter
        font.pixelSize: 15
        font.bold: true
        text: language?("第"+calibrationIndex+"次自标定解算中..."):("In the "+calibrationIndex+" self-calibration solution...")
    }

    Timer{
        id: animTimer
        interval: 50
        repeat: true
        running: true
        onTriggered: {
            if(imageIndex < 12)
            {
                imageIndex++
            }
            else
            {
                imageIndex = 1
            }
        }
    }

}
